// WP-39: the generic network of CommNet: CommNet/Network/Net.cs, Node.cs, Link.cs, Path.cs and CommNet/Occluder.cs.
//
// The C# classes are CRTP generics (Net<_Net, _Data, _Link, _Path> ...) that only CommNet instantiates (CommNetwork, CommNode,
// CommLink, CommPath). Here they are concrete base classes and the derived CommNet classes override the virtual hooks; the
// downcasts live in comm_network.cpp. Behaviour that depends on the .NET collections is kept:
//   * Node derives from Dictionary<_Data, _Link>: its links enumerate in Dictionary order (insertion order, removed slots
//     refilled most-recently-freed first), which decides the order Dijkstra visits the neighbours and so every tie-break.
//     The port uses ksp::dotnet::Dictionary for that.
//   * Net.nodes / links / occluders are Lists with reference-equality Contains / Remove (first match).
//   * Path derives from List<_Link>; CommPath.Clear is `new virtual` (it hides List.Clear), so the generic Net code, which only
//     knows Path, calls the plain List clear and leaves commType / signalStrength / signal / first / last untouched.
//
// Seams: the display-only members (GetLinkPoints / GetPoints with Vector3 line lists, ToString with Localizer) are ported; a
// Node's `position` is whatever the owner supplies (CommNode: Transform.position).
#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/dotnet/collections.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp::CommNet {

namespace Network {
class Node;
class Link;
class Path;
class Net;
}  // namespace Network

// CommNet/Occluder.cs
class Occluder {
public:
    double radius = 0.0;
    Vector3d position;
    virtual ~Occluder() = default;
    virtual bool InRange(const Vector3d& source, double distance);
    virtual bool Raycast(const Vector3d& source, const Vector3d& dest) = 0;
    virtual void Update() {}
};

namespace Network {

// CommNet/Network/Link.cs
class Link {
public:
    Node* a = nullptr;
    Node* b = nullptr;
    double cost = 0.0;
    int pathingID = 0;

    virtual ~Link() = default;
    Node* start() const { return a; }
    Node* end() const { return b; }
    virtual void Set(Node* a_, Node* b_, double cost_);
    virtual void Update(double cost_);
    virtual void OnDestroy() {}
    Node* OtherEnd(const Node* toNode) const;
    bool Contains(const Node* node) const;
    virtual void GetPoints(std::vector<Vector3f>* points) const;
};

// CommNet/Network/Node.cs (derives from Dictionary<_Data, _Link>)
class Node {
public:
    using LinkPtr = std::shared_ptr<Link>;
    using Links = dotnet::Dictionary<Node*, LinkPtr>;

    bool isInCandidateList = false;
    int pathingID = 0;
    double bestCost = 0.0;
    Link* bestLink = nullptr;
    Node* bestLinkNode = nullptr;

    virtual ~Node() = default;
    Net* GetNet() const { return net_; }                                  // the `Net` property
    virtual std::string name() const { return std::string(); }
    virtual std::string displayName() const { return std::string(); }
    virtual Vector3d position() const { return Vector3d(0.0, 0.0, 0.0); }
    Occluder* occluder() const { return occluder_; }
    virtual void SetNet(Net* network) { net_ = network; }
    virtual void NetworkPreUpdate() {}
    virtual void NetworkPostUpdate() {}
    void SetOccluder(Occluder* occ);
    virtual void GetLinkPoints(std::vector<Vector3f>* discreteLines);

    // ---- the Dictionary<_Data, _Link> base ----
    int Count() const { return links_.Count(); }
    bool TryGetValue(Node* key, LinkPtr& value) const { return links_.TryGetValue(key, value); }
    LinkPtr* Find(Node* key) { return links_.Find(key); }
    LinkPtr& At(Node* key) { return links_.At(key); }                    // indexer get (KeyNotFoundException)
    bool ContainsKey(Node* key) const { return links_.ContainsKey(key); }
    void Add(Node* key, LinkPtr value) { links_.Add(key, std::move(value)); }
    bool Remove(Node* key) { return links_.Remove(key); }
    void Clear() { links_.Clear(); }
    const Links& links() const { return links_; }

private:
    Net* net_ = nullptr;
    Occluder* occluder_ = nullptr;
    Links links_;
};

// CommNet/Network/Path.cs (derives from List<_Link>)
class Path {
public:
    using LinkPtr = std::shared_ptr<Link>;
    virtual ~Path() = default;

    int Count() const { return static_cast<int>(list_.size()); }
    LinkPtr& operator[](int i) { return list_.at(static_cast<size_t>(i)); }
    const LinkPtr& operator[](int i) const { return list_.at(static_cast<size_t>(i)); }
    void Add(LinkPtr l) { list_.push_back(std::move(l)); }
    void Insert(int index, LinkPtr l) { list_.insert(list_.begin() + index, std::move(l)); }
    void ClearList() { list_.clear(); }              // List<_Link>.Clear (what Net.cs calls)
    void AddRange(const Path& p) { list_.insert(list_.end(), p.list_.begin(), p.list_.end()); }
    const std::vector<LinkPtr>& list() const { return list_; }

    virtual LinkPtr First() const;
    virtual LinkPtr Last() const;
    virtual bool IsEmpty() const { return list_.empty(); }
    virtual void CopyTo(Path& path, bool deepCopy, const std::function<LinkPtr()>& newLink) const;
    virtual void UpdateFromPath() {}
    virtual void GetPoints(std::vector<Vector3f>* points, bool discrete = true) const;
    // operator==(Path, Path) as written (compares start / end of the first path's links against every link of the second)
    static bool Equal(const Path* a, const Path* b);

private:
    std::vector<LinkPtr> list_;
};

// CommNet/Network/Net.cs
class Net {
public:
    virtual ~Net() = default;
    std::function<void()> OnNetworkPreUpdate;
    std::function<void()> OnNetworkPostUpdate;

    Node* operator[](int i) const { return nodes_.at(static_cast<size_t>(i)); }
    int Count() const { return static_cast<int>(nodes_.size()); }
    const std::vector<std::shared_ptr<Link>>& Links() const { return links_; }
    int OccludersCount() const { return static_cast<int>(nodes_.size()); }  // (sic) Net.cs returns nodes.Count
    bool Contains(Node* conn) const;
    bool Contains(Occluder* conn) const;

    virtual Node* Add(Node* conn);
    virtual bool Remove(Node* conn);
    virtual Occluder* Add(Occluder* conn);
    virtual bool Remove(Occluder* conn);
    virtual void Rebuild();
    virtual bool FindPath(Node* start, Path* path, Node* end);
    using WhereFn = std::function<bool(Node*, Node*)>;
    virtual Node* FindClosestWhere(Node* start, Path* path, const WhereFn& where);
    virtual void GetLinkPoints(std::vector<Vector3f>* discreteLines);

protected:
    std::vector<Node*> nodes_;
    std::vector<std::shared_ptr<Link>> links_;
    std::vector<Occluder*> occluders_;
    int pathingID_ = static_cast<int>(0x80000000u);  // int.MinValue
    std::deque<Node*> candidates_;

    int pathingID() const { return pathingID_; }
    virtual bool SetNodeConnection(Node* connA, Node* connB) = 0;
    virtual std::shared_ptr<Link> NewLink() = 0;     // `new _Link()`
    virtual std::shared_ptr<Link> Connect(Node* a, Node* b, double distance);
    virtual void Disconnect(Node* a, Node* b, bool removeFromA = true);
    void IncrementPathingID();
    virtual void PreUpdateNodes();
    virtual void UpdateOccluders();
    virtual void UpdateNetwork();
    virtual void PostUpdateNodes();
    virtual void CreateShortestPathTree(Node* start, Node* end);
    virtual void UpdateShortestPath(Node* node, Node* neighbor, Link* link, double bestCost, Node* startNode, Node* endNode);
    virtual Node* UpdateShortestWhere(Node* a, Node* b, Link* link, double bestDistance, Node* startNode, const WhereFn& whereClause);
};

}  // namespace Network
}  // namespace ksp::CommNet
