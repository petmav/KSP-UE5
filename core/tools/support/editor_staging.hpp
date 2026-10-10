// Test / tool support (not part of the library), WP-16: replays what the editor does with a loaded craft so that the staging model
// runs on it -- Part.Start's staging half for every part (modules' OnStart, stagingIcon check, UpdateStageability, stack icon), the
// StageManager that listens (Awake), then EditorLogic's onEditorLoad and the deferred SortIcons. ShipConstruct editing (WP-18)
// will own this sequence; until then it is what the dataset tools/golden_runner/ds_staging_sequence.cpp and
// tests/test_staging.cpp share. WP-40 added the EditorLogic-driven constructor (EditorLogicStagingHost).
#pragma once

#include <memory>

#include "craft_tree.hpp"
#include "ksp/editor/editor_logic.hpp"
#include "ksp/staging/stage_manager.hpp"

namespace ksp::support {

struct EditorSession {
    EditorStagingHost host;
    std::unique_ptr<EditorLogicStagingHost> logicHost;   // WP-40: the host of a session that follows an EditorLogic (root / selected part live)
    std::unique_ptr<StageManager> sm;

    // WP-40: the same session over an EditorLogic (EditorLogic.cs: StageManager reads EditorLogic.RootPart / SelectedPart): the part-tree
    // operations of the editor (attachPart / detachPart ...) then regenerate the staging sequence through Part.onAttach
    explicit EditorSession(EditorLogic& editor) : logicHost(std::make_unique<EditorLogicStagingHost>(editor)) {
        sm = std::make_unique<StageManager>(logicHost.get());
        sm->Awake();
    }

    // The root is the part without a parent (not necessarily the first of the craft file). The StageManager is created before the parts start so that the icons they create are held by it.
    explicit EditorSession(CraftTree& tree) {
        for (auto& p : tree.parts)
            if (p->parent == nullptr) {
                host.root = p.get();
                break;
            }
        sm = std::make_unique<StageManager>(&host);
        sm->Awake();
    }
    // The same over a ShipConstruct's live parts (WP-17: VesselDeltaV works on the editor ship)
    explicit EditorSession(const std::vector<Part*>& parts) {
        for (Part* p : parts)
            if (p->parent == nullptr) {
                host.root = p;
                break;
            }
        sm = std::make_unique<StageManager>(&host);
        sm->Awake();
    }
    static void StartParts(const std::vector<Part*>& parts) {
        for (Part* p : parts) {
            p->ModulesOnStart(StartState::Editor);
            p->StartStaging();
        }
    }
    // Part.Start in the editor (Part.cs:1727-1747): ModulesOnStart, then the staging block
    static void StartParts(CraftTree& tree) {
        for (auto& p : tree.parts) {
            p->ModulesOnStart(StartState::Editor);
            p->StartStaging();
        }
    }
    // EditorLogic.LoadShipFromFile: onEditorLoad, then SortIcons(instant: false) runs its three frames
    void Load() {
        sm->OnEditorLoad();
        sm->SortIcons(false);
        sm->FlushSort();
    }
};

}  // namespace ksp::support
