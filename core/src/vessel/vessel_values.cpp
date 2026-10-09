// VesselValues.cs (see include/ksp/vessel/vessel_values.hpp)
#include "ksp/vessel/vessel_values.hpp"

#include "ksp/parts/part.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {
// The (float a, float b) => a > b / a < b lambdas of VesselValues.cs:117-129 compare float values
bool Gt(float a, float b) { return a > b; }
bool Lt(float a, float b) { return a < b; }
bool GtI(int a, int b) { return a > b; }
}  // namespace

// VesselValues.cs:128
VesselValues::VesselValues(Vessel& host)
    : MaxThrottle([&host]() -> std::vector<Part*>& { return host.parts; }, [] { return Vessel::env.frameCount; }, 1.0f, Gt,
                  [](Part& p) { return p.partValues.MaxThrottle.value(); }),
      HeatProduction([&host]() -> std::vector<Part*>& { return host.parts; }, [] { return Vessel::env.frameCount; }, 1.0f, Lt,
                     [](Part& p) { return p.partValues.HeatProduction.value(); }),
      FuelUsage([&host]() -> std::vector<Part*>& { return host.parts; }, [] { return Vessel::env.frameCount; }, 1.0f, Lt,
                [](Part& p) { return p.partValues.FuelUsage.value(); }),
      EnginePower([&host]() -> std::vector<Part*>& { return host.parts; }, [] { return Vessel::env.frameCount; }, 1.0f, Gt,
                  [](Part& p) { return p.partValues.EnginePower.value(); }),
      SteeringRadius([&host]() -> std::vector<Part*>& { return host.parts; }, [] { return Vessel::env.frameCount; }, 1.0f, Gt,
                     [](Part& p) { return p.partValues.SteeringRadius.value(); }),
      AutopilotSkill([&host]() -> std::vector<Part*>& { return host.parts; }, [] { return Vessel::env.frameCount; }, -1, GtI,
                     [](Part& p) { return p.partValues.AutopilotSkill.value(); }),
      AutopilotKerbalSkill([&host]() -> std::vector<Part*>& { return host.parts; }, [] { return Vessel::env.frameCount; }, -1, GtI,
                           [](Part& p) { return p.partValues.AutopilotKerbalSkill.value(); }),
      AutopilotSASSkill([&host]() -> std::vector<Part*>& { return host.parts; }, [] { return Vessel::env.frameCount; }, -1, GtI,
                        [](Part& p) { return p.partValues.AutopilotSASSkill.value(); }),
      RepairSkill([&host]() -> std::vector<Part*>& { return host.parts; }, [] { return Vessel::env.frameCount; }, -1, GtI,
                  [](Part& p) { return p.partValues.RepairSkill.value(); }),
      FailureRepairSkill([&host]() -> std::vector<Part*>& { return host.parts; }, [] { return Vessel::env.frameCount; }, -1, GtI,
                         [](Part& p) { return p.partValues.FailureRepairSkill.value(); }),
      ScienceSkill([&host]() -> std::vector<Part*>& { return host.parts; }, [] { return Vessel::env.frameCount; }, -1, GtI,
                   [](Part& p) { return p.partValues.ScienceSkill.value(); }),
      EVAChuteSkill([&host]() -> std::vector<Part*>& { return host.parts; }, [] { return Vessel::env.frameCount; }, 0, GtI,
                    [](Part& p) { return p.partValues.EVAChuteSkill.value(); }),
      CommsRange([&host]() -> std::vector<Part*>& { return host.parts; }, [] { return Vessel::env.frameCount; }, 1.0f, Gt,
                 [](Part& p) { return p.partValues.CommsRange.value(); }) {}

}  // namespace ksp
