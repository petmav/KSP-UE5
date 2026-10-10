// ProtoCrewMember.cs members beyond WP-04's data side (WP-31): experience traits and effects, g-force, per-frame callbacks,
// names of the EVA / IVA prefabs, icon paths, Equals / GetHashCode. The data side is src/save/crew.cpp.
#include <cmath>
#include <cstring>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/enum_extensions.hpp"
#include "ksp/util/misc_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using Localizer = KSP::Localization::Localizer;
using GE = events::GameEvents;
using Suit = ProtoCrewMember_KerbalSuit;

namespace {
GameParameters& CurrentParameters() {
    Game* g = SaveEnvironment::Current().currentGame;
    if (g == nullptr || !g->Parameters) throw NullReferenceException("HighLogic.CurrentGame.Parameters is null");
    return *g->Parameters;
}
void Post(const std::string& text, float duration) {
    if (ProtoCrewMember::PostScreenMessage) ProtoCrewMember::PostScreenMessage(text, duration);
}
// float.Equals / double.Equals: == or both NaN
template <class T>
bool NumEquals(T a, T b) {
    return a == b || (std::isnan(a) && std::isnan(b));
}
int32_t FloatHash(float v) {  // Single.GetHashCode (Mono/mscorlib/System/Single.cs:177)
    if (v == 0.0f) return 0;
    int32_t r;
    std::memcpy(&r, &v, sizeof r);
    return r;
}
int32_t DoubleHash(double v) {  // Double.GetHashCode (Double.cs:191)
    if (v == 0.0) return 0;
    int64_t l;
    std::memcpy(&l, &v, sizeof l);
    return static_cast<int32_t>(l) ^ static_cast<int32_t>(l >> 32);
}
}  // namespace

// ProtoCrewMember.cs:193 (LingoonaGrammarExtensions.LocalizeName -> LocalizeNoun(gender, isProperNoun: true))
std::string ProtoCrewMember::nameWithGender() const {
    if (!name_) throw NullReferenceException("ProtoCrewMember._name is null");
    std::string input = *name_;
    const size_t pos = input.rfind('^');  // LocalizeRemoveGender
    if (pos != std::string::npos) input.erase(pos);
    return input + (gender() == Gender::Male ? "^M" : "^F");
}

// ProtoCrewMember.cs:673 (EnumExtensions.Description: the raw [Description] tag, not localized)
std::string ProtoCrewMember::GetLocalizedType() const { return util::Description(type()); }
// ProtoCrewMember.cs:678
std::string ProtoCrewMember::GetLocalizedStatus() const { return util::Description(rosterStatus()); }
// ProtoCrewMember.cs:683
std::string ProtoCrewMember::GetLocalizedTrait() const { return KerbalRoster::GetLocalizedExperienceTraitName(trait); }

// ProtoCrewMember.cs:1054
std::string ProtoCrewMember::GetKerbalIVAName() {
    const SaveEnvironment& env = SaveEnvironment::Current();
    if (gender() == Gender::Female) {
        switch (suit()) {
        case Suit::Vintage:
            if (env.IsExpansionKerbalSuitInstalled(Suit::Vintage)) return "kerbalFemaleVintage";
            return "kerbalFemale";
        case Suit::Future:
            if (env.IsExpansionKerbalSuitInstalled(Suit::Future)) return "kerbalFemaleFuture";
            return "kerbalFemale";
        case Suit::Slim: return "slimSuitIVAFemale";
        default: return "kerbalFemale";
        }
    }
    switch (suit()) {
    case Suit::Vintage:
        if (env.IsExpansionKerbalSuitInstalled(Suit::Vintage)) return "kerbalMaleVintage";
        return "kerbalMale";
    case Suit::Future:
        if (env.IsExpansionKerbalSuitInstalled(Suit::Future)) return "kerbalMaleFuture";
        return "kerbalMale";
    case Suit::Slim: return "slimSuitIVAMale";
    default: return "kerbalMale";
    }
}

// ProtoCrewMember.cs:1100
std::string ProtoCrewMember::GetKerbalIconSuitSuffix() {
    const SaveEnvironment& env = SaveEnvironment::Current();
    switch (suit()) {
    case Suit::Vintage: return env.IsExpansionKerbalSuitInstalled(Suit::Vintage) ? "_vintage" : "";
    case Suit::Future: return env.IsExpansionKerbalSuitInstalled(Suit::Future) ? "_future" : "";
    case Suit::Slim: return "_slim";
    default: return "";
    }
}

// ProtoCrewMember.cs:1123
std::string ProtoCrewMember::GetKerbalIconSuitPath() {
    if (SpritePath.has_value()) return *SpritePath;
    const std::string suffix = GetKerbalIconSuitSuffix();
    // the suffixed variants: _vintage / _future live under Kerbals/Textures/kerbalIcons/, _slim and "" are bare names
    auto suited = [&](const std::string& baseName) -> std::string {
        if (suffix == "_vintage" || suffix == "_future") return "Kerbals/Textures/kerbalIcons/" + baseName + suffix;
        if (suffix == "_slim") return baseName + suffix;
        return baseName;
    };
    if (gender() == Gender::Female) {
        if (veteran) return suited("kerbalicon_suit_orange_female");
        switch (type()) {
        case KerbalType::Applicant: return "kerbalicon_recruit_female";
        case KerbalType::Unowned:
        case KerbalType::Tourist: return "kerbalicon_tourist_female";
        default: return suited("kerbalicon_suit_female");
        }
    }
    if (veteran) return suited("kerbalicon_suit_orange");
    switch (type()) {
    case KerbalType::Applicant: return "kerbalicon_recruit";
    case KerbalType::Unowned:
    case KerbalType::Tourist: return "kerbalicon_tourist";
    default: return suited("kerbalicon_suit");
    }
}

// ProtoCrewMember.cs:1296
void ProtoCrewMember::RegisterExperienceTraits(Part& part) {
    if (experienceTrait != nullptr) experienceTrait->Register(part);
}

// ProtoCrewMember.cs:1304
void ProtoCrewMember::UnregisterExperienceTraits(Part& part) {
    if (experienceTrait != nullptr) experienceTrait->Unregister(part);
}

// ProtoCrewMember.cs:1317
bool ProtoCrewMember::HasEffect(const std::string& effect) { return GetEffect(effect) != nullptr; }

// ProtoCrewMember.cs:1339
Experience::ExperienceEffect* ProtoCrewMember::GetEffect(const std::string& effect) {
    if (experienceTrait == nullptr) throw NullReferenceException("ProtoCrewMember.experienceTrait is null");
    auto& effects = experienceTrait->Effects();
    int count = static_cast<int>(effects.size());
    do {
        if (count-- <= 0) return nullptr;
    } while (!(effects[static_cast<size_t>(count)]->Name() == effect));
    return effects[static_cast<size_t>(count)].get();
}

// ProtoCrewMember.cs:1358 (sic: `hasToured.Equals(hasToured)` compares the field with itself)
bool ProtoCrewMember::Equals(const ProtoCrewMember& o) const {
    if (!name_ || !o.name_) throw NullReferenceException("ProtoCrewMember.Equals: _name is null");
    if (*name_ == *o.name_ && NumEquals(stupidity, o.stupidity) && NumEquals(courage, o.courage) && isBadass == o.isBadass &&
        veteran == o.veteran && hasToured == hasToured)
        return NumEquals(UTaR, o.UTaR);
    return false;
}

// ProtoCrewMember.cs:1371
int32_t ProtoCrewMember::GetHashCode() const {
    if (!name_) throw NullReferenceException("ProtoCrewMember.GetHashCode: _name is null");
    return StringExtensions::GetHashCode_Net35(*name_) ^ FloatHash(stupidity) ^ FloatHash(courage) ^ (isBadass ? 1 : 0) ^
           (veteran ? 1 : 0) ^ (hasToured ? 1 : 0) ^ DoubleHash(UTaR);
}

// ProtoCrewMember.cs:149
double ProtoCrewMember::GExperiencedNormalized() const {
    const double mult = GToleranceMult(const_cast<ProtoCrewMember&>(*this));
    return gExperienced / (PhysicsGlobals::KerbalGThresholdLOC() * mult *
                           static_cast<double>(CurrentParameters().CustomParams<GameParameters::AdvancedParams>().KerbalGToleranceMult));
}

// ProtoCrewMember.cs:856
double ProtoCrewMember::_GToleranceMult(ProtoCrewMember& pcm) {
    double num = 1.0 + (PhysicsGlobals::KerbalGBraveMult() - 1.0) * static_cast<double>(pcm.courage);
    if (pcm.experienceTrait == nullptr) throw NullReferenceException("ProtoCrewMember.experienceTrait is null");
    Experience::Effects::GeeForceTolerance* effect = pcm.GetEffect<Experience::Effects::GeeForceTolerance>();
    if (effect != nullptr) num *= static_cast<double>(effect->GeeTolerance());
    if (pcm.isBadass) num *= PhysicsGlobals::KerbalGBadMult();
    return num;
}

// ProtoCrewMember.cs:871
double ProtoCrewMember::_MaxSustainedG(ProtoCrewMember& pcm) {
    return dotnet::Math::Pow(PhysicsGlobals::KerbalGOffset() * GToleranceMult(pcm), 1.0 / PhysicsGlobals::KerbalGPower());
}

// ProtoCrewMember.cs:876
void ProtoCrewMember::ActiveFixedUpdate(Part& p) {
    if (p.vessel == nullptr) throw NullReferenceException("ProtoCrewMember.ActiveFixedUpdate: p.vessel is null");
    geeForce = p.vessel->geeForce;
    GameParameters& gp = CurrentParameters();
    auto& adv = gp.CustomParams<GameParameters::AdvancedParams>();
    if (adv.GKerbalLimits) {
        const double num = static_cast<double>(TimeWarp::fixedDeltaTime());
        const double num2 = GToleranceMult(*this) * static_cast<double>(adv.KerbalGToleranceMult);
        if (doStockGCalcs) {
            gIncrement = (dotnet::Math::Pow(dotnet::Math::Min(geeForce, PhysicsGlobals::KerbalGClamp()), PhysicsGlobals::KerbalGPower()) -
                          PhysicsGlobals::KerbalGOffset() * num2) * num;
            if (gIncrement < 0.0) {
                gIncrement = dotnet::Math::Pow(0.0 - gIncrement, PhysicsGlobals::KerbalGDecayPower());
                gIncrement = 0.0 - gIncrement;
            }
            gIncrement *= 1.0 + (static_cast<double>(UnityRandom::Global().Range(0.0f, 1.0f)) * 0.1 - 0.05);
            gExperienced += gIncrement;
        } else if (ApplyGCallback) {
            ApplyGCallback(*this, p);
        }
        if (gExperienced < 0.0) {
            gExperienced = 0.0;
        } else if (gExperienced > PhysicsGlobals::KerbalGThresholdWarn() * num2) {
            if (gExperienced > PhysicsGlobals::KerbalGThresholdLOC() * num2) {
                if (inactive_) {
                    if (PhysicsGlobals::KerbalGClampGExperienced())
                        SetInactive(UtilMath::Clamp(gIncrement, num, PhysicsGlobals::KerbalGLOCMaxTimeIncrement() * num));
                    else
                        SetInactive(num);
                } else {
                    if (FlightRegistry::ActiveVessel == p.vessel)
                        Post(Localizer::Format("#autoLOC_143389", {name_.value_or("")}), 5.0f);
                    outDueToG = true;
                    SetInactive(PhysicsGlobals::KerbalGLOCBaseTime(), false);
                    GE::onKerbalPassedOutFromGeeForce.Fire(this);
                }
                if (PhysicsGlobals::KerbalGClampGExperienced()) gExperienced = PhysicsGlobals::KerbalGThresholdLOC() * num2;
            } else if (outDueToG) {
                SetInactive(num);
            } else if (FlightRegistry::ActiveVessel == p.vessel) {
                Post(Localizer::Format("#autoLOC_143403", {name_.value_or("")}), 3.0f);
            }
        }
    } else {
        gExperienced = 0.0;
    }
    if (outDueToG && !inactive_) {
        outDueToG = false;
        if (FlightRegistry::ActiveVessel == p.vessel) Post(Localizer::Format("#autoLOC_143415", {name_.value_or("")}), 3.0f);
    }
    if (CallbackActiveFixedUpdate) CallbackActiveFixedUpdate(*this, p);
}

// ProtoCrewMember.cs:960
void ProtoCrewMember::ActiveUpdate(Part& p) {
    if (CallbackActiveUpdate) CallbackActiveUpdate(*this, p);
}

// ProtoCrewMember.cs:968
void ProtoCrewMember::Update(double UT) {
    if (inactive_) CheckActive(UT);
    if (CallbackUpdate) CallbackUpdate(*this, UT);
}

// ProtoCrewMember.cs:840
void ProtoCrewMember::OnPartUnpack(Part& p) {
    if (CallbackOnPartUnpack) CallbackOnPartUnpack(*this, p);
}

// ProtoCrewMember.cs:848
void ProtoCrewMember::OnPartPack(Part& p) {
    if (CallbackOnPartPack) CallbackOnPartPack(*this, p);
}

// ProtoCrewMember.cs:761
float ProtoCrewMember::InventoryCosts() {
    float result = 0.0f;
    if (type() == KerbalType::Tourist) return result;
    if (kerbalModuleCost) result = kerbalModuleCost(*this);
    return result;
}

// ProtoCrewMember.cs:775
float ProtoCrewMember::InventoryMass() {
    float result = 0.0f;
    if (type() == KerbalType::Tourist) return result;
    if (kerbalModuleMass) result = kerbalModuleMass(*this);
    return result;
}

// ProtoCrewMember.cs:789
float ProtoCrewMember::ResourceMass() {
    float num = 0.0f;
    if (type() == KerbalType::Tourist) return num;
    if (kerbalPrefabResourceMass) num = kerbalPrefabResourceMass(*this);
    return num;
}

}  // namespace ksp
