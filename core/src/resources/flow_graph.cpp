// RUI/Algorithms (WP-15): StackFlowGraph, StronglyConnectedComponentFinder, SCCFlowGraph, SCCFlowGraphUCFinder.
// The templates Vertex / FlowGraph are in ksp/resources/flow_graph.hpp.
#include "ksp/resources/flow_graph.hpp"

#include <algorithm>

#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_list.hpp"
#include "ksp/resources/resource_types.hpp"

namespace ksp::RUI::Algorithms {

namespace {
// `x is CompoundPart` (the Part subclass chosen by the cfg `module =` line, kept as Part::className)
bool IsCompoundPart(const Part* p) { return p->className == "CompoundPart"; }

// string.IsNullOrEmpty(NoCrossFeedNodeKey) || !id.Contains(key)  (ordinal substring)
bool NodeAllowsCrossfeed(const Part* part, const AttachNode& node) {
    const std::string key = part->fields.NoCrossFeedNodeKey.value_or(std::string());
    return key.empty() || node.id.find(key) == std::string::npos;
}
}  // namespace

// ---- StackFlowGraph.cs --------------------------------------------------------------------------------------------------

// StackFlowGraph.cs:13
StackFlowGraph::StackFlowGraph(const std::vector<Part*>& ship) {
    delivery = std::make_unique<FlowGraph<Part*>>();
    request = std::make_unique<FlowGraph<Part*>>();
    for (size_t i = 0; i < ship.size(); i++) {
        Part* part = ship[i];
        delivery->Add(part);
        request->Add(part);
    }
    for (size_t j = 0; j < ship.size(); j++) {
        Part* part = ship[j];
        UpdatePartVertex(false, part, delivery->lookup);
        UpdatePartVertex(true, part, request->lookup);
    }
}

// StackFlowGraph.cs:33
void StackFlowGraph::UpdatePartVertex(bool request, Part* part, dotnet::Dictionary<Part*, Vertex<Part*>*>& lookup) {
    Vertex<Part*>* vertex = lookup.At(part);
    for (size_t i = 0; i < part->fuelLookupTargets.size(); i++) {
        Part* target = part->fuelLookupTargets[i];
        // continue unless (target == parent && part.isAttached) || (target != parent && target.isAttached)
        if (!((target == part->parent && part->isAttached()) || (target != part->parent && target->isAttached()))) continue;
        Vertex<Part*>* value = nullptr;
        if (IsCompoundPart(target)) {
            if (target->parent != nullptr && lookup.TryGetValue(IsCompoundPart(target) ? target->parent : target, value)) {
                if (request) vertex->Dependencies.push_back(value);
                else value->Dependencies.push_back(vertex);
            }
        } else if (lookup.TryGetValue(target, value)) {
            if (request) vertex->Dependencies.push_back(value);
            else value->Dependencies.push_back(vertex);
        }
    }
    if (!part->fields.fuelCrossFeed) return;
    for (size_t j = 0; j < part->attachNodes.size(); j++) {
        AttachNode& attachNode = part->attachNodes[j];
        if (attachNode.attachedPart == nullptr || !attachNode.ResourceXFeed) continue;
        bool flag = false;
        AttachNode* attachNode2 = attachNode.FindOpposingNode();
        if (attachNode2 != nullptr) flag = !attachNode2->ResourceXFeed && !attachNode2->AllowOneWayXFeed;
        Vertex<Part*>* value2 = nullptr;
        if (!flag && NodeAllowsCrossfeed(part, attachNode) && lookup.TryGetValue(attachNode.attachedPart, value2)) {
            if (request) vertex->Dependencies.push_back(value2);
            else value2->Dependencies.push_back(vertex);
        }
    }
    Vertex<Part*>* value3 = nullptr;
    if (part->srfAttachNode && part->srfAttachNode->attachedPart != nullptr && part->srfAttachNode->attachedPart->fields.fuelCrossFeed &&
        lookup.TryGetValue(part->srfAttachNode->attachedPart, value3)) {
        vertex->Dependencies.push_back(value3);
        value3->Dependencies.push_back(vertex);
    }
}

// ---- StronglyConnectedComponentFinder.cs ---------------------------------------------------------------------------------

namespace {
struct TarjanState {
    int index = 0;
    std::vector<Vertex<Part*>*> stack;
};

// StronglyConnectedComponentFinder.cs:33 StrongConnect
void StrongConnect(Vertex<Part*>* v, TarjanState& st, std::list<std::list<Part*>>& out) {
    v->Index = st.index;
    v->LowLink = st.index;
    st.index++;
    st.stack.push_back(v);
    v->onStack = true;
    for (size_t i = 0; i < v->Dependencies.size(); i++) {
        Vertex<Part*>* w = v->Dependencies[i];
        if (w->Index < 0) {
            StrongConnect(w, st, out);
            v->LowLink = std::min(v->LowLink, w->LowLink);
        } else if (w->onStack) {
            v->LowLink = std::min(v->LowLink, w->Index);
        }
    }
    if (v->LowLink == v->Index) {
        std::list<Part*> component;
        Vertex<Part*>* w;
        do {
            w = st.stack.back();
            st.stack.pop_back();
            w->onStack = false;
            component.push_back(w->Value);
        } while (v != w);
        out.push_back(std::move(component));
    }
}

// StronglyConnectedComponentFinder.cs:84 StrongConnectSet
void StrongConnectSet(Vertex<Part*>* v, TarjanState& st, std::list<PartHashSetPtr>& out) {
    v->Index = st.index;
    v->LowLink = st.index;
    st.index++;
    st.stack.push_back(v);
    v->onStack = true;
    for (size_t i = 0; i < v->Dependencies.size(); i++) {
        Vertex<Part*>* w = v->Dependencies[i];
        if (w->Index < 0) {
            StrongConnectSet(w, st, out);
            v->LowLink = std::min(v->LowLink, w->LowLink);
        } else if (w->onStack) {
            v->LowLink = std::min(v->LowLink, w->Index);
        }
    }
    if (v->LowLink == v->Index) {
        auto component = std::make_shared<PartHashSet>();
        Vertex<Part*>* w;
        do {
            w = st.stack.back();
            st.stack.pop_back();
            w->onStack = false;
            component->Add(w->Value);
        } while (v != w);
        out.push_back(std::move(component));
    }
}
}  // namespace

// StronglyConnectedComponentFinder.cs:16
std::list<std::list<Part*>> StronglyConnectedComponentFinder::DetectCycle(FlowGraph<Part*>& flowGraph) {
    std::list<std::list<Part*>> result;
    TarjanState st;
    for (size_t i = 0; i < flowGraph.graph.size(); i++) {
        Vertex<Part*>* vertex = flowGraph.graph[i];
        if (vertex->Index < 0) StrongConnect(vertex, st, result);
    }
    return result;
}

// StronglyConnectedComponentFinder.cs:67
std::list<PartHashSetPtr> StronglyConnectedComponentFinder::DetectCycleSets(FlowGraph<Part*>& flowGraph) {
    std::list<PartHashSetPtr> result;
    TarjanState st;
    for (size_t i = 0; i < flowGraph.graph.size(); i++) {
        Vertex<Part*>* vertex = flowGraph.graph[i];
        if (vertex->Index < 0) StrongConnectSet(vertex, st, result);
    }
    return result;
}

// ---- SCCFlowGraph.cs ------------------------------------------------------------------------------------------------------

// SCCFlowGraph.cs:32
SCCFlowGraph::SCCFlowGraph(const std::vector<Part*>& ship) {
    stackFlowGraph = std::make_unique<StackFlowGraph>(ship);
    requests = std::make_unique<GraphSet>(StronglyConnectedComponentFinder::DetectCycleSets(*stackFlowGraph->request));
    delivery = std::make_unique<GraphSet>(StronglyConnectedComponentFinder::DetectCycleSets(*stackFlowGraph->delivery));
    Process(*requests, *stackFlowGraph->request);
    Process(*delivery, *stackFlowGraph->delivery);
}

// SCCFlowGraph.cs:41
PartHashSetPtr SCCFlowGraph::GetAllRequests(Part* part) { return requests->supersetLookup.At(requests->lookup.At(part)); }

// SCCFlowGraph.cs:46
PartHashSetPtr SCCFlowGraph::GetAllDeliveries(Part* part) { return delivery->supersetLookup.At(delivery->lookup.At(part)); }

// SCCFlowGraph.cs:61
void SCCFlowGraph::Process(GraphSet& sCCFlowGraphSet, FlowGraph<Part*>& stackFlowGraph_) {
    for (const PartHashSetPtr& current : sCCFlowGraphSet.sCC) {
        Vertex<PartHashSetPtr>* value = sCCFlowGraphSet.flowGraph->Add(current);
        for (Part* current2 : *current) sCCFlowGraphSet.lookup.Add(current2, value);
    }
    for (const PartHashSetPtr& current : sCCFlowGraphSet.sCC) {
        for (Part* current2 : *current) {
            Vertex<Part*>* vertex = stackFlowGraph_.lookup.At(current2);
            for (size_t i = 0; i < vertex->Dependencies.size(); i++) {
                Vertex<Part*>* vertex2 = vertex->Dependencies[i];
                if (!current->Contains(vertex2->Value)) {
                    Vertex<PartHashSetPtr>* value = sCCFlowGraphSet.flowGraph->lookup.At(current);
                    value->Dependencies.push_back(sCCFlowGraphSet.lookup.At(vertex2->Value));
                }
            }
        }
    }
    for (size_t j = 0; j < sCCFlowGraphSet.flowGraph->graph.size(); j++) {
        Vertex<PartHashSetPtr>* value = sCCFlowGraphSet.flowGraph->graph[j];
        Connect(sCCFlowGraphSet, value, std::make_shared<PartHashSet>());
    }
}

// SCCFlowGraph.cs:104
PartHashSetPtr SCCFlowGraph::Connect(GraphSet& graph, Vertex<PartHashSetPtr>* vhp, PartHashSetPtr hp) {
    if (vhp->Index < 0) {
        hp = std::make_shared<PartHashSet>(*vhp->Value);
        vhp->Index = 1;
        for (size_t i = 0; i < vhp->Dependencies.size(); i++) {
            PartHashSetPtr dep = Connect(graph, vhp->Dependencies[i], hp);
            hp->UnionWith(*dep);
        }
        graph.supersetLookup.Add(vhp, hp);
    } else {
        hp = graph.supersetLookup.At(vhp);
    }
    return hp;
}

// ---- SCCFlowGraphUCFinder.cs ---------------------------------------------------------------------------------------------

// SCCFlowGraphUCFinder.cs:22
void SCCFlowGraphUCFinder::GraphSet::AddEntrypoint(int id, Vertex<Part*>* partVertex) {
    if (entriesVertex.ContainsKey(id)) {
        entriesVertex.At(id)->Add(partVertex);
        entries.At(id)->Add(partVertex->Value);
    } else {
        auto vs = std::make_shared<dotnet::HashSet<Vertex<Part*>*>>();
        vs->Add(partVertex);
        entriesVertex.Add(id, vs);
        auto ps = std::make_shared<PartHashSet>();
        ps->Add(partVertex->Value);
        entries.Add(id, ps);
    }
    if (reverseEntries.ContainsKey(partVertex->Value)) {
        reverseEntries.At(partVertex->Value)->Add(id);
        return;
    }
    auto ids = std::make_shared<dotnet::HashSet<int>>();
    ids->Add(id);
    reverseEntries.Add(partVertex->Value, ids);
}

// SCCFlowGraphUCFinder.cs:49
SCCFlowGraphUCFinder::SCCFlowGraphUCFinder(const std::vector<Part*>& ship) {
    ownedGraph = std::make_unique<SCCFlowGraph>(ship);
    sccFlowGraph = ownedGraph.get();
    BuildEntrySets();
}

// SCCFlowGraphUCFinder.cs:57
SCCFlowGraphUCFinder::SCCFlowGraphUCFinder(SCCFlowGraph* graph) {
    sccFlowGraph = graph;
    BuildEntrySets();
}

// SCCFlowGraphUCFinder.cs:65
void SCCFlowGraphUCFinder::BuildEntrySets() {
    StackFlowGraph& stackFlowGraph = *sccFlowGraph->stackFlowGraph;
    for (size_t i = 0; i < stackFlowGraph.delivery->graph.size(); i++) {
        Part* value = stackFlowGraph.delivery->graph[i]->Value;
        Vertex<Part*>* partVertex = stackFlowGraph.delivery->GetPartVertex(value);
        Vertex<Part*>* partVertexR = stackFlowGraph.request->GetPartVertex(value);
        IsEntryPoint(
            value, [&](int id) { delivery.AddEntrypoint(id, partVertex); }, [&](int id) { request.AddEntrypoint(id, partVertexR); });
    }
}

// SCCFlowGraphUCFinder.cs:84
bool SCCFlowGraphUCFinder::IsEntryPoint(Part* part) { return IsEntryPoint(part, nullptr, nullptr); }
// SCCFlowGraphUCFinder.cs:89
bool SCCFlowGraphUCFinder::IsEntryPointDelivery(Part* part) { return IsEntryPointDelivery(part, nullptr); }
// SCCFlowGraphUCFinder.cs:94
bool SCCFlowGraphUCFinder::IsEntryPointRequest(Part* part) { return IsEntryPointRequest(part, nullptr); }

// SCCFlowGraphUCFinder.cs:99
bool SCCFlowGraphUCFinder::IsEntryPoint(Part* part, const std::function<void(int)>& deliveryCallback,
                                        const std::function<void(int)>& requestCallback) {
    if (deliveryCallback) IsEntryPointDelivery(part, deliveryCallback);
    else if (IsEntryPointDelivery(part, deliveryCallback)) return true;
    if (requestCallback) IsEntryPointRequest(part, requestCallback);
    else if (IsEntryPointRequest(part, requestCallback)) return true;
    return false;
}

namespace {
bool IsStackMode(ResourceFlowMode m) {
    return m == ResourceFlowMode::STACK_PRIORITY_SEARCH || m == ResourceFlowMode::STAGE_STACK_FLOW ||
           m == ResourceFlowMode::STAGE_STACK_FLOW_BALANCE;
}
}  // namespace

// SCCFlowGraphUCFinder.cs:120
bool SCCFlowGraphUCFinder::IsEntryPointDelivery(Part* part, const std::function<void(int)>& deliveryCallback) {
    int count = part->Resources.Count();
    for (int num = 0; num < count; num++) {
        PartResource& partResource = part->Resources.At(num);
        if (partResource.amount > 0.0 && IsStackMode(AsFlowMode(partResource.info->resourceFlowMode()))) {
            if (!deliveryCallback) return true;
            deliveryCallback(partResource.info->id());
        }
    }
    return false;
}

// SCCFlowGraphUCFinder.cs:145
bool SCCFlowGraphUCFinder::IsEntryPointRequest(Part* part, const std::function<void(int)>& requestCallback) {
    std::vector<PartModule*> modules =
        part->Modules().GetModules([](PartModule& m) { return dynamic_cast<IResourceConsumer*>(&m) != nullptr; });
    for (size_t i = 0; i < modules.size(); i++) {
        std::vector<const PartResourceDefinition*> consumedResources = dynamic_cast<IResourceConsumer*>(modules[i])->GetConsumedResources();
        for (size_t j = 0; j < consumedResources.size(); j++) {
            const PartResourceDefinition* d = consumedResources[j];
            if (IsStackMode(AsFlowMode(d->resourceFlowMode()))) {
                if (!requestCallback) return true;
                requestCallback(d->id());
            }
        }
    }
    return false;
}

// SCCFlowGraphUCFinder.cs:169
PartHashSetPtr SCCFlowGraphUCFinder::GetUnreachableFuelRequests(int resourceId) {
    PartHashSetPtr value, value2;
    const bool num = request.entries.TryGetValue(resourceId, value);
    const bool flag = delivery.entries.TryGetValue(resourceId, value2);
    if (num) {
        if (flag) {
            auto hashSet = std::make_shared<PartHashSet>(*value);
            for (Part* current : *value)
                if (sccFlowGraph->GetAllRequests(current)->Overlaps(*value2)) hashSet->Remove(current);
            return hashSet;
        }
        return value;
    }
    return std::make_shared<PartHashSet>();
}

// SCCFlowGraphUCFinder.cs:196
PartHashSetPtr SCCFlowGraphUCFinder::GetUnreachableFuelDeliveries(int resourceId) {
    PartHashSetPtr value, value2;
    const bool num = delivery.entries.TryGetValue(resourceId, value);
    const bool flag = request.entries.TryGetValue(resourceId, value2);
    if (num) {
        if (flag) {
            auto hashSet = std::make_shared<PartHashSet>(*value);
            for (Part* current : *value)
                if (sccFlowGraph->GetAllDeliveries(current)->Overlaps(*value2)) hashSet->Remove(current);
            return hashSet;
        }
        return value;
    }
    return std::make_shared<PartHashSet>();
}

}  // namespace ksp::RUI::Algorithms
