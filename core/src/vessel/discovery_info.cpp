// DiscoveryInfo.cs (see include/ksp/vessel/discovery_info.hpp)
#include "ksp/vessel/discovery_info.hpp"

#include <limits>

#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/unity/random.hpp"

namespace ksp {

// DiscoveryInfo.cs:138
DiscoveryInfo::DiscoveryInfo(IDiscoverable* host)
    : host_(host), level_(DiscoveryLevels::Owned), unobservedLifetime_(std::numeric_limits<double>::infinity()),
      referenceLifetime_(std::numeric_limits<double>::infinity()), objectSize_(UntrackedObjectClass::C) {}

// DiscoveryInfo.cs:151
DiscoveryInfo::DiscoveryInfo(IDiscoverable* host, double untrackedLifetime)
    : host_(host), level_(DiscoveryLevels::Owned), unobservedLifetime_(untrackedLifetime), referenceLifetime_(untrackedLifetime * 2.0),
      objectSize_(UntrackedObjectClass::C) {}

// DiscoveryInfo.cs:164
DiscoveryInfo::DiscoveryInfo(IDiscoverable* host, DiscoveryLevels level, double untrackedLifetime)
    : host_(host), level_(level), unobservedLifetime_(untrackedLifetime), referenceLifetime_(untrackedLifetime * 2.0),
      objectSize_(UntrackedObjectClass::C) {}

// DiscoveryInfo.cs:177
void DiscoveryInfo::SetLevel(DiscoveryLevels level) {
    const DiscoveryLevels level2 = level_;
    level_ = level;
    events::GameEvents::onKnowledgeChanged.Fire(
        events::GameEvents::HostedFromToAction<IDiscoverable*, DiscoveryLevels>(host_, level2, level_));
}

// DiscoveryInfo.cs:184
void DiscoveryInfo::SetLastObservedTime(double UT) {
    lastObservedTime_ = UT;
    fadeUT_ = UT + unobservedLifetime_;
}

// DiscoveryInfo.cs:190
void DiscoveryInfo::SetUnobservedLifetime(double time) {
    unobservedLifetime_ = time;
    fadeUT_ = lastObservedTime_ + time;
}

// DiscoveryInfo.cs:196
void DiscoveryInfo::SetReferenceLifetime(double time) { referenceLifetime_ = time; }

// DiscoveryInfo.cs:201
void DiscoveryInfo::SetUntrackedObjectSize(UntrackedObjectClass size) { objectSize_ = size; }

namespace {
double CsMax(double a, double b) { return (a > b || a != a) ? a : b; }  // System.Math.Max (NaN-propagating)
double CsMin(double a, double b) { return (a < b || a != a) ? a : b; }  // System.Math.Min
}  // namespace

// DiscoveryInfo.cs:221
double DiscoveryInfo::GetSignalStrength(double UT) const {
    return CsMax(0.0, CsMin(1.0, unobservedLifetime_ / referenceLifetime_)) * GetSignalLife(UT);
}

// DiscoveryInfo.cs:226
double DiscoveryInfo::GetSignalLife(double UT) const {
    if (HaveKnowledgeAbout(DiscoveryLevels::StateVectors)) return 1.0;
    return CsMax(0.0, CsMin(1.0, 1.0 - (UT - lastObservedTime_) / (fadeUT_ - lastObservedTime_)));
}

// DiscoveryInfo.cs:235
bool DiscoveryInfo::HaveKnowledgeAbout(DiscoveryLevels lvl) const {
    return (static_cast<int>(lvl) & static_cast<int>(level_)) != 0;
}

// DiscoveryInfo.cs:240
void DiscoveryInfo::Load(const ConfigNode& node) {
    if (node.HasValue("state")) level_ = static_cast<DiscoveryLevels>(dotnet::ParseInt32(*node.GetValue("state")));
    if (node.HasValue("lastObservedTime")) lastObservedTime_ = dotnet::ParseDouble(*node.GetValue("lastObservedTime"));
    if (node.HasValue("lifetime")) unobservedLifetime_ = dotnet::ParseDouble(*node.GetValue("lifetime"));
    if (node.HasValue("refTime")) referenceLifetime_ = dotnet::ParseDouble(*node.GetValue("refTime"));
    if (node.HasValue("size")) objectSize_ = static_cast<UntrackedObjectClass>(dotnet::ParseInt32(*node.GetValue("size")));
    fadeUT_ = lastObservedTime_ + unobservedLifetime_;
}

// DiscoveryInfo.cs:265
void DiscoveryInfo::Save(ConfigNode& node) const {
    node.AddValue("state", static_cast<int32_t>(level_));
    node.AddValue("lastObservedTime", lastObservedTime_);
    node.AddValue("lifetime", unobservedLifetime_);
    node.AddValue("refTime", referenceLifetime_);
    node.AddValue("size", static_cast<int32_t>(objectSize_));
}

// DiscoveryInfo.cs:349
UntrackedObjectClass DiscoveryInfo::GetClassFromSize(float size, int seed) {
    UntrackedObjectClass result = UntrackedObjectClass::A;
    // Enum.GetValues order: A..I
    for (int i = 0; i <= static_cast<int>(UntrackedObjectClass::I); i++) {
        const auto value = static_cast<UntrackedObjectClass>(i);
        if (GetMinRadius(value, seed) < size && GetMaxRadius(value, seed) > size) result = value;
    }
    return result;
}

// DiscoveryInfo.cs:362
float DiscoveryInfo::GetClassRadius(UntrackedObjectClass sizeClass, int seed) {
    UnityRandom::Global().InitState(seed);
    float num;
    switch (sizeClass) {
        default: num = 0.0f; break;
        case UntrackedObjectClass::A: num = 3.24f; break;
        case UntrackedObjectClass::B: num = 5.4f; break;
        case UntrackedObjectClass::C: num = 9.0f; break;
        case UntrackedObjectClass::D: num = 15.0f; break;
        case UntrackedObjectClass::E: num = 25.0f; break;
        case UntrackedObjectClass::F: num = 41.7f; break;
        case UntrackedObjectClass::G: num = 69.5f; break;
        case UntrackedObjectClass::H: num = 115.8f; break;
        case UntrackedObjectClass::I: num = 193.0f; break;
    }
    return f32store(fx(num) * fx(UnityRandom::Global().Range(0.75f, 1.25f)));
}

// DiscoveryInfo.cs:402
UntrackedObjectClass DiscoveryInfo::GetObjectClass(const std::string& classString) {
    if (classString == "D") return UntrackedObjectClass::D;
    if (classString == "E") return UntrackedObjectClass::E;
    if (classString == "F") return UntrackedObjectClass::F;
    if (classString == "G") return UntrackedObjectClass::G;
    if (classString == "C") return UntrackedObjectClass::C;
    if (classString == "A") return UntrackedObjectClass::A;
    if (classString == "H") return UntrackedObjectClass::H;
    if (classString == "I") return UntrackedObjectClass::I;
    if (classString == "B") return UntrackedObjectClass::B;
    return UntrackedObjectClass::A;
}

// DiscoveryInfo.cs:430
float DiscoveryInfo::GetMinRadius(UntrackedObjectClass sizeClass, int seed) {
    return f32store(fx(GetClassRadius(sizeClass, seed)) * fx(0.75f));
}

// DiscoveryInfo.cs:435
float DiscoveryInfo::GetMaxRadius(UntrackedObjectClass sizeClass, int seed) {
    return f32store(fx(GetClassRadius(sizeClass, seed)) * fx(1.25f));
}

}  // namespace ksp
