// Jolt spike: the direct method of StructuralGraph (structure_graph.hpp): K is constant in the frame of a rigid cluster, so it is
// assembled and factored once per structural change (sparse block Cholesky over 6x6 node blocks, greedy minimum-degree order)
// and every tick costs two block-triangular solves.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>

#include "structure_graph.hpp"

namespace spike {

using JPH::DVec3;
using JPH::RVec3;
using JPH::Vec3;

namespace {

using M6 = std::array<double, 36>;

#ifdef JPH_DOUBLE_PRECISION
DVec3 D(const RVec3& v) { return DVec3(v); }
#endif
DVec3 D(const Vec3& v) { return DVec3(v); }

M6 Zero6() {
    M6 m;
    m.fill(0.0);
    return m;
}

// [r]x (row-major)
void Skew(const DVec3& r, double R[3][3]) {
    const double x = r.GetX(), y = r.GetY(), z = r.GetZ();
    R[0][0] = 0;
    R[0][1] = -z;
    R[0][2] = y;
    R[1][0] = z;
    R[1][1] = 0;
    R[1][2] = -x;
    R[2][0] = -y;
    R[2][1] = x;
    R[2][2] = 0;
}

// node block of one element end: k_lin [[I, -[r]x], [[r]x, |r|^2 I - r r^T]] + k_ang [[0, 0], [0, I]]
void AddDiag(M6& m, const DVec3& r, double kl, double ka) {
    double R[3][3];
    Skew(r, R);
    const double rv[3] = {r.GetX(), r.GetY(), r.GetZ()};
    const double rr = rv[0] * rv[0] + rv[1] * rv[1] + rv[2] * rv[2];
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

// coupling block K_ab of one element: -k_lin [[I, -Rb], [Ra, -Ra Rb]] - k_ang [[0, 0], [0, I]]
void AddCross(M6& m, const DVec3& ra, const DVec3& rb, double kl, double ka) {
    double Ra[3][3], Rb[3][3];
    Skew(ra, Ra);
    Skew(rb, Rb);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            double rr = 0.0;
            for (int t = 0; t < 3; t++) rr += Ra[i][t] * Rb[t][j];
            m[i * 6 + j] += -kl * (i == j ? 1.0 : 0.0);
            m[i * 6 + 3 + j] += kl * Rb[i][j];
            m[(3 + i) * 6 + j] += -kl * Ra[i][j];
            m[(3 + i) * 6 + 3 + j] += kl * rr - (i == j ? ka : 0.0);
        }
}

M6 Transpose(const M6& a) {
    M6 t;
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 6; j++) t[i * 6 + j] = a[j * 6 + i];
    return t;
}

bool Chol6(const M6& a, M6& l) {
    l = Zero6();
    for (int j = 0; j < 6; j++) {
        double d = a[j * 6 + j];
        for (int k = 0; k < j; k++) d -= l[j * 6 + k] * l[j * 6 + k];
        if (!(d > 0.0)) return false;
        const double s = std::sqrt(d);
        l[j * 6 + j] = s;
        for (int i = j + 1; i < 6; i++) {
            double v = a[i * 6 + j];
            for (int k = 0; k < j; k++) v -= l[i * 6 + k] * l[j * 6 + k];
            l[i * 6 + j] = v / s;
        }
    }
    return true;
}

// X = A * L^-T
M6 RightSolveLT(const M6& a, const M6& l) {
    M6 x = Zero6();
    for (int r = 0; r < 6; r++)
        for (int j = 0; j < 6; j++) {
            double v = a[r * 6 + j];
            for (int k = 0; k < j; k++) v -= x[r * 6 + k] * l[j * 6 + k];
            x[r * 6 + j] = v / l[j * 6 + j];
        }
    return x;
}

// C -= A * B^T
void SubABt(M6& c, const M6& a, const M6& b) {
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 6; j++) {
            double s = 0.0;
            for (int k = 0; k < 6; k++) s += a[i * 6 + k] * b[j * 6 + k];
            c[i * 6 + j] -= s;
        }
}

}  // namespace

StructuralGraph::Stats StructuralGraph::Factor(const std::vector<RVec3>& partCom, const std::vector<std::array<RVec3, 3>>& subAnchors,
                                               JPH::QuatArg clusterRot) {
    const auto t0 = std::chrono::steady_clock::now();
    Stats st;
    const size_t nn = partOfNode_.size();
    factorRot_ = clusterRot;
    anchorF_.resize(elems_.size());
    for (size_t i = 0; i < elems_.size(); i++) {
        Elem& e = elems_[i];
        const DVec3 p = D(subAnchors[e.joint][e.sub]);
        anchorF_[i] = p;
        e.ra = p - D(partCom[partOfNode_[e.a]]);
        e.rb = p - D(partCom[partOfNode_[e.b]]);
    }
    // ---- greedy minimum-degree elimination order over the free nodes (symbolic factorisation on the node graph)
    std::vector<std::vector<int>> adj(nn);
    for (const Elem& e : elems_)
        if (!fixed_[e.a] && !fixed_[e.b] && e.a != e.b) {
            adj[e.a].push_back(e.b);
            adj[e.b].push_back(e.a);
        }
    for (auto& a : adj) {
        std::sort(a.begin(), a.end());
        a.erase(std::unique(a.begin(), a.end()), a.end());
    }
    std::vector<bool> done(nn, false);
    order_.clear();
    pos_.assign(nn, -1);
    std::vector<std::vector<int>> colNodes;
    size_t freeCount = 0;
    for (size_t k = 0; k < nn; k++) freeCount += fixed_[k] ? 0 : 1;
    while (order_.size() < freeCount) {
        int best = -1;
        size_t bestDeg = SIZE_MAX;
        for (size_t k = 0; k < nn; k++)
            if (!fixed_[k] && !done[k] && adj[k].size() < bestDeg) {
                best = static_cast<int>(k);
                bestDeg = adj[k].size();
            }
        done[best] = true;
        pos_[best] = static_cast<int>(order_.size());
        order_.push_back(best);
        const std::vector<int> nb = adj[best];
        colNodes.push_back(nb);
        for (int a : nb) {  // the neighbours of an eliminated node become a clique (fill)
            auto& la = adj[a];
            la.erase(std::remove(la.begin(), la.end(), best), la.end());
            for (int b : nb)
                if (b != a) la.push_back(b);
            std::sort(la.begin(), la.end());
            la.erase(std::unique(la.begin(), la.end()), la.end());
        }
        adj[best].clear();
    }
    // ---- assemble K into the filled block structure (columns by elimination position, rows sorted)
    const size_t nf = order_.size();
    cols_.assign(nf, Col{});
    std::vector<M6> diag(nf, Zero6());
    lBlocks_ = 0;
    for (size_t k = 0; k < nf; k++) {
        std::vector<int> rows;
        for (int node : colNodes[k]) rows.push_back(pos_[node]);
        std::sort(rows.begin(), rows.end());
        cols_[k].rows = rows;
        cols_[k].L.assign(rows.size(), Zero6());
        lBlocks_ += rows.size();
    }
    auto block = [&](int r, int c) -> M6& {  // r > c (positions); exists by construction of the fill
        Col& col = cols_[c];
        const auto it = std::lower_bound(col.rows.begin(), col.rows.end(), r);
        return col.L[static_cast<size_t>(it - col.rows.begin())];
    };
    for (const Elem& e : elems_) {
        const int pa = pos_[e.a], pb = pos_[e.b];
        if (pa >= 0) AddDiag(diag[pa], e.ra, e.kLin, e.kAng);
        if (pb >= 0) AddDiag(diag[pb], e.rb, e.kLin, e.kAng);
        if (pa >= 0 && pb >= 0 && pa != pb) {
            M6 kab = Zero6();
            AddCross(kab, e.ra, e.rb, e.kLin, e.kAng);  // block (a, b)
            M6& t = pa > pb ? block(pa, pb) : block(pb, pa);
            const M6 add = pa > pb ? kab : Transpose(kab);
            for (int i = 0; i < 36; i++) t[i] += add[i];
        }
    }
    // ---- right-looking block Cholesky
    for (size_t k = 0; k < nf; k++) {
        Col& c = cols_[k];
        if (!Chol6(diag[k], c.Lkk)) {
            factored_ = false;
            st.iterations = -1;
            st.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
            return st;
        }
        for (M6& L : c.L) L = RightSolveLT(L, c.Lkk);
        for (size_t a = 0; a < c.rows.size(); a++) {
            SubABt(diag[c.rows[a]], c.L[a], c.L[a]);
            for (size_t b = 0; b < a; b++) SubABt(block(c.rows[a], c.rows[b]), c.L[a], c.L[b]);
        }
    }
    factored_ = true;
    u_.assign(nn, V6{});
    st.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return st;
}

StructuralGraph::Stats StructuralGraph::SolveFactored(const std::vector<RVec3>& partCom, const std::vector<Vec3>& demandF,
                                                      const std::vector<Vec3>& demandM, JPH::QuatArg clusterRot) {
    const auto t0 = std::chrono::steady_clock::now();
    Stats st;
    const CraftModel& craft = *craft_;
    const size_t nf = order_.size();
    // demands into the frame of the factorisation: q = factorRot * clusterRot^-1
    const JPH::Quat q = factorRot_ * clusterRot.Conjugated();
    std::vector<V6> b(nf, V6{});
    for (size_t i = 0; i < craft.parts.size(); i++) {
        const int k = nodeOfPart_[i];
        const int p = pos_[k];
        if (p < 0) continue;  // the fixed node takes the reaction (zero for self-equilibrated demands)
        const Vec3 m = demandM[i] + Vec3(partCom[i] - partCom[partOfNode_[k]]).Cross(demandF[i]);
        const Vec3 fF = q * demandF[i], mF = q * m;
        for (int c = 0; c < 3; c++) {
            b[p][c] -= fF[c];
            b[p][3 + c] -= mF[c];
        }
    }
    for (size_t k = 0; k < nf; k++) {  // L y = b
        const Col& c = cols_[k];
        V6& y = b[k];
        for (int i = 0; i < 6; i++) {
            double v = y[i];
            for (int t = 0; t < i; t++) v -= c.Lkk[i * 6 + t] * y[t];
            y[i] = v / c.Lkk[i * 6 + i];
        }
        for (size_t a = 0; a < c.rows.size(); a++) {
            V6& bj = b[c.rows[a]];
            const M6& L = c.L[a];
            for (int i = 0; i < 6; i++) {
                double s = 0.0;
                for (int t = 0; t < 6; t++) s += L[i * 6 + t] * y[t];
                bj[i] -= s;
            }
        }
    }
    for (size_t k = nf; k-- > 0;) {  // L^T x = y
        const Col& c = cols_[k];
        V6& x = b[k];
        for (size_t a = 0; a < c.rows.size(); a++) {
            const V6& xj = b[c.rows[a]];
            const M6& L = c.L[a];
            for (int i = 0; i < 6; i++) {
                double s = 0.0;
                for (int t = 0; t < 6; t++) s += L[t * 6 + i] * xj[t];
                x[i] -= s;
            }
        }
        for (int i = 6; i-- > 0;) {
            double v = x[i];
            for (int t = i + 1; t < 6; t++) v -= c.Lkk[t * 6 + i] * x[t];
            x[i] = v / c.Lkk[i * 6 + i];
        }
    }
    for (V6& u : u_) u.fill(0.0);
    for (size_t k = 0; k < nf; k++) u_[order_[k]] = b[k];
    ElementLoads(nullptr);
    st.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return st;
}

void StructuralGraph::ElementLoads(const std::vector<std::array<RVec3, 3>>* subAnchorsWorld) {
    const CraftModel& craft = *craft_;
    std::vector<DVec3> F(craft.joints.size(), DVec3::sZero()), T(craft.joints.size(), DVec3::sZero());
    std::vector<DVec3> C(craft.joints.size(), DVec3::sZero());
    std::vector<int> cnt(craft.joints.size(), 0);
    auto anchor = [&](size_t i) { return subAnchorsWorld ? D((*subAnchorsWorld)[elems_[i].joint][elems_[i].sub]) : anchorF_[i]; };
    for (size_t i = 0; i < elems_.size(); i++) {
        C[elems_[i].joint] += anchor(i);
        cnt[elems_[i].joint]++;
    }
    for (size_t j = 0; j < C.size(); j++)
        if (cnt[j]) C[j] /= static_cast<double>(cnt[j]);
    for (size_t i = 0; i < elems_.size(); i++) {
        const Elem& e = elems_[i];
        const V6& ua = u_[e.a];
        const V6& ub = u_[e.b];
        const DVec3 xa(ua[0], ua[1], ua[2]), ta(ua[3], ua[4], ua[5]);
        const DVec3 xb(ub[0], ub[1], ub[2]), tb(ub[3], ub[4], ub[5]);
        const DVec3 f = (xb + tb.Cross(e.rb) - xa - ta.Cross(e.ra)) * e.kLin;
        const DVec3 m = (tb - ta) * e.kAng;
        F[e.joint] += f;
        T[e.joint] += m + (anchor(i) - C[e.joint]).Cross(f);
    }
    for (size_t j = 0; j < F.size(); j++)
        jointLoads[j] = cnt[j] ? std::make_pair(static_cast<float>(F[j].Length()), static_cast<float>(T[j].Length())) : std::make_pair(-1.0f, -1.0f);
}

}  // namespace spike
