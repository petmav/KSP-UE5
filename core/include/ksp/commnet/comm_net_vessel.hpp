// WP-39: CommNetVessel.cs (the VesselModule that gives a Vessel its CommNode and its connection to the network).
//
// Ported as written: UpdateComm (the antenna scan with the combinable-antenna power sums, `Math.Pow(total / best, exponent)`, the relay /
// transmit split, the RangeModifier scaling), the loaded and unloaded (ProtoVessel) paths, the control-state evaluation over the registered
// ICommNetControlSource command sources, CalculatePlasmaMult / GetSignalStrengthModifier (plasma blackout), OnNetworkPreUpdate / PostUpdate,
// CreateControlConnection (FindHome, then FindClosestControlSource; the OnCommStatusChange / OnCommHomeStatusChange events),
// GetBestTransmitter, GetControlLevel and the command-source bookkeeping.
//
// The two [KSPField(isPersistant)] members (controlState, canComm) live in the module's FieldStore like every KSPField, so a save / load
// round trip is the generic field list. Seams: gameObject.name is the vessel's name; the MonoBehaviour callbacks are explicit calls
// (OnAwake / OnStart through VesselModule, Update(), OnDestroy()); GameEvents.onPlanetariumTargetChanged -> OnMapFocusChange(vessel).
#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "ksp/commnet/comm_network.hpp"
#include "ksp/commnet/science_transmitter.hpp"
#include "ksp/modules/vessel_module.hpp"
#include "ksp/vessel/vessel_types.hpp"

namespace ksp {
class Vessel;
class Part;
class PartModule;
}  // namespace ksp

namespace ksp::CommNet {

// CommNetVessel.cs:8
class CommNetVessel : public VesselModule {
public:
    explicit CommNetVessel(const ModuleClassInfo& cls) : VesselModule(cls) {}
    ~CommNetVessel() override;

    std::function<void()> OnNetworkUpdate;

    bool IsConnected = false;       // protected set
    bool IsConnectedHome = false;
    bool CanScience() const { return hasScienceAntenna_ ? IsConnectedHome : false; }  // :46
    VesselControlState ControlState() const;                                          // :60
    bool CanComm() const;                                                             // :62 (`canComm`)
    bool InPlasma() const { return inPlasma_; }
    CommPath& ControlPath() { return controlPath_; }
    SignalStrength Signal() const { return controlPath_.signal(); }                   // :68 (`Signal`)
    double SignalStrengthValue() const { return controlPath_.signalStrength(); }      // :70 (`SignalStrength`)
    double SignalDelay() const { return signalDelay_; }
    std::vector<ICommNetControlSource*>& CommandSources() { return commandSources_; }
    CommNode* Comm() const { return comm_.get(); }
    void set_Comm(std::unique_ptr<CommNode> c) { comm_ = std::move(c); }

    // test / host controls of the protected switches
    bool overridePreUpdate = false;
    bool overridePostUpdate = false;
    bool doUnloadedUpdate = false;
    bool unloadedDoOnce = true;

    Vessel* VesselPtr() const { return static_cast<Vessel*>(vessel); }

    void OnAwake() override;                 // :88
    void OnStart() override;                 // :96
    void OnDestroy();                        // :124
    void OnMapFocusChange(Vessel* targetVessel);  // :131
    void Update();                           // :144
    void OnNetworkInitialized();             // :158
    virtual void OnNetworkPreUpdate();       // :164
    virtual double GetSignalStrengthModifier(CommNode* b);  // :192
    virtual void OnNetworkPostUpdate();      // :459
    virtual IScienceDataTransmitter* GetBestTransmitter();  // :521
    virtual ControlLevel GetControlLevel();  // :555
    void RegisterCommandSource(ICommNetControlSource* cmd);    // :572
    void UnregisterCommandSource(ICommNetControlSource* cmd);  // :577
    void FindCommandSources();               // :582
    // test hook: the private state of the plasma model
    void SetPlasmaForTest(bool inPlasma, double mult) { inPlasma_ = inPlasma; plasmaMult_ = mult; }

    // the protected members the subclass-style callers use
    void UpdateControlState();               // :482
    virtual bool CreateControlConnection();  // :498

protected:
    virtual void CalculatePlasmaMult();      // :178
    virtual void UpdateComm();               // :218

private:
    void SetControlState(VesselControlState s);
    void SetCanComm(bool v);
    bool hasScienceAntenna_ = false;
    bool inPlasma_ = false;
    double plasmaMult_ = 1.0;
    CommPath controlPath_;
    double signalDelay_ = 0.0;
    std::vector<ICommNetControlSource*> commandSources_;
    int partCountCache_ = 0;
    bool networkInitialised_ = false;
    std::unique_ptr<CommNode> comm_;
    bool started_ = false;
    bool destroyed_ = false;
};

}  // namespace ksp::CommNet
