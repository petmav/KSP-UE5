// WP-18: ShipTemplate (Assembly-CSharp/ShipTemplate.cs, 251 lines): what the craft browser reads from a .craft file WITHOUT
// instantiating parts -- name, description, part count, controllability, cost / mass totals (ShipConstruction.GetPartCostsAndMass
// over the PART nodes), root part node, stage count, tech / experimental / mission-filter flags.
//
// Everything the template asks of other systems goes through the hooks of ShipTemplateEnvironment (R&D tech tree, the Making History
// mission part filter); the PartLoader / PartResourceLibrary lookups use SaveEnvironment::getPartInfoByName and the live
// PartResourceLibrary::Instance(). Unset hooks answer as a sandbox game without missions: every part is available and not
// experimental.
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

struct AvailablePart;

struct ShipTemplateEnvironment {
    // ResearchAndDevelopment.IsExperimentalPart(part) / PartTechAvailable(part) (ShipTemplate.cs:123, :127); unset: false / true
    static inline std::function<bool(const AvailablePart&)> isExperimentalPart;
    static inline std::function<bool(const AvailablePart&)> partTechAvailable;
    // ShipTemplate.cs:57-85: when Making History is installed, the game mode is MISSION / MISSION_BUILDER and a mission exists, the
    // part names its situation's part filter (and every pending vessel situation's filter) excludes. nullopt = that branch is not
    // taken (no expansion, other mode or no mission); a returned list may be empty.
    static inline std::function<std::optional<std::vector<std::string>>()> missionExcludedParts;
};

class ShipTemplate {
public:
    std::string shipName = "";         // :11
    std::string shipDescription = "";  // :13
    std::string filename = "";         // :15
    int shipType = 0;                  // :17 (0 VAB, 1 SPH)
    int partCount = 0;                 // :19
    bool isControllable = false;       // :21
    ConfigNode::NodePtr config;        // :23
    ConfigNode::NodePtr rootPartNode;  // :25
    bool shipPartsUnlocked = true;     // :27
    bool shipPartsExperimental = false;  // :29
    bool duplicatedParts = false;      // :31
    float totalCost = 0.0f;            // :33
    float fuelCost = 0.0f;             // :35
    float dryCost = 0.0f;              // :37
    float totalMass = 0.0f;            // :39
    int stageCount = 0;                // :41
    Vector3f shipSize{};               // :43

    Vector3f GetShipSize() const;                                     // :45 (ShipConstruction.CalculateCraftSize(ShipTemplate))
    void LoadShip(const std::string& filename, const ConfigNode::NodePtr& root);  // :50
    void LoadShip(const ConfigNode::NodePtr& root);                   // :56
    static std::string GetPartName(const ConfigNode& partNode);       // :211
    static std::string GetPartId(const ConfigNode& partNode);         // :217

private:
    void SetIfControllable();  // :224
};

}  // namespace ksp
