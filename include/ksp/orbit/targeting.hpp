// Targeting (Assembly-CSharp/Targeting.cs, 1,112 lines): conic geometry and the closest-approach intercept search
// that Orbit._FindClosestPoints (Orbit.cs:1385) runs for PatchedConics.CalculatePatch, OrbitTargeter and
// OrbitUtil.CalculateIntersections. Ported operation for operation in double precision (System.Math via
// ksp::dotnet::Math).
//
// Object pools. KSP borrows Conic / Sample / Interval / PairOfIntervals objects from Smooth.Pools stacks whose
// Reset hooks clear nothing (PairOfIntervals clears its lists). The port keeps the one pool effect that can change a
// result: Interval objects are released twice when an interval list has 2 or 3 entries (subdivide_intervals adds the
// same interval twice, find_search_intervals / ReleaseContent then release it twice), and two later Borrow() calls
// in the same search can then return the same object. Targeting::Workspace reproduces that with a LIFO Interval pool
// per search. Stale fields of reused objects (Sample info of invalid samples, Interval minimum/maximum without a
// root) are never read on any path, so samples and conics are plain objects owned by the Workspace.
//
// Deviation: Interval.FindRoot and find_region_edge loop forever in KSP when a NaN reaches the bisection; here they
// throw std::runtime_error after 10,000 iterations (the bisection needs ~30). Index errors (an interval list with
// fewer than two entries in subdivide_intervals) throw std::out_of_range where C# throws ArgumentOutOfRangeException.
#pragma once

#include <deque>
#include <memory>
#include <vector>

#include "ksp/math/vector3d.hpp"

namespace ksp {

class Orbit;

struct Targeting {
    struct Plane;

    // Targeting.cs:8
    struct Ray {
        Vector3d p;
        Vector3d v;
        Ray() = default;
        Ray(const Vector3d& p_, const Vector3d& v_) : p(p_), v(v_) {}
        Vector3d ClosestPoint(const Vector3d& point, bool forward = false) const;  // Targeting.cs:20
        Vector3d ClosestPoint(const Ray& ray, bool forward = false) const;      // Targeting.cs:30
        Ray Project(const Plane& plane) const;                                  // Targeting.cs:45
        Vector3d Point(double t) const;                                         // Targeting.cs:52
    };

    // Targeting.cs:63
    struct Plane {
        Vector3d p;
        Vector3d n;
        Plane() = default;
        Plane(const Vector3d& p_, const Vector3d& n_) : p(p_), n(n_) {}
        Vector3d Intersept(const Ray& ray) const;  // Targeting.cs:75 (sic)
    };

    // Targeting.cs:92
    struct Frame {
        Vector3d p;
        Vector3d t;
        Vector3d n;
        Frame() = default;
        Frame(const Vector3d& p_, const Vector3d& t_, const Vector3d& n_) : p(p_), t(t_), n(n_) {}
    };

    // Targeting.cs:108 (a conic in the world-like orbit frame, focus at the origin)
    struct Conic {
        double l = 0.0;
        double e = 0.0;
        double a = 0.0;
        double b = 0.0;
        double pe = 0.0;
        Vector3d pe_point;
        double ap = 0.0;
        Vector3d ap_point;
        Vector3d X;
        Vector3d Y;
        Vector3d N;
        Vector3d focus;
        Vector3d center;
        Plane conic_plane;
        double min_v = 0.0;
        double max_v = 0.0;

        static Conic Borrow(const Orbit& o);                                 // Targeting.cs:162
        double Radius(double v) const;                                       // Targeting.cs:203
        Vector3d Point(double v) const;                                      // Targeting.cs:208
        double Curvature(double v) const;                                    // Targeting.cs:215
        Vector3d Tangent(double v) const;                                    // Targeting.cs:223
        void true_anomaly_xy(const Vector3d& p, double& c, double& s) const;  // Targeting.cs:230
        double TrueAnomaly(const Vector3d& p) const;                         // Targeting.cs:237
        void tangent_xy(const Vector3d& p, double& vx, double& vy) const;    // Targeting.cs:243
        Vector3d Tangent(const Vector3d& p) const;                           // Targeting.cs:258
        Vector3d Normal(double v) const;                                     // Targeting.cs:264
        Vector3d Normal(const Vector3d& p) const;                            // Targeting.cs:270
        Frame GetFrame(const Vector3d& p) const;                             // Targeting.cs:276
        int Intercepts(Ray line, Vector3d& i1, Vector3d& i2) const;          // Targeting.cs:284
        bool point_is_real(const Vector3d& p) const;                         // Targeting.cs:356
        bool point_inside(const Vector3d& p) const;                          // Targeting.cs:365
    };

    class Workspace;

    // Targeting.cs:386
    struct Sample {
        // Targeting.cs:388
        struct Info {
            Vector3d d;
            double cos0 = 0.0;
            double cos1 = 0.0;
            double dot0 = 0.0;
            double dot1 = 0.0;
            double tangent_dot = 0.0;
            void Set(const Frame& f1, const Frame& f2);  // Targeting.cs:402
        };

        const Conic* orbit1 = nullptr;
        const Conic* orbit2 = nullptr;
        double v = 0.0;
        bool region_edge = false;
        bool valid = false;
        bool minimum = false;
        Frame src;
        Frame tgt1;
        Frame tgt2;
        Info info1;
        Info info2;
        int tgt_index = 0;

        const Frame& tgt() const { return GetTgt(tgt_index); }     // Targeting.cs:441
        const Info& info() const { return GetInfo(tgt_index); }    // Targeting.cs:443
        const Info& GetInfo(int index) const { return index <= 0 ? info1 : info2; }  // Targeting.cs:502
        const Frame& GetTgt(int index) const { return index <= 0 ? tgt1 : tgt2; }    // Targeting.cs:511
        // Targeting.cs:520
        static int get_normal_intercepts(const Conic& o1, const Vector3d& p1, const Conic& o2, Vector3d& i1, Vector3d& i2);
        // Targeting.cs:533 (returns the intercept count even when the frames were zeroed for non-real points, sic)
        static int get_normal_intercept_frames(const Conic& o1, const Vector3d& p1, const Conic& o2, Frame& f1, Frame& f2);
    };

    // Targeting.cs:644
    struct Interval {
        Sample* s1 = nullptr;
        Sample* s2 = nullptr;
        int tgt_index = 0;
        Sample::Info info1;
        Sample::Info info2;
        bool void_region = false;
        bool has_root = false;
        bool minimum = false;
        bool maximum = false;
        Interval* prev = nullptr;
        Interval* next = nullptr;
        static constexpr double epsilon = 1E-08;
        static constexpr double golden = 0.6180339887498949;

        void Link(Interval* prev_, Interval* next_);                              // Targeting.cs:712
        std::vector<Interval*> Subdivide(Workspace& ws);                          // Targeting.cs:726
        Sample* FindRoot(Workspace& ws);                                          // Targeting.cs:791
        static Sample* find_region_edge(Workspace& ws, const Conic& orbit1, const Conic& orbit2, double v1,
                                        double v2);                               // Targeting.cs:867
    };

    // Owns every Sample and Interval of one search (KSP's pools; see the header comment). Results returned by
    // Intercepts point into the Workspace and stay valid while it lives.
    class Workspace {
    public:
        Workspace() = default;
        Workspace(const Workspace&) = delete;
        Workspace& operator=(const Workspace&) = delete;
        Sample* BorrowSample(const Conic& o1, const Conic& o2, double v);       // Sample.Borrow, Targeting.cs:472
        Interval* BorrowInterval(Sample* s1, Sample* s2, int tgt_index);        // Interval.Borrow, Targeting.cs:691
        void ReleaseInterval(Interval* ival);                                   // Interval.Release (LIFO pool)

    private:
        std::deque<Sample> samples_;
        std::vector<std::unique_ptr<Interval>> intervals_;
        std::vector<Interval*> freeIntervals_;
    };

    // Targeting.cs:1091 Intercepts(orbit1, orbit2, samples): closest-approach candidates of orbit2 seen from orbit1.
    static std::vector<Sample*> Intercepts(const Conic& orbit1, const Conic& orbit2, int samples, Workspace& ws);
};

}  // namespace ksp
