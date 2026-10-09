// WP-13: the small enums, records and interfaces of the Part model that have no logic of their own.
//   AttachModes.cs, PartStates.cs, PartSpaceMode.cs, KSPAxisMode.cs, Part.PhysicalSignificance / ShowRigidAttachmentOption /
//   ForceHolder (Part.cs), Attachment.cs, AxisFieldLimit.cs + IAxisFieldLimits.cs, PartValues.cs, PartPointer.cs,
//   PartRawInfo.cs, PartGameObjectInfo.cs, PartHeightQuery.cs, IPartSizeModifier.cs, KSPPart.cs, KSPModelEvent.cs.
// KSPAxisGroup.cs (a [Flags] enum) was ported by WP-04 (save/save_enums.hpp) and is reused; ModifierChangeWhen /
// ModifierStagingSituation, IPartMassModifier and IPartCostModifier are WP-06's (modules/part_module.hpp).
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "ksp/core/delegate.hpp"
#include "ksp/events/event_data.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part_data.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
class BaseAxisField;

// AttachModes.cs
enum class AttachModes : int32_t { STACK = 0, SRF_ATTACH = 1 };
// PartStates.cs
enum class PartStates : int32_t { IDLE = 0, ACTIVE = 1, DEACTIVATED = 2, DEAD = 3, FAILED = 4, PLACEMENT = 5, CARGO = 6 };
// PartSpaceMode.cs
enum class PartSpaceMode : int32_t { Pristine = 0, Current = 1 };
// KSPAxisMode.cs
enum class KSPAxisMode : int32_t { Absolute = 0, Incremental = 1 };
// Part.cs:23 Part.PhysicalSignificance
enum class Part_PhysicalSignificance : int32_t { FULL = 0, NONE = 1 };
// Part.cs:83 Part.ShowRigidAttachmentOption
enum class Part_ShowRigidAttachmentOption : int32_t { Never = 0, Editor = 1, Always = 2 };

// Part.cs:97 Part.ForceHolder
struct Part_ForceHolder {
    Vector3d force;
    Vector3d pos;
    Part_ForceHolder() = default;
    Part_ForceHolder(const Vector3d& f, const Vector3d& p) : force(f), pos(p) {}
};

// Attachment.cs (internal class used by the editor's attachment search)
struct Attachment {
    bool possible = false;
    bool collision = false;
    AttachModes mode = AttachModes::STACK;
    Part* potentialParent = nullptr;
    Part* caller = nullptr;
    AttachNode* callerPartNode = nullptr;
    AttachNode* otherPartNode = nullptr;
    Vector3f position{};
    Quaternionf rotation{};
};

// AxisFieldLimit.cs / IAxisFieldLimits.cs
struct AxisFieldLimit {
    BaseAxisField* limitedField = nullptr;
    Vector2f hardLimits{};
    Vector2f softLimits{};
};
class IAxisFieldLimits {
public:
    virtual ~IAxisFieldLimits() = default;
    virtual bool HasAxisFieldLimits() = 0;
    virtual bool HasAxisFieldLimit(const std::string& fieldName) = 0;
    virtual std::vector<AxisFieldLimit>& GetAxisFieldLimits() = 0;
    virtual AxisFieldLimit* GetAxisFieldLimit(const std::string& fieldName) = 0;
    virtual Vector2f GetHardLimits(const std::string& fieldName) = 0;
    virtual Vector2f GetSoftLimits(const std::string& fieldName) = 0;
    virtual void SetHardLimits(const std::string& fieldName, const Vector2f& newLimits) = 0;
    virtual void SetSoftLimits(const std::string& fieldName, const Vector2f& newLimits) = 0;
    // IAxisFieldLimits.LimitsChanged (Callback<AxisFieldLimit>)
    Delegate<void(AxisFieldLimit*)> LimitsChanged;
};

// IConstruction.cs (the editor / EVA-construction permissions a module can veto)
class IConstruction {
public:
    virtual ~IConstruction() = default;
    virtual bool CanBeDetached() = 0;
    virtual bool CanBeOffset() = 0;
    virtual bool CanBeRotated() = 0;
};

// PartValues.cs: the per-part event value wrapper. The C# constructor reflects over the public fields (declaration order)
// and collects their Update(); here the order is registered explicitly.
class PartValues : public events::EventValueWrapper {
public:
    events::EventValueComparison<float> MaxThrottle;
    events::EventValueComparison<float> HeatProduction;
    events::EventValueComparison<float> FuelUsage;
    events::EventValueComparison<float> EnginePower;
    events::EventValueComparison<float> SteeringRadius;
    events::EventValueComparison<int32_t> AutopilotSkill;
    events::EventValueComparison<int32_t> AutopilotKerbalSkill;
    events::EventValueComparison<int32_t> AutopilotSASSkill;
    events::EventValueComparison<int32_t> RepairSkill;
    events::EventValueComparison<int32_t> FailureRepairSkill;
    events::EventValueComparison<int32_t> ScienceSkill;
    events::EventValueComparison<float> CommsRange;
    events::EventValueOperation<float> ScienceReturnSum;
    events::EventValueComparison<float> ScienceReturnMax;
    events::EventValueComparison<int32_t> EVAChuteSkill;
    PartValues();
    PartValues(const PartValues&) = delete;
    PartValues& operator=(const PartValues&) = delete;
};

// PartPointer.cs (a MonoBehaviour holding a Part reference)
class PartPointer {
public:
    Part* PartReference() const { return part_; }  // PartPointer.cs:10
    void SetPart(Part* part) { part_ = part; }     // :12
    Part* GetPart() const { return part_; }        // :17 (internal)

private:
    Part* part_ = nullptr;
};

// PartRawInfo.cs
class PartRawInfo {
public:
    PartRawInfo(std::string name, std::string value) : name_(std::move(name)), value_(std::move(value)) {}  // :18
    const std::string& Name() const { return name_; }
    const std::string& Value() const { return value_; }

private:
    std::string name_, value_;
};

// PartGameObjectInfo.cs
class PartGameObjectInfo {
public:
    PartGameObjectInfo(std::string name, bool status) : name_(std::move(name)), status_(status) {}  // :18
    const std::string& Name() const { return name_; }
    bool Status() const { return status_; }

private:
    std::string name_;
    bool status_ = false;
};

// PartHeightQuery.cs
struct PartHeightQuery {
    float lowestPoint = 0.0f;
    std::map<Part*, float> lowestOnParts;  // Dictionary<Part, float>
    explicit PartHeightQuery(float lowest) : lowestPoint(lowest) {}  // :11
};

// IPartSizeModifier.cs
class IPartSizeModifier {
public:
    virtual ~IPartSizeModifier() = default;
    virtual Vector3f GetModuleSize(const Vector3f& defaultSize, ModifierStagingSituation sit) = 0;
    virtual ModifierChangeWhen GetModuleSizeChangeWhen() = 0;
};

// KSPPart.cs ([KSPPart] class attribute; DefaultIcons.CUSTOM is the default stage icon)
struct KSPPart {
    int32_t stageIcon = 0;  // DefaultIcons.CUSTOM (enum member order is UI data; only the attribute shape is kept)
};

// KSPModelEvent.cs ([KSPModelEvent] method attribute)
struct KSPModelEvent {
    std::string startEvent, finishEvent;
    bool allowMultiple = false;
    KSPModelEvent(std::string start, std::string finish, bool allow)
        : startEvent(std::move(start)), finishEvent(std::move(finish)), allowMultiple(allow) {}  // :14
};

}  // namespace ksp
