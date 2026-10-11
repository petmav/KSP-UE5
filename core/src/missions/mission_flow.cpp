// WP-54: Expansions/Missions/Flow/*.cs and MissionFlow.cs. See mission_flow.hpp.
#include "ksp/missions/mission_flow.hpp"

#include <algorithm>

#include "ksp/core/log.hpp"
#include "ksp/missions/mission.hpp"

namespace ksp::Expansions::Missions {

namespace Flow {

// ---- MEPath ------------------------------------------------------------------------------------------------------------------

// MEPath.cs:31
MEPath::MEPath(Mission* mission) : mission_(mission) {}

// MEPath.cs:31
MEPath::MEPath(MENode* firstNode) : mission_(firstNode->mission) { Nodes.push_back(firstNode); }

// MEPath.cs:13
MENode* MEPath::First() const {
    if (!Nodes.empty()) return Nodes[0];
    return nullptr;
}

// MEPath.cs:37
std::shared_ptr<MEPath> MEPath::Clone() const {
    auto mEPath = std::make_shared<MEPath>(mission_);
    mEPath->AddNodes(Nodes);
    return mEPath;
}

// MEPath.cs:44
void MEPath::AddNodes(const std::vector<MENode*>& addNodes) { Nodes.insert(Nodes.end(), addNodes.begin(), addNodes.end()); }

// MEPath.cs:49
bool MEPath::GetConvergence(const MEPath& target, std::unique_ptr<MEFlowConvergence>& c, int minHops) const {
    int num = minHops;
    while (true) {
        if (num < static_cast<int>(Nodes.size())) {
            if (std::find(target.Nodes.begin(), target.Nodes.end(), Nodes[static_cast<size_t>(num)]) != target.Nodes.end()) break;
            ++num;
            continue;
        }
        c.reset();
        return false;
    }
    c = std::make_unique<MEFlowConvergence>(Nodes[static_cast<size_t>(num)], num);
    return true;
}

// MEPath.cs:70
void MEPath::Load(const ConfigNode& node) {
    Nodes.clear();
    const std::vector<std::string> values = node.GetValuesList("nodeId");
    for (size_t i = 0; i < values.size(); ++i) {
        const Guid key = Guid::Parse(values[i]);
        if (mission_->nodes.ContainsKey(key)) {
            Nodes.push_back(mission_->nodes[key]);
            continue;
        }
        log::Error("Unable to find a node in the mission from the flow - Mission flow may be incorrect. id=" + values[i] + " ");
    }
}

// MEPath.cs:86
void MEPath::Save(ConfigNode& node) const {
    for (size_t i = 0; i < Nodes.size(); ++i) node.AddValue("nodeId", Nodes[i]->id().ToString());
}

// ---- MEPathGroup -------------------------------------------------------------------------------------------------------------

// MEPathGroup.cs:25
MEPathGroup::MEPathGroup(std::shared_ptr<MEPath> first) {
    Paths.push_back(std::move(first));
    minHops = 0;
}

// MEPathGroup.cs:13
MEPath* MEPathGroup::First() const {
    if (!Paths.empty()) return Paths[0].get();
    return nullptr;
}

// ---- MENodePathInfo ----------------------------------------------------------------------------------------------------------

// MENodePathInfo.cs:15
MENodePathInfo::MENodePathInfo(Mission* mission) : mission_(mission) {}

// MENodePathInfo.cs:21
MENodePathInfo::MENodePathInfo(MENode* n) : node(n), mission_(n->mission) {}

// MENodePathInfo.cs:27
void MENodePathInfo::AddPath(std::shared_ptr<MEPath> path) { paths.push_back(std::move(path)); }

// MENodePathInfo.cs:32
void MENodePathInfo::Load(const ConfigNode& cfg) {
    Guid value = Guid::Empty();
    if (cfg.TryGetValue("startNodeId", value)) {
        if (mission_->nodes.ContainsKey(value)) {
            node = mission_->nodes[value];
        } else {
            log::Error("Unable to find startNodeId (" + value.ToString() + ") in the loaded mission (" + mission_->title() + ") Nodes.");
        }
    }
    paths.clear();
    const std::vector<ConfigNode*> nodes = cfg.GetNodes("PATH");
    for (size_t i = 0; i < nodes.size(); ++i) {
        auto mEPath = std::make_shared<MEPath>(mission_);
        mEPath->Load(*nodes[i]);
        paths.push_back(mEPath);
    }
}

// MENodePathInfo.cs:56
void MENodePathInfo::Save(ConfigNode& cfg) const {
    cfg.AddValue("startNodeId", node->id().ToString());
    for (size_t i = 0; i < paths.size(); ++i) {
        ConfigNode* configNode = cfg.AddNode("PATH");
        paths[i]->Save(*configNode);
    }
}

// ---- MEFlowExtensions --------------------------------------------------------------------------------------------------------

// MEFlowExtensions.cs:7
std::vector<MENode*> DistinctStartNodes(const PathList& paths) {
    std::vector<MENode*> list;
    for (size_t i = 0; i < paths.size(); ++i) {
        if (!paths[i]->Nodes.empty() && std::find(list.begin(), list.end(), paths[i]->Nodes[0]) == list.end()) list.push_back(paths[i]->Nodes[0]);
    }
    return list;
}

// MEFlowExtensions.cs:20
int CountStartNodes(const PathList& paths) { return static_cast<int>(DistinctStartNodes(paths).size()); }

// MEFlowExtensions.cs:25
PathList GetPathsStarting(const PathList& paths, MENode* node) {
    PathList list;
    for (size_t i = 0; i < paths.size(); ++i) {
        if (!paths[i]->Nodes.empty() && paths[i]->Nodes[0] == node) list.push_back(paths[i]);
    }
    return list;
}

// ---- MEFlowBlock -------------------------------------------------------------------------------------------------------------

// MEFlowBlock.cs:96
MEFlowBlock::MEFlowBlock(Mission* mission) : mission_(mission) {}

// MEFlowBlock.cs:13
int MEFlowBlock::CountObjectives() const {
    int num = 0;
    for (size_t i = 0; i < Blocks.size(); ++i) {
        const IMEFlowBlock::Kind k = Blocks[i]->FlowKind();
        if (k == Kind::Node) {
            if (static_cast<MENode*>(Blocks[i])->isObjective) ++num;
        } else if ((k == Kind::Or || k == Kind::Then) && static_cast<MEFlowBlock*>(Blocks[i])->HasObjectives()) {
            ++num;
        }
    }
    return num;
}

// MEFlowBlock.cs:36
bool MEFlowBlock::HasObjectives() const { return CountObjectives() > 0; }

// MEFlowBlock.cs:38
bool MEFlowBlock::HasReachableObjectives() const {
    for (size_t num = 0; num < Blocks.size(); ++num) {
        const IMEFlowBlock::Kind k = Blocks[num]->FlowKind();
        if (k == Kind::Node) {
            if (static_cast<MENode*>(Blocks[num])->IsReachable()) return true;
        } else if ((k == Kind::Or || k == Kind::Then) && static_cast<MEFlowBlock*>(Blocks[num])->HasReachableObjectives()) {
            return true;
        }
    }
    return false;
}

// MEFlowBlock.cs:67
bool MEFlowBlock::HasVisibleChildren() const {
    for (size_t num = 0; num < Blocks.size(); ++num) {
        const IMEFlowBlock::Kind k = Blocks[num]->FlowKind();
        if (k == Kind::Node) {
            if (!static_cast<MENode*>(Blocks[num])->IsHiddenByEvent()) return true;
        } else if ((k == Kind::Or || k == Kind::Then) && static_cast<MEFlowBlock*>(Blocks[num])->HasVisibleChildren()) {
            return true;
        }
    }
    return false;
}

// MEFlowBlock.cs:102
void MEFlowBlock::UpdateMissionFlowUI(MEFlowParser* parser) {
    (void)parser;
    log::Error("[MEFlowBlock]: This method should be hidden by new declaration on child types");
}

// MEFlowBlock.cs:107
void MEFlowBlock::AddBlock(IMEFlowBlock* newBlock) { Blocks.push_back(newBlock); }

void MEFlowBlock::AddOwnedBlock(std::shared_ptr<MEFlowBlock> block) {
    Blocks.push_back(block.get());
    ownedBlocks_.push_back(std::move(block));
}

// MEFlowBlock.cs:112
void MEFlowBlock::Load(const ConfigNode& node) {
    const std::vector<ConfigNode*> children = node.GetNodes();
    for (size_t i = 0; i < children.size(); ++i) {
        const ConfigNode* configNode = children[i];
        if (configNode->name == "NODE") {
            Guid value = Guid::Empty();
            if (configNode->TryGetValue("id", value)) {
                if (mission_->nodes.ContainsKey(value)) {
                    Blocks.push_back(mission_->nodes[value]);
                    continue;
                }
                log::Error("[MEFlowBlock] Node ID loaded that doesnt exist in the mission. The flow will be invalid. ID=" + value.ToString());
            }
        } else if (configNode->name == "THEN") {
            auto mEFlowThenBlock = std::make_shared<MEFlowThenBlock>(mission_);
            mEFlowThenBlock->Load(*configNode);
            AddOwnedBlock(mEFlowThenBlock);
        } else if (configNode->name == "OR") {
            auto mEFlowOrBlock = std::make_shared<MEFlowOrBlock>(mission_);
            mEFlowOrBlock->Load(*configNode);
            AddOwnedBlock(mEFlowOrBlock);
        }
    }
}

// MEFlowBlock.cs:145
void MEFlowBlock::Save(ConfigNode& node) const {
    for (size_t i = 0; i < Blocks.size(); ++i) {
        const IMEFlowBlock::Kind k = Blocks[i]->FlowKind();
        if (k == Kind::Node) {
            node.AddNode("NODE")->AddValue("id", static_cast<MENode*>(Blocks[i])->id().ToString());
        } else if (k == Kind::Then) {
            ConfigNode* node2 = node.AddNode("THEN");
            static_cast<MEFlowThenBlock*>(Blocks[i])->Save(*node2);
        } else if (k == Kind::Or) {
            ConfigNode* node3 = node.AddNode("OR");
            static_cast<MEFlowOrBlock*>(Blocks[i])->Save(*node3);
        }
    }
}

// MEFlowThenBlock.cs:12
void MEFlowThenBlock::UpdateMissionFlowUIDirect(MEFlowParser* parser) {
    if (OnUpdateFlowUI) OnUpdateFlowUI(parser);
}

// MEFlowOrBlock.cs:12
void MEFlowOrBlock::UpdateMissionFlowUIDirect(MEFlowParser* parser) {
    if (OnUpdateFlowUI) OnUpdateFlowUI(parser);
}

// ---- MEFlowParser ------------------------------------------------------------------------------------------------------------

namespace {
// the shared tail of the four path-building loops in BuildPathsByDFS / BuildReversePathsByDFS:
//   MEPath p = new MEPath(first); p.AddNodes(sub.Nodes); if (p.Nodes.Count > 1 && p.Nodes[0] == p.Nodes[p.Nodes.Count - 1]) p.Nodes.RemoveAt(...)
std::shared_ptr<MEPath> ExtendedPath(MENode* first, const MEPath& sub) {
    auto p = std::make_shared<MEPath>(first);
    p->AddNodes(sub.Nodes);
    if (p->Nodes.size() > 1 && p->Nodes[0] == p->Nodes[p->Nodes.size() - 1]) p->Nodes.pop_back();
    return p;
}
}  // namespace

// MEFlowParser.cs:172
std::shared_ptr<MEFlowThenBlock> MEFlowParser::ParseMission(Mission* mission) {
    if (mission != nullptr && mission->startNode() != nullptr) {
        mission->flow = std::make_unique<MissionFlow>(mission);
        BuildPathsByDFS(mission->startNode(), nullptr);
        std::vector<MENode*> list;
        for (int i = 0; i < static_cast<int>(mission->nodes.ValuesList().size()); ++i) {
            MENode* mENode = mission->nodes.ValuesList()[static_cast<size_t>(i)];
            if (mENode->IsDockedToStartNode()) continue;
            if (mENode->isEndNode) {
                list.push_back(mENode);
                continue;
            }
            if (static_cast<int>(mENode->toNodes.size()) < 1) {
                list.push_back(mENode);
                continue;
            }
            bool flag = false;
            for (size_t j = 0; j < mENode->toNodes.size(); ++j) {
                if (!mENode->toNodes[j]->IsOrphanNode()) {
                    flag = true;
                    break;
                }
            }
            if (!flag) list.push_back(mENode);
        }
        for (size_t k = 0; k < list.size(); ++k) BuildReversePathsByDFS(list[k], nullptr);
        auto mEFlowThenBlock = std::make_shared<MEFlowThenBlock>(mission);
        PathList list2;
        const auto& startPaths = mission->flow->NodePaths.At(mission->startNode())->paths;
        for (size_t l = 0; l < startPaths.size(); ++l) list2.push_back(startPaths[l]->Clone());
        if (!list2.empty()) {
            mEFlowThenBlock = ParseBlock(list2);
            mission->flow->missionBlock = mEFlowThenBlock;
        }
        return mEFlowThenBlock;
    }
    log::Info("No Start Node defined in mission - skippping parse");
    return nullptr;
}

// MEFlowParser.cs:231
PathList* MEFlowParser::BuildPathsByDFS(MENode* startNode, std::unordered_set<MENode*>* visited) {
    MissionFlow* flow = startNode->mission->flow.get();
    if (flow->NodePaths.ContainsKey(startNode)) return &flow->NodePaths.At(startNode)->paths;
    std::unordered_set<MENode*> visitedOwned;
    if (visited == nullptr) visited = &visitedOwned;
    visited->insert(startNode);
    auto mENodePathInfo = std::make_shared<MENodePathInfo>(startNode);
    PathList emptyList;
    for (size_t i = 0; i < startNode->toNodes.size(); ++i) {
        MENode* mENode = startNode->toNodes[i];
        if (mENode->IsOrphanNode()) continue;
        const PathList* list = flow->NodePaths.ContainsKey(mENode) ? &flow->NodePaths.At(mENode)->paths
                               : (!visited->count(mENode) ? BuildPathsByDFS(mENode, visited) : &emptyList);
        if (list->empty()) {
            mENodePathInfo->AddPath(std::make_shared<MEPath>(mENode));
            continue;
        }
        for (size_t j = 0; j < list->size(); ++j) mENodePathInfo->AddPath(ExtendedPath(mENode, *(*list)[j]));
    }
    if (!startNode->isStartNode) {
        for (size_t k = 0; k < startNode->dockedNodes.size(); ++k) {
            MENode* mENode2 = startNode->dockedNodes[k];
            if (!mENode2->IsLogicNode() || mENode2->IsOrphanNode()) continue;
            const PathList* list2 = flow->NodePaths.ContainsKey(mENode2) ? &flow->NodePaths.At(mENode2)->paths
                                    : (!visited->count(mENode2) ? BuildPathsByDFS(mENode2, visited) : &emptyList);
            if (list2->empty()) {
                mENodePathInfo->AddPath(std::make_shared<MEPath>(mENode2));
                continue;
            }
            for (size_t l = 0; l < list2->size(); ++l) mENodePathInfo->AddPath(ExtendedPath(mENode2, *(*list2)[l]));
        }
    }
    flow->NodePaths.Add(startNode, mENodePathInfo);
    return &mENodePathInfo->paths;
}

// MEFlowParser.cs:299
PathList* MEFlowParser::BuildReversePathsByDFS(MENode* startNode, std::unordered_set<MENode*>* visited) {
    MissionFlow* flow = startNode->mission->flow.get();
    if (flow->NodeReversePaths.ContainsKey(startNode)) return &flow->NodeReversePaths.At(startNode)->paths;
    std::unordered_set<MENode*> visitedOwned;
    if (visited == nullptr) visited = &visitedOwned;
    visited->insert(startNode);
    auto mENodePathInfo = std::make_shared<MENodePathInfo>(startNode);
    if (startNode->IsOrphanNode()) {
        // sic: the info is not registered; the caller gets the (empty) list of a temporary (C# returns mENodePathInfo.paths)
        static PathList orphanEmpty;
        return &orphanEmpty;
    }
    PathList emptyList;
    for (size_t i = 0; i < startNode->fromNodes.size(); ++i) {
        MENode* mENode = startNode->fromNodes[i];
        const PathList* list = flow->NodeReversePaths.ContainsKey(mENode) ? &flow->NodeReversePaths.At(mENode)->paths
                               : (!visited->count(mENode) ? BuildReversePathsByDFS(mENode, visited) : &emptyList);
        if (list->empty()) {
            mENodePathInfo->AddPath(std::make_shared<MEPath>(mENode));
            continue;
        }
        for (size_t j = 0; j < list->size(); ++j) mENodePathInfo->AddPath(ExtendedPath(mENode, *(*list)[j]));
    }
    if (startNode->dockParentNode != nullptr) {
        MENode* dp = startNode->dockParentNode;
        const PathList* list2 = flow->NodeReversePaths.ContainsKey(dp) ? &flow->NodeReversePaths.At(dp)->paths
                                : (!visited->count(dp) ? BuildReversePathsByDFS(dp, visited) : &emptyList);
        if (list2->empty()) {
            mENodePathInfo->AddPath(std::make_shared<MEPath>(dp));
        } else {
            for (size_t k = 0; k < list2->size(); ++k) mENodePathInfo->AddPath(ExtendedPath(dp, *(*list2)[k]));
        }
    }
    flow->NodeReversePaths.Add(startNode, mENodePathInfo);
    return &mENodePathInfo->paths;
}

// MEFlowParser.cs:361
std::shared_ptr<MEFlowThenBlock> MEFlowParser::ParseBlock(PathList& pathsIn, MENode* insertNode) {
    // `(*holder)` is the caller's list (mutated in place) until `(*holder) = list` rebinds the local reference
    std::shared_ptr<PathList> holder(&pathsIn, [](PathList*) {});
    if (static_cast<int>((*holder).size()) >= 1 && !((*holder)[0]->First() == nullptr)) {
        auto mEFlowThenBlock = std::make_shared<MEFlowThenBlock>((*holder)[0]->First()->mission);
        if (insertNode != nullptr) mEFlowThenBlock->AddBlock(insertNode);
        bool flag = false;
        while (!flag) {
            if (CountStartNodes((*holder)) == 1) {
                while (CountStartNodes((*holder)) == 1) {
                    if (static_cast<int>((*holder)[0]->Nodes.size()) < 1) {
                        (*holder).erase((*holder).begin());
                        continue;
                    }
                    mEFlowThenBlock->AddBlock((*holder)[0]->Nodes[0]);
                    for (auto& path : (*holder)) {
                        if (static_cast<int>(path->Nodes.size()) > 0) path->Nodes.erase(path->Nodes.begin());
                    }
                }
            }
            PathList list;
            if (CountStartNodes((*holder)) > 1) {
                std::vector<std::shared_ptr<MEPathGroup>> list2 = ExtractPathGroups((*holder));
                if ((*holder)[0]->First() != nullptr) {
                    auto mEFlowOrBlock = std::make_shared<MEFlowOrBlock>((*holder)[0]->First()->mission);
                    if (static_cast<int>(list2.size()) > 1) {
                        for (size_t i = 0; i < list2.size(); ++i) {
                            std::shared_ptr<MEFlowThenBlock> mEFlowBlock = ParseBlock(list2[i]->Paths);
                            if (mEFlowBlock != nullptr) mEFlowOrBlock->AddOwnedBlock(mEFlowBlock);
                        }
                    } else {
                        PathList& paths2 = list2[0]->Paths;
                        for (size_t j = 0; j < paths2.size(); ++j) {
                            std::shared_ptr<MEPath> mEPath = paths2[j];
                            if (!(mEPath->First() != nullptr)) continue;
                            auto mEPath2 = std::make_shared<MEPath>(mEPath->Nodes[0]->mission);
                            int count = static_cast<int>(mEPath->Nodes.size());
                            while (count-- > 0) {
                                const auto it = std::find(mEPath->Nodes.begin(), mEPath->Nodes.end(), list2[0]->joinNode);
                                const int num = it == mEPath->Nodes.end() ? -1 : static_cast<int>(it - mEPath->Nodes.begin());
                                if (num < 0) break;
                                if (num <= count) {
                                    mEPath2->Nodes.insert(mEPath2->Nodes.begin(), mEPath->Nodes[static_cast<size_t>(count)]);
                                    mEPath->Nodes.erase(mEPath->Nodes.begin() + count);
                                }
                            }
                            list.push_back(mEPath2);
                        }
                        const std::vector<MENode*> list3 = DistinctStartNodes(paths2);
                        for (size_t k = 0; k < list3.size(); ++k) {
                            PathList starting = GetPathsStarting(paths2, list3[k]);
                            std::shared_ptr<MEFlowThenBlock> mEFlowBlock = ParseBlock(starting);
                            if (mEFlowBlock != nullptr) mEFlowOrBlock->AddOwnedBlock(mEFlowBlock);
                        }
                    }
                    if (!mEFlowOrBlock->Blocks.empty()) mEFlowThenBlock->AddOwnedBlock(mEFlowOrBlock);
                }
            }
            if (!list.empty()) {
                holder = std::make_shared<PathList>(list);
            } else {
                flag = true;
            }
        }
        return mEFlowThenBlock;
    }
    return nullptr;
}

// MEFlowParser.cs:467
std::vector<std::shared_ptr<MEPathGroup>> MEFlowParser::ExtractPathGroups(PathList& paths) {
    std::vector<std::shared_ptr<MEPathGroup>> list;
    for (int i = 0; i < static_cast<int>(paths.size()); ++i) {
        std::shared_ptr<MEPath> mEPath = paths[static_cast<size_t>(i)];
        bool flag = false;
        for (size_t j = 0; j < list.size(); ++j) {
            std::shared_ptr<MEPathGroup> mEPathGroup = list[j];
            std::unique_ptr<MEFlowConvergence> c;
            if (mEPathGroup->First() == mEPath.get()) {
                flag = true;
            } else if (mEPathGroup->First()->GetConvergence(*mEPath, c, mEPathGroup->minHops)) {
                flag = true;
                mEPathGroup->Paths.push_back(mEPath);
                if (mEPathGroup->joinNode == nullptr) {
                    mEPathGroup->joinNode = c->Node();
                } else {
                    const auto& p0 = mEPathGroup->Paths[0]->Nodes;
                    const auto it = std::find(p0.begin(), p0.end(), mEPathGroup->joinNode);
                    const int idx = it == p0.end() ? -1 : static_cast<int>(it - p0.begin());
                    if (idx < c->Hops()) {
                        mEPathGroup->joinNode = c->Node();
                        mEPathGroup->minHops = c->Hops();
                        mEPathGroup->Paths.erase(mEPathGroup->Paths.begin() + 1, mEPathGroup->Paths.end());
                        i = -1;
                    }
                }
                break;
            }
        }
        if (!flag) list.push_back(std::make_shared<MEPathGroup>(mEPath));
    }
    return list;
}

// MEFlowParser.cs:574
void MEFlowParser::UpdateFlowUIItems(IMEFlowBlock* start) {
    if (start->FlowKind() == IMEFlowBlock::Kind::Or) {
        auto* mEFlowOrBlock = static_cast<MEFlowOrBlock*>(start);
        mEFlowOrBlock->UpdateMissionFlowUIDirect(this);
        for (size_t i = 0; i < mEFlowOrBlock->Blocks.size(); ++i) UpdateFlowUIItems(mEFlowOrBlock->Blocks[i]);
    } else if (start->FlowKind() == IMEFlowBlock::Kind::Then) {
        auto* mEFlowThenBlock = static_cast<MEFlowThenBlock*>(start);
        mEFlowThenBlock->UpdateMissionFlowUIDirect(this);
        for (size_t j = 0; j < mEFlowThenBlock->Blocks.size(); ++j) UpdateFlowUIItems(mEFlowThenBlock->Blocks[j]);
    } else if (start->FlowKind() == IMEFlowBlock::Kind::Node) {
        start->UpdateMissionFlowUI(this);
    }
}

// MEFlowParser.cs:569
void MEFlowParser::UpdateFlowUIItems(Mission* mission) { UpdateFlowUIItems(mission->flow->missionBlock.get()); }

}  // namespace Flow

// ---- MissionFlow -------------------------------------------------------------------------------------------------------------

// MissionFlow.cs:16
MissionFlow::MissionFlow(Mission* mission) : mission_(mission) { missionBlock = std::make_shared<Flow::MEFlowBlock>(mission); }

// MissionFlow.cs:24
void MissionFlow::Load(const ConfigNode& node) {
    NodePaths = {};
    NodeReversePaths = {};
    missionBlock = std::make_shared<Flow::MEFlowThenBlock>(mission_);
    ConfigNode* node2 = nullptr;
    if (node.TryGetNode("BLOCKS", node2)) missionBlock->Load(*node2);
}

// MissionFlow.cs:36
void MissionFlow::Save(ConfigNode& node) const {
    ConfigNode* node2 = node.AddNode("BLOCKS");
    missionBlock->Save(*node2);
}

}  // namespace ksp::Expansions::Missions
