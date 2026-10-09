// math_shim candidate: mirrors harness/src/KspGolden/Generators/MathGen.cs input-for-input (same loops, same row
// numbering; the in-game replica HarnessCapture.cs MathShimReplica is the same code), evaluated with kspcore's Mathf /
// Vector3f / Quaternionf / Vector3d / QuaternionD / UtilMath / CelestialFrame. Float inputs computed by the generator
// itself (x / 3f, x + 1e-6f) follow Mono's float rule like everything else. Gates: f32 table vs the in-game capture
// harness/captures/out/math_shim_mono.json, f64 table vs harness/golden/math_shim (runtime-independent).
#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "datasets.hpp"
#include "golden_json.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

using namespace ksp;

namespace {

struct Builder {
    golden::Table& t;
    std::map<std::string, int64_t> counter;
    template <class T>
    void Add(const std::string& fn, std::initializer_list<T> a, std::initializer_list<T> r) {
        int64_t& i = counter[fn];
        std::vector<golden::Cell> row(12);
        row[0] = fn;
        row[1] = i++;
        int k = 0;
        for (T v : a) row[static_cast<size_t>(2 + k++)] = golden::Cell(v);
        k = 0;
        for (T v : r) row[static_cast<size_t>(8 + k++)] = golden::Cell(v);
        t.Add(std::move(row));
    }
};

golden::Table MakeTable(const char* name, const char* desc, golden::ColType ct) {
    golden::Table t(name, desc);
    t.Col("fn", golden::ColType::Str).Col("i", golden::ColType::I64);
    for (int k = 0; k < 6; ++k) t.Col("a" + std::to_string(k), ct);
    for (int k = 0; k < 4; ++k) t.Col("r" + std::to_string(k), ct);
    t.key = {"fn", "i"};
    return t;
}

}  // namespace

static int RunMathShim(const RunnerOptions& opt) {
    golden::File g;
    g.dataset = "math_shim";
    g.description = "kspcore candidate for math_shim (MathGen.cs inputs, kspcore implementations).";
    g.tables.push_back(MakeTable("f32", "Unity float math.", golden::ColType::F32));
    g.tables.push_back(MakeTable("f64", "KSP double math.", golden::ColType::F64));
    Builder b32{g.tables[0], {}};
    Builder b64{g.tables[1], {}};

    // MathGen.cs: static readonly float[] F
    const float F[] = {0.0f, -0.0f, 0.5f, -0.5f, 1.5f, 2.5f, -2.5f, 1e-7f, 3.1415927f, -7.25f, 123.456f, 359.9f, 720.5f,
                       -1000.75f, 1e20f};
    auto B = [](bool v) { return v ? 1.0f : 0.0f; };
    for (float x : F) {
        b32.Add<float>("Mathf.Round", {x}, {Mathf::Round(x)});
        b32.Add<float>("Mathf.RoundToInt", {x}, {static_cast<float>(Mathf::RoundToInt(std::fabs(x) < 1e9f ? x : 0.0f))});
        b32.Add<float>("Mathf.Sign", {x}, {Mathf::Sign(x)});
        b32.Add<float>("Mathf.Clamp01", {x}, {Mathf::Clamp01(x)});
        b32.Add<float>("Mathf.Sqrt", {x}, {Mathf::Sqrt(x)});
        b32.Add<float>("Mathf.Sin", {x}, {Mathf::Sin(x)});
        b32.Add<float>("Mathf.Cos", {x}, {Mathf::Cos(x)});
        b32.Add<float>("Mathf.Exp", {x}, {Mathf::Exp(x)});
        b32.Add<float>("Mathf.Log", {x}, {Mathf::Log(x)});
        b32.Add<float>("Mathf.Floor", {x}, {Mathf::Floor(x)});
        b32.Add<float>("Mathf.Ceil", {x}, {Mathf::Ceil(x)});
        b32.Add<float>("Mathf.Repeat", {x, 360.0f}, {Mathf::Repeat(x, 360.0f)});
        b32.Add<float>("Mathf.PingPong", {x, 3.0f}, {Mathf::PingPong(x, 3.0f)});
        b32.Add<float>("Mathf.DeltaAngle", {x, 10.0f}, {Mathf::DeltaAngle(x, 10.0f)});
        const float xe = f32store(fx(x) + fx(1e-6f));
        b32.Add<float>("Mathf.Approximately", {x, xe}, {B(Mathf::Approximately(x, xe))});
        for (float y : {0.25f, 2.0f, -3.0f}) {
            b32.Add<float>("Mathf.Pow", {x, y}, {Mathf::Pow(x, y)});
            b32.Add<float>("Mathf.Atan2", {x, y}, {Mathf::Atan2(x, y)});
            const float x3 = f32store(fx(x) / 3.0);
            b32.Add<float>("Mathf.Lerp", {2.0f, 7.0f, x3}, {Mathf::Lerp(2.0f, 7.0f, x3)});
            b32.Add<float>("Mathf.LerpUnclamped", {2.0f, 7.0f, x3}, {Mathf::LerpUnclamped(2.0f, 7.0f, x3)});
            b32.Add<float>("Mathf.InverseLerp", {y, 7.0f, x}, {Mathf::InverseLerp(y, 7.0f, x)});
            b32.Add<float>("Mathf.MoveTowards", {x, y, 0.75f}, {Mathf::MoveTowards(x, y, 0.75f)});
            const float x5 = f32store(fx(x) / 5.0);
            b32.Add<float>("Mathf.SmoothStep", {y, 7.0f, x5}, {Mathf::SmoothStep(y, 7.0f, x5)});
            b32.Add<float>("Mathf.Clamp", {x, y, 5.0f}, {Mathf::Clamp(x, y, 5.0f)});
        }
    }

    const Vector3f V[] = {Vector3f(0, 0, 0),           Vector3f(1, 0, 0),
                          Vector3f(3, 4, 12),          Vector3f(-0.3f, 0.9f, -0.2f),
                          Vector3f(1e-6f, 2e-6f, -1e-6f), Vector3f(1000.5f, -250.25f, 0.125f),
                          Vector3f(0, 1, 0)};
    for (const Vector3f& a : V) {
        const Vector3f n = a.normalized();
        b32.Add<float>("Vector3.normalized", {a.x, a.y, a.z}, {n.x, n.y, n.z});
        b32.Add<float>("Vector3.magnitude", {a.x, a.y, a.z}, {a.magnitude(), a.sqrMagnitude()});
        const Vector3f cm = Vector3f::ClampMagnitude(a, 2.0f);
        b32.Add<float>("Vector3.ClampMagnitude2", {a.x, a.y, a.z}, {cm.x, cm.y, cm.z});
        for (const Vector3f& bb : V) {
            const Vector3f c = Vector3f::Cross(a, bb);
            b32.Add<float>("Vector3.Cross", {a.x, a.y, a.z, bb.x, bb.y, bb.z}, {c.x, c.y, c.z});
            b32.Add<float>("Vector3.Dot", {a.x, a.y, a.z, bb.x, bb.y, bb.z}, {Vector3f::Dot(a, bb)});
            b32.Add<float>("Vector3.Angle", {a.x, a.y, a.z, bb.x, bb.y, bb.z}, {Vector3f::Angle(a, bb)});
            const Vector3f p = Vector3f::Project(a, bb);
            b32.Add<float>("Vector3.Project", {a.x, a.y, a.z, bb.x, bb.y, bb.z}, {p.x, p.y, p.z});
            const Vector3f q = Vector3f::ProjectOnPlane(a, bb);
            b32.Add<float>("Vector3.ProjectOnPlane", {a.x, a.y, a.z, bb.x, bb.y, bb.z}, {q.x, q.y, q.z});
        }
    }
    const Quaternionf Q[] = {Quaternionf(0, 0, 0, 1), Quaternionf(0.70710677f, 0, 0, 0.70710677f),
                             Quaternionf(0.1f, -0.2f, 0.3f, 0.9f), Quaternionf(0.5f, 0.5f, -0.5f, 0.5f)};
    for (const Quaternionf& q : Q) {
        for (int vi = 0; vi < 4; ++vi) {
            const Vector3f& v = V[vi];
            const Vector3f r = q * v;
            b32.Add<float>("Quaternion*Vector3", {q.x, q.y, q.z, q.w, v.x, v.y}, {r.x, r.y, r.z});
        }
        for (const Quaternionf& q2 : Q) {
            const Quaternionf m = q * q2;
            b32.Add<float>("Quaternion*Quaternion", {q.x, q.y, q.z, q.w, q2.x, q2.y}, {m.x, m.y, m.z, m.w});
            b32.Add<float>("Quaternion.Angle", {q.x, q.y, q.z, q.w, q2.x, q2.y}, {Quaternionf::Angle(q, q2), Quaternionf::Dot(q, q2)});
        }
        const Quaternionf nq = Quaternionf::Normalize(q);
        b32.Add<float>("Quaternion.Normalize", {q.x, q.y, q.z, q.w}, {nq.x, nq.y, nq.z, nq.w});
    }

    // ---- KSP double math
    const Vector3d D[] = {Vector3d(0, 0, 0),          Vector3d(1, 0, 0),
                          Vector3d(3, 4, 12),         Vector3d(-0.3, 0.9, -0.2),
                          Vector3d(1e-9, 2e-9, -1e-9), Vector3d(13599840256.0, -21658398.5, 0.125),
                          Vector3d(0, 0, 1)};
    for (const Vector3d& a : D) {
        const Vector3d n = a.normalized();
        b64.Add<double>("Vector3d.normalized", {a.x, a.y, a.z}, {n.x, n.y, n.z});
        b64.Add<double>("Vector3d.magnitude", {a.x, a.y, a.z}, {a.magnitude(), a.sqrMagnitude()});
        for (const Vector3d& bb : D) {
            const Vector3d c = Vector3d::Cross(a, bb);
            b64.Add<double>("Vector3d.Cross", {a.x, a.y, a.z, bb.x, bb.y, bb.z}, {c.x, c.y, c.z});
            b64.Add<double>("Vector3d.Dot", {a.x, a.y, a.z, bb.x, bb.y, bb.z}, {Vector3d::Dot(a, bb)});
            b64.Add<double>("Vector3d.Angle", {a.x, a.y, a.z, bb.x, bb.y, bb.z}, {Vector3d::Angle(a, bb)});
            b64.Add<double>("UtilMath.AngleBetween", {a.x, a.y, a.z, bb.x, bb.y, bb.z}, {UtilMath::AngleBetween(a, bb)});
            const Vector3d ex = Vector3d::Exclude(a, bb);
            b64.Add<double>("Vector3d.Exclude", {a.x, a.y, a.z, bb.x, bb.y, bb.z}, {ex.x, ex.y, ex.z});
            const Vector3d pr = Vector3d::Project(a, bb);
            b64.Add<double>("Vector3d.Project", {a.x, a.y, a.z, bb.x, bb.y, bb.z}, {pr.x, pr.y, pr.z});
        }
    }
    for (double ang : {0.0, 1e-8, 30.0, 90.0, 180.0, -45.0, 359.0, 720.0}) {
        for (const Vector3d& axis : {Vector3d(0, 0, 1), Vector3d(1, 1, 0), Vector3d(0.3, -0.4, 0.5)}) {
            const QuaternionD q = QuaternionD::AngleAxis(ang, axis);
            b64.Add<double>("QuaternionD.AngleAxis", {ang, axis.x, axis.y, axis.z}, {q.x, q.y, q.z, q.w});
            const QuaternionD iq = QuaternionD::Inverse(q);
            b64.Add<double>("QuaternionD.Inverse", {q.x, q.y, q.z, q.w}, {iq.x, iq.y, iq.z, iq.w});
            const Vector3d r = q * Vector3d(1, 2, 3);
            b64.Add<double>("QuaternionD*Vector3d(1,2,3)", {q.x, q.y, q.z, q.w}, {r.x, r.y, r.z});
            const QuaternionD q2 = QuaternionD::AngleAxis(17.0, Vector3d(0, 1, 0));
            const QuaternionD m = q * q2;
            b64.Add<double>("QuaternionD*AngleAxis(17,Y)", {q.x, q.y, q.z, q.w}, {m.x, m.y, m.z, m.w});
            b64.Add<double>("QuaternionD.Angle(q,AngleAxis(17,Y))", {q.x, q.y, q.z, q.w}, {QuaternionD::Angle(q, q2)});
        }
    }
    const std::pair<Vector3d, Vector3d> LR[] = {{Vector3d(0, 0, 1), Vector3d(0, 1, 0)},
                                                {Vector3d(1, 2, 3), Vector3d(0, 0, 1)},
                                                {Vector3d(-0.5, 0.1, 0.2), Vector3d(0.3, 0.9, 0)}};
    for (const auto& [f, u] : LR) {
        const QuaternionD q = QuaternionD::LookRotation(f, u);
        b64.Add<double>("QuaternionD.LookRotation", {f.x, f.y, f.z, u.x, u.y, u.z}, {q.x, q.y, q.z, q.w});
    }
    const double PI = 3.141592653589793;
    for (double x : {-10.0, -PI, -1e-12, 0.0, 1.0, PI, 2 * PI, 7.0, 1e6}) {
        b64.Add<double>("UtilMath.ClampRadiansTwoPI", {x}, {UtilMath::ClampRadiansTwoPI(x)});
        b64.Add<double>("UtilMath.ClampRadiansPI", {x}, {UtilMath::ClampRadiansPI(x)});
        b64.Add<double>("UtilMath.ClampDegrees360", {x * 57.0}, {UtilMath::ClampDegrees360(x * 57.0)});
        b64.Add<double>("UtilMath.ClampDegrees180", {x * 57.0}, {UtilMath::ClampDegrees180(x * 57.0)});
        b64.Add<double>("UtilMath.WrapAround", {x, -1.0, 3.0}, {UtilMath::WrapAround(x, -1.0, 3.0)});
        b64.Add<double>("UtilMath.Lerp", {2.0, 7.0, x / 5}, {UtilMath::Lerp(2.0, 7.0, x / 5)});
        b64.Add<double>("UtilMath.Clamp01", {x}, {UtilMath::Clamp01(x)});
    }
    struct Tri {
        double lan, inc, ape;
    };
    for (const Tri& tr : {Tri{0.0, 0.0, 0.0}, Tri{77.7, 28.5, 123.4}, Tri{300.0, 135.0, 200.0}, Tri{0.0, 180.0, 30.0},
                          Tri{52.0, 1.304, 0.0}}) {
        CelestialFrame cf;
        CelestialFrame::OrbitalFrame(tr.lan, tr.inc, tr.ape, cf);
        b64.Add<double>("CelestialFrame.OrbitalFrame.X", {tr.lan, tr.inc, tr.ape}, {cf.X.x, cf.X.y, cf.X.z});
        b64.Add<double>("CelestialFrame.OrbitalFrame.Y", {tr.lan, tr.inc, tr.ape}, {cf.Y.x, cf.Y.y, cf.Y.z});
        b64.Add<double>("CelestialFrame.OrbitalFrame.Z", {tr.lan, tr.inc, tr.ape}, {cf.Z.x, cf.Z.y, cf.Z.z});
        CelestialFrame::PlanetaryFrame(tr.lan, tr.inc, tr.ape, cf);
        b64.Add<double>("CelestialFrame.PlanetaryFrame.X", {tr.lan, tr.inc, tr.ape}, {cf.X.x, cf.X.y, cf.X.z});
        b64.Add<double>("CelestialFrame.PlanetaryFrame.Y", {tr.lan, tr.inc, tr.ape}, {cf.Y.x, cf.Y.y, cf.Y.z});
        b64.Add<double>("CelestialFrame.PlanetaryFrame.Z", {tr.lan, tr.inc, tr.ape}, {cf.Z.x, cf.Z.y, cf.Z.z});
        const QuaternionD rq = cf.Rotation();
        b64.Add<double>("CelestialFrame.Rotation", {tr.lan, tr.inc, tr.ape}, {rq.x, rq.y, rq.z, rq.w});
    }
    // Planetarium.Awake with the level9 prefab value inverseRotAngle = 0 (golden/serialized/scene_singletons)
    CelestialFrame zup;
    QuaternionD rot;
    PlanetariumAwakeFrames(0.0, zup, rot);
    b64.Add<double>("Planetarium.Zup(after Awake)", {}, {zup.X.x, zup.X.y, zup.X.z});
    b64.Add<double>("Planetarium.Zup(after Awake).Y", {}, {zup.Y.x, zup.Y.y, zup.Y.z});
    b64.Add<double>("Planetarium.Zup(after Awake).Z", {}, {zup.Z.x, zup.Z.y, zup.Z.z});

    return WriteCandidate(opt, g, "math_shim/math_shim.json");
}

KSP_GOLDEN_DATASET("math_shim", 10, RunMathShim);
