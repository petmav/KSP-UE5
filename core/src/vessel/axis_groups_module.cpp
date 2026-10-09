// AxisGroupsModule.cs (see include/ksp/vessel/axis_groups_module.hpp)
#include "ksp/vessel/axis_groups_module.hpp"

#include <string_view>

#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {
bool Has(KSPAxisGroup g, KSPAxisGroup bit) { return (static_cast<int>(g) & static_cast<int>(bit)) != 0; }
constexpr float kAxisIncrementalSpeedMultiplierDefault = 0.2f;  // GameSettings.cs:1369
}  // namespace

// BaseAxisField.cs:218
void BaseAxisField_IncrementAxis(BaseAxisField& f, float axisRate) {
    if (!f.ignoreIncrementByZero || axisRate != 0.0f) {
        float num = std::get<float>(f.GetValue());
        if (f.incrementalSpeedMultiplier == 0.0f) f.incrementalSpeedMultiplier = kAxisIncrementalSpeedMultiplierDefault;
        f.incrementalSpeed = f32store((fx(f.maxValue) - fx(f.minValue)) * fx(f.incrementalSpeedMultiplier));
        num = f32store(fx(num) + fx(axisRate) * fx(f.incrementalSpeed) * fx(TimeWarp::fixedDeltaTime()));
        if (!f.ignoreClampWhenIncremental) {
            num = num < f.maxValue ? num : f.maxValue;  // Mathf.Min
            num = num > f.minValue ? num : f.minValue;  // Mathf.Max
        }
        f.SetValue(num);
    }
}

// BaseAxisField.cs:394
void BaseAxisField_UpdateAxis(BaseAxisField& f, KSPAxisGroup group, int groupOverride, float axisValue) {
    int64_t inverted = f.axisInverted;
    int64_t incremental = f.axisIncremental;
    if (groupOverride > 0 && groupOverride <= static_cast<int>(f.overrideGroups.size())) {
        inverted = f.overrideInverted[static_cast<size_t>(groupOverride - 1)];
        incremental = f.overrideIncremental[static_cast<size_t>(groupOverride - 1)];
    }
    if ((static_cast<int64_t>(group) & inverted) != 0) axisValue = 0.0f - axisValue;
    if ((static_cast<int64_t>(group) & incremental) != 0) BaseAxisField_IncrementAxis(f, axisValue);
    else f.SetAxis(axisValue);
}

AxisGroupsModule::AxisGroupsModule(const ModuleClassInfo& cls) : VesselModule(cls) {}

AxisGroupsModule::~AxisGroupsModule() {  // OnDestroy (:85)
    if (subscribed_) {
        GE::onVesselWasModified.Remove(this, &AxisGroupsModule::onVesselWasModified);
        GE::OnVesselOverrideGroupChanged.Remove(this, &AxisGroupsModule::OnVesselOverrideGroupChanged);
    }
}

// AxisGroupsModule.cs:79
void AxisGroupsModule::OnAwake() {
    GE::onVesselWasModified.Add(this, &AxisGroupsModule::onVesselWasModified);
    GE::OnVesselOverrideGroupChanged.Add(this, &AxisGroupsModule::OnVesselOverrideGroupChanged);
    subscribed_ = true;
}

// AxisGroupsModule.cs:11
void AxisGroupsModule::AddAxis(KSPAxisGroup axisGroup, BaseAxisField& axisField) {
    Vessel* v = vesselPtr();
    groupOverride_ = 0;
    bool overrideDefault = false;
    if (GameSettings::ADDITIONAL_ACTION_GROUPS) {
        groupOverride_ = v->GroupOverride;
        if (groupOverride_ > 0 && groupOverride_ <= static_cast<int>(v->OverrideDefault.size()))
            overrideDefault = v->OverrideDefault[static_cast<size_t>(groupOverride_ - 1)];
    }
    if (axisField.IsInGroup(static_cast<int64_t>(axisGroup), groupOverride_, overrideDefault, true)) {
        AxisGroups[static_cast<int>(axisGroup)].push_back(&axisField);  // creates the list when absent
    }
}

// AxisGroupsModule.cs:31 (BaseAxisField.AxisGroupsLength = Enum.GetNames(KSPAxisGroup).Length - 2 = 13)
void AxisGroupsModule::BuildAxisGroups() {
    axisGroupsBuilt = true;
    AxisGroups.clear();
    std::vector<PartModule*> list = vesselPtr()->FindPartModulesImplementing([](PartModule&) { return true; });
    int count = static_cast<int>(list.size());
    while (count-- > 0) {
        PartModule* partModule = list[static_cast<size_t>(count)];
        int count2 = static_cast<int>(partModule->Fields().Count());
        while (count2-- > 0) {
            auto* f = dynamic_cast<BaseAxisField*>(partModule->Fields()[static_cast<size_t>(count2)]);
            if (f != nullptr && std::string_view(f->FieldInfo().typeName) == "Single") {
                for (int i = 0; i < 13; i++) AddAxis(static_cast<KSPAxisGroup>(1 << i), *f);
            }
        }
    }
}

// AxisGroupsModule.cs:60
void AxisGroupsModule::onVesselWasModified(Vessel* v) {
    if (v == vesselPtr() && vesselPtr()->loaded) BuildAxisGroups();
}

// AxisGroupsModule.cs:67
void AxisGroupsModule::OnVesselOverrideGroupChanged(Vessel* v) {
    if (v == vesselPtr() && vesselPtr()->loaded) BuildAxisGroups();
}

// AxisGroupsModule.cs:99
bool AxisGroupsModule::ShouldBeActive(bool loadedSceneIsFlight, bool vesselLoaded) {
    return VesselModule::ShouldBeActive(loadedSceneIsFlight, vesselLoaded);
}

// AxisGroupsModule.cs:104
void AxisGroupsModule::SetAxisGroup(KSPAxisGroup axisGroup, float axisValue) {
    if (!axisGroupsBuilt) return;
    auto it = AxisGroups.find(static_cast<int>(axisGroup));
    if (it == AxisGroups.end()) return;
    int count = static_cast<int>(it->second.size());
    while (count-- > 0) it->second[static_cast<size_t>(count)]->SetAxis(axisValue);
}

// AxisGroupsModule.cs:116
void AxisGroupsModule::IncrementAxisGroup(KSPAxisGroup axisGroup, float axisRate) {
    if (!axisGroupsBuilt) return;
    auto it = AxisGroups.find(static_cast<int>(axisGroup));
    if (it == AxisGroups.end()) return;
    int count = static_cast<int>(it->second.size());
    while (count-- > 0) BaseAxisField_IncrementAxis(*it->second[static_cast<size_t>(count)], axisRate);
}

// AxisGroupsModule.cs:128
void AxisGroupsModule::UpdateAxisGroup(KSPAxisGroup axisGroup, float axisValue) {
    if (!axisGroupsBuilt) return;
    auto it = AxisGroups.find(static_cast<int>(axisGroup));
    if (it == AxisGroups.end()) return;
    int count = static_cast<int>(it->second.size());
    while (count-- > 0) BaseAxisField_UpdateAxis(*it->second[static_cast<size_t>(count)], axisGroup, groupOverride_, axisValue);
}

std::shared_ptr<VesselModule> CreateAxisGroupsModule(const ModuleClassInfo& cls) { return std::make_shared<AxisGroupsModule>(cls); }

}  // namespace ksp
