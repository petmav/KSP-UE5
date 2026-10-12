// WP-29a: VehiclePhysics/TireFriction.cs (the five tyre curves and the contact-patch maths) and EdyCommonTools/MathUtility.cs, InterpolatedFloat.cs.
// Every function cites File.cs:line of decompiled/KSP-Decompiled/Assembly-CSharp (tools/wp28/check_cites.py verifies the citations).
// Float32 follows the Mono rule (ksp/dotnet/mono_float.hpp): double expressions, rounded where the C# stores, passes, returns or casts.
#include "ksp/wheels/vehicle_physics.hpp"

namespace ksp::VehiclePhysics {

// EdyCommonTools/MathUtility.cs:31
float MathUtility::FastAbs(float x) {
    if (!(x < 0.0f)) return x;
    return f32store(0.0 - fx(x));
}

// EdyCommonTools/MathUtility.cs:58
bool MathUtility::Vector3Equals(const Vector3f& a, const Vector3f& b) {
    if (a.x == b.x && a.y == b.y) return a.z == b.z;
    return false;
}

// EdyCommonTools/MathUtility.cs:107
float MathUtility::UnclampedLerp(float from, float to, float t) { return f32store(fx(from) + (fx(to) - fx(from)) * fx(t)); }

// EdyCommonTools/InterpolatedFloat.cs:11
InterpolatedFloat::InterpolatedFloat(float initialValue) { Reset(initialValue); }

// EdyCommonTools/InterpolatedFloat.cs:16
void InterpolatedFloat::Reset(float value) {
    m_prevValue = value;
    m_value = value;
}

// EdyCommonTools/InterpolatedFloat.cs:22
void InterpolatedFloat::Set(float value) {
    m_prevValue = m_value;
    m_value = value;
}

// EdyCommonTools/InterpolatedFloat.cs:44
float InterpolatedFloat::GetInterpolated(float t) const { return Mathf::Lerp(m_prevValue, m_value, t); }

// ---- TireFriction.Settings ------------------------------------------------------------------------------------------------------------
// VehiclePhysics/TireFriction.cs:56
void TireFriction::Settings::ApplyAPLConstraints() {
    adherent.x = Mathf::Max(0.01f, adherent.x);
    peak.x = Mathf::Max(adherent.x, peak.x);
    limit.x = Mathf::Max(peak.x, limit.x);
    adherent.y = Mathf::Max(0.0f, adherent.y);
    peak.y = Mathf::Max(0.0f, peak.y);
    limit.y = Mathf::Max(0.0f, limit.y);
    adherent.y = Mathf::Min(peak.y, adherent.y);
    limit.y = Mathf::Min(peak.y, limit.y);
}

// ---- CurveBase -------------------------------------------------------------------------------------------------------------------------
// VehiclePhysics/TireFriction.cs:87
float TireFriction::CurveBase::EvaluateForce(float, const ContactPatch&) { return 1.0f; }
// VehiclePhysics/TireFriction.cs:92
float TireFriction::CurveBase::GetAdherentSlip(const ContactPatch&) { return 0.5f; }
// VehiclePhysics/TireFriction.cs:97
float TireFriction::CurveBase::GetPeakSlip(const ContactPatch&) { return 1.5f; }
// VehiclePhysics/TireFriction.cs:102
float TireFriction::CurveBase::GetLimitSlip(const ContactPatch&) { return 4.0f; }

// ---- FlatFriction ----------------------------------------------------------------------------------------------------------------------
// VehiclePhysics/TireFriction.cs:110
TireFriction::FlatFriction::FlatFriction(std::shared_ptr<Settings> settings) : CurveBase(std::move(settings)) {
    m_params->adherent.x = Mathf::Max(0.01f, m_params->adherent.x);
    m_params->adherent.y = Mathf::Max(0.0f, m_params->adherent.y);
}
// VehiclePhysics/TireFriction.cs:117
float TireFriction::FlatFriction::EvaluateForce(float, const ContactPatch& cp) {
    return f32store(fx(m_params->adherent.y) * fx(cp.load) * fx(cp.groundGrip));
}
// VehiclePhysics/TireFriction.cs:122
float TireFriction::FlatFriction::GetAdherentSlip(const ContactPatch&) { return m_params->adherent.x; }
// VehiclePhysics/TireFriction.cs:127
float TireFriction::FlatFriction::GetPeakSlip(const ContactPatch&) { return m_params->adherent.x; }
// VehiclePhysics/TireFriction.cs:132
float TireFriction::FlatFriction::GetLimitSlip(const ContactPatch&) { return m_params->adherent.x; }

// ---- LinealFriction --------------------------------------------------------------------------------------------------------------------
// VehiclePhysics/TireFriction.cs:140
TireFriction::LinealFriction::LinealFriction(std::shared_ptr<Settings> settings) : CurveBase(std::move(settings)) { m_params->ApplyAPLConstraints(); }
// VehiclePhysics/TireFriction.cs:146
float TireFriction::LinealFriction::FastLerp(const Vector2f& P0, const Vector2f& P1, float x) {
    return f32store(fx(P0.y) + (fx(x) - fx(P0.x)) * (fx(P1.y) - fx(P0.y)) / (fx(P1.x) - fx(P0.x)));
}
// VehiclePhysics/TireFriction.cs:151
float TireFriction::LinealFriction::EvaluateForce(float s, const ContactPatch& cp) {
    const float num = f32store(fx(cp.load) * fx(cp.groundGrip));
    if (s <= m_params->adherent.x) return f32store(fx(num) * fx(m_params->adherent.y));
    if (s < m_params->peak.x) return f32store(fx(num) * fx(FastLerp(m_params->adherent, m_params->peak, s)));
    if (s < m_params->limit.x) return f32store(fx(num) * fx(FastLerp(m_params->peak, m_params->limit, s)));
    return f32store(fx(num) * fx(m_params->limit.y));
}
// VehiclePhysics/TireFriction.cs:169
float TireFriction::LinealFriction::GetAdherentSlip(const ContactPatch&) { return m_params->adherent.x; }
// VehiclePhysics/TireFriction.cs:174
float TireFriction::LinealFriction::GetPeakSlip(const ContactPatch&) { return m_params->peak.x; }
// VehiclePhysics/TireFriction.cs:179
float TireFriction::LinealFriction::GetLimitSlip(const ContactPatch&) { return m_params->limit.x; }

// ---- SmoothFriction --------------------------------------------------------------------------------------------------------------------
// VehiclePhysics/TireFriction.cs:187
TireFriction::SmoothFriction::SmoothFriction(std::shared_ptr<Settings> settings) : CurveBase(std::move(settings)) { m_params->ApplyAPLConstraints(); }
// VehiclePhysics/TireFriction.cs:193
float TireFriction::SmoothFriction::CubicLerp(float x, const Vector2f& P0, const Vector2f& P1) {
    const float num = f32store((fx(x) - fx(P0.x)) / (fx(P1.x) - fx(P0.x)));
    const float num2 = f32store(fx(num) * fx(num));
    const float num3 = f32store(fx(num) * fx(num2));
    return f32store(fx(P0.y) * (2.0 * fx(num3) - 3.0 * fx(num2) + 1.0) + fx(P1.y) * (-2.0 * fx(num3) + 3.0 * fx(num2)));
}
// VehiclePhysics/TireFriction.cs:201
float TireFriction::SmoothFriction::EvaluateForce(float s, const ContactPatch& cp) {
    const float num = f32store(fx(cp.load) * fx(cp.groundGrip));
    if (s <= m_params->adherent.x) return f32store(fx(num) * fx(m_params->adherent.y));
    if (s < m_params->peak.x) return f32store(fx(num) * fx(CubicLerp(s, m_params->adherent, m_params->peak)));
    if (s < m_params->limit.x) return f32store(fx(num) * fx(CubicLerp(s, m_params->peak, m_params->limit)));
    return f32store(fx(num) * fx(m_params->limit.y));
}
// VehiclePhysics/TireFriction.cs:219
float TireFriction::SmoothFriction::GetAdherentSlip(const ContactPatch&) { return m_params->adherent.x; }
// VehiclePhysics/TireFriction.cs:224
float TireFriction::SmoothFriction::GetPeakSlip(const ContactPatch&) { return m_params->peak.x; }
// VehiclePhysics/TireFriction.cs:229
float TireFriction::SmoothFriction::GetLimitSlip(const ContactPatch&) { return m_params->limit.x; }

// ---- ParametricFriction ----------------------------------------------------------------------------------------------------------------
// VehiclePhysics/TireFriction.cs:237
TireFriction::ParametricFriction::ParametricFriction(std::shared_ptr<Settings> settings) : CurveBase(std::move(settings)) { m_params->ApplyAPLConstraints(); }
// VehiclePhysics/TireFriction.cs:243
float TireFriction::ParametricFriction::TangentLerp(float x, const Vector2f& P0, const Vector2f& P1, float a, float b) {
    const float num = f32store(fx(P1.y) - fx(P0.y));
    const float num2 = f32store(3.0 * fx(num) * fx(a));
    const float num3 = f32store(3.0 * fx(num) * fx(b));
    const float num4 = f32store((fx(x) - fx(P0.x)) / (fx(P1.x) - fx(P0.x)));
    const float num5 = f32store(fx(num4) * fx(num4));
    const float num6 = f32store(fx(num4) * fx(num5));
    return f32store(fx(P0.y) * (2.0 * fx(num6) - 3.0 * fx(num5) + 1.0) + fx(P1.y) * (-2.0 * fx(num6) + 3.0 * fx(num5)) +
                    fx(num2) * (fx(num6) - 2.0 * fx(num5) + fx(num4)) + fx(num3) * (fx(num6) - fx(num5)));
}
// VehiclePhysics/TireFriction.cs:254
float TireFriction::ParametricFriction::EvaluateForce(float s, const ContactPatch& cp) {
    const float num = f32store(fx(cp.load) * fx(cp.groundGrip));
    if (s <= m_params->adherent.x) return f32store(fx(num) * fx(m_params->adherent.y));
    if (s < m_params->peak.x) return f32store(fx(num) * fx(TangentLerp(s, m_params->adherent, m_params->peak, m_params->a, m_params->b)));
    if (s < m_params->limit.x) return f32store(fx(num) * fx(TangentLerp(s, m_params->peak, m_params->limit, m_params->c, m_params->d)));
    return f32store(fx(num) * fx(m_params->limit.y));
}
// VehiclePhysics/TireFriction.cs:272
float TireFriction::ParametricFriction::GetAdherentSlip(const ContactPatch&) { return m_params->adherent.x; }
// VehiclePhysics/TireFriction.cs:277
float TireFriction::ParametricFriction::GetPeakSlip(const ContactPatch&) { return m_params->peak.x; }
// VehiclePhysics/TireFriction.cs:282
float TireFriction::ParametricFriction::GetLimitSlip(const ContactPatch&) { return m_params->limit.x; }

// ---- PacejkaFriction -------------------------------------------------------------------------------------------------------------------
// VehiclePhysics/TireFriction.cs:296
TireFriction::PacejkaFriction::PacejkaFriction(std::shared_ptr<Settings> settings) : CurveBase(std::move(settings)) {
    Settings& s = *m_params;
    s.A = Mathf::Max(s.A, 0.01f);
    s.D = Mathf::Max(s.D, 0.0f);
    s.B = Mathf::Max(s.B, 0.1f);
    s.C = Mathf::Clamp(s.C, 1.0f, 2.0f);
    s.E = Mathf::Min(s.E, 1.0f);
    GetPacejkaPoints(s.A, s.B, s.C, s.D, s.E, m_adherent, m_peak, m_limit);
}

// VehiclePhysics/TireFriction.cs:307
void TireFriction::PacejkaFriction::GetPacejkaPoints(float A, float B, float C, float D, float E, Vector2f& adherent, Vector2f& peak, Vector2f& limit) {
    bool flag = false;
    bool flag2 = false;
    bool flag3 = false;
    float num = 0.0f;
    const float num2 = 0.01f;
    const float num3 = 15.0f;
    float num4 = Pacejka(num, B, C, D, E);
    while (num < num3) {
        const float num5 = Pacejka(f32store(fx(num) + fx(num2)), B, C, D, E);
        if (!flag && A >= num4 && A < num5) {
            adherent.x = Mathf::Lerp(num, f32store(fx(num) + fx(num2)), Mathf::InverseLerp(num4, num5, A));
            adherent.y = Pacejka(adherent.x, B, C, D, E);
            flag = true;
        }
        if (!flag2 && num5 <= num4) {
            peak.x = num;
            peak.y = num4;
            flag2 = true;
        }
        if (!flag2 || !(fx(num) > fx(peak.x) + 1.0 / fx(B)) ||
            fx(MathUtility::FastAbs(f32store(fx(num5) - fx(num4)))) / fx(num2) >= fx(0.01f)) {
            num = f32store(fx(num) + fx(num2));
            num4 = num5;
            continue;
        }
        limit.x = f32store(fx(num) + fx(num2));
        flag3 = true;
        break;
    }
    if (!flag3) limit.x = num3;
    limit.y = Pacejka(limit.x, B, C, D, E);
    if (!flag2) peak = limit;
    if (!flag) adherent = flag2 ? peak : limit;
}

// VehiclePhysics/TireFriction.cs:17
float TireFriction::PacejkaFriction::Pacejka(float slip, float B, float C, float D, float E) {
    const float num = f32store(fx(B) * fx(slip));
    return f32store(fx(D) * fx(Mathf::Sin(f32store(fx(C) * fx(Mathf::Atan(f32store(fx(num) - fx(E) * (fx(num) - fx(Mathf::Atan(num))))))))));
}
// VehiclePhysics/TireFriction.cs:362
float TireFriction::PacejkaFriction::EvaluateForce(float s, const ContactPatch& cp) {
    const float num = f32store(fx(cp.load) * fx(cp.groundGrip));
    if (s <= m_adherent.x) return f32store(fx(num) * fx(m_adherent.y));
    if (s > m_limit.x) return f32store(fx(num) * fx(m_limit.y));
    return f32store(fx(num) * fx(Pacejka(s, m_params->B, m_params->C, m_params->D, m_params->E)));
}
// VehiclePhysics/TireFriction.cs:376
float TireFriction::PacejkaFriction::GetAdherentSlip(const ContactPatch&) { return m_adherent.x; }
// VehiclePhysics/TireFriction.cs:381
float TireFriction::PacejkaFriction::GetPeakSlip(const ContactPatch&) { return m_peak.x; }
// VehiclePhysics/TireFriction.cs:386
float TireFriction::PacejkaFriction::GetLimitSlip(const ContactPatch&) { return m_limit.x; }

// ---- TireFriction ----------------------------------------------------------------------------------------------------------------------
// VehiclePhysics/TireFriction.cs:401
TireFriction::TireFriction() { SetupFrictionCurves(); }

// VehiclePhysics/TireFriction.cs:406
void TireFriction::OnBeforeSerialize() {}

// VehiclePhysics/TireFriction.cs:410
void TireFriction::OnAfterDeserialize() { SetupFrictionCurves(); }

// VehiclePhysics/TireFriction.cs:415
void TireFriction::SetupFrictionCurves() {
    switch (model) {
        case Model::Flat: m_curve = std::make_shared<FlatFriction>(settings); break;
        case Model::Lineal: m_curve = std::make_shared<LinealFriction>(settings); break;
        case Model::Smooth: m_curve = std::make_shared<SmoothFriction>(settings); break;
        case Model::Parametric: m_curve = std::make_shared<ParametricFriction>(settings); break;
        case Model::Pacejka: m_curve = std::make_shared<PacejkaFriction>(settings); break;
    }
}

// VehiclePhysics/TireFriction.cs:437
Vector2f TireFriction::GetForce(const ContactPatch& cp) {
    Vector2f slip = cp.slip;
    const float magnitude = slip.magnitude();
    if (magnitude < 1E-06f) {
        slip.x = 0.0f;
        slip.y = 0.0f;
        return slip;
    }
    slip.x = f32store(fx(slip.x) / fx(magnitude));
    slip.y = f32store(fx(slip.y) / fx(magnitude));
    const float d = f32store(0.0 - fx(m_curve->EvaluateForce(magnitude, cp)));
    return (d * slip) * frictionMultiplier;
}

// VehiclePhysics/TireFriction.cs:452
bool TireFriction::IsAdherentSlip(const ContactPatch& cp) { return cp.slip.magnitude() < m_curve->GetAdherentSlip(cp); }

// VehiclePhysics/TireFriction.cs:457
float TireFriction::GetAdherentSlipForward(const ContactPatch& cp) {
    const float x = cp.slip.x;
    const float adherentSlip = m_curve->GetAdherentSlip(cp);
    if (!(MathUtility::FastAbs(x) < adherentSlip)) return 0.0f;
    return Mathf::Sqrt(f32store(fx(adherentSlip) * fx(adherentSlip) - fx(x) * fx(x)));
}

// VehiclePhysics/TireFriction.cs:468
float TireFriction::GetAdherentSlipSideways(const ContactPatch& cp) {
    const float y = cp.slip.y;
    const float adherentSlip = m_curve->GetAdherentSlip(cp);
    if (!(MathUtility::FastAbs(y) < adherentSlip)) return 0.0f;
    return Mathf::Sqrt(f32store(fx(adherentSlip) * fx(adherentSlip) - fx(y) * fx(y)));
}

// VehiclePhysics/TireFriction.cs:479
Vector2f TireFriction::GetAdherentSlipBounds(const ContactPatch& cp) {
    const float adherentSlip = m_curve->GetAdherentSlip(cp);
    return Vector2f(adherentSlip, adherentSlip);
}

// VehiclePhysics/TireFriction.cs:485
Vector2f TireFriction::GetPeakSlipBounds(const ContactPatch& cp) {
    const float peakSlip = m_curve->GetPeakSlip(cp);
    return Vector2f(peakSlip, peakSlip);
}

// VehiclePhysics/TireFriction.cs:491
Vector2f TireFriction::GetLimitSlipBounds(const ContactPatch& cp) {
    const float limitSlip = m_curve->GetLimitSlip(cp);
    return Vector2f(limitSlip, limitSlip);
}

// VehiclePhysics/TireFriction.cs:497
Vector2f TireFriction::GetAdherentForce(const ContactPatch& cp) {
    const float adherentSlip = m_curve->GetAdherentSlip(cp);
    const float d = f32store(0.0 - fx(m_curve->EvaluateForce(adherentSlip, cp)) * fx(frictionMultiplier) / fx(adherentSlip));
    return d * cp.slip;
}

}  // namespace ksp::VehiclePhysics
