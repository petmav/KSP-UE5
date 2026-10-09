// ShipConstruct (Assembly-CSharp/ShipConstruct.cs): the .craft file load/save path, data side.
//
// KSP's LoadShip instantiates every part from its prefab and assigns the craft file's values to the live Part / its
// AttachNodes; SaveShip writes the craft back from those live objects (modules, resources, events, actions and the
// part data through their own Save methods). kspcore keeps the same values in ShipConstruct::PartData (named after the
// Part / AttachNode fields LoadShip assigns) and writes them in SaveShip's exact order and format; the editor logic and
// the live-part path are WP-40 / WP-13. Things that come from the part prefab in KSP and are therefore taken from the
// file here (documented approximations until the part database, WP-05, is wired in):
//  * Part.partName ("partName"), the module cost / mass / size (modCost, modMass, modSize), and the prefab defaults of
//    any field a file does not contain;
//  * the attach node list: KSP writes one attN per prefab attach node (in prefab order, "Null_0" when unattached);
//    kspcore writes the file's attN entries in file order, and AttachNode vectors an old file does not carry (KSP keeps
//    the prefab's) fall back to the file's original position / a zero orientation (`*Known` flags);
//  * MODULE / RESOURCE / EVENTS / ACTIONS / PARTDATA nodes are written back as loaded (KSP re-generates them from the
//    PartModule / PartResource / BaseEventList objects).
// What SaveShip rewrites on every save: "version" is the running game's (Versioning), the description's '\n' are
// written as U+00A8, size is CalculateCraftSize (kept from the file here), links / sym / srfN / attN partner names are
// re-derived from the partner part (craftID lookup, last match), srfN is only written when its partner exists, and the
// position is written twice in srfN (the third vector slot is position, not secondaryAxis -- sic).
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
struct AvailablePart;
class ShipConstruct;

// VesselCrewManifest.cs: the editor's crew assignment, seen through what ShipConstruct / ShipConstruction read of it (WP-18
// seam; the editor package implements it). ShipConstruction.ShipManifest is a pointer to one.
class VesselCrewManifest {
public:
    virtual ~VesselCrewManifest() = default;
    virtual int CrewCount() const = 0;                 // VesselCrewManifest.cs:18
    virtual float GetCrewInventoryCosts() = 0;         // :112
    virtual float GetCrewInventoryMass() = 0;          // :126
    virtual float GetCrewResourceMass() = 0;           // :145
    virtual void AssignCrewToVessel(ShipConstruct& ship) = 0;  // :490
};

// IShipConstructIDChanges.cs (implemented by ModuleRoboticController, Serenity): a module that tracks the part persistentIds
// LoadShip re-issued
class IShipConstructIDChanges {
public:
    virtual ~IShipConstructIDChanges() = default;
    virtual void UpdatePersistentIDs(const std::map<uint32_t, uint32_t>& changedIDs) = 0;
};

class ShipConstruct : public IResourceVessel {
public:
    // the AttachNode fields a craft file carries (AttachNode.cs)
    struct AttachNodeData {
        std::string id;
        uint32_t attachedPartId = 0;
        std::optional<std::string> srfAttachMeshName;
        Vector3f originalPosition;
        Vector3f originalOrientation;
        Vector3f position;
        Vector3f orientation;
        Vector3f secondaryAxis;
        Vector3f originalSecondaryAxis;
        bool originalOrientationKnown = false;  // false: the prefab's value (not available without the part database)
        bool positionKnown = false;
        bool orientationKnown = false;
    };

    // the Part fields a craft file carries (Part.cs field names)
    struct PartData {
        std::string partInfoName;  // AvailablePart.name (from "part" = name_craftID)
        uint32_t craftID = 0;
        std::optional<std::string> partName;  // Part.partName (prefab; carried from the file)
        uint32_t persistentId = 0;
        Vector3f position;      // transform.position ("pos")
        Quaternionf rotation;   // transform.rotation ("rot")
        Vector3f attPos;
        Vector3f attPos0;
        Quaternionf attRotation;
        Quaternionf attRotation0;
        Vector3f mirrorVector = Vector3f{1.0f, 1.0f, 1.0f};
        SymmetryMethod symMethod = SymmetryMethod::Radial;
        Part_AutoStrutMode autoStrutMode = Part_AutoStrutMode::Off;
        bool rigidAttachment = false;
        int inverseStage = 0;
        int resourcePriorityOffset = 0;
        int defaultInverseStage = 0;
        int inStageIndex = -1;
        int manualStageOffset = -1;
        int separationIndex = 0;
        int attachMode = 0;
        bool sameVesselCollision = false;
        std::string customPartData;
        std::optional<float> modCost;      // GetModuleCosts (module host; carried from the file)
        std::optional<float> modMass;      // GetModuleMass
        std::optional<Vector3f> modSize;   // GetModuleSize
        std::vector<uint32_t> linkIds;      // "link" craftIDs (children, in file order)
        std::vector<uint32_t> symIds;       // "sym" / "pSym" craftIDs (symmetryCounterparts)
        AttachNodeData srfAttachNode;
        bool srfAttachNodeInFile = false;
        std::vector<AttachNodeData> attachNodes;
        std::vector<ConfigNode::NodePtr> modules;    // MODULE
        std::vector<ConfigNode::NodePtr> resources;  // RESOURCE
        ConfigNode::NodePtr events;                  // EVENTS (BaseEventList)
        ConfigNode::NodePtr actions;                 // ACTIONS (BaseActionList)
        ConfigNode::NodePtr partData;                // PARTDATA (Part.OnSave)
        ConfigNode::NodePtr effects;                 // EFFECTS (read by LoadEffects, not written by SaveShip)
        std::unique_ptr<VesselNaming> vesselNaming;
        // resolved by LoadShip (Part.parent / children / symmetryCounterparts)
        int parent = -1;
        std::vector<int> children;
        std::vector<int> symmetryCounterparts;  // -1 = not found (KSP adds null)
    };

    static inline int lastCompatibleMajor = 0;
    static inline int lastCompatibleMinor = 18;
    static inline int lastCompatibleRev = 0;

    std::string shipName = "";
    std::string shipDescription = "";
    uint32_t persistentId = 0;
    Quaternionf rotation;
    std::vector<bool> OverrideDefault;
    std::vector<KSPActionGroup> OverrideActionControl;
    std::vector<KSPAxisGroup> OverrideAxisControl;
    std::vector<std::optional<std::string>> OverrideGroupNames;
    EditorFacility shipFacility = EditorFacility::None;
    bool shipPartsUnlocked = true;
    Vector3f shipSize;
    uint64_t steamPublishedFileId = 0;
    std::string missionFlag = "";
    VesselType vesselType = VesselType::Debris;
    std::vector<PartData> parts;

    // ShipConstruct.cs:76 (persistentId = FlightGlobals.GetUniquepersistentId(), identity rotation, 4-entry override
    // arrays)
    ShipConstruct();
    // ShipConstruct.cs:301 (returnErrors: error text instead of a popup; false when the version is incompatible, a part
    // is missing from the part database (SaveEnvironment::partExists, when set) or a link names no part)
    bool LoadShip(const ConfigNode& root, uint32_t persistentID, bool returnErrors, std::string& errorString);
    bool LoadShip(const ConfigNode& root, uint32_t persistentID = 0);  // ShipConstruct.cs:295 / 704
    // ShipConstruct.cs:153
    ConfigNode::NodePtr SaveShip();

    int Count() const { return static_cast<int>(parts.size()); }
    // KSPUtil.cs:2076 parts.GetPartByCraftID (last match; 0 -> none)
    int GetPartByCraftID(uint32_t id) const;

    // ======== WP-18: the live ship (ShipConstruct.cs `parts` is a List<Part>) ================================================
    //
    // C# `parts` (List<Part>) is `liveParts` here; `parts` above is WP-04's file-side record of the same PART nodes. A ShipConstruct
    // that was only LoadShip'd (WP-04) has no live parts; LoadShipWithParts / InstantiateParts (the part-creating half of
    // ShipConstruct.cs:301-700, the one the game runs with a part database) or the Part constructors below fill them. `ownedParts`
    // owns the Parts the ship created itself (AssembleForLaunch hands them to the Vessel); parts added with Add() stay the caller's.
    std::vector<Part*> liveParts;
    std::vector<std::unique_ptr<Part>> ownedParts;
    std::string highestPriorityName = "";  // :19 private
    Part* vesselNamedBy = nullptr;         // :40 internal
    ShipResourceSets resourceSets;         // :46 resourcePartSet (+ the cache of :48), WP-15
    // the PartSet of C# `resourcePartSet`
    const std::shared_ptr<PartSet>& resourcePartSet() const { return resourceSets.resourcePartSet; }

    static void RegisterResourceHost();
    ~ShipConstruct() override;
    // not movable or copyable: the Parts point back at their ship (Part.ship) and the resource sets at the Parts
    ShipConstruct(ShipConstruct&&) = delete;
    ShipConstruct& operator=(ShipConstruct&&) = delete;
    ShipConstruct(const ShipConstruct&) = delete;
    ShipConstruct& operator=(const ShipConstruct&) = delete;

    // ShipConstruct.cs:89 / :102 / :120 / :135 (CalculateCraftSize, persistentId and the Override arrays as the C# constructors do)
    explicit ShipConstruct(EditorFacility facility);
    ShipConstruct(const std::string& shipName, EditorFacility facility, std::vector<Part*> parts);
    ShipConstruct(const std::string& shipName, const std::string& shipDescription, Part* rootPart);
    ShipConstruct(const std::string& shipName, const std::string& shipDescription, std::vector<Part*> parts);

    // ShipConstruct.cs:36-62 Parts / indexer / Count (live)
    const std::vector<Part*>& Parts() const { return liveParts; }
    // IEnumerable<Part> (:942 GetEnumerator)
    std::vector<Part*>::iterator begin() { return liveParts.begin(); }
    std::vector<Part*>::iterator end() { return liveParts.end(); }
    Part*& operator[](size_t index) { return liveParts.at(index); }
    int LiveCount() const { return static_cast<int>(liveParts.size()); }
    // :742 Clear, :747 Add, :752 Remove, :757 Contains, :772 AddToConstruct (recursive over `children`)
    void Clear() { liveParts.clear(); }
    void Add(Part* p) { liveParts.push_back(p); }
    void Remove(Part* p);
    bool Contains(const Part* p) const;
    void AddToConstruct(Part* rootPart);

    // The part-creating half of LoadShip (ShipConstruct.cs:378-700) over the PART records `LoadShip` filled: Instantiate the prefab
    // of every record (PartLoader.getPartInfoByName = SaveEnvironment::getPartInfoByName), assign the file's values, load
    // MODULE / RESOURCE / PARTDATA / EVENTS / ACTIONS / EFFECTS / VESSELNAMING, link parent / children / symmetry / attach nodes,
    // orgPos / orgRot, InitializeModules, the IShipConstructIDChanges hand-over, StageManager.SetStageCount (a no-op in the C#),
    // UpdateVesselNaming, VesselDeltaV (editor, WP-17: `onShipLoaded` hook). Returns false with `errorString` when a part has no prefab.
    bool InstantiateParts(bool returnErrors, std::string& errorString);
    // ShipConstruct.LoadShip as the game runs it in a scene with a part database: LoadShip (file side) + InstantiateParts
    bool LoadShipWithParts(const ConfigNode& root, uint32_t persistentID, bool returnErrors, std::string& errorString);
    bool LoadShipWithParts(const ConfigNode& root, uint32_t persistentID = 0);

    // ShipConstruct.cs:153 SaveShip over the live parts (every value comes from the Part / PartModule / PartResource objects; the
    // part's EVENTS / ACTIONS lists are the nodes it was loaded with, PARTDATA is Part.OnSave). `SaveShip()` above dispatches here
    // when the ship has live parts.
    ConfigNode::NodePtr SaveShipFromParts();

    // :709 AreAllPartsConnected, :727 isControllable ([Obsolete])
    bool AreAllPartsConnected() const;
    bool isControllable() const;

    // :786 / :791 GetShipCosts, :822 / :827 GetShipMass, :859 GetTotalMass
    float GetShipCosts(float& dryCost, float& fuelCost, VesselCrewManifest* vcm = nullptr);
    float GetShipMass(float& dryMass, float& fuelMass, VesselCrewManifest* vcm = nullptr);
    float GetTotalMass();  // uses ShipConstruction::ShipManifest

    // :866 UpdateVesselNaming / :878 RunVesselNamingUpdates (the screen message is UI; the GameEvents are fired)
    bool UpdateVesselNaming(bool noGameEvent = false);
    void RunVesselNamingUpdates(Part* pNewName, bool noGameEvent);
    // hook: EditorLogic.RootPart (HighLogic.LoadedSceneIsEditor && vesselNamedBy == null); unset in the flight scene
    static inline std::function<Part*()> editorRootPart;

    // :916 / :927 / :936 / :941 (ShipResourceSets, WP-15)
    double RequestResource(Part* part, int id, double demand, bool usePriority, bool simulate) override;
    void UpdateResourceSets() { resourceSets.UpdateResourceSets(this, liveParts); }
    void UpdateResourceSets(const std::vector<Part*>* inputParts, RUI::Algorithms::SCCFlowGraph* sccGraph) {
        resourceSets.UpdateResourceSets(this, liveParts, inputParts, sccGraph);
    }
    // IResourceVessel (the host WP-15's Part.RequestResource reaches through `Part.ship`)
    const std::vector<Part*>& ResourceParts() const override { return liveParts; }
    void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, bool pulling = true) override;  // :916
    void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, double threshold, bool pulling) override;

    // Hooks for the editor / UI side of LoadShip (unset = the answer for "no such system")
    //   ResearchAndDevelopment.PartTechAvailable(availablePart) (ShipConstruct.cs:441; unset: true)
    static inline std::function<bool(const AvailablePart&)> partTechAvailable;
    //   VesselDeltaV.Create / UpdateModuleEngines in the editor (ShipConstruct.cs:690-700; WP-17)
    static inline std::function<void(ShipConstruct&)> onEditorShipLoaded;
    //   ScreenMessages.PostScreenMessage of RunVesselNamingUpdates (UI)
    static inline std::function<void(const std::string&)> postScreenMessage;

private:
    // ShipConstruct.cs:376 the `dictionary` of persistentIds LoadShip re-issued (old id -> new id); filled by the file-side LoadShip
    std::map<uint32_t, uint32_t> changedPersistentIds_;
    // LoadShipWithParts: the CheckPartpersistentId of each PART is made by InstantiateParts with the real Part (the argument of the
    // `persistentID` parameter of LoadShip is kept for it)
    bool deferPartIdCheck_ = false;
    bool partIdCheckDone_ = false;  // the file-side LoadShip already ran CheckPartpersistentId (with stand-in keys)
    uint32_t loadPersistentID_ = 0;
    friend class ShipConstruction;
};

}  // namespace ksp
