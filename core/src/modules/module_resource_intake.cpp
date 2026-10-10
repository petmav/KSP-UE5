// WP-23: ModuleResourceIntake.cs (see ksp/modules/module_resource_intake.hpp for scope).
#include "ksp/modules/module_resource_intake.hpp"

#include <limits>

#include "ksp/core/exceptions.hpp"
#include "ksp/curves/float_curve.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

Part* ModuleResourceIntake::PartPtr() const { return dynamic_cast<Part*>(part); }

// ModuleResourceIntake.cs:75
void ModuleResourceIntake::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("Deactivate", BaseEvent::Handler([this] { Deactivate(); }));
    Events().Bind("Activate", BaseEvent::Handler([this] { Activate(); }));
    Actions().Bind("ToggleAction", [this](KSPActionParam& p) { ToggleAction(p); });
    if (IsNull("machCurve")) {
        FloatCurve c;
        c.Add(0.0f, 1.0f);
        Set<FloatCurve>("machCurve", std::move(c));
    }
}

// ModuleResourceIntake.cs:85
void ModuleResourceIntake::OnStart(StartState) {
    Part* p = PartPtr();
    if (p == nullptr) throw NullReferenceException();
    if (!hasIntakeTransform && findModelTransform) hasIntakeTransform = findModelTransform(*p, Get<std::string>("intakeTransformName"));
    if (isEnabled) {
        if (Get<bool>("intakeEnabled")) {
            Events()["Deactivate"]->active = true;
            Events()["Activate"]->active = false;
        } else {
            Events()["Deactivate"]->active = false;
            Events()["Activate"]->active = true;
        }
        Fields()["airFlow"]->guiActive = true;
        Fields()["status"]->guiActive = true;
        Fields()["airSpeedGui"]->guiActive = true;
        Actions()["ToggleAction"]->active = true;
    } else {
        Events()["Deactivate"]->active = false;
        Events()["Activate"]->active = false;
        Fields()["airFlow"]->guiActive = false;
        Fields()["status"]->guiActive = false;
        Fields()["airSpeedGui"]->guiActive = false;
        Actions()["ToggleAction"]->active = false;
    }
    PartResourceLibrary* lib = PartResourceLibrary::Instance();
    if (lib == nullptr) throw NullReferenceException();
    resourceDef = lib->GetDefinition(MonoStringHash(Get<std::string>("resourceName")));
    if (resourceDef == nullptr) throw NullReferenceException();
    densityRecip = 1.0 / static_cast<double>(resourceDef->density());
    const std::string occludeNode = Get<std::string>("occludeNode");
    if (!occludeNode.empty()) {
        node = p->FindAttachNode(occludeNode);
        if (node != nullptr) checkNode = true;
    }
    res = p->Resources.Get(std::string_view(Get<std::string>("resourceName")));
}

// ModuleResourceIntake.cs:134
void ModuleResourceIntake::OnLoad(const ConfigNode&) {
    if (Get<bool>("intakeEnabled")) {
        Events()["Deactivate"]->active = false;
        Events()["Activate"]->active = true;
    }
    resourceId = MonoStringHash(Get<std::string>("resourceName"));
}

// ModuleResourceIntake.cs:161
void ModuleResourceIntake::ToggleAction(KSPActionParam&) {
    if (Get<bool>("intakeEnabled")) Deactivate();
    else Activate();
}
// ModuleResourceIntake.cs:173
void ModuleResourceIntake::ToggleAction(KSPActionType action) {
    if (action == KSPActionType::Deactivate) Deactivate();
    else Activate();
}
// ModuleResourceIntake.cs:186
void ModuleResourceIntake::Deactivate() {
    Events()["Deactivate"]->active = false;
    Events()["Activate"]->active = true;
    Set<bool>("intakeEnabled", false);
}
// ModuleResourceIntake.cs:194
void ModuleResourceIntake::Activate() {
    Events()["Deactivate"]->active = true;
    Events()["Activate"]->active = false;
    Set<bool>("intakeEnabled", true);
}

// ModuleResourceIntake.cs:201
void ModuleResourceIntake::FixedUpdate(const IntakeFlightInput& in) {
    Part* p = PartPtr();
    if (p == nullptr) throw NullReferenceException();
    if (Get<bool>("intakeEnabled") && isEnabled && in.hasVessel && in.hasIntakeTransform) {
        const bool disableUnderwater = Get<bool>("disableUnderwater");
        const bool underwaterOnly = Get<bool>("underwaterOnly");
        if (!p->ShieldedFromAirstream() && (!checkNode || !(node->attachedPart != nullptr))) {
            if ((in.atmosphereContainsOxygen || !Get<bool>("checkForOxygen")) && in.staticPressurekPa >= Get<double>("kPaThreshold") &&
                ((!disableUnderwater && !underwaterOnly) || (disableUnderwater && (!in.oceanBody || in.intakeAltitude >= 0.0)) ||
                 (underwaterOnly && in.oceanBody && in.intakeAltitude < 0.0))) {
                double num = UtilMath::Clamp01(static_cast<double>(Vector3f::Dot(in.srf_vel_direction, in.intakeForward))) * in.srfSpeed +
                             Get<double>("intakeSpeed");
                Set<float>("airSpeedGui", static_cast<float>(num));
                const FloatCurve* machCurve = std::get_if<FloatCurve>(Value("machCurve"));
                if (machCurve == nullptr) throw NullReferenceException();
                num *= Get<double>("unitScalar") * Get<double>("area") * static_cast<double>(machCurve->Evaluate(static_cast<float>(in.mach)));
                const double num2 = num * (underwaterOnly ? in.oceanDensity : in.atmDensity);
                resourceUnits = num2 * densityRecip;
                if (resourceUnits > 0.0) {
                    Set<float>("airFlow", static_cast<float>(resourceUnits));
                    resourceUnits *= static_cast<double>(in.fixedDeltaTime);
                    if (res == nullptr) throw NullReferenceException();
                    if (res->maxAmount - res->amount >= resourceUnits) p->TransferResource(resourceId, resourceUnits);
                    else p->RequestResource(resourceId, 0.0 - resourceUnits);
                } else {
                    resourceUnits = 0.0;
                    Set<float>("airFlow", 0.0f);
                }
                Set<std::string>("status", Localizer::Format("#autoLOC_235936"));
            } else {
                Set<float>("airFlow", 0.0f);
                Set<float>("airSpeedGui", 0.0f);
                p->TransferResource(resourceId, std::numeric_limits<double>::lowest());  // double.MinValue
                Set<std::string>("status", Localizer::Format("#autoLOC_235946"));
            }
        } else {
            Set<float>("airFlow", 0.0f);
            Set<float>("airSpeedGui", 0.0f);
            Set<std::string>("status", Localizer::Format("#autoLOC_235899"));
        }
    } else {
        Set<std::string>("status", Localizer::Format("#autoLOC_8005416"));
    }
}

namespace detail {
void RegisterIntakeModule() {
    ModuleFactory::Register("ModuleResourceIntake", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleResourceIntake>(c)); });
}
}  // namespace detail

}  // namespace ksp
