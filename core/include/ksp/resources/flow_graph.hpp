// RUI/Algorithms (WP-15): the resource flow graphs behind the crossfeed part sets.
//   Vertex.cs, FlowGraph.cs, StackFlowGraph.cs, StronglyConnectedComponentFinder.cs, SCCFlowGraph.cs, SCCFlowGraphUCFinder.cs
//
// StackFlowGraph builds two directed graphs over the parts of a ship (delivery: who can deliver to whom, request: who can
// request from whom) from fuelLookupTargets (fuel lines, docking ports), attach-node crossfeed and surface attachment.
// The strongly connected components of each graph (Tarjan, StronglyConnectedComponentFinder) are condensed into a DAG
// (SCCFlowGraph) and every component is given the superset of the parts reachable from it: that superset is the part set
// a request of any part in the component can draw from (PartSet.BuildPartSets).
//
// C++ mapping:
//   HashSet<Part>            -> dotnet::HashSet<Part*> held by shared_ptr (PartSet keeps the supersets of a discarded
//                               SCCFlowGraph; reference identity of the set object is the key of PartSet's setsHolder)
//   LinkedList<HashSet<Part>> -> std::list<shared_ptr<PartHashSet>>
//   Vertex.transformGuide / StackFlowGraph.BuildTransformGuides: not ported (they hold Unity Transforms of the fuel line
//     mesh, CModuleLinkedMesh: rendering).
//   StronglyConnectedComponentFinder's `stack.Contains(vertex)` is a linear search in C#; the vertex carries an on-stack flag
//     here (same result, O(1)).
#pragma once

#include <functional>
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/hash_set.hpp"

namespace ksp {

class Part;

using PartHashSet = dotnet::HashSet<Part*>;
using PartHashSetPtr = std::shared_ptr<PartHashSet>;

namespace RUI::Algorithms {

// Vertex.cs:6
template <class T>
class Vertex {
public:
    dotnet::HashSet<int> contained;  // :8
    int Index = -1;                  // :10 internal
    int LowLink = 0;                 // :12 internal
    T Value{};                       // :14
    std::vector<Vertex<T>*> Dependencies;  // :16
    bool onStack = false;            // (C++: replaces Stack.Contains)

    Vertex() = default;                                                // :20
    explicit Vertex(T value) : Value(std::move(value)) {}              // :25
};

// FlowGraph.cs:7
template <class T>
class FlowGraph {
public:
    std::vector<Vertex<T>*> graph;                      // :9
    dotnet::Dictionary<T, Vertex<T>*> lookup;           // :11
    FlowGraph() = default;                              // :12
    FlowGraph(const FlowGraph&) = delete;
    FlowGraph& operator=(const FlowGraph&) = delete;

    // :18 Add: a duplicate logs an error and returns null
    Vertex<T>* Add(const T& part) {
        if (lookup.ContainsKey(part)) {
            log::Error("[FlowGraph]: Graph already contains item!");
            return nullptr;
        }
        storage_.push_back(std::make_unique<Vertex<T>>(part));
        Vertex<T>* v = storage_.back().get();
        graph.push_back(v);
        lookup.Add(part, v);
        return v;
    }
    // :37 GetPartVertex (KeyNotFoundException when absent)
    Vertex<T>* GetPartVertex(const T& part) { return lookup.At(part); }

private:
    std::vector<std::unique_ptr<Vertex<T>>> storage_;
};

// StackFlowGraph.cs:7
class StackFlowGraph {
public:
    std::unique_ptr<FlowGraph<Part*>> delivery;  // :9
    std::unique_ptr<FlowGraph<Part*>> request;   // :11
    explicit StackFlowGraph(const std::vector<Part*>& ship);  // :13

private:
    static void UpdatePartVertex(bool request, Part* part, dotnet::Dictionary<Part*, Vertex<Part*>*>& lookup);  // :33
};

// StronglyConnectedComponentFinder.cs:6 (static class; the recursion state is per call here)
class StronglyConnectedComponentFinder {
public:
    static std::list<std::list<Part*>> DetectCycle(FlowGraph<Part*>& flowGraph);       // :16
    static std::list<PartHashSetPtr> DetectCycleSets(FlowGraph<Part*>& flowGraph);     // :67
};

// SCCFlowGraph.cs:5
class SCCFlowGraph {
public:
    class GraphSet {
    public:
        std::unique_ptr<FlowGraph<PartHashSetPtr>> flowGraph;                        // :9
        std::list<PartHashSetPtr> sCC;                                                // :11
        dotnet::Dictionary<Part*, Vertex<PartHashSetPtr>*> lookup;                    // :13
        dotnet::Dictionary<Vertex<PartHashSetPtr>*, PartHashSetPtr> supersetLookup;   // :15
        explicit GraphSet(std::list<PartHashSetPtr> scc) : flowGraph(std::make_unique<FlowGraph<PartHashSetPtr>>()), sCC(std::move(scc)) {}  // :17
    };

    std::unique_ptr<GraphSet> requests;               // :26
    std::unique_ptr<GraphSet> delivery;               // :28
    std::unique_ptr<StackFlowGraph> stackFlowGraph;   // :30

    explicit SCCFlowGraph(const std::vector<Part*>& ship);  // :32
    PartHashSetPtr GetAllRequests(Part* part);        // :41
    PartHashSetPtr GetAllDeliveries(Part* part);      // :46
    const std::list<PartHashSetPtr>& GetConnectedComponentsInRequests() { return requests->sCC; }    // :51
    const std::list<PartHashSetPtr>& GetConnectedComponentsInDeliveries() { return delivery->sCC; }  // :56

private:
    void Process(GraphSet& sCCFlowGraphSet, FlowGraph<Part*>& stackFlowGraph);  // :61
    PartHashSetPtr Connect(GraphSet& graph, Vertex<PartHashSetPtr>* vhp, PartHashSetPtr hp);  // :104
};

// SCCFlowGraphUCFinder.cs:5: "unreachable consumers": which fuel requests have no reachable delivery
class SCCFlowGraphUCFinder {
public:
    class GraphSet {
    public:
        dotnet::Dictionary<int, PartHashSetPtr> entries;                                         // :9
        dotnet::Dictionary<int, std::shared_ptr<dotnet::HashSet<Vertex<Part*>*>>> entriesVertex;  // :11
        dotnet::Dictionary<Part*, std::shared_ptr<dotnet::HashSet<int>>> reverseEntries;         // :13
        void AddEntrypoint(int id, Vertex<Part*>* partVertex);                                    // :22
    };

    std::unique_ptr<SCCFlowGraph> ownedGraph;  // (when constructed from a ship the finder owns its SCCFlowGraph)
    SCCFlowGraph* sccFlowGraph = nullptr;      // :43
    GraphSet delivery;                         // :45
    GraphSet request;                          // :47

    explicit SCCFlowGraphUCFinder(const std::vector<Part*>& ship);  // :49
    explicit SCCFlowGraphUCFinder(SCCFlowGraph* graph);             // :57

    static bool IsEntryPoint(Part* part);          // :84
    static bool IsEntryPointDelivery(Part* part);  // :89
    static bool IsEntryPointRequest(Part* part);   // :94
    PartHashSetPtr GetUnreachableFuelRequests(int resourceId);    // :169
    PartHashSetPtr GetUnreachableFuelDeliveries(int resourceId);  // :196

private:
    void BuildEntrySets();  // :65
    static bool IsEntryPoint(Part* part, const std::function<void(int)>& deliveryCallback, const std::function<void(int)>& requestCallback);  // :99
    static bool IsEntryPointDelivery(Part* part, const std::function<void(int)>& deliveryCallback);  // :120
    static bool IsEntryPointRequest(Part* part, const std::function<void(int)>& requestCallback);    // :145
};

}  // namespace RUI::Algorithms
}  // namespace ksp
