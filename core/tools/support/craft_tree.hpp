// Test/tool support (not part of the library): the part-side of ShipConstruct.LoadShip (ShipConstruct.cs:378-640) over WP-04's craft
// form -- Instantiate the prefab of every PART, assign the file's values, load its MODULE / RESOURCE / PARTDATA nodes, link parent /
// children / symmetry / attach nodes -- plus the CompoundPart target link of fuel lines and a minimal Vessel stand-in that owns the
// WP-15 PartSets. Used by tests/test_resource_flow.cpp and tools/golden_runner/ds_resource_mass.cpp. ShipConstruction proper (editor
// integration, staging) is WP-18; WP-14 supplies the real Vessel.
#pragma once

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/resource_modules.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_set.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/save/upgrade_pipeline.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::support {

struct CraftTree {
    std::vector<std::unique_ptr<Part>> parts;
    bool ok = false;
    std::string missing;
    std::vector<Part*> Raw() const {
        std::vector<Part*> r;
        for (const auto& p : parts) r.push_back(p.get());
        return r;
    }
};

// The v180 upgrade scripts read the prefab module's float field (v180_ModuleControlSurface.cs:52 GetBaseModule: the last module of the
// class in partPrefab.Modules); SaveEnvironment::prefabModuleFloat answers from the loaded parts.
inline void InstallUpgradeHooks(GameData& data) {
    SaveEnvironment::Current().prefabModuleFloat = [&data](std::string_view part, std::string_view moduleClass,
                                                           std::string_view field) -> std::optional<float> {
        const AvailablePart* ap = data.parts.getPartInfoByName(part);
        if (ap == nullptr || !ap->partPrefab) return std::nullopt;
        PartModuleList& mods = ap->partPrefab->Modules();
        for (size_t i = mods.Count(); i-- > 0;) {
            PartModule* m = mods.GetModule(i);
            if (m->ClassName() != moduleClass) continue;
            const ModuleValue v = m->Fields().GetValue(field);
            if (const float* f = std::get_if<float>(&v)) return *f;
            return std::nullopt;
        }
        return std::nullopt;
    };
}

// ShipConstruction.CreateConstructFromTemplate (ShipConstruction.cs:1113): the craft file goes through the upgrade pipeline
// (renamed parts such as liquidEngine1-2 -> liquidEngineMainsail.v2) and then ShipConstruct.LoadShip.
inline bool LoadCraftFile(const std::string& path, ShipConstruct& ship, std::string& err) {
    ConfigNode::NodePtr root = ConfigNode::Load(path);
    if (!root) {
        err = "cannot read " + path;
        return false;
    }
    const std::string shipName = root->GetValue("ship") ? *root->GetValue("ship") : std::string();
    ConfigNode::NodePtr upgraded;
    KSPUpgradePipeline::Process(
        root, shipName, SaveUpgradePipeline::LoadContext::Craft, [&](const ConfigNode::NodePtr& n) { upgraded = n; },
        [&](KSPUpgradePipeline::UpgradeFailOption, const ConfigNode::NodePtr&) {});
    if (!upgraded) {
        err = "upgrade pipeline failed for " + path;
        return false;
    }
    return ship.LoadShip(*upgraded, 0, true, err);
}

// ShipConstruct.cs:378-640 (part creation); false in `ok` when a part is missing from the stock database
inline CraftTree BuildCraftTree(GameData& data, const ShipConstruct& ship) {
    CraftTree out;
    for (const auto& d : ship.parts) {
        const AvailablePart* ap = data.parts.getPartInfoByName(d.partInfoName);
        if (ap == nullptr || !ap->partPrefab) {
            out.missing = d.partInfoName;
            return out;
        }
    }
    for (const auto& d : ship.parts) {
        const AvailablePart* ap = data.parts.getPartInfoByName(d.partInfoName);
        auto part = Part::Instantiate(*ap->partPrefab);
        part->gameObjectName = d.partInfoName;
        part->fields.craftID = d.craftID;
        part->partInfo = ap;
        part->fields.symMethod = static_cast<int>(d.symMethod);
        part->fields.persistentId = d.persistentId;
        part->partTransform.position = d.position;
        part->partTransform.rotation = d.rotation;
        part->fields.attPos = d.attPos;
        part->fields.attPos0 = d.attPos0;
        part->fields.attRotation = d.attRotation;
        part->fields.attRotation0 = d.attRotation0;
        part->SetMirror(d.mirrorVector);
        part->fields.autoStrutMode = static_cast<int>(d.autoStrutMode);
        part->fields.rigidAttachment = d.rigidAttachment;
        part->fields.inverseStage = d.inverseStage;
        part->fields.resourcePriorityOffset = d.resourcePriorityOffset;
        part->fields.defaultInverseStage = d.defaultInverseStage;
        part->fields.inStageIndex = d.inStageIndex;
        part->fields.manualStageOffset = d.manualStageOffset;
        part->fields.separationIndex = d.separationIndex;
        part->fields.attachMode = d.attachMode;
        part->fields.sameVesselCollision = d.sameVesselCollision;
        part->fields.customPartData = d.customPartData;
        if (d.srfAttachNodeInFile) {
            AttachNode& n = *part->srfAttachNode;
            n.attachedPartId = d.srfAttachNode.attachedPartId;
            n.srfAttachMeshName = d.srfAttachNode.srfAttachMeshName.value_or(std::string());
            if (d.srfAttachNode.originalOrientationKnown) {
                n.originalPosition = n.position = d.srfAttachNode.position;
                n.orientation = n.originalOrientation = d.srfAttachNode.orientation;
            }
        }
        for (const auto& a : d.attachNodes) {
            AttachNode* n = part->FindAttachNode(a.id);
            if (n == nullptr) continue;
            n->attachedPartId = a.attachedPartId;
            if (a.positionKnown) n->position = a.position;
            if (a.orientationKnown) n->orientation = a.orientation;
        }
        int moduleIndex = 0;
        for (const auto& m : d.modules) part->LoadModule(*m, moduleIndex);
        for (const auto& r : d.resources) part->SetResource(*r);
        if (d.partData) part->OnLoad(*d.partData);
        out.parts.push_back(std::move(part));
    }
    for (size_t n = 0; n < out.parts.size(); n++) {
        Part* part2 = out.parts[n].get();
        for (int c : ship.parts[n].children) out.parts[static_cast<size_t>(c)]->setParent(part2);
        for (int s : ship.parts[n].symmetryCounterparts) part2->symmetryCounterparts.push_back(s >= 0 ? out.parts[static_cast<size_t>(s)].get() : nullptr);
        std::vector<Part*> all = out.Raw();
        part2->srfAttachNode->owner = part2;
        part2->srfAttachNode->FindAttachedPart(all);
        for (AttachNode& an : part2->attachNodes) {
            an.owner = part2;
            an.FindAttachedPart(all);
        }
    }
    for (auto& p : out.parts) p->InitializeModules();
    out.ok = true;
    return out;
}

// CompoundPart.Start (CompoundPart.cs:170-235) for the fuel lines of a craft: the PARTDATA `tgt` craftID names the target part;
// SetTarget -> SetLink -> every CompoundPartModule.OnTargetSet(target) (CompoundPart.cs:708). The CompoundPart class itself is WP-26;
// this is the part of it the fuel-line flow needs. Returns the number of fuel lines linked.
inline int LinkFuelLines(CraftTree& tree, const ShipConstruct& ship) {
    int linked = 0;
    for (size_t i = 0; i < tree.parts.size(); i++) {
        Part& p = *tree.parts[i];
        if (p.className != "CompoundPart" || !ship.parts[i].partData) continue;
        if (p.Modules().GetModule([](PartModule& pm) { return dynamic_cast<CModuleFuelLine*>(&pm) != nullptr; }) == nullptr) continue;
        const auto tgt = ship.parts[i].partData->GetValue("tgt");
        if (!tgt) continue;
        uint32_t tgtId = 0;
        if (!dotnet::TryParseUInt32(*tgt, tgtId)) continue;
        Part* target = nullptr;
        for (auto& q : tree.parts)
            if (q->fields.craftID == tgtId) target = q.get();
        if (target == nullptr) continue;
        for (PartModule* m : p.Modules().GetModules([](PartModule& pm) { return dynamic_cast<CompoundPartModule*>(&pm) != nullptr; })) {
            auto* cm = dynamic_cast<CompoundPartModule*>(m);
            cm->compoundTarget = [target](const Part&) { return target; };
            cm->OnTargetSet(target);
        }
        linked++;
    }
    return linked;
}

// A Vessel stand-in: owns the parts' PartSets (Vessel.cs resourcePartSet / crossfeedSets) through VesselResourceSets and is what
// ResourceFlowEnvironment::vesselHost resolves Part.vessel to. Part.vessel is a real (bare) Vessel owned here, so modules may read
// Part.vessel->packed / ->parts (wpfix7: it used to be a reinterpret_cast of this object, undefined behaviour).
class TestVessel : public IResourceVessel {
public:
    // Vessel.cs:1574-1577: the four events that keep the resource sets current
    TestVessel() {
        Registry().push_back(this);
        ResourceFlowEnvironment::vesselHost = [](Vessel* v) -> IResourceVessel* {
            for (TestVessel* t : Registry())
                if (t->real_.get() == v) return t;
            return nullptr;
        };
        GameEvents::onPartResourceListChange.Add(this, &TestVessel::UpdateResourceSetsEventCheckPart);
        GameEvents::onPartCrossfeedStateChange.Add(this, &TestVessel::UpdateResourceSetsEventCheckPart);
        GameEvents::onPartFuelLookupStateChange.Add(this, &TestVessel::UpdateCrossfeedResourceSetFL);
        GameEvents::onPartPriorityChanged.Add(this, &TestVessel::UpdateResourceSetsEventCheckPart);
    }
    ~TestVessel() override {
        Registry().erase(std::remove(Registry().begin(), Registry().end(), this), Registry().end());
        GameEvents::onPartResourceListChange.Remove(this, &TestVessel::UpdateResourceSetsEventCheckPart);
        GameEvents::onPartCrossfeedStateChange.Remove(this, &TestVessel::UpdateResourceSetsEventCheckPart);
        GameEvents::onPartFuelLookupStateChange.Remove(this, &TestVessel::UpdateCrossfeedResourceSetFL);
        GameEvents::onPartPriorityChanged.Remove(this, &TestVessel::UpdateResourceSetsEventCheckPart);
        for (Part* p : parts) {
            p->vessel = nullptr;
            p->crossfeedPartSet.reset();
            p->simulationCrossfeedPartSet.reset();
        }
        real_->parts.clear();  // ~Vessel walks its parts; the fixture's parts may already be gone
    }
    Vessel* handle() { return real_.get(); }
    std::vector<Part*> parts;
    VesselResourceSets sets;
private:
    static std::vector<TestVessel*>& Registry() {
        static std::vector<TestVessel*> r;
        return r;
    }
    std::unique_ptr<Vessel> real_ = std::make_unique<Vessel>();
public:
    // Vessel.parts: the craft's parts become this vessel's (Part.vessel = vessel) and the resource sets are built (Vessel.cs:3913)
    void Adopt(const std::vector<Part*>& p) {
        parts = p;
        real_->parts = p;
        for (Part* q : parts) q->vessel = handle();
        UpdateResourceSets();
    }
    void UpdateResourceSets() { sets.UpdateResourceSets(handle(), parts); }
    void UpdateResourceSetsIfDirty() { sets.UpdateResourceSetsIfDirty(handle(), parts); }
    bool updateResourcesOnEvent = true;  // Vessel.cs:458
    // Vessel.cs:3951
    void UpdateResourceSetsEventCheckPart(Part* p) {
        if (p->vessel == handle()) {
            if (updateResourcesOnEvent) UpdateResourceSets();
            else sets.resourcesDirty = true;
        }
    }
    // Vessel.cs:3943 (HostedFromToAction<bool, Part>: data.from is the part)
    void UpdateCrossfeedResourceSetFL(GameEvents::HostedFromToAction<bool, Part*> data) {
        if (data.from->vessel == handle()) sets.resourcesDirty = true;
    }
    const std::vector<Part*>& ResourceParts() const override { return parts; }
    double RequestResource(Part* part, int id, double demand, bool usePriority, bool simulate) override {
        return sets.RequestResource(part, id, demand, usePriority, simulate);
    }
    void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, bool pulling) override {
        sets.GetConnectedResourceTotals(id, simulate, amount, maxAmount, pulling);
    }
    void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, double threshold, bool pulling) override {
        sets.GetConnectedResourceTotals(id, simulate, amount, maxAmount, threshold, pulling);
    }
};

}  // namespace ksp::support
