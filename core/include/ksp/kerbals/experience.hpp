// Experience/ExperienceEffect.cs, Experience/ExperienceTrait.cs and the 20 effects of Experience/Effects/*.cs (WP-31).
// The config side (ExperienceSystemConfig / ExperienceTraitConfig / ExperienceEffectConfig) is WP-04's (ksp/save/crew.hpp).
//
// C# reflection (KerbalRoster.GetExperienceEffectType / GetExperienceTraitType) becomes a name -> factory registry:
// `ExperienceEffectFactory::Create(name, trait)` knows exactly the ExperienceEffect subclasses of Assembly-CSharp; a trait
// type registry exists for mods (none in stock: every trait is a plain ExperienceTrait).
//
// Effects register delegates in Part.PartValues (OnRegister) that Vessel.VesselValues reads. A delegate holds a raw
// pointer to its effect: Unregister before the trait dies (KSP does, ProtoCrewMember.UnregisterExperienceTraits).
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/save/crew.hpp"

namespace ksp {
class Part;
class ProtoCrewMember;
}  // namespace ksp

namespace ksp::Experience {

class ExperienceTrait;

// Experience/ExperienceEffect.cs
class ExperienceEffect {
public:
    // :22 (the only constructor that sets the parent)
    explicit ExperienceEffect(ExperienceTrait* parent) : parent_(parent) {}
    // :27 -- sic: the parent is never stored and modifiers.CopyTo(LevelModifiers = null) throws ArgumentNullException
    ExperienceEffect(ExperienceTrait* parent, const std::vector<float>& modifiers);
    // :32 -- sic: the parent is not stored
    ExperienceEffect(ExperienceTrait* parent, int level);
    virtual ~ExperienceEffect() = default;

    ExperienceTrait* Parent() const { return parent_; }      // :9
    std::string Description() { return GetDescription(); }   // :11
    const std::string& Name();                                // :13 (name ??= GetType().Name)
    // :26 (null until a `modifiers` config value was loaded: NullReferenceException on access, like C#)
    const std::vector<float>& LevelModifiers() const;
    bool HasLevelModifiers() const { return levelModifiers_.has_value(); }
    int Level() const { return level_; }                      // :28

    void Register(Part& part) { OnRegister(part); }           // :40
    void Unregister(Part& part) { OnUnregister(part); }       // :45
    void LoadFromConfig(const ConfigNode& node);              // :50

    // the C# type's Name (what GetType().Name returns)
    virtual const char* ClassName() const = 0;

protected:
    virtual std::string GetDescription() { return ""; }       // :80
    virtual void OnRegister(Part&) {}                         // :85
    virtual void OnUnregister(Part&) {}                       // :89
    virtual void OnLoadFromConfig(const ConfigNode&) {}       // :93
    virtual float GetDefaultValue() { return 0.0f; }          // :97
    // LevelModifiers[index] with C# array semantics (NullReferenceException / IndexOutOfRangeException)
    float LevelModifier(int index) const;

private:
    ExperienceTrait* parent_ = nullptr;
    std::string name_;
    bool haveName_ = false;
    std::optional<std::vector<float>> levelModifiers_;
    int level_ = 0;
};

// Experience/ExperienceTrait.cs
class ExperienceTrait {
public:
    ExperienceTrait() = default;
    virtual ~ExperienceTrait() = default;
    ExperienceTrait(const ExperienceTrait&) = delete;
    ExperienceTrait& operator=(const ExperienceTrait&) = delete;

    ExperienceTraitConfig* Config() const { return config_; }    // :14
    ProtoCrewMember* CrewMember() const { return protoCrewMember_; }  // :16
    virtual std::string TypeName() const;                       // :18 (Config.Title for the plain type, else the class name)
    std::string Title() const;                                  // :31 (Config.Title: NullReferenceException when null config)
    std::string Description() const;                            // :33
    std::string DescriptionEffects();                           // :35
    std::vector<std::unique_ptr<ExperienceEffect>>& Effects() { return effects_; }  // :56
    const std::vector<std::unique_ptr<ExperienceEffect>>& Effects() const { return effects_; }

    // :58 (typeName empty means the plain ExperienceTrait; config == null logs an error and returns nullptr)
    static std::shared_ptr<ExperienceTrait> Create(const std::string& typeName, ExperienceTraitConfig* config,
                                                   ProtoCrewMember* pcm);
    int CrewMemberExperienceLevel(int levelCap = -1) const;    // :104
    void Register(Part& part);                                  // :134
    void Unregister(Part& part);                                // :144

    // KerbalRoster.GetExperienceTraitType registry (mods): typeName -> factory of the subclass
    using TraitFactory = std::function<std::shared_ptr<ExperienceTrait>()>;
    static void RegisterTraitType(const std::string& typeName, TraitFactory factory);
    static bool HasTraitType(const std::string& typeName);

protected:
    virtual void OnLoad(const ConfigNode&) {}      // :113
    virtual void OnSave(ConfigNode&) {}            // :117
    virtual void OnRegister(Part&) {}              // :121
    virtual void OnUnregister(Part&) {}            // :125
    // :154 SendStateMessage: MessageSystem is UI; the Debug.Log line is kept
    void SendStateMessage(const std::string& title, const std::string& message);

private:
    void SetupConfig(ExperienceTraitConfig* cfg);               // :74
    void AddEffect(ExperienceEffectConfig& cfg);                // :83
    ProtoCrewMember* protoCrewMember_ = nullptr;
    std::vector<std::unique_ptr<ExperienceEffect>> effects_;
    ExperienceTraitConfig* config_ = nullptr;
};

// KerbalRoster.GetExperienceEffectType + Activator.CreateInstance(type, trait)
class ExperienceEffectFactory {
public:
    // nullptr when no ExperienceEffect subclass of that class name exists
    static std::unique_ptr<ExperienceEffect> Create(const std::string& typeName, ExperienceTrait* parent);
    static bool Exists(const std::string& typeName);
    static const std::vector<std::string>& TypeNames();  // the 20 stock subclasses
};

}  // namespace ksp::Experience

namespace ksp {
// VesselAutopilot.AutopilotMode (VesselAutopilot.cs:849)
enum class VesselAutopilot_AutopilotMode : int {
    StabilityAssist, Prograde, Retrograde, Normal, Antinormal, RadialIn, RadialOut, Target, AntiTarget, Maneuver
};
}  // namespace ksp

namespace ksp::Experience::Effects {

// Experience/Effects/AutopilotSkill.cs
class AutopilotSkill : public ExperienceEffect {
public:
    enum Skills { StabilityAssist = 0, Prograde = 1, Retrograde = 1, Normal = 2, Antinormal = 2, RadialIn = 2, RadialOut = 2,
                  Target = 3, AntiTarget = 3, Maneuver = 3 };
    // :20 (static initialiser: Localizer.Format at the first use of the class, then Format again in GetDescription)
    static std::vector<std::string>& SkillsReadable();
    explicit AutopilotSkill(ExperienceTrait* parent);
    AutopilotSkill(ExperienceTrait* parent, const std::vector<float>& modifiers) : ExperienceEffect(parent, modifiers) {}
    const char* ClassName() const override { return "AutopilotSkill"; }
protected:
    float GetDefaultValue() override { return 0.0f; }
    std::string GetDescription() override;
    void OnRegister(Part& part) override;
    void OnUnregister(Part& part) override;
private:
    int GetValue();
};

// Experience/Effects/ConverterSkill.cs
class ConverterSkill : public ExperienceEffect {
public:
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "ConverterSkill"; }
protected:
    float GetDefaultValue() override { return 0.0f; }
    std::string GetDescription() override;
};

// Experience/Effects/DeployedScienceExpSkill.cs
class DeployedScienceExpSkill : public ExperienceEffect {
public:
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "DeployedScienceExpSkill"; }
    float GetValue();  // public
protected:
    float GetDefaultValue() override;
    std::string GetDescription() override;
};

// Experience/Effects/DeployedSciencePowerSkill.cs
class DeployedSciencePowerSkill : public ExperienceEffect {
public:
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "DeployedSciencePowerSkill"; }
    float GetValue();                // public
    float GetTraitBonus(int level);  // internal
protected:
    float GetDefaultValue() override;
    std::string GetDescription() override;
};

// Experience/Effects/DrillSkill.cs
class DrillSkill : public ExperienceEffect {
public:
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "DrillSkill"; }
protected:
    float GetDefaultValue() override { return 0.0f; }
    std::string GetDescription() override;
};

// Experience/Effects/EnginePower.cs
class EnginePower : public ExperienceEffect {
public:
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "EnginePower"; }
protected:
    float GetDefaultValue() override { return 1.0f; }
    std::string GetDescription() override;
    void OnRegister(Part& part) override;
    void OnUnregister(Part& part) override;
private:
    float GetValue();
};

// Experience/Effects/EVAChuteSkill.cs
class EVAChuteSkill : public ExperienceEffect {
public:
    explicit EVAChuteSkill(ExperienceTrait* parent) : ExperienceEffect(parent) {}
    EVAChuteSkill(ExperienceTrait* parent, int level) : ExperienceEffect(parent, level) {}
    const char* ClassName() const override { return "EVAChuteSkill"; }
    int GetValue();  // public
protected:
    void OnRegister(Part& part) override;
    void OnUnregister(Part& part) override;
};

// Experience/Effects/ExternalExperimentSkill.cs
class ExternalExperimentSkill : public ExperienceEffect {
public:
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "ExternalExperimentSkill"; }
protected:
    float GetDefaultValue() override { return 0.0f; }
    std::string GetDescription() override;
};

// Experience/Effects/FailureRepairSkill.cs
class FailureRepairSkill : public ExperienceEffect {
public:
    enum Skills { Failures = 1 };
    static std::vector<std::string>& SkillsReadable();  // { "Repair Failures" }
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "FailureRepairSkill"; }
protected:
    float GetDefaultValue() override { return 0.0f; }
    std::string GetDescription() override;
    void OnRegister(Part& part) override;
    void OnUnregister(Part& part) override;
private:
    int GetValue();
};

// Experience/Effects/FuelUsage.cs
class FuelUsage : public ExperienceEffect {
public:
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "FuelUsage"; }
protected:
    float GetDefaultValue() override { return 1.0f; }
    std::string GetDescription() override;
    void OnRegister(Part& part) override;
    void OnUnregister(Part& part) override;
private:
    float GetValue();
};

// Experience/Effects/FullVesselControlSkill.cs
class FullVesselControlSkill : public ExperienceEffect {
public:
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "FullVesselControlSkill"; }
protected:
    float GetDefaultValue() override { return 0.0f; }
    std::string GetDescription() override;
};

// Experience/Effects/GeeForceTolerance.cs
class GeeForceTolerance : public ExperienceEffect {
public:
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "GeeForceTolerance"; }
    virtual float GeeTolerance();  // public virtual
protected:
    float GetDefaultValue() override { return 1.0f; }
    std::string GetDescription() override;
};

// Experience/Effects/HeatProduction.cs
class HeatProduction : public ExperienceEffect {
public:
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "HeatProduction"; }
protected:
    float GetDefaultValue() override { return 1.0f; }
    std::string GetDescription() override;
    void OnRegister(Part& part) override;
    void OnUnregister(Part& part) override;
private:
    float GetValue();
};

// Experience/Effects/MaxThrottle.cs
class MaxThrottle : public ExperienceEffect {
public:
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "MaxThrottle"; }
protected:
    float GetDefaultValue() override { return 1.0f; }
    std::string GetDescription() override;
    void OnRegister(Part& part) override;
    void OnUnregister(Part& part) override;
private:
    float GetValue();
};

// Experience/Effects/PartScienceReturn.cs
class PartScienceReturn : public ExperienceEffect {
public:
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "PartScienceReturn"; }
protected:
    float GetDefaultValue() override { return 1.0f; }
    std::string GetDescription() override;
    void OnRegister(Part& part) override;
    void OnUnregister(Part& part) override;
private:
    float GetValue();
};

// Experience/Effects/RepairSkill.cs
class RepairSkill : public ExperienceEffect {
public:
    enum Skills { Parachutes = 1, Wheels = 3, LandingLegs = 2 };
    static std::vector<std::string>& SkillsReadable();  // 3 Localizer.Format results
    explicit RepairSkill(ExperienceTrait* parent);
    RepairSkill(ExperienceTrait* parent, const std::vector<float>& modifiers) : ExperienceEffect(parent, modifiers) {}
    const char* ClassName() const override { return "RepairSkill"; }
protected:
    float GetDefaultValue() override { return 0.0f; }
    std::string GetDescription() override;
    void OnRegister(Part& part) override;
    void OnUnregister(Part& part) override;
private:
    int GetValue();
};

// Experience/Effects/ScienceResetSkill.cs
class ScienceResetSkill : public ExperienceEffect {
public:
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "ScienceResetSkill"; }
protected:
    float GetDefaultValue() override { return 0.0f; }
    std::string GetDescription() override;
};

// Experience/Effects/ScienceSkill.cs
class ScienceSkill : public ExperienceEffect {
public:
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "ScienceSkill"; }
protected:
    float GetDefaultValue() override { return 0.0f; }
    std::string GetDescription() override { return ""; }
    void OnRegister(Part& part) override;
    void OnUnregister(Part& part) override;
private:
    int GetValue();
};

// Experience/Effects/SpecialExperimentSkill.cs
class SpecialExperimentSkill : public ExperienceEffect {
public:
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "SpecialExperimentSkill"; }
protected:
    float GetDefaultValue() override { return 0.0f; }
    std::string GetDescription() override { return ""; }
};

// Experience/Effects/VesselScienceReturn.cs
class VesselScienceReturn : public ExperienceEffect {
public:
    using ExperienceEffect::ExperienceEffect;
    const char* ClassName() const override { return "VesselScienceReturn"; }
protected:
    float GetDefaultValue() override { return 1.0f; }
    std::string GetDescription() override;
    void OnRegister(Part& part) override;
    void OnUnregister(Part& part) override;
private:
    float GetValue();
};

}  // namespace ksp::Experience::Effects

namespace ksp {
// ProtoCrewMember.cs:1309 GetEffect<T>(): the last effect of the trait that is a T (NullReferenceException without a trait)
template <class T>
T* ProtoCrewMember::GetEffect() {
    if (experienceTrait == nullptr) throw NullReferenceException("ProtoCrewMember.experienceTrait is null");
    auto& effects = experienceTrait->Effects();
    int count = static_cast<int>(effects.size());
    while (count-- > 0)
        if (T* val = dynamic_cast<T*>(effects[static_cast<size_t>(count)].get())) return val;
    return nullptr;
}
}  // namespace ksp
