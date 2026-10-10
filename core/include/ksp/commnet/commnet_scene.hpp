// WP-39: the scene objects of CommNet: CommNetScenario.cs (the ScenarioModule), CommNetNetwork.cs (the MonoBehaviour that owns the CommNetwork and
// rebuilds it on a timer), CommNetHome.cs (ground stations), CommNetBody.cs (the planets as horizon occluders), CommNetNode.cs, CommNetParams.cs
// (already ported by WP-04: ksp/save/game_parameters.hpp) and the two GameVariables functions CommNet reads (GetDSNRange, GetDSNRangeCurve).
//
// MonoBehaviour lifecycles are explicit calls (Awake / Start / Update / OnDestroy); GameObject plumbing, AddComponent<CommNetUI> and the UI
// are not modelled. Everything the engine / other packages provide is in CommNetEnvironment (each hook documents its unset default).
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/commnet/comm_network.hpp"
#include "ksp/save/game.hpp"

namespace ksp {
class CelestialBody;
class Vessel;
}  // namespace ksp

namespace ksp::CommNet {

class CommNetNetwork;
class CommNetVessel;

struct CommNetEnvironment {
    // ScenarioUpgradeableFacilities.GetFacilityLevel(SpaceCenterFacility.TrackingStation) (normalised 0..1, WP-36). Unset: 0 (level 1).
    static inline std::function<float()> trackingStationLevel;
    // CelestialBody.transform (the body's GameObject; the horizon occluder reads its rotation). Unset: an identity transform owned by the body object.
    static inline std::function<UnityEngine::Transform*(CelestialBody&)> bodyTransform;
    // Time.timeSinceLevelLoad (float). Unset: 0.
    static inline std::function<float()> timeSinceLevelLoad;
    // GameSettings / UI: Debug.Log of CommNetNetwork.DebugInfo. Unset: nothing.
    static inline std::function<void(const std::string&)> debugLog;
    static void Reset();
};

// GameVariables.cs:537 / :562 (CommNet only reads these two; the rest of GameVariables belongs to WP-36)
namespace DSN {
DoubleCurve GetDSNRangeCurve();
double GetDSNRange(float level);
}  // namespace DSN

// CommNetScenario.cs:12
class CommNetScenario : public ScenarioModule {
public:
    CommNetScenario();
    ~CommNetScenario() override;
    static CommNetScenario* Instance() { return instance_; }
    static IRangeModel& RangeModel();                    // :22 (creates a CommRangeModel on first use)
    static void SetRangeModel(std::shared_ptr<IRangeModel> m);
    static bool CommNetEnabled();                        // :38

    void OnAwake() override;                             // :50 (Object.Destroy(this) when disabled is reported by returning through destroyed())
    void Start();                                        // :65 (AddComponent<CommNetNetwork>; CommNetUI is not modelled)
    void OnDestroy();                                    // :71
    void OnLoad(ConfigNode& gameNode) override;          // :87
    void OnSave(ConfigNode& gameNode) override;          // :95
    bool destroyed() const { return destroyed_; }        // Object.Destroy(this) happened
    CommNetNetwork* Network() const { return network_.get(); }

private:
    static inline CommNetScenario* instance_ = nullptr;
    static inline std::shared_ptr<IRangeModel> rangeModel_;
    std::unique_ptr<CommNetNetwork> network_;
    bool destroyed_ = false;
};

// CommNetNetwork.cs:5
class CommNetNetwork {
public:
    CommNetNetwork() = default;
    ~CommNetNetwork();
    static CommNetNetwork* Instance() { return instance_; }
    static bool Initialized() { return instance_ != nullptr; }  // :64
    CommNetwork* CommNet() const { return commNet_.get(); }
    void set_CommNet(std::unique_ptr<CommNetwork> n) { commNet_ = std::move(n); }
    double UnpackedInterval() const { return unpackedInterval_; }
    double PackedInterval() const { return packedInterval_; }
    void set_UnpackedInterval(double v) { unpackedInterval_ = v; }
    void set_PackedInterval(double v) { packedInterval_ = v; }
    bool GraphDirty() const { return graphDirty_; }
    Vessel* focusedVessel = nullptr;

    void Awake();                            // :66
    void OnMapFocusChange(Vessel* targetVessel) { focusedVessel = targetVessel; }  // :81 (target.vessel; the host subscribes onPlanetariumTargetChanged)
    void OnDestroy();                        // :86
    void Update();                           // :96
    void QueueRebuild() { queueRebuild_ = true; }  // :110
    static void Add(CommNode* node);         // :115
    static void Remove(CommNode* node);      // :123
    static void Add(Occluder* occluder);     // :131
    static void Remove(Occluder* occluder);  // :139
    void ResetNetwork();                     // :147
    static void Reset();                     // :153
    void DebugInfo();                        // :162

protected:
    static inline CommNetNetwork* instance_ = nullptr;
    std::unique_ptr<CommNetwork> commNet_;
    double unpackedInterval_ = 5.0;
    double packedInterval_ = 0.5;
    double prevUpdate_ = -1.7976931348623157E+308;  // double.MinValue
    bool queueRebuild_ = false;
    bool graphDirty_ = false;
    bool awake_ = false;
};

// CommNetHome.cs:5
class CommNetHome {
public:
    std::string nodeName = std::string();
    std::string displaynodeName = std::string();
    UnityEngine::Transform* nodeTransform = nullptr;
    bool isKSC = false;
    bool isPermanent = false;
    double antennaPower = 500000.0;
    // host-supplied components: GetComponentInParent<CelestialBody>() and base.transform
    CelestialBody* body = nullptr;
    UnityEngine::Transform* transform = nullptr;

    ~CommNetHome();
    CommNode* Comm() const { return comm_.get(); }
    void Start();                     // :29
    void OnDestroy();                 // :43
    void Update();                    // :52
    void OnNetworkPreUpdate();        // :60
    void OnNetworkInitialized();      // :65
    void CreateNode();                // :74

protected:
    std::unique_ptr<CommNode> comm_;
    double lat_ = 0.0, lon_ = 0.0, alt_ = 0.0;
    std::unique_ptr<DoubleCurve> homeRangeCurve_;  // GameVariables.GetDSNRangeCurve() result held by comm.antennaRelay.rangeCurve
    bool started_ = false;
};

// CommNetBody.cs:6
class CommNetBody {
public:
    CelestialBody* body = nullptr;  // GetComponent<CelestialBody>()
    ~CommNetBody();
    Occluder* GetOccluder() const { return occluder_.get(); }
    void Start();                   // :12
    void OnDestroy();               // :22
    void OnNetworkInitialized();    // :27
    void OnNetworkPreUpdate();      // :33
    std::unique_ptr<Occluder> CreateOccluder();  // :38

protected:
    std::unique_ptr<Occluder> occluder_;
    std::unique_ptr<UnityEngine::Transform> ownTransform_;  // CommNetEnvironment::bodyTransform unset
    bool started_ = false;
};

// CommNetNode.cs:5
class CommNetNode {
public:
    UnityEngine::Transform* transform = nullptr;  // base.transform
    CommNode* Comm() const { return comm_.get(); }
    void set_Comm(std::unique_ptr<CommNode> c) { comm_ = std::move(c); }
    void Start();                   // :23
    void OnDestroy();               // :40
    void OnNetworkInitialized();    // :50
    virtual void OnNetworkPreUpdate() {}   // :56
    virtual void OnNetworkPostUpdate() {}  // :60
    virtual ~CommNetNode();

protected:
    std::unique_ptr<CommNode> comm_;
    bool networkInitialised_ = false;
    bool started_ = false;
};

}  // namespace ksp::CommNet
