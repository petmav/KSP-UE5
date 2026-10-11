#include "structure_graph.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>

namespace spike {

using JPH::DVec3;
using JPH::RVec3;
using JPH::Vec3;

namespace {

#ifdef JPH_DOUBLE_PRECISION
DVec3 D(const RVec3& v) { return DVec3(v); }  // RVec3 is DVec3
#endif
DVec3 D(const Vec3& v) { return DVec3(v); }

// in-place inverse of a 6x6 SPD matrix (Gauss-Jordan with partial pivoting); returns false when singular
bool Invert6(std::array<double, 36>& m) {
    std::array<double, 36> inv{};
    for (int i = 0; i < 6; i++) inv[i * 6 + i] = 1.0;
    for (int c = 0; c < 6; c++) {
        int p = c;
        for (int r = c + 1; r < 6; r++)
            if (std::abs(m[r * 6 + c]) > std::abs(m[p * 6 + c])) p = r;
        if (std::abs(m[p * 6 + c]) < 1e-300) return false;
        if (p != c)
            for (int k = 0; k < 6; k++) {
                std::swap(m[p * 6 + k], m[c * 6 + k]);
                std::swap(inv[p * 6 + k], inv[c * 6 + k]);
            }
        const double d = 1.0 / m[c * 6 + c];
        for (int k = 0; k < 6; k++) {
            m[c * 6 + k] *= d;
            inv[c * 6 + k] *= d;
        }
        for (int r = 0; r < 6; r++)
            if (r != c) {
                const double f = m[r * 6 + c];
                if (f == 0.0) continue;
                for (int k = 0; k < 6; k++) {
                    m[r * 6 + k] -= f * m[c * 6 + k];
                    inv[r * 6 + k] -= f * inv[c * 6 + k];
                }
            }
    }
    m = inv;
    return true;
}

// node block of one element end at anchor offset r: k_lin [[I, -[r]x], [[r]x, |r|^2 I - r r^T]] + k_ang [[0, 0], [0, I]]
void AddBlock(std::array<double, 36>& m, const DVec3& r, double kl, double ka) {
    const double x = r.GetX(), y = r.GetY(), z = r.GetZ();
    const double R[3][3] = {{0, -z, y}, {z, 0, -x}, {-y, x, 0}};  // [r]x
    const double rr = x * x + y * y + z * z;
    const double rv[3] = {x, y, z};
    for (int i = 0; i < 3; i++) {
        m[i * 6 + i] += kl;
        for (int j = 0; j < 3; j++) {
            m[i * 6 + 3 + j] += -kl * R[i][j];
            m[(3 + i) * 6 + j] += kl * R[i][j];
            m[(3 + i) * 6 + 3 + j] += kl * ((i == j ? rr : 0.0) - rv[i] * rv[j]);
        }
        m[(3 + i) * 6 + 3 + i] += ka;
    }
}

}  // namespace

void StructuralGraph::Build(const CraftModel& craft, const std::vector<bool>& live, double kLinear, const std::vector<int>& groundParts) {
    craft_ = &craft;
    const size_t n = craft.parts.size();
    nodeOfPart_.assign(n, -1);
    partOfNode_.clear();
    for (size_t i = 0; i < n; i++)
        if (craft.parts[i].physical) {
            nodeOfPart_[i] = static_cast<int>(partOfNode_.size());
            partOfNode_.push_back(static_cast<int>(i));
        }
    for (size_t i = 0; i < n; i++) nodeOfPart_[i] = nodeOfPart_[craft.parts[i].rbOwner];
    ground_ = groundParts.empty() ? -1 : static_cast<int>(partOfNode_.size());
    nn_ = partOfNode_.size() + (ground_ >= 0 ? 1 : 0);
    elems_.clear();
    for (size_t j = 0; j < craft.joints.size(); j++) {
        if (!live[j]) continue;
        const JointSpec& js = craft.joints[j];
        const int a = nodeOfPart_[js.target], b = nodeOfPart_[js.host];
        if (a == b) continue;
        for (int s = 0; s < js.internalJoints; s++) {
            Elem e;
            e.a = a;
            e.b = b;
            e.joint = static_cast<int>(j);
            e.sub = s;
            e.kLin = kLinear;
            e.kAng = js.rigid ? kLinear : static_cast<double>(js.stiffness);
            elems_.push_back(e);
        }
    }
    // contacts with static geometry: point supports (linear spring, no moment) from the part to the fixed ground node, so the
    // split of the support reaction between several touching parts follows the stiffness too
    for (int g : groundParts) {
        Elem e;
        e.a = ground_;
        e.b = nodeOfPart_[g];
        e.part = g;
        e.kLin = kLinear;
        e.kAng = 0.0;
        elems_.push_back(e);
    }
    // one fixed node per connected piece: the ground where the piece touches it, else its heaviest part
    std::vector<int> uf(nn_);
    std::iota(uf.begin(), uf.end(), 0);
    auto find = [&](int x) {
        while (uf[x] != x) x = uf[x] = uf[uf[x]];
        return x;
    };
    for (const Elem& e : elems_) uf[find(e.a)] = find(e.b);
    std::vector<int> best(nn_, -1);
    for (size_t k = 0; k < partOfNode_.size(); k++) {
        const int r = find(static_cast<int>(k));
        if (best[r] < 0 || craft.parts[partOfNode_[k]].rbMass > craft.parts[partOfNode_[best[r]]].rbMass) best[r] = static_cast<int>(k);
    }
    if (ground_ >= 0) best[find(ground_)] = ground_;
    fixed_.assign(nn_, false);
    for (size_t k = 0; k < nn_; k++)
        if (best[k] >= 0) fixed_[best[k]] = true;
    u_.assign(nn_, V6{});
    jointLoads.assign(craft.joints.size(), {-1.0f, -1.0f});
    factored_ = false;
}

void StructuralGraph::Geometry(const std::vector<RVec3>& partCom, const std::vector<std::array<RVec3, 3>>& subAnchors) {
    anchorF_.resize(elems_.size());
    for (size_t i = 0; i < elems_.size(); i++) {
        Elem& e = elems_[i];
        const DVec3 p = e.joint >= 0 ? D(subAnchors[e.joint][e.sub]) : D(partCom[e.part]);
        anchorF_[i] = p;
        e.ra = e.a == ground_ ? DVec3::sZero() : p - D(partCom[partOfNode_[e.a]]);
        e.rb = p - D(partCom[partOfNode_[e.b]]);
    }
}

void StructuralGraph::Apply(const std::vector<V6>& u, std::vector<V6>& out) const {
    for (V6& o : out) o.fill(0.0);
    for (const Elem& e : elems_) {
        const V6& ua = u[e.a];
        const V6& ub = u[e.b];
        const DVec3 xa(ua[0], ua[1], ua[2]), ta(ua[3], ua[4], ua[5]);
        const DVec3 xb(ub[0], ub[1], ub[2]), tb(ub[3], ub[4], ub[5]);
        const DVec3 dl = xb + tb.Cross(e.rb) - xa - ta.Cross(e.ra);
        const DVec3 F = dl * e.kLin;
        const DVec3 M = (tb - ta) * e.kAng;
        const DVec3 mb = e.rb.Cross(F) + M, ma = e.ra.Cross(F) + M;
        V6& ob = out[e.b];
        V6& oa = out[e.a];
        for (int k = 0; k < 3; k++) {
            ob[k] += F[k];
            ob[3 + k] += mb[k];
            oa[k] -= F[k];
            oa[3 + k] -= ma[k];
        }
    }
    for (size_t k = 0; k < out.size(); k++)
        if (fixed_[k]) out[k].fill(0.0);
}

StructuralGraph::Stats StructuralGraph::Solve(const std::vector<RVec3>& partCom, const std::vector<std::array<RVec3, 3>>& subAnchors,
                                              const std::vector<Vec3>& demandF, const std::vector<Vec3>& demandM, int maxIter, double tol) {
    const auto t0 = std::chrono::steady_clock::now();
    Stats st;
    const CraftModel& craft = *craft_;
    const size_t nn = nn_;
    Geometry(partCom, subAnchors);
    // right-hand side: -demand, physicsless parts folded into their owner node (moment carried to the owner's CoM)
    std::vector<V6> rhs(nn, V6{});
    for (size_t i = 0; i < craft.parts.size(); i++) {
        const int k = nodeOfPart_[i];
        const DVec3 f = D(demandF[i]);
        const DVec3 m = D(demandM[i]) + (D(partCom[i]) - D(partCom[partOfNode_[k]])).Cross(f);
        for (int c = 0; c < 3; c++) {
            rhs[k][c] -= f[c];
            rhs[k][3 + c] -= m[c];
        }
    }
    for (size_t k = 0; k < nn; k++)
        if (fixed_[k]) rhs[k].fill(0.0);
    // block-Jacobi preconditioner
    std::vector<std::array<double, 36>> P(nn);
    for (auto& b : P) b.fill(0.0);
    for (const Elem& e : elems_) {
        AddBlock(P[e.a], e.ra, e.kLin, e.kAng);
        AddBlock(P[e.b], e.rb, e.kLin, e.kAng);
    }
    for (size_t k = 0; k < nn; k++) {
        if (fixed_[k] || !Invert6(P[k])) {
            P[k].fill(0.0);
            if (!fixed_[k])
                for (int c = 0; c < 6; c++) P[k][c * 6 + c] = 1.0;
        }
    }
    auto precond = [&](const std::vector<V6>& r, std::vector<V6>& z) {
        for (size_t k = 0; k < nn; k++)
            for (int i = 0; i < 6; i++) {
                double s = 0.0;
                for (int j = 0; j < 6; j++) s += P[k][i * 6 + j] * r[k][j];
                z[k][i] = s;
            }
    };
    auto dot = [&](const std::vector<V6>& a, const std::vector<V6>& b) {
        double s = 0.0;
        for (size_t k = 0; k < nn; k++)
            for (int i = 0; i < 6; i++) s += a[k][i] * b[k][i];
        return s;
    };
    // PCG, warm start
    std::vector<V6> r(nn), z(nn), p(nn), Ap(nn);
    for (size_t k = 0; k < nn; k++)
        if (fixed_[k]) u_[k].fill(0.0);
    Apply(u_, Ap);
    for (size_t k = 0; k < nn; k++)
        for (int i = 0; i < 6; i++) r[k][i] = rhs[k][i] - Ap[k][i];
    const double bnorm = std::sqrt(std::max(dot(rhs, rhs), 1e-300));
    precond(r, z);
    p = z;
    double rz = dot(r, z);
    int it = 0;
    double rel = std::sqrt(dot(r, r)) / bnorm;
    for (; it < maxIter && rel > tol; it++) {
        Apply(p, Ap);
        const double pAp = dot(p, Ap);
        if (!(pAp > 0.0)) break;
        const double alpha = rz / pAp;
        for (size_t k = 0; k < nn; k++)
            for (int i = 0; i < 6; i++) {
                u_[k][i] += alpha * p[k][i];
                r[k][i] -= alpha * Ap[k][i];
            }
        rel = std::sqrt(dot(r, r)) / bnorm;
        precond(r, z);
        const double rz2 = dot(r, z);
        const double beta = rz2 / rz;
        rz = rz2;
        for (size_t k = 0; k < nn; k++)
            for (int i = 0; i < 6; i++) p[k][i] = z[k][i] + beta * p[k][i];
    }
    st.iterations = it;
    st.residual = rel;
    ElementLoads();
    st.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return st;
}

}  // namespace spike
