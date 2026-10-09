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
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class ShipConstruct {
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
};

}  // namespace ksp
