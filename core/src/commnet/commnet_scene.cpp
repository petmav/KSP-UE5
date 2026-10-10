// WP-39: CommNetScenario.cs, CommNetNetwork.cs, CommNetHome.cs, CommNetBody.cs, CommNetNode.cs and the CommNet part of GameVariables.cs
// (see ksp/commnet/commnet_scene.hpp).
#include "ksp/commnet/commnet_scene.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/commnet/comm_net_vessel.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::CommNet {

namespace {
Game* CurrentGame() { return SaveEnvironment::Current().currentGame; }  // HighLogic.CurrentGame
CommNetParams* Params() {
    Game* g = CurrentGame();
    if (g == nullptr || !g->Parameters) throw NullReferenceException();
    return &g->Parameters->CustomParams<CommNetParams>();
}
}  // namespace

void CommNetEnvironment::Reset() {
    trackingStationLevel = nullptr;
    bodyTransform = nullptr;
    timeSinceLevelLoad = nullptr;
    debugLog = nullptr;
}

// ---- GameVariables.cs --------------------------------------------------------------------------------------------------------

// GameVariables.cs:537
DoubleCurve DSN::GetDSNRangeCurve() {
    DoubleCurve doubleCurve;
    doubleCurve.Add(0.0, 0.0, 0.0, 0.0);
    doubleCurve.Add(1.0, 1.0, 0.0, 0.0);
    return doubleCurve;
}

// GameVariables.cs:562
double DSN::GetDSNRange(float level) {
    double num = 0.0;
    num = (((double)level > 0.500000001) ? 250000000000.0 : ((!((double)level > 1E-09)) ? 2000000000.0 : 50000000000.0));
    return num * ((CurrentGame() != nullptr) ? ((double)Params()->DSNModifier) : 1.0);
}

// ---- CommNetScenario.cs ------------------------------------------------------------------------------------------------------

CommNetScenario::CommNetScenario() : ScenarioModule("CommNetScenario") {}
CommNetScenario::~CommNetScenario() {
    if (instance_ == this) instance_ = nullptr;
}

// CommNetScenario.cs:22
IRangeModel& CommNetScenario::RangeModel() {
    if (!rangeModel_) rangeModel_ = std::make_shared<CommRangeModel>();
    return *rangeModel_;
}
void CommNetScenario::SetRangeModel(std::shared_ptr<IRangeModel> m) { rangeModel_ = std::move(m); }

// CommNetScenario.cs:38
bool CommNetScenario::CommNetEnabled() {
    if (CurrentGame() == nullptr) return false;
    if (!CurrentGame()->Parameters) throw NullReferenceException();
    return CurrentGame()->Parameters->Difficulty.EnableCommNet;
}

// CommNetScenario.cs:50
void CommNetScenario::OnAwake() {
    if (!CommNetEnabled()) {
        destroyed_ = true;  // Object.Destroy(this)
        return;
    }
    if (instance_ != nullptr && instance_ != this) {
        log::Error("[CommNetScenario]: Instance already exists!");
        CommNetScenario* old = instance_;
        old->OnDestroy();  // Object.Destroy(Instance): the old module is destroyed (Instance is reassigned below)
        old->destroyed_ = true;
    }
    instance_ = this;
}

// CommNetScenario.cs:65 (ui = AddComponent<CommNetUI>() is not modelled)
void CommNetScenario::Start() {
    network_ = std::make_unique<CommNetNetwork>();
    network_->Awake();
}

// CommNetScenario.cs:71
void CommNetScenario::OnDestroy() {
    if (network_) {
        network_->OnDestroy();
        network_.reset();
    }
    if (instance_ != nullptr && instance_ == this) instance_ = nullptr;
}

// CommNetScenario.cs:87 (base.OnLoad is the empty ScenarioModule.OnLoad)
void CommNetScenario::OnLoad(ConfigNode& gameNode) {
    if (!(instance_ != this)) ScenarioModule::OnLoad(gameNode);
}

// CommNetScenario.cs:95
void CommNetScenario::OnSave(ConfigNode& gameNode) {
    if (!(instance_ != this)) ScenarioModule::OnSave(gameNode);
}

// ---- CommNetNetwork.cs -------------------------------------------------------------------------------------------------------

CommNetNetwork::~CommNetNetwork() {
    if (instance_ == this || awake_) OnDestroy();
}

// CommNetNetwork.cs:66
void CommNetNetwork::Awake() {
    if (instance_ != nullptr && instance_ != this) instance_->OnDestroy();  // Object.DestroyImmediate(Instance)
    instance_ = this;
    awake_ = true;
    // GameEvents.onPlanetariumTargetChanged (TRACKSTATION): the host calls OnMapFocusChange(target.vessel)
    GameEvents::OnGameSettingsApplied.Add(this, &CommNetNetwork::ResetNetwork);
    Reset();
}

// CommNetNetwork.cs:86
void CommNetNetwork::OnDestroy() {
    if (instance_ != nullptr && instance_ == this) instance_ = nullptr;
    if (awake_) {
        GameEvents::OnGameSettingsApplied.Remove(this, &CommNetNetwork::ResetNetwork);
        awake_ = false;
    }
}

// CommNetNetwork.cs:96
void CommNetNetwork::Update() {
    const double num = (double)(CommNetEnvironment::timeSinceLevelLoad ? CommNetEnvironment::timeSinceLevelLoad() : 0.0f);
    Vessel* active = FlightRegistry::ActiveVessel;
    if (commNet_ == nullptr) throw NullReferenceException();
    if (!queueRebuild_ && !commNet_->IsDirty() &&
        !(prevUpdate_ + ((!(active != nullptr) || active->packed) ? packedInterval_ : unpackedInterval_) <= num) &&
        prevUpdate_ + ((focusedVessel != nullptr) ? packedInterval_ : 0.0) > num) {
        graphDirty_ = true;
        return;
    }
    commNet_->Rebuild();
    prevUpdate_ = num;
    graphDirty_ = false;
    queueRebuild_ = false;
}

// CommNetNetwork.cs:115
void CommNetNetwork::Add(CommNode* node) {
    if (instance_ != nullptr && instance_->commNet_ != nullptr) instance_->commNet_->Add(node);
}

// CommNetNetwork.cs:123
void CommNetNetwork::Remove(CommNode* node) {
    if (instance_ != nullptr && instance_->commNet_ != nullptr) instance_->commNet_->Remove(node);
}

// CommNetNetwork.cs:131
void CommNetNetwork::Add(Occluder* occluder) {
    if (instance_ != nullptr && instance_->commNet_ != nullptr) instance_->commNet_->Add(occluder);
}

// CommNetNetwork.cs:139
void CommNetNetwork::Remove(Occluder* occluder) {
    if (instance_ != nullptr && instance_->commNet_ != nullptr) instance_->commNet_->Remove(occluder);
}

// CommNetNetwork.cs:147
void CommNetNetwork::ResetNetwork() {
    commNet_ = std::make_unique<CommNetwork>();
    GameEvents::CommNet::OnNetworkInitialized.Fire();
}

// CommNetNetwork.cs:153
void CommNetNetwork::Reset() {
    if (instance_ != nullptr) instance_->ResetNetwork();
}

// CommNetNetwork.cs:162
void CommNetNetwork::DebugInfo() {
    if (commNet_ == nullptr) throw NullReferenceException();
    if (CommNetEnvironment::debugLog) CommNetEnvironment::debugLog(commNet_->CreateDebug());
}

// ---- CommNetHome.cs ----------------------------------------------------------------------------------------------------------

CommNetHome::~CommNetHome() {
    if (started_) OnDestroy();
}

// CommNetHome.cs:29
void CommNetHome::Start() {
    started_ = true;
    // body = GetComponentInParent<CelestialBody>() is the host's `body`
    if (nodeTransform == nullptr) nodeTransform = transform;
    if (CommNetNetwork::Initialized()) OnNetworkInitialized();
    GameEvents::CommNet::OnNetworkInitialized.Add(this, &CommNetHome::OnNetworkInitialized);
}

// CommNetHome.cs:43
void CommNetHome::OnDestroy() {
    if (comm_ != nullptr) CommNetNetwork::Remove(comm_.get());
    GameEvents::CommNet::OnNetworkInitialized.Remove(this, &CommNetHome::OnNetworkInitialized);
    started_ = false;
}

// CommNetHome.cs:52 (`comm.position = base.transform.position`: CommNode.position has no setter body, so this changes nothing)
void CommNetHome::Update() {
    if (comm_ != nullptr) {
        if (transform == nullptr) throw NullReferenceException();
    }
}

// CommNetHome.cs:60
void CommNetHome::OnNetworkPreUpdate() {
    if (comm_ == nullptr || body == nullptr) throw NullReferenceException();
    comm_->set_precisePosition(body->GetWorldSurfacePosition(lat_, lon_, alt_));
}

// CommNetHome.cs:65
void CommNetHome::OnNetworkInitialized() {
    CreateNode();
    if (comm_ != nullptr) CommNetNetwork::Add(comm_.get());
}

// CommNetHome.cs:74
void CommNetHome::CreateNode() {
    if (CurrentGame() != nullptr && !Params()->enableGroundStations &&
        ((!isKSC && !isPermanent) || (!isKSC && isPermanent && !CurrentGame()->Parameters->Difficulty.AllowOtherLaunchSites))) {
        if (comm_ != nullptr) {
            CommNetNetwork::Remove(comm_.get());
            comm_.reset();
        }
        return;
    }
    if (comm_ == nullptr) {
        comm_ = std::make_unique<CommNode>(nodeTransform);
        comm_->OnNetworkPreUpdate = [this] { OnNetworkPreUpdate(); };
        comm_->isHome = true;
        comm_->isControlSource = true;
        comm_->isControlSourceMultiHop = true;
    }
    comm_->set_name(nodeName);
    comm_->set_displayName(displaynodeName);
    // the curve is the one GameVariables.GetDSNRangeCurve() returns (a fresh DoubleCurve per call); the node keeps it alive
    homeRangeCurve_ = std::make_unique<DoubleCurve>(DSN::GetDSNRangeCurve());
    const float level = CommNetEnvironment::trackingStationLevel ? CommNetEnvironment::trackingStationLevel() : 0.0f;
    comm_->antennaRelay.Update(isPermanent ? antennaPower : DSN::GetDSNRange(level), homeRangeCurve_.get(), false);
    if (body == nullptr) throw NullReferenceException();
    if (nodeTransform == nullptr) {
        if (transform == nullptr) throw NullReferenceException();
        body->GetLatLonAlt(Vector3d(transform->position), lat_, lon_, alt_);
    } else {
        body->GetLatLonAlt(Vector3d(nodeTransform->position), lat_, lon_, alt_);
    }
}

// ---- CommNetBody.cs ----------------------------------------------------------------------------------------------------------

CommNetBody::~CommNetBody() {
    if (started_) OnDestroy();
}

// CommNetBody.cs:12
void CommNetBody::Start() {
    started_ = true;
    if (CommNetNetwork::Initialized()) OnNetworkInitialized();
    GameEvents::CommNet::OnNetworkInitialized.Add(this, &CommNetBody::OnNetworkInitialized);
}

// CommNetBody.cs:22
void CommNetBody::OnDestroy() {
    GameEvents::CommNet::OnNetworkInitialized.Remove(this, &CommNetBody::OnNetworkInitialized);
    started_ = false;
}

// CommNetBody.cs:27
void CommNetBody::OnNetworkInitialized() {
    occluder_ = CreateOccluder();
    CommNetNetwork::Add(occluder_.get());
}

// CommNetBody.cs:33
void CommNetBody::OnNetworkPreUpdate() {
    if (occluder_ == nullptr || body == nullptr) throw NullReferenceException();
    occluder_->position = body->position;
}

// CommNetBody.cs:38
std::unique_ptr<Occluder> CommNetBody::CreateOccluder() {
    if (body == nullptr) throw NullReferenceException();
    double num = body->Radius * body->scaledRadiusHorizonMultiplier *
                 ((CurrentGame() != nullptr) ? ((double)(body->atmosphere ? Params()->occlusionMultiplierAtm : Params()->occlusionMultiplierVac)) : 1.0);
    UnityEngine::Transform* t;
    if (CommNetEnvironment::bodyTransform) {
        t = CommNetEnvironment::bodyTransform(*body);
    } else {
        if (!ownTransform_) ownTransform_ = std::make_unique<UnityEngine::Transform>();
        t = ownTransform_.get();
    }
    return std::make_unique<OccluderHorizonCulling>(t, body->scaledElipRadMult.x * num, body->scaledElipRadMult.y * num, body->scaledElipRadMult.z * num, body);
}

// ---- CommNetNode.cs ----------------------------------------------------------------------------------------------------------

CommNetNode::~CommNetNode() {
    if (started_) OnDestroy();
}

// CommNetNode.cs:23
void CommNetNode::Start() {
    started_ = true;
    comm_ = std::make_unique<CommNode>(transform);
    comm_->OnNetworkPreUpdate = [this] { OnNetworkPreUpdate(); };
    comm_->OnNetworkPostUpdate = [this] { OnNetworkPostUpdate(); };
    if (CommNetNetwork::Initialized()) {
        networkInitialised_ = true;
        CommNetNetwork::Add(comm_.get());
    } else {
        networkInitialised_ = false;
    }
    GameEvents::CommNet::OnNetworkInitialized.Add(this, &CommNetNode::OnNetworkInitialized);
}

// CommNetNode.cs:40
void CommNetNode::OnDestroy() {
    if (networkInitialised_) {
        networkInitialised_ = false;
        CommNetNetwork::Remove(comm_.get());
    }
    GameEvents::CommNet::OnNetworkInitialized.Remove(this, &CommNetNode::OnNetworkInitialized);
    started_ = false;
}

// CommNetNode.cs:50
void CommNetNode::OnNetworkInitialized() {
    networkInitialised_ = true;
    CommNetNetwork::Add(comm_.get());
}

}  // namespace ksp::CommNet
