// WP-39: CommNet/Network/Net.cs, Node.cs, Link.cs, Path.cs, CommNet/Occluder.cs (see ksp/commnet/network.hpp).
#include "ksp/commnet/network.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"

namespace ksp::CommNet {

// Occluder.cs:9
bool Occluder::InRange(const Vector3d& source, double distance) {
    double num = distance + radius;
    num *= num;
    return (source - position).sqrMagnitude() <= num;
}

namespace Network {

// ---- Link.cs --------------------------------------------------------------------------------------------------------------

// Link.cs:21
void Link::Set(Node* a_, Node* b_, double cost_) {
    this->a = a_;
    this->b = b_;
    Update(cost_);
    this->cost = cost_;
}

// Link.cs:29
void Link::Update(double cost_) { this->cost = cost_; }

// Link.cs:38
Node* Link::OtherEnd(const Node* toNode) const {
    if (a == toNode) return b;
    if (b == toNode) return a;
    return nullptr;
}

// Link.cs:51
bool Link::Contains(const Node* node) const {
    if (a != node) return b == node;
    return true;
}

// Link.cs:60
void Link::GetPoints(std::vector<Vector3f>* points) const {
    if (points != nullptr) {
        if (2 != static_cast<int>(points->size())) {
            points->clear();
            points->push_back(a->position().ToVector3f());
            points->push_back(b->position().ToVector3f());
        } else {
            (*points)[0] = a->position().ToVector3f();
            (*points)[1] = b->position().ToVector3f();
        }
    }
}

// ---- Node.cs --------------------------------------------------------------------------------------------------------------

// Node.cs:71 (as written: the add branch registers the OLD occluder, then the new one is stored)
void Node::SetOccluder(Occluder* occ) {
    if (occ == nullptr) {
        if (net_ != nullptr) net_->Remove(this->occluder_);
        this->occluder_ = nullptr;
        return;
    }
    if (net_ != nullptr) {
        if (this->occluder_ != nullptr) net_->Remove(this->occluder_);
        net_->Add(this->occluder_);
    }
    this->occluder_ = occ;
}

// Node.cs:98
void Node::GetLinkPoints(std::vector<Vector3f>* discreteLines) {
    if (discreteLines == nullptr) return;
    const int num = Count();
    if (num == 0) {
        discreteLines->clear();
        return;
    }
    if (num * 2 != static_cast<int>(discreteLines->size())) {
        discreteLines->clear();
        for (const auto& kv : links_) {
            discreteLines->push_back(kv.second->start()->position().ToVector3f());
            discreteLines->push_back(kv.second->end()->position().ToVector3f());
        }
        return;
    }
    int num2 = 0;
    for (const auto& kv : links_) {
        (*discreteLines)[static_cast<size_t>(num2)] = kv.second->start()->position().ToVector3f();
        (*discreteLines)[static_cast<size_t>(++num2)] = kv.second->end()->position().ToVector3f();
        num2++;
    }
}

// ---- Path.cs --------------------------------------------------------------------------------------------------------------

// Path.cs:8
Path::LinkPtr Path::First() const {
    if (list_.empty()) return nullptr;
    return list_[0];
}

// Path.cs:20
Path::LinkPtr Path::Last() const {
    if (list_.empty()) return nullptr;
    return list_.back();
}

// Path.cs:37
void Path::CopyTo(Path& path, bool deepCopy, const std::function<LinkPtr()>& newLink) const {
    path.ClearList();
    if (deepCopy) {
        for (size_t i = 0, count = list_.size(); i < count; i++) {
            const LinkPtr& val = list_[i];
            LinkPtr val2 = newLink();
            val2->Set(val->a, val->b, val->cost);
            path.Add(val2);
        }
    } else {
        path.AddRange(*this);
    }
}

// Path.cs:69
void Path::GetPoints(std::vector<Vector3f>* points, bool discrete) const {
    if (points == nullptr) return;
    const int count = Count();
    if (count == 0) {
        points->clear();
    } else if (discrete) {
        if (count * 2 != static_cast<int>(points->size())) {
            points->clear();
            for (int i = 0; i < count; i++) {
                points->push_back(list_[static_cast<size_t>(i)]->start()->position().ToVector3f());
                points->push_back(list_[static_cast<size_t>(i)]->end()->position().ToVector3f());
            }
            return;
        }
        int num = 0, num2 = 0;
        while (num < count) {
            (*points)[static_cast<size_t>(num2)] = list_[static_cast<size_t>(num)]->start()->position().ToVector3f();
            (*points)[static_cast<size_t>(++num2)] = list_[static_cast<size_t>(num)]->end()->position().ToVector3f();
            num++;
            num2++;
        }
    } else if (count + 1 != static_cast<int>(points->size())) {
        points->clear();
        for (int j = 0; j < count; j++) points->push_back(list_[static_cast<size_t>(j)]->start()->position().ToVector3f());
        points->push_back(list_[static_cast<size_t>(count - 1)]->end()->position().ToVector3f());
    } else {
        for (int k = 0; k < count; k++) (*points)[static_cast<size_t>(k)] = list_[static_cast<size_t>(k)]->start()->position().ToVector3f();
        (*points)[static_cast<size_t>(count)] = list_[static_cast<size_t>(count - 1)]->end()->position().ToVector3f();
    }
}

// Path.cs operator == (:121)
bool Path::Equal(const Path* a, const Path* b) {
    if (a == b) return true;
    if (a != nullptr && b != nullptr) {
        if (a->Count() != b->Count()) return false;
        for (int i = 0, count = a->Count(); i < count; i++) {
            const LinkPtr& val = (*a)[i];
            for (int j = 0, count2 = b->Count(); j < count2; j++) {
                const LinkPtr& val2 = (*b)[j];
                if (val->start() != val2->start() && val->end() != val2->end()) return false;
            }
        }
        return true;
    }
    return false;
}

// ---- Net.cs ---------------------------------------------------------------------------------------------------------------

// Net.cs:39
bool Net::Contains(Node* conn) const { return std::find(nodes_.begin(), nodes_.end(), conn) != nodes_.end(); }

// Net.cs:116
bool Net::Contains(Occluder* conn) const { return std::find(occluders_.begin(), occluders_.end(), conn) != occluders_.end(); }

// Net.cs:44
Node* Net::Add(Node* conn) {
    if (conn == nullptr) return nullptr;
    if (!Contains(conn)) {
        conn->SetNet(this);
        nodes_.push_back(conn);
    }
    if (conn->occluder() != nullptr) Add(conn->occluder());
    conn->Clear();
    return conn;
}

// Net.cs:63
bool Net::Remove(Node* conn) {
    if (conn == nullptr) return false;
    if (conn->occluder() != nullptr) Remove(conn->occluder());
    // nodeLinkEnum = conn.GetEnumerator(): Disconnect(.., removeFromA: false) only mutates the other end's dictionary
    for (const auto& kv : conn->links()) {
        Link* value = kv.second.get();
        Disconnect(conn, value->OtherEnd(conn), false);
    }
    conn->Clear();
    auto it = std::find(nodes_.begin(), nodes_.end(), conn);
    if (it == nodes_.end()) return false;
    nodes_.erase(it);
    return true;
}

// Net.cs:85
std::shared_ptr<Link> Net::Connect(Node* a, Node* b, double distance) {
    std::shared_ptr<Link> value;
    if (a->TryGetValue(b, value)) {
        value->Update(distance);
    } else {
        value = NewLink();
        value->Set(a, b, distance);
        a->Add(b, value);
        b->Add(a, value);
        links_.push_back(value);
    }
    return value;
}

// Net.cs:102
void Net::Disconnect(Node* a, Node* b, bool removeFromA) {
    std::shared_ptr<Link> value;
    if (a != nullptr && b != nullptr && (a->TryGetValue(b, value) || b->TryGetValue(a, value))) {
        value->OnDestroy();
        b->Remove(a);
        if (removeFromA) a->Remove(b);
        auto it = std::find(links_.begin(), links_.end(), value);
        if (it != links_.end()) links_.erase(it);
    }
}

// Net.cs:121
Occluder* Net::Add(Occluder* conn) {
    if (!Contains(conn)) occluders_.push_back(conn);
    return conn;
}

// Net.cs:130
bool Net::Remove(Occluder* conn) {
    auto it = std::find(occluders_.begin(), occluders_.end(), conn);
    if (it == occluders_.end()) return false;
    occluders_.erase(it);
    return true;
}

// Net.cs:135
void Net::IncrementPathingID() {
    if (pathingID_ == 0x7fffffff) pathingID_ = static_cast<int>(0x80000000u);
    pathingID_++;
}

// Net.cs:144
void Net::Rebuild() {
    if (OnNetworkPreUpdate) OnNetworkPreUpdate();
    PreUpdateNodes();
    UpdateOccluders();
    UpdateNetwork();
    PostUpdateNodes();
    if (OnNetworkPostUpdate) OnNetworkPostUpdate();
}

// Net.cs:160
void Net::PreUpdateNodes() {
    for (int i = 0, count = static_cast<int>(nodes_.size()); i < count; i++) nodes_[static_cast<size_t>(i)]->NetworkPreUpdate();
}

// Net.cs:169
void Net::UpdateOccluders() {
    for (int i = 0, count = static_cast<int>(occluders_.size()); i < count; i++) occluders_[static_cast<size_t>(i)]->Update();
}

// Net.cs:178
void Net::UpdateNetwork() {
    const int count = static_cast<int>(nodes_.size());
    for (int i = 0, num = count - 1; i < num; i++) {
        Node* connA = nodes_[static_cast<size_t>(i)];
        for (int j = i + 1; j < count; j++) {
            Node* connB = (*this)[j];
            SetNodeConnection(connA, connB);
        }
    }
}

// Net.cs:193
void Net::PostUpdateNodes() {
    for (int i = 0, count = static_cast<int>(nodes_.size()); i < count; i++) nodes_[static_cast<size_t>(i)]->NetworkPostUpdate();
}

// Net.cs:202
bool Net::FindPath(Node* start, Path* path, Node* end) {
    if (path != nullptr) path->ClearList();
    if (end == nullptr) return false;
    if (start == end) return false;
    CreateShortestPathTree(start, end);
    Node* val = end;
    if (path == nullptr) throw NullReferenceException();
    {
        auto l = NewLink();
        l->a = end;
        l->cost = 0.0;
        path->Add(l);
    }
    if (val->bestLinkNode != nullptr) {
        while (val != start) {
            val = val->bestLinkNode;
            auto val2 = NewLink();
            val2->Set(val, nullptr, val->bestCost);
            path->Add(val2);
        }
    }
    if (path->Count() == 0) return false;
    if (!(*path)[0]->Contains(start)) {
        path->ClearList();
        return false;
    }
    path->UpdateFromPath();
    return true;
}

// Net.cs:246
Node* Net::FindClosestWhere(Node* start, Path* path, const WhereFn& where) {
    if (path != nullptr) path->ClearList();
    int count = static_cast<int>(nodes_.size());
    Node* val;
    while (count-- > 0) {
        val = (*this)[count];
        val->bestCost = 1.7976931348623157E+308;
        val->bestLinkNode = nullptr;
        val->isInCandidateList = false;
    }
    Node* val2 = nullptr;
    double num = 1.7976931348623157E+308;
    candidates_.push_back(start);
    start->bestCost = 0.0;
    start->isInCandidateList = true;
    while (!candidates_.empty()) {
        val = candidates_.front();
        candidates_.pop_front();
        val->isInCandidateList = false;
        val->pathingID = pathingID();
        for (const auto& kv : val->links()) {
            Node* val3 = UpdateShortestWhere(val, kv.first, kv.second.get(), val->bestCost + kv.second->cost, start, where);
            if (val3 != nullptr && val3->bestCost < num) {
                val2 = val3;
                num = val3->bestCost;
            }
        }
    }
    if (val2 == nullptr) return nullptr;
    val = val2;
    if (path != nullptr) {
        path->ClearList();
        auto val4 = NewLink();
        val4->Set(val, nullptr, 0.0);
        path->Add(val4);
        if (val->bestLinkNode != nullptr) {
            while (val != start) {
                val = val->bestLinkNode;
                val4 = NewLink();
                val4->Set(val, nullptr, 0.0);
                path->Add(val4);
            }
        }
        if (!(*path)[0]->Contains(start)) {
            path->ClearList();
            return nullptr;
        }
        path->UpdateFromPath();
        return val2;
    }
    return val2;
}

// Net.cs:315
void Net::CreateShortestPathTree(Node* start, Node* end) {
    IncrementPathingID();
    int count = static_cast<int>(nodes_.size());
    while (count-- > 0) {
        Node* val = (*this)[count];
        val->bestCost = 1.7976931348623157E+308;
        val->bestLinkNode = nullptr;
        val->isInCandidateList = false;
    }
    candidates_.push_back(start);
    start->bestCost = 0.0;
    start->isInCandidateList = true;
    while (!candidates_.empty()) {
        Node* val = candidates_.front();
        candidates_.pop_front();
        val->isInCandidateList = false;
        val->pathingID = pathingID();
        for (const auto& kv : val->links()) UpdateShortestPath(val, kv.first, kv.second.get(), val->bestCost, start, end);
    }
}

// Net.cs:343
void Net::UpdateShortestPath(Node* node, Node* neighbor, Link* link, double bestCost, Node*, Node*) {
    if (bestCost < neighbor->bestCost) {
        neighbor->bestCost = bestCost;
        neighbor->bestLinkNode = node;
        neighbor->bestLink = link;
        if (!neighbor->isInCandidateList && neighbor->pathingID != pathingID()) {
            candidates_.push_back(neighbor);
            neighbor->isInCandidateList = true;
            neighbor->pathingID = pathingID();
        }
    }
}

// Net.cs:359
Node* Net::UpdateShortestWhere(Node* a, Node* b, Link* link, double bestDistance, Node* startNode, const WhereFn& whereClause) {
    if (bestDistance < b->bestCost) {
        b->bestCost = bestDistance;
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

// Net.cs:381
void Net::GetLinkPoints(std::vector<Vector3f>* discreteLines) {
    if (discreteLines == nullptr) return;
    const int count = static_cast<int>(links_.size());
    if (count == 0) {
        discreteLines->clear();
        return;
    }
    if (count * 2 != static_cast<int>(discreteLines->size())) {
        discreteLines->clear();
        for (int i = 0; i < count; i++) {
            discreteLines->push_back(links_[static_cast<size_t>(i)]->start()->position().ToVector3f());
            discreteLines->push_back(links_[static_cast<size_t>(i)]->end()->position().ToVector3f());
        }
        return;
    }
    int num = 0, num2 = 0;
    while (num < count) {
        (*discreteLines)[static_cast<size_t>(num2)] = links_[static_cast<size_t>(num)]->start()->position().ToVector3f();
        (*discreteLines)[static_cast<size_t>(++num2)] = links_[static_cast<size_t>(num)]->end()->position().ToVector3f();
        num++;
        num2++;
    }
}

}  // namespace Network
}  // namespace ksp::CommNet
