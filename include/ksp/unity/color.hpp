// UnityEngine.Color, Color32, Rect, Matrix4x4 (value-type data only; the parts ConfigNode and the parsers need).
// Spec: decompiled/KSP-Decompiled/Unity/UnityEngine.CoreModule/UnityEngine/{Color,Color32,Rect,Matrix4x4}.cs
#pragma once

#include <cstdint>

#include "ksp/unity/mathf.hpp"

namespace ksp {

struct Color {
    float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
    constexpr Color() = default;
    constexpr Color(float r_, float g_, float b_, float a_) : r(r_), g(g_), b(b_), a(a_) {}
    constexpr Color(float r_, float g_, float b_) : r(r_), g(g_), b(b_), a(1.0f) {}
    static constexpr Color white() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    static constexpr Color black() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    static constexpr Color clear() { return {0.0f, 0.0f, 0.0f, 0.0f}; }
    bool Equals(const Color& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
};

struct Color32 {
    uint8_t r = 0, g = 0, b = 0, a = 0;
    constexpr Color32() = default;
    constexpr Color32(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_) : r(r_), g(g_), b(b_), a(a_) {}
    bool Equals(const Color32& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }

    // Color32.cs:72 implicit operator Color32(Color)
    static Color32 FromColor(const Color& c) {
        auto ch = [](float v) { return static_cast<uint8_t>(static_cast<int>(Mathf::Round(Mathf::Clamp01(v) * 255.0f))); };
        return {ch(c.r), ch(c.g), ch(c.b), ch(c.a)};
    }
    // Color32.cs:77 implicit operator Color(Color32)
    Color ToColor() const {
        return {static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f,
                static_cast<float>(a) / 255.0f};
    }
};

struct Rect {
    float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;  // m_XMin, m_YMin, m_Width, m_Height
    constexpr Rect() = default;
    constexpr Rect(float x_, float y_, float w_, float h_) : x(x_), y(y_), width(w_), height(h_) {}
    bool Equals(const Rect& o) const { return x == o.x && y == o.y && width == o.width && height == o.height; }
};

// UnityEngine.Matrix4x4 storage (row/column naming as Unity: mRC).
struct Matrix4x4f {
    float m00 = 0, m10 = 0, m20 = 0, m30 = 0;
    float m01 = 0, m11 = 0, m21 = 0, m31 = 0;
    float m02 = 0, m12 = 0, m22 = 0, m32 = 0;
    float m03 = 0, m13 = 0, m23 = 0, m33 = 0;
    static constexpr Matrix4x4f identity() {
        Matrix4x4f m;
        m.m00 = m.m11 = m.m22 = m.m33 = 1.0f;
        return m;
    }
};

}  // namespace ksp
