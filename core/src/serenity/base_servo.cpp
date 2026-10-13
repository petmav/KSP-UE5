// WP-52: BaseServo.cs (see include/ksp/serenity/base_servo.hpp). Every function cites BaseServo.cs:<line> of
// decompiled/KSP-Decompiled/Assembly-CSharp/Expansions/Serenity/.
#include "ksp/serenity/base_servo.hpp"

#include <algorithm>
#include <cmath>

#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/compound_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/vessel_physics.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Expansions::Serenity {

using GE = GameEvents;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
float F(double v) { return f32store(v); }
}  // namespace

// ---------------------------------------------------------------------------------------------------------------- environment

bool ServoEnvironment::SceneIsFlight() {
    if (sceneIsFlight) return sceneIsFlight();
    return SaveEnvironment::Current().loadedScene == GameScenes::FLIGHT;
}
bool ServoEnvironment::SceneIsEditor() {
    if (sceneIsEditor) return sceneIsEditor();
    return SaveEnvironment::Current().loadedScene == GameScenes::EDITOR;
}
void ServoEnvironment::Reset() {
    scheduler = nullptr;
    sceneIsFlight = sceneIsEditor = sceneIsMissionBuilder = nullptr;
    flightDriverPause = nullptr;
    screenMessages.clear();
    cycleAllAutoStrut = nullptr;
    cycleAutoStrut = nullptr;
    cycleAllAutoStrutCalls = cycleAutoStrutCalls = 0;
    attachJointData = nullptr;
    partToVesselSpacePos = nullptr;
    editorShipParts = nullptr;
    serenityInstalled = nullptr;
    universalTime = nullptr;
    resourcePartSetContains = nullptr;
}

namespace {
bool Paused() { return ServoEnvironment::flightDriverPause && ServoEnvironment::flightDriverPause(); }  // FlightDriver.Pause
double Ut() { return ServoEnvironment::universalTime ? ServoEnvironment::universalTime() : Planetarium::GetUniversalTime(); }
bool SerenityInstalled() {
    return ServoEnvironment::serenityInstalled ? ServoEnvironment::serenityInstalled() : SaveEnvironment::Current().IsExpansionInstalled("Serenity");
}
// EditorLogic.fetch.ship.parts: the host's list, else the depth-first walk of the tree from the root
std::vector<Part*> EditorParts(Part& any) {
    if (ServoEnvironment::editorShipParts) return ServoEnvironment::editorShipParts();
    std::vector<Part*> out;
    std::function<void(Part*)> walk = [&](Part* p) {
        out.push_back(p);
        for (Part* c : p->children) walk(c);
    };
    walk(any.localRoot());
    return out;
}
}  // namespace

Vector3f ServoNode::localEulerAngles() const { return UnityNative::EulerAngles(localRotation); }

// ---------------------------------------------------------------------------------------------------------------- construction

BaseServo::BaseServo(const ModuleClassInfo& cls) : PartModule(cls) {}

BaseServo::~BaseServo() {
    if (!destroyed) OnDestroy();
}

Part* BaseServo::PartOf() const { return dynamic_cast<Part*>(part); }

bool BaseServo::PartIsFlightScene() const { return ServoEnvironment::SceneIsFlight(); }

// BaseServo.cs:277
void BaseServo::ToggleServoLockedAction(KSPActionParam&) {
    if (servoIsLocked()) {
        DisengageServoLock();
    } else if (!EngageServoLock()) {
        ServoEnvironment::PostScreenMessage("#autoLOC_8003335");
    }
}
// BaseServo.cs:290
void BaseServo::ServoEngageLockAction(KSPActionParam&) {
    if (!EngageServoLock()) ServoEnvironment::PostScreenMessage("#autoLOC_8003335");
}
// BaseServo.cs:299
void BaseServo::ServoDisgageLockAction(KSPActionParam&) { DisengageServoLock(); }
// BaseServo.cs:305
void BaseServo::ToggleMotorAction(KSPActionParam&) {
    if (servoMotorIsEngaged())
        DisengageMotor();
    else
        EngageMotor();
}
// BaseServo.cs:318
void BaseServo::MotorOnAction(KSPActionParam&) { EngageMotor(); }
// BaseServo.cs:324
void BaseServo::MotorOffAction(KSPActionParam&) { DisengageMotor(); }

// BaseServo.cs:364
double BaseServo::CalculatePower() {
    if (!ServoInitComplete()) return 0.0;
    double result = 0.0;
    double num = 0.0;
    if (resourceConsumptionMode() == ResourceConsumptionTypes::VelocityLimit)
        num = fx(currentVelocityLimit) / fx(Get<float>("referenceConsumptionVelocity"));  // double slot: no float rounding (Mono)
    else
        num = fx(transformRateOfMotion) / fx(Get<float>("referenceConsumptionVelocity"));
    if (servoMotorIsEngaged() && IsMoving()) result = (fx(Get<float>("baseResourceConsumptionRate")) * fx(motorOutput)) * num;
    return result;
}

// BaseServo.cs:392
void BaseServo::CalculateAverageRateOfMovement() {
    currentFrameVelocity = F(fx(GetFrameDisplacement()) / fx(Time::fixedDeltaTime));
    const size_t i = static_cast<size_t>(velocityReadingIndex);
    velocityReadingsSum = F(fx(velocityReadingsSum) - fx(velocityReadings.at(i)));
    velocityReadings[i] = currentFrameVelocity;
    velocityReadingsSum = F(fx(velocityReadingsSum) + fx(velocityReadings[i]));
    velocityReadingIndex = (velocityReadingIndex + 1) % velocityAverageReadings;
    transformRateOfMotion = F(fx(velocityReadingsSum) / fx(static_cast<float>(velocityAverageReadings)));
}

// BaseServo.cs:404
void BaseServo::CalculateResourceDrain() {
    if (ServoEnvironment::SceneIsEditor()) return;
    rate = CalculatePower() / fx(Get<float>("efficiency"));
    hasEnoughResources = true;
    if (std::fabs(rate) < 0.001) rate = 0.0;
    double rateMultiplier, rateMultiplier2;
    if (rate >= 0.0) {
        rateMultiplier = rate;
        rateMultiplier2 = 0.0;
    } else {
        rateMultiplier = 0.0;
        rateMultiplier2 = 0.0 - rate;
    }
    std::string motorState = Get<std::string>("motorState");
    hasEnoughResources = resHandler.UpdateModuleResourceInputs(motorState, /*useFlowMode*/ true, rateMultiplier, 0.999, /*returnOnFirstLack*/ true,
                                                               /*stringOps*/ false);
    Set<std::string>("motorState", motorState);
    resHandler.UpdateModuleResourceOutputs(rateMultiplier2);
    if (!hasEnoughResources && servoMotorIsEngaged()) {
        set_servoMotorIsEngaged(false);
        rate = 0.0;
        OnModifyServo();
    }
}

// BaseServo.cs:439 (the OnValueModified unsubscriptions go with the field objects, which the module owns)
void BaseServo::OnDestroy() {
    destroyed = true;
    GE::onVesselWasModified.Remove(this, &BaseServo::OnVesselModified);
}

// BaseServo.cs:454 (the ShipConstruct argument is not read)
void BaseServo::OnSaveShip() {
    if (ServoInitComplete()) {
        ApplyCoordsUpdate();
        if (Part* p = PartOf()) {
            p->UpdateAttachNodes();
            p->UpdateSrfAttachNode();
        }
    }
}

// BaseServo.cs:465
void BaseServo::ApplyCoordsUpdate() {
    Part* p = PartOf();
    if (p == nullptr) return;
    if (ServoEnvironment::SceneIsFlight() && !servoIsLocked() && p->vessel != nullptr) {
        RecurseCoordUpdate(p, p->vessel->rootPart);
        RecurseAttachNodeUpdate(p);
    } else if (ServoEnvironment::SceneIsEditor()) {
        std::vector<Part*> ship = EditorParts(*p);
        if (!ship.empty() && std::find(ship.begin(), ship.end(), p) != ship.end()) {
            RecurseCoordUpdate(p, ship[0]);
            RecurseAttachNodeUpdate(p);
        }
    }
}

// BaseServo.cs:479
void BaseServo::RecurseCoordUpdate(Part* p, Part* rootPart) {
    if (p == nullptr || rootPart == nullptr) return;
    p->UpdateOrgPosAndRot(rootPart);
    for (size_t i = 0; i < p->children.size(); i++) RecurseCoordUpdate(p->children[i], rootPart);
}

// BaseServo.cs:491
void BaseServo::RecurseAttachNodeUpdate(Part* p) {
    if (!ServoInitComplete() || p == nullptr) return;
    if (auto* servo = dynamic_cast<BaseServo*>(p->FindModuleOfClass("BaseServo"))) {  // Part.isBaseServo
        for (size_t i = 0; i < servo->attachNodes.size(); i++) {
            AttachNode* n = servo->attachNodes[i];
            auto t = servo->attachNodeTransforms.find(n->id);
            if (t == servo->attachNodeTransforms.end()) throw Nre();  // nodeTransform == null
            n->position = p->partTransform.InverseTransformPoint(t->second->worldPosition);
            n->orientation = p->partTransform.InverseTransformDirection(t->second->worldRotation * Vector3f::forward());
        }
    }
    for (size_t j = 0; j < p->children.size(); j++) RecurseAttachNodeUpdate(p->children[j]);
}

// BaseServo.cs:514
void BaseServo::SetLaunchPosition(float val) {
    if (ServoEnvironment::SceneIsEditor()) Set<float>("launchPosition", val);
}

// BaseServo.cs:524
void BaseServo::OnAwake() {
    consumedResources_.clear();
    for (size_t i = 0, count = resHandler.inputResources.size(); i < count; i++) {
        PartResourceLibrary* lib = PartResourceLibrary::Instance();
        consumedResources_.push_back(lib != nullptr ? lib->GetDefinition(resHandler.inputResources[i]->name) : nullptr);
    }
    axisFieldNames_.clear();
    axisFieldLimitList_.clear();
    InitAxisFieldLimits();
    velocityReadings.assign(static_cast<size_t>(velocityAverageReadings), 0.0f);
    velocityReadingIndex = 0;
    PartModule::OnAwake();
    // [KSPAction] / [KSPEvent] methods (:277-:339)
    Actions().Bind("ToggleServoLockedAction", [this](KSPActionParam& p) { ToggleServoLockedAction(p); });
    Actions().Bind("ServoEngageLockAction", [this](KSPActionParam& p) { ServoEngageLockAction(p); });
    Actions().Bind("ServoDisgageLockAction", [this](KSPActionParam& p) { ServoDisgageLockAction(p); });
    Actions().Bind("ToggleMotorAction", [this](KSPActionParam& p) { ToggleMotorAction(p); });
    Actions().Bind("MotorOnAction", [this](KSPActionParam& p) { MotorOnAction(p); });
    Actions().Bind("MotorOffAction", [this](KSPActionParam& p) { MotorOffAction(p); });
    Actions().Bind("ResetPosition", [this](KSPActionParam& p) { ResetPositionAction(p); });
    Events().Bind("ResetPosition", BaseEvent::Handler([this] { ResetPositionEvent(); }));
}

// BaseServo.cs:546
void BaseServo::OnLoad(const ConfigNode& node) {
    servoTransformRotLoaded = false;
    servoTransformPosLoaded = false;
    if (node.HasValue("servoTransformPosition")) servoTransformPosLoaded = true;
    if (node.HasValue("servoTransformRotation")) servoTransformRotLoaded = true;
    if (node.HasValue("jointParentRotation")) jointParentRotationLoaded = true;
}

// BaseServo.cs:564
void BaseServo::OnSave(ConfigNode& node) {
    Part* p = PartOf();
    const bool flightLoaded = ServoEnvironment::SceneIsFlight() && p != nullptr && p->vessel != nullptr && p->vessel->loaded;
    if (!servoInitComplete || (!flightLoaded && !ServoEnvironment::SceneIsEditor())) return;
    ApplyCoordsUpdate();
    if (servoInitComplete && movingPartObject != nullptr) {
        if (!servoIsLocked() || ServoEnvironment::SceneIsEditor()) {
            Set<Vector3f>("servoTransformPosition", movingPartObject->localPosition);
            Set<Quaternionf>("servoTransformRotation", movingPartObject->localRotation);
        }
        node.SetValue("servoTransformPosition", Get<Vector3f>("servoTransformPosition"));
        node.SetValue("servoTransformRotation", Get<Quaternionf>("servoTransformRotation"));
    }
    if (jointParent != nullptr) node.SetValue("jointParentRotation", jointParent->localRotation);
}

// BaseServo.cs:587
void BaseServo::OnStart(StartState) {
    OutputUnit = KSP::Localization::Localizer::Format("#autoLOC_6013039");
    if (!ServoEnvironment::SceneIsFlight() && !ServoEnvironment::SceneIsEditor()) return;
    Part* p = PartOf();
    if (p == nullptr) throw Nre();
    prevServoMotorIsEngaged = !servoMotorIsEngaged();
    prevServoIsMotorized = !servoIsMotorized();
    const std::string servoTransformName = Get<std::string>("servoTransformName");
    if (!servoTransformName.empty()) {
        movingPartObject = rig.Find(servoTransformName);
        if (movingPartObject != nullptr) {
            if (!servoTransformPosLoaded) Set<Vector3f>("servoTransformPosition", movingPartObject->localPosition);
            if (!servoTransformRotLoaded) Set<Quaternionf>("servoTransformRotation", movingPartObject->localRotation);
        }
    }
    const std::string baseTransformName = Get<std::string>("baseTransformName");
    if (!baseTransformName.empty()) basePartObject = rig.Find(baseTransformName);
    const std::string jointParentName = Get<std::string>("jointParentName");
    if (!jointParentName.empty()) jointParent = rig.Find(jointParentName);  // part.FindModelTransform
    if (jointParent == nullptr) {
        if (movingPartObject == nullptr) throw Nre();  // movingPartObject.transform.parent
        jointParent = movingPartObject->parent;
    }
    if (jointParent != nullptr) {
        if (jointParentRotationLoaded) jointParent->localRotation = Get<Quaternionf>("jointParentRotation");
        servoParentTransform = jointParent->localToWorld;
    } else {
        throw Nre();  // movingPartObject.transform.parent.localToWorldMatrix with no parent
    }
    servoParentTransform = MatrixMultiply(rig.partWorldToLocal, servoParentTransform);  // :635
    servoParentTransformInverse = MatrixInverse(servoParentTransform);                 // :636
    motorOutput = F(fx(servoMotorSize()) * fx(0.01f) * fx(maxMotorOutput()));          // :637
    if (ServoEnvironment::SceneIsFlight()) motorOutput = F(fx(motorOutput) * (fx(servoMotorLimit()) * fx(0.01f)));  // :640
    // BaseServo.cs:642-:661 the servo's attach nodes and their transforms
    const std::string servoAttachNodes = Get<std::string>("servoAttachNodes");
    if (!servoAttachNodes.empty()) {
        size_t begin = 0;
        while (true) {
            const size_t comma = servoAttachNodes.find(',', begin);
            const std::string id = servoAttachNodes.substr(begin, comma == std::string::npos ? std::string::npos : comma - begin);
            AttachNode* node = p->FindAttachNode(id);
            if (node != nullptr && std::find(attachNodes.begin(), attachNodes.end(), node) == attachNodes.end()) {
                attachNodes.push_back(node);  // AddUnique
                auto go = std::make_shared<ServoNode>();
                go->name = "AttachNode_" + node->id;
                go->worldPosition = p->partTransform.TransformPoint(node->originalPosition);
                // transform.forward = TransformDirection(originalOrientation): rotation = LookRotation(forward) (native, up = Vector3.up)
                go->worldRotation = UnityNative::LookRotation(p->partTransform.TransformDirection(node->originalOrientation), Vector3f::up());
                attachNodeTransforms[node->id] = go;
            }
            if (comma == std::string::npos) break;
            begin = comma + 1;
        }
    }
    const std::string servoSrfMeshNames = Get<std::string>("servoSrfMeshNames");
    if (!servoSrfMeshNames.empty()) {
        servoSrfMeshes.clear();
        servoSrfMeshesSet_ = true;
        size_t begin = 0;
        while (true) {
            const size_t comma = servoSrfMeshNames.find(',', begin);
            servoSrfMeshes.push_back(servoSrfMeshNames.substr(begin, comma == std::string::npos ? std::string::npos : comma - begin));
            if (comma == std::string::npos) break;
            begin = comma + 1;
        }
    }
    if (movingPartObject != nullptr) {
        for (size_t j = 0; j < attachNodes.size(); j++) {
            auto t = attachNodeTransforms.find(attachNodes[j]->id);
            if (t == attachNodeTransforms.end()) throw Nre();
            if (t->second->parent == nullptr) {
                t->second->parent = movingPartObject;  // SetParent
                t->second->localScale = Vector3f::one();
            }
        }
    }
    GE::onVesselWasModified.Add(this, &BaseServo::OnVesselModified);
    // onAboutToSaveShip / onPartActionUIShown / Dismiss, onEditorPartPlaced / Picked / CompoundPartLinked: the host forwards them to OnSaveShip(),
    // OnPartMenuOpen / Close, OnEditorPartPlaced / Picked / CompoundPartLinked (their payload types are UI / editor objects)
    if (ServoEnvironment::SceneIsEditor()) {
        upstreamParts.clear();
        downstreamParts.clear();
        spanningParts.clear();
        BuildPartSets();
    }
    InitJoint();
    BaseField* engaged = Fields()["servoMotorIsEngaged"];
    if (ServoEnvironment::SceneIsFlight() && !servoIsMotorized() && engaged != nullptr) {
        engaged->guiActive = false;
        set_servoMotorIsEngaged(false);
        prevServoMotorIsEngaged = true;
    }
    if (engaged != nullptr) {
        const ModuleValue& ov = engaged->originalValue();
        if (const bool* b = std::get_if<bool>(&ov)) {
            if (!*b) motorManualDisengaged = true;
        }
    }
    if (BaseField* f = Fields()["servoMotorSize"]) {
        if (ServoEnvironment::SceneIsFlight())
            f->guiActive = false;
        else
            f->guiActiveEditor = !Get<bool>("hideUIServoMotorSize");
    }
    if (BaseField* f = Fields()["servoMotorLimit"]) {
        if (ServoEnvironment::SceneIsEditor())
            f->guiActive = false;
        else
            f->guiActive = !Get<bool>("hideUIServoMotorLimit");
    }
    if (ServoEnvironment::SceneIsFlight()) UpdatePAWUI(UI_Scene::Flight);
    Fields()["servoIsMotorized"]->guiActiveEditor = ServoEnvironment::SceneIsEditor() && !Get<bool>("hideUIServoIsMotorized");
    Fields()["servoMotorSize"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyServo(&v); });
    Fields()["servoMotorLimit"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyServo(&v); });
    Fields()["servoMotorIsEngaged"]->OnValueModified.push_back([this](const ModuleValue& v) { ManuallyModifyEngaged(&v); });
    Fields()["servoIsLocked"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyLocked(&v); });
    if (ServoEnvironment::SceneIsEditor() && ServoEnvironment::scheduler != nullptr) {
        // StartCoroutine(WaitAndVisualizeServo()): `yield return null`, then servoInitComplete = true; SetDriveUnit(); VisualizeServo(false)
        visualizeCoroutine_ = ServoEnvironment::scheduler->StartCoroutine(WaitAndVisualizeServo());
    }
    CalculateResourceDrain();
}

// BaseServo.cs:741
void BaseServo::RecurseBuildPartSets(Part* p, std::set<uint32_t>* partSet) {
    if (p == PartOf()) {
        partSet = &downstreamParts;
    } else {
        if (dynamic_cast<CompoundPart*>(p) != nullptr) return;
        partSet->insert(p->fields.craftID);
    }
    size_t count = p->children.size();
    while (count-- > 0) RecurseBuildPartSets(p->children[count], partSet);
}

// BaseServo.cs:762
void BaseServo::RecurseRemovePartSets(Part* p, std::set<uint32_t>* partSet) {
    if (p == PartOf()) {
        partSet = &downstreamParts;
    } else {
        if (dynamic_cast<CompoundPart*>(p) != nullptr) return;
        partSet->erase(p->fields.craftID);
    }
    size_t count = p->children.size();
    while (count-- > 0) RecurseRemovePartSets(p->children[count], partSet);
}

// BaseServo.cs:783
void BaseServo::BuildPartSets() {
    upstreamParts.clear();
    downstreamParts.clear();
    spanningParts.clear();
    RecurseBuildPartSets(PartOf()->localRoot(), &upstreamParts);
}

// BaseServo.cs:791 WaitAndVisualizeServo
Enumerator BaseServo::WaitAndVisualizeServo() {
    co_yield YieldInstruction::Null();
    servoInitComplete = true;
    SetDriveUnit();
    VisualizeServo(false);
}

// BaseServo.cs:799
void BaseServo::OnStartBeforePartAttachJoint(StartState modStartState) {
    if (ServoEnvironment::SceneIsFlight() || ServoEnvironment::SceneIsEditor()) {
        if (movingPartRB != nullptr && movingPartRB->isKinematic) {
            movingPartRB->isKinematic = false;
            partIsPhysicsLess = false;
        }
        if (modStartState != StartState::Editor) PostStartInitJoint();
        SetInitialDisplacement();
        servoInitComplete = true;
        ApplyCoordsUpdate();
    }
}

// BaseServo.cs:818
void BaseServo::PromoteToPhysicalPart() {
    if (servoJoint == nullptr) InitJoint();
}

// BaseServo.cs:826
void BaseServo::DemoteToPhysicslessPart() {
    if (movingPartRB != nullptr) movingPartRB->isKinematic = true;
    partIsPhysicsLess = true;
}

// BaseServo.cs:850
void BaseServo::OnCopy(PartModule&) {
    Part* p = PartOf();
    for (size_t i = 0; i < attachNodes.size(); i++) {
        const std::string id = attachNodes[i]->id;
        AttachNode* attachNode = p != nullptr ? p->FindAttachNode(id) : nullptr;
        if (attachNode != nullptr) attachNodes[i] = attachNode;  // the nodeTransform is kept (attachNodeTransforms is keyed by id)
    }
    RecurseAttachNodeUpdate(p);
}

// BaseServo.cs:865
void BaseServo::Start() {
    if (!SerenityInstalled() && SaveEnvironment::Current().loadedSceneIsGame) {
        behaviourEnabled = false;
        destroyed = true;  // Object.Destroy(this)
    }
}

// BaseServo.cs:873
void BaseServo::FixedUpdate() {
    if (!ServoInitComplete() || partIsPhysicsLess) return;
    if (ServoEnvironment::SceneIsFlight() && !Paused()) {
        CalculateAverageRateOfMovement();
        if (servoIsMotorized()) {
            if (servoMotorIsEngaged() && !servoIsBraking && !servoIsLocked()) {
                CalculateResourceDrain();
            } else if (!servoMotorIsEngaged() && resHandler.HasEnoughResourcesToAutoStart() && !motorManualDisengaged) {
                hasEnoughResources = true;
                if (lockPartOnPowerLoss() && servoIsLocked()) DisengageServoLock();
                if (servoIsBraking) servoIsBraking = false;
                EngageMotor();
                motorManualDisengaged = false;
            }
        }
    }
    ServoFixedUpdate();
    if (ServoEnvironment::SceneIsFlight() && !Paused()) RecurseAttachNodeUpdate(PartOf());
}

// BaseServo.cs:911
void BaseServo::Update() {
    if (partIsPhysicsLess) return;
    if (partActionMenuOpen) {
        UpdatePAWUI(ServoEnvironment::SceneIsEditor() ? UI_Scene::Editor : UI_Scene::Flight);
    } else {
        prevServoIsLocked = servoIsLocked();
    }
    if (!ServoEnvironment::SceneIsFlight() || (!IsMoving() && !wasMoving)) return;
    ApplyCoordsUpdate();
    wasMoving = true;
    if (!IsMoving()) {
        if (stoppedMovingTimerOn) {
            if (Ut() - timeStoppedMoving > 3.0) {
                stoppedMovingTimerOn = false;
                wasMoving = false;
            }
        } else {
            stoppedMovingTimerOn = true;
            timeStoppedMoving = Ut();
        }
    } else {
        stoppedMovingTimerOn = false;
    }
}

// BaseServo.cs:960
bool BaseServo::EngageServoLock() {
    if (servoIsLocked()) return true;
    if (!servoIsLocked() && !IsMoving()) {
        Fields()["servoIsLocked"]->SetValue(ModuleValue(true));
        ModifyLocked(nullptr);
        return true;
    }
    return false;
}

// BaseServo.cs:975
void BaseServo::DisengageServoLock() {
    if (servoIsLocked()) {
        Fields()["servoIsLocked"]->SetValue(ModuleValue(false));
        ModifyLocked(nullptr);
    }
}

// BaseServo.cs:984
void BaseServo::EngageMotor() {
    if (!servoMotorIsEngaged()) {
        Fields()["servoMotorIsEngaged"]->SetValue(ModuleValue(true));
        ModifyServo(nullptr);
    }
}

// BaseServo.cs:993
void BaseServo::DisengageMotor() {
    if (servoMotorIsEngaged()) {
        Fields()["servoMotorIsEngaged"]->SetValue(ModuleValue(false));
        ModifyServo(nullptr);
    }
}

// BaseServo.cs:1002
void BaseServo::OnPartMenuOpen(Part* inpPart) {
    Part* p = PartOf();
    if (inpPart->fields.persistentId == p->fields.persistentId) {
        partActionMenuOpen = true;
        UpdatePAWUI(ServoEnvironment::SceneIsEditor() ? UI_Scene::Editor : UI_Scene::Flight);
    }
}

// BaseServo.cs:1018
void BaseServo::OnPartMenuClose(Part* inpPart) {
    if (inpPart->fields.persistentId == PartOf()->fields.persistentId) partActionMenuOpen = false;
}

// BaseServo.cs:1026
void BaseServo::OnEditorPartPlaced(Part* placedPart) {
    if (placedPart == nullptr || placedPart->parent == nullptr) return;
    if (placedPart == PartOf()) {
        RecurseBuildPartSets(placedPart->localRoot(), &upstreamParts);
    } else if (dynamic_cast<CompoundPart*>(placedPart) == nullptr) {
        const uint32_t craftID = placedPart->parent->fields.craftID;
        if (upstreamParts.count(craftID) != 0)
            RecurseBuildPartSets(placedPart, &upstreamParts);
        else
            RecurseBuildPartSets(placedPart, &downstreamParts);
    }
}

// BaseServo.cs:1050
void BaseServo::OnEditorPartPicked(Part* pickedPart) {
    if (pickedPart == PartOf()) {
        upstreamParts.clear();
    } else if (auto* cp = dynamic_cast<CompoundPart*>(pickedPart)) {
        spanningParts.erase(std::remove(spanningParts.begin(), spanningParts.end(), cp), spanningParts.end());
    } else if (upstreamParts.count(pickedPart->fields.craftID) != 0) {
        RecurseRemovePartSets(pickedPart, &upstreamParts);
    } else {
        RecurseRemovePartSets(pickedPart, &downstreamParts);
    }
}

// BaseServo.cs:1070
void BaseServo::OnEditorCompoundPartLinked(CompoundPart* linkedPart) {
    if (linkedPart == nullptr) return;
    if (linkedPart->target != nullptr && linkedPart->parent != nullptr) {
        const uint32_t craftID = linkedPart->parent->fields.craftID;
        const uint32_t craftID2 = linkedPart->target->fields.craftID;
        Part* self = PartOf();
        if (linkedPart->parent == self || linkedPart->target == self || (upstreamParts.count(craftID) != 0 && downstreamParts.count(craftID2) != 0) ||
            (upstreamParts.count(craftID2) != 0 && downstreamParts.count(craftID) != 0)) {
            if (std::find(spanningParts.begin(), spanningParts.end(), linkedPart) == spanningParts.end()) spanningParts.push_back(linkedPart);  // AddUnique
        }
    } else {
        spanningParts.erase(std::remove(spanningParts.begin(), spanningParts.end(), linkedPart), spanningParts.end());
    }
}

// BaseServo.cs:1091
void BaseServo::InitJoint() {
    Part* p = PartOf();
    std::shared_ptr<ConfigurableJointData> attachJoint;
    if (p != nullptr && ServoEnvironment::attachJointData) attachJoint = ServoEnvironment::attachJointData(*p);
    hasPJoint = attachJoint != nullptr;
    if (servoJoint == nullptr && attachJoint != nullptr) {
        servoJoint = attachJoint;
        axis = servoJoint->axis;
        secAxis = servoJoint->secondaryAxis;
    }
    if (hasPJoint) pivot = ServoEnvironment::partToVesselSpacePos ? ServoEnvironment::partToVesselSpacePos(*p, Vector3f::zero()) : Vector3f::zero();
    OnJointInit(true);
    if (movingPartRB != nullptr) {
        // AddComponent<RoboticCollisions>() with .part = part: the collision forwarder lives on the moving object (serenity_misc.hpp)
        roboticCollisions_ = std::make_shared<RoboticCollisions>();
        roboticCollisions_->part = p;
    }
    if (ServoEnvironment::SceneIsEditor()) VisualizeServo(false);
}

// BaseServo.cs:1132
void BaseServo::PostStartInitJoint() {
    Part* p = PartOf();
    std::shared_ptr<ConfigurableJointData> attachJoint;
    if (!hasPJoint && p != nullptr && ServoEnvironment::attachJointData) attachJoint = ServoEnvironment::attachJointData(*p);
    if (attachJoint != nullptr) hasPJoint = true;
    if (servoJoint == nullptr && attachJoint != nullptr) {
        servoJoint = attachJoint;
        axis = servoJoint->axis;
        secAxis = servoJoint->secondaryAxis;
    }
    if (servoJoint == nullptr) throw Nre();                // servoJoint.connectedBody = ...
    servoJoint->hasConnectedBody = true;                    // connectedBody = part.Rigidbody
    servoJoint->connectedMassScale = Get<float>("connectedMassScale");
    if (movingPartObject != nullptr) {
        if (servoTransformPosLoaded) movingPartObject->localPosition = Get<Vector3f>("servoTransformPosition");
        if (servoTransformRotLoaded) movingPartObject->localRotation = Get<Quaternionf>("servoTransformRotation");
        targetRotation = movingPartObject->localRotation;
        cachedStartingRotation = Quaternionf::identity();
        cachedStartingRotationOffset = 0.0f;
    }
    OnPostStartJointInit();
}

// BaseServo.cs:1187
void BaseServo::SetServoMass() {
    if (movingPartRB != nullptr) {
        movingPartRB->mass = Get<float>("servoMass");
        movingPartRB->centerOfMass = Get<Vector3f>("servoCoMOffset");
    }
}

// BaseServo.cs:1196
void BaseServo::CreateServoRigidbody() {
    if (movingPartObject != nullptr) {
        if (movingPartRB == nullptr) movingPartRB = std::make_shared<ServoBody>();  // GetComponent<Rigidbody>() ?? AddComponent
        Part* p = PartOf();  // part.servoRb = movingPartRB (the Part field is the engine's Rigidbody handle: `movingPartRB` here)
        movingPartRB->isKinematic = !ServoEnvironment::SceneIsFlight() || (p != nullptr && p->fields.packed);
        movingPartRB->useGravity = false;
        movingPartRB->maxAngularVelocity = PhysicsGlobals::MaxAngularVelocity();
        SetServoMass();
    }
}

// BaseServo.cs:1213
void BaseServo::ModifyServo(const ModuleValue*) {
    OnPreModifyServo();
    SetDriveUnit();
    motorOutput = driveUnit;
    if (ServoEnvironment::SceneIsFlight()) motorOutput = F(fx(motorOutput) * (fx(servoMotorLimit()) * fx(0.01f)));
    if (ServoEnvironment::SceneIsEditor())
        VisualizeServo(true);
    else if (servoJoint != nullptr)
        OnModifyServo();
}

// BaseServo.cs:1232
void BaseServo::SetDriveUnit() { driveUnit = F(fx(servoMotorSize()) * fx(0.01f) * fx(maxMotorOutput())); }

// BaseServo.cs:1237
void BaseServo::ModifyLocked(const ModuleValue* field) {
    bool flag = false;
    Part* p = PartOf();
    if (servoIsLocked() && !prevServoIsLocked) {
        GE::onRoboticPartLockChanging.Fire(p, servoIsLocked());
        flag = true;
        if (movingPartObject == nullptr) throw Nre();
        Set<Vector3f>("servoTransformPosition", movingPartObject->localPosition);
        Set<Quaternionf>("servoTransformRotation", movingPartObject->localRotation);
        if (ServoEnvironment::SceneIsFlight() || ServoEnvironment::SceneIsEditor()) {
            if (ServoEnvironment::SceneIsFlight())
                RecurseCoordUpdate(p, p->vessel->rootPart);
            else
                RecurseCoordUpdate(p, EditorParts(*p).at(0));
        }
        RecurseAttachNodeUpdate(p);
        OnServoLockApplied();
    } else if (!servoIsLocked() && prevServoIsLocked) {
        GE::onRoboticPartLockChanging.Fire(p, servoIsLocked());
        flag = true;
        OnServoLockRemoved();
    }
    if (ServoEnvironment::SceneIsFlight()) {
        if (p->vessel != nullptr) {
            if (ServoEnvironment::cycleAllAutoStrut)
                ServoEnvironment::cycleAllAutoStrut(*p->vessel);
            else
                ServoEnvironment::cycleAllAutoStrutCalls++;
        }
        ModifyServo(field);
    } else if (ServoEnvironment::SceneIsEditor()) {
        for (Part* q : EditorParts(*p)) {
            if (ServoEnvironment::cycleAutoStrut)
                ServoEnvironment::cycleAutoStrut(*q);
            else
                ServoEnvironment::cycleAutoStrutCalls++;
        }
    }
    if (flag) GE::onRoboticPartLockChanged.Fire(p, servoIsLocked());
}

// BaseServo.cs:1280
void BaseServo::ModifyEngaged(const ModuleValue* field) {
    if (servoMotorIsEngaged() && !prevServoMotorIsEngaged)
        CalculateResourceDrain();
    else if (!servoMotorIsEngaged() && prevServoMotorIsEngaged)
        OnServoMotorDisengaged();
    ModifyServo(field);
}

// BaseServo.cs:1293
void BaseServo::ManuallyModifyEngaged(const ModuleValue* field) {
    if (servoMotorIsEngaged() && !prevServoMotorIsEngaged) {
        OnServoMotorEngaged();
        CalculateResourceDrain();
    } else if (!servoMotorIsEngaged() && prevServoMotorIsEngaged) {
        OnServoMotorDisengaged();
    }
    ModifyServo(field);
}

// BaseServo.cs:1307
void BaseServo::OnVesselModified(Vessel* v) {
    Part* p = PartOf();
    if (p != nullptr && p->State() != PartStates::DEAD) {
        if (v == p->vessel) ApplyCoordsUpdate();
    } else {
        log::Info("[BaseServo]: host part is null, servoJoint must terminate.");
    }
}

// BaseServo.cs:1322
void BaseServo::SaveSpanningPartTargets() {
    size_t count = spanningParts.size();
    while (count-- > 0) {
        if (spanningParts[count]->target == nullptr)
            spanningParts.erase(spanningParts.begin() + static_cast<std::ptrdiff_t>(count));
        else
            spanningParts[count]->UpdateWorldValues();
    }
}

// BaseServo.cs:1342
void BaseServo::RestoreSpanningPartTargets() {
    size_t count = spanningParts.size();
    while (count-- > 0) spanningParts[count]->UpdateTargetCoords();
}

// BaseServo.cs:1354 (SetChildParentTransform, the editor's GameObject re-parenting of the children, is the host's scene graph)
void BaseServo::VisualizeServo(bool /*moveChildren*/) {
    const std::string servoTransformName = Get<std::string>("servoTransformName");
    const std::string baseTransformName = Get<std::string>("baseTransformName");
    if (movingPartObject == nullptr && !servoTransformName.empty()) movingPartObject = rig.Find(servoTransformName);
    if (basePartObject == nullptr && !baseTransformName.empty()) basePartObject = rig.Find(baseTransformName);
    if (movingPartObject == nullptr) return;
    Part* p = PartOf();
    bool rotateBase = false;
    if (basePartObject != nullptr) {
        for (size_t i = 0; i < attachNodes.size(); i++) {
            if (attachNodes[i]->attachedPart != nullptr && attachNodes[i]->attachedPart == p->parent) {
                rotateBase = true;
                break;
            }
        }
    }
    SaveSpanningPartTargets();
    OnVisualizeServo(rotateBase);
    RestoreSpanningPartTargets();
    RecurseAttachNodeUpdate(p);
}

// BaseServo.cs:1460
Vector3f BaseServo::GetMainAxis() const {
    const std::string a = mainAxis();
    if (a == "Z-") return Vector3f::back();
    if (a == "Y") return Vector3f::up();
    if (a == "Y-") return Vector3f::down();
    if (a == "X") return Vector3f::right();
    if (a == "X-") return Vector3f::left();
    return Vector3f::forward();
}

// BaseServo.cs:1524
float BaseServo::StartingRotationOffset() {
    float num = 0.0f;
    const Vector3f e = UnityNative::EulerAngles(cachedStartingRotation);
    const std::string a = mainAxis();
    if (a == "Y")
        num = e.y;
    else if (a == "X")
        num = e.x;
    else
        num = e.z;
    if (num > 180.0f) num = F(360.0 - fx(num));
    return num;
}

// BaseServo.cs:1546
float BaseServo::currentTransformAngle() {
    if (angleSource) return angleSource();
    float num = 0.0f;
    if (movingPartObject == nullptr) return num;
    const std::string a = mainAxis();
    const Vector3f e = movingPartObject->localEulerAngles();
    if (a == "Z")
        num = e.z;
    else if (a == "Y")
        num = e.y;
    else if (a == "X")
        num = e.x;
    if (num > 180.0f) num = F(fx(num) - 360.0);
    return num;
}

// BaseServo.cs:1572
float BaseServo::currentTransformPosition() {
    if (positionSource) return positionSource();
    float result = 0.0f;
    if (movingPartObject == nullptr) throw Nre();
    const std::string a = mainAxis();
    if (a == "Z")
        result = movingPartObject->localPosition.z;
    else if (a == "Y")
        result = movingPartObject->localPosition.y;
    else if (a == "X")
        result = movingPartObject->localPosition.x;
    return result;
}

// BaseServo.cs:1590 (the UI_FloatRange half of the C# is UI)
float BaseServo::UpdateFieldLimits(BaseAxisField& axisField, const Vector2f& newlimits, float currentValue) {
    axisField.minValue = newlimits.x;
    axisField.maxValue = newlimits.y;
    return Mathf::Clamp(currentValue, newlimits.x, newlimits.y);
}

// BaseServo.cs:1602
BaseServo::BodyOwner BaseServo::AttachServoRigidBody(const AttachNode& node) {
    if (movingPartRB != nullptr) {
        if (node.nodeType == AttachNodeType::Surface && !node.srfAttachMeshName.empty()) {
            if (!servoSrfMeshesSet_) throw Nre();
            for (size_t i = 0; i < servoSrfMeshes.size(); i++)
                if (servoSrfMeshes[i] == node.srfAttachMeshName) return BodyOwner::Servo;
        }
        for (size_t j = 0; j < attachNodes.size(); j++)
            if (attachNodes[j]->id == node.id) return BodyOwner::Servo;
    }
    return BodyOwner::Part;
}

// BaseServo.cs:1627
BaseServo::BodyOwner BaseServo::NodeRigidBody(const AttachNode& node) {
    if (movingPartRB != nullptr) {
        for (size_t i = 0; i < attachNodes.size(); i++)
            if (attachNodes[i] == &node) return BodyOwner::Servo;
    }
    return BodyOwner::Part;
}

// BaseServo.cs:1642
bool BaseServo::ServoTransformCollider(const std::string& colName) {
    if (!servoSrfMeshesSet_) throw Nre();
    for (size_t i = 0; i < servoSrfMeshes.size(); i++)
        if (servoSrfMeshes[i] == colName) return true;
    return false;
}

// BaseServo.cs:1480 (state side: the text and the field flags; the UI_* controls' scene visibility is not ported)
void BaseServo::UpdatePAWUI(UI_Scene) {
    using KSP::Localization::Localizer;
    Set<std::string>("motorState", (!servoIsMotorized() || !servoMotorIsEngaged()) ? Localizer::Format("#autoLOC_8002352") : Localizer::Format("#autoLOC_8002351"));
    if (ServoEnvironment::SceneIsEditor()) {
        Part& p = *PartOf();
        Fields()["motorOutputInformation"]->guiActiveEditor = servoIsMotorized() && GameSettings::ADVANCED_TWEAKABLES;
        Set<std::string>("motorOutputInformation",
                         Localizer::Format("#autoLOC_8002342", {dotnet::FormatSingleF(motorOutput, 1), dotnet::FormatSingleF(F(fx(p.fields.mass) - fx(p.fields.prefabMass)), 2)}));
        Set<std::string>("servoCurrentTorque", Localizer::Format("#autoLOC_8002350"));
        Fields()["servoIsMotorized"]->guiActiveEditor = !Get<bool>("hideUIServoIsMotorized");
        Fields()["servoMotorIsEngaged"]->guiActiveEditor = servoIsMotorized();
    }
    if (ServoEnvironment::SceneIsFlight()) {
        Fields()["servoIsLocked"]->guiInteractable = (servoIsLocked() || !IsMoving()) && GameSettings::ADVANCED_TWEAKABLES;
        Fields()["resourceConsumption"]->guiActive = servoMotorIsEngaged() && GameSettings::ADVANCED_TWEAKABLES;
        std::string inputs;
        for (size_t i = 0; i < resHandler.inputResources.size(); i++) {
            const double amount = resHandler.inputResources[i]->currentRequest / fx(TimeWarp::fixedDeltaTime());
            inputs += "\n    " + Localizer::Format(resHandler.inputResources[i]->title) + " " + KSPUtil::PrintSI(amount, Localizer::Format("#autoLOC_6013040"));
        }
        const std::string text = KSPUtil::PrintSI(fx(F(fx(motorOutput) * 1000.0)), OutputUnit);
        Set<std::string>("resourceConsumption", Localizer::Format("#autoLOC_8005432", {text, inputs}));
    }
}

// BaseServo.cs:1671
float BaseServo::GetModuleCost(float, ModifierStagingSituation) {
    if (servoIsMotorized()) {
        SetDriveUnit();
        return F(fx(Get<float>("motorizedCostPerDriveUnit")) * fx(driveUnit));
    }
    return 0.0f;
}

// BaseServo.cs:1686
float BaseServo::GetModuleMass(float, ModifierStagingSituation) {
    float result = 0.0f;
    if (servoIsMotorized()) result = F(fx(Get<float>("motorizedMassPerKN")) * fx(driveUnit));
    return result;
}

// BaseServo.cs:1703
void BaseServo::UpdateAxisFieldLimit(const std::string& fieldName, const Vector2f& hardLimits, const Vector2f& softLimits) {
    if (AxisFieldLimit* l = GetAxisFieldLimit(fieldName)) {
        l->hardLimits = hardLimits;
        l->softLimits = softLimits;
    }
}

// axisFieldLimits.Add(name, limit)
void BaseServo::AddAxisFieldLimit(const std::string& name, const AxisFieldLimit& limit) {
    if (std::find(axisFieldNames_.begin(), axisFieldNames_.end(), name) != axisFieldNames_.end()) throw std::invalid_argument("An item with the same key has already been added.");
    axisFieldNames_.push_back(name);
    axisFieldLimitList_.push_back(limit);
}

// BaseServo.cs:1716
void BaseServo::SetHardLimits(const std::string& fieldName, const Vector2f& newLimits) { UpdateAxisFieldHardLimit(fieldName, newLimits); }

// BaseServo.cs:1721
void BaseServo::SetSoftLimits(const std::string& fieldName, const Vector2f& newLimits) {
    if (HasAxisFieldLimit(fieldName)) {
        Vector2f nl = newLimits;
        const Vector2f hardLimits = GetHardLimits(fieldName);
        nl.x = Mathf::Max(nl.x, hardLimits.x);
        nl.y = Mathf::Min(nl.y, hardLimits.y);
        UpdateAxisFieldSoftLimit(fieldName, nl);
    }
}

// BaseServo.cs:1732
bool BaseServo::HasAxisFieldLimits() { return !axisFieldLimitList_.empty(); }

// BaseServo.cs:1741
bool BaseServo::HasAxisFieldLimit(const std::string& fieldName) {
    return !axisFieldLimitList_.empty() && std::find(axisFieldNames_.begin(), axisFieldNames_.end(), fieldName) != axisFieldNames_.end();
}

// BaseServo.cs:1759
AxisFieldLimit* BaseServo::GetAxisFieldLimit(const std::string& fieldName) {
    auto it = std::find(axisFieldNames_.begin(), axisFieldNames_.end(), fieldName);
    if (it == axisFieldNames_.end()) return nullptr;
    return &axisFieldLimitList_[static_cast<size_t>(it - axisFieldNames_.begin())];
}

// BaseServo.cs:1772
Vector2f BaseServo::GetHardLimits(const std::string& fieldName) {
    if (AxisFieldLimit* l = GetAxisFieldLimit(fieldName)) return l->hardLimits;
    return Vector2f(1.0f, 1.0f);  // Vector2.one
}

// BaseServo.cs:1781
Vector2f BaseServo::GetSoftLimits(const std::string& fieldName) {
    if (AxisFieldLimit* l = GetAxisFieldLimit(fieldName)) return l->softLimits;
    return Vector2f(1.0f, 1.0f);
}

// PartJoint.SetupJoint (PartJoint.cs:213-258) as the reduced model sees it: the joint between a robotic part and its neighbour is an articulation
// point when the node it uses belongs to the servo's moving side (AttachServoRigidBody / NodeRigidBody return the servo body).
bool ServoArticulation(const physics::PartJointSpec& j) {
    if (j.kind != physics::JointKind::Attach || j.child == nullptr || j.parent == nullptr) return false;
    Part* child = j.child;
    Part* parent = j.parent;
    auto nodeOn = [](Part& on, Part* other) -> AttachNode* {  // Part.AttachedPartNode: the node of `on` attached to `other`
        for (size_t i = 0; i < on.attachNodes.size(); i++)
            if (on.attachNodes[i].attachedPart != nullptr && on.attachNodes[i].attachedPart->fields.persistentId == other->fields.persistentId) return &on.attachNodes[i];
        if (on.srfAttachNode && on.srfAttachNode->attachedPart != nullptr && on.srfAttachNode->attachedPart->fields.persistentId == other->fields.persistentId)
            return on.srfAttachNode.get();
        if (other->srfAttachNode && other->srfAttachNode->attachedPart != nullptr && other->srfAttachNode->attachedPart->fields.persistentId == on.fields.persistentId)
            return other->srfAttachNode.get();
        return nullptr;
    };
    AttachNode* nodeToParent = nodeOn(*child, parent);
    AttachNode* nodeFromParent = nodeOn(*parent, child);
    if (auto* s2 = dynamic_cast<BaseServo*>(child->FindModuleOfClass("BaseServo"))) {
        AttachNode* n = nodeToParent != nullptr ? nodeToParent : nodeFromParent;
        if (n != nullptr && s2->AttachServoRigidBody(*n) == BaseServo::BodyOwner::Servo) return true;
    }
    if (auto* s = dynamic_cast<BaseServo*>(parent->FindModuleOfClass("BaseServo"))) {
        AttachNode* n = nodeFromParent != nullptr ? nodeFromParent : nodeToParent;
        if (n != nullptr && s->AttachServoRigidBody(*n) == BaseServo::BodyOwner::Servo) return true;
    }
    return false;
}

}  // namespace ksp::Expansions::Serenity
