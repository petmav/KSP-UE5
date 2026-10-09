// AxisGroupsModule.cs (139 lines): the VesselModule that gathers a vessel's [KSPAxisField] fields per axis group and drives them.
// WP-06 registered the class name with a stand-in (order 999, FlightScene | LoadedVessels); this is the real module.
//
// BaseAxisField.IncrementAxis (BaseAxisField.cs:218) and UpdateAxis (:394) are used by the module and were not part of WP-06's
// BaseAxisField: they are ported here as the free functions BaseAxisField_IncrementAxis / BaseAxisField_UpdateAxis.
#pragma once

#include <map>
#include <memory>
#include <vector>

#include "ksp/modules/base_field.hpp"
#include "ksp/modules/vessel_module.hpp"
#include "ksp/save/save_enums.hpp"

namespace ksp {

class Vessel;

using BaseAxisFieldList = std::vector<BaseAxisField*>;  // BaseAxisFieldList : List<BaseAxisField>

void BaseAxisField_IncrementAxis(BaseAxisField& f, float axisRate);                                   // BaseAxisField.cs:218
void BaseAxisField_UpdateAxis(BaseAxisField& f, KSPAxisGroup group, int groupOverride, float axisValue);  // BaseAxisField.cs:394

class AxisGroupsModule final : public VesselModule {
public:
    explicit AxisGroupsModule(const ModuleClassInfo& cls);
    ~AxisGroupsModule() override;

    std::map<int, BaseAxisFieldList> AxisGroups;  // Dictionary<int, BaseAxisFieldList>
    bool axisGroupsBuilt = false;                 // AxisGroups != null

    Vessel* vesselPtr() const { return static_cast<Vessel*>(vessel); }

    void AddAxis(KSPAxisGroup axisGroup, BaseAxisField& axisField);   // :11
    void BuildAxisGroups();                                           // :31
    Activation GetActivation() override { return static_cast<Activation>(FlightScene | LoadedVessels); }  // :74
    void OnLoadVessel() override { BuildAxisGroups(); }               // :94
    bool ShouldBeActive(bool loadedSceneIsFlight, bool vesselLoaded) override;  // :99
    void SetAxisGroup(KSPAxisGroup axisGroup, float axisValue);       // :104
    void IncrementAxisGroup(KSPAxisGroup axisGroup, float axisRate);  // :116
    void UpdateAxisGroup(KSPAxisGroup axisGroup, float axisValue);    // :128

protected:
    void OnAwake() override;  // :79 (the GameEvents subscriptions)

private:
    void onVesselWasModified(Vessel* v);              // :60
    void OnVesselOverrideGroupChanged(Vessel* v);     // :67
    int groupOverride_ = 0;
    bool subscribed_ = false;
};

std::shared_ptr<VesselModule> CreateAxisGroupsModule(const ModuleClassInfo& cls);  // the VesselModuleFactory creator

}  // namespace ksp
