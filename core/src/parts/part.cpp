// WP-13: Part (Part.cs), the engine-agnostic part model: tree, attach nodes, resources, mass, crew, state.
// Modules, copy semantics and events are in part_modules.cpp. Every function cites Part.cs:line (Assembly-CSharp) or the
// file named in its comment. Float32 paths follow the Mono rule (ksp/dotnet/mono_float.hpp).
#include "ksp/parts/part.hpp"

#include <algorithm>
#include <cmath>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

using KSP::Localization::Localizer;

// ---- PartTransform (Unity Transform stand-in) ------------------------------------------------------------------------------
namespace {
Quaternionf Conjugate(const Quaternionf& q) { return {-q.x, -q.y, -q.z, q.w}; }
Vector3f InvScale(const Vector3f& v, const Vector3f& s) {
    return {f32store(fx(v.x) / fx(s.x)), f32store(fx(v.y) / fx(s.y)), f32store(fx(v.z) / fx(s.z))};
}
}  // namespace

Vector3f PartTransform::TransformPoint(const Vector3f& p) const {
    return rotation * Vector3f::Scale(localScale, p) + position;
}
Vector3f PartTransform::InverseTransformPoint(const Vector3f& p) const {
    return InvScale(Conjugate(rotation) * (p - position), localScale);
}
Vector3f PartTransform::TransformDirection(const Vector3f& d) const { return rotation * d; }
Vector3f PartTransform::InverseTransformDirection(const Vector3f& d) const { return Conjugate(rotation) * d; }
Vector3f PartTransform::up() const { return rotation * Vector3f::up(); }
Vector3f PartTransform::forward() const { return rotation * Vector3f::forward(); }

// ---- PartValues.cs ---------------------------------------------------------------------------------------------------------
namespace {
bool Gt(float a, float b) { return a > b; }
bool Lt(float a, float b) { return a < b; }
bool GtI(int32_t a, int32_t b) { return a > b; }
float Add(float a, float b) { return f32store(fx(a) + fx(b)); }
}  // namespace

PartValues::PartValues()
    : MaxThrottle("MaxThrottle", 1.0f, Delegate<bool(float, float)>::Static(&Gt)),
      HeatProduction("HeatProduction", 1.0f, Delegate<bool(float, float)>::Static(&Lt)),
      FuelUsage("FuelUsage", 1.0f, Delegate<bool(float, float)>::Static(&Lt)),
      EnginePower("EnginePower", 1.0f, Delegate<bool(float, float)>::Static(&Gt)),
      SteeringRadius("SteeringRadius", 1.0f, Delegate<bool(float, float)>::Static(&Gt)),
      AutopilotSkill("PilotSkill", -1, Delegate<bool(int32_t, int32_t)>::Static(&GtI)),
      AutopilotKerbalSkill("PilotKerbalSkill", -1, Delegate<bool(int32_t, int32_t)>::Static(&GtI)),
      AutopilotSASSkill("PilotSASSkill", -1, Delegate<bool(int32_t, int32_t)>::Static(&GtI)),
      RepairSkill("EngineerSkill", -1, Delegate<bool(int32_t, int32_t)>::Static(&GtI)),
      FailureRepairSkill("EngineerSkill", -1, Delegate<bool(int32_t, int32_t)>::Static(&GtI)),
      ScienceSkill("ScientistSkill", -1, Delegate<bool(int32_t, int32_t)>::Static(&GtI)),
      CommsRange("CommsRange", 1.0f, Delegate<bool(float, float)>::Static(&Gt)),
      ScienceReturnSum("ScienceReturn", 1.0f, Delegate<float(float, float)>::Static(&Add)),
      ScienceReturnMax("ScienceReturn_Max", 1.0f, Delegate<bool(float, float)>::Static(&Gt)),
      EVAChuteSkill("EVAChuteSkill", 0, Delegate<bool(int32_t, int32_t)>::Static(&GtI)) {
    // EventValueWrapper's constructor collects the Update() of the public fields in declaration order
    RegisterUpdates(MaxThrottle, HeatProduction, FuelUsage, EnginePower, SteeringRadius, AutopilotSkill, AutopilotKerbalSkill,
                    AutopilotSASSkill, RepairSkill, FailureRepairSkill, ScienceSkill, CommsRange, ScienceReturnSum,
                    ScienceReturnMax, EVAChuteSkill);
}

// ---- AttachNode.cs (the parts that need a Part) ------------------------------------------------------------------------------
// AttachNode.cs:86
AttachNode AttachNode::FromTransform(std::string id_, const Vector3f& transformPosition, const Vector3f& transformForward,
                                     int32_t size_, AttachNodeMethod attachMethod_, bool crossfeed, bool rigid_) {
    AttachNode n;
    n.id = std::move(id_);
    n.position = transformPosition;
    n.orientation = transformForward;
    n.secondaryAxis = Vector3f::zero();
    n.originalPosition = transformPosition;
    n.originalOrientation = transformForward;
    n.originalSecondaryAxis = Vector3f::zero();
    n.radius = 0.4f;
    n.offset = Vector3f::zero();
    n.owner = nullptr;
    n.attachedPart = nullptr;
    n.attachMethod = attachMethod_;
    n.size = size_;
    n.AllowOneWayXFeed = true;
    n.ResourceXFeed = crossfeed;
    n.rigid = rigid_;
    return n;
}

// AttachNode.cs:107 (KSPUtil.cs:2076 GetPartByCraftID: the last part with that craftID; id 0 matches nothing)
void AttachNode::FindAttachedPart(const std::vector<Part*>& parts) {
    attachedPart = nullptr;
    if (attachedPartId == 0) return;
    for (Part* p : parts)
        if (p && p->fields.craftID == attachedPartId) attachedPart = p;
}

// AttachNode.cs:112
AttachNode* AttachNode::FindOpposingNode() const {
    if (attachedPart != nullptr) return attachedPart->FindAttachNodeByPart(owner);
    return nullptr;
}

// AttachNode.cs:132
void AttachNode::ReverseSrfNodeDirection(AttachNode& fromNode) {
    if (owner == nullptr || fromNode.owner == nullptr) return;
    fromNode.attachedPart = nullptr;
    attachedPart = fromNode.owner;
    const Vector3f vector = fromNode.owner->partTransform.TransformPoint(fromNode.position);
    position = owner->partTransform.InverseTransformPoint(vector);
}

// AttachNode.cs:143 (the `num > 0` test is the source's, not `>= 0`)
void AttachNode::ChangeSrfNodePosition() {
    if (attachedPart != nullptr || owner == nullptr) return;
    const int count = static_cast<int>(owner->attachNodes.size());
    const int count2 = static_cast<int>(owner->children.size());
    std::vector<bool> array(static_cast<size_t>(count), false);
    int num = -1;
    for (int i = 0; i < count; i++) {
        AttachNode& attachNode = owner->attachNodes[static_cast<size_t>(i)];
        array[static_cast<size_t>(i)] = attachNode.attachedPart == nullptr;
        if ((position - attachNode.position).sqrMagnitude() < nodeDistEpsilon) num = i;
        if (!array[static_cast<size_t>(i)]) continue;
        for (int j = 0; j < count2; j++) {
            Part* part = owner->children[static_cast<size_t>(j)];
            AttachNode* srf = part->srfAttachNode.get();
            if (srf->attachedPart == owner &&
                !((owner->partTransform.TransformPoint(attachNode.position) - part->partTransform.TransformPoint(srf->position))
                      .sqrMagnitude() >= nodeDistEpsilon)) {
                array[static_cast<size_t>(i)] = false;
                break;
            }
        }
    }
    if (num > 0 && array[static_cast<size_t>(num)]) return;
    int num2 = 0;
    while (true) {
        if (num2 < count) {
            if (array[static_cast<size_t>(num2)]) break;
            num2++;
            continue;
        }
        log::Warning("[AttachNode]: Unable to invert surface node on " + (owner->partInfo ? owner->partInfo->title : std::string()) +
                     ". Set to center of part.");
        position = Vector3f::zero();
        orientation = originalOrientation;
        secondaryAxis = originalSecondaryAxis;
        return;
    }
    position = owner->attachNodes[static_cast<size_t>(num2)].position;
    orientation = owner->attachNodes[static_cast<size_t>(num2)].orientation;
    secondaryAxis = originalSecondaryAxis;
}

// ---- Part: construction, statics, properties ---------------------------------------------------------------------------------
Part::Part() {
    instanceID = nextInstanceID_;
    nextInstanceID_ += 2;
    allParts.push_back(this);
}

Part::~Part() {
    allParts.erase(std::remove(allParts.begin(), allParts.end(), this), allParts.end());
}

// Part.cs:6284
int Part::NewRequestID() {
    if (ResourceRequestID_ < INT32_MAX) ResourceRequestID_++;
    else ResourceRequestID_ = 0;
    return ResourceRequestID_;
}

// Part.cs:4674
uint32_t Part::getFuelReqId() {
    fuelRequestID++;
    return fuelRequestID;
}

// Part.cs:6297
bool Part::AlreadyProcessedRequest(int requestID) {
    if (lastRequestID_ == requestID) return true;
    lastRequestID_ = requestID;
    return false;
}

// Part.cs:873
bool Part::isVesselEVA() const {
    if (vessel == nullptr) return false;
    return vesselHooks.isEVA ? vesselHooks.isEVA(*vessel) : false;
}

// Part.cs:897
Part* Part::localRoot() {
    if (attached_ && parent != nullptr && parent->state_ != PartStates::DEAD) return parent->localRoot();
    return this;
}

// Part.cs:909
void Part::set_State(PartStates value) {
    if (value == PartStates::FAILED) {
        if (CanFail()) {
            if (state_ == PartStates::ACTIVE) fields.PreFailState = static_cast<int32_t>(PartStates::IDLE);
            else if (state_ != PartStates::FAILED) fields.PreFailState = static_cast<int32_t>(state_);
            state_ = value;
        }
    } else {
        state_ = value;
    }
}

// Part.cs:939
bool Part::hasStagingIcon() const { return fields.stagingIcon != std::string(); }

// Part.cs:955
bool Part::isAttachable() const {
    if (static_cast<AttachModes>(fields.attachMode) == AttachModes::SRF_ATTACH) return true;
    for (const AttachNode& n : attachNodes)
        if (n.attachedPart == nullptr) return true;
    return false;
}

// Part.cs:1030
bool Part::CanFail() const {
    if (SaveEnvironment::Current().IsExpansionInstalled("MakingHistory")) return canFail_;
    return false;
}
void Part::set_CanFail(bool v) {
    if (SaveEnvironment::Current().IsExpansionInstalled("MakingHistory")) canFail_ = v;
}

// Part.cs:1171 (isShieldedDisplay = Localizer.Format(partIsShielded ? cacheAutoLOC_439839 : cacheAutoLOC_439840))
void Part::set_ShieldedFromAirstream(bool v) {
    partIsShielded_ = v;
    fields.isShieldedDisplay = Localizer::Format(partIsShielded_ ? "#autoLOC_439839" : "#autoLOC_439840");
}

// Part.cs:1091, classID = className.GetHashCode() at Part.cs:54
int Part::ClassID() const { return MonoStringHash(className); }

// Part.cs:1184
Part* Part::RigidBodyPart() {
    if (!hasRigidbody) {
        if (parent == nullptr) return nullptr;
        return parent->RigidBodyPart();
    }
    return this;
}

// ---- symmetry and type queries ------------------------------------------------------------------------------------------------
// Part.cs:1227 (index %= count + 1; a negative remainder is an ArgumentOutOfRangeException, sic)
Part* Part::getSymmetryCounterPart(int index) {
    index %= static_cast<int>(symmetryCounterparts.size()) + 1;
    if (index == 0) return this;
    return symmetryCounterparts.at(static_cast<size_t>(index - 1));
}

// Part.cs:1237 (scans from the end)
bool Part::isSymmetryCounterPart(const Part* cPart) const {
    int count = static_cast<int>(symmetryCounterparts.size());
    while (count-- > 0)
        if (symmetryCounterparts[static_cast<size_t>(count)] == cPart) return true;
    return false;
}

bool Part::isKerbalEVA() { return FindModuleOfClass("KerbalEVA") != nullptr; }                    // Part.cs:1254
bool Part::isCargoPart() { return FindModuleOfClass("ModuleCargoPart") != nullptr; }              // :1259
bool Part::isInventoryPart() { return FindModuleOfClass("ModuleInventoryPart") != nullptr; }      // :1264
bool Part::isGroundDeployable() { return FindModuleOfClass("ModuleGroundPart") != nullptr; }      // :1269
bool Part::isLaunchClamp() { return FindModuleOfClass("LaunchClamp") != nullptr; }                // :1274
bool Part::isAnchoredDecoupler() { return FindModuleOfClass("ModuleAnchoredDecoupler") != nullptr; }  // :1279
bool Part::isDecoupler() { return FindModuleOfClass("ModuleDecouple") != nullptr; }               // :1285
bool Part::isDockingPort() { return FindModuleOfClass("ModuleDockingNode") != nullptr; }          // :1291
// Part.cs:1297: only the intake of IntakeAir counts
bool Part::isAirIntake() {
    PartModule* intake = FindModuleOfClass("ModuleResourceIntake");
    if (intake != nullptr && intake->Get<std::string>("resourceName") != "IntakeAir") intake = nullptr;
    return intake != nullptr;
}
bool Part::isSolarPanel() { return FindModuleOfClass("ModuleDeployableSolarPanel") != nullptr; }  // :1307
bool Part::isRadiator() { return FindModuleOfClass("ModuleDeployableRadiator") != nullptr; }      // :1313
bool Part::isAntenna() { return FindModuleOfClass("ModuleDeployableAntenna") != nullptr; }        // :1319
bool Part::isGenerator() { return FindModuleOfClass("ModuleGenerator") != nullptr; }              // :1325
bool Part::isFairing() { return FindModuleOfClass("ModuleProceduralFairing") != nullptr; }        // :1331
bool Part::isEngine() { return FindModuleOfClass("ModuleEngines") != nullptr; }                   // :1336
bool Part::isParachute() { return FindModuleOfClass("ModuleParachute") != nullptr; }              // :1351
// Part.cs:1362 isRobotic(out IRoboticServo): Serenity installed and a BaseServo (the only IRoboticServo implementer)
bool Part::isRobotic() {
    if (SaveEnvironment::Current().IsExpansionInstalled("Serenity")) return FindModuleOfClass("BaseServo") != nullptr;
    return false;
}
bool Part::isControlSurface() { return FindModuleOfClass("ModuleControlSurface") != nullptr; }    // :1458
bool Part::isKerbalSeat() { return FindModuleOfClass("KerbalSeat") != nullptr; }                  // :1498

// Part.cs:1525
const std::string* Part::findFxGroup(const std::string& groupID) const {
    for (const std::string& g : fxGroupNames)
        if (g == groupID) return &g;
    return nullptr;
}

// ---- lifecycle -------------------------------------------------------------------------------------------------------------------
// Part.cs:1593
void Part::OnLoad() {
    for (const char* n : {"prelaunch", "activate", "active", "deactivate"}) fxGroupNames.emplace_back(n);
    // Fields.SetOriginalValue(): the Part's own [KSPField]s are plain PartFields here
    onPartLoad();
}

// Part.cs:1603 (PartLoader.getPartInfoByName(partInfo.name) re-links a stale AvailablePart: the live loader is optional)
void Part::RelinkPrefab() {
    if (partInfo != nullptr && relinkPartInfo) {
        const AvailablePart* p = relinkPartInfo(partInfo->name);
        if (p != nullptr) partInfo = p;
    }
}

// Part.cs:1615 (Unity-side setup omitted: model component caches, ModularSetup of the Part's own field/event/action lists,
// colliders, highlighter, stack icon, airlock lookup). modules_ already holds the module components (PartModuleList ctor).
void Part::Awake() {
    RelinkPrefab();
    fields.craftID = static_cast<uint32_t>(instanceID);
    for (PartModule* m : modules_.GetModules([](PartModule&) { return true; })) m->part = this;
    if (!fields.partRendererBoundsIgnore) fields.partRendererBoundsIgnore = std::vector<std::string>();
    // AttributeSetup (:6195): PartAttributes.partInfo is empty for Part / CompoundPart -> partName = className
    fields.partName = className;
    SetupResources();
    SetupSimulationResources();
    DragCubes.SetPart(this);  // :1633 (WP-21)
    onPartAwake();
    connected_ = true;
    attached_ = true;
    fields.originalStage = fields.inverseStage;
    dockingPorts.clear();
    // isShieldedDisplay = Localizer.Format(partIsShielded ? cacheAutoLOC_439839 : cacheAutoLOC_439840)
    fields.isShieldedDisplay = Localizer::Format("#autoLOC_439840");
    awoken_ = true;
}

// Part.cs:1663 Start(), the data block (everything not about Unity components, audio, UI, joints or coroutines)
void Part::StartData(const StartContext& ctx) {
    // :1664-1676
    uint32_t vesselId = 0;
    fields.persistentId = SaveEnvironment::Current().persistentIds.CheckPartpersistentId(fields.persistentId, this, false, true, vesselId);
    // :1678
    if (fields.skinMaxTemp < 0.0) fields.skinMaxTemp = fields.maxTemp;
    if (fields.absorptiveConstant < 0.0) fields.absorptiveConstant = fields.emissiveConstant;
    // :1764
    if (fields.PhysicsSignificance != -1) {
        if (ctx.flight && parent != nullptr && parent->isRobotic()) {
            PromoteToPhysicalPart();
            if (fields.prefabMass < 0.0065f) {
                fields.prefabMass = 0.0065f;
                fields.mass = 0.0065f;
            }
        } else {
            fields.PhysicsSignificance = Mathf::Max(0, fields.PhysicsSignificance);
            fields.physicalSignificance = fields.PhysicsSignificance;
            fields.previousPhysicalSignificance = fields.physicalSignificance;
        }
    }
    if (ctx.flight) {
        // :1805
        if (fields.temperature < 0.0 && ctx.externalTemperature)
            fields.temperature = static_cast<double>(static_cast<float>(*ctx.externalTemperature));
        if (fields.physicalSignificance != 0 && vessel != nullptr && ctx.isRootOfVessel) {
            fields.physicalSignificance = static_cast<int32_t>(Part_PhysicalSignificance::FULL);
            fields.previousPhysicalSignificance = fields.physicalSignificance;
        }
        // :1857
        double thermalMassOfResources = 0.0;
        fields.resourceMass = GetResourceMass(thermalMassOfResources);
        fields.resourceThermalMass = thermalMassOfResources;
        fields.thermalMass = static_cast<double>(fields.mass) * ctx.standardSpecificHeatCapacity * fields.thermalMassModifier +
                             fields.resourceThermalMass;
        fields.thermalMassReciprocal = 1.0 / dotnet::Math::Max(fields.thermalMass, 0.001);
        if (fields.thermalMass > 0.0 && fields.temperature == -1.0) {
            fields.skinUnexposedTemperature = 288.0;
            fields.skinTemperature = 288.0;
            fields.temperature = 288.0;
        }
        fields.started = true;
    } else if (ctx.editor) {
        fields.editorStarted = true;
    }
}

// ---- crew ----------------------------------------------------------------------------------------------------------------------
// Part.cs:2279 (the InternalModel seat logic, RegisterExperienceTraits and Vessel.CrewListSetDirty are other packages')
bool Part::AddCrewmember(const std::shared_ptr<ProtoCrewMember>& crew) {
    for (const auto& c : protoModuleCrew)
        if (c == crew) {
            log::Warning("Cannot add crewmember " + crew->name().value_or(std::string()) + " to " + gameObjectName + ". Crewmember is already in this part.");
            return false;
        }
    if (static_cast<int>(protoModuleCrew.size()) < fields.CrewCapacity) {
        crew->set_rosterStatus(ProtoCrewMember::RosterStatus::Assigned);
        protoModuleCrew.push_back(crew);
        crew->seatIdx = -1;
        if (vessel != nullptr && vesselHooks.crewListSetDirty) vesselHooks.crewListSetDirty(*vessel);
        return true;
    }
    log::Warning("Cannot add crewmember " + crew->name().value_or(std::string()) + " to " + gameObjectName + ". Part Crew Capacity Exceeded.");
    return false;
}

// Part.cs:2310 (no InternalModel: the seat index is stored on the crew member)
bool Part::AddCrewmemberAt(const std::shared_ptr<ProtoCrewMember>& crew, int seatIndex) {
    for (const auto& c : protoModuleCrew)
        if (c == crew) {
            log::Error("Cannot add crewmember " + crew->name().value_or(std::string()) + " to " + gameObjectName + ". Crewmember is already in this part.");
            return false;
        }
    if (static_cast<int>(protoModuleCrew.size()) < fields.CrewCapacity) {
        crew->set_rosterStatus(ProtoCrewMember::RosterStatus::Assigned);
        protoModuleCrew.push_back(crew);
        crew->seatIdx = seatIndex;
        if (vessel != nullptr && vesselHooks.crewListSetDirty) vesselHooks.crewListSetDirty(*vessel);
        return true;
    }
    log::Error("Cannot add crewmember " + crew->name().value_or(std::string()) + " to " + gameObjectName + ". Part Crew Capacity Exceeded.");
    return false;
}

// Part.cs:2260
void Part::RemoveCrewmember(const std::shared_ptr<ProtoCrewMember>& crew) {
    auto it = std::find(protoModuleCrew.begin(), protoModuleCrew.end(), crew);
    if (it != protoModuleCrew.end()) {
        crew->set_rosterStatus(ProtoCrewMember::RosterStatus::Available);
        protoModuleCrew.erase(it);
        if (vessel != nullptr && vesselHooks.removeCrew) vesselHooks.removeCrew(*vessel, crew);
    } else {
        log::Warning("Cannot remove crewmember " + crew->name().value_or(std::string()) + " from " + gameObjectName + ". Crewmember is not in this part.");
    }
}

// ---- tree ----------------------------------------------------------------------------------------------------------------------
// Part.cs:2437
void Part::setParent(Part* p) {
    if (p != nullptr) {
        GameEvents::onPartAttach.Fire(GameEvents::HostTargetAction<Part*, Part*>(this, p));
        p->addChild(this);
    } else if (parent != nullptr) {
        GameEvents::onPartRemove.Fire(GameEvents::HostTargetAction<Part*, Part*>(p, this));
        parent->removeChild(this);
    }
    parent = p;
    potentialParent = p;
    connected_ = p != nullptr;
}

// Part.cs:2454
void Part::clearParent() {
    parent = nullptr;
    potentialParent = nullptr;
    connected_ = false;
}

// Part.cs:2461 (SendMessage "OnChildAdd": no receiver in the engine-agnostic model)
void Part::addChild(Part* child) { children.push_back(child); }

// Part.cs:2467
void Part::removeChild(Part* child) {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        children.erase(it);
    } else if (child != nullptr) {
        log::Warning("Part Warning: part " + child->gameObjectName + " is not a child of " + gameObjectName);
    } else {
        log::Error("Part Error: trying to remove a null child from " + gameObjectName);
    }
}

// Part.cs:2484
bool Part::hasIndirectChild(const Part* tgtPart) const {
    if (tgtPart == this) return true;
    for (const Part* c : children)
        if (c->hasIndirectChild(tgtPart)) return true;
    return false;
}

// Part.cs:2508 (sic: the recursion passes the parent itself, so a part with a parent always answers true)
bool Part::hasIndirectParent(const Part* tgtPart) const {
    if (tgtPart == this) return true;
    if (parent != nullptr) return parent->hasIndirectParent(parent);
    return false;
}

// Part.cs:2521 (RefreshHighlighter, CycleAutoStrut, SetRemoveSymmetryVisibililty and StageManager.GenerateStagingSequence
// are the UI / joint / staging packages')
void Part::onAttach(Part* parent_, bool first) {
    (void)first;
    onPartAttach(parent_);
    OnEditorAttach();
    connected_ = true;
    for (size_t i = 0; i < children.size(); i++) children[i]->onAttach(this, false);
}

// Part.cs:2639 (ReleaseAutoStruts: the joint package's)
void Part::onDetach(bool /*first*/) {
    onPartDetach();
    OnEditorDetach();
    connected_ = false;
    for (size_t i = 0; i < children.size(); i++) children[i]->onDetach(false);
}

// Part.cs:2712
void Part::OnDelete() {
    SaveEnvironment& env = SaveEnvironment::Current();
    if (env.persistentIds.fetch) env.persistentIds.persistentLoadedPartIds.Remove(fields.persistentId);
    onPartDelete();
    OnEditorDestroy();
}

// Part.cs:2728
void Part::force_activate(bool /*playFX*/) {
    state_ = PartStates::ACTIVE;
    if (!onPartActivate()) {
        state_ = PartStates::IDLE;
        log::Info("[" + gameObjectName + "]: Did not activate");
        return;
    }
    ModulesOnActivate();
}

// Part.cs:2748
bool Part::activate(int currentStage, const Vessel* activeVessel) {
    if (state_ == PartStates::DEAD) return false;
    if (currentStage != fields.inverseStage) return false;
    if (vessel != activeVessel && !fields.ActivatesEvenIfDisconnected) return false;
    state_ = PartStates::ACTIVE;
    if (!onPartActivate()) {
        state_ = PartStates::IDLE;
        log::Info("[" + gameObjectName + "]: Did not activate");
        return false;
    }
    ModulesOnActivate();
    return true;
}

// Part.cs:2785
void Part::deactivate() {
    state_ = PartStates::DEACTIVATED;
    onPartDeactivate();
    ModulesOnDeactivate();
}

// Part.cs:2888
void Part::FindNonPhysicslessChildren(std::vector<Part*>& parts) {
    for (size_t i = 0; i < children.size(); i++) {
        if (children[i] != nullptr &&
            children[i]->fields.physicalSignificance == static_cast<int32_t>(Part_PhysicalSignificance::FULL)) {
            parts.push_back(children[i]);
            if (!children[i]->children.empty()) children[i]->FindNonPhysicslessChildren(parts);
        }
    }
}

// Part.cs:2907
Part* Part::FindNonPhysicslessParent() {
    if (parent == nullptr) return this;
    if (fields.physicalSignificance == static_cast<int32_t>(Part_PhysicalSignificance::FULL)) return this;
    return parent->FindNonPhysicslessParent();
}

// Part.cs:2920 (the state change only: the Rigidbody, CollisionEnhancer, PartBuoyancy and PartJoints belong to the physics
// engine; the modules' own promotion hooks run)
void Part::PromoteToPhysicalPart() {
    if (fields.physicalSignificance == static_cast<int32_t>(Part_PhysicalSignificance::FULL)) return;
    fields.physicalSignificance = static_cast<int32_t>(Part_PhysicalSignificance::FULL);
    hasRigidbody = true;
    for (size_t j = 0; j < modules_.Count(); j++) modules_.GetModule(j)->PromoteToPhysicalPart();
}

// Part.cs:2990
void Part::DemoteToPhysicslessPart() {
    if (fields.physicalSignificance != static_cast<int32_t>(Part_PhysicalSignificance::NONE) && parent != nullptr) {
        for (size_t i = 0; i < modules_.Count(); i++) modules_.GetModule(i)->DemoteToPhysicslessPart();
        fields.previousPhysicalSignificance = fields.physicalSignificance;
        fields.physicalSignificance = static_cast<int32_t>(Part_PhysicalSignificance::NONE);
        if (hasRigidbody && FindModuleOfClass("ModuleWheelBase") == nullptr) hasRigidbody = false;
    }
}

// Part.cs:3027
float Part::GetPhysicslessChildMass() {
    float num = 0.0f;
    for (size_t i = 0; i < children.size(); i++) {
        Part* part = children[i];
        if (!part->hasRigidbody) {
            const float num2 = part->GetResourceMass();
            num = f32store(fx(num) + (fx(part->fields.mass) + fx(num2)));
        }
    }
    return num;
}

// Part.cs:3043
AttachNode* Part::FindPartThroughNodes(Part* tgtPart, Part* src) {
    if (src == this) return nullptr;
    const int count = static_cast<int>(attachNodes.size());
    for (int num = 0; num < count; num++) {
        AttachNode& attachNode = attachNodes[static_cast<size_t>(num)];
        if (attachNode.attachedPart == tgtPart) return &attachNode;
        if (attachNode.attachedPart != nullptr && attachNode.attachedPart != src &&
            attachNode.attachedPart->FindPartThroughNodes(tgtPart, this) != nullptr)
            return &attachNode;
    }
    if (srfAttachNode) {
        if (srfAttachNode->attachedPart == tgtPart) return srfAttachNode.get();
        if (srfAttachNode->attachedPart != nullptr && srfAttachNode->attachedPart != src &&
            srfAttachNode->attachedPart->FindPartThroughNodes(tgtPart, this) != nullptr)
            return srfAttachNode.get();
    }
    return nullptr;
}

// Part.cs:3101
void Part::CleanSymmetryVesselReferencesRecursively() {
    CleanSymmetryVesselReferences();
    for (size_t i = 0; i < children.size(); i++) children[i]->CleanSymmetryVesselReferencesRecursively();
}

// Part.cs:3113
void Part::CleanSymmetryReferences() {
    int count = static_cast<int>(symmetryCounterparts.size());
    while (count-- > 0) {
        Part* part = symmetryCounterparts[static_cast<size_t>(count)];
        if (part == nullptr) continue;
        auto& pc = part->symmetryCounterparts;
        pc.erase(std::remove(pc.begin(), pc.end(), this), pc.end());
        symmetryCounterparts.erase(symmetryCounterparts.begin() + count);
    }
}

// Part.cs:3129
void Part::CleanSymmetryVesselReferences() {
    int count = static_cast<int>(symmetryCounterparts.size());
    while (count-- > 0) {
        Part* part = symmetryCounterparts[static_cast<size_t>(count)];
        if (part == nullptr) {
            symmetryCounterparts.erase(symmetryCounterparts.begin() + count);
        } else if (part->vessel != vessel) {
            auto& pc = part->symmetryCounterparts;
            pc.erase(std::remove(pc.begin(), pc.end(), this), pc.end());
            symmetryCounterparts.erase(symmetryCounterparts.begin() + count);
        }
    }
}

// Part.cs:3214 (the joint / transform re-parenting branch of the source is the physics engine's)
void Part::SetHierarchyRoot(Part* root) {
    Part* part = parent;
    const bool flag = srfAttachNode->attachedPart == part;
    if (root == this) parent = nullptr;
    else parent = root;
    if (part != nullptr) {
        addChild(part);
        part->removeChild(this);
        part->SetHierarchyRoot(this);
        if (flag && srfAttachNode->nodeType == AttachNodeType::Surface) {
            part->srfAttachNode->ReverseSrfNodeDirection(*srfAttachNode);
            part->fields.attachMode = static_cast<int32_t>(AttachModes::SRF_ATTACH);
        } else {
            part->fields.attachMode = static_cast<int32_t>(AttachModes::STACK);
        }
        srfAttachNode->ChangeSrfNodePosition();
    }
}

// Part.cs:3257
void Part::UpdateOrgPosAndRot(Part* newRoot) {
    fields.orgPos = newRoot->partTransform.InverseTransformPoint(partTransform.position);
    fields.orgRot = Conjugate(newRoot->partTransform.rotation) * partTransform.rotation;
}

// Part.cs:6949 (HashSet<Part>: contains check, insertion order kept)
void Part::GetPartsOutTo(Part* part, std::vector<Part*>& parts, int maxLinks) {
    if (std::find(parts.begin(), parts.end(), part) != parts.end()) return;
    parts.push_back(part);
    if (maxLinks <= 0) return;
    maxLinks--;
    int count = static_cast<int>(part->children.size());
    while (count-- > 0) {
        Part* part2 = part->children[static_cast<size_t>(count)];
        if (part2 != nullptr && part2 != part->parent) GetPartsOutTo(part2, parts, maxLinks);
    }
    if (part->parent != nullptr) GetPartsOutTo(part->parent, parts, maxLinks);
}

// Part.cs:4915
Part* Part::FindChildPart(const std::string& childName, bool recursive) {
    if (recursive) return FindChildPart(this, childName);
    for (Part* c : children)
        if (c->gameObjectName == childName) return c;
    return nullptr;
}

// Part.cs:4941
Part* Part::FindChildPart(Part* parent_, const std::string& childName) {
    if (parent_ == nullptr) return nullptr;
    for (Part* part2 : parent_->children) {
        if (part2->gameObjectName == childName) return part2;
        Part* part = FindChildPart(part2, childName);
        if (part != nullptr) return part;
    }
    return nullptr;
}

// Part.cs:8848 (stack icon and editor backup are UI)
void Part::RemoveFromSymmetry() { CleanSymmetryReferences(); }

// Part.cs:8960 (CacheIConstructionPartModules: the modules implementing IConstruction, in module order)
bool Part::PartCanBeDetached() {
    bool flag = true;
    for (size_t i = 0; i < modules_.Count(); i++) {
        IConstruction* c = modules_.GetModule(i)->AsConstruction();
        if (c != nullptr && !(flag = c->CanBeDetached())) break;
    }
    if (flag) return !fields.hasKerbalOnLadder;
    return false;
}

namespace {
// the vesselType test shared by PartCanBeOffset / PartCanBeRotated (Part.cs:8982, :9005)
bool OffsetBlockedByVesselType(const Part& p) {
    const auto vt = static_cast<VesselType>(p.fields.vesselType);
    return vt == VesselType::DroppedPart ||
           (vt == VesselType::Debris && p.vessel != nullptr && Part::vesselHooks.partCount && Part::vesselHooks.partCount(*p.vessel) == 1) ||
           vt == VesselType::EVA;
}
}  // namespace

// Part.cs:8978
bool Part::PartCanBeOffset() {
    bool flag = true;
    if (!OffsetBlockedByVesselType(*this)) {
        for (size_t i = 0; i < modules_.Count(); i++) {
            IConstruction* c = modules_.GetModule(i)->AsConstruction();
            if (c != nullptr && !(flag = c->CanBeOffset())) break;
        }
        if (flag) return !fields.hasKerbalOnLadder;
        return false;
    }
    return false;
}

// Part.cs:9001 (sic: asks CanBeOffset, not CanBeRotated)
bool Part::PartCanBeRotated() {
    bool flag = true;
    if (!OffsetBlockedByVesselType(*this)) {
        for (size_t i = 0; i < modules_.Count(); i++) {
            IConstruction* c = modules_.GetModule(i)->AsConstruction();
            if (c != nullptr && !(flag = c->CanBeOffset())) break;
        }
        if (flag) return !fields.hasKerbalOnLadder;
        return false;
    }
    return false;
}

// ---- attach nodes --------------------------------------------------------------------------------------------------------------
// Part.cs:6006
AttachNode* Part::AddAttachNode(const ConfigNode& node,
                                const std::function<std::optional<std::pair<Vector3f, Vector3f>>(const std::string&)>& findTransform) {
    const auto value = node.GetValue("name");
    const auto value2 = node.GetValue("transform");
    if (!value) throw Exception("Part: Cannot add attach node. Node requires a 'name' value.");
    if (!value2) throw Exception("Part: Cannot add attach node. Node requires a 'transform' value.");
    std::optional<std::pair<Vector3f, Vector3f>> transform;
    if (findTransform) transform = findTransform(*value2);
    if (!transform) throw Exception("Part: Cannot add attach node. Transform of name '" + *value2 + "' not found");
    int32_t result = 1;
    if (node.HasValue("size") && !cs::IntTryParse(*node.GetValue("size"), result)) {
        log::Error("Part: Attach Node ('" + *value + "') 'size' value is incorrect format");
        result = 1;
    }
    AttachNodeMethod method = AttachNodeMethod::FIXED_JOINT;
    if (*value == "srfAttach" || *value == "attach") method = AttachNodeMethod::HINGE_JOINT;
    if (node.HasValue("method")) {
        int64_t mv;
        if (kEnum_AttachNodeMethod.TryParse(*node.GetValue("method"), mv, true)) method = static_cast<AttachNodeMethod>(mv);
        else log::Error("Part: Attach Node ('" + *value + "') 'method' value is incorrect format");
    }
    bool crossfeed = true;
    node.TryGetValue("crossfeed", crossfeed);
    bool rigid = false;
    node.TryGetValue("rigid", rigid);
    AttachNode an = AttachNode::FromTransform(*value, transform->first, transform->second, result, method, crossfeed, rigid);
    an.nodeTransform = *value2;
    an.owner = this;
    attachNodes.push_back(std::move(an));
    return &attachNodes.back();
}

// Part.cs:6054 (the node transforms come from the model: nodes without a transform name keep their values)
void Part::UpdateAttachNodes() {
    for (AttachNode& n : attachNodes) {
        if (n.nodeTransform.empty() || !nodeTransformResolver) continue;
        if (auto t = nodeTransformResolver(n.nodeTransform)) {
            n.position = partTransform.InverseTransformPoint(t->first);
            n.orientation = partTransform.InverseTransformDirection(t->second);
        }
    }
}

// Part.cs:6068
void Part::UpdateSrfAttachNode() {
    if (srfAttachNode && !srfAttachNode->nodeTransform.empty() && nodeTransformResolver) {
        if (auto t = nodeTransformResolver(srfAttachNode->nodeTransform)) {
            srfAttachNode->position = partTransform.InverseTransformPoint(t->first);
            srfAttachNode->orientation = partTransform.InverseTransformDirection(t->second);
        }
    }
}

// Part.cs:6077 (topNode is TopNode(); srfAttachNode is the "attach" node renamed "srfAttach", else "srfAttach")
void Part::SetupAttachNodes() {
    AttachNode* attach = FindAttachNode("attach");
    if (attach == nullptr) {
        AttachNode* srf = FindAttachNode("srfAttach");
        srfAttachNode = srf ? std::make_shared<AttachNode>(*srf) : nullptr;
    } else {
        attach->id = "srfAttach";
        srfAttachNode = std::make_shared<AttachNode>(*attach);
    }
}

// Part.cs:6091
AttachNode* Part::FindAttachNode(std::string_view nodeId) {
    for (AttachNode& n : attachNodes)
        if (n.id == nodeId) return &n;
    return nullptr;
}

// Part.cs:6113
std::vector<AttachNode*> Part::FindAttachNodes(std::string_view partialNodeId) {
    std::vector<AttachNode*> list;
    for (AttachNode& n : attachNodes)
        if (n.id.find(partialNodeId) != std::string::npos) list.push_back(&n);
    return list;
}

// Part.cs:6132 (a null srfAttachNode is a NullReferenceException when no node matched, as in the source)
AttachNode* Part::FindAttachNodeByPart(const Part* connectedPart) {
    if (connectedPart == nullptr) return nullptr;
    for (AttachNode& n : attachNodes)
        if (n.attachedPart == connectedPart) return &n;
    if (!srfAttachNode) throw NullReferenceException();
    if (srfAttachNode->attachedPart == connectedPart) return srfAttachNode.get();
    return nullptr;
}

// Part.cs:7001
void Part::SetMirror(const Vector3f& mirrorVector_) {
    fields.mirrorVector = mirrorVector_;
    fields.isMirrored = !(mirrorVector_ == Vector3f::one());
    fields.mirrorAxis = Vector3f::Scale(fields.mirrorRefAxis, mirrorVector_);
    for (AttachNode& n : attachNodes) {
        n.position = Vector3f::Scale(n.originalPosition, mirrorVector_);
        n.orientation = Vector3f::Scale(n.originalOrientation, mirrorVector_);
    }
}

// ---- resources -------------------------------------------------------------------------------------------------------------------
// Part.cs:6307 (WP-15: over PartResourceList; `_resources == null || !IsValid` is !Resources.IsValid())
void Part::SetupResources() {
    if (!Resources.IsValid()) {
        const PartResourceList* prefabList =
            (partInfo == nullptr || partInfo->partPrefab == nullptr) ? nullptr : &partInfo->partPrefab->Resources;
        Resources = PartResourceList(this, prefabList);
    }
}

// Part.cs:6315
void Part::SetupSimulationResources() {
    if (!Resources.IsValid()) SetupResources();
    if (!simulationResources_.IsValid()) simulationResources_ = PartResourceList(this, &Resources, true);
}

// Part.cs:6327
void Part::ResetSimulationResources() { simulationResources_.RefreshSimulationListAmounts(&Resources); }

// Part.cs:6332
void Part::ResetSimulationResources(const PartResourceList* sourceList) { simulationResources_.RefreshSimulationListAmounts(sourceList); }

// Part.cs:6337
void Part::ResetSimulation() {
    if (simulationResources_.IsValid()) ResetSimulationResources();
    else SetupSimulationResources();
}

// Part.cs:6349
PartResource* Part::AddResource(const ConfigNode& node) {
    SetupSimulationResources();
    const auto name = node.GetValue("name");
    PartResource* partResource = name ? simulationResources_.Get(*name) : nullptr;  // (null name: NullReferenceException in C#)
    if (partResource != nullptr) partResource->Load(node);
    else simulationResources_.Add(node);
    return Resources.Add(node);
}

// Part.cs:6365
void Part::SetResource(const ConfigNode& node) {
    const auto value = node.GetValue("name");
    if (value && !value->empty()) {
        PartResource* partResource = Resources.Get(*value);
        if (partResource != nullptr) partResource->Load(node);
        else AddResource(node);
    }
}

// PartResourceList.Get(name)
PartResource* Part::GetResource(std::string_view name) { return Resources.Get(name); }

// Part.cs:6382
bool Part::RemoveResource(const PartResource* res) {
    SetupSimulationResources();
    simulationResources_.Remove(*res);
    return Resources.Remove(*res);
}

// Part.cs:6389
bool Part::RemoveResource(std::string_view rName) {
    SetupSimulationResources();
    simulationResources_.Remove(rName);
    return Resources.Remove(rName);
}

// Part.cs:6396
bool Part::RemoveResource(int resID) {
    SetupSimulationResources();
    simulationResources_.Remove(resID);
    return Resources.Remove(resID);
}

// Part.cs:6547 (double sum, one float rounding)
float Part::GetResourceMass(bool simulate) {
    double num = 0.0;
    PartResourceList& list = simulate ? simulationResources_ : Resources;
    int index = list.Count();
    while (index-- > 0) {
        const PartResource& partResource = list.At(index);
        num += partResource.amount * static_cast<double>(partResource.info->density());
    }
    return static_cast<float>(num);
}

// Part.cs:6559 (float products: `(float)amount * density`, `thermalMass += num2 * specificHeatCapacity` in float)
float Part::GetResourceMass(float& thermalMass) {
    float num = 0.0f;
    thermalMass = 0.0f;
    int count = Resources.Count();
    while (count-- > 0) {
        const PartResource& partResource = Resources.At(count);
        const float num2 = f32store(fx(static_cast<float>(partResource.amount)) * fx(partResource.info->density()));
        num = f32store(fx(num) + fx(num2));
        thermalMass = f32store(fx(thermalMass) + fx(num2) * fx(partResource.info->specificHeatCapacity()));
    }
    return num;
}

// Part.cs:6574 (thermalMass is a double: `thermalMass += num2 * specificHeatCapacity`)
float Part::GetResourceMass(double& thermalMass) {
    float num = 0.0f;
    thermalMass = 0.0;
    int count = Resources.Count();
    while (count-- > 0) {
        const PartResource& partResource = Resources.At(count);
        const float num2 = f32store(fx(static_cast<float>(partResource.amount)) * fx(partResource.info->density()));
        num = f32store(fx(num) + fx(num2));
        thermalMass += static_cast<double>(num2) * static_cast<double>(partResource.info->specificHeatCapacity());
    }
    return num;
}

// ---- mass, cost, size ------------------------------------------------------------------------------------------------------------
// Part.cs:4026
void Part::UpdateMass() {
    const size_t count = modules_.Count();
    float num = 0.0f;
    moduleMass = 0.0f;
    inventoryMass = 0.0f;
    if (fields.needPrefabMass) {
        fields.prefabMass = fields.mass;
        if (partInfo != nullptr && partInfo->partPrefab != nullptr) fields.prefabMass = partInfo->partPrefab->fields.mass;
        fields.needPrefabMass = false;
    }
    for (size_t i = 0; i < count; i++) {
        PartModule* m = modules_.GetModule(i);
        if (IPartMassModifier* mm = m->AsMassModifier()) {
            num = mm->GetModuleMass(fields.prefabMass, ModifierStagingSituation::CURRENT);
            moduleMass = f32store(fx(moduleMass) + fx(num));
            if (m->IsA("ModuleInventoryPart")) inventoryMass = f32store(fx(inventoryMass) + fx(num));
        }
    }
    fields.mass = f32store(fx(fields.prefabMass) + fx(moduleMass));
    if (partInfo == nullptr) throw NullReferenceException();
    if (fields.mass < partInfo->MinimumMass()) fields.mass = partInfo->MinimumMass();
}

// Part.cs:7518 (Part.GetModuleCosts: the sum of the IPartCostModifier modules, float accumulation)
float Part::GetModuleCosts(float defaultCost, ModifierStagingSituation sit) { return PartGetModuleCosts(modules_, defaultCost, sit); }

// Part.cs:7532
float Part::GetModuleMass(float defaultMass, ModifierStagingSituation sit) { return PartGetModuleMass(modules_, defaultMass, sit); }

// Part.cs:7546
Vector3f Part::GetModuleSize(const Vector3f& defaultSize, ModifierStagingSituation sit) {
    fields.prefabSize = defaultSize;
    fields.moduleSize = Vector3f::zero();
    for (size_t i = 0; i < modules_.Count(); i++)
        if (IPartSizeModifier* sm = modules_.GetModule(i)->AsSizeModifier()) fields.moduleSize = fields.moduleSize + sm->GetModuleSize(defaultSize, sit);
    return fields.moduleSize;
}

// ---- resource priority, stageability -----------------------------------------------------------------------------------------------
// Part.cs:7744
int Part::GetResourcePriority() {
    return ((!fields.resourcePriorityUseParentInverseStage || parent == nullptr) ? fields.inverseStage : parent->fields.inverseStage) * 10 +
           fields.resourcePriorityOffset;
}

// Part.cs:7749
void Part::ResetPri() {
    fields.resourcePriorityOffset = 0;
    int count = static_cast<int>(symmetryCounterparts.size());
    while (count-- > 0)
        if (symmetryCounterparts[static_cast<size_t>(count)] != this) symmetryCounterparts[static_cast<size_t>(count)]->fields.resourcePriorityOffset = 0;
    GameEvents::onPartPriorityChanged.Fire(this);
}

// Part.cs:7763
void Part::ChangeResourcePriority(int offset) {
    fields.resourcePriorityOffset += offset;
    int count = static_cast<int>(symmetryCounterparts.size());
    while (count-- > 0)
        if (symmetryCounterparts[static_cast<size_t>(count)] != this) symmetryCounterparts[static_cast<size_t>(count)]->fields.resourcePriorityOffset += offset;
    GameEvents::onPartPriorityChanged.Fire(this);
}

// Part.cs:7777 (the stack icon is the StageManager's: `stagingHooks`; without them `stageIconCreated_` stands for
// `stackIcon.StageIcon != null`. `stageCount` is StageManager.StageCount unless stagingHooks.stageCount answers)
void Part::UpdateStageability(bool propagate, bool iconUpdate, int stageCount) {
    if (!hasStagingIcon() || fields.stagingIconAlwaysShown) return;
    bool flag = false;
    bool flag2 = false;
    int count = static_cast<int>(modules_.Count());
    while (count-- > 0) {
        PartModule* partModule = modules_.GetModule(static_cast<size_t>(count));
        if (partModule->IsStageable()) {
            flag2 = true;
            if (partModule->StagingEnabled()) {
                flag = true;
                break;
            }
        }
    }
    bool flag3 = false;
    if (flag2) {
        if (flag) {
            flag3 = !fields.stagingOn;
            fields.stagingOn = true;
            if (flag3 && !HasStageIcon()) {
                const int sc = stagingHooks.stageCount ? stagingHooks.stageCount() : stageCount;
                if (fields.inverseStage > sc) {
                    fields.inverseStage = sc;
                    GameEvents::onPartPriorityChanged.Fire(this);
                }
                if (iconUpdate) {
                    CreateStageIcon();
                    GameEvents::StageManager::OnPartUpdateStageability.Fire(this);
                }
            }
        } else {
            flag3 = fields.stagingOn;
            fields.stagingOn = false;
            if (HasStageIcon() && iconUpdate) {
                RemoveStageIcon();
                GameEvents::StageManager::OnPartUpdateStageability.Fire(this);
            }
        }
        if (!(flag3 && propagate)) return;
        int num = static_cast<int>(symmetryCounterparts.size()) - 1;
        if (num < 0) return;
        for (int num2 = num; num2 >= 0; num2--) {
            Part* part = symmetryCounterparts[static_cast<size_t>(num2)];
            if (part != this && part->fields.inverseStage == fields.inverseStage) {
                int count2 = static_cast<int>(part->modules_.Count());
                while (count2-- > 0) part->modules_.GetModule(static_cast<size_t>(count2))->SetStaging(fields.stagingOn);
                part->UpdateStageability(false, iconUpdate, stageCount);
            }
        }
    } else if (fields.stagingOn) {
        fields.stagingOn = false;
        if (iconUpdate && HasStageIcon()) {
            RemoveStageIcon();
            GameEvents::StageManager::OnPartUpdateStageability.Fire(this);
        }
    }
}

// stackIcon.StageIcon != null
bool Part::HasStageIcon() const { return stagingHooks.hasIcon ? stagingHooks.hasIcon(*this) : stageIconCreated_; }

// ProtoStageIcon.cs:57 (`!string.IsNullOrEmpty(part.stagingIcon)`; the flight-scene filter is the StageManager's hook)
void Part::CreateStageIcon(bool alertStagingSequencer) {
    if (!fields.stagingIcon || fields.stagingIcon->empty()) return;
    if (stagingHooks.createIcon) stagingHooks.createIcon(*this, alertStagingSequencer);
    else stageIconCreated_ = true;
}

// ProtoStageIcon.cs:101
void Part::RemoveStageIcon(bool alertStagingSequencer) {
    if (stagingHooks.removeIcon) stagingHooks.removeIcon(*this, alertStagingSequencer);
    else stageIconCreated_ = false;
}

// ---- effects ----------------------------------------------------------------------------------------------------------------------
// Part.cs:6175 (EffectList.Load: the EFFECTS nodes are kept; the FX objects are Unity's)
void Part::LoadEffects(const ConfigNode& node) { effectNodes_.push_back(const_cast<ConfigNode&>(node).CreateCopy()); }

// Part.cs:6180
void Part::SaveEffects(ConfigNode& node) const {
    for (const auto& e : effectNodes_) e->CopyTo(node);
}

}  // namespace ksp
