// libm_check: the portable VS2013-CRT libm (ksp::dotnet::vs2013) against the real msvcr120 DLL, on this machine.
// Windows only (loads msvcr120_clr0400.dll / msvcr120.dll); elsewhere prints a note and exits 0.
//   libm_check [count-per-function] [--fma3=0|1]
// Inputs: random bit patterns, uniform ranges per function domain, values near integers/powers of two, specials.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "ksp/dotnet/ucrt_libm.hpp"
#include "ksp/dotnet/vs2013_libm.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace v = ksp::dotnet::vs2013;
using F1 = double (*)(double);
using F2 = double (*)(double, double);

static uint64_t Bits(double d) { uint64_t b; std::memcpy(&b, &d, 8); return b; }
[[maybe_unused]] static double FromBits(uint64_t b) { double d; std::memcpy(&d, &b, 8); return d; }
[[maybe_unused]] static bool Same(double a, double b) { return Bits(a) == Bits(b) || (std::isnan(a) && std::isnan(b) && Bits(a) == Bits(b)); }

int main(int argc, char** argv) {
    int n = 300000;
    int fma3 = -1;
    for (int i = 1; i < argc; ++i) {
        if (!std::strncmp(argv[i], "--fma3=", 7)) fma3 = std::atoi(argv[i] + 7);
        else n = std::atoi(argv[i]);
    }
    if (fma3 >= 0) v::SetFma3(fma3 != 0);
#ifndef _WIN32
    (void)n;
    std::puts("libm_check: no msvcr120 on this platform: skipped (the capture gates check the portable libm)");
    return 0;
#else
    HMODULE h = nullptr;
    const wchar_t* which = nullptr;
    for (const wchar_t* dll : {L"msvcr120_clr0400.dll", L"msvcr120.dll"}) {
        h = LoadLibraryW(dll);
        if (h) { which = dll; break; }
    }
    if (!h) { std::puts("libm_check: no msvcr120 DLL found: skipped"); return 0; }
    std::wprintf(L"libm_check: %ls, portable fma3=%d, %d inputs per function\n", which, v::GetFma3() ? 1 : 0, n);
    struct Fn1 { const char* name; F1 dll; F1 sw; double lo, hi; };
    struct Fn2 { const char* name; F2 dll; F2 sw; };
    auto g1 = [&](const char* nm) { return reinterpret_cast<F1>(GetProcAddress(h, nm)); };
    auto g2 = [&](const char* nm) { return reinterpret_cast<F2>(GetProcAddress(h, nm)); };
    std::vector<Fn1> f1 = {{"acos", g1("acos"), v::acos, -1.2, 1.2},   {"asin", g1("asin"), v::asin, -1.2, 1.2},
                           {"atan", g1("atan"), v::atan, -50, 50},     {"cosh", g1("cosh"), v::cosh, -30, 30},
                           {"sinh", g1("sinh"), v::sinh, -30, 30},     {"tanh", g1("tanh"), v::tanh, -10, 10},
                           {"exp", g1("exp"), v::exp, -50, 50},        {"log", g1("log"), v::log, 0, 1000},
                           {"log10", g1("log10"), v::log10, 0, 1000},  {"tan", g1("tan"), v::tan, -10, 10}};
    std::vector<Fn2> f2 = {{"atan2", g2("atan2"), v::atan2}, {"pow", g2("pow"), v::pow}};
    std::mt19937_64 rng(12345);
    auto rnd = [&](double lo, double hi) { return lo + (hi - lo) * (static_cast<double>(rng() >> 11) / 9007199254740992.0); };
    auto special = [&](int i) {
        static const double sp[] = {0.0, -0.0, 1.0, -1.0, 0.5, -0.5, 2.0, 10.0, 1e-300, 1e300, -1e300, 1e-5, 4.94e-324,
                                    INFINITY, -INFINITY, NAN, 710.0, -745.0, 709.78, 0.7, 1e-10, 0x1p-27, 0x1p-28, 20.0, 22.0, 36.0};
        return sp[i % (sizeof sp / sizeof sp[0])];
    };
    uint64_t totalBad = 0;
    auto gen = [&](int i, double lo, double hi) {
        switch (i % 6) {
            case 0: return FromBits(rng());
            case 1: return rnd(lo, hi);
            case 2: return rnd(lo, hi) * 1e-3;
            case 3: return std::round(rnd(lo, hi)) + (rng() & 1 ? 1 : -1) * std::ldexp(1.0, -static_cast<int>(rng() % 52));
            case 4: return special(i / 6);
            default: return rnd(lo, hi) * 1e3;
        }
    };
    for (const Fn1& f : f1) {
        uint64_t bad = 0;
        double firstX = 0, firstD = 0, firstS = 0;
        for (int i = 0; i < n; ++i) {
            const double x = gen(i, f.lo, f.hi);
            const double d = f.dll(x), s = f.sw(x);
            if (!Same(d, s)) { if (!bad) { firstX = x; firstD = d; firstS = s; } ++bad; }
        }
        std::printf("  %-6s %s  mismatches %llu / %d", f.name, bad ? "FAIL" : "ok  ", static_cast<unsigned long long>(bad), n);
        if (bad) std::printf("   first x=%a dll=%a soft=%a", firstX, firstD, firstS);
        std::puts("");
        totalBad += bad;
    }
    for (const Fn2& f : f2) {
        uint64_t bad = 0;
        double fx = 0, fy = 0, fd = 0, fs = 0;
        for (int i = 0; i < n; ++i) {
            const double x = gen(i, -20, 20), y = gen(i / 2 + (i % 3), -20, 20);
            const double d = f.dll(x, y), s = f.sw(x, y);
            if (!Same(d, s)) { if (!bad) { fx = x; fy = y; fd = d; fs = s; } ++bad; }
        }
        std::printf("  %-6s %s  mismatches %llu / %d", f.name, bad ? "FAIL" : "ok  ", static_cast<unsigned long long>(bad), n);
        if (bad) std::printf("   first x=%a y=%a dll=%a soft=%a", fx, fy, fd, fs);
        std::puts("");
        totalBad += bad;
    }
    // ucrtbase single-precision functions used by Unity's native code
    {
        HMODULE u = LoadLibraryW(L"ucrtbase.dll");
        using G1 = float (*)(float);
        using G2 = float (*)(float, float);
        struct Fnf { const char* name; G1 dll; G1 sw; };
        Fnf fs[] = {{"sinf", reinterpret_cast<G1>(GetProcAddress(u, "sinf")), ksp::dotnet::ucrt::sinf},
                    {"cosf", reinterpret_cast<G1>(GetProcAddress(u, "cosf")), ksp::dotnet::ucrt::cosf}};
        auto powd = reinterpret_cast<G2>(GetProcAddress(u, "powf"));
        auto fbits = [](float f) { uint32_t b; std::memcpy(&b, &f, 4); return b; };
        auto rf = [&](float lo, float hi) { return static_cast<float>(lo + (hi - lo) * (static_cast<double>(rng() >> 11) / 9007199254740992.0)); };
        for (const Fnf& f : fs) {
            uint64_t bad = 0;
            for (int i = 0; i < n; ++i) {
                float x;
                if (i % 3 == 0) { uint32_t b = static_cast<uint32_t>(rng()); std::memcpy(&x, &b, 4); }
                else x = rf(-8.0f, 8.0f) * (i % 3 == 1 ? 1.0f : 100.0f);
                const float a = f.dll(x), b = f.sw(x);
                if (fbits(a) != fbits(b) && !(std::isnan(a) && std::isnan(b))) ++bad;
            }
            std::printf("  %-6s %s  mismatches %llu / %d\n", f.name, bad ? "FAIL" : "ok  ", static_cast<unsigned long long>(bad), n);
            totalBad += bad;
        }
        uint64_t bad = 0;
        for (int i = 0; i < n; ++i) {
            float x = i % 4 == 0 ? rf(0.0f, 1.0f) : i % 4 == 1 ? rf(-5.0f, 5.0f) : i % 4 == 2 ? rf(0.0f, 100.0f) : static_cast<float>(static_cast<int>(rf(-6, 6)));
            float y = i % 5 == 0 ? 0.333333343f : i % 5 == 1 ? 2.0f : i % 5 == 2 ? rf(-4.0f, 4.0f) : i % 5 == 3 ? static_cast<float>(static_cast<int>(rf(-4, 4))) : rf(0.0f, 1.0f);
            const float a = powd(x, y), b = ksp::dotnet::ucrt::powf(x, y);
            if (fbits(a) != fbits(b) && !(std::isnan(a) && std::isnan(b))) { if (bad < 4) std::printf("    powf x=%a y=%a dll=%a soft=%a\n", (double)x, (double)y, (double)a, (double)b); ++bad; }
        }
        std::printf("  %-6s %s  mismatches %llu / %d\n", "powf", bad ? "FAIL" : "ok  ", static_cast<unsigned long long>(bad), n);
        totalBad += bad;
    }
    std::printf("libm_check: %s (%llu mismatches)\n", totalBad ? "FAILED" : "all functions bit-exact", static_cast<unsigned long long>(totalBad));
    return totalBad ? 1 : 0;
#endif
}
