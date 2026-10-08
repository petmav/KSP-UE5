// unity_random candidate: mirrors the C2 capture (decompiled/KSP-Decompiled branch `profiling`,
// Assembly-CSharp/PhysProf/HarnessCapture.cs C2()) with ksp::UnityRandom, ksp::KSPRandom and dotnet::SystemRandom.
// Golden: harness/captures/out/unity_random.json (in-game). Tables ksp_class_radius / ksp_asteroid need later work
// packages (DiscoveryInfo, Orbit) and are not produced.
#include <functional>
#include <string>
#include <vector>

#include "datasets.hpp"
#include "golden_json.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/unity/random.hpp"

using namespace ksp;
using golden::Cell;
using golden::ColType;

namespace {
int64_t S(uint32_t v) { return static_cast<int32_t>(v); }  // state words are reported as signed int32
}  // namespace

static int RunUnityRandom(const RunnerOptions& opt) {
    golden::File g;
    g.dataset = "unity_random";
    g.description = "kspcore candidate for the C2 capture (UnityEngine.Random, KSPRandom).";

    std::vector<int32_t> seeds = {0, 1, 2, 42, 12345, -1, INT32_MIN, INT32_MAX, 24301};
    std::vector<std::string> seedSrc(9, "fixed");
    seedSrc[8] = "fixed (0x5EED)";
    dotnet::SystemRandom sr(20261008);
    for (int i = 0; i < 20; ++i) {
        seeds.push_back(sr.Next());
        seedSrc.push_back("System.Random(20261008).Next()#" + std::to_string(i) + " (Mono)");
    }

    golden::Table tseed("seeds", "Seeds used.");
    tseed.Col("idx", ColType::I64).Col("seed", ColType::I64).Col("source", ColType::Str);
    tseed.key = {"idx"};
    golden::Table tst("states", "Random.state after each block.");
    tst.Col("seed", ColType::I64).Col("block", ColType::Str).Col("s0", ColType::I64).Col("s1", ColType::I64)
        .Col("s2", ColType::I64).Col("s3", ColType::I64);
    tst.key = {"seed", "block"};
    golden::Table tv("value", "64 x Random.value after InitState(seed).");
    tv.Col("seed", ColType::I64).Col("k", ColType::I64).Col("value", ColType::F32);
    tv.key = {"seed", "k"};
    golden::Table trf("range_f", "64 x Random.Range(-10f, 10f) after InitState(seed).");
    trf.Col("seed", ColType::I64).Col("k", ColType::I64).Col("value", ColType::F32);
    trf.key = {"seed", "k"};
    golden::Table tri("range_i", "64 x Range(0,7) then 64 x Range(-1000,1000).");
    tri.Col("seed", ColType::I64).Col("call", ColType::Str).Col("k", ColType::I64).Col("value", ColType::I64);
    tri.key = {"seed", "call", "k"};
    golden::Table tvec("vec", "16 x insideUnitSphere, onUnitSphere, rotation, insideUnitCircle (one stream).");
    tvec.Col("seed", ColType::I64).Col("fn", ColType::Str).Col("k", ColType::I64).Col("x", ColType::F32)
        .Col("y", ColType::F32).Col("z", ColType::F32).Col("w", ColType::F32);
    tvec.key = {"seed", "fn", "k"};
    golden::Table ted("edge", "Edge-case calls (one stream).");
    ted.Col("seed", ColType::I64).Col("k", ColType::I64).Col("call", ColType::Str).Col("ri", ColType::I64)
        .Col("f0", ColType::F32).Col("f1", ColType::F32).Col("f2", ColType::F32).Col("f3", ColType::F32);
    ted.key = {"seed", "k"};
    golden::Table ttr("trace", "State before each of 12 Random.value calls and the value.");
    ttr.Col("seed", ColType::I64).Col("k", ColType::I64).Col("s0", ColType::I64).Col("s1", ColType::I64)
        .Col("s2", ColType::I64).Col("s3", ColType::I64).Col("value", ColType::F32);
    ttr.key = {"seed", "k"};
    golden::Table ttri("trace_int", "State before each of 12 Random.Range(int.MinValue, int.MaxValue) calls.");
    ttri.Col("seed", ColType::I64).Col("k", ColType::I64).Col("s0", ColType::I64).Col("s1", ColType::I64)
        .Col("s2", ColType::I64).Col("s3", ColType::I64).Col("value", ColType::I64);
    ttri.key = {"seed", "k"};

    UnityRandom R;
    auto state = [&](int32_t seed, const char* block) {
        const auto s = R.state();
        tst.Add({int64_t(seed), block, S(s.s0), S(s.s1), S(s.s2), S(s.s3)});
    };
    for (size_t i = 0; i < seeds.size(); ++i) tseed.Add({int64_t(i), int64_t(seeds[i]), seedSrc[i]});
    for (const int32_t seed : seeds) {
        R.InitState(seed);
        state(seed, "init");
        for (int k = 0; k < 64; ++k) tv.Add({int64_t(seed), int64_t(k), R.value()});
        state(seed, "after_value");
        R.InitState(seed);
        for (int k = 0; k < 64; ++k) trf.Add({int64_t(seed), int64_t(k), R.Range(-10.0f, 10.0f)});
        state(seed, "after_range_f");
        R.InitState(seed);
        for (int k = 0; k < 64; ++k) tri.Add({int64_t(seed), "0_7", int64_t(k), int64_t(R.Range(0, 7))});
        for (int k = 0; k < 64; ++k) tri.Add({int64_t(seed), "-1000_1000", int64_t(k), int64_t(R.Range(-1000, 1000))});
        state(seed, "after_range_i");
        R.InitState(seed);
        for (int k = 0; k < 16; ++k) {
            const Vector3f v = R.insideUnitSphere();
            tvec.Add({int64_t(seed), "insideUnitSphere", int64_t(k), v.x, v.y, v.z, nullptr});
        }
        for (int k = 0; k < 16; ++k) {
            const Vector3f v = R.onUnitSphere();
            tvec.Add({int64_t(seed), "onUnitSphere", int64_t(k), v.x, v.y, v.z, nullptr});
        }
        for (int k = 0; k < 16; ++k) {
            const Quaternionf q = R.rotation();
            tvec.Add({int64_t(seed), "rotation", int64_t(k), q.x, q.y, q.z, q.w});
        }
        for (int k = 0; k < 16; ++k) {
            const Vector2f v = R.insideUnitCircle();
            tvec.Add({int64_t(seed), "insideUnitCircle", int64_t(k), v.x, v.y, nullptr, nullptr});
        }
        state(seed, "after_vec");
        R.InitState(seed);
        int ke = 0;
        auto E = [&](const char* call, Cell ri, std::vector<float> f) {
            std::vector<Cell> row = {int64_t(seed), int64_t(ke++), call, ri};
            for (size_t j = 0; j < 4; ++j) row.push_back(j < f.size() ? Cell(f[j]) : Cell(nullptr));
            ted.Add(std::move(row));
        };
        E("Range(5,5)", int64_t(R.Range(5, 5)), {});
        E("Range(7,3)", int64_t(R.Range(7, 3)), {});
        E("Range(-3,-7)", int64_t(R.Range(-3, -7)), {});
        E("Range(int.MinValue,int.MaxValue)", int64_t(R.Range(INT32_MIN, INT32_MAX)), {});
        E("Range(0,int.MaxValue)", int64_t(R.Range(0, INT32_MAX)), {});
        E("Range(0,int.MaxValue)", int64_t(R.Range(0, INT32_MAX)), {});
        E("Range(0,1)", int64_t(R.Range(0, 1)), {});
        E("Range(0,2)", int64_t(R.Range(0, 2)), {});
        E("Range(10f,-10f)", nullptr, {R.Range(10.0f, -10.0f)});
        E("Range(1f,1f)", nullptr, {R.Range(1.0f, 1.0f)});
        E("Range(0f,1f)", nullptr, {R.Range(0.0f, 1.0f)});
        E("Range(0f,1.1f)", nullptr, {R.Range(0.0f, 1.1f)});
        E("Range(100f,500f)", nullptr, {R.Range(100.0f, 500.0f)});
        E("Range(-1e30f,1e30f)", nullptr, {R.Range(-1E+30f, 1E+30f)});
        for (int k = 0; k < 4; ++k) {
            const Quaternionf q = R.rotationUniform();
            E("rotationUniform", nullptr, {q.x, q.y, q.z, q.w});
        }
        for (int k = 0; k < 2; ++k) {
            const Color c = R.ColorHSV();
            E("ColorHSV()", nullptr, {c.r, c.g, c.b, c.a});
        }
        E("value", nullptr, {R.value()});
        E("value", nullptr, {R.value()});
        state(seed, "after_edge");
    }
    for (const int32_t seed : {0, 1, 42, INT32_MIN, 12345}) {
        R.InitState(seed);
        for (int k = 0; k < 12; ++k) {
            const auto s = R.state();
            const float v = R.value();
            ttr.Add({int64_t(seed), int64_t(k), S(s.s0), S(s.s1), S(s.s2), S(s.s3), v});
        }
        R.InitState(seed);
        for (int k = 0; k < 12; ++k) {
            const auto s = R.state();
            const int32_t v = R.Range(INT32_MIN, INT32_MAX);
            ttri.Add({int64_t(seed), int64_t(k), S(s.s0), S(s.s1), S(s.s2), S(s.s3), int64_t(v)});
        }
    }

    golden::Table ttv("trace_vec", "16 calls of each vector function (fresh InitState per function), state before/after.");
    ttv.Col("seed", ColType::I64).Col("fn", ColType::Str).Col("k", ColType::I64).Col("s0", ColType::I64)
        .Col("s1", ColType::I64).Col("s2", ColType::I64).Col("s3", ColType::I64).Col("x", ColType::F32)
        .Col("y", ColType::F32).Col("z", ColType::F32).Col("w", ColType::F32).Col("a0", ColType::I64)
        .Col("a1", ColType::I64).Col("a2", ColType::I64).Col("a3", ColType::I64);
    ttv.key = {"seed", "fn", "k"};
    for (const int32_t vs : {0, 1, 42}) {
        for (const char* fn : {"insideUnitSphere", "onUnitSphere", "rotation", "rotationUniform", "insideUnitCircle", "ColorHSV()"}) {
            R.InitState(vs);
            const std::string f = fn;
            for (int k = 0; k < 16; ++k) {
                const auto b = R.state();
                float r4[4] = {0, 0, 0, 0};
                int nr = 3;
                if (f == "insideUnitSphere") {
                    const Vector3f v = R.insideUnitSphere();
                    r4[0] = v.x; r4[1] = v.y; r4[2] = v.z;
                } else if (f == "onUnitSphere") {
                    const Vector3f v = R.onUnitSphere();
                    r4[0] = v.x; r4[1] = v.y; r4[2] = v.z;
                } else if (f == "rotation") {
                    const Quaternionf q = R.rotation();
                    r4[0] = q.x; r4[1] = q.y; r4[2] = q.z; r4[3] = q.w;
                    nr = 4;
                } else if (f == "rotationUniform") {
                    const Quaternionf q = R.rotationUniform();
                    r4[0] = q.x; r4[1] = q.y; r4[2] = q.z; r4[3] = q.w;
                    nr = 4;
                } else if (f == "insideUnitCircle") {
                    const Vector2f v = R.insideUnitCircle();
                    r4[0] = v.x; r4[1] = v.y;
                    nr = 2;
                } else {
                    const Color c = R.ColorHSV();
                    r4[0] = c.r; r4[1] = c.g; r4[2] = c.b; r4[3] = c.a;
                    nr = 4;
                }
                const auto a = R.state();
                ttv.Add({int64_t(vs), fn, int64_t(k), S(b.s0), S(b.s1), S(b.s2), S(b.s3), r4[0],
                         nr > 1 ? Cell(r4[1]) : Cell(nullptr), nr > 2 ? Cell(r4[2]) : Cell(nullptr),
                         nr > 3 ? Cell(r4[3]) : Cell(nullptr), S(a.s0), S(a.s1), S(a.s2), S(a.s3)});
            }
        }
    }

    golden::Table tkr("ksp_random", "KSPRandom(seed): separate instances for NextDouble() and Next().");
    tkr.Col("seed", ColType::I64).Col("k", ColType::I64).Col("nextDouble", ColType::F64).Col("next", ColType::I64)
        .Col("err", ColType::Str);
    tkr.key = {"seed", "k"};
    for (const int32_t seed : {0, 1, 42, 12345, 24301, INT32_MAX, INT32_MIN}) {
        try {
            KSPRandom ra(seed), rb(seed);
            for (int k = 0; k < 16; ++k) tkr.Add({int64_t(seed), int64_t(k), ra.NextDouble(), int64_t(rb.Next()), nullptr});
        } catch (const dotnet::OverflowException&) {
            tkr.Add({int64_t(seed), int64_t(0), nullptr, nullptr, "OverflowException"});
        }
    }

    g.tables = {std::move(tseed), std::move(tst), std::move(tv),  std::move(trf), std::move(tri), std::move(tvec),
                std::move(ted),   std::move(ttr), std::move(ttri), std::move(ttv), std::move(tkr)};
    return WriteCandidate(opt, g, "unity_random/unity_random.json");
}

KSP_GOLDEN_DATASET("unity_random", 50, RunUnityRandom);
