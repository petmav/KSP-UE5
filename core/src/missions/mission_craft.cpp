// WP-54c: Assembly-CSharp/Expansions/Missions/Mission.cs, craft-file bookkeeping half (RebuildCraftFileList, ProcessCraftRenames, the Get*Craft* lookups, the
// vessel / part event handlers that keep mappedVessels current, ProcessMappedVesselNodeMap). See mission.hpp.
// Active when MissionsEnvironment::craftFileBookkeeping is true (the WP-54 hooks otherwise).
// Deviations from the C# (the language, not the behaviour): `part.vesselNaming.vesselName` null is "" (std::string has no null);
// `MissionsApp.Instance.RefreshMissionRequested(this, false)` is MissionsEnvironment::refreshMissionRequested (unset = no MissionsApp).
#include <algorithm>
#include <filesystem>

#include "../save/save_util.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission.hpp"
#include "ksp/missions/mission_actions3.hpp"
#include "ksp/missions/mission_tests2.hpp"
#include "ksp/missions/mission_tests4.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Expansions::Missions {

using KSP::Localization::Localizer;

namespace {
// Directory.Exists / Directory.GetFiles(path, "*.craft")
bool DirectoryExists(const std::string& path) {
    if (MissionDataEnvironment::directoryExists) return MissionDataEnvironment::directoryExists(path);
    std::error_code ec;
    return std::filesystem::is_directory(path, ec);
}
std::vector<std::string> GetCraftFiles(const std::string& path) {
    if (MissionDataEnvironment::getFiles) return MissionDataEnvironment::getFiles(path, ".craft");
    std::vector<std::string> result;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
        if (entry.is_regular_file(ec) && entry.path().extension() == ".craft") result.push_back(path + entry.path().filename().string());
    }
    std::sort(result.begin(), result.end());
    return result;
}
// Path.GetFileName
std::string GetFileName(const std::string& path) {
    const size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}
}  // namespace

// MissionsApp.Instance.RefreshMissionRequested(this, changePositions: false)
void Mission::RefreshMissionRequested() {
    if (MissionsEnvironment::refreshMissionRequested) MissionsEnvironment::refreshMissionRequested(*this, false);
}

// Mission.cs:1350
void Mission::RebuildCraftFileList() {
    ClearCraftFiles();
    mappedVesselKeys.clear();
    ProcessCraftFileFolder(ShipsPath() + "VAB/");
    ProcessCraftFileFolder(ShipsPath() + "SPH/");
    ClearMappedVessels();
}

// Mission.cs:1363
void Mission::ClearCraftFiles() {
    if (craftFileList.Count() > 0) {
        for (int i = 0; i < craftFileList.Count(); ++i) craftFileList.At(i)->Clear();
        craftFileList.Clear();
    }
}

// Mission.cs:1392
void Mission::ClearMappedVessels() {
    int count = static_cast<int>(mappedVessels.size());
    while (count-- > 0) {
        const MissionMappedVessel& m = mappedVessels[static_cast<size_t>(count)];
        if (!MappedVessels::Exists(mappedVesselKeys, m.currentVesselPersistentId, m.partPersistentId, m.partVesselName, m.craftFileName))
            mappedVessels.erase(mappedVessels.begin() + count);
    }
}

// Mission.cs:1399
void Mission::ProcessCraftFileFolder(const std::string& saveShipFolderPath) {
    if (DirectoryExists(saveShipFolderPath)) {
        const std::vector<std::string> files = GetCraftFiles(saveShipFolderPath);
        for (size_t i = 0; i < files.size(); ++i) {
            const std::string fileName = GetFileName(files[i]);
            auto craftFile = std::make_shared<MissionCraft>(saveShipFolderPath, fileName);
            ProcessCraftRenames(fileName, craftFile.get());
        }
    }
}

// Mission.cs:1413
void Mission::ProcessCraftRenames(const std::string& craftFileName, MissionCraft* craftFile) {
    if (craftFile == nullptr) return;
    ConfigNode* craftNode = craftFile->CraftNode();
    GetAllVesselSituations();
    uint32_t value = 0;
    VesselSituation* missionSituationByCraftFileName = GetMissionSituationByCraftFileName(craftFile->craftFile);
    if (missionSituationByCraftFileName != nullptr) {
        value = missionSituationByCraftFileName->persistentId;
    } else {
        if (craftFile->CraftNode() == nullptr) throw NullReferenceException();
        craftFile->CraftNode()->TryGetValue("persistentId", value);
    }
    if (craftNode == nullptr || value == 0) return;
    std::string value2;
    if (!craftNode->TryGetValue("ship", value2)) return;
    const std::vector<ConfigNode*> array = craftNode->GetNodes("PART");
    for (size_t i = 0; i < array.size(); ++i) {
        std::string value3;
        const uint32_t num = craftFile->GetPartPersistentId(array[i]);
        ConfigNode* node = nullptr;
        if (array[i]->TryGetNode("VESSELNAMING", node)) {
            node->TryGetValue("name", value3);
            if (!value3.empty()) {
                const MissionMappedVessel* found = MappedVessels::Find(mappedVessels, value, num, value3, craftFile->craftFile);
                MissionMappedVessel missionMappedVessel = found != nullptr ? *found : AddMappedPartVesselName(num, value, value3, craftFile, true, missionSituationByCraftFileName);
                if (!MappedVessels::Exists(mappedVesselKeys, value, num, value3, craftFile->craftFile)) mappedVesselKeys.push_back(missionMappedVessel);
                if (i == 0) AddMappedCraft(craftFileName, craftFile);
            }
        } else if (i == 0) {
            AddMappedPartVesselName(num, value, craftFileName, craftFile, false, missionSituationByCraftFileName);
        }
    }
}

// Mission.cs:1465
MissionMappedVessel Mission::AddMappedPartVesselName(uint32_t partId, uint32_t vslPersistentId, const std::string& craftFileName, MissionCraft* craftFile, bool mappedItem,
                                                      VesselSituation* vesselSituation) {
    const std::string text = mappedItem ? craftFileName : craftFile->VesselName();
    auto missionCraft = std::make_shared<MissionCraft>(craftFile->craftFolder, craftFile->craftFile);
    missionCraft->mappedVesselName = text;
    missionCraft->mappedItem = mappedItem;
    if (!mappedItem && craftFileList.ContainsKey(craftFileName)) craftFileList.Remove(craftFileName);
    MissionMappedVessel missionMappedVessel(partId, vesselSituation != nullptr ? vesselSituation->persistentId : vslPersistentId, vslPersistentId, text, craftFile->craftFile,
                                            vesselSituation != nullptr ? Localizer::Format(vesselSituation->vesselName) : std::string());
    mappedVesselKeys.push_back(missionMappedVessel);  // List.AddUnique on a class without Equals: reference equality, always added
    if (!mappedItem) craftFileList.Add(craftFileName, missionCraft);
    if (!MappedVessels::Exists(mappedVessels, vslPersistentId, partId, text, craftFile->craftFile)) mappedVessels.push_back(missionMappedVessel);
    return missionMappedVessel;
}

// Mission.cs:1493
void Mission::AddMappedCraft(const std::string& craftFileName, MissionCraft* craftFile) {
    const std::string vesselName = craftFile->VesselName();
    auto missionCraft = std::make_shared<MissionCraft>(craftFile->craftFolder, craftFile->craftFile);
    missionCraft->mappedVesselName = vesselName;
    missionCraft->mappedItem = false;
    if (craftFileList.ContainsKey(craftFileName)) craftFileList.Remove(craftFileName);
    craftFileList.Add(craftFileName, missionCraft);
}

// Mission.cs:1510
MissionCraft* Mission::GetCraftBySituationsVesselID(uint32_t persistentId) {
    persistentId = MappedVessels::ConvertMappedId(mappedVessels, persistentId);
    DictionaryValueList<VesselSituation*, Guid> allVesselSituationsGuid = GetAllVesselSituationsGuid();
    int num = 0;
    while (true) {
        if (num < allVesselSituationsGuid.Count()) {
            if (allVesselSituationsGuid.KeyAt(num)->persistentId == persistentId) break;
            num++;
            continue;
        }
        return nullptr;
    }
    if (craftFileList.ContainsKey(allVesselSituationsGuid.KeyAt(num)->craftFile)) return craftFileList[allVesselSituationsGuid.KeyAt(num)->craftFile].get();
    return nullptr;
}

// Mission.cs:1535
MissionCraft* Mission::GetMissionCraftByFileName(const std::string& craftFileName) {
    std::shared_ptr<MissionCraft> val;
    craftFileList.TryGetValue(craftFileName, val);
    return val.get();
}

// Mission.cs:1543
VesselSituation* Mission::GetMissionSituationByCraftFileName(const std::string& craftFileName) {
    std::vector<VesselSituation*> allVesselSituations = GetAllVesselSituations();
    size_t num = 0;
    while (true) {
        if (num < allVesselSituations.size()) {
            if (allVesselSituations[num]->craftFile == craftFileName) break;
            num++;
            continue;
        }
        return nullptr;
    }
    return allVesselSituations[num];
}

// Mission.cs:1562
MissionCraft* Mission::GetMissionCraftByName(const std::string& vesselName) {
    int num = 0;
    while (true) {
        if (num < craftFileList.Count()) {
            if (craftFileList.At(num)->VesselName() == vesselName) break;
            num++;
            continue;
        }
        return nullptr;
    }
    return craftFileList.At(num).get();
}

// Mission.cs:1583
int Mission::GetSituationsIndexByVessel(uint32_t persistentId) {
    persistentId = MappedVessels::ConvertMappedId(mappedVessels, persistentId);
    std::vector<VesselSituation*> allVesselSituations = GetAllVesselSituations();
    size_t num = 0;
    while (true) {
        if (num < allVesselSituations.size()) {
            if (allVesselSituations[num]->persistentId == persistentId) break;
            num++;
            continue;
        }
        return -1;
    }
    return static_cast<int>(num);
}

// Mission.cs:1602
int Mission::GetSituationsIndexByPart(uint32_t persistentId) {
    std::vector<VesselSituation*> allVesselSituations = GetAllVesselSituations();
    for (size_t i = 0; i < allVesselSituations.size(); ++i) {
        MissionCraft* missionCraftByFileName = GetMissionCraftByFileName(allVesselSituations[i]->craftFile);
        if (missionCraftByFileName == nullptr) continue;
        if (missionCraftByFileName->CraftNode() == nullptr) throw NullReferenceException();
        const std::vector<ConfigNode*> array = missionCraftByFileName->CraftNode()->GetNodes("PART");
        for (size_t j = 0; j < array.size(); ++j) {
            if (missionCraftByFileName->GetPartPersistentId(array[j]) == persistentId) return static_cast<int>(i);
        }
    }
    return -1;
}

// Mission.cs:1625
int Mission::GetCraftFileIndexByPart(uint32_t persistentId) {
    for (int i = 0; i < craftFileList.Count(); ++i) {
        if (craftFileList.At(i)->CraftNode() == nullptr) continue;
        const std::vector<ConfigNode*> array = craftFileList.At(i)->CraftNode()->GetNodes("PART");
        for (size_t j = 0; j < array.size(); ++j) {
            if (craftFileList.At(i)->GetPartPersistentId(array[j]) == persistentId) return i;
        }
    }
    return 0;
}

// ---- the vessel / part event handlers (Mission.cs:1217-1348) ---------------------------------------------------------------------

// Mission.cs:1217
void Mission::OnPartVesselNamingChanged(Part* part) {
    if (part == nullptr) return;
    bool flag = false;
    for (size_t i = 0; i < mappedVessels.size(); ++i) {
        if (mappedVessels[i].partPersistentId == part->fields.persistentId) {
            if (part->vesselNaming == nullptr) throw NullReferenceException();
            mappedVessels[i].partVesselName = part->vesselNaming->vesselName.value_or("");
            if (part->vessel != nullptr) mappedVessels[i].currentVesselPersistentId = part->vessel->persistentId;
            flag = true;
        }
    }
    if (flag) RefreshMissionRequested();
}

// Mission.cs:1244
void Mission::OnVesselRename(GameEvents::HostedFromToAction<Vessel*, std::string> hostedFromTo) {
    if (hostedFromTo.host == nullptr) return;
    bool flag = false;
    std::vector<VesselSituation*> allVesselSituations = GetAllVesselSituations();
    for (size_t i = 0; i < allVesselSituations.size(); ++i) {
        if (allVesselSituations[i]->persistentId == hostedFromTo.host->persistentId) {
            allVesselSituations[i]->vesselName = hostedFromTo.to;
            flag = true;
        }
    }
    for (size_t j = 0; j < mappedVessels.size(); ++j) {
        if (mappedVessels[j].currentVesselPersistentId == hostedFromTo.host->persistentId) {
            mappedVessels[j].situationVesselName = hostedFromTo.to;
            flag = true;
        }
    }
    if (flag) RefreshMissionRequested();
}

// Mission.cs:1262
void Mission::OnPartDecoupleNewVesselComplete(Vessel* oldVessel, Vessel* newVessel) {
    bool flag = false;
    for (size_t i = 0; i < mappedVessels.size(); ++i) {
        if (mappedVessels[i].currentVesselPersistentId == oldVessel->persistentId && newVessel->PartsContain(mappedVessels[i].partPersistentId) != nullptr) {
            mappedVessels[i].currentVesselPersistentId = newVessel->persistentId;
            flag = true;
        }
    }
    if (flag) RefreshMissionRequested();
}

// Mission.cs:1281
void Mission::OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) {
    bool flag = false;
    for (size_t i = 0; i < mappedVessels.size(); ++i) {
        if (mappedVessels[i].currentVesselPersistentId == oldId) {
            mappedVessels[i].currentVesselPersistentId = newId;
            flag = true;
        }
    }
    if (flag) RefreshMissionRequested();
}

// Mission.cs:1297
void Mission::OnPartPersistentIdChanged(uint32_t vesselID, uint32_t oldId, uint32_t newId) {
    bool flag = false;
    for (size_t i = 0; i < mappedVessels.size(); ++i) {
        if (mappedVessels[i].currentVesselPersistentId == vesselID && mappedVessels[i].partPersistentId == oldId) {
            mappedVessels[i].partPersistentId = newId;
            flag = true;
        }
    }
    if (flag) RefreshMissionRequested();
}

// Mission.cs:1313
void Mission::OnVesselDocking(uint32_t oldId, uint32_t newId) {
    bool flag = false;
    for (size_t i = 0; i < mappedVessels.size(); ++i) {
        if (mappedVessels[i].currentVesselPersistentId == oldId) {
            mappedVessels[i].currentVesselPersistentId = newId;
            flag = true;
        }
    }
    if (flag) RefreshMissionRequested();
}

// Mission.cs:1329
void Mission::OnVesselsUndocking(Vessel* oldVessel, Vessel* newVessel) {
    bool flag = false;
    for (size_t i = 0; i < mappedVessels.size(); ++i) {
        if (mappedVessels[i].currentVesselPersistentId == oldVessel->persistentId && newVessel->PartsContain(mappedVessels[i].partPersistentId) != nullptr) {
            mappedVessels[i].currentVesselPersistentId = newVessel->persistentId;
            flag = true;
        }
    }
    if (flag) RefreshMissionRequested();
}

// ---- ProcessMappedVesselNodeMap (Mission.cs:1964) --------------------------------------------------------------------------------

// Mission.cs:1964
void Mission::ProcessMappedVesselNodeMap(uint32_t mappedPersistentId, uint32_t newPersistentId) {
    const bool flag = newPersistentId != 0 && mappedPersistentId != 0;
    // `if (flag) { if (id == mapped) id = new; } else id = UpgradeMappedVesselId(id, node)`
    auto remap = [&](uint32_t& id, MENode* current) {
        if (flag) {
            if (id == mappedPersistentId) id = newPersistentId;
        } else {
            id = UpgradeMappedVesselId(id, current);
        }
    };
    for (MENode* current : nodes.GetListEnumerator()) {
        for (size_t i = 0; i < current->testGroups.size(); ++i) {
            for (size_t j = 0; j < current->testGroups[i]->testModules.size(); ++j) {
                ITestModule* tm = current->testGroups[i]->testModules[j].get();
                if (auto* testVessel = dynamic_cast<Tests::TestVessel*>(tm)) {
                    if (testVessel->vesselID != 0) remap(testVessel->vesselID, current);
                }
                if (auto* testDistance = dynamic_cast<Tests::TestDistance*>(tm)) {
                    if (testDistance->distanceToVesselID != 0 && testDistance->distanceToTarget == Tests::TestDistance::DistanceToChoices::Vessel)
                        remap(testDistance->distanceToVesselID, current);
                    if (testDistance->distanceFromVesselID != 0 && testDistance->distanceFromTarget == Tests::TestDistance::DistanceFromChoices::Vessel)
                        remap(testDistance->distanceFromVesselID, current);
                }
                if (auto* testPartDocking = dynamic_cast<Tests::TestPartDocking*>(tm)) {
                    if (testPartDocking->partOnevesselPartIDs->VesselID != 0) remap(testPartDocking->partOnevesselPartIDs->VesselID, current);
                    if (testPartDocking->partTwovesselPartIDs->VesselID != 0) remap(testPartDocking->partTwovesselPartIDs->VesselID, current);
                }
                if (auto* testCrewAssignment = dynamic_cast<Tests::TestCrewAssignment*>(tm)) {
                    if (testCrewAssignment->vesselPartIDs->VesselID != 0) remap(testCrewAssignment->vesselPartIDs->VesselID, current);
                }
            }
        }
        for (size_t k = 0; k < current->actionModules.size(); ++k) {
            IActionModule* am = current->actionModules[k].get();
            if (auto* a = dynamic_cast<Actions::ActionPartExplode*>(am)) {
                if (a->vesselPartIDs.VesselID != 0) remap(a->vesselPartIDs.VesselID, current);
            }
            if (auto* a = dynamic_cast<Actions::ActionPartFailure*>(am)) {
                if (a->vesselPartIDs.VesselID != 0) remap(a->vesselPartIDs.VesselID, current);
            }
            if (auto* a = dynamic_cast<Actions::ActionPartRepair*>(am)) {
                if (a->vesselPartIDs.VesselID != 0) remap(a->vesselPartIDs.VesselID, current);
            }
            if (auto* a = dynamic_cast<Actions::ActionPartResourceDrain*>(am)) {
                if (a->vesselPartIDs.VesselID != 0) remap(a->vesselPartIDs.VesselID, current);
            }
        }
    }
}

// Mission.cs:2153
void Mission::RemapVesselId(uint32_t currentVslId) {
    const int num = MappedVessels::IndexCurrentVesselId(mappedVessels, currentVslId);
    if (num != -1) {
        ProcessMappedVesselNodeMap(mappedVessels[static_cast<size_t>(num)].mappedVesselPersistentId, mappedVessels[static_cast<size_t>(num)].currentVesselPersistentId);
        mappedVessels.erase(mappedVessels.begin() + num);
    }
}

// Mission.cs:2164
uint32_t Mission::UpgradeMappedVesselId(uint32_t currentVslId, MENode* currentNode) {
    (void)currentNode;  // only used by the Debug.LogFormat line
    const int num = MappedVessels::IndexCurrentVesselId(mappedVessels, currentVslId);
    if (num != -1) return mappedVessels[static_cast<size_t>(num)].mappedVesselPersistentId;
    return currentVslId;
}

}  // namespace ksp::Expansions::Missions
