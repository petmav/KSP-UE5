// WP-39: the CommNet network proper: CommNode.cs, CommLink.cs, CommPath.cs, CommNetwork.cs, NodeUtilities.cs, SignalStrength.cs, HopType.cs,
// IRangeModel.cs, CommRangeModel.cs, ICommAntenna.cs, AntennaType.cs, IRelayEnabler.cs, ICommNetControlSource.cs and
// Occluders/OccluderHorizonCulling.cs, on top of the generic network (ksp/commnet/network.hpp).
//
// Ported as written: link creation (SetNodeConnection / TryConnect with the signal-strength sums of the three relay combinations), the
// occlusion test, the best-path searches (CreateShortestPathTree / UpdateShortestPath / UpdateShortestWhere keep the maximum PRODUCT of
// link signal strengths, not the shortest distance) and FindHome / FindClosestControlSource.
//
// Seams: CommNode.transform is a UnityEngine::Transform (the engine owns the real one); an antenna's range curve is a non-owning pointer to the
// DoubleCurve of the module that supplied it (CommNetVessel.UpdateComm stores `commAntenna.CommRangeCurve`); the horizon occluder takes the
// CelestialBody it would find with transform.GetComponent<CelestialBody>() as a constructor argument.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/commnet/network.hpp"
#include "ksp/curves/double_curve.hpp"
#include "ksp/math/quaterniond.hpp"
#include "ksp/commnet/control_source.hpp"
#include "ksp/unity/transform.hpp"

namespace ksp {
class ProtoPartModuleSnapshot;
class CelestialBody;

// AntennaType.cs (global enum); the [Description] strings are #autoLOC_6004049 / 6004050 / 6004051
enum class AntennaType : int { INTERNAL = 0, DIRECT = 1, RELAY = 2 };
}  // namespace ksp

namespace ksp::CommNet {

class CommNetwork;
class CommLink;
class CommNode;
class CommPath;

// SignalStrength.cs
enum class SignalStrength : int { None, Red, Orange, Yellow, Green };
// HopType.cs
enum class HopType : int { Relay, ControlPoint, Home };

// NodeUtilities.cs
namespace NodeUtilities {
SignalStrength ConvertSignalStrength(double signalStrength);
}

// IRangeModel.cs
class IRangeModel {
public:
    virtual ~IRangeModel() = default;
    virtual bool InRange(double aPower, double bPower, double sqrDistance) = 0;
    virtual double GetNormalizedRange(double aPower, double bPower, double distance) = 0;
    virtual double GetMaximumRange(double aPower, double bPower) = 0;
};

// CommRangeModel.cs
class CommRangeModel : public IRangeModel {
public:
    bool InRange(double aPower, double bPower, double sqrDistance) override;
    double GetNormalizedRange(double aPower, double bPower, double distance) override;
    double GetMaximumRange(double aPower, double bPower) override;
};

// ICommAntenna.cs
class ICommAntenna {
public:
    virtual ~ICommAntenna() = default;
    virtual bool CommCombinable() const = 0;
    virtual double CommCombinableExponent() const = 0;
    virtual double CommPower() = 0;
    virtual AntennaType CommType() const = 0;
    virtual const DoubleCurve* CommRangeCurve() const = 0;
    virtual const DoubleCurve* CommScienceCurve() const = 0;
    virtual bool CanComm() = 0;
    virtual bool CanCommUnloaded(ProtoPartModuleSnapshot* mSnap) = 0;
    virtual bool CanScienceTo(bool combined, double bPower, double sqrDistance) = 0;
    virtual double CommPowerUnloaded(ProtoPartModuleSnapshot* mSnap) = 0;
};

// IRelayEnabler.cs
class IRelayEnabler {
public:
    virtual ~IRelayEnabler() = default;
    virtual bool CanRelay() = 0;
    virtual bool CanRelayUnloaded(ProtoPartModuleSnapshot* mSnap) = 0;
};

// CommNode.cs:7
class CommNode : public Network::Node {
public:
    // CommNode.cs:9
    class AntennaInfo {
    public:
        double power = 0.0;
        const DoubleCurve* rangeCurve = nullptr;
        bool combined = false;
        AntennaInfo() = default;
        AntennaInfo(double pow, const DoubleCurve* range, bool combine) : power(pow), rangeCurve(range), combined(combine) {}
        void Update(double pow, const DoubleCurve* curve, bool combine) {
            power = pow;
            rangeCurve = curve;
            combined = combine;
        }
    };

    double distanceOffset = 0.0;
    bool isHome = false;
    bool isControlSource = false;
    bool isControlSourceMultiHop = true;
    AntennaInfo antennaTransmit;
    AntennaInfo antennaRelay;
    const DoubleCurve* scienceCurve = nullptr;
    std::function<void()> OnNetworkPreUpdate;
    std::function<void()> OnNetworkPostUpdate;
    std::function<double(CommNode*)> OnLinkCreateSignalModifier;
    UnityEngine::Transform* transform = nullptr;

    CommNode() = default;
    explicit CommNode(UnityEngine::Transform* t) : transform(t) {}

    std::string name() const override { return name_; }
    void set_name(const std::string& v) { name_ = v; }
    std::string displayName() const override { return displayname_; }
    void set_displayName(const std::string& v) { displayname_ = v; }
    virtual Vector3d precisePosition() const { return position_; }
    virtual void set_precisePosition(const Vector3d& v) { position_ = v; }
    Vector3d position() const override;
    void NetworkPreUpdate() override;
    void NetworkPostUpdate() override;
    virtual double GetSignalStrengthMultiplier(CommNode* b);
    std::string ToString();

private:
    std::string name_ = "";
    std::string displayname_ = "";
    Vector3d position_;
};

// CommLink.cs:6
class CommLink : public Network::Link {
public:
    SignalStrength signal = SignalStrength::None;
    HopType hopType = HopType::Relay;
    bool aCanRelay = false;
    bool bCanRelay = false;
    bool bothRelay = false;
    double strengthRR = 0.0;
    double strengthAR = 0.0;
    double strengthBR = 0.0;

    using Network::Link::Set;
    double signalStrength() const { return sS_; }
    void set_signalStrength(double v) { sS_ = v; }
    virtual void Set(CommNode* a_, CommNode* b_, double distance, double signalStrength_);
    void Update(double cost_) override;
    double GetSignalStrength(bool aMustRelay, bool bMustRelay) const;
    double GetSignalStrength(const CommNode* pathStartNode) const;
    double GetBestSignal() const;
    void SetSignalStrength(double ss) { sS_ = ss; }
    void SetSignalStrength(bool aMustRelay, bool bMustRelay) { sS_ = GetSignalStrength(aMustRelay, bMustRelay); }
    void ZeroSignalStrength() { sS_ = 0.0; }
    CommNode* A() const;
    CommNode* B() const;

private:
    double sS_ = 0.0;
};

// CommPath.cs:5
class CommPath : public Network::Path {
public:
    VesselControlState commType() const { return commType_; }
    double signalStrength() const { return signalStrength_; }
    SignalStrength signal() const { return signal_; }
    LinkPtr First() const override { return first_; }
    LinkPtr Last() const override { return last_; }
    std::shared_ptr<CommLink> FirstLink() const { return first_; }
    std::shared_ptr<CommLink> LastLink() const { return last_; }
    std::shared_ptr<CommLink> LinkAt(int i) const { return std::static_pointer_cast<CommLink>((*this)[i]); }
    bool IsLastHopHome() const;
    // `public new virtual void Clear()`: hides List.Clear; Net.cs only knows Path and calls ClearList()
    virtual void Clear();
    void UpdateFromPath() override;
    std::string ToString() const;

private:
    std::shared_ptr<CommLink> first_;
    std::shared_ptr<CommLink> last_;
    VesselControlState commType_ = VesselControlState::None;
    double signalStrength_ = 0.0;
    SignalStrength signal_ = SignalStrength::None;
};

// Occluders/OccluderHorizonCulling.cs:6
class OccluderHorizonCulling : public Occluder {
public:
    // `transform` = the body's transform; `bodyComponent` = transform.GetComponent<CelestialBody>() (null for a non-body occluder)
    OccluderHorizonCulling(UnityEngine::Transform* transform, double radiusX, double radiusY, double radiusZ, CelestialBody* bodyComponent = nullptr);
    bool InRange(const Vector3d& source, double distance) override;
    bool Raycast(const Vector3d& source, const Vector3d& dest) override;
    void Update() override;

protected:
    UnityEngine::Transform* transform_ = nullptr;
    CelestialBody* body_ = nullptr;
    bool useBody_ = false;
    double radiusXRecip_ = 0.0, radiusYRecip_ = 0.0, radiusZRecip_ = 0.0;
    bool anyZero_ = false;
    QuaternionD invRotation_;
};

// CommNetwork.cs:8
class CommNetwork : public Network::Net {
public:
    bool IsDirty() const { return isDirty_; }
    void MarkDirty() { isDirty_ = true; }  // (test hook: Add / Remove set it in C#)
    void Rebuild() override;
    virtual bool FindHome(CommNode* from, CommPath* path = nullptr);
    virtual bool FindClosestControlSource(CommNode* from, CommPath* path = nullptr);
    bool FindPath(Network::Node* start, Network::Path* path, Network::Node* end) override;
    Network::Node* FindClosestWhere(Network::Node* start, Network::Path* path, const WhereFn& where) override;
    Network::Node* Add(Network::Node* conn) override;
    bool Remove(Network::Node* conn) override;
    using Network::Net::Add;
    using Network::Net::Remove;
    std::string CreateDebug();
    CommNode* NodeAt(int i) const { return static_cast<CommNode*>((*this)[i]); }

protected:
    bool isDirty_ = false;
    virtual bool TryConnect(CommNode* a, CommNode* b, double distance, bool aCanRelay, bool bCanRelay, bool bothRelay);
    bool SetNodeConnection(Network::Node* a, Network::Node* b) override;
    virtual bool TestOcclusion(const Vector3d& aPos, Occluder* a, const Vector3d& bPos, Occluder* b, double distance);
    static bool IsControlSource(CommNode* start, CommNode* a);
    static bool IsHome(CommNode* start, CommNode* a);
    std::shared_ptr<Network::Link> NewLink() override;
    void CreateShortestPathTree(Network::Node* start, Network::Node* end) override;
    void UpdateShortestPath(Network::Node* a, Network::Node* b, Network::Link* link, double bestCost, Network::Node* startNode, Network::Node* endNode) override;
    Network::Node* UpdateShortestWhere(Network::Node* a, Network::Node* b, Network::Link* link, double bestCost, Network::Node* startNode,
                                       const WhereFn& whereClause) override;
};

}  // namespace ksp::CommNet
