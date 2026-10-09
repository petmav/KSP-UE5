// WP-16: ModuleDecouplerBase.cs, ModuleDecouple.cs, ModuleAnchoredDecoupler.cs (see include/ksp/modules/module_decouplers.hpp).
// Every function cites File.cs:line of decompiled/KSP-Decompiled/Assembly-CSharp/.
#include "ksp/modules/module_decouplers.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/staging/stage_manager.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = GameEvents;
using KSP::Localization::Localizer;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }

// float.ToString("0.0###"): at least one and at most four decimals
std::string FormatForce(float v) {
    std::string s = dotnet::FormatSingleF(v, 4);
    const size_t dot = s.find('.');
    if (dot != std::string::npos) {
        while (s.size() > dot + 2 && s.back() == '0') s.pop_back();
    }
    return s;
}

// FlightGlobals.GetPartUpwardsCached(vessel.ReferenceTransform.gameObject): the part that owns the vessel's reference transform
// (the reference part, else the root part the Vessel component sits on)
Part* ReferenceTransformPart(Vessel& v) {
    Part* p = v.GetReferenceTransformPart();
    return p != nullptr ? p : v.rootPart;
}
}  // namespace

// ============================================================================================ ModuleDecouplerBase

ModuleDecouplerBase::ModuleDecouplerBase(const ModuleClassInfo& cls) : PartModule(cls) { live_.push_back(this); }

// ModuleDecouplerBase.cs:122
ModuleDecouplerBase::~ModuleDecouplerBase() {
    live_.erase(std::remove(live_.begin(), live_.end(), this), live_.end());
    if (subscribed_) GE::onVesselWasModified.Remove(this, &ModuleDecouplerBase::OnVesselWasModified);
}

Part* ModuleDecouplerBase::PartOf() const { return dynamic_cast<Part*>(part); }

// one frame: LateUpdate, then the coroutines in start order
void ModuleDecouplerBase::TickAll() {
    std::vector<ModuleDecouplerBase*> mods(live_);
    for (ModuleDecouplerBase* m : mods) {
        if (std::find(live_.begin(), live_.end(), m) == live_.end()) continue;
        m->LateUpdate();
        std::vector<Delayed> pending;
        pending.swap(m->delayed_);
        for (Delayed& d : pending) {
            if (--d.ticks <= 0) d.fn();
            else m->delayed_.push_back(std::move(d));
        }
    }
}

// ModuleDecouplerBase.cs:61
void ModuleDecouplerBase::DecoupleAction(KSPActionParam& /*param*/) { Decouple(); }

// ModuleDecouplerBase.cs:67
void ModuleDecouplerBase::Decouple() { OnDecouple(); }

// ModuleDecouplerBase.cs:72 (no error log when the FX group is absent: the groups are model data)
void ModuleDecouplerBase::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("Decouple", BaseEvent::Handler([this] { Decouple(); }));
    Actions().Bind("DecoupleAction", [this](KSPActionParam& p) { DecoupleAction(p); });
    if (Part* p = PartOf()) fxFound = p->findFxGroup(Get<std::string>("fxGroupName")) != nullptr;
}

// ModuleDecouplerBase.cs:81
void ModuleDecouplerBase::OnStart(StartState state) {
    PartModule::OnStart(state);
    Part* p = PartOf();
    if (p == nullptr) throw Nre();
    if (Get<std::string>("explosiveNodeID") == "srf") explosiveNode = p->srfAttachNode.get();
    else explosiveNode = p->FindAttachNode(Get<std::string>("explosiveNodeID"));
    if (explosiveNode == nullptr) log::Error("[ModuleDecouple Error]: No attachnode found with id " + Get<std::string>("explosiveNodeID"));
    if (Get<bool>("isEnginePlate")) {
        jettisonModule = p->FindModuleOfClass("ModuleJettison");
        if (jettisonModule != nullptr && !jettisonModule->Get<bool>("isJettisoned")) {
            SetAirstream(true);
            // jettisonModule.OnStop.Add(OnJettison): the jettison module calls OnJettison (ModuleJettison, WP-30)
            if (!subscribed_) {
                GE::onVesselWasModified.Add(this, &ModuleDecouplerBase::OnVesselWasModified);
                subscribed_ = true;
            }
        }
    }
}

// ModuleDecouplerBase.cs:107
void ModuleDecouplerBase::OnActive() {}

// ModuleDecouplerBase.cs:111
void ModuleDecouplerBase::LateUpdate() {
    if (refreshStaging) {
        Set<bool>("stagingEnabled", false);
        SetModuleAttributesIsStageable(false);  // base.ModuleAttributes.isStageable = false (per class; this IsStageable() override answers `staged` anyway)
        if (Part* p = PartOf()) p->UpdateStageability(false, true);
        refreshStaging = false;
    }
}

// ModuleDecouplerBase.cs:131
void ModuleDecouplerBase::OnVesselWasModified(Vessel* vsl) {
    Part* p = PartOf();
    if (p != nullptr && p->vessel != nullptr && vsl->persistentId == p->vessel->persistentId) CheckShielded();
}

// ModuleDecouplerBase.cs:139
void ModuleDecouplerBase::CheckShielded() {
    if (jettisonModule != nullptr && shroudOn) {
        AttachNode* attachNode = PartOf()->FindAttachNode(jettisonModule->Get<std::string>("bottomNodeName"));
        if (attachNode != nullptr && attachNode->attachedPart == nullptr) SetAirstream(false);
    }
}

// ModuleDecouplerBase.cs:151
void ModuleDecouplerBase::OnJettison(float value) {
    if (value == 1.0f) SetAirstream(false);
}

// ModuleDecouplerBase.cs:159 (Part.AddShield / RemoveShield: the aero package's)
void ModuleDecouplerBase::SetAirstream(bool enclosed) {
    if (jettisonModule != nullptr) {
        Part* p = PartOf();
        for (size_t i = 0; i < p->attachNodes.size(); i++) {
            if (p->attachNodes[i].id != jettisonModule->Get<std::string>("bottomNodeName") && p->attachNodes[i].attachedPart != nullptr)
                p->attachNodes[i].attachedPart->set_ShieldedFromAirstream(enclosed);
        }
    }
    shroudOn = enclosed;
}

// ModuleDecouplerBase.cs:182
void ModuleDecouplerBase::OnDecouple() {
    if (!Get<bool>("isDecoupled")) IsAdjusterBlockingDecouple();
}

// ModuleDecouplerBase.cs:190
std::string ModuleDecouplerBase::GetModuleTitle() {
    if (Get<bool>("isOmniDecoupler")) return Localizer::Format("#autoLOC_6001039");
    return Localizer::Format("#autoLOC_6001040");
}

// ModuleDecouplerBase.cs:204
std::string ModuleDecouplerBase::GetPrimaryField() { return Localizer::Format("#autoLOC_240300", std::vector<std::string>{FormatForce(Get<float>("ejectionForce"))}); }

// ModuleDecouplerBase.cs:209
int ModuleDecouplerBase::GetStageIndex(int /*fallback*/) {
    Part* p = PartOf();
    if (!StagingEnabled() && p->parent != nullptr) return p->parent->fields.inverseStage;
    return p->fields.inverseStage;
}

// ModuleDecouplerBase.cs:218
bool ModuleDecouplerBase::PartDetaches(std::vector<Part*>& decoupledParts) {
    decoupledParts.clear();
    Part* p = PartOf();
    if (explosiveNode == nullptr) {
        if (Get<std::string>("explosiveNodeID") == "srf") explosiveNode = p->srfAttachNode.get();
        else explosiveNode = p->FindAttachNode(Get<std::string>("explosiveNodeID"));
    }
    if (explosiveNode != nullptr && explosiveNode->attachedPart != nullptr) decoupledParts.push_back(explosiveNode->attachedPart);
    return Get<bool>("partDecoupled");
}

// ModuleDecouplerBase.cs:239
bool ModuleDecouplerBase::IsEnginePlate() { return Get<bool>("isEnginePlate"); }

// ModuleDecouplerBase.cs:249 / :254
Vessel* ModuleDecouplerBase::GetVessel() { return PartOf() != nullptr ? PartOf()->vessel : nullptr; }
Part* ModuleDecouplerBase::GetPart() { return PartOf(); }

// ModuleDecouplerBase.cs:259
bool ModuleDecouplerBase::IsStageable() { return Get<bool>("staged"); }

// ModuleDecouplerBase.cs:264
bool ModuleDecouplerBase::StagingEnabled() {
    if (PartModule::StagingEnabled()) return Get<bool>("staged");
    return false;
}

// ModuleDecouplerBase.cs:273 / :278
bool ModuleDecouplerBase::StagingToggleEnabledEditor() { return Get<bool>("staged"); }
bool ModuleDecouplerBase::StagingToggleEnabledFlight() { return PartModule::StagingToggleEnabledFlight(); }

// ModuleDecouplerBase.cs:283 / :292
std::string ModuleDecouplerBase::GetStagingEnableText() {
    const std::string t = Get<std::string>("stagingEnableText");
    if (!t.empty()) return t;
    return Localizer::Format("#autoLOC_240328");
}
std::string ModuleDecouplerBase::GetStagingDisableText() {
    const std::string t = Get<std::string>("stagingDisableText");
    if (!t.empty()) return t;
    return Localizer::Format("#autoLOC_240329");
}

// ============================================================================================ ModuleDecouple

// ModuleDecouple.cs:31
void ModuleDecouple::OnAwake() {
    ModuleDecouplerBase::OnAwake();
    if (Get<std::string>("menuName") != std::string()) {
        Events()["Decouple"]->SetGuiName(Get<std::string>("menuName"));
        Actions()["DecoupleAction"]->SetGuiName(Get<std::string>("menuName"));
    }
}

// ModuleDecouple.cs:41
void ModuleDecouple::OnStart(StartState state) {
    ModuleDecouplerBase::OnStart(state);
    Part* p = PartOf();
    if (p->fields.stagingIcon && p->fields.stagingIcon->empty() && Get<bool>("overrideStagingIconIfBlank")) p->fields.stagingIcon = std::string("DECOUPLER_VERT");
    if (Get<bool>("isDecoupled")) Events()["Decouple"]->active = false;  // findFxGroup("activate")?.setActive(false): FX
}

// ModuleDecouple.cs:55
void ModuleDecouple::OnActive() {
    if (Get<bool>("staged")) OnDecouple();
}

// ModuleDecouple.cs:63
void ModuleDecouple::OnDecouple() {
    ModuleDecouplerBase::OnDecouple();
    // fx.Burst(): FX
    Part* p = PartOf();
    if (p == nullptr || p->vessel == nullptr) throw Nre();
    Part* parent = p->parent;
    Part* partUpwardsCached = ReferenceTransformPart(*p->vessel);
    Vessel* oldVessel = p->vessel;
    std::vector<Jettisonning> list;
    const float ejectionForce = Get<float>("ejectionForce");
    const float ejectionForcePercent = Get<float>("ejectionForcePercent");
    const bool automaticDir = Get<bool>("automaticDir");
    const Vector3f explosiveDir = Get<Vector3f>("explosiveDir");
    if (Get<bool>("isOmniDecoupler")) {
        std::vector<Part*> list2(p->children);
        const int num = static_cast<int>(p->children.size()) + 1;
        float num2 = f32store(fx(ejectionForce) * (fx(ejectionForcePercent) * fx(0.01f)) / fx(static_cast<float>(num)));
        for (size_t i = 0, count = list2.size(); i < count; i++) {
            Part* part = list2[i];
            part->decouple();
            Vector3f vector = !automaticDir ? p->partTransform.TransformDirection(explosiveDir)
                                            : Vector3f::Normalize(p->partTransform.position - part->partTransform.position);
            vector *= num2;
            list.push_back({part, -vector});
            list.push_back({p, vector});
        }
        if (p->parent != nullptr) {
            p->decouple();
            num2 = f32store(fx(ejectionForce) * (fx(ejectionForcePercent) * fx(0.01f)) * fx(0.5f));
            Vector3f vector2 = !automaticDir ? p->partTransform.TransformDirection(explosiveDir)
                                             : Vector3f::Normalize(p->partTransform.position - parent->partTransform.position);
            vector2 *= num2;
            list.push_back({parent, -vector2});
            list.push_back({p, vector2});
        }
    } else if (explosiveNode == nullptr) {
        throw Nre();
    } else if (explosiveNode->attachedPart != nullptr) {
        parent = explosiveNode->attachedPart;
        if (parent == p->parent) p->decouple();
        else parent->decouple();
        const float num3 = f32store(fx(ejectionForce) * (fx(ejectionForcePercent) * fx(0.01f)) * fx(0.5f));
        Vector3f vector3 = !automaticDir ? p->partTransform.TransformDirection(explosiveDir)
                                         : Vector3f::Normalize(p->partTransform.position - parent->partTransform.position);
        vector3 *= num3;
        list.push_back({parent, -vector3});
        list.push_back({p, vector3});
    }
    lastEjections_ = list;
    // StartCoroutine(ApplyEjectionForces(2, list)) (:143): two frames later part.AddForce(ejectionVector), last entry first
    StartCoroutine(2, [list] {
        for (size_t count = list.size(); count-- > 0;)
            if (list[count].part != nullptr && addForce) addForce(*list[count].part, list[count].ejectionVector);
    });
    EventReport rep(FlightEvents::STAGESEPARATION, p, std::string(), std::string(), StageManager::Instance != nullptr ? StageManager::CurrentStage() : 0);
    GE::onStageSeparation.Fire(&rep);
    Set<bool>("isDecoupled", true);
    refreshStaging = true;
    Events()["Decouple"]->active = false;
    StartCoroutine(1, [this, partUpwardsCached, oldVessel] { WaitAndSwitchFocus(partUpwardsCached, oldVessel); });  // :121
}

// ModuleDecouple.cs:124 (after the first `yield return null`)
void ModuleDecouple::WaitAndSwitchFocus(Part* p, Vessel* oldVessel) {
    if (p->vessel != oldVessel) {
        oldVessel->vesselType = VesselType::Debris;
        oldVessel->vesselType = oldVessel->FindDefaultVesselType();
        if (!oldVessel->UpdateVesselNaming()) oldVessel->vesselName = Vessel::AutoRename(*oldVessel, oldVessel->vesselName);
        p->vessel->ctrlState.CopyFrom(oldVessel->ctrlState);
        p->vessel->ActionGroups().CopyFrom(oldVessel->ActionGroups());
        p->vessel->currentStage = oldVessel->currentStage;
        p->vessel->SetReferenceTransform(p);
        if (forceSetActiveVessel) forceSetActiveVessel(p->vessel);
    }
}

// ModuleDecouple.cs:165
std::string ModuleDecouple::GetInfo() { return "" + Localizer::Format("#autoLOC_240611", std::vector<std::string>{FormatForce(Get<float>("ejectionForce"))}); }

// ModuleDecouple.cs:170
std::string ModuleDecouple::GetModuleDisplayName() {
    if (Get<bool>("isOmniDecoupler")) return Localizer::Format("#autoLOC_6001039");
    return Localizer::Format("#autoLOC_6001040");
}

// ============================================================================================ ModuleAnchoredDecoupler

// ModuleAnchoredDecoupler.cs:18
void ModuleAnchoredDecoupler::OnStart(StartState state) {
    ModuleDecouplerBase::OnStart(state);
    Part* p = PartOf();
    if (p->fields.stagingIcon && p->fields.stagingIcon->empty() && Get<bool>("overrideStagingIconIfBlank")) p->fields.stagingIcon = std::string("DECOUPLER_HOR");
    // anchor = FindModelTransform(anchorName); `if (isDecoupled) anchor.gameObject.SetActive(false)`: the model's
    if (Get<bool>("isDecoupled")) Events()["Decouple"]->active = false;
}

// ModuleAnchoredDecoupler.cs:33
void ModuleAnchoredDecoupler::OnActive() {
    if (Get<bool>("staged") && Get<bool>("stagingEnabled")) OnDecouple();
}

// ModuleAnchoredDecoupler.cs:41
void ModuleAnchoredDecoupler::OnDecouple() {
    ModuleDecouplerBase::OnDecouple();
    Part* p = PartOf();
    if (explosiveNode != nullptr && explosiveNode->attachedPart != nullptr) {
        // fx.Burst(); anchor.parent = otherPart.transform: FX / model
        otherPart = explosiveNode->attachedPart;
        if (otherPart == p->parent) p->decouple();
        else otherPart->decouple();
    }
    // StartCoroutine(ApplyEjectionForces(2, part, otherPart, TransformDirection(explosiveDir) * ejectionForce * 0.5f * (percent / 100f), part))
    Vector3f force = p->partTransform.TransformDirection(Get<Vector3f>("explosiveDir")) * Get<float>("ejectionForce");
    force = force * 0.5f;
    force = force * f32store(fx(Get<float>("ejectionForcePercent")) / fx(100.0f));
    Part* other = otherPart;
    StartCoroutine(2, [p, other, force] {
        if (p != nullptr) {
            if (addForce) addForce(*p, -force);
            if (other != nullptr && addForceAtPosition) addForceAtPosition(*other, force, p->partTransform.position);
        }
    });
    EventReport rep(FlightEvents::STAGESEPARATION, p, std::string(), std::string(), StageManager::Instance != nullptr ? StageManager::CurrentStage() : 0);
    GE::onStageSeparation.Fire(&rep);
    Set<bool>("isDecoupled", true);
    refreshStaging = true;
    Events()["Decouple"]->active = false;
}

// ModuleAnchoredDecoupler.cs:91
std::string ModuleAnchoredDecoupler::GetInfo() { return "" + Localizer::Format("#autoLOC_240283", std::vector<std::string>{FormatForce(Get<float>("ejectionForce"))}); }

// ModuleAnchoredDecoupler.cs:96
std::string ModuleAnchoredDecoupler::GetModuleDisplayName() { return Localizer::Format("#autoLOC_6001040"); }

// ============================================================================================ IStageSeparator stand-ins

namespace {
// LaunchClamp.cs:369: `public int GetStageIndex(int fallback) { return base.part.inverseStage; }` -- the clamp's other behaviour is
// the physics package's; this stand-in only answers the staging query (replaced when LaunchClamp is ported)
class LaunchClampStageSeparator : public PartModule, public IStageSeparator {
public:
    using PartModule::PartModule;
    int GetStageIndex(int /*fallback*/) override { return dynamic_cast<Part*>(part)->fields.inverseStage; }
};
// ModuleDockingNode.cs:1944: `return fallback + 1;` (the staging overrides are PartModule's class rules, module_staging.cpp)
class DockingNodeStageSeparator : public PartModule, public IStageSeparator {
public:
    using PartModule::PartModule;
    int GetStageIndex(int fallback) override { return fallback + 1; }
};
}  // namespace

namespace detail {
void RegisterDecouplerModules() {
    ModuleFactory::Register("ModuleDecouplerBase", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleDecouplerBase>(c)); });
    ModuleFactory::Register("ModuleDecouple", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleDecouple>(c)); });
    ModuleFactory::Register("ModuleAnchoredDecoupler", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleAnchoredDecoupler>(c)); });
    ModuleFactory::Register("LaunchClamp", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<LaunchClampStageSeparator>(c)); });
    ModuleFactory::Register("ModuleDockingNode", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<DockingNodeStageSeparator>(c)); });
}
}  // namespace detail

}  // namespace ksp
