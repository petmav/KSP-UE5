// DiscoveryInfo.cs (490 lines; listed under WP-09 but not ported there; the vessel model needs it: Vessel.discoveryInfo,
// ProtoVessel's DISCOVERY node, Vessel.GetClassFromSize...). Data side: levels, lifetimes, signal strength, Load / Save and the
// size-class maths. Not ported: the KnowledgeItem<T> getters of the tracking-station UI (CompileInfoGetters, :111-123),
// the localised captions (GetSignalStrengthCaption, GetSizeClassDescription, GetSizeClassSizes) and CacheLocalStrings.
#pragma once

#include <string>

#include "ksp/config/config_node.hpp"

namespace ksp {

class IDiscoverable;

// DiscoveryLevels.cs ([Flags]); GameEvents declares the enum opaque
enum class DiscoveryLevels : int { None = 0, Presence = 1, Name = 4, StateVectors = 8, Appearance = 0x10, Unowned = 0x1D, Owned = -1 };

// UntrackedObjectClass.cs
enum class UntrackedObjectClass : int { A, B, C, D, E, F, G, H, I };

class DiscoveryInfo {
public:
    DiscoveryLevels Level() const { return level_; }             // :126
    double lastObservedTime() const { return lastObservedTime_; }  // :128
    double fadeUT() const { return fadeUT_; }                      // :130
    double unobservedLifetime() const { return unobservedLifetime_; }  // :132
    double referenceLifetime() const { return referenceLifetime_; }    // :134
    UntrackedObjectClass objectSize() const { return objectSize_; }    // :136

    explicit DiscoveryInfo(IDiscoverable* host);                                         // :138
    DiscoveryInfo(IDiscoverable* host, double untrackedLifetime);                        // :151
    DiscoveryInfo(IDiscoverable* host, DiscoveryLevels level, double untrackedLifetime);  // :164

    void SetLevel(DiscoveryLevels level);        // :177 (fires GameEvents.onKnowledgeChanged)
    void SetLastObservedTime(double UT);         // :184
    void SetUnobservedLifetime(double time);     // :190
    void SetReferenceLifetime(double time);      // :196
    void SetUntrackedObjectSize(UntrackedObjectClass size);  // :201

    double GetSignalStrength(double UT) const;   // :221
    double GetSignalLife(double UT) const;       // :226
    bool HaveKnowledgeAbout(DiscoveryLevels lvl) const;  // :235
    void Load(const ConfigNode& node);           // :240
    void Save(ConfigNode& node) const;           // :265

    static UntrackedObjectClass GetClassFromSize(float size, int seed);  // :349
    static float GetClassRadius(UntrackedObjectClass sizeClass, int seed);  // :362 (UnityEngine.Random.InitState(seed); Range(0.75, 1.25))
    static UntrackedObjectClass GetObjectClass(const std::string& classString);  // :402
    static float GetMinRadius(UntrackedObjectClass sizeClass, int seed);  // :430
    static float GetMaxRadius(UntrackedObjectClass sizeClass, int seed);  // :435

private:
    IDiscoverable* host_ = nullptr;
    DiscoveryLevels level_ = DiscoveryLevels::Owned;
    double lastObservedTime_ = 0.0;
    double fadeUT_ = 0.0;
    double unobservedLifetime_ = 0.0;
    double referenceLifetime_ = 0.0;
    UntrackedObjectClass objectSize_ = UntrackedObjectClass::C;
};


// IDiscoverable.cs
class IDiscoverable {
public:
    virtual ~IDiscoverable() = default;
    virtual DiscoveryInfo& GetDiscoveryInfo() = 0;  // DiscoveryInfo property
    virtual std::string RevealName() = 0;
    virtual std::string RevealDisplayName() = 0;
    virtual double RevealSpeed() = 0;
    virtual double RevealAltitude() = 0;
    virtual std::string RevealSituationString() = 0;
    virtual std::string RevealType() = 0;
    virtual float RevealMass() = 0;
};

}  // namespace ksp
