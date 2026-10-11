// WP-54: Expansions/Missions/Flow/*.cs and MissionFlow.cs: the mission flow graph (paths, path groups, the Then / Or block tree) and the
// parser that builds it from the node graph (MEFlowParser.ParseMission / BuildPathsByDFS / BuildReversePathsByDFS / ParseBlock /
// ExtractPathGroups). The UI half of MEFlowParser (CreateMissionFlowUI*, CreateFlowUIBlock, Create, the prefabs and colours) is the mission
// UI and is not ported; UpdateFlowUIItems (the traversal that fires the nodes' / blocks' OnUpdateFlowUI callbacks) is.
//
// Blocks: `List<IMEFlowBlock>` holds MENode (owned by the mission) and child blocks (owned by their parent block: `ownedBlocks`).
#pragma once

#include <memory>
#include <unordered_set>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/missions/mission_node.hpp"

namespace ksp::Expansions::Missions {

class Mission;

namespace Flow {

// Flow/MEFlowConvergence.cs:3
class MEFlowConvergence {
public:
    MEFlowConvergence(MENode* node, int hops) : Node_(node), Hops_(hops) {}  // :11
    int Hops() const { return Hops_; }                                         // :5
    MENode* Node() const { return Node_; }                                     // :7

private:
    MENode* Node_;
    int Hops_;
};

// Flow/MEPath.cs:8
class MEPath {
public:
    explicit MEPath(Mission* mission);                                          // :32
    explicit MEPath(MENode* firstNode);                                         // :38
    std::vector<MENode*> Nodes;                                                 // :12 (property with a private setter)
    MENode* First() const;                                                      // :16
    std::shared_ptr<MEPath> Clone() const;                                      // :43
    void AddNodes(const std::vector<MENode*>& addNodes);                        // :50
    bool GetConvergence(const MEPath& target, std::unique_ptr<MEFlowConvergence>& c, int minHops = 0) const;  // :55
    void Load(const ConfigNode& node);                                          // :74
    void Save(ConfigNode& node) const;                                          // :91

private:
    Mission* mission_;
};

// Flow/MEPathGroup.cs:5
class MEPathGroup {
public:
    explicit MEPathGroup(std::shared_ptr<MEPath> first);  // :24
    MENode* joinNode = nullptr;                           // :7
    int minHops = 0;                                      // :9
    std::vector<std::shared_ptr<MEPath>> Paths;           // :11
    MEPath* First() const;                                // :15
};

// Flow/MENodePathInfo.cs:8
class MENodePathInfo {
public:
    explicit MENodePathInfo(Mission* mission);            // :17
    explicit MENodePathInfo(MENode* node);                // :23
    MENode* node = nullptr;                               // :10
    std::vector<std::shared_ptr<MEPath>> paths;           // :12
    void AddPath(std::shared_ptr<MEPath> path);           // :29
    void Load(const ConfigNode& node);                    // :34
    void Save(ConfigNode& node) const;                    // :55

private:
    Mission* mission_;
};

// Flow/MEFlowExtensions.cs:5 (extension methods on List<MEPath>)
using PathList = std::vector<std::shared_ptr<MEPath>>;
std::vector<MENode*> DistinctStartNodes(const PathList& paths);           // :7
int CountStartNodes(const PathList& paths);                               // :20
PathList GetPathsStarting(const PathList& paths, MENode* node);           // :25

// Flow/MEFlowBlock.cs:8
class MEFlowBlock : public IMEFlowBlock {
public:
    explicit MEFlowBlock(Mission* mission);                               // :90
    ~MEFlowBlock() override = default;
    MEFlowBlock(const MEFlowBlock&) = delete;
    MEFlowBlock& operator=(const MEFlowBlock&) = delete;

    std::vector<IMEFlowBlock*> Blocks;                                    // :12
    Kind FlowKind() const override { return Kind::Block; }
    int CountObjectives() const override;                                 // :14
    bool HasObjectives() const override;                                  // :36
    bool HasReachableObjectives() const override;                         // :38
    bool HasVisibleChildren() const override;                             // :64
    void UpdateMissionFlowUI(MEFlowParser* parser) override;              // :96
    void AddBlock(IMEFlowBlock* newBlock);                                // :101
    // C# stores the child block object in the list; in C++ the parent block keeps it alive
    void AddOwnedBlock(std::shared_ptr<MEFlowBlock> block);
    void Load(const ConfigNode& node);                                    // :106
    void Save(ConfigNode& node) const;                                    // :139

protected:
    Mission* mission_;
    std::vector<std::shared_ptr<MEFlowBlock>> ownedBlocks_;
};

// Flow/MEFlowThenBlock.cs:3 / MEFlowOrBlock.cs:3
class MEFlowThenBlock : public MEFlowBlock {
public:
    explicit MEFlowThenBlock(Mission* mission) : MEFlowBlock(mission) {}  // :7
    Kind FlowKind() const override { return Kind::Then; }
    std::function<void(MEFlowParser*)> OnUpdateFlowUI;                    // :5
    // :11 `internal new void UpdateMissionFlowUI(MEFlowParser)`: hides the interface method; MEFlowParser.UpdateFlowUIItems calls this one
    void UpdateMissionFlowUIDirect(MEFlowParser* parser);
};
class MEFlowOrBlock : public MEFlowBlock {
public:
    explicit MEFlowOrBlock(Mission* mission) : MEFlowBlock(mission) {}    // :7
    Kind FlowKind() const override { return Kind::Or; }
    std::function<void(MEFlowParser*)> OnUpdateFlowUI;                    // :5
    void UpdateMissionFlowUIDirect(MEFlowParser* parser);                 // :11
};

// Flow/MEFlowParser.cs:9 (only the static parse half and UpdateFlowUIItems)
class MEFlowParser {
public:
    bool showNonObjectives = false;  // :21
    bool showEvents = false;         // :23

    // :172
    static std::shared_ptr<MEFlowThenBlock> ParseMission(Mission* mission);
    // :574 UpdateFlowUIItems(IMEFlowBlock start); :569 is UpdateFlowUIItems(mission.flow.missionBlock)
    void UpdateFlowUIItems(IMEFlowBlock* start);
    void UpdateFlowUIItems(Mission* mission);

private:
    static PathList* BuildPathsByDFS(MENode* startNode, std::unordered_set<MENode*>* visited);          // :231
    static PathList* BuildReversePathsByDFS(MENode* startNode, std::unordered_set<MENode*>* visited);   // :299
    static std::shared_ptr<MEFlowThenBlock> ParseBlock(PathList& paths, MENode* insertNode = nullptr);  // :361
    static std::vector<std::shared_ptr<MEPathGroup>> ExtractPathGroups(PathList& paths);                // :467
};

}  // namespace Flow

// MissionFlow.cs:6
class MissionFlow {
public:
    explicit MissionFlow(Mission* mission);                                  // :18
    // :8 / :10: Dictionary<MENode, MENodePathInfo>
    dotnet::Dictionary<MENode*, std::shared_ptr<Flow::MENodePathInfo>> NodePaths;
    dotnet::Dictionary<MENode*, std::shared_ptr<Flow::MENodePathInfo>> NodeReversePaths;
    std::shared_ptr<Flow::MEFlowBlock> missionBlock;                         // :12
    void Load(const ConfigNode& node);                                       // :27
    void Save(ConfigNode& node) const;                                       // :40

private:
    Mission* mission_;
};

}  // namespace ksp::Expansions::Missions
