// WP-39: CommNetVessel.cs (see ksp/commnet/comm_net_vessel.hpp).
#include "ksp/commnet/comm_net_vessel.hpp"

#include <algorithm>

#include "ksp/commnet/commnet_scene.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/modules/module_commnet_parts.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::CommNet {

namespace {
CommNetParams& Params() {
    Game* g = SaveEnvironment::Current().currentGame;  // HighLogic.CurrentGame
    if (g == nullptr || !g->Parameters) throw NullReferenceException();
    return g->Parameters->CustomParams<CommNetParams>();
}

// ProtoPartSnapshot.cs:1260 FindModule(PartModule pm, int moduleIndex)
ProtoPartModuleSnapshot* FindProtoModule(ProtoPartSnapshot& snap, PartModule& pm, int moduleIndex) {
    ProtoPartModuleSnapshot* result = nullptr;
    auto nameOf = [](const ProtoPartModuleSnapshot& m) { return m.moduleName.value_or(std::string()); };
    if (moduleIndex < static_cast<int>(snap.modules.size())) {
        result = snap.modules[static_cast<size_t>(moduleIndex)].get();
        if (nameOf(*result) != pm.moduleName) result = snap.FindModule(pm.moduleName);
    } else {
        result = snap.FindModule(pm.moduleName);
    }
    return result;
}

// ScienceUtil.cs:327 GetTransmitterScore
float GetTransmitterScore(IScienceDataTransmitter& t) {
    // 1f / t.DataRate * (1f + (float)t.DataResourceCost) * (busy ? 100f : 1f) * (canTransmit ? 1f : 100000f): one store at the return
    return f32store(1.0 / fx(t.DataRate()) * (1.0 + fx(f32store(t.DataResourceCost()))) * fx(t.IsBusy() ? 100.0f : 1.0f) *
                    fx(t.CanTransmit() ? 1.0f : 100000.0f));
}
}  // namespace

CommNetVessel::~CommNetVessel() {
    // Unity destroys the component: no event handler and no network node may outlive it
    if (started_ && !destroyed_) {
        if (comm_ != nullptr) CommNetNetwork::Remove(comm_.get());
        GameEvents::CommNet::OnNetworkInitialized.Remove(this, &CommNetVessel::OnNetworkInitialized);
    }
}

VesselControlState CommNetVessel::ControlState() const {
    const ModuleValue* v = Value("controlState");
    const EnumValue* e = v ? std::get_if<EnumValue>(v) : nullptr;
    return e ? static_cast<VesselControlState>(e->value) : VesselControlState::None;
}

void CommNetVessel::SetControlState(VesselControlState s) {
    ModuleValue* v = Value("controlState");
    if (v == nullptr) return;
    if (EnumValue* e = std::get_if<EnumValue>(v)) e->value = static_cast<int>(s);
}

bool CommNetVessel::CanComm() const {
    const ModuleValue* v = Value("canComm");
    const bool* b = v ? std::get_if<bool>(v) : nullptr;
    return b ? *b : true;
}

void CommNetVessel::SetCanComm(bool value) {
    if (ModuleValue* v = Value("canComm")) *v = value;
}

// CommNetVessel.cs:88
void CommNetVessel::OnAwake() {
    if (VesselPtr() != nullptr) VesselPtr()->connection = this;
}

// CommNetVessel.cs:96
void CommNetVessel::OnStart() {
    Vessel* v = VesselPtr();
    if (v == nullptr) throw NullReferenceException();
    started_ = true;
    if (v->vesselType != VesselType::Flag && static_cast<int>(v->vesselType) > static_cast<int>(VesselType::Unknown) &&
        v->vesselType != VesselType::DeployedSciencePart) {
        comm_ = std::make_unique<CommNode>(nullptr);  // new CommNode(base.transform): the v transform is the engine's
        comm_->OnNetworkPreUpdate = [this] { OnNetworkPreUpdate(); };
        comm_->OnNetworkPostUpdate = [this] { OnNetworkPostUpdate(); };
        comm_->OnLinkCreateSignalModifier = [this](CommNode* b) { return GetSignalStrengthModifier(b); };
        v->connection = this;
        networkInitialised_ = false;
        if (CommNetNetwork::Initialized()) OnNetworkInitialized();
        GameEvents::CommNet::OnNetworkInitialized.Add(this, &CommNetVessel::OnNetworkInitialized);
        // TRACKSTATION: GameEvents.onPlanetariumTargetChanged.Add(OnMapFocusChange): the host forwards the focused v
    } else {
        auto& mods = v->vesselModules;
        mods.erase(std::remove_if(mods.begin(), mods.end(), [this](const std::shared_ptr<VesselModule>& m) { return m.get() == this; }), mods.end());
        v->connection = nullptr;
        started_ = false;
        destroyed_ = true;  // UnityEngine.Object.Destroy(this)
    }
}

// CommNetVessel.cs:124
void CommNetVessel::OnDestroy() {
    if (comm_ != nullptr) CommNetNetwork::Remove(comm_.get());
    GameEvents::CommNet::OnNetworkInitialized.Remove(this, &CommNetVessel::OnNetworkInitialized);
    destroyed_ = true;
    if (VesselPtr() != nullptr && VesselPtr()->connection == this) VesselPtr()->connection = nullptr;  // a destroyed component compares == null
}

// CommNetVessel.cs:131
void CommNetVessel::OnMapFocusChange(Vessel* targetVessel) {
    if (targetVessel == VesselPtr()) {
        doUnloadedUpdate = true;
        unloadedDoOnce = true;
    } else {
        doUnloadedUpdate = false;
    }
}

// CommNetVessel.cs:144
void CommNetVessel::Update() {
    Vessel* v = VesselPtr();
    if (v == nullptr) throw NullReferenceException();
    if (v->loaded) {
        const int count = static_cast<int>(v->Parts().size());
        if (count != partCountCache_) {
            partCountCache_ = count;
            FindCommandSources();
        }
        UpdateControlState();
    }
}

// CommNetVessel.cs:158
void CommNetVessel::OnNetworkInitialized() {
    networkInitialised_ = true;
    CommNetNetwork::Add(comm_.get());
}

// CommNetVessel.cs:164
void CommNetVessel::OnNetworkPreUpdate() {
    Vessel* v = VesselPtr();
    if (!unloadedDoOnce && !v->loaded && !overridePreUpdate) {
        inPlasma_ = false;
    } else {
        UpdateComm();
        CalculatePlasmaMult();
    }
    comm_->set_precisePosition(v->GetWorldPos3D());
}

// CommNetVessel.cs:178
void CommNetVessel::CalculatePlasmaMult() {
    Vessel* v = VesselPtr();
    inPlasma_ = false;
    if (v->loaded && !v->packed && Params().plasmaBlackout && v->externalTemperature > PhysicsGlobals::CommNetTempForBlackout() &&
        v->atmDensity > PhysicsGlobals::CommNetDensityForBlackout()) {
        const double num = v->dynamicPressurekPa * v->srfSpeed;
        if (num > PhysicsGlobals::CommNetQTimesVelForBlackoutMin()) {
            plasmaMult_ = UtilMath::InverseLerp(PhysicsGlobals::CommNetQTimesVelForBlackoutMin(), PhysicsGlobals::CommNetQTimesVelForBlackoutMax(), num);
            inPlasma_ = plasmaMult_ > 0.0;
        }
    }
}

// CommNetVessel.cs:192
double CommNetVessel::GetSignalStrengthModifier(CommNode* b) {
    Vessel* v = VesselPtr();
    if (CanComm()) {
        if (inPlasma_) {
            const double num = Vector3d::Dot((b->precisePosition() - v->CoMD).normalized(), v->srf_vel_direction);
            double num2 = ((num < PhysicsGlobals::CommNetDotForBlackoutMin())
                               ? 0.0
                               : ((!(num > PhysicsGlobals::CommNetDotForBlackoutMax()))
                                      ? UtilMath::InverseLerp(PhysicsGlobals::CommNetDotForBlackoutMin(), PhysicsGlobals::CommNetDotForBlackoutMax(), num)
                                      : 1.0));
            num2 *= plasmaMult_;
            num2 = 1.0 - num2;
            if (num2 < PhysicsGlobals::CommNetBlackoutThreshold()) return 0.0;
            num2 = (num2 - PhysicsGlobals::CommNetBlackoutThreshold()) / (1.0 - PhysicsGlobals::CommNetBlackoutThreshold());
            if (num2 > 1.0) num2 = 1.0;
            return num2;
        }
        return 1.0;
    }
    return 0.0;
}

// CommNetVessel.cs:218
void CommNetVessel::UpdateComm() {
    Vessel* v = VesselPtr();
    if (v == nullptr) return;
    if (comm_->name() != v->vesselName) comm_->set_name(v->vesselName);  // gameObject.name
    if (comm_->displayName() != v->GetDisplayName()) comm_->set_displayName(v->GetDisplayName());
    ICommAntenna* commAntenna = nullptr;
    int num = -1;
    ICommAntenna* commAntenna2 = nullptr;
    double num2 = 0.0;
    double num3 = 0.0;
    double num4 = 0.0;
    double num5 = 0.0;
    ICommAntenna* commAntenna3 = nullptr;
    double num6 = 0.0;
    double num7 = 0.0;
    double num8 = 0.0;
    double num9 = 0.0;
    ICommAntenna* commAntenna4 = nullptr;
    ICommAntenna* commAntenna5 = nullptr;
    ProtoPartSnapshot* protoPartSnapshot = nullptr;
    ProtoPartModuleSnapshot* mSnap = nullptr;
    bool flag = false;
    comm_->isControlSource = false;
    comm_->isControlSourceMultiHop = false;
    CommNode::AntennaInfo& antennaRelay = comm_->antennaRelay;
    CommNode::AntennaInfo& antennaTransmit = comm_->antennaTransmit;
    double power = 0.0;
    antennaTransmit.power = 0.0;
    antennaRelay.power = power;
    hasScienceAntenna_ = false;
    int index = (v->loaded ? static_cast<int>(v->Parts().size()) : static_cast<int>(v->protoVessel->protoPartSnapshots.size()));
    while (index-- > 0) {
        Part* part;
        if (v->loaded) {
            part = v->Parts()[static_cast<size_t>(index)];
        } else {
            protoPartSnapshot = v->protoVessel->protoPartSnapshots[static_cast<size_t>(index)].get();
            part = protoPartSnapshot->partInfo->partPrefab.get();
        }
        int count = static_cast<int>(part->Modules().Count());
        while (count-- > 0) {
            PartModule* partModule = part->Modules()[static_cast<size_t>(count)];
            if (IRelayEnabler* relay = dynamic_cast<IRelayEnabler*>(partModule)) {
                if (v->loaded) {
                    if (relay->CanRelay()) flag = true;
                } else if (relay->CanRelayUnloaded(FindProtoModule(*protoPartSnapshot, *partModule, count))) {
                    flag = true;
                }
            } else if (dynamic_cast<ICommAntenna*>(partModule) != nullptr) {
                commAntenna = dynamic_cast<ICommAntenna*>(partModule);
                if (v->loaded) {
                    if (!commAntenna->CanComm()) continue;
                } else if (!commAntenna->CanCommUnloaded(mSnap = FindProtoModule(*protoPartSnapshot, *partModule, count))) {
                    continue;
                }
                const double num10 = (v->loaded ? commAntenna->CommPower() : commAntenna->CommPowerUnloaded(mSnap));
                if (commAntenna->CommType() != AntennaType::RELAY) {
                    if (commAntenna->CommType() != AntennaType::INTERNAL) hasScienceAntenna_ = true;
                    if (num10 > num2) {
                        commAntenna2 = commAntenna;
                        num2 = num10;
                    }
                    if (commAntenna->CommCombinable()) {
                        num3 += num10;
                        num4 += num10 * commAntenna->CommCombinableExponent();
                        if (commAntenna4 == nullptr || num10 > num5) {
                            commAntenna4 = commAntenna;
                            num5 = num10;
                        }
                    }
                    continue;
                }
                hasScienceAntenna_ = true;
                if (num10 > num6) {
                    commAntenna3 = commAntenna;
                    num6 = num10;
                    if (commAntenna3->CommCombinable()) commAntenna5 = commAntenna;
                }
                if (commAntenna->CommCombinable()) {
                    num7 += num10;
                    num8 += num10 * commAntenna->CommCombinableExponent();
                    if (commAntenna5 == nullptr || num10 > num9) {
                        commAntenna5 = commAntenna;
                        num9 = num10;
                    }
                }
            } else {
                ModuleProbeControlPoint* moduleProbeControlPoint = dynamic_cast<ModuleProbeControlPoint*>(partModule);
                if (moduleProbeControlPoint == nullptr) continue;
                if (v->loaded) {
                    if (!moduleProbeControlPoint->CanControl()) continue;
                } else if (!moduleProbeControlPoint->CanControlUnloaded(mSnap = FindProtoModule(*protoPartSnapshot, *partModule, count))) {
                    continue;
                }
                if (moduleProbeControlPoint->minimumCrew() > 0) {
                    if (num < 0) {
                        const std::vector<Vessel::PCM>& vesselCrew = v->GetVesselCrew();
                        num = 0;
                        int count2 = static_cast<int>(vesselCrew.size());
                        while (count2-- > 0) {
                            if (vesselCrew[static_cast<size_t>(count2)]->HasEffect<Experience::Effects::FullVesselControlSkill>() &&
                                !vesselCrew[static_cast<size_t>(count2)]->inactive())
                                num++;
                        }
                    }
                    if (num >= moduleProbeControlPoint->minimumCrew()) comm_->isControlSource = true;
                } else {
                    comm_->isControlSource = true;
                }
                if (moduleProbeControlPoint->multiHop()) comm_->isControlSourceMultiHop = true;
            }
        }
    }
    if (commAntenna2 != nullptr) {
        double num11 = num3 + num7;
        const bool flag2 = num5 > num9;
        if (num11 > 0.0) {
            const double num12 = (flag2 ? num5 : num9);
            if (num11 != num12) {
                const double y = (num4 + num8) / num11;
                num11 = num12 * dotnet::Math::Pow(num11 / num12, y);
            }
            if (num7 > 0.0 && num7 != num9) num7 = num9 * dotnet::Math::Pow(num7 / num9, num8 / num7);
        }
        if (num3 > 0.0 && num11 > num2) {
            commAntenna2 = ((commAntenna5 == nullptr) ? commAntenna4 : (flag2 ? commAntenna4 : commAntenna5));
            comm_->antennaTransmit.combined = true;
        } else {
            num11 = num2;
            comm_->antennaTransmit.combined = false;
        }
        comm_->antennaTransmit.power = num11;
        comm_->antennaTransmit.rangeCurve = commAntenna2->CommRangeCurve();
        comm_->scienceCurve = commAntenna2->CommScienceCurve();
    }
    if (commAntenna3 != nullptr) {
        double num13;
        if (num7 > num6) {
            num13 = num7;
            commAntenna3 = commAntenna5;
            comm_->antennaRelay.combined = true;
        } else {
            num13 = num6;
            comm_->antennaRelay.combined = false;
        }
        comm_->antennaRelay.power = num13;
        comm_->antennaRelay.rangeCurve = commAntenna3->CommRangeCurve();
        if (commAntenna2 == nullptr || num13 > comm_->antennaTransmit.power) comm_->scienceCurve = commAntenna3->CommScienceCurve();
    }
    if (flag && comm_->antennaTransmit.power > comm_->antennaRelay.power) {
        comm_->antennaRelay.power = comm_->antennaTransmit.power;
        comm_->antennaRelay.combined = comm_->antennaTransmit.combined;
        comm_->antennaRelay.rangeCurve = comm_->antennaTransmit.rangeCurve;
        comm_->antennaTransmit.power = 0.0;
    }
    comm_->antennaTransmit.power *= (double)Params().rangeModifier;
    comm_->antennaRelay.power *= (double)Params().rangeModifier;
}

// CommNetVessel.cs:459
void CommNetVessel::OnNetworkPostUpdate() {
    Vessel* v = VesselPtr();
    if (!unloadedDoOnce && !v->loaded && !overridePostUpdate) return;
    CreateControlConnection();
    if (v->loaded) {
        const int count = static_cast<int>(commandSources_.size());
        for (int i = 0; i < count; i++) commandSources_[static_cast<size_t>(i)]->UpdateNetwork();
        UpdateControlState();
    }
    if (OnNetworkUpdate) OnNetworkUpdate();
    unloadedDoOnce = doUnloadedUpdate;
}

// CommNetVessel.cs:482
void CommNetVessel::UpdateControlState() {
    VesselControlState controlState = VesselControlState::None;
    bool canComm = false;
    const int count = static_cast<int>(commandSources_.size());
    for (int i = 0; i < count; i++) {
        const VesselControlState controlSourceState = commandSources_[static_cast<size_t>(i)]->GetControlSourceState();
        if (static_cast<int>(controlSourceState) > static_cast<int>(controlState)) controlState = controlSourceState;
        canComm |= commandSources_[static_cast<size_t>(i)]->IsCommCapable();
    }
    SetControlState(controlState);
    SetCanComm(canComm);
}

// CommNetVessel.cs:498
bool CommNetVessel::CreateControlConnection() {
    const bool isConnected = IsConnected;
    const bool isConnectedHome = IsConnectedHome;
    CommNetwork* net = static_cast<CommNetwork*>(comm_->GetNet());
    if (net == nullptr) throw NullReferenceException();
    if (!net->FindHome(comm_.get(), &controlPath_)) net->FindClosestControlSource(comm_.get(), &controlPath_);
    IsConnected = controlPath_.signal() != SignalStrength::None;
    if (IsConnected && !controlPath_.LastLink()) throw NullReferenceException();
    IsConnectedHome = IsConnected && controlPath_.LastLink()->hopType == HopType::Home;
    const bool num = isConnected != IsConnected;
    const bool flag = isConnectedHome != IsConnectedHome;
    if (num) GameEvents::CommNet::OnCommStatusChange.Fire(VesselPtr(), IsConnected);
    if (flag) GameEvents::CommNet::OnCommHomeStatusChange.Fire(VesselPtr(), IsConnectedHome);
    return num;
}

// CommNetVessel.cs:521
IScienceDataTransmitter* CommNetVessel::GetBestTransmitter() {
    if (!IsConnected) return nullptr;
    IScienceDataTransmitter* result = nullptr;
    float num = 3.40282347E+38f;
    std::shared_ptr<Network::Link> first = controlPath_.First();
    if (!first) throw NullReferenceException();
    double cost = first->start()->At(first->end())->cost;
    cost *= cost;
    const double power = static_cast<CommNode*>(first->end())->antennaRelay.power;
    const bool combined = ((comm_->antennaRelay.power > comm_->antennaTransmit.power) ? comm_->antennaRelay.combined : comm_->antennaTransmit.combined);
    Vessel* v = VesselPtr();
    int count = static_cast<int>(v->Parts().size());
    while (count-- > 0) {
        Part* part = v->Parts()[static_cast<size_t>(count)];
        int count2 = static_cast<int>(part->Modules().Count());
        while (count2-- > 0) {
            PartModule* partModule = part->Modules()[static_cast<size_t>(count2)];
            IScienceDataTransmitter* scienceDataTransmitter = dynamic_cast<IScienceDataTransmitter*>(partModule);
            if (scienceDataTransmitter != nullptr && scienceDataTransmitter->CanTransmit()) {
                ICommAntenna* commAntenna = dynamic_cast<ICommAntenna*>(partModule);
                if (commAntenna == nullptr || commAntenna->CanScienceTo(combined, power, cost)) {
                    const float transmitterScore = GetTransmitterScore(*scienceDataTransmitter);
                    if (transmitterScore < num) {
                        num = transmitterScore;
                        result = scienceDataTransmitter;
                    }
                }
            }
        }
    }
    return result;
}

// CommNetVessel.cs:555
ControlLevel CommNetVessel::GetControlLevel() {
    const int s = static_cast<int>(ControlState());
    if ((s & static_cast<int>(VesselControlState::Full)) > 0) return ControlLevel::FULL;
    if ((s & static_cast<int>(VesselControlState::Partial)) > 0) {
        if ((s & static_cast<int>(VesselControlState::Kerbal)) > 0) return ControlLevel::PARTIAL_MANNED;
        return ControlLevel::PARTIAL_UNMANNED;
    }
    return ControlLevel::NONE;
}

// CommNetVessel.cs:572
void CommNetVessel::RegisterCommandSource(ICommNetControlSource* cmd) {
    if (std::find(commandSources_.begin(), commandSources_.end(), cmd) == commandSources_.end()) commandSources_.push_back(cmd);
}

// CommNetVessel.cs:577
void CommNetVessel::UnregisterCommandSource(ICommNetControlSource* cmd) {
    auto it = std::find(commandSources_.begin(), commandSources_.end(), cmd);
    if (it != commandSources_.end()) commandSources_.erase(it);
}

// CommNetVessel.cs:582
void CommNetVessel::FindCommandSources() {
    commandSources_.clear();
    Vessel* v = VesselPtr();
    for (int i = 0; i < partCountCache_; i++) {
        Part* part = v->Parts()[static_cast<size_t>(i)];
        for (int j = 0, count = static_cast<int>(part->Modules().Count()); j < count; j++) {
            if (ICommNetControlSource* item = dynamic_cast<ICommNetControlSource*>(part->Modules()[static_cast<size_t>(j)])) commandSources_.push_back(item);
        }
    }
}

}  // namespace ksp::CommNet
