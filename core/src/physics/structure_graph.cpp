// WP-20c: the structural load model (see ksp/physics/structure_graph.hpp). Engine glue in double; the direct factorisation is the spike's
// (core/spikes/jolt/src/structure_graph_direct.cpp), moved out of Jolt types and given the element sign convention of the header.
#include "ksp/physics/structure_graph.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace ksp::physics {

namespace {

using M6 = std::array<double, 36>;
using V6 = std::array<double, 6>;

M6 Zero6() {
    M6 m;
    m.fill(0.0);
    return m;
}

// [r]x, row-major
void Skew(const Vector3d& r, double R[3][3]) {
    R[0][0] = 0.0;
    R[0][1] = -r.z;
    R[0][2] = r.y;
    R[1][0] = r.z;
    R[1][1] = 0.0;
    R[1][2] = -r.x;
    R[2][0] = -r.y;
    R[2][1] = r.x;
    R[2][2] = 0.0;
}

// the node block of one element end at anchor offset r: kl [[I, -[r]x], [[r]x, |r|^2 I - r r^T]] + ka [[0, 0], [0, I]]
void AddDiag(M6& m, const Vector3d& r, double kl, double ka) {
    double R[3][3];
    Skew(r, R);
    const double rv[3] = {r.x, r.y, r.z};
    const double rr = rv[0] * rv[0] + rv[1] * rv[1] + rv[2] * rv[2];
    for (int i = 0; i < 3; ++i) {
        m[i * 6 + i] += kl;
        for (int j = 0; j < 3; ++j) {
            m[i * 6 + 3 + j] += -kl * R[i][j];
            m[(3 + i) * 6 + j] += kl * R[i][j];
            m[(3 + i) * 6 + 3 + j] += kl * ((i == j ? rr : 0.0) - rv[i] * rv[j]);
        }
        m[(3 + i) * 6 + 3 + i] += ka;
    }
}

// the coupling block K(a, b) of one element (rows: node a): -kl [[I, -[rb]x], [[ra]x, -[ra]x [rb]x]] - ka [[0, 0], [0, I]]
void AddCross(M6& m, const Vector3d& ra, const Vector3d& rb, double kl, double ka) {
    double Ra[3][3], Rb[3][3];
    Skew(ra, Ra);
    Skew(rb, Rb);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            double rr = 0.0;
            for (int t = 0; t < 3; ++t) rr += Ra[i][t] * Rb[t][j];
            m[i * 6 + j] += -kl * (i == j ? 1.0 : 0.0);
            m[i * 6 + 3 + j] += kl * Rb[i][j];
            m[(3 + i) * 6 + j] += -kl * Ra[i][j];
            m[(3 + i) * 6 + 3 + j] += kl * rr - (i == j ? ka : 0.0);
        }
}

M6 Transpose(const M6& a) {
    M6 t;
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j) t[i * 6 + j] = a[j * 6 + i];
    return t;
}

// lower Cholesky factor of a 6x6 SPD block; false when not positive definite
bool Chol6(const M6& a, M6& l) {
    l = Zero6();
    for (int j = 0; j < 6; ++j) {
        double d = a[j * 6 + j];
        for (int k = 0; k < j; ++k) d -= l[j * 6 + k] * l[j * 6 + k];
        if (!(d > 0.0)) return false;
        const double s = std::sqrt(d);
        l[j * 6 + j] = s;
        for (int i = j + 1; i < 6; ++i) {
            double v = a[i * 6 + j];
            for (int k = 0; k < j; ++k) v -= l[i * 6 + k] * l[j * 6 + k];
            l[i * 6 + j] = v / s;
        }
    }
    return true;
}

// X = A L^-T
M6 RightSolveLT(const M6& a, const M6& l) {
    M6 x = Zero6();
    for (int r = 0; r < 6; ++r)
        for (int j = 0; j < 6; ++j) {
            double v = a[r * 6 + j];
            for (int k = 0; k < j; ++k) v -= x[r * 6 + k] * l[j * 6 + k];
            x[r * 6 + j] = v / l[j * 6 + j];
        }
    return x;
}

// C -= A B^T
void SubABt(M6& c, const M6& a, const M6& b) {
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j) {
            double s = 0.0;
            for (int k = 0; k < 6; ++k) s += a[i * 6 + k] * b[j * 6 + k];
            c[i * 6 + j] -= s;
        }
}

// in-place inverse of a 6x6 matrix (Gauss-Jordan, partial pivoting); false when singular
bool Invert6(M6& m) {
    M6 inv = Zero6();
    for (int i = 0; i < 6; ++i) inv[i * 6 + i] = 1.0;
    double scale = 0.0;
    for (double v : m) scale = std::max(scale, std::fabs(v));
    if (!(scale > 0.0)) return false;
    for (int c = 0; c < 6; ++c) {
        int p = c;
        for (int r = c + 1; r < 6; ++r)
            if (std::fabs(m[r * 6 + c]) > std::fabs(m[p * 6 + c])) p = r;
        if (!(std::fabs(m[p * 6 + c]) > 1e-14 * scale)) return false;
        if (p != c)
            for (int k = 0; k < 6; ++k) {
                std::swap(m[p * 6 + k], m[c * 6 + k]);
                std::swap(inv[p * 6 + k], inv[c * 6 + k]);
            }
        const double d = 1.0 / m[c * 6 + c];
        for (int k = 0; k < 6; ++k) {
            m[c * 6 + k] *= d;
            inv[c * 6 + k] *= d;
        }
        for (int r = 0; r < 6; ++r)
            if (r != c) {
                const double f = m[r * 6 + c];
                if (f == 0.0) continue;
                for (int k = 0; k < 6; ++k) {
                    m[r * 6 + k] -= f * m[c * 6 + k];
                    inv[r * 6 + k] -= f * inv[c * 6 + k];
                }
            }
    }
    m = inv;
    return true;
}

Vector3d Mul3(const double I[3][3], const Vector3d& v) {
    return {I[0][0] * v.x + I[0][1] * v.y + I[0][2] * v.z, I[1][0] * v.x + I[1][1] * v.y + I[1][2] * v.z,
            I[2][0] * v.x + I[2][1] * v.y + I[2][2] * v.z};
}

}  // namespace

NodeDemand RigidPartDemand(double m, const Vector3d& x, const double I[3][3], const Vector3d& com, const Vector3d& a, const Vector3d& alpha,
                           const Vector3d& w, const Vector3d& g, const Vector3d& F, const Vector3d& M) {
    const Vector3d r = x - com;
    const Vector3d acc = a + Vector3d::Cross(alpha, r) + Vector3d::Cross(w, Vector3d::Cross(w, r)) - g;
    NodeDemand d;
    d.force = acc * m - F;
    d.moment = Mul3(I, alpha) + Vector3d::Cross(w, Mul3(I, w)) - M;
    return d;
}

void TreeSolve(const std::vector<int>& parentNode, const std::vector<int>& parentJoint, const std::vector<Vector3d>& nodeCom,
               const std::vector<NodeDemand>& demand, const std::vector<Vector3d>& jointCentroid, std::vector<JointResultant>& out) {
    const size_t n = parentNode.size();
    out.assign(jointCentroid.size(), JointResultant{});
    // depth of every node (roots 0); a cycle (not expected) is cut where it closes
    std::vector<int> depth(n, -1);
    std::vector<int> chain;
    for (size_t i = 0; i < n; ++i) {
        if (depth[i] >= 0) continue;
        chain.clear();
        int k = static_cast<int>(i);
        while (k >= 0 && depth[static_cast<size_t>(k)] < 0 && chain.size() <= n) {
            if (std::find(chain.begin(), chain.end(), k) != chain.end()) break;
            chain.push_back(k);
            k = parentNode[static_cast<size_t>(k)];
        }
        int d = (k >= 0 && depth[static_cast<size_t>(k)] >= 0) ? depth[static_cast<size_t>(k)] : -1;
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) depth[static_cast<size_t>(*it)] = ++d;
    }
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int x, int y) {
        if (depth[static_cast<size_t>(x)] != depth[static_cast<size_t>(y)]) return depth[static_cast<size_t>(x)] > depth[static_cast<size_t>(y)];
        return x > y;
    });
    // subtree sums, the moment about each node's own centre of mass (short lever arms)
    std::vector<Vector3d> sf(n), sm(n);
    for (size_t i = 0; i < n; ++i) {
        sf[i] = demand[i].force;
        sm[i] = demand[i].moment;
    }
    for (int i : order) {
        const int p = parentNode[static_cast<size_t>(i)];
        if (p < 0 || depth[static_cast<size_t>(p)] >= depth[static_cast<size_t>(i)]) continue;
        const size_t ui = static_cast<size_t>(i), up = static_cast<size_t>(p);
        const int j = parentJoint[ui];
        if (j >= 0 && static_cast<size_t>(j) < out.size()) {
            out[static_cast<size_t>(j)].force = sf[ui];
            out[static_cast<size_t>(j)].moment = sm[ui] + Vector3d::Cross(nodeCom[ui] - jointCentroid[static_cast<size_t>(j)], sf[ui]);
        }
        sf[up] += sf[ui];
        sm[up] += sm[ui] + Vector3d::Cross(nodeCom[ui] - nodeCom[up], sf[ui]);
    }
}

// ===================================================================================== JointSplit

bool JointSplit::Setup(const std::vector<Vector3d>& rel, const std::vector<double>& kLinear, const std::vector<double>& kAngular) {
    rel_ = rel;
    kl_ = kLinear;
    ka_ = kAngular;
    M6 k = Zero6();
    for (size_t e = 0; e < rel.size(); ++e) AddDiag(k, rel[e], kLinear[e], kAngular[e]);
    inv_ = k;
    return !rel.empty() && Invert6(inv_);
}

// u = K^-1 (F, M): the displacement of the loaded side; the element loads are kl (t + th x r) and ka th (see the header)
void JointSplit::Apply(const Vector3d& force, const Vector3d& moment, ElementLoad* out) const {
    const double b[6] = {force.x, force.y, force.z, moment.x, moment.y, moment.z};
    double u[6];
    for (int i = 0; i < 6; ++i) {
        double s = 0.0;
        for (int j = 0; j < 6; ++j) s += inv_[static_cast<size_t>(i * 6 + j)] * b[j];
        u[i] = s;
    }
    const Vector3d t(u[0], u[1], u[2]), th(u[3], u[4], u[5]);
    for (size_t e = 0; e < rel_.size(); ++e) {
        out[e].force = (t + Vector3d::Cross(th, rel_[e])) * kl_[e];
        out[e].torque = th * ka_[e];
    }
}

// ===================================================================================== StructuralGraph

bool StructuralGraph::Build(const std::vector<Vector3d>& nodeCom, const std::vector<double>& nodeMass, const std::vector<StructElement>& elements) {
    const size_t nn = nodeCom.size();
    elems_ = elements;
    factored_ = false;
    ra_.assign(elems_.size(), Vector3d());
    rb_.assign(elems_.size(), Vector3d());
    for (size_t e = 0; e < elems_.size(); ++e) {
        const StructElement& el = elems_[e];
        rb_[e] = el.anchor - nodeCom[static_cast<size_t>(el.b)];
        ra_[e] = el.a == StructElement::kGround ? Vector3d() : el.anchor - nodeCom[static_cast<size_t>(el.a)];
    }
    // ---- one fixed node per connected piece without ground (its heaviest node, the first on a tie); the ground is fixed itself
    std::vector<int> uf(nn + 1);
    std::iota(uf.begin(), uf.end(), 0);
    auto find = [&uf](int x) {
        while (uf[static_cast<size_t>(x)] != x) x = uf[static_cast<size_t>(x)] = uf[static_cast<size_t>(uf[static_cast<size_t>(x)])];
        return x;
    };
    const int groundIdx = static_cast<int>(nn);
    for (const StructElement& el : elems_) {
        const int a = el.a == StructElement::kGround ? groundIdx : el.a;
        uf[static_cast<size_t>(find(a))] = find(el.b);
    }
    std::vector<int> best(nn + 1, -1);
    for (size_t k = 0; k < nn; ++k) {
        const int r = find(static_cast<int>(k));
        if (best[static_cast<size_t>(r)] < 0 || nodeMass[k] > nodeMass[static_cast<size_t>(best[static_cast<size_t>(r)])])
            best[static_cast<size_t>(r)] = static_cast<int>(k);
    }
    std::vector<bool> fixed(nn, false);
    const int groundRoot = find(groundIdx);
    for (size_t r = 0; r < nn + 1; ++r)
        if (best[r] >= 0 && static_cast<int>(r) != groundRoot) fixed[static_cast<size_t>(best[r])] = true;
    // ---- greedy minimum-degree elimination order over the free nodes (symbolic factorisation on the node graph)
    std::vector<std::vector<int>> adj(nn);
    for (const StructElement& el : elems_)
        if (el.a != StructElement::kGround && el.a != el.b && !fixed[static_cast<size_t>(el.a)] && !fixed[static_cast<size_t>(el.b)]) {
            adj[static_cast<size_t>(el.a)].push_back(el.b);
            adj[static_cast<size_t>(el.b)].push_back(el.a);
        }
    for (auto& a : adj) {
        std::sort(a.begin(), a.end());
        a.erase(std::unique(a.begin(), a.end()), a.end());
    }
    size_t freeCount = 0;
    for (size_t k = 0; k < nn; ++k) freeCount += fixed[k] ? 0 : 1;
    std::vector<bool> done(nn, false);
    order_.clear();
    pos_.assign(nn, -1);
    std::vector<std::vector<int>> colNodes;
    colNodes.reserve(freeCount);
    while (order_.size() < freeCount) {
        int pick = -1;
        size_t pickDeg = SIZE_MAX;
        for (size_t k = 0; k < nn; ++k)
            if (!fixed[k] && !done[k] && adj[k].size() < pickDeg) {
                pick = static_cast<int>(k);
                pickDeg = adj[k].size();
            }
        done[static_cast<size_t>(pick)] = true;
        pos_[static_cast<size_t>(pick)] = static_cast<int>(order_.size());
        order_.push_back(pick);
        const std::vector<int> nb = adj[static_cast<size_t>(pick)];
        colNodes.push_back(nb);
        for (int a : nb) {  // the neighbours of an eliminated node become a clique (fill)
            auto& la = adj[static_cast<size_t>(a)];
            la.erase(std::remove(la.begin(), la.end(), pick), la.end());
            for (int b : nb)
                if (b != a) la.push_back(b);
            std::sort(la.begin(), la.end());
            la.erase(std::unique(la.begin(), la.end()), la.end());
        }
        adj[static_cast<size_t>(pick)].clear();
    }
    // ---- assemble K into the filled block structure (columns by elimination position, rows ascending)
    const size_t nf = order_.size();
    cols_.assign(nf, Col{});
    std::vector<M6> diag(nf, Zero6());
    lBlocks_ = 0;
    for (size_t k = 0; k < nf; ++k) {
        std::vector<int> rows;
        for (int node : colNodes[k]) rows.push_back(pos_[static_cast<size_t>(node)]);
        std::sort(rows.begin(), rows.end());
        cols_[k].rows = rows;
        cols_[k].L.assign(rows.size(), Zero6());
        lBlocks_ += rows.size();
    }
    auto block = [this](int r, int c) -> M6& {  // r > c (positions); exists by construction of the fill
        Col& col = cols_[static_cast<size_t>(c)];
        const auto it = std::lower_bound(col.rows.begin(), col.rows.end(), r);
        return col.L[static_cast<size_t>(it - col.rows.begin())];
    };
    for (size_t e = 0; e < elems_.size(); ++e) {
        const StructElement& el = elems_[e];
        const int pa = el.a == StructElement::kGround ? -1 : pos_[static_cast<size_t>(el.a)];
        const int pb = pos_[static_cast<size_t>(el.b)];
        if (pa >= 0) AddDiag(diag[static_cast<size_t>(pa)], ra_[e], el.kLinear, el.kAngular);
        if (pb >= 0) AddDiag(diag[static_cast<size_t>(pb)], rb_[e], el.kLinear, el.kAngular);
        if (pa >= 0 && pb >= 0 && pa != pb) {
            M6 kab = Zero6();
            AddCross(kab, ra_[e], rb_[e], el.kLinear, el.kAngular);  // block (a, b)
            M6& t = pa > pb ? block(pa, pb) : block(pb, pa);
            const M6 add = pa > pb ? kab : Transpose(kab);
            for (size_t i = 0; i < 36; ++i) t[i] += add[i];
        }
    }
    // ---- right-looking block Cholesky
    for (size_t k = 0; k < nf; ++k) {
        Col& c = cols_[k];
        if (!Chol6(diag[k], c.Lkk)) return false;
        for (M6& L : c.L) L = RightSolveLT(L, c.Lkk);
        for (size_t a = 0; a < c.rows.size(); ++a) {
            SubABt(diag[static_cast<size_t>(c.rows[a])], c.L[a], c.L[a]);
            for (size_t b = 0; b < a; ++b) SubABt(block(c.rows[a], c.rows[b]), c.L[a], c.L[b]);
        }
    }
    u_.assign(nn, V6{});
    factored_ = true;
    return true;
}

void StructuralGraph::Solve(const std::vector<NodeDemand>& demand, std::vector<ElementLoad>& out) {
    const size_t nf = order_.size();
    std::vector<V6>& b = rhs_;
    b.assign(nf, V6{});
    for (size_t k = 0; k < pos_.size(); ++k) {
        const int p = pos_[k];
        if (p < 0) continue;
        const NodeDemand& d = demand[k];
        V6& bp = b[static_cast<size_t>(p)];
        bp[0] = -d.force.x;
        bp[1] = -d.force.y;
        bp[2] = -d.force.z;
        bp[3] = -d.moment.x;
        bp[4] = -d.moment.y;
        bp[5] = -d.moment.z;
    }
    for (size_t k = 0; k < nf; ++k) {  // L y = b
        const Col& c = cols_[k];
        V6& y = b[k];
        for (int i = 0; i < 6; ++i) {
            double v = y[static_cast<size_t>(i)];
            for (int t = 0; t < i; ++t) v -= c.Lkk[static_cast<size_t>(i * 6 + t)] * y[static_cast<size_t>(t)];
            y[static_cast<size_t>(i)] = v / c.Lkk[static_cast<size_t>(i * 6 + i)];
        }
        for (size_t a = 0; a < c.rows.size(); ++a) {
            V6& bj = b[static_cast<size_t>(c.rows[a])];
            const M6& L = c.L[a];
            for (int i = 0; i < 6; ++i) {
                double s = 0.0;
                for (int t = 0; t < 6; ++t) s += L[static_cast<size_t>(i * 6 + t)] * y[static_cast<size_t>(t)];
                bj[static_cast<size_t>(i)] -= s;
            }
        }
    }
    for (size_t k = nf; k-- > 0;) {  // L^T x = y
        const Col& c = cols_[k];
        V6& x = b[k];
        for (size_t a = 0; a < c.rows.size(); ++a) {
            const V6& xj = b[static_cast<size_t>(c.rows[a])];
            const M6& L = c.L[a];
            for (int i = 0; i < 6; ++i) {
                double s = 0.0;
                for (int t = 0; t < 6; ++t) s += L[static_cast<size_t>(t * 6 + i)] * xj[static_cast<size_t>(t)];
                x[static_cast<size_t>(i)] -= s;
            }
        }
        for (int i = 6; i-- > 0;) {
            double v = x[static_cast<size_t>(i)];
            for (int t = i + 1; t < 6; ++t) v -= c.Lkk[static_cast<size_t>(t * 6 + i)] * x[static_cast<size_t>(t)];
            x[static_cast<size_t>(i)] = v / c.Lkk[static_cast<size_t>(i * 6 + i)];
        }
    }
    for (V6& u : u_) u.fill(0.0);
    for (size_t k = 0; k < nf; ++k) u_[static_cast<size_t>(order_[k])] = b[k];
    // element loads on the b node: f = -kl s, t = -ka (th_b - th_a)
    out.resize(elems_.size());
    for (size_t e = 0; e < elems_.size(); ++e) {
        const StructElement& el = elems_[e];
        const V6& ub = u_[static_cast<size_t>(el.b)];
        const Vector3d tb(ub[0], ub[1], ub[2]), thb(ub[3], ub[4], ub[5]);
        Vector3d ta, tha;
        if (el.a != StructElement::kGround) {
            const V6& ua = u_[static_cast<size_t>(el.a)];
            ta = Vector3d(ua[0], ua[1], ua[2]);
            tha = Vector3d(ua[3], ua[4], ua[5]);
        }
        const Vector3d s = tb + Vector3d::Cross(thb, rb_[e]) - ta - Vector3d::Cross(tha, ra_[e]);
        out[e].force = s * -el.kLinear;
        out[e].torque = (thb - tha) * -el.kAngular;
    }
}

JointResultant StructuralGraph::Resultant(const std::vector<StructElement>& elements, const std::vector<ElementLoad>& loads, int joint,
                                          const Vector3d& centroid) {
    JointResultant r;
    for (size_t e = 0; e < elements.size(); ++e) {
        if (elements[e].joint != joint) continue;
        r.force += loads[e].force;
        r.moment += loads[e].torque + Vector3d::Cross(elements[e].anchor - centroid, loads[e].force);
    }
    return r;
}

}  // namespace ksp::physics
