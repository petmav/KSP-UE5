// WP-39: CommNode.cs, CommLink.cs, CommPath.cs, CommNetwork.cs, NodeUtilities.cs, CommRangeModel.cs, OccluderHorizonCulling.cs
// (see ksp/commnet/comm_network.hpp).
#include "ksp/commnet/comm_network.hpp"

#include <sstream>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/commnet/commnet_scene.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"

namespace ksp::CommNet {

namespace {
double Evaluate(const DoubleCurve* c, double t) {
    if (c == nullptr) throw NullReferenceException();
    return c->Evaluate(t);
}
}  // namespace

// ---- NodeUtilities.cs --------------------------------------------------------------------------------------------------------

// NodeUtilities.cs:5
SignalStrength NodeUtilities::ConvertSignalStrength(double signalStrength) {
    if (signalStrength > 0.75) return SignalStrength::Green;
    if (signalStrength > 0.5) return SignalStrength::Yellow;
    if (signalStrength > 0.25) return SignalStrength::Orange;
    if (signalStrength > 1E-09) return SignalStrength::Red;
    return SignalStrength::None;
}

// ---- CommRangeModel.cs -------------------------------------------------------------------------------------------------------

// CommRangeModel.cs:7
bool CommRangeModel::InRange(double aPower, double bPower, double sqrDistance) {
    const double num = aPower * bPower;
    if (sqrDistance <= num) return num > 0.0;
    return false;
}

// CommRangeModel.cs:17
double CommRangeModel::GetNormalizedRange(double aPower, double bPower, double distance) {
    return 1.0 - distance / dotnet::Math::Sqrt(aPower * bPower);
}

// CommRangeModel.cs:22
double CommRangeModel::GetMaximumRange(double aPower, double bPower) { return dotnet::Math::Sqrt(aPower * bPower); }

// ---- CommNode.cs -------------------------------------------------------------------------------------------------------------

// CommNode.cs:102 (`transform.position`; NullReferenceException without a transform)
Vector3d CommNode::position() const {
    if (transform == nullptr) throw NullReferenceException();
    return Vector3d(transform->position);
}

// CommNode.cs:113
void CommNode::NetworkPreUpdate() {
    if (transform != nullptr) position_ = Vector3d(transform->position);
    if (OnNetworkPreUpdate) OnNetworkPreUpdate();
}

// CommNode.cs:125
void CommNode::NetworkPostUpdate() {
    if (OnNetworkPostUpdate) OnNetworkPostUpdate();
}

// CommNode.cs:133
double CommNode::GetSignalStrengthMultiplier(CommNode* b) {
    if (OnLinkCreateSignalModifier) return OnLinkCreateSignalModifier(b);
    return 1.0;
}

// CommNode.cs:142 (Node.ToString passes displayName through Localizer.Format)
std::string CommNode::ToString() {
    return "Node: " + Localizer::Format(displayName()) + " Links=" + std::to_string(Count()) + " : " + (isHome ? "Home " : std::string()) + " " +
           (isControlSource ? "Control " : std::string()) + " " + (isControlSourceMultiHop ? "MultiHop " : std::string());
}

// ---- CommLink.cs -------------------------------------------------------------------------------------------------------------

CommNode* CommLink::A() const { return static_cast<CommNode*>(a); }
CommNode* CommLink::B() const { return static_cast<CommNode*>(b); }

// CommLink.cs:38
void CommLink::Set(CommNode* a_, CommNode* b_, double distance, double signalStrength_) {
    this->a = a_;
    this->b = b_;
    cost = distance;
    Update(signalStrength_);
}

// CommLink.cs:46
void CommLink::Update(double cost_) {
    set_signalStrength(cost_);
    signal = NodeUtilities::ConvertSignalStrength(signalStrength());
}

// CommLink.cs:52
double CommLink::GetSignalStrength(bool aMustRelay, bool bMustRelay) const {
    if (aMustRelay) {
        if (!bMustRelay && strengthRR <= strengthAR) {
            if (strengthAR == 0.0) return strengthBR;
            return strengthAR;
        }
        return strengthRR;
    }
    if (strengthRR > strengthBR) return strengthRR;
    if (strengthBR == 0.0) return strengthAR;
    return strengthBR;
}

// CommLink.cs:77
double CommLink::GetSignalStrength(const CommNode* pathStartNode) const { return GetSignalStrength(a != pathStartNode, b != pathStartNode); }

// CommLink.cs:82
double CommLink::GetBestSignal() const { return dotnet::Math::Max(dotnet::Math::Max(strengthAR, strengthBR), strengthRR); }

// ---- CommPath.cs -------------------------------------------------------------------------------------------------------------

// CommPath.cs:21
bool CommPath::IsLastHopHome() const {
    if (!last_) throw NullReferenceException();
    return last_->B()->isHome;
}

// CommPath.cs:26
void CommPath::Clear() {
    ClearList();
    commType_ = VesselControlState::None;
    signalStrength_ = 0.0;
    signal_ = SignalStrength::None;
    first_ = nullptr;
    last_ = nullptr;
}

// CommPath.cs:36
void CommPath::UpdateFromPath() {
    const int count = Count();
    if (count < 1) {
        Clear();
        return;
    }
    first_ = LinkAt(0);
    last_ = LinkAt(count - 1);
    signalStrength_ = 1.0;
    for (int i = 0; i < count; i++) {
        std::shared_ptr<CommLink> commLink = LinkAt(i);
        signalStrength_ *= commLink->signalStrength();
    }
    last_->hopType = ((!last_->B()->isHome) ? HopType::ControlPoint : HopType::Home);
    signal_ = NodeUtilities::ConvertSignalStrength(signalStrength_);
}

// CommPath.cs:56
std::string CommPath::ToString() const {
    if (Count() == 0) return std::string();
    std::string text = (*this)[0]->start()->name();
    for (int i = 0, count = Count(); i < count; i++) text = text + ";" + (*this)[i]->end()->name();
    return text;
}

// ---- OccluderHorizonCulling.cs -----------------------------------------------------------------------------------------------

// OccluderHorizonCulling.cs:24
OccluderHorizonCulling::OccluderHorizonCulling(UnityEngine::Transform* transform, double radiusX, double radiusY, double radiusZ,
                                               CelestialBody* bodyComponent) {
    this->transform_ = transform;
    if (radiusX != 0.0 && radiusY != 0.0 && radiusZ != 0.0) {
        anyZero_ = false;
        radiusXRecip_ = 1.0 / radiusX;
        radiusYRecip_ = 1.0 / radiusY;
        radiusZRecip_ = 1.0 / radiusZ;
        radius = dotnet::Math::Max(dotnet::Math::Max(radiusX, radiusY), radiusZ);
        body_ = bodyComponent;  // transform.GetComponent<CelestialBody>()
        if (body_ != nullptr) useBody_ = true;
    } else {
        anyZero_ = true;
    }
}

// OccluderHorizonCulling.cs:46
bool OccluderHorizonCulling::InRange(const Vector3d& source, double distance) {
    if (anyZero_) return false;
    return Occluder::InRange(source, distance);
}

// OccluderHorizonCulling.cs:55
bool OccluderHorizonCulling::Raycast(const Vector3d& source_, const Vector3d& dest_) {
    if (anyZero_) return false;
    Vector3d source = source_;
    Vector3d dest = dest_;
    source = source - position;
    dest = dest - position;
    const double sqrMagnitude = source.sqrMagnitude();
    const double sqrMagnitude2 = dest.sqrMagnitude();
    if (sqrMagnitude > sqrMagnitude2) {
        Vector3d vector3d = source;
        source = dest;
        dest = vector3d;
    }
    source = invRotation_ * source;
    dest = invRotation_ * dest;
    source.x *= radiusXRecip_;
    source.y *= radiusYRecip_;
    source.z *= radiusZRecip_;
    dest.x *= radiusXRecip_;
    dest.y *= radiusYRecip_;
    dest.z *= radiusZRecip_;
    const double num = source.sqrMagnitude() - 1.0;
    const Vector3d lhs = dest - source;
    const double num2 = 0.0 - Vector3d::Dot(lhs, source);
    if (num < 0.0) return num2 > 0.0;
    const double sqrMagnitude3 = lhs.sqrMagnitude();
    if (num2 > num && num2 * num2 / sqrMagnitude3 > num) return true;
    return false;
}

// OccluderHorizonCulling.cs:94
void OccluderHorizonCulling::Update() {
    if (useBody_) {
        position = body_->position;
    } else {
        if (transform_ == nullptr) throw NullReferenceException();
        position = Vector3d(transform_->position);
    }
    if (transform_ == nullptr) throw NullReferenceException();
    invRotation_ = QuaternionD::Inverse(QuaternionD(transform_->rotation));
}

// ---- CommNetwork.cs ----------------------------------------------------------------------------------------------------------

std::shared_ptr<Network::Link> CommNetwork::NewLink() { return std::make_shared<CommLink>(); }

// CommNetwork.cs:14
void CommNetwork::Rebuild() {
    isDirty_ = false;
    Network::Net::Rebuild();
}

// CommNetwork.cs:20
bool CommNetwork::FindHome(CommNode* from, CommPath* path) {
    const WhereFn where = [](Network::Node* s, Network::Node* a) { return IsHome(static_cast<CommNode*>(s), static_cast<CommNode*>(a)); };
    if (FindClosestWhere(from, path, where) != nullptr) return true;
    return false;
}

// CommNetwork.cs:29
bool CommNetwork::FindClosestControlSource(CommNode* from, CommPath* path) {
    const WhereFn where = [](Network::Node* s, Network::Node* a) { return IsControlSource(static_cast<CommNode*>(s), static_cast<CommNode*>(a)); };
    if (FindClosestWhere(from, path, where) != nullptr) return true;
    return false;
}

// CommNetwork.cs:38
bool CommNetwork::FindPath(Network::Node* start_, Network::Path* path_, Network::Node* end_) {
    CommNode* start = static_cast<CommNode*>(start_);
    CommPath* path = static_cast<CommPath*>(path_);
    CommNode* end = static_cast<CommNode*>(end_);
    if (path != nullptr) path->Clear();
    if (end == nullptr) return false;
    if (start == end) return false;
    if (isDirty_) Rebuild();
    CreateShortestPathTree(start, end);
    CommNode* commNode = end;
    if (commNode->bestLinkNode != nullptr) {
        while (commNode != start) {
            CommNode* b = commNode;
            CommLink* bestLink = static_cast<CommLink*>(commNode->bestLink);
            commNode = static_cast<CommNode*>(commNode->bestLinkNode);
            auto commLink = std::make_shared<CommLink>();
            commLink->Set(commNode, b, bestLink->signalStrength(), bestLink->GetSignalStrength(start));
            if (path == nullptr) throw NullReferenceException();
            path->Insert(0, commLink);
        }
    }
    if (path == nullptr) throw NullReferenceException();
    if (path->Count() == 0) return false;
    if (!(*path)[0]->Contains(start)) {
        path->Clear();
        return false;
    }
    path->UpdateFromPath();
    return true;
}

// CommNetwork.cs:83
Network::Node* CommNetwork::FindClosestWhere(Network::Node* start_, Network::Path* path_, const WhereFn& where) {
    CommNode* start = static_cast<CommNode*>(start_);
    CommPath* path = static_cast<CommPath*>(path_);
    if (path != nullptr) path->Clear();
    if (start != nullptr && where) {
        if (isDirty_) Rebuild();
        double num = -1.7976931348623157E+308;  // double.MinValue
        CommNode* commNode = nullptr;
        IncrementPathingID();
        int count = static_cast<int>(nodes_.size());
        CommNode* commNode2;
        while (count-- > 0) {
            commNode2 = static_cast<CommNode*>((*this)[count]);
            commNode2->bestCost = -1.7976931348623157E+308;
            commNode2->bestLink = nullptr;
            commNode2->bestLinkNode = nullptr;
            commNode2->isInCandidateList = false;
        }
        candidates_.clear();
        candidates_.push_back(start);
        start->bestCost = 1.0;
        start->isInCandidateList = true;
        start->pathingID = pathingID();
        while (!candidates_.empty()) {
            commNode2 = static_cast<CommNode*>(candidates_.front());
            candidates_.pop_front();
            commNode2->isInCandidateList = false;
            for (const auto& kv : commNode2->links()) {
                Network::Link* value = kv.second.get();
                if (value->pathingID != pathingID()) {
                    value->pathingID = pathingID();
                    CommNode* commNode3 = static_cast<CommNode*>(UpdateShortestWhere(commNode2, kv.first, value, commNode2->bestCost, start, where));
                    if (commNode3 != nullptr && commNode3->bestCost > num) {
                        commNode = commNode3;
                        num = commNode3->bestCost;
                    }
                }
            }
        }
        if (commNode == nullptr) return nullptr;
        commNode2 = commNode;
        if (path != nullptr) {
            path->Clear();
            if (commNode2->bestLinkNode != nullptr) {
                while (commNode2 != start) {
                    CommNode* b = commNode2;
                    CommLink* value = static_cast<CommLink*>(commNode2->bestLink);
                    commNode2 = static_cast<CommNode*>(commNode2->bestLinkNode);
                    auto commLink = std::make_shared<CommLink>();
                    commLink->Set(commNode2, b, value->signalStrength(), value->GetSignalStrength(start));
                    path->Insert(0, commLink);
                }
            }
            if (!(*path)[0]->Contains(start)) {
                path->Clear();
                return nullptr;
            }
            path->UpdateFromPath();
            return commNode;
        }
        return commNode;
    }
    return nullptr;
}

// CommNetwork.cs:167
Network::Node* CommNetwork::Add(Network::Node* conn) {
    if (Network::Net::Add(conn) == nullptr) return nullptr;
    isDirty_ = true;
    return conn;
}

// CommNetwork.cs:177
bool CommNetwork::Remove(Network::Node* conn) {
    if (!Network::Net::Remove(conn)) return false;
    isDirty_ = true;
    return true;
}

// CommNetwork.cs:187
bool CommNetwork::TryConnect(CommNode* a, CommNode* b, double distance, bool aCanRelay, bool bCanRelay, bool bothRelay) {
    bool flag = false;
    const double num = a->GetSignalStrengthMultiplier(b) * b->GetSignalStrengthMultiplier(a);
    double num2;
    IRangeModel& range = CommNetScenario::RangeModel();
    if (bothRelay) {
        const double normalizedRange = range.GetNormalizedRange(a->antennaRelay.power, b->antennaRelay.power, distance);
        if (normalizedRange > 0.0) {
            num2 = dotnet::Math::Sqrt(Evaluate(a->antennaRelay.rangeCurve, normalizedRange) * Evaluate(b->antennaRelay.rangeCurve, normalizedRange));
            num2 *= num;
            if (num2 > 0.0) flag = true;
        } else {
            bothRelay = false;
            num2 = 0.0;
        }
    } else {
        num2 = 0.0;
    }
    double num3;
    if (aCanRelay) {
        const double normalizedRange2 = range.GetNormalizedRange(a->antennaRelay.power, b->antennaTransmit.power, distance);
        if (normalizedRange2 > 0.0) {
            num3 = dotnet::Math::Sqrt(Evaluate(a->antennaRelay.rangeCurve, normalizedRange2) * Evaluate(b->antennaTransmit.rangeCurve, normalizedRange2));
            num3 *= num;
            if (num3 > 0.0) flag = true;
        } else {
            aCanRelay = false;
            num3 = 0.0;
        }
    } else {
        aCanRelay = false;
        num3 = 0.0;
    }
    double num4;
    if (bCanRelay) {
        const double normalizedRange3 = range.GetNormalizedRange(a->antennaTransmit.power, b->antennaRelay.power, distance);
        if (normalizedRange3 > 0.0) {
            num4 = dotnet::Math::Sqrt(Evaluate(b->antennaRelay.rangeCurve, normalizedRange3) * Evaluate(a->antennaTransmit.rangeCurve, normalizedRange3));
            num4 *= num;
            if (num4 > 0.0) flag = true;
        } else {
            bCanRelay = false;
            num4 = 0.0;
        }
    } else {
        bCanRelay = false;
        num4 = 0.0;
    }
    if (flag) {
        std::shared_ptr<Network::Link> link = Connect(a, b, distance);
        CommLink* commLink = static_cast<CommLink*>(link.get());
        commLink->strengthRR = num2;
        commLink->strengthAR = num3;
        commLink->strengthBR = num4;
        commLink->aCanRelay = aCanRelay;
        commLink->bCanRelay = bCanRelay;
        commLink->bothRelay = bothRelay;
        return true;
    }
    Disconnect(a, b);
    return false;
}

// CommNetwork.cs:277
bool CommNetwork::SetNodeConnection(Network::Node* a_, Network::Node* b_) {
    CommNode* a = static_cast<CommNode*>(a_);
    CommNode* b = static_cast<CommNode*>(b_);
    if (a->isHome && b->isHome) {
        Disconnect(a, b);
        return false;
    }
    if (a->antennaRelay.power + a->antennaTransmit.power != 0.0 && b->antennaRelay.power + b->antennaTransmit.power != 0.0) {
        const Vector3d precisePosition = a->precisePosition();
        const Vector3d precisePosition2 = b->precisePosition();
        double num = (precisePosition2 - precisePosition).sqrMagnitude();
        double num2 = a->distanceOffset + b->distanceOffset;
        if (num2 != 0.0) {
            num2 = dotnet::Math::Sqrt(num) + num2;
            if (num2 > 0.0) {
                num = num2 * num2;
            } else {
                num2 = 0.0;
                num = 0.0;
            }
        }
        IRangeModel& range = CommNetScenario::RangeModel();
        bool flag;
        bool flag2 = (flag = range.InRange(a->antennaRelay.power, b->antennaRelay.power, num));
        bool flag3 = flag;
        if (!flag) {
            flag2 = range.InRange(a->antennaRelay.power, b->antennaTransmit.power, num);
            flag3 = range.InRange(a->antennaTransmit.power, b->antennaRelay.power, num);
        }
        if (!flag2 && !flag3) {
            Disconnect(a, b);
            return false;
        }
        if (num == 0.0 && (flag || flag2 || flag3)) return TryConnect(a, b, 1E-07, flag2, flag3, flag);
        if (num2 == 0.0) num2 = dotnet::Math::Sqrt(num);
        if (TestOcclusion(precisePosition, a->occluder(), precisePosition2, b->occluder(), num2)) return TryConnect(a, b, num2, flag2, flag3, flag);
        Disconnect(a, b);
        return false;
    }
    Disconnect(a, b);
    return false;
}

// CommNetwork.cs:335
bool CommNetwork::TestOcclusion(const Vector3d& aPos, Occluder* a, const Vector3d& bPos, Occluder* b, double distance) {
    int count = static_cast<int>(occluders_.size());
    Occluder* occ;
    do {
        if (count-- > 0) {
            occ = occluders_[static_cast<size_t>(count)];
            continue;
        }
        return true;
    } while (occ == a || occ == b || !occ->InRange(aPos, distance) || !occ->Raycast(aPos, bPos));
    return false;
}

// CommNetwork.cs:352
bool CommNetwork::IsControlSource(CommNode* start, CommNode* a) {
    if (!a->isControlSourceMultiHop) {
        if (a->isControlSource) return a->ContainsKey(start);
        return false;
    }
    return a->isControlSource;
}

// CommNetwork.cs:365
bool CommNetwork::IsHome(CommNode*, CommNode* a) { return a->isHome; }

// CommNetwork.cs:370
void CommNetwork::CreateShortestPathTree(Network::Node* start_, Network::Node* end) {
    CommNode* start = static_cast<CommNode*>(start_);
    IncrementPathingID();
    int count = static_cast<int>(nodes_.size());
    while (count-- > 0) {
        CommNode* commNode = static_cast<CommNode*>((*this)[count]);
        commNode->bestCost = -1.7976931348623157E+308;
        commNode->bestLinkNode = nullptr;
        commNode->isInCandidateList = false;
        commNode->bestLink = nullptr;
    }
    candidates_.clear();
    candidates_.push_back(start);
    start->bestCost = 1.0;
    start->isInCandidateList = true;
    start->pathingID = pathingID();
    while (!candidates_.empty()) {
        CommNode* commNode = static_cast<CommNode*>(candidates_.front());
        candidates_.pop_front();
        commNode->isInCandidateList = false;
        for (const auto& kv : commNode->links()) UpdateShortestPath(commNode, kv.first, kv.second.get(), commNode->bestCost, start, end);
    }
}

// CommNetwork.cs:400
void CommNetwork::UpdateShortestPath(Network::Node* a, Network::Node* b, Network::Link* link, double bestCost, Network::Node* startNode, Network::Node*) {
    double num = 0.0;
    num = static_cast<CommLink*>(link)->GetSignalStrength(static_cast<CommNode*>(startNode));
    const double num2 = bestCost * num;
    if (num > 0.0 && num2 > b->bestCost) {
        b->bestCost = num2;
        b->bestLinkNode = a;
        b->bestLink = link;
        if (!b->isInCandidateList && b->pathingID != pathingID()) {
            candidates_.push_back(b);
            b->isInCandidateList = true;
            b->pathingID = pathingID();
        }
    }
}

// CommNetwork.cs:419
Network::Node* CommNetwork::UpdateShortestWhere(Network::Node* a, Network::Node* b, Network::Link* link, double bestCost, Network::Node* startNode,
                                                const WhereFn& whereClause) {
    double num = 0.0;
    num = static_cast<CommLink*>(link)->GetSignalStrength(static_cast<CommNode*>(startNode));
    const double num2 = bestCost * num;
    if (num > 0.0 && num2 > b->bestCost) {
        b->bestCost = num2;
        b->bestLinkNode = a;
        b->bestLink = link;
        if (!b->isInCandidateList && b->pathingID != pathingID()) {
            candidates_.push_back(b);
            b->isInCandidateList = true;
            b->pathingID = pathingID();
        }
        if (whereClause(startNode, b)) return b;
        return nullptr;
    }
    return nullptr;
}

// CommNetwork.cs:444
std::string CommNetwork::CreateDebug() {
    std::ostringstream sb;
    for (int i = 0; i < Count(); i++) {
        sb << NodeAt(i)->ToString() << "\n";
        for (const auto& kv : nodes_[static_cast<size_t>(i)]->links()) {
            CommLink* l = static_cast<CommLink*>(kv.second.get());
            sb << "      " << "Link: " << l->start()->displayName() << " -to- " << l->end()->displayName() << " : " << l->signalStrength() << "\n";
        }
    }
    return sb.str();
}

}  // namespace ksp::CommNet
