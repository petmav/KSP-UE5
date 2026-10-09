// Targeting (Assembly-CSharp/Targeting.cs), operation-for-operation port; see include/ksp/orbit/targeting.hpp for
// the pool emulation and the two deviations (NaN loop guard, std::out_of_range).
#include "ksp/orbit/targeting.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "ksp/dotnet/math.hpp"
#include "ksp/orbit/orbit.hpp"

namespace ksp {

namespace M = dotnet::Math;

namespace {

constexpr double kPI = 3.141592653589793;
constexpr int kLoopGuard = 10000;

// Targeting.PairOfIntervals (Targeting.cs:590): two interval lists, indexed 0 / 1.
struct PairOfIntervals {
    std::vector<Targeting::Interval*> lists[2];
    // C# List indexer: ArgumentOutOfRangeException -> std::out_of_range
    static Targeting::Interval* At(const std::vector<Targeting::Interval*>& l, int i) {
        return l.at(static_cast<size_t>(i < 0 ? std::numeric_limits<size_t>::max() : static_cast<size_t>(i)));
    }
};

}  // namespace

// ---- Ray / Plane -----------------------------------------------------------------------------------------------------

// Targeting.cs:20
Vector3d Targeting::Ray::ClosestPoint(const Vector3d& point, bool forward) const {
    const double num = Vector3d::Dot(point - p, v) / v.sqrMagnitude();
    if (forward && num < 0.0) return p;
    return p + v * num;
}

// Targeting.cs:30
Vector3d Targeting::Ray::ClosestPoint(const Ray& ray, bool forward) const {
    const Vector3d rhs = v - ray.v * Vector3d::Dot(v, ray.v) / ray.v.sqrMagnitude();
    if (rhs.sqrMagnitude() == 0.0) return p;
    const double num = Vector3d::Dot(ray.p - p, rhs) / Vector3d::Dot(v, rhs);
    if (forward && num < 0.0) return p;
    return p + v * num;
}

// Targeting.cs:45
Targeting::Ray Targeting::Ray::Project(const Plane& plane) const {
    const Vector3d vector3d = p - plane.n * Vector3d::Dot(p - plane.p, plane.n) / plane.n.sqrMagnitude();
    const Vector3d vector3d2 = v - plane.n * Vector3d::Dot(v, plane.n) / plane.n.sqrMagnitude();
    return Ray(vector3d, vector3d2);
}

// Targeting.cs:52
Vector3d Targeting::Ray::Point(double t) const { return p + t * v; }

// Targeting.cs:75
Vector3d Targeting::Plane::Intersept(const Ray& ray) const {
    const double num = Vector3d::Dot(p - ray.p, n);
    const double num2 = Vector3d::Dot(ray.v, n);
    if (num2 == 0.0) return ray.p;
    return ray.p + ray.v * num / num2;
}

// ---- Conic -----------------------------------------------------------------------------------------------------------

// Targeting.cs:162
Targeting::Conic Targeting::Conic::Borrow(const Orbit& o) {
    Conic conic;
    conic.l = o.semiLatusRectum();
    conic.e = o.eccentricity;
    conic.X = o.OrbitFrame.X;
    conic.Y = o.OrbitFrame.Y;
    conic.N = o.OrbitFrame.Z;
    conic.focus = Vector3d::zero();
    conic.conic_plane = Plane(conic.focus, conic.N);
    if (conic.e != 1.0) {
        conic.a = conic.l / (1.0 - conic.e * conic.e);
        conic.b = conic.l / M::Sqrt(M::Abs(1.0 - conic.e * conic.e));
        conic.center = -conic.X * conic.l * conic.e / (1.0 - conic.e * conic.e);
    } else {
        conic.center = conic.X * conic.l / 2.0;
        const double num = std::numeric_limits<double>::infinity();
        conic.b = std::numeric_limits<double>::infinity();
        conic.a = num;
    }
    conic.pe = conic.l / (1.0 + conic.e);
    conic.pe_point = conic.X * conic.pe;
    if (conic.e < 1.0) {
        conic.ap = conic.l / (1.0 - conic.e);
        conic.ap_point = (0.0 - conic.ap) * conic.X;
        conic.max_v = kPI;
    } else {
        conic.ap = std::numeric_limits<double>::infinity();
        conic.ap_point = (0.0 - conic.ap) * conic.X;
        conic.max_v = M::Acos(-1.0 / conic.e);
    }
    conic.min_v = 0.0 - conic.max_v;
    return conic;
}

// Targeting.cs:203
double Targeting::Conic::Radius(double v) const { return l / (1.0 + e * M::Cos(v)); }

// Targeting.cs:208
Vector3d Targeting::Conic::Point(double v) const {
    const double num = M::Cos(v);
    const double num2 = M::Sin(v);
    return l / (1.0 + e * num) * (num * X + num2 * Y);
}

// Targeting.cs:215
double Targeting::Conic::Curvature(double v) const {
    const double num = M::Cos(v);
    const double num2 = 1.0 + e * num;
    const double num3 = 1.0 + 2.0 * num * num + e * e;
    return M::Pow(num2 * num2 / num3, 1.5) / l;
}

// Targeting.cs:223
Vector3d Targeting::Conic::Tangent(double v) const {
    const double num = 0.0 - M::Sin(v);
    const double num2 = M::Cos(v) + e;
    return num * X + num2 * Y;
}

// Targeting.cs:230
void Targeting::Conic::true_anomaly_xy(const Vector3d& p, double& c, double& s) const {
    const Vector3d normalized = p.normalized();
    c = Vector3d::Dot(normalized, X);
    s = Vector3d::Dot(normalized, Y);
}

// Targeting.cs:237
double Targeting::Conic::TrueAnomaly(const Vector3d& p) const {
    double c, s;
    true_anomaly_xy(p, c, s);
    return M::Atan2(s, c);
}

// Targeting.cs:243
void Targeting::Conic::tangent_xy(const Vector3d& p, double& vx, double& vy) const {
    const Vector3d normalized = p.normalized();
    if (e > 1.0 && Vector3d::Dot(p - center, X) > 0.0) {
        vx = Vector3d::Dot(normalized, Y);
        vy = 0.0 - Vector3d::Dot(normalized, X) + e;
    } else {
        vx = 0.0 - Vector3d::Dot(normalized, Y);
        vy = Vector3d::Dot(normalized, X) + e;
    }
}

// Targeting.cs:258
Vector3d Targeting::Conic::Tangent(const Vector3d& p) const {
    double vx, vy;
    tangent_xy(p, vx, vy);
    return vx * X + vy * Y;
}

// Targeting.cs:264
Vector3d Targeting::Conic::Normal(double v) const {
    const double num = 0.0 - M::Sin(v);
    return (M::Cos(v) + e) * X - num * Y;
}

// Targeting.cs:270
Vector3d Targeting::Conic::Normal(const Vector3d& p) const {
    double vx, vy;
    tangent_xy(p, vx, vy);
    return vy * X - vx * Y;
}

// Targeting.cs:276
Targeting::Frame Targeting::Conic::GetFrame(const Vector3d& p) const {
    double vx, vy;
    tangent_xy(p, vx, vy);
    const Vector3d t = vx * X + vy * Y;
    const Vector3d n = vy * X - vx * Y;
    return Frame(p, t, n);
}

// Targeting.cs:284
int Targeting::Conic::Intercepts(Ray line, Vector3d& i1, Vector3d& i2) const {
    line = line.Project(conic_plane);
    const Vector3d lhs = line.p - center;
    const double num = Vector3d::Dot(lhs, X);
    const double num2 = Vector3d::Dot(lhs, Y);
    const double num3 = Vector3d::Dot(line.v, X);
    const double num4 = Vector3d::Dot(line.v, Y);
    double num5;
    double num6;
    if (e == 1.0) {
        if (num4 == 0.0) {
            num5 = (0.0 - (num2 * num2 + 2.0 * l * num)) / (2.0 * l * num3);
            num6 = 0.0;
        } else {
            const double num7 = num4 * num4;
            num5 = (0.0 - (l * num3 + num2 * num4)) / num7;
            num6 = (l * l * num3 * num3 + 2.0 * l * (num2 * num3 * num4 - num * num4 * num4)) / (num7 * num7);
        }
    } else {
        const double num8 = a * a;
        const double num9 = b * b;
        double num10;
        double num11;
        double num12;
        if (e < 1.0) {
            num10 = num9 * num3 * num3 + num8 * num4 * num4;
            num11 = num9 * num * num3 + num8 * num2 * num4;
            num12 = num9 * num * num + num8 * num2 * num2 - num8 * num9;
        } else {
            num10 = num9 * num3 * num3 - num8 * num4 * num4;
            num11 = num9 * num * num3 - num8 * num2 * num4;
            num12 = num9 * num * num - num8 * num2 * num2 - num8 * num9;
        }
        if (num10 == 0.0) {
            num6 = 0.0;
            num5 = (0.0 - num12) / (2.0 * num11);
        } else {
            num6 = num11 * num11 / (num10 * num10) - num12 / num10;
            num5 = (0.0 - num11) / num10;
        }
    }
    if (num6 < 0.0) {
        i1 = Vector3d::zero();
        i2 = Vector3d::zero();
        return 0;
    }
    if (num6 > 0.0) {
        num6 = M::Sqrt(num6);
        i1 = line.Point(num5 + num6);
        i2 = line.Point(num5 - num6);
        return 2;
    }
    i1 = line.Point(num5);
    i2 = Vector3d::zero();
    return 1;
}

// Targeting.cs:356
bool Targeting::Conic::point_is_real(const Vector3d& p) const {
    if (e <= 1.0) return true;
    return Vector3d::Dot(p - center, X) < 0.0;
}

// Targeting.cs:365
bool Targeting::Conic::point_inside(const Vector3d& p) const {
    const Vector3d lhs = p - center;
    const double num = Vector3d::Dot(lhs, X);
    const double num2 = Vector3d::Dot(lhs, Y);
    if (e < 1.0) return num * num / (a * a) + num2 * num2 / (b * b) <= 1.0;
    if (e > 1.0) {
        if (num < 0.0) return num * num / (a * a) - num2 * num2 / (b * b) >= 1.0;
        return false;
    }
    return num2 * num2 + 2.0 * l * num < 0.0;
}

// ---- Sample ----------------------------------------------------------------------------------------------------------

// Targeting.cs:402
void Targeting::Sample::Info::Set(const Frame& f1, const Frame& f2) {
    d = f2.p - f1.p;
    cos0 = Vector3d::Dot(f1.t.normalized(), d.normalized());
    cos1 = Vector3d::Dot(f2.t.normalized(), d.normalized());
    dot0 = Vector3d::Dot(f1.t.normalized(), d);
    dot1 = Vector3d::Dot(f2.t.normalized(), d);
    tangent_dot = Vector3d::Dot(f1.t, f2.t);
}

// Targeting.cs:520
int Targeting::Sample::get_normal_intercepts(const Conic& o1, const Vector3d& p1, const Conic& o2, Vector3d& i1,
                                             Vector3d& i2) {
    const Frame frame = o1.GetFrame(p1);
    Vector3d p2 = o2.conic_plane.Intersept(Ray(p1, frame.n));
    Vector3d vector3d = Vector3d::Cross(frame.t, o2.N);
    if (vector3d.sqrMagnitude() == 0.0) {
        p2 = p1;
        vector3d = frame.n;
    }
    return o2.Intercepts(Ray(p2, vector3d), i1, i2);
}

// Targeting.cs:533 (the decompiled goto structure, flattened)
int Targeting::Sample::get_normal_intercept_frames(const Conic& o1, const Vector3d& p1, const Conic& o2, Frame& f1,
                                                   Frame& f2) {
    Vector3d i;
    Vector3d i2;
    const int num = get_normal_intercepts(o1, p1, o2, i, i2);
    bool single = false;  // IL_0080: f1 = f2 = o2.GetFrame(i)
    switch (num) {
        case 1:
            single = o2.point_is_real(i);
            break;
        case 2:
            if (!o2.point_is_real(i2)) {
                single = o2.point_is_real(i);
            } else {
                if (o2.point_is_real(i)) {
                    f1 = o2.GetFrame(i);
                    f2 = o2.GetFrame(i2);
                    return num;
                }
                i = i2;
                single = true;
            }
            break;
        default:
            break;
    }
    if (single) {
        f1 = f2 = o2.GetFrame(i);
    } else {
        f1 = Frame(Vector3d::zero(), Vector3d::zero(), Vector3d::zero());
        f2 = Frame(Vector3d::zero(), Vector3d::zero(), Vector3d::zero());
    }
    return num;
}

// ---- Workspace (pools) -----------------------------------------------------------------------------------------------

// Targeting.cs:472 Sample.Borrow
Targeting::Sample* Targeting::Workspace::BorrowSample(const Conic& o1, const Conic& o2, double v) {
    Sample* sample = &samples_.emplace_back();
    sample->orbit1 = &o1;
    sample->orbit2 = &o2;
    sample->v = v;
    sample->region_edge = false;
    sample->tgt_index = -1;
    if (o1.e >= 1.0 && (v <= o1.min_v || v >= o1.max_v)) {
        sample->valid = false;
        return sample;
    }
    const Vector3d vector3d = o1.Point(v);
    sample->src = o1.GetFrame(vector3d);
    const int num = Sample::get_normal_intercept_frames(o1, vector3d, o2, sample->tgt1, sample->tgt2);
    sample->valid = num != 0;
    if (sample->valid) {
        sample->info1.Set(sample->src, sample->tgt1);
        sample->info2.Set(sample->src, sample->tgt2);
    }
    return sample;
}

// Targeting.cs:691 Interval.Borrow (Smooth.Pools stack: reuse the most recently released object)
Targeting::Interval* Targeting::Workspace::BorrowInterval(Sample* s1, Sample* s2, int tgt_index) {
    Interval* interval;
    if (!freeIntervals_.empty()) {
        interval = freeIntervals_.back();
        freeIntervals_.pop_back();
    } else {
        intervals_.push_back(std::make_unique<Interval>());
        interval = intervals_.back().get();
    }
    interval->s1 = s1;
    interval->s2 = s2;
    interval->tgt_index = tgt_index;
    interval->info1 = s1->GetInfo(tgt_index);
    interval->info2 = s2->GetInfo(tgt_index);
    interval->void_region = s1->region_edge && s2->region_edge;
    interval->has_root = !interval->void_region && interval->info1.dot1 * interval->info2.dot1 <= 0.0;
    if (interval->has_root) {
        const double num = interval->info1.dot1 * interval->info1.tangent_dot;
        const double num2 = interval->info2.dot1 * interval->info2.tangent_dot;
        interval->maximum = num >= 0.0 && num2 <= 0.0;
        interval->minimum = num <= 0.0 && num2 >= 0.0;
    }
    interval->prev = interval->next = nullptr;
    return interval;
}

// Interval.Release (Targeting.cs:686): Pool.Release pushes without checking for duplicates
void Targeting::Workspace::ReleaseInterval(Interval* ival) { freeIntervals_.push_back(ival); }

// ---- Interval --------------------------------------------------------------------------------------------------------

// Targeting.cs:712
void Targeting::Interval::Link(Interval* prev_, Interval* next_) {
    prev = prev_;
    next = next_;
    if (prev_ != nullptr) prev_->next = this;
    if (next_ != nullptr) next_->prev = this;
}

// Targeting.cs:726
std::vector<Targeting::Interval*> Targeting::Interval::Subdivide(Workspace& ws) {
    double num = s2->v - s1->v;
    if (num < 0.0) num += kPI * 2.0;
    double num2 = s1->v + num * 0.6180339887498949 * 0.6180339887498949;
    double num3 = s1->v + num * 0.6180339887498949;
    if (num2 >= kPI) num2 -= kPI;
    if (num3 >= kPI) num3 -= kPI;
    const Conic& orbit = *s1->orbit1;
    const Conic& orbit2 = *s1->orbit2;
    Sample* sample = ws.BorrowSample(orbit, orbit2, num2);
    Sample* sample2 = ws.BorrowSample(orbit, orbit2, num3);
    std::vector<Interval*> list;
    if (!sample->valid && !sample2->valid) {
        sample = find_region_edge(ws, orbit, orbit2, s1->v, num2);
        sample2 = find_region_edge(ws, orbit, orbit2, num3, s2->v);
        list.push_back(ws.BorrowInterval(s1, sample, tgt_index));
        list.push_back(ws.BorrowInterval(sample, sample2, tgt_index));
        list.push_back(ws.BorrowInterval(sample2, s2, tgt_index));
    } else if (!sample->valid && sample2->valid) {
        Sample* sample3 = find_region_edge(ws, orbit, orbit2, s1->v, num2);
        sample = find_region_edge(ws, orbit, orbit2, num2, num3);
        list.push_back(ws.BorrowInterval(s1, sample3, tgt_index));
        list.push_back(ws.BorrowInterval(sample3, sample, tgt_index));
        list.push_back(ws.BorrowInterval(sample, sample2, tgt_index));
        list.push_back(ws.BorrowInterval(sample2, s2, tgt_index));
    } else if (sample->valid && !sample2->valid) {
        sample2 = find_region_edge(ws, orbit, orbit2, num2, num3);
        Sample* sample3 = find_region_edge(ws, orbit, orbit2, num3, s2->v);
        list.push_back(ws.BorrowInterval(s1, sample, tgt_index));
        list.push_back(ws.BorrowInterval(sample, sample2, tgt_index));
        list.push_back(ws.BorrowInterval(sample2, sample3, tgt_index));
        list.push_back(ws.BorrowInterval(sample3, s2, tgt_index));
    } else {
        list.push_back(ws.BorrowInterval(s1, sample, tgt_index));
        list.push_back(ws.BorrowInterval(sample, sample2, tgt_index));
        list.push_back(ws.BorrowInterval(sample2, s2, tgt_index));
    }
    Interval* interval = prev;
    size_t i;
    for (i = 0; i < list.size() - 1; i++) {
        list[i]->Link(interval, list[i + 1]);
        interval = list[i];
    }
    list[i]->Link(interval, next);
    return list;
}

// Targeting.cs:791 (bisection on dot1 * tangent_dot; minimum selects the side)
Targeting::Sample* Targeting::Interval::FindRoot(Workspace& ws) {
    Sample* sample = s1;
    Sample* sample2 = s2;
    double num = sample->v;
    double num2 = sample2->v;
    const Conic& orbit = *sample->orbit1;
    const Conic& orbit2 = *sample->orbit2;
    if (num2 < num) num2 += kPI * 2.0;
    Sample* sample3;
    for (int guard = 0;; ++guard) {
        if (guard >= kLoopGuard) throw std::runtime_error("Targeting.Interval.FindRoot: no progress (NaN; KSP loops forever)");
        const double num3 = (num + num2) / 2.0;
        sample3 = ws.BorrowSample(orbit, orbit2, num3);
        if (sample3->valid) {
            const Sample::Info& info = sample3->GetInfo(tgt_index);
            const double num4 = info.dot1 * info.tangent_dot;
            if (num2 - num < 1E-08 || num4 == 0.0) break;
            if (num4 < 0.0) {
                if (minimum) {
                    sample = sample3;
                    num = num3;
                } else {
                    sample2 = sample3;
                    num2 = num3;
                }
            } else if (num4 > 0.0) {
                if (minimum) {
                    sample2 = sample3;
                    num2 = num3;
                } else {
                    sample = sample3;
                    num = num3;
                }
            }
            continue;
        }
        Sample* sample4 = find_region_edge(ws, orbit, orbit2, num, num3);
        Sample* sample5 = find_region_edge(ws, orbit, orbit2, num3, num2);
        Interval* interval = ws.BorrowInterval(sample, sample4, tgt_index);
        Interval* interval2 = ws.BorrowInterval(sample5, sample2, tgt_index);
        if (interval->has_root) {
            Sample* result = interval->FindRoot(ws);
            ws.ReleaseInterval(interval);
            return result;
        }
        if (interval2->has_root) {
            Sample* result2 = interval2->FindRoot(ws);
            ws.ReleaseInterval(interval2);
            return result2;
        }
        return nullptr;
    }
    sample3->tgt_index = tgt_index;
    sample3->minimum = minimum;
    return sample3;
}

// Targeting.cs:867
Targeting::Sample* Targeting::Interval::find_region_edge(Workspace& ws, const Conic& orbit1, const Conic& orbit2, double v1,
                                                         double v2) {
    if (v2 < v1) v2 += kPI * 2.0;
    Sample* sample = ws.BorrowSample(orbit1, orbit2, v1);
    Sample* sample2 = ws.BorrowSample(orbit1, orbit2, v2);
    Sample* sample3;
    for (int guard = 0;; ++guard) {
        if (guard >= kLoopGuard) throw std::runtime_error("Targeting.find_region_edge: no progress (NaN; KSP loops forever)");
        const double num = (v1 + v2) / 2.0;
        if (v2 - v1 < 1E-08) break;
        sample3 = ws.BorrowSample(orbit1, orbit2, num);
        if (!sample->valid) {
            if (sample3->valid) {
                v2 = num;
                sample2 = sample3;
            } else {
                v1 = num;
                sample = sample3;
            }
        } else if (sample3->valid) {
            v1 = num;
            sample = sample3;
        } else {
            v2 = num;
            sample2 = sample3;
        }
    }
    sample3 = (!sample2->valid) ? sample : sample2;
    sample3->region_edge = true;
    return sample3;
}

// ---- the search ------------------------------------------------------------------------------------------------------

namespace {

using Interval = Targeting::Interval;
using Sample = Targeting::Sample;
using Conic = Targeting::Conic;
using Workspace = Targeting::Workspace;

// Targeting.cs:914
double sample_v(int index, int count, const Conic& o) {
    return static_cast<double>(2 * index - count) * o.max_v / static_cast<double>(count);
}

// Targeting.cs:919
void add_interval(Workspace& ws, PairOfIntervals& intervals, Sample* start, Sample* end) {
    intervals.lists[0].push_back(ws.BorrowInterval(start, end, 0));
    intervals.lists[1].push_back(ws.BorrowInterval(start, end, 1));
}

// Targeting.cs:925
Sample* check_void_transition(Workspace& ws, PairOfIntervals& intervals, Sample* prev, Sample* samp, Sample* ival_start) {
    if (prev != nullptr && prev->valid != samp->valid) {
        Sample* sample = Interval::find_region_edge(ws, *prev->orbit1, *prev->orbit2, prev->v, samp->v);
        if (ival_start != nullptr) add_interval(ws, intervals, ival_start, sample);
        ival_start = sample;
    }
    return ival_start;
}

// Targeting.cs:939
PairOfIntervals sample_orbits(Workspace& ws, const Conic& orbit1, const Conic& orbit2, int count) {
    PairOfIntervals pairOfIntervals;
    Sample* sample = nullptr;
    Sample* sample2 = nullptr;
    Sample* sample3 = nullptr;
    Sample* sample4 = nullptr;
    Sample* sample5 = nullptr;
    const int count2 = count;
    if (orbit1.e >= 1.0) count++;
    for (int i = 0; i < count; i++) {
        const double v = sample_v(i, count2, orbit1);
        Sample* sample6 = ws.BorrowSample(orbit1, orbit2, v);
        if (sample6->valid) {
            if (sample3 == nullptr) sample3 = sample6;
            sample = check_void_transition(ws, pairOfIntervals, sample5, sample6, sample);
            sample2 = sample6;
        } else {
            sample = check_void_transition(ws, pairOfIntervals, sample5, sample6, sample);
            sample2 = nullptr;
        }
        if (sample != nullptr && sample2 != nullptr) add_interval(ws, pairOfIntervals, sample, sample2);
        if (sample2 != nullptr) sample = sample2;
        sample2 = nullptr;
        sample5 = sample6;
        if (i == 0) sample4 = sample5;
    }
    if (orbit1.e < 1.0) {
        if ((sample5->valid && !sample4->valid) || (!sample5->valid && sample4->valid)) {
            Sample* sample7 = Interval::find_region_edge(ws, orbit1, orbit2, sample5->v, sample4->v);
            if (sample != nullptr) add_interval(ws, pairOfIntervals, sample, sample7);
            sample = sample7;
        }
        if (sample != nullptr && sample3 != nullptr) add_interval(ws, pairOfIntervals, sample, sample3);
    }
    return pairOfIntervals;
}

// Targeting.cs:1004
PairOfIntervals find_search_intervals(Workspace& ws, const PairOfIntervals& ivals) {
    PairOfIntervals pairOfIntervals;
    for (int i = 0; i < 2; i++) {
        for (Interval* interval : ivals.lists[i]) {
            if (interval->has_root) pairOfIntervals.lists[i].push_back(interval);
            else ws.ReleaseInterval(interval);
        }
    }
    return pairOfIntervals;
}

// Targeting.cs:1026
void add_crossing_subdivisions(Workspace& ws, std::vector<Interval*>& intervals, Interval* ival, bool reversed) {
    const Vector3d p = ival->s1->src.p;
    const Vector3d p2 = ival->s2->src.p;
    if (ival->s1->orbit2->point_inside(p) == ival->s1->orbit2->point_inside(p2)) {
        intervals.push_back(ival);
        return;
    }
    std::vector<Interval*> list = ival->Subdivide(ws);
    if (reversed) {
        size_t i;
        for (i = 0; i < list.size() - 1; i++) intervals.push_back(list[i]);
        add_crossing_subdivisions(ws, intervals, list[i], reversed);
    } else {
        add_crossing_subdivisions(ws, intervals, list[0], reversed);
        for (size_t i = 1; i < list.size(); i++) intervals.push_back(list[i]);
    }
}

// Targeting.cs:1056 (with 2 or 3 intervals the first/last interval is visited twice, sic)
PairOfIntervals subdivide_intervals(Workspace& ws, const PairOfIntervals& ivals) {
    PairOfIntervals pairOfIntervals;
    const int num = 0;
    const int num2 = static_cast<int>(ivals.lists[0].size()) - 1;
    const int num3 = static_cast<int>(ivals.lists[0].size()) / 2 - 1;
    const int num4 = num3 + 1;
    for (int i = 0; i < 2; i++) {
        const std::vector<Interval*>& src = ivals.lists[i];
        std::vector<Interval*>& dst = pairOfIntervals.lists[i];
        add_crossing_subdivisions(ws, dst, PairOfIntervals::At(src, num), false);
        for (int j = num + 1; j < num3; j++) dst.push_back(PairOfIntervals::At(src, j));
        add_crossing_subdivisions(ws, dst, PairOfIntervals::At(src, num3), false);
        add_crossing_subdivisions(ws, dst, PairOfIntervals::At(src, num4), true);
        for (int k = num4 + 1; k < num2; k++) dst.push_back(PairOfIntervals::At(src, k));
        add_crossing_subdivisions(ws, dst, PairOfIntervals::At(src, num2), true);
        for (Interval* interval : src) {
            if (std::find(dst.begin(), dst.end(), interval) == dst.end()) ws.ReleaseInterval(interval);
        }
    }
    return pairOfIntervals;
}

}  // namespace

// Targeting.cs:1091
std::vector<Targeting::Sample*> Targeting::Intercepts(const Conic& orbit1, const Conic& orbit2, int samples, Workspace& ws) {
    std::vector<Sample*> list;
    PairOfIntervals ivals = sample_orbits(ws, orbit1, orbit2, samples);
    ivals = subdivide_intervals(ws, ivals);
    ivals = find_search_intervals(ws, ivals);
    for (int i = 0; i < 2; i++) {
        for (int num = static_cast<int>(ivals.lists[i].size()) - 1; num >= 0; num--) {
            Sample* sample = ivals.lists[i][static_cast<size_t>(num)]->FindRoot(ws);
            if (sample != nullptr) list.push_back(sample);
        }
    }
    // ivals.ReleaseContent(): every interval of both lists goes back to the pool (twice if listed twice)
    for (int i = 0; i < 2; i++) {
        for (Interval* interval : ivals.lists[i]) ws.ReleaseInterval(interval);
    }
    return list;
}

}  // namespace ksp
