// See include/ksp/ship/ship_template.hpp. Spec: Assembly-CSharp/ShipTemplate.cs
#include "ksp/ship/ship_template.hpp"

#include <algorithm>
#include <utility>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

namespace {
// `value.Substring(0, value.IndexOf('_'))` (IndexOf == -1 is an ArgumentOutOfRangeException)
std::string PartNameOf(const std::string& value) {
    const size_t i = value.find('_');
    if (i == std::string::npos) throw ArgumentException("Length cannot be less than zero. (Parameter 'length')");
    return value.substr(0, i);
}
}  // namespace

// ShipTemplate.cs:45
Vector3f ShipTemplate::GetShipSize() const { return ShipConstruction::CalculateCraftSize(*this); }

// ShipTemplate.cs:50
void ShipTemplate::LoadShip(const std::string& filename_, const ConfigNode::NodePtr& root) {
    filename = filename_;
    LoadShip(root);
}

// ShipTemplate.cs:56
void ShipTemplate::LoadShip(const ConfigNode::NodePtr& rootPtr) {
    ConfigNode& root = *rootPtr;
    config = rootPtr;
    partCount = root.CountNodes();
    {
        const std::string* sn = root.GetValuePtr("ship");
        shipName = sn != nullptr ? *sn : std::string();  // C# null string
    }
    if (root.HasValue("size")) shipSize = KSPUtil::ParseVector3(*root.GetValuePtr("size"));
    int count = root.nodes().Count();
    // Dictionary<string, ConfigNode> (entries are only added, found and removed by key; the one that remains is the root)
    std::vector<std::pair<std::string, ConfigNode::NodePtr>> dictionary;
    auto dictFind = [&dictionary](const std::string& k) {
        return std::find_if(dictionary.begin(), dictionary.end(), [&k](const auto& e) { return e.first == k; });
    };
    int index = count;
    while (true) {
        if (index-- > 0) {
            if (root.nodes()[index]->name == "PART") {
                const std::string* key = root.nodes()[index]->GetValuePtr("part");
                if (key == nullptr) throw ArgumentNullException("key");  // Dictionary.ContainsKey(null)
                if (dictFind(*key) != dictionary.end()) break;
                dictionary.emplace_back(*key, root.nodes().Shared(index));
            }
            continue;
        }
        bool flag = false;
        std::vector<std::string> list;
        // :57-85 (Making History mission part filters)
        if (ShipTemplateEnvironment::missionExcludedParts) {
            if (auto excluded = ShipTemplateEnvironment::missionExcludedParts()) {
                list = std::move(*excluded);
                if (!list.empty()) flag = true;
            }
        }
        for (int j = 0; j < count; j++) {
            ConfigNode& configNode = *root.nodes()[j];
            const std::string* partValue = configNode.GetValuePtr("part");
            if (partValue == nullptr) {
                partCount = 0;
                continue;
            }
            const std::string value = PartNameOf(*partValue);
            const AvailablePart* partInfoByName = SaveEnvironment::Current().getPartInfoByName ? SaveEnvironment::Current().getPartInfoByName(value) : nullptr;
            if (partInfoByName == nullptr) {
                log::Error("[ShipConstruct]: Trying to load " + shipName + " - No AvailablePart found for " + value);
                shipPartsUnlocked = false;
                continue;
            }
            float num3 = 0.0f, num2 = 0.0f, dryMass = 0.0f, fuelMass = 0.0f;
            const float num = ShipConstruction::GetPartCostsAndMass(configNode, partInfoByName, num3, num2, dryMass, fuelMass);
            totalCost = f32store(fx(totalCost) + fx(num));
            fuelCost = f32store(fx(fuelCost) + fx(num2));
            dryCost = f32store(fx(dryCost) + fx(num3));
            totalMass = f32store(fx(totalMass) + (fx(dryMass) + fx(fuelMass)));
            int count2 = configNode.values().Count();
            while (count2-- > 0) {
                const ConfigNode::Value& v = configNode.values()[count2];
                if (v.name == "link") {
                    auto it = dictFind(v.value);
                    if (it != dictionary.end()) dictionary.erase(it);
                }
                if (v.name == "istg") stageCount = std::max(stageCount, dotnet::ParseInt32(v.value));  // Mathf.Max(int, int)
            }
            if (ShipTemplateEnvironment::isExperimentalPart && ShipTemplateEnvironment::isExperimentalPart(*partInfoByName)) {
                shipPartsExperimental = true;
            } else if (ShipTemplateEnvironment::partTechAvailable && !ShipTemplateEnvironment::partTechAvailable(*partInfoByName)) {
                shipPartsUnlocked = false;
                continue;
            }
            if (!flag) continue;
            for (const std::string& excludedName : list) {
                if (excludedName == value) {
                    shipPartsUnlocked = false;
                    break;
                }
            }
        }
        int count3 = root.values().Count();
        while (count3-- > 0) {
            const ConfigNode::Value& value2 = root.values()[count3];
            if (value2.name == "description") {
                shipDescription = dotnet::Replace(value2.value, "\xC2\xA8", "\n");  // Replace('\u00a8', '\n')
            } else if (value2.name == "type") {
                if (value2.value == "SPH") shipType = 1;
                if (value2.value == "VAB") shipType = 0;
            } else if (value2.name == "ship") {
                shipName = value2.value;
            }
        }
        count = static_cast<int>(dictionary.size());
        if (count != 1) {
            log::Error("[ShipTemplate]: Could not locate root part in " + shipName + " as " + std::to_string(count) +
                       " entries remain after eliminating all parts listed as children. This is probably wrong.");
        } else {
            rootPartNode = dictionary.front().second;
        }
        stageCount++;
        SetIfControllable();
        return;
    }
    duplicatedParts = true;
    shipPartsUnlocked = false;
}

// ShipTemplate.cs:211
std::string ShipTemplate::GetPartName(const ConfigNode& partNode) {
    const std::string* value = partNode.GetValuePtr("part");
    if (value == nullptr) throw NullReferenceException();
    return PartNameOf(*value);
}

// ShipTemplate.cs:217
std::string ShipTemplate::GetPartId(const ConfigNode& partNode) {
    const std::string* value = partNode.GetValuePtr("part");
    if (value == nullptr) throw NullReferenceException();
    const size_t num = value->find('_') == std::string::npos ? 0 : value->find('_') + 1;  // IndexOf('_') + 1 (-1 + 1 = 0)
    return value->substr(num);
}

// ShipTemplate.cs:224
void ShipTemplate::SetIfControllable() {
    ConfigNode* node = config->GetNode("PART");
    if (node == nullptr) {
        log::Error(shipName + " has no root part defined, aborting");
        isControllable = false;
        return;
    }
    for (ConfigNode* m : node->GetNodes("MODULE")) {
        const std::string* name = m->GetValuePtr("name");
        if (name != nullptr && *name == "ModuleCommand") {
            isControllable = true;
            return;
        }
    }
}

}  // namespace ksp
