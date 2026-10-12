// WP-54: Mission.cs, MissionSituation.cs, MissionInstructor.cs, MissionPartFilter.cs, MissionMappedVessel.cs,
// MissionMappedVesselsExtension.cs, MissionPack.cs, MissionFileInfo.cs (paths), MissionsUtils.cs (pure helpers), MEBannerEntry.cs (stand-in).
// See mission.hpp.
#include "ksp/missions/mission.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <queue>
#include <unordered_map>

#include "../save/save_util.hpp"
#include "ksp/career/currency.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission_awards.hpp"
#include "ksp/missions/mission_score.hpp"
#include "ksp/missions/mission_validator.hpp"
#include "ksp/resources/planetary_resources.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/science/science.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp::Expansions::Missions {

using KSP::Localization::Localizer;

namespace {
template <class T>
void AddUniqueTo(std::vector<T>& list, const T& item) {
    if (std::find(list.begin(), list.end(), item) == list.end()) list.push_back(item);
}
template <class T>
bool ListContains(const std::vector<T>& list, const T& item) {
    return std::find(list.begin(), list.end(), item) != list.end();
}
template <class T>
void ListRemove(std::vector<T>& list, const T& item) {
    auto it = std::find(list.begin(), list.end(), item);
    if (it != list.end()) list.erase(it);
}
const char* BannerTypeName(MEBannerType t) {
    switch (t) {
        case MEBannerType::Menu: return "Menu";
        case MEBannerType::Success: return "Success";
        default: return "Fail";
    }
}
}  // namespace

// ---- MEBannerEntry (WP-56 stand-in) ------------------------------------------------------------------------------------------

// MEBannerEntry.cs:46
MEBannerEntry::MEBannerEntry(MEBannerType bannerType) { SetToDefault(bannerType); }

// MEBannerEntry.cs:117
std::string MEBannerEntry::GetBannerGameDataPath(MEBannerType bannerType) { return std::string("SquadExpansion/MakingHistory/Banners/") + BannerTypeName(bannerType) + "/"; }

// MEBannerEntry.cs:122
std::string MEBannerEntry::GetStockBannerGameDataPath(MEBannerType bannerType) { return std::string("Squad/Missions/Banners/") + BannerTypeName(bannerType) + "/"; }

// MEBannerEntry.cs:97
void MEBannerEntry::SetToDefault(MEBannerType bannerType) {
    isInMissionFolder = false;
    fileName = "default.png";
    // GetBannerDefaultTexture: the expansion's default.png when the file exists, else the stock one
    const std::filesystem::path p = std::filesystem::path(MissionsUtils::Paths::applicationRootPath) / "GameData" / GetBannerGameDataPath(bannerType) / "default.png";
    std::error_code ec;
    relativePath = std::filesystem::exists(p, ec) ? GetBannerGameDataPath(bannerType) : GetStockBannerGameDataPath(bannerType);
}

// MEBannerEntry.cs:51
void MEBannerEntry::LoadFromMissionFolder(const std::string& newFileName, MEBannerType bannerType, const Mission* mission) {
    if (!newFileName.empty()) {
        isInMissionFolder = true;
        fileName = newFileName;
        relativePath = std::string(BannerTypeName(bannerType)) + "/";
        missionFolderPath = mission->BannersPath();
    }
}

// MEBannerEntry.cs:33
std::string MEBannerEntry::FullPath(const std::string& applicationRootPath) const {
    if (isInMissionFolder && missionFolderPath != "") return missionFolderPath + relativePath + fileName;
    return applicationRootPath + "GameData/" + relativePath + fileName;
}

// ---- MissionMappedVessel -----------------------------------------------------------------------------------------------------

// MissionMappedVessel.cs:23
MissionMappedVessel::MissionMappedVessel(uint32_t partPersistentId_, uint32_t originalVesselId, uint32_t currentVesselId, const std::string& partVslName,
                                         const std::string& craftFileName_, const std::string& situationVslName) {
    static uint32_t counter = 0;
    mappedVesselPersistentId = getUniquePersistentId ? getUniquePersistentId() : ++counter;
    currentVesselPersistentId = currentVesselId;
    originalVesselPersistentId = originalVesselId;
    partPersistentId = partPersistentId_;
    partVesselName = partVslName;
    craftFileName = craftFileName_;
    situationVesselName = situationVslName;
}

// MissionMappedVessel.cs:34
std::vector<MissionMappedVessel> MissionMappedVessel::Load(const ConfigNode& node) {
    std::vector<MissionMappedVessel> list;
    for (const ConfigNode* configNode : node.GetNodes("MAPPEDVESSEL")) {
        uint32_t value = 0u;
        if (!configNode->TryGetValue("partId", value)) continue;
        uint32_t value2 = 0u;
        uint32_t value3 = 0u;
        uint32_t num = 0u;
        std::string value4;
        std::string value5;
        std::string value6;
        if (configNode->TryGetValue("mappedId", value2) && configNode->TryGetValue("currentId", value3)) {
            num = value3;
            if (configNode->TryGetValue("partVslName", value4)) {
                configNode->TryGetValue("originalId", num);
                configNode->TryGetValue("situationVslName", value6);
                configNode->TryGetValue("craftFileName", value5);
                MissionMappedVessel missionMappedVessel(value, num, value3, value4, value5, value6);
                missionMappedVessel.mappedVesselPersistentId = value2;
                list.push_back(missionMappedVessel);
            }
        }
    }
    return list;
}

// MissionMappedVessel.cs:68
ConfigNode* MissionMappedVessel::Save(ConfigNode& node, const std::vector<MissionMappedVessel>& list) {
    if (!list.empty()) {
        ConfigNode* configNode = node.AddNode("MAPPEDVESSELS");
        for (size_t i = 0; i < list.size(); ++i) {
            ConfigNode* configNode2 = configNode->AddNode("MAPPEDVESSEL");
            configNode2->AddValue("partId", list[i].partPersistentId);
            configNode2->AddValue("mappedId", list[i].mappedVesselPersistentId);
            configNode2->AddValue("originalId", list[i].originalVesselPersistentId);
            configNode2->AddValue("currentId", list[i].currentVesselPersistentId);
            configNode2->AddValue("partVslName", list[i].partVesselName);
            configNode2->AddValue("craftFileName", list[i].craftFileName);
            configNode2->AddValue("situationVslName", list[i].situationVesselName);
        }
    }
    return &node;
}

namespace MappedVessels {

// MissionMappedVesselsExtension.cs:8
int IndexPartId(const std::vector<MissionMappedVessel>& list, uint32_t id) {
    int num = 0;
    while (true) {
        if (num < static_cast<int>(list.size())) {
            if (list[static_cast<size_t>(num)].partPersistentId == id) break;
            ++num;
            continue;
        }
        return -1;
    }
    return num;
}

// MissionMappedVesselsExtension.cs:23
int IndexMappedVesselId(const std::vector<MissionMappedVessel>& list, uint32_t id) {
    int num = 0;
    while (true) {
        if (num < static_cast<int>(list.size())) {
            if (list[static_cast<size_t>(num)].mappedVesselPersistentId == id) break;
            ++num;
            continue;
        }
        return -1;
    }
    return num;
}

// MissionMappedVesselsExtension.cs:38
int IndexCurrentVesselId(const std::vector<MissionMappedVessel>& list, uint32_t id) {
    int num = 0;
    while (true) {
        if (num < static_cast<int>(list.size())) {
            if (list[static_cast<size_t>(num)].currentVesselPersistentId == id) break;
            ++num;
            continue;
        }
        return -1;
    }
    return num;
}

// MissionMappedVesselsExtension.cs:53
int MappedVesselName(const std::vector<MissionMappedVessel>& list, const std::string& vesselName) {
    int num = 0;
    while (true) {
        if (num < static_cast<int>(list.size())) {
            if (Localizer::Format(list[static_cast<size_t>(num)].partVesselName) == vesselName) break;
            ++num;
            continue;
        }
        return -1;
    }
    return num;
}

// MissionMappedVesselsExtension.cs:68
uint32_t ConvertMappedId(const std::vector<MissionMappedVessel>& list, uint32_t id) {
    uint32_t result = id;
    int num = IndexPartId(list, id);
    if (num != -1) {
        result = list[static_cast<size_t>(num)].currentVesselPersistentId;
    } else {
        int num2 = IndexMappedVesselId(list, id);
        if (num2 != -1) result = list[static_cast<size_t>(num2)].currentVesselPersistentId;
    }
    return result;
}

// MissionMappedVesselsExtension.cs:89
bool Exists(const std::vector<MissionMappedVessel>& list, uint32_t vslId, uint32_t partId, const std::string& vesselName, const std::string& craftName) {
    return Find(list, vslId, partId, vesselName, craftName) != nullptr;
}

// MissionMappedVesselsExtension.cs:111
const MissionMappedVessel* Find(const std::vector<MissionMappedVessel>& list, uint32_t vslId, uint32_t partId, const std::string& vesselName, const std::string& craftName) {
    int num = 0;
    while (true) {
        if (num < static_cast<int>(list.size())) {
            const MissionMappedVessel& m = list[static_cast<size_t>(num)];
            if ((m.currentVesselPersistentId == vslId || m.originalVesselPersistentId == vslId) && m.partPersistentId == partId && m.partVesselName == vesselName &&
                m.craftFileName == craftName)
                break;
            ++num;
            continue;
        }
        return nullptr;
    }
    return &list[static_cast<size_t>(num)];
}

}  // namespace MappedVessels

// ---- MissionInstructor -------------------------------------------------------------------------------------------------------

// MissionInstructor.cs:21
MissionInstructor::MissionInstructor() {
    instructorName = "Instructor_Gene";
    vintageSuit = false;
}

// MissionInstructor.cs:49
std::string MissionInstructor::GetNodeBodyParameterString(const Editor::BaseAPField*) {
    std::string text;
    struct Row {
        const char* name;
        const char* tag;
        bool veteran;
    };
    static const Row rows[] = {{"Instructor_Wernher", "#autoLOC_900000", false}, {"Instructor_Gene", "#autoLOC_900001", false},
                               {"Strategy_MechanicGuy", "#autoLOC_900002", false}, {"Strategy_Mortimer", "#autoLOC_900004", false},
                               {"Strategy_PRGuy", "#autoLOC_900005", false},       {"Strategy_ScienceGuy", "#autoLOC_900006", false},
                               {"Instructor_Jeb_MaleVeteran", "#autoLOC_20803", true}, {"Instructor_Bill_MaleVeteran", "#autoLOC_20811", true},
                               {"Instructor_Bob_MaleVeteran", "#autoLOC_20819", true}, {"Instructor_Val_FemaleVeteran", "#autoLOC_20827", true}};
    for (const Row& r : rows) {
        if (instructorName == r.name) {
            text = Localizer::Format("#autoLOC_8006002") + ": " + Localizer::Format(r.tag);
            if (r.veteran) text = text + "\n" + Localizer::Format("#autoLOC_8002109") + ": " + dotnet::FormatBool(vintageSuit);
            return text;
        }
    }
    return "";
}

// MissionInstructor.cs:105
bool MissionInstructor::Equals(const MissionInstructor& other) const {
    if (instructorName == other.instructorName) return vintageSuit == other.vintageSuit;
    return false;
}

// MissionInstructor.cs:117
void MissionInstructor::Save(ConfigNode& node) const {
    if (!instructorName.empty()) node.AddValue("instructorName", instructorName);
    node.AddValue("vintageSuit", vintageSuit);
}

// MissionInstructor.cs:126
void MissionInstructor::Load(const ConfigNode& node) {
    instructorName = "";
    node.TryGetValue("instructorName", instructorName);
    node.TryGetValue("vintageSuit", vintageSuit);
}

// ---- MissionPartFilter -------------------------------------------------------------------------------------------------------

// MissionPartFilter.cs:39
MissionPartFilter::MissionPartFilter(Mission* m, VesselSituation* v) : mission(m), vesselSituation(v) {}

// MissionPartFilter.cs:105
std::vector<std::string> MissionPartFilter::GetExcludedParts() {
    if (filterType == Choices::unavailableParts) {
        std::vector<std::string> list;
        for (size_t i = 0; i < unavailableParts.size(); ++i) list.push_back(unavailableParts[i]);
        return list;
    }
    const std::vector<std::string> loaded = MissionsEnvironment::loadedPartNames ? MissionsEnvironment::loadedPartNames() : std::vector<std::string>();
    std::vector<std::string> list2;
    int j = 0;
    for (int count = static_cast<int>(loaded.size()); j < count; ++j) {
        if (!ListContains(availableParts, loaded[static_cast<size_t>(j)])) {
            AddUniqueTo(list2, loaded[static_cast<size_t>(j)]);
        } else {
            log::Info(loaded[static_cast<size_t>(j)]);
        }
    }
    return list2;
}

// MissionPartFilter.cs:160
bool MissionPartFilter::CheckPartExcludelist(const std::string& partName) const {
    if (filterType == Choices::unavailableParts) {
        if (!unavailableParts.empty() &&
            std::find(unavailableParts.begin(), unavailableParts.end(), partName) != unavailableParts.end())
            return false;
    } else if (!availableParts.empty() && std::find(availableParts.begin(), availableParts.end(), partName) == availableParts.end()) {
        return false;
    }
    return true;
}

// MissionPartFilter.cs:261
void MissionPartFilter::Load(const ConfigNode& node) {
    save_detail::TryGetEnum(node, "filterType", filterType, Choices::unavailableParts);
    if (filterType == Choices::availableParts) {
        availableParts = node.GetValuesList("availableParts");
        MissionsUtils::UpdatePartNames(availableParts);
    } else {
        unavailableParts = node.GetValuesList("unavailableParts");
        MissionsUtils::UpdatePartNames(unavailableParts);
    }
}

// MissionPartFilter.cs:276
void MissionPartFilter::Save(ConfigNode& node) const {
    if (filterType == Choices::availableParts) {
        for (size_t i = 0; i < availableParts.size(); ++i) {
            if (i == 0) node.AddValue("filterType", dotnet::EnumToString(filterType));
            node.AddValue("availableParts", availableParts[i]);
        }
        return;
    }
    for (size_t j = 0; j < unavailableParts.size(); ++j) {
        if (j == 0) node.AddValue("filterType", dotnet::EnumToString(filterType));
        node.AddValue("unavailableParts", unavailableParts[j]);
    }
}

// ---- MissionSituation --------------------------------------------------------------------------------------------------------

// MissionSituation.cs:211
MissionSituation::MissionSituation() {
    currentVesselToBuildId = 0u;
    vesselsBuilt = 0;
    vesselsToBuildNode = std::make_shared<ConfigNode>();
    showBriefing = false;
    missionRoster = Localizer::Format("#autoLOC_8003056");
}

// MissionSituation.cs:224
MissionSituation::MissionSituation(Mission* m) : MissionSituation() {
    mission = m;
    partFilter = std::make_unique<MissionPartFilter>(m, nullptr);
    if (MissionsEnvironment::createSituationDefaults) {
        crewRoster = KerbalRoster::GenerateInitialCrewRoster(Game::Modes::MISSION);
        gameParameters = GameParameters::GetDefaultParameters(Game::Modes::MISSION, GameParameters::Preset::Normal);
        int i = 0;
        for (int count = crewRoster->Count(); i < count; ++i) KerbalRoster::SetExperienceLevel(*(*crewRoster)[i], 5, *gameParameters, Game::Modes::MISSION);
    }
}

MissionSituation::~MissionSituation() = default;

// MissionSituation.cs:125
bool MissionSituation::VesselsArePending() const { return NumberofVesselsPending() > 0; }

// MissionSituation.cs:137
int MissionSituation::NumberofVesselsPending() const {
    int num = 0;
    for (int i = 0; i < vesselSituationList.Count(); ++i) {
        VesselSituation* vesselSituation = vesselSituationList.KeyAt(i);
        if (vesselSituation->playerCreated && !vesselSituation->readyToLaunch) ++num;
    }
    return num;
}

// MissionSituation.cs:156
VesselSituation* MissionSituation::CurrentVesselToBuild() {
    if (currentVesselToBuildId == 0) return nullptr;
    int num = 0;
    while (true) {
        if (num < vesselSituationList.Count()) {
            if (vesselSituationList.KeyAt(num)->persistentId == currentVesselToBuildId) break;
            ++num;
            continue;
        }
        return nullptr;
    }
    return vesselSituationList.KeyAt(num);
}

// MissionSituation.cs:184
MENode* MissionSituation::StartNode() {
    if (startNode_ == nullptr) {
        for (MENode* n : mission->nodes.GetListEnumerator()) {
            if (n->isStartNode) {
                startNode_ = n;
                break;
            }
        }
    }
    return startNode_;
}

// MissionSituation.cs:238
void MissionSituation::InitSituation() {
    if (HighLogic::LoadedScene() == GameScenes::EDITOR) {
        // partFilter.ApplyPartsFilter(): EditorPartList.Instance exclude filters (editor UI, not modelled)
    }
    if (ResourceScenario::Instance() != nullptr) {
        ResourceScenario::Instance()->gameSettings()->Seed = resourceSeed;
        if (rocMissionSeed != 0) ResourceScenario::Instance()->gameSettings()->ROCMissionSeed = rocMissionSeed;
    }
}

// MissionSituation.cs:254
void MissionSituation::DestroySituation() {
    // partFilter.RemovePartsFilter(): editor part list filters (not modelled)
}

// MissionSituation.cs:303
void MissionSituation::RunValidationWrapper(Editor::MissionEditorValidator* validator) { RunValidation(validator); }

// MissionSituation.cs:312
void MissionSituation::Load(const ConfigNode& node) {
    save_detail::TryGetEnum(node, "startType", startType, StartTypeEnum::VesselList);
    node.TryGetValue("startVessel", startVesselID);
    node.TryGetValue("autoGenerateCrew", autoGenerateCrew);
    vesselsToBuildNode = std::make_shared<ConfigNode>();
    ConfigNode* found = nullptr;
    if (node.TryGetNode("VESSELSTOBUILDLIST", found)) vesselsToBuildNode = found->CreateCopy();
    startingActionsNode = std::make_shared<ConfigNode>();
    if (node.TryGetNode("STARTINGACTIONSLIST", found)) startingActionsNode = found->CreateCopy();
    ConfigNode* node2 = nullptr;
    if (node.TryGetNode("ROSTER", node2)) crewRoster = std::make_shared<KerbalRoster>(*node2, Game::Modes::MISSION);
    node.TryGetValue("startUT", startUT);
    ConfigNode* node3 = nullptr;
    if (node.TryGetNode("PARAMETERS", node3)) gameParameters->Load(*node3);
    ConfigNode* node4 = nullptr;
    if (node.TryGetNode("SAP_PARAMETERS", node4)) parametersDisplayedInSAP = node4->GetValuesList("parametersDisplayedInSAP");
    partFilter->Load(node);
    node.TryGetValue("resourceSeed", resourceSeed);
    node.TryGetValue("rocMissionSeed", rocMissionSeed);
    node.TryGetValue("showBriefing", showBriefing);
    missionInstructor.Load(node);
}

// MissionSituation.cs:344
void MissionSituation::setStartType() {
    startType = StartTypeEnum::SpaceCenter;
    std::vector<MENode*>& dockedNodes = StartNode()->dockedNodes;
    bool flag = false;
    for (size_t i = 0; i < dockedNodes.size(); ++i) {
        auto& actionModules = dockedNodes[i]->actionModules;
        for (size_t j = 0; j < actionModules.size(); ++j) {
            if (actionModules[j]->TypeName() == "ActionCreateVessel") {
                auto* actionCreateVessel = dynamic_cast<IActionCreateVessel*>(actionModules[j].get());
                if (!(actionCreateVessel != nullptr)) continue;
                VesselSituation* vs = actionCreateVessel->GetVesselSituation();
                if (vs->playerCreated) {
                    if (startType == StartTypeEnum::SpaceCenter || startType == StartTypeEnum::VesselList)
                        startType = (vs->location != nullptr && vs->location->facility == EditorFacility::SPH) ? StartTypeEnum::SPH : StartTypeEnum::VAB;
                } else if (startType == StartTypeEnum::SpaceCenter) {
                    startType = StartTypeEnum::VesselList;
                }
                if (!flag) {
                    startVesselID = vs->persistentId;
                    flag = true;
                }
            } else if (actionModules[j]->TypeName() == "ActionCreateKerbal") {
                auto* actionCreateKerbal = dynamic_cast<IActionCreateKerbal*>(actionModules[j].get());
                if (actionCreateKerbal != nullptr) {
                    if (startType == StartTypeEnum::SpaceCenter) startType = StartTypeEnum::VesselList;
                    if (!flag) {
                        startVesselID = actionCreateKerbal->GetPersistentId();
                        flag = true;
                    }
                }
            } else if (actionModules[j]->TypeName() == "ActionCreateAsteroid") {
                auto* actionCreateAsteroid = dynamic_cast<IActionCreateAsteroid*>(actionModules[j].get());
                if (actionCreateAsteroid != nullptr) {
                    if (startType == StartTypeEnum::SpaceCenter) startType = StartTypeEnum::VesselList;
                    if (!flag) {
                        startVesselID = actionCreateAsteroid->GetPersistentID();
                        flag = true;
                    }
                }
            } else {
                if (!(actionModules[j]->TypeName() == "ActionCreateComet")) continue;
                auto* actionCreateComet = dynamic_cast<IActionCreateComet*>(actionModules[j].get());
                if (actionCreateComet != nullptr) {
                    if (startType == StartTypeEnum::SpaceCenter) startType = StartTypeEnum::VesselList;
                    if (!flag) {
                        startVesselID = actionCreateComet->GetPersistentID();
                        flag = true;
                    }
                }
            }
        }
    }
}

// MissionSituation.cs:442
void MissionSituation::SaveVesselsToBuild(ConfigNode& node) {
    if (vesselsToBuildNode) node.AddNode("VESSELSTOBUILDLIST", vesselsToBuildNode);
    if (startingActionsNode) node.AddNode("STARTINGACTIONSLIST", startingActionsNode);
}

// MissionSituation.cs:454
void MissionSituation::Save(ConfigNode& node) {
    node.AddValue("startType", dotnet::EnumToString(startType));
    node.AddValue("startVessel", startVesselID);
    node.AddValue("autoGenerateCrew", autoGenerateCrew);
    if (HighLogic::LoadedScene() == GameScenes::MISSIONBUILDER) {
        setStartType();
        node.SetValue("startType", dotnet::EnumToString(startType));
        node.SetValue("startVessel", startVesselID, true);
    }
    if (AwardEnvironment::CurrentGameIsMission()) {
        if (vesselSituationList.Count() > 0) {
            ConfigNode* configNode = node.AddNode("VESSELSTOBUILDLIST");
            for (int i = 0; i < vesselSituationList.Count(); ++i) configNode->AddNode("VESSELTOBUILD")->AddValue("NodeGuid", vesselSituationList.At(i).ToString());
        }
        if (!startingActions.empty()) {
            ConfigNode* configNode2 = node.AddNode("STARTINGACTIONSLIST");
            for (size_t j = 0; j < startingActions.size(); ++j) configNode2->AddNode("STARTINGACTION")->AddValue("NodeGuid", startingActions[j]->GetNode()->id().ToString());
        }
    }
    if (crewRoster) {
        ConfigNode* node2 = node.AddNode("ROSTER");
        crewRoster->Save(*node2);
    }
    node.AddValue("startUT", startUT);
    if (gameParameters) {
        ConfigNode* node3 = node.AddNode("PARAMETERS");
        gameParameters->Save(*node3);
    }
    ConfigNode* configNode3 = node.AddNode("SAP_PARAMETERS");
    for (size_t k = 0; k < parametersDisplayedInSAP.size(); ++k) configNode3->AddValue("parametersDisplayedInSAP", parametersDisplayedInSAP[k]);
    partFilter->Save(node);
    if (ResourceScenario::Instance() != nullptr) {
        node.AddValue("resourceSeed", ResourceScenario::Instance()->gameSettings()->Seed);
        node.AddValue("rocMissionSeed", ResourceScenario::Instance()->gameSettings()->ROCMissionSeed);
    } else {
        node.AddValue("resourceSeed", resourceSeed);
        node.AddValue("rocMissionSeed", rocMissionSeed);
    }
    node.AddValue("showBriefing", showBriefing);
    missionInstructor.Save(node);
}

// MissionSituation.cs:509
VesselSituation* MissionSituation::GetVesselSituationByVesselID(uint32_t PersistentId) {
    int num = 0;
    while (true) {
        if (num < vesselSituationList.Count()) {
            if (vesselSituationList.KeyAt(num)->persistentId == PersistentId) break;
            ++num;
            continue;
        }
        return nullptr;
    }
    return vesselSituationList.KeyAt(num);
}

// MissionSituation.cs:528
bool MissionSituation::VesselSituationListContains(uint32_t persistentId) {
    int num = 0;
    while (true) {
        if (num < vesselSituationList.Count()) {
            if (vesselSituationList.KeyAt(num)->persistentId == persistentId) break;
            ++num;
            continue;
        }
        return false;
    }
    return true;
}

// MissionSituation.cs:547
void MissionSituation::FillVesselSituationList(MENode* missionStartNode, bool replaceIfFound, bool fillAllNodes) {
    vesselSituationList.Clear();
    startingActions.clear();
    std::vector<MENode*> list = (!fillAllNodes) ? missionStartNode->dockedNodes : mission->nodes.ValuesList();
    for (size_t i = 0; i < list.size(); ++i) {
        auto& actionModules = list[i]->actionModules;
        for (size_t j = 0; j < actionModules.size(); ++j) {
            if (actionModules[j]->TypeName() == "ActionCreateVessel") {
                auto* actionCreateVessel = dynamic_cast<IActionCreateVessel*>(actionModules[j].get());
                if (!(actionCreateVessel != nullptr)) continue;
                VesselSituation* vs = actionCreateVessel->GetVesselSituation();
                if (vs->playerCreated) {
                    vs->readyToLaunch = false;
                    if (fillAllNodes && MissionsEnvironment::createPlaceholderCraft) MissionsEnvironment::createPlaceholderCraft(actionModules[j].get());
                } else {
                    vs->readyToLaunch = true;
                }
                if (!replaceIfFound && VesselSituationListContains(vs->persistentId)) {
                    log::Info("[MissionSituation]: Skipped adding " + vs->vesselName + " to Vessel Create List");
                } else if (vesselSituationList.Add(vs, dynamic_cast<ActionModule*>(actionModules[j].get())->node->id())) {
                    log::Info("[MissionSituation]: Replaced " + vs->vesselName + " in Vessel Create List");
                }
            } else {
                startingActions.push_back(actionModules[j].get());
            }
        }
    }
    UpdateVesselBuildValues();
}

// MissionSituation.cs:599
void MissionSituation::resetVesselsCountsLists() {
    currentVesselToBuildId = 0u;
    vesselsBuilt = 0;
}

// MissionSituation.cs:605
void MissionSituation::UpdateVesselBuildValues() {
    if (!VesselsArePending()) return;
    vesselsBuilt = 0;
    for (int i = 0; i < vesselSituationList.Count(); ++i) {
        VesselSituation* v = vesselSituationList.KeyAt(i);
        if (v->playerCreated && (!v->playerCreated || !v->launched)) {
            if (v->readyToLaunch) {
                ++vesselsBuilt;
            } else if (currentVesselToBuildId == 0) {
                currentVesselToBuildId = v->persistentId;
                GameEvents::Mission::onMissionCurrentVesselToBuildChanged.Fire(mission, v);
            }
        }
    }
}

// MissionSituation.cs:629
std::vector<VesselSituation*> MissionSituation::GetAvailableVesselSituations() {
    std::vector<VesselSituation*> list;
    for (MENode* n : mission->nodes.GetListEnumerator()) {
        for (auto& am : n->actionModules) {
            if (am->TypeName() == "ActionCreateVessel") {
                if (auto* actionCreateVessel = dynamic_cast<IActionCreateVessel*>(am.get())) list.push_back(actionCreateVessel->GetVesselSituation());
            }
        }
    }
    return list;
}

// MissionSituation.cs:655
bool MissionSituation::AddNodeWithVesselToBuild(MENode* node) {
    if (AwardEnvironment::CurrentGameIsMission()) {
        if (mission->GetNodeById(node->id()) == nullptr) return false;
        auto& actionModules = node->actionModules;
        for (size_t i = 0; i < actionModules.size(); ++i) {
            auto* actionCreateVessel = dynamic_cast<IActionCreateVessel*>(actionModules[i].get());
            if (actionCreateVessel != nullptr) {
                VesselSituation* vs = actionCreateVessel->GetVesselSituation();
                if (!VesselSituationListContains(vs->persistentId)) {
                    vesselSituationList.Add(vs, node->id());
                    return true;
                }
                log::Info("[MissionSituation]: Skipped adding duplicate " + vs->vesselName + " to Vessel Create List");
                return false;
            }
        }
    }
    return false;
}

// MissionSituation.cs:682
bool MissionSituation::VesselSituationReadyToLaunch(VesselSituation* situation) {
    const Guid nodeGuidByVesselID = mission->GetNodeGuidByVesselID(situation->persistentId);
    if (!(nodeGuidByVesselID == Guid::Empty())) {
        MENode* nodeById = mission->GetNodeById(nodeGuidByVesselID);
        if (nodeById != nullptr) {
            IActionCreateVessel* actionCreateVessel = mission->GetActionCreateVessel(nodeById);
            if (actionCreateVessel != nullptr) {
                actionCreateVessel->GetVesselSituation()->readyToLaunch = true;
                return true;
            }
        }
    }
    return false;
}

// MissionSituation.cs:701
bool MissionSituation::VesselSituationRevertLaunch(VesselSituation* situation) {
    const Guid nodeGuidByVesselID = mission->GetNodeGuidByVesselID(situation->persistentId);
    if (!(nodeGuidByVesselID == Guid::Empty())) {
        MENode* nodeById = mission->GetNodeById(nodeGuidByVesselID);
        if (nodeById != nullptr) {
            IActionCreateVessel* actionCreateVessel = mission->GetActionCreateVessel(nodeById);
            if (actionCreateVessel != nullptr) {
                actionCreateVessel->GetVesselSituation()->readyToLaunch = false;
                return true;
            }
        }
    }
    return false;
}

// MissionSituation.cs:720
bool MissionSituation::VesselSituationLaunched(VesselSituation* situation) {
    const Guid nodeGuidByVesselID = mission->GetNodeGuidByVesselID(situation->persistentId);
    if (!(nodeGuidByVesselID == Guid::Empty())) {
        MENode* nodeById = mission->GetNodeById(nodeGuidByVesselID);
        if (nodeById != nullptr) {
            IActionCreateVessel* actionCreateVessel = mission->GetActionCreateVessel(nodeById);
            if (actionCreateVessel != nullptr) {
                actionCreateVessel->GetVesselSituation()->launched = true;
                return true;
            }
        }
    }
    return false;
}

// MissionSituation.cs:739
void MissionSituation::RemoveLaunchedVessels() {
    std::vector<VesselSituation*> list;
    for (int i = 0; i < vesselSituationList.Count(); ++i) {
        if (vesselSituationList.KeyAt(i)->launched) list.push_back(vesselSituationList.KeyAt(i));
    }
    for (size_t j = 0; j < list.size(); ++j) vesselSituationList.Remove(list[j]);
}

// MissionSituation.cs:755
std::string MissionSituation::GetName() { return "MissionSituation"; }

// MissionSituation.cs:760
std::string MissionSituation::GetDisplayName() { return Localizer::Format("#autoLOC_8004154"); }

// MissionSituation.cs:765
void MissionSituation::AddParameterToSAP(const std::string& parameter) { AddUniqueTo(parametersDisplayedInSAP, parameter); }

// MissionSituation.cs:770
void MissionSituation::RemoveParameterFromSAP(const std::string& parameter) {
    ListRemove(parametersDisplayedInSAP, parameter);
    if (StartNode()->HasNodeBodyParameter("MissionSituation", parameter)) {
        StartNode()->RemoveParameterFromNodeBody("MissionSituation", parameter);
        UpdateNodeBodyUI();
    }
}

// MissionSituation.cs:780
void MissionSituation::AddParameterToNodeBody(const std::string& parameter) { StartNode()->AddParameterToNodeBody("MissionSituation", parameter); }

// MissionSituation.cs:785
void MissionSituation::AddParameterToNodeBodyAndUpdateUI(const std::string& parameter) {
    StartNode()->AddParameterToNodeBody("MissionSituation", parameter);
    UpdateNodeBodyUI();
}

// MissionSituation.cs:791
void MissionSituation::RemoveParameterFromNodeBody(const std::string& parameter) { StartNode()->RemoveParameterFromNodeBody("MissionSituation", parameter); }

// MissionSituation.cs:796
void MissionSituation::RemoveParameterFromNodeBodyAndUpdateUI(const std::string& parameter) {
    StartNode()->RemoveParameterFromNodeBody("MissionSituation", parameter);
    UpdateNodeBodyUI();
}

// MissionSituation.cs:802
bool MissionSituation::HasNodeBodyParameter(const std::string& parameter) { return StartNode()->HasNodeBodyParameter("MissionSituation", parameter); }

// MissionSituation.cs:807
bool MissionSituation::HasSAPParameter(const std::string& parameter) { return ListContains(parametersDisplayedInSAP, parameter); }

// MissionSituation.cs:812
std::string MissionSituation::GetNodeBodyParameterString(const Editor::BaseAPField&) { return ""; }

// MissionSituation.cs:856
void MissionSituation::UpdateNodeBodyUI() { StartNode()->guiNode->DisplayNodeBodyParameters(); }

// MissionSituation.cs:861
std::vector<IMENodeDisplay*> MissionSituation::GetInternalParametersToDisplay() { return {}; }

// MissionSituation.cs:866
MENode* MissionSituation::GetNode() { return nullptr; }

// MissionSituation.cs:871
std::string MissionSituation::GetInfo() { return Localizer::Format("#autoLOC_8004153"); }

// MissionSituation.cs:876
void MissionSituation::ParameterSetupComplete() {
    // OnShowBriefingChange(showBriefing): instructorControl.gameObject.SetActive(value) (editor UI)
}

// ---- MissionPack -------------------------------------------------------------------------------------------------------------

// MissionPack.cs:24
MissionPack::MissionPack() {
    name = displayName = description = "";
    order = INT32_MAX;
    color = "#5576AEFF";
}

// MissionPack.cs:31
int MissionPack::CompareOrder(const MissionPack& a, const MissionPack& b) {
    int num = (a.order < b.order) ? -1 : (a.order > b.order ? 1 : 0);
    if (num == 0) num = CompareDisplayName(a, b);
    return num;
}

// MissionPack.cs:41
int MissionPack::CompareDisplayName(const MissionPack& a, const MissionPack& b) { return dotnet::CompareCulture(a.displayName, b.displayName); }

// MissionPack.cs:46
void MissionPack::Load(const ConfigNode& node) {
    if (!node.TryGetValue("name", name)) name = Localizer::Format("#autoLOC_168872");
    if (!node.TryGetValue("displayName", displayName)) displayName = Localizer::Format("#autoLOC_168872");
    if (!node.TryGetValue("description", description)) description = Localizer::Format("#autoLOC_168872");
    node.TryGetValue("order", order);
    node.TryGetValue("color", color);
}

// MissionPack.cs:64
void MissionPack::Save(ConfigNode& node) const {
    node.AddValue("name", name);
    node.AddValue("displayName", displayName);
    node.AddValue("description", description);
    node.AddValue("order", order);
    node.AddValue("color", color);
}

// ---- MissionsUtils + MissionFileInfo paths -----------------------------------------------------------------------------------

namespace MissionsUtils {

// MissionsUtils.cs:178
std::string ExpansionRootPath() { return Paths::expansionsGameDataPath + "MakingHistory/"; }
// MissionsUtils.cs:180
std::string StockMissionsPath() { return ExpansionRootPath() + "Missions/"; }
// MissionsUtils.cs:182
std::string BaseMissionsPath() { return Paths::applicationRootPath + "GameData/Squad/Missions/"; }
// MissionsUtils.cs:184
std::string UsersMissionsPath() { return Paths::applicationRootPath + "Missions/"; }
// MissionsUtils.cs:194
std::string SavesPath() {
    if (!Paths::isTestMode && !Paths::loadedSceneIsMissionBuilder) return "missions/";
    return "test_missions/";
}
// MissionsUtils.cs:192
std::string SavesRootPath() { return Paths::applicationRootPath + "saves/" + SavesPath(); }

// MissionsUtils.cs:320
std::string GetMissionsFolder(MissionTypes missionType) {
    switch (missionType) {
        default: return StockMissionsPath();
        case MissionTypes::User: return UsersMissionsPath();
        case MissionTypes::Steam: return Paths::steamWorkshopFolder;
        case MissionTypes::Base: return BaseMissionsPath();
    }
}

// MissionsUtils.cs:582
void UpdatePartNames(std::vector<std::string>& partNames) {
    for (size_t i = 0; i < partNames.size(); ++i) partNames[i] = UpdatePartName(partNames[i]);
}

// MissionsUtils.cs:593
std::string UpdatePartName(const std::string& partName) {
    static const std::pair<const char*, const char*> table[] = {{"mk1pod", "mk1pod.v2"},
                                                                {"probeCoreHex", "probeCoreHex.v2"},
                                                                {"probeCoreOcto", "probeCoreOcto.v2"},
                                                                {"probeCoreOcto2", "probeCoreOcto2.v2"},
                                                                {"probeCoreSphere", "probeCoreSphere.v2"},
                                                                {"roverBody", "roverBody.v2"},
                                                                {"solidBooster.sm", "solidBooster.sm.v2"},
                                                                {"solidBooster", "solidBooster.v2"},
                                                                {"mk2LanderCabin", "mk2LanderCabin.v2"},
                                                                {"liquidEngineMini", "liquidEngineMini.v2"},
                                                                {"liquidEngine2-2", "liquidEngine2-2.v2"},
                                                                {"liquidEngine3", "liquidEngine3.v2"},
                                                                {"Size3To2Adapter", "Size3To2Adapter.v2"},
                                                                {"stackBiCoupler", "stackBiCoupler.v2"},
                                                                {"stackTriCoupler", "stackTriCoupler.v2"},
                                                                {"smallRadialEngine", "smallRadialEngine.v2"},
                                                                {"microEngine", "microEngine.v2"},
                                                                {"radialEngineMini", "radialEngineMini.v2"},
                                                                {"fuelTank1-2", "Rockomax32.BW"},
                                                                {"fuelTank2-2", "Rockomax16.BW"},
                                                                {"fuelTank3-2", "Rockomax64.BW"},
                                                                {"fuelTank4-2", "Rockomax8BW"},
                                                                {"Mark1-2Pod", "mk1-3pod"},
                                                                {"stackDecoupler", "Decoupler.1"},
                                                                {"decoupler1-2", "Decoupler.2"},
                                                                {"stackDecouplerMini", "Decoupler.0"},
                                                                {"size3Decoupler", "Decoupler.3"},
                                                                {"stackSeparator", "Separator.1"},
                                                                {"stackSeparatorBig", "Separator.2"},
                                                                {"stackSeparatorMini", "Separator.0"},
                                                                {"toroidalFuelTank", "externalTankToroid"}};
    for (const auto& kv : table)
        if (partName == kv.first) return kv.second;
    return partName;
}

// MissionsUtils.cs:58 BreadthSearch over MissionsUtils.Objectives.Graph (:15-:56): AdjacencyList[vertex] is a HashSet<MENode>; the visit order is the
// queue order, children of a node are taken in the HashSet's enumeration order (insertion order while nothing is removed)
std::vector<MENode*> BreadthSearch(const std::vector<MENode*>& vertices, const std::vector<std::pair<MENode*, MENode*>>& edges, MENode* start) {
    // Graph(vertices, edges): AddVertex resets the set (`AdjacencyList[vertex] = new HashSet`), AddEdge adds when both ends are vertices
    std::unordered_map<MENode*, std::vector<MENode*>> adjacency;
    for (MENode* v : vertices) adjacency[v] = {};
    for (const auto& e : edges) {
        if (adjacency.count(e.first) && adjacency.count(e.second)) {
            auto& set = adjacency[e.first];
            if (!ListContains(set, e.second)) set.push_back(e.second);
        }
    }
    std::vector<MENode*> visited;
    if (!adjacency.count(start)) return visited;
    std::queue<MENode*> queue;
    queue.push(start);
    while (!queue.empty()) {
        MENode* n = queue.front();
        queue.pop();
        if (ListContains(visited, n)) continue;
        visited.push_back(n);
        for (MENode* item : adjacency[n])
            if (!ListContains(visited, item)) queue.push(item);
    }
    return visited;
}

// MissionsUtils.cs:521
std::vector<MENode*> GetNodesOrdered(Mission* m, bool objectivesOnly) {
    std::vector<std::pair<MENode*, MENode*>> list;
    for (MENode* values : m->nodes.ValuesList()) {
        for (MENode* fromNode : values->fromNodes) list.emplace_back(fromNode, values);
    }
    const std::vector<MENode*> hashSet = BreadthSearch(m->nodes.ValuesList(), list, m->startNode());
    std::vector<MENode*> list2;
    for (MENode* item : hashSet) {
        if (!objectivesOnly || item->isObjective) list2.push_back(item);
    }
    return list2;
}

}  // namespace MissionsUtils

// MissionFileInfo.cs:98
std::string MissionFileInfo::FolderPath() const { return MissionsUtils::GetMissionsFolder(missionType) + folderName + "/"; }
// MissionFileInfo.cs:100
std::string MissionFileInfo::ShipFolderPath() const { return FolderPath() + "Ships/"; }
// MissionFileInfo.cs:102
std::string MissionFileInfo::FilePath() const { return FolderPath() + "persistent" + MissionsUtils::fileExtension; }
// MissionFileInfo.cs:116
std::string MissionFileInfo::BannerPath() const { return FolderPath() + "Banners/"; }
// MissionFileInfo.cs:118
std::string MissionFileInfo::SaveFolderPath() const { return MissionsUtils::SavesRootPath() + folderName + "/"; }

// ---- Mission -----------------------------------------------------------------------------------------------------------------

// Mission.cs:314 Awake (+ :314 Spawn)
Mission::Mission() {
    InitSeed();
    id_ = MissionsEnvironment::NewGuid();
    situation = std::make_unique<MissionSituation>(this);
    flow = std::make_unique<MissionFlow>(this);
    currentScore = 0.0f;
    isSuccesful_ = false;
    isEnded_ = false;
    isStarted_ = false;
    isInitialized_ = false;
    hasBeenValidated = false;
    globalScore = std::make_unique<MissionScore>(nullptr);
    awards = std::make_unique<MissionAwards>(nullptr);
    flagURL = "Squad/Flags/default";
    order = INT32_MAX;
    GameEvents::onGameSceneSwitchRequested.Add(this, &Mission::OnGameSceneSwitchRequested);
    Game* g = HighLogic::CurrentGame();
    if (g != nullptr && g->Mode == Game::Modes::MISSION) RegisterGameEvents();
}

// Mission.cs:323
Mission::Mission(std::shared_ptr<MissionFileInfo> info) : Mission() {
    missionInfo = std::move(info);
    gameObjectName = GetGameObjectName(*missionInfo);
}

// Mission.cs:286 OnDestroy
Mission::~Mission() {
    Game* g = HighLogic::CurrentGame();
    if (g != nullptr && g->Mode == Game::Modes::MISSION && (HighLogic::LoadedScene() == GameScenes::FLIGHT || HighLogic::LoadedScene() == GameScenes::SPACECENTER))
        awards->StopTracking();
    GameEvents::onGameSceneSwitchRequested.Remove(this, &Mission::OnGameSceneSwitchRequested);
}

// Mission.cs:331
std::unique_ptr<Mission> Mission::SpawnAndLoad(std::shared_ptr<MissionFileInfo> info, const ConfigNode& missionNode) {
    auto mission = std::make_unique<Mission>();
    mission->missionInfo = info;
    mission->gameObjectName = GetGameObjectName(*info);
    mission->Load(missionNode, false);
    mission->InitMission();
    Flow::MEFlowParser::ParseMission(mission.get());
    return mission;
}

// Mission.cs:342
void Mission::InitSeed() { seed_ = static_cast<int32_t>(MissionsEnvironment::NowTicks() & 0x7FFFFFFFLL); }

// Mission.cs:347
std::string Mission::GetGameObjectName(const MissionFileInfo& info) {
    const std::string text = info.idName.empty() ? Localizer::Format(info.title) : info.idName;
    return "Mission (" + text + ")";
}

// Mission.cs:354
std::string Mission::GetGameObjectName(const Mission& mission) {
    const std::string text = mission.idName_.empty() ? Localizer::Format(mission.title_) : mission.idName_;
    return "Mission (" + text + ")";
}

// Mission.cs:137
void Mission::SetTitle(const std::string& value) {
    title_ = value;
    gameObjectName = GetGameObjectName(*this);
}

// Mission.cs:361
bool Mission::CheckHash() {
    if (MissionsEnvironment::MakingHistoryInstalled()) return true;
    if (MissionsEnvironment::makingHistorySignedMission && MissionsEnvironment::makingHistorySignedMission()) return true;
    log::Error("[Mission]: Mission Save failed Validation Check.");
    return false;
}

// Mission.cs:375
void Mission::RegenerateMissionID() {
    id_ = MissionsEnvironment::NewGuid();
    packName = "";
}

// Mission.cs:381
void Mission::OnGameSceneSwitchRequested(GameEvents::FromToAction<GameScenes, GameScenes>) {
    for (int i = 0; i < nodes.Count(); ++i) nodes.ValuesList()[static_cast<size_t>(i)]->ClearTestGroups();
}

// Mission.cs:389
void Mission::RegisterGameEvents() {
    // the vessel / part event handlers (OnPartVesselNamingChanged ... OnVesselsUndocking) belong to the craft-file bookkeeping (deferred)
    registeredGameEvents = true;
}

// Mission.cs:533
void Mission::SetStartNode() {
    for (MENode* current : nodes.GetListEnumerator()) {
        if (current->isStartNode) {
            startNode_ = current;
            break;
        }
    }
}

// Mission.cs:555
void Mission::SetActiveNode() {
    if (nodes.Contains(loadActiveNodeID_)) {
        activeNode_ = nodes[loadActiveNodeID_];
    } else {
        activeNode_ = startNode_;
    }
    activeNodeName_ = activeNode_->Title();
    loadActiveNodeID_ = Guid::Empty();
}

// Mission.cs:569
void Mission::SetDockedNodes() {
    for (MENode* current : nodes.GetListEnumerator()) {
        if (nodes.ContainsKey(current->dockParentNodeID)) current->dockParentNode = nodes[current->dockParentNodeID];
        if (current->dockedNodesIDsOnLoad.empty()) continue;
        for (size_t i = 0; i < current->dockedNodesIDsOnLoad.size(); ++i) {
            if (nodes.ContainsKey(current->dockedNodesIDsOnLoad[i])) {
                AddUniqueTo(current->dockedNodes, nodes[current->dockedNodesIDsOnLoad[i]]);
                continue;
            }
            log::Info("[Mission]: (" + title_ + ") SetDockedNodes found invalid docked Node reference from " + current->Title() + "(" + current->id().ToString() +
                      ") contains reference to " + current->dockedNodesIDsOnLoad[i].ToString() + ", ignoring.");
        }
    }
}

// Mission.cs:401
void Mission::InitMission() {
    orphanNodes_.clear();
    SetStartNode();
    for (MENode* current : nodes.GetListEnumerator()) {
        current->fromNodes.clear();
        if (!current->fromNodeIDs.empty()) {
            for (size_t i = 0; i < current->fromNodeIDs.size(); ++i) {
                if (nodes.ContainsKey(current->fromNodeIDs[i])) current->fromNodes.push_back(nodes[current->fromNodeIDs[i]]);
            }
        } else if (current->IsOrphanNode()) {
            orphanNodes_.push_back(current);
        }
    }
    for (MENode* current2 : nodes.GetListEnumerator()) {
        if (current2->toNodeIDs.empty()) continue;
        for (size_t j = 0; j < current2->toNodeIDs.size(); ++j) {
            if (nodes.ContainsKey(current2->toNodeIDs[j]) && ListContains(nodes[current2->toNodeIDs[j]]->fromNodes, current2))
                AddUniqueTo(current2->toNodes, nodes[current2->toNodeIDs[j]]);
        }
        current2->toNodeIDs.clear();
    }
    for (MENode* current3 : nodes.GetListEnumerator()) {
        for (size_t k = 0; k < current3->fromNodes.size(); ++k) AddUniqueTo(current3->fromNodes[k]->toNodes, current3);
        if (!current3->isEndNode) {
            current3->toNodes.insert(current3->toNodes.end(), orphanNodes_.begin(), orphanNodes_.end());
            if (current3->IsOrphanNode()) ListRemove(current3->toNodes, current3);
        }
    }
    for (MENode* current4 : nodes.GetListEnumerator()) {
        if (ListContains(current4->toNodes, current4)) ListRemove(current4->toNodes, current4);
    }
    if (!isInitialized_) {
        isInitialized_ = true;
        situation->InitSituation();
    }
    DictionaryValueList<VesselSituation*, Guid> dictionaryValueList;
    for (int l = 0; l < situation->VesselSituationList().Count(); ++l) {
        if (!situation->VesselSituationList().KeyAt(l)->craftFile.empty() && !craftFileExists(situation->VesselSituationList().KeyAt(l)->craftFile))
            dictionaryValueList.Add(situation->VesselSituationList().KeyAt(l), situation->VesselSituationList().At(l));
    }
    for (int m = 0; m < dictionaryValueList.Count(); ++m) {
        log::Warning("[Mission]: Vessel " + dictionaryValueList.KeyAt(m)->vesselName + " removed from mission as filename:" + dictionaryValueList.KeyAt(m)->craftFile + " not found.");
        situation->VesselSituationList().Remove(dictionaryValueList.KeyAt(m));
    }
    if (dictionaryValueList.Count() > 0) MissionsEnvironment::PostScreenMessage("#autoLOC_8006009", 10.0f);
    if (HighLogic::LoadedScene() != GameScenes::EDITOR) {
        GameEvents::onEditorStarted.Add(this, &Mission::OnEditorStarted);
    } else {
        OnEditorStarted();
    }
    Game* g = HighLogic::CurrentGame();
    if (g != nullptr && g->Mode == Game::Modes::MISSION && (HighLogic::LoadedScene() == GameScenes::FLIGHT || HighLogic::LoadedScene() == GameScenes::SPACECENTER))
        awards->StartTracking();
}

// Mission.cs:1195
void Mission::OnEditorStarted() {
    Game* g = HighLogic::CurrentGame();
    if (ResearchAndDevelopment::Instance != nullptr && g != nullptr && (g->Mode == Game::Modes::MISSION || g->Mode == Game::Modes::MISSION_BUILDER)) {
        ResearchAndDevelopment::Instance->SetScience(1337.0f, TransactionReasons::Cheating);
        ResearchAndDevelopment::Instance->CheatTechnology();
    }
}

// Mission.cs:1535 GetMissionCraftByFileName(name) != null (MissionCraft and the craft-file list are deferred: the host provides the test)
bool Mission::craftFileExists(const std::string& craftFileName) {
    if (MissionsEnvironment::craftFileExists) return MissionsEnvironment::craftFileExists(*this, craftFileName);
    return true;
}

// Mission.cs:602
bool Mission::BlockPlayMission(bool showDialog) {
    (void)showDialog;
    blockedMessages_.clear();
    bool flag = false;
    const std::vector<VesselSituation*> allVesselSituations = GetAllVesselSituations();
    for (size_t i = 0; i < allVesselSituations.size(); ++i) {
        if (!allVesselSituations[i]->playerCreated && allVesselSituations[i]->craftFile.empty()) {
            std::string text = Localizer::Format("#autoLOC_8003065");
            if (allVesselSituations[i]->node != nullptr) text += Localizer::Format("#autoLOC_8003066", {allVesselSituations[i]->node->Title()});
            blockedMessages_.push_back(text);
            flag = true;
        }
    }
    std::string text2 = Localizer::Format("#autoLOC_8003067");
    for (size_t j = 0; j < blockedMessages_.size(); ++j) text2 = text2 + blockedMessages_[j] + "\n";
    text2 += Localizer::Format("#autoLOC_8003068");
    if (flag) {
        log::Error("[Mission] Unable to be started due to blocking errors - " + dotnet::Replace(dotnet::Replace(text2, "\n", "-"), "\t", " "));
        // showDialog: MissionEditorLogic.Instance.unableToStartMission = PopupDialog.SpawnPopupDialog(...) (UI)
    }
    return flag;
}

// Mission.cs:773
void Mission::StartMission() {
    if (CheckHash()) {
        for (size_t i = 0; i < orphanNodes_.size(); ++i) orphanNodes_[i]->InitializeTestGroups();
        if (activeNode_ == nullptr) InitMission();
        activeNode_->ActivateNode(true);
        if (!isStarted_ && HighLogic::CurrentGame() != nullptr) {
            isStarted_ = true;
            startedUT = MissionsEnvironment::GameUniversalTime();
            startNode_->activatedUT = startedUT;
            if (MissionsEnvironment::analytics) MissionsEnvironment::analytics("LogMissionStart", this);
            GameEvents::Mission::onStarted.Fire(this);
        }
    }
}

// Mission.cs:797
void Mission::ResumeMission() {
    if (!CheckHash() || !isStarted_ || isEnded_ || !isInitialized_) return;
    for (size_t i = 0; i < orphanNodes_.size(); ++i) orphanNodes_[i]->InitializeTestGroups();
    if (briefingNodeActive && MissionsEnvironment::checkFireBriefingDialog) MissionsEnvironment::checkFireBriefingDialog(this);
    activeNode_->ActivateNode(true);
    Game* g = HighLogic::CurrentGame();
    if (g == nullptr || g->Mode != Game::Modes::MISSION || !HighLogic::LoadedSceneHasPlanetarium()) return;
    for (size_t j = 0; j < nodes.ValuesList().size(); ++j) {
        MENode* n = nodes.ValuesList()[j];
        for (size_t k = 0; k < n->actionModules.size(); ++k) {
            auto* actionModule = dynamic_cast<ActionModule*>(n->actionModules[k].get());
            if (actionModule != nullptr && actionModule->isRunning && actionModule->restartOnSceneLoad) MissionsEnvironment::StartCoroutine(actionModule->Fire());
        }
    }
}

// Mission.cs:829
void Mission::SetStartNode(MENode* newStart) {
    startNode_ = newStart;
    newStart->isStartNode = true;
}

// Mission.cs:835
void Mission::UpdateMission() {
    if (!registeredGameEvents) RegisterGameEvents();
    if (activeNode_ == nullptr || !isStarted_ || !MissionsEnvironment::MissionSystemActive() || activeNode_->isPaused || briefingNodeActive) return;
    if (activeNode_->isEndNode) {
        DoEndMissionActions();
        return;
    }
    if (switchActiveVessel) {
        switchActiveVessel = false;
        if (HighLogic::LoadedSceneIsFlight && MissionsEnvironment::setActiveVessel) MissionsEnvironment::setActiveVessel(vesselToSwitchActive);
        return;
    }
    if (saveRevertOnSwitchActiveVessel) {
        bool flag = false;
        if (briefingNodeActive) {
            flag = true;
            briefingNodeActive = false;
        }
        saveRevertOnSwitchActiveVessel = false;
        // GamePersistence.SaveGame("persistent", ...) and the lastcreatevesselspawn.missionsfs copy (:869-:885)
        if (MissionsEnvironment::saveRevertToVesselSpawn) MissionsEnvironment::saveRevertToVesselSpawn(this);
        if (flag) briefingNodeActive = true;
    }
    if (PendingVesselLaunch(activeNode_)) return;
    for (int i = 0; i < static_cast<int>(activeNode_->toNodes.size()); ++i) {
        MENode* mENode = activeNode_->toNodes[static_cast<size_t>(i)];
        if (!mENode->RunTests()) continue;
        SwitchActiveNodes(mENode);
        i = static_cast<int>(activeNode_->toNodes.size());
        for (int j = 0; j < static_cast<int>(activeNode_->dockedNodes.size()); ++j) {
            MENode* mENode2 = activeNode_->dockedNodes[static_cast<size_t>(j)];
            if (mENode2->HasTestModules()) {
                if (mENode2->RunTests()) {
                    SwitchActiveNodes(mENode2);
                    i = static_cast<int>(activeNode_->toNodes.size());
                }
            } else {
                mENode2->ActivateNode();
            }
        }
        if (mENode->isEndNode && !mENode->isPaused) DoEndMissionActions();
    }
}

// Mission.cs:922
void Mission::DoEndMissionActions() {
    isSuccesful_ = activeNode_->missionEndOptions == MissionEndOptions::Success;
    if (isScoreEnabled) globalScore->AwardScore(this);
    if (isSuccesful_) {
        if (isScoreEnabled) awards->EvaluateAwards(this);
        if (MissionsEnvironment::analytics) MissionsEnvironment::analytics("LogMissionCompleted", this);
        GameEvents::Mission::onCompleted.Fire(this);
    } else {
        if (MissionsEnvironment::analytics) MissionsEnvironment::analytics("LogMissionFailed", this);
        GameEvents::Mission::onFailed.Fire(this);
    }
    GameEvents::Mission::onFinished.Fire(this);
    isEnded_ = true;
    GameEvents::onEditorStarted.Remove(this, &Mission::OnEditorStarted);
}

// Mission.cs:948
void Mission::SwitchActiveNodes(MENode* newNode) {
    MENode* mENode = activeNode_;
    GameEvents::Mission::onActiveNodeChanging.Fire(this, GameEvents::FromToAction<MENode*, MENode*>(mENode, newNode));
    mENode->DeactivateNode();
    activeNode_ = newNode->ActivateNode();
    GameEvents::Mission::onActiveNodeChanged.Fire(this, GameEvents::FromToAction<MENode*, MENode*>(mENode, newNode));
}

// Mission.cs:957
void Mission::SetNextObjectives(MENode* node) {
    nextObjectivesNodesSet_ = true;
    nextObjectivesNodes_.clear();
    if (!flow->NodePaths.ContainsKey(node)) return;
    const auto& mENodePathInfo = flow->NodePaths.At(node);
    for (size_t i = 0; i < mENodePathInfo->paths.size(); ++i) {
        const auto& mEPath = mENodePathInfo->paths[i];
        for (size_t j = 0; j < mEPath->Nodes.size() && (!mEPath->Nodes[j]->isEvent || mEPath->Nodes[j]->HasBeenActivated()); ++j) {
            if (mEPath->Nodes[j]->isObjective) {
                AddUniqueTo(nextObjectivesNodes_, mEPath->Nodes[j]);
                break;
            }
        }
    }
}

// Mission.cs:986
bool Mission::IsNextObjective(MENode* node) {
    if (nextObjectivesNodesSet_ && ListContains(nextObjectivesNodes_, node)) return true;
    return false;
}

// Mission.cs:995
void Mission::RemoveInactiveEvent(MENode* node) {
    if (inactiveEventNodesSet_ && ListContains(inactiveEventNodes_, node)) ListRemove(inactiveEventNodes_, node);
}

// Mission.cs:1003
void Mission::UpdateOrphanNodeState(MENode* node, bool makeOrphan) {
    if (makeOrphan && node->IsOrphanNode()) {
        for (int i = 0; i < nodes.Count(); ++i) {
            if (nodes.ValuesList()[static_cast<size_t>(i)] != node) AddUniqueTo(nodes.ValuesList()[static_cast<size_t>(i)]->toNodes, node);
        }
        AddUniqueTo(orphanNodes_, node);
        return;
    }
    for (int j = 0; j < nodes.Count(); ++j) {
        if (!ListContains(node->fromNodes, nodes.ValuesList()[static_cast<size_t>(j)])) ListRemove(nodes.ValuesList()[static_cast<size_t>(j)]->toNodes, node);
    }
    ListRemove(orphanNodes_, node);
}

// Mission.cs:1027
std::string Mission::PrintObjectives(bool onlyPrintActivatedNodes, bool startWithActiveNode) {
    return PrintObjectives(onlyPrintActivatedNodes, startWithActiveNode, false, false);
}

// Mission.cs:1032
std::string Mission::PrintScoreObjectives(bool onlyPrintActivatedNodes, bool startWithActiveNode, bool onlyAwardedScores) {
    return PrintObjectives(onlyPrintActivatedNodes, startWithActiveNode, true, onlyAwardedScores);
}

// Mission.cs:1037
std::string Mission::PrintObjectives(bool onlyPrintActivatedNodes, bool startWithActiveNode, bool onlyPrintScore, bool onlyAwardedScores) {
    std::string objectiveString;
    std::vector<MENode*> visitedNodesList;
    if (startWithActiveNode) {
        visitedNodesList.push_back(activeNode_);
        for (size_t i = 0; i < activeNode_->toNodes.size(); ++i) {
            if (onlyPrintScore) {
                RecursivelyCreateObjectiveScoreString(activeNode_->toNodes[i], objectiveString, onlyPrintActivatedNodes, onlyAwardedScores, visitedNodesList, true);
            } else {
                RecursivelyCreateObjectiveString(activeNode_->toNodes[i], objectiveString, "", onlyPrintActivatedNodes, visitedNodesList, true);
            }
        }
    } else if (onlyPrintScore) {
        RecursivelyCreateObjectiveScoreString(startNode_, objectiveString, onlyPrintActivatedNodes, onlyAwardedScores, visitedNodesList, true);
    } else {
        RecursivelyCreateObjectiveString(startNode_, objectiveString, "", onlyPrintActivatedNodes, visitedNodesList, true);
    }
    for (MENode* current : nodes.GetListEnumerator()) {
        if (current->fromNodeIDs.empty() && !current->isStartNode) {
            if (onlyPrintScore) {
                RecursivelyCreateObjectiveScoreString(current, objectiveString, onlyPrintActivatedNodes, onlyAwardedScores, visitedNodesList, false);
            } else {
                RecursivelyCreateObjectiveString(current, objectiveString, "", onlyPrintActivatedNodes, visitedNodesList, false);
            }
        }
    }
    if (objectiveString == "") objectiveString = "#autoLOC_8100310";
    return objectiveString;
}

// Mission.cs:1087
void Mission::RecursivelyCreateObjectiveString(MENode* currentNode, std::string& objectiveString, std::string indent, bool onlyPrintActivatedNodes,
                                               std::vector<MENode*>& visitedNodesList, bool rejectOrphanNodes) {
    if (currentNode == nullptr || (onlyPrintActivatedNodes && !currentNode->isStartNode && !currentNode->HasBeenActivated()) || (rejectOrphanNodes && currentNode->IsOrphanNode()))
        return;
    if (currentNode->isObjective) {
        std::string sb;
        sb += indent;
        sb += "<color=#EDFEAAFF>\xE2\x80\xA2 ";  // U+2022 BULLET
        sb += Localizer::Format(currentNode->ObjectiveString());
        sb += "</color> ";
        if (currentNode->HasBeenActivated()) sb += Localizer::Format("#autoLOC_8001017", {KSPUtil::PrintTimeCompact(currentNode->activatedUT, false)});
        sb += "\n";
        objectiveString += sb;
        indent += "\t";
    }
    visitedNodesList.push_back(currentNode);
    for (size_t i = 0; i < currentNode->toNodes.size(); ++i) {
        if (!ListContains(visitedNodesList, currentNode->toNodes[i]))
            RecursivelyCreateObjectiveString(currentNode->toNodes[i], objectiveString, indent, onlyPrintActivatedNodes, visitedNodesList, true);
    }
}

// Mission.cs:1116
void Mission::RecursivelyCreateObjectiveScoreString(MENode* currentNode, std::string& objectiveString, bool onlyPrintActivatedNodes, bool onlyAwardedScores,
                                                    std::vector<MENode*>& visitedNodesList, bool rejectOrphanNodes) {
    if (currentNode == nullptr || (onlyPrintActivatedNodes && !currentNode->isStartNode && !currentNode->HasBeenActivated()) || (rejectOrphanNodes && currentNode->IsOrphanNode()))
        return;
    bool flag = false;
    auto isScore = [](IActionModule* am) { return am->IsA("ActionMissionScore"); };
    const std::string bullet = "<color=#EDFEAAFF>\xE2\x80\xA2 ";
    if (currentNode->isObjective) {
        for (MENode* dockedNode : currentNode->dockedNodes) {
            if (static_cast<int>(dockedNode->toNodes.size()) < 1 || static_cast<int>(dockedNode->toNodes[0]->actionModules.size()) <= 0 ||
                !isScore(dockedNode->toNodes[0]->actionModules[0].get()))
                continue;
            std::string sb;
            if (!flag) {
                sb += bullet;
                sb += Localizer::Format(currentNode->ObjectiveString());
                sb += "</color> ";
                if (currentNode->HasBeenActivated()) sb += Localizer::Format("#autoLOC_8001017", {KSPUtil::PrintTimeCompact(currentNode->activatedUT, false)});
            }
            auto* score = dynamic_cast<IActionMissionScore*>(dockedNode->toNodes[0]->actionModules[0].get());
            if (onlyAwardedScores && dockedNode->HasBeenActivated()) {
                sb += "\n" + score->GetAwardedScoreDescription();
            } else if (!onlyAwardedScores) {
                sb += score->GetScoreDescription();
            }
            visitedNodesList.push_back(dockedNode->toNodes[0]);
            objectiveString += sb;
            flag = true;
        }
        if (flag) objectiveString += "\n\n";
        if (!flag && onlyAwardedScores && (currentNode->actionModules.empty() || !isScore(currentNode->actionModules[0].get()))) {
            std::string sb2;
            sb2 += bullet;
            sb2 += Localizer::Format(currentNode->ObjectiveString());
            sb2 += "</color> ";
            if (currentNode->HasBeenActivated()) sb2 += Localizer::Format("#autoLOC_8001017", {KSPUtil::PrintTimeCompact(currentNode->activatedUT, false)});
            sb2 += "\n\n";
            objectiveString += sb2;
        }
    }
    if (!flag && currentNode->dockParentNode == nullptr && !currentNode->actionModules.empty() && isScore(currentNode->actionModules[0].get())) {
        std::string sb3;
        auto* score = dynamic_cast<IActionMissionScore*>(currentNode->actionModules[0].get());
        if (onlyAwardedScores && currentNode->HasBeenActivated()) {
            sb3 += bullet;
            sb3 += Localizer::Format(currentNode->ObjectiveString());
            sb3 += "</color>";
            sb3 += "\n" + score->GetAwardedScoreDescription();
            sb3 += "\n\n";
        } else if (!onlyAwardedScores) {
            sb3 += bullet;
            sb3 += Localizer::Format(currentNode->ObjectiveString());
            sb3 += "</color>";
            sb3 += score->GetScoreDescription();
            sb3 += "\n\n";
        }
        objectiveString += sb3;
    }
    visitedNodesList.push_back(currentNode);
    for (size_t i = 0; i < currentNode->toNodes.size(); ++i) {
        if (!ListContains(visitedNodesList, currentNode->toNodes[i]))
            RecursivelyCreateObjectiveScoreString(currentNode->toNodes[i], objectiveString, onlyPrintActivatedNodes, onlyAwardedScores, visitedNodesList, true);
    }
}

// Mission.cs:166
bool Mission::MissionLoadedFromSFS() const {
    Game* g = HighLogic::CurrentGame();
    if (!HighLogic::LoadedSceneIsMissionBuilder && g != nullptr) {
        if (g->Mode != Game::Modes::MISSION) {
            if (g->Mode == Game::Modes::MISSION_BUILDER) return HighLogic::LoadedScene() == GameScenes::TRACKSTATION;
            return false;
        }
        return true;
    }
    return false;
}

// Mission.cs:154
std::string Mission::PersistentSaveName() const {
    if (MissionLoadedFromSFS()) return dotnet::Replace(HighLogic::SaveFolder(), MissionsUtils::SavesPath(), "");
    return missionInfo->folderName;
}

// Mission.cs:202
std::string Mission::BannersPath() const {
    if (MissionLoadedFromSFS()) return MissionsUtils::SavesRootPath() + PersistentSaveName() + "/Banners/";
    if (missionInfo == nullptr) return "";
    return missionInfo->BannerPath();
}

// Mission.cs:186
std::string Mission::ShipsPath() const {
    if (MissionLoadedFromSFS()) return MissionsUtils::SavesRootPath() + PersistentSaveName() + "/Ships/";
    if (missionInfo == nullptr) return "";
    return missionInfo->ShipFolderPath();
}

// Mission.cs:1688
void Mission::Load(const ConfigNode& node) { Load(node, false); }

// Mission.cs:1693
void Mission::Load(const ConfigNode& node, bool simple) {
    hasBeenValidated = false;
    ConfigNode emptyNode;
    const ConfigNode* configNode = &emptyNode;
    if (!(node.name == "MISSION") && !(node.name == "MISSIONTOSTART")) {
        if (node.HasNode("MISSION")) configNode = node.GetNode("MISSION");
    } else {
        configNode = &node;
    }
    if (configNode->HasValue("id")) id_ = Guid::Parse(*configNode->GetValue("id"));
    if (configNode->HasValue("idName")) idName_ = *configNode->GetValue("idName");
    configNode->TryGetValue("expansionVersion", expansionVersion);
    std::string value;
    configNode->TryGetValue("title", value);
    SetTitle(value);
    configNode->TryGetValue("briefing", briefing);
    configNode->TryGetValue("author", author);
    configNode->TryGetValue("modsBriefing", modsBriefing);
    configNode->TryGetValue("isBriefingSet", isBriefingSet);
    configNode->TryGetValue("packName", packName);
    configNode->TryGetValue("order", order);
    configNode->TryGetValue("hardIcon", hardIcon);
    save_detail::TryGetEnum(*configNode, "difficulty", difficulty, hardIcon ? MissionDifficulty::Advanced : MissionDifficulty::Beginner);
    if (!configNode->TryGetValue("seed", seed_)) InitSeed();
    configNode->TryGetValue("historyId", historyId);
    configNode->TryGetValue("steamPublishedFileId", steamPublishedFileId);
    configNode->TryGetValue("flag", flagURL);
    std::string value2;
    configNode->TryGetValue("bannerMenu", value2);
    bannerMenu.LoadFromMissionFolder(value2, MEBannerType::Menu, this);
    std::string value3;
    configNode->TryGetValue("bannerSuccess", value3);
    bannerSuccess.LoadFromMissionFolder(value3, MEBannerType::Success, this);
    std::string value4;
    configNode->TryGetValue("bannerFail", value4);
    configNode->TryGetValue("isScoreEnabled", isScoreEnabled);
    configNode->TryGetValue("maxScore", maxScore);
    bannerFail.LoadFromMissionFolder(value4, MEBannerType::Fail, this);
    configNode->TryGetValue("activeNodeID", loadActiveNodeID_);
    configNode->TryGetValue("currentScore", currentScore);
    configNode->TryGetValue("exportName", exportName);
    configNode->TryGetValue("missionNameAtLastExport", missionNameAtLastExport);
    if (configNode->HasValue("isStarted")) isStarted_ = save_detail::ParseBool(*configNode->GetValue("isStarted"));
    if (configNode->HasValue("isEnded")) isEnded_ = save_detail::ParseBool(*configNode->GetValue("isEnded"));
    if (configNode->HasValue("isSuccesful")) isSuccesful_ = save_detail::ParseBool(*configNode->GetValue("isSuccesful"));
    save_detail::TryGetEnum(*configNode, "cameraLockMode", cameraLockMode, MissionCameraModeOptions::NoChange);
    save_detail::TryGetEnum(*configNode, "cameraLockOptions", cameraLockOptions, MissionCameraLockOptions::NoChange);
    configNode->TryGetValue("saveRevertOnSwitchActiveVessel", saveRevertOnSwitchActiveVessel);
    ConfigNode* node2 = nullptr;
    if (configNode->TryGetNode("TAGS", node2)) {
        for (const ConfigNode* tag : node2->GetNodes("TAG")) {
            std::string value5;
            tag->TryGetValue("name", value5);
            tags.push_back(value5);
        }
    }
    configNode->TryGetValue("briefingNodeActive", briefingNodeActive);
    globalScore->Load(*configNode);
    awards->Load(*configNode);
    if (configNode->HasNode("MAPPEDVESSELS")) {
        mappedVessels = MissionMappedVessel::Load(*configNode->GetNode("MAPPEDVESSELS"));
        for (size_t j = 0; j < mappedVessels.size(); ++j) {
            MissionMappedVessel missionMappedVessel(mappedVessels[j].partPersistentId, mappedVessels[j].originalVesselPersistentId, mappedVessels[j].currentVesselPersistentId,
                                                    mappedVessels[j].partVesselName, mappedVessels[j].craftFileName, mappedVessels[j].situationVesselName);
            missionMappedVessel.mappedVesselPersistentId = mappedVessels[j].mappedVesselPersistentId;
            mappedVesselKeys.push_back(missionMappedVessel);  // List.AddUnique on a class without Equals: reference equality, always added
        }
    }
    if (configNode->HasNode("NODES")) {
        ConfigNode* node3 = nullptr;
        if (configNode->TryGetNode("SITUATION", node3)) {
            situation->Load(*node3);
            if (expansionVersion == "" && node3->HasValue("version")) expansionVersion = "1.2.0";
        }
        ConfigNode* node4 = nullptr;
        if (configNode->TryGetNode("NODES", node4)) {
            inactiveEventNodes_.clear();
            inactiveEventNodesSet_ = true;
            for (const ConfigNode* node5 : node4->GetNodes("NODE")) {
                MENode* mENode = MENode::Spawn(this);
                mENode->Load(*node5);
                if (mENode->isEvent && !mENode->HasBeenActivated()) AddUniqueTo(inactiveEventNodes_, mENode);
                nodes.Add(mENode->id(), mENode);
            }
        }
        if (AwardEnvironment::CurrentGameIsMission()) {
            for (const ConfigNode* obj : situation->vesselsToBuildNode->GetNodes("VESSELTOBUILD")) {
                Guid value6 = Guid::Empty();
                obj->TryGetValue("NodeGuid", value6);
                if (!(value6 == Guid::Empty())) {
                    MENode* nodeById = GetNodeById(value6);
                    if (nodeById != nullptr) {
                        IActionCreateVessel* actionCreateVessel = GetActionCreateVessel(nodeById);
                        if (actionCreateVessel != nullptr) situation->vesselSituationList.Add(actionCreateVessel->GetVesselSituation(), value6);
                    }
                }
            }
            if (!situation->startingActionsNode) throw NullReferenceException();  // situation.startingActionsNode.GetNodes(...) with no SITUATION node
            for (const ConfigNode* obj2 : situation->startingActionsNode->GetNodes("STARTINGACTION")) {
                Guid value7 = Guid::Empty();
                obj2->TryGetValue("NodeGuid", value7);
                if (!(value7 == Guid::Empty())) {
                    MENode* nodeById2 = GetNodeById(value7);
                    if (!(nodeById2 != nullptr)) continue;
                    if (IActionCreateKerbal* a = GetActionCreateKerbal(nodeById2)) {
                        situation->startingActions.push_back(dynamic_cast<IActionModule*>(a));
                        continue;
                    }
                    if (IActionCreateAsteroid* b = GetActionCreateAsteroid(nodeById2)) {
                        situation->startingActions.push_back(dynamic_cast<IActionModule*>(b));
                        continue;
                    }
                    if (IActionCreateComet* c = GetActionCreateComet(nodeById2)) {
                        situation->startingActions.push_back(dynamic_cast<IActionModule*>(c));
                        continue;
                    }
                    if (IActionCreateFlag* d = GetActionCreateFlag(nodeById2)) situation->startingActions.push_back(dynamic_cast<IActionModule*>(d));
                }
            }
        }
        if (!simple) {
            if (MissionsEnvironment::rebuildCraftFileList) MissionsEnvironment::rebuildCraftFileList(*this);  // RebuildCraftFileList (:1350)
            if (KSPUtil::CheckVersion(expansionVersion, 1, 10, 0) == VersionCompareResult::INCOMPATIBLE_TOO_EARLY) {
                if (MissionsEnvironment::processMappedVesselNodeMap) MissionsEnvironment::processMappedVesselNodeMap(*this);  // :1860
            }
        }
        SetStartNode();
        SetActiveNode();
        SetDockedNodes();
        Game* g = HighLogic::CurrentGame();
        if (g != nullptr && g->Mode == Game::Modes::MISSION && missionInfo == nullptr) {
            std::string text;
            if (MissionsUtils::Paths::isTestMode && MissionsEnvironment::missionToTest && !MissionsEnvironment::missionToTest().empty()) {
                text = MissionsEnvironment::missionToTest();
            } else {
                const std::string text2 = HighLogic::SaveFolder().substr(MissionsUtils::SavesPath().size());
                const bool squad = packName.rfind("squad_", 0) == 0;
                text = (!squad ? MissionsUtils::UsersMissionsPath() : MissionsUtils::StockMissionsPath()) + text2 + "/persistent.mission";
                std::error_code ec;
                if (!std::filesystem::exists(text, ec)) text = (!squad ? MissionsUtils::StockMissionsPath() : MissionsUtils::UsersMissionsPath()) + text2 + "/persistent.mission";
                if (!std::filesystem::exists(text, ec) && squad) text = MissionsUtils::BaseMissionsPath() + text2 + "/persistent.mission";
                if (!std::filesystem::exists(text, ec)) text = MissionsUtils::Paths::steamWorkshopFolder + text2 + "/persistent.mission";
            }
            std::error_code ec;
            if (std::filesystem::exists(text, ec)) {
                if (MissionsEnvironment::updateMissionFileInfo) missionInfo = MissionsEnvironment::updateMissionFileInfo(*this, text);  // :1204
            } else {
                log::Error("Failed to load mission. Unable to find Mission folder");
            }
        }
        if (configNode->HasNode("FLOW")) {
            try {
                flow = std::make_unique<MissionFlow>(this);
                flow->Load(*configNode->GetNode("FLOW"));
                SetNextObjectives(activeNode_);
            } catch (const std::exception&) {
                log::Warning("Unable to load mission flow from ConfigNode");
            }
        }
    } else {
        log::Error("Unable to load mission from ConfigNode");
    }
    hashBytes = MissionsEnvironment::buildMissionFileHash ? MissionsEnvironment::buildMissionFileHash(*configNode) : std::vector<uint8_t>();
    if (missionInfo != nullptr) {
        if (missionInfo->missionType == MissionTypes::Base) {
            std::ifstream f(missionInfo->FolderPath() + "signature", std::ios::binary);
            if (f) signature = std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        }
    } else if (configNode->HasValue("signature")) {
        signature = configNode->GetValue("signature");
    }
    if (!simple && !MissionsEnvironment::MakingHistoryInstalled() && !CheckHash()) log::Error("[Mission]: Mission Save failed Validation Check.");
}

// Mission.cs:2174
void Mission::Save(ConfigNode& node) {
    node.AddValue("id", id_.ToString());
    if (!idName_.empty()) node.AddValue("idName", idName_);
    if (MissionsEnvironment::MakingHistoryInstalled()) {
        node.AddValue("expansionVersion", MissionsEnvironment::makingHistoryVersion ? MissionsEnvironment::makingHistoryVersion() : std::string());
    } else {
        node.AddValue("expansionVersion", expansionVersion);
    }
    node.AddValue("title", title_);
    std::string text = dotnet::Replace(briefing, "\n", "\\n");
    text = dotnet::Replace(text, "\t", "\\t");
    node.AddValue("briefing", text);
    node.AddValue("author", author);
    std::string text2 = dotnet::Replace(modsBriefing, "\n", "\\n");
    text2 = dotnet::Replace(text2, "\t", "\\t");
    node.AddValue("modsBriefing", text2);
    if (!packName.empty()) node.AddValue("packName", packName);
    if (order < INT32_MAX) node.AddValue("order", order);
    node.AddValue("hardIcon", hardIcon);
    node.AddValue("difficulty", dotnet::EnumToString(difficulty));
    node.AddValue("isBriefingSet", isBriefingSet);
    node.AddValue("seed", seed_);
    node.AddValue("flag", flagURL);
    node.AddValue("bannerMenu", bannerMenu.fileName);
    node.AddValue("bannerSuccess", bannerSuccess.fileName);
    node.AddValue("bannerFail", bannerFail.fileName);
    node.AddValue("isScoreEnabled", isScoreEnabled);
    node.AddValue("maxScore", maxScore);
    node.AddValue("steamPublishedFileId", steamPublishedFileId);
    node.AddValue("historyId", historyId.ToString());
    if (activeNode_ != nullptr) node.AddValue("activeNodeID", activeNode_->id().ToString());
    node.AddValue("currentScore", currentScore);
    node.AddValue("exportName", exportName);
    node.AddValue("missionNameAtLastExport", missionNameAtLastExport);
    node.AddValue("isStarted", isStarted_);
    node.AddValue("isEnded", isEnded_);
    node.AddValue("isSuccesful", isSuccesful_);
    node.AddValue("briefingNodeActive", briefingNodeActive);
    node.AddValue("cameraLockMode", dotnet::EnumToString(cameraLockMode));
    node.AddValue("cameraLockOptions", dotnet::EnumToString(cameraLockOptions));
    if (saveRevertOnSwitchActiveVessel) node.AddValue("saveRevertOnSwitchActiveVessel", saveRevertOnSwitchActiveVessel);
    if (signature.has_value()) node.AddValue("signature", *signature);
    ConfigNode* node2 = node.AddNode("SITUATION");
    situation->Save(*node2);
    globalScore->Save(node);
    awards->Save(node);
    MissionMappedVessel::Save(node, mappedVessels);
    ConfigNode* configNode = node.AddNode("NODES");
    for (MENode* value : nodes.Values()) {
        ConfigNode* node3 = configNode->AddNode("NODE");
        value->Save(*node3);
    }
    ConfigNode* configNode2 = node.AddNode("TAGS");
    for (size_t i = 0; i < tags.size(); ++i) configNode2->AddNode("TAG")->AddValue("name", tags[i]);
    ConfigNode* node4 = node.AddNode("FLOW");
    flow->Save(*node4);
}

// Mission.cs:2258
uint32_t Mission::CurrentVesselID(MENode* node, uint32_t vesselID) {
    uint32_t num = MappedVessels::ConvertMappedId(mappedVessels, vesselID);
    const bool exists = MissionsEnvironment::persistentVesselIdExists ? MissionsEnvironment::persistentVesselIdExists(num)
                                                                        : AwardEnvironment::VesselByPersistentId(num) != nullptr;
    if (!exists) {
        int num2 = MappedVessels::IndexMappedVesselId(node->mission->mappedVessels, vesselID);
        if (num2 != -1) {
            uint32_t partPersistentId = mappedVessels[static_cast<size_t>(num2)].partPersistentId;
            uint32_t vesselOfPart = 0;
            MissionsEnvironment::PartLookup r = MissionsEnvironment::PartLookup::NotFound;
            if (MissionsEnvironment::partVessel) r = MissionsEnvironment::partVessel(partPersistentId, true, vesselOfPart);
            if (r != MissionsEnvironment::PartLookup::NotFound) {
                if (r == MissionsEnvironment::PartLookup::Found) {
                    num = vesselOfPart;
                } else if (!ListContains(missingVessels, num)) {
                    missingVessels.push_back(num);
                    log::Error("[TestVessel] Unable to find VesselID (" + std::to_string(num) + ") from Node (" + (node != nullptr ? node->Title() : std::string("Unknown")) +
                               ") in FlightGlobals.");
                }
            } else {
                r = MissionsEnvironment::PartLookup::NotFound;
                if (MissionsEnvironment::partVessel) r = MissionsEnvironment::partVessel(partPersistentId, false, vesselOfPart);
                if (r != MissionsEnvironment::PartLookup::NotFound) {
                    if (r == MissionsEnvironment::PartLookup::Found) {
                        num = vesselOfPart;
                    } else if (!ListContains(missingVessels, num)) {
                        missingVessels.push_back(num);
                        log::Error("[TestVessel] Unable to find VesselID (" + std::to_string(num) + ") from Node (" + (node != nullptr ? node->Title() : std::string("Unknown")) +
                                   ") in FlightGlobals.");
                    }
                }
            }
        }
    }
    return num;
}

// Mission.cs:1665
void Mission::SetLockedCamera(MissionCameraModeOptions newCameraMode, MissionCameraLockOptions newCameraLock) {
    cameraLockMode = newCameraMode;
    cameraLockOptions = newCameraLock;
    if (MissionsEnvironment::setLockedCamera) MissionsEnvironment::setLockedCamera(this, cameraLockMode, cameraLockOptions);
}

// Mission.cs:2296
MENode* Mission::GetNodeById(const Guid& guid) {
    int num = 0;
    while (true) {
        if (num < nodes.Count()) {
            if (nodes.KeyAt(num) == guid) break;
            ++num;
            continue;
        }
        return nullptr;
    }
    return nodes.At(num);
}

// Mission.cs:2315
Guid Mission::GetNodeGuidByVesselID(uint32_t persistentId) { return GetNodeGuidByVesselID(persistentId, false); }

// Mission.cs:2320
Guid Mission::GetNodeGuidByVesselID(uint32_t persistentId, bool processMappedVessels) {
    if (processMappedVessels) persistentId = MappedVessels::ConvertMappedId(mappedVessels, persistentId);
    DictionaryValueList<VesselSituation*, Guid> allVesselSituationsGuid = GetAllVesselSituationsGuid();
    int num = 0;
    while (true) {
        if (num < allVesselSituationsGuid.Count()) {
            if (allVesselSituationsGuid.KeyAt(num)->persistentId == persistentId) break;
            ++num;
            continue;
        }
        return Guid::Empty();
    }
    return allVesselSituationsGuid.At(num);
}

// Mission.cs:2344
bool Mission::PendingVesselLaunch(MENode* node) { return node->HasPendingVesselLaunch(); }

// Mission.cs:2349
VesselSituation* Mission::GetVesselSituationByVesselID(uint32_t persistentId) { return GetVesselSituationByVesselID(persistentId, false); }

// Mission.cs:2354
VesselSituation* Mission::GetVesselSituationByVesselID(uint32_t persistentId, bool processMappedVessels) {
    const Guid nodeGuidByVesselID = GetNodeGuidByVesselID(persistentId, processMappedVessels);
    if (!(nodeGuidByVesselID == Guid::Empty())) {
        MENode* nodeById = GetNodeById(nodeGuidByVesselID);
        if (nodeById != nullptr) {
            IActionCreateVessel* actionCreateVessel = GetActionCreateVessel(nodeById);
            if (actionCreateVessel != nullptr) return actionCreateVessel->GetVesselSituation();
        }
    }
    return nullptr;
}

// Mission.cs:2372
Asteroid* Mission::GetAsteroidByPersistentID(uint32_t persistentId) {
    std::vector<IActionCreateAsteroid*> all;
    for (MENode* n : nodes.GetListEnumerator())
        for (auto& am : n->actionModules)
            if (am->TypeName() == "ActionCreateAsteroid")
                if (auto* t = dynamic_cast<IActionCreateAsteroid*>(am.get())) all.push_back(t);
    int num = 0;
    while (true) {
        if (num < static_cast<int>(all.size())) {
            // allActionModules[num].asteroid.persistentId (the Asteroid class is slice 2: its persistent id is read through the action)
            if (all[static_cast<size_t>(num)]->GetPersistentID() == persistentId) break;
            ++num;
            continue;
        }
        return nullptr;
    }
    return all[static_cast<size_t>(num)]->GetAsteroid();
}

// Mission.cs:2392
Comet* Mission::GetCometByPersistentID(uint32_t PersistentId) {
    std::vector<IActionCreateComet*> all;
    for (MENode* n : nodes.GetListEnumerator())
        for (auto& am : n->actionModules)
            if (am->TypeName() == "ActionCreateComet")
                if (auto* t = dynamic_cast<IActionCreateComet*>(am.get())) all.push_back(t);
    int num = 0;
    while (true) {
        if (num < static_cast<int>(all.size())) {
            if (all[static_cast<size_t>(num)]->GetComet() != nullptr && all[static_cast<size_t>(num)]->GetPersistentID() == PersistentId) break;
            ++num;
            continue;
        }
        return nullptr;
    }
    return all[static_cast<size_t>(num)]->GetComet();
}

// Mission.cs:2412
IActionCreateFlag* Mission::GetActionCreateFlagByPersistentID(uint32_t persistentId) {
    std::vector<IActionCreateFlag*> all;
    for (MENode* n : nodes.GetListEnumerator())
        for (auto& am : n->actionModules)
            if (am->TypeName() == "ActionCreateFlag")
                if (auto* t = dynamic_cast<IActionCreateFlag*>(am.get())) all.push_back(t);
    int num = 0;
    while (true) {
        if (num < static_cast<int>(all.size())) {
            if (all[static_cast<size_t>(num)]->GetPersistentID() == persistentId) break;
            ++num;
            continue;
        }
        return nullptr;
    }
    return all[static_cast<size_t>(num)];
}

namespace {
template <class I>
I* FindAction(MENode* node, const char* typeName) {
    auto& actionModules = node->actionModules;
    for (size_t num = 0; num < actionModules.size(); ++num)
        if (actionModules[num]->IsA(typeName)) return dynamic_cast<I*>(actionModules[num].get());
    return nullptr;
}
}  // namespace

// Mission.cs:2432
IActionCreateVessel* Mission::GetActionCreateVessel(MENode* node) { return FindAction<IActionCreateVessel>(node, "ActionCreateVessel"); }
// Mission.cs:2452
IActionCreateKerbal* Mission::GetActionCreateKerbal(MENode* node) { return FindAction<IActionCreateKerbal>(node, "ActionCreateKerbal"); }
// Mission.cs:2472
IActionCreateAsteroid* Mission::GetActionCreateAsteroid(MENode* node) { return FindAction<IActionCreateAsteroid>(node, "ActionCreateAsteroid"); }
// Mission.cs:2492
IActionCreateComet* Mission::GetActionCreateComet(MENode* node) { return FindAction<IActionCreateComet>(node, "ActionCreateComet"); }
// Mission.cs:2512
IActionCreateFlag* Mission::GetActionCreateFlag(MENode* node) { return FindAction<IActionCreateFlag>(node, "ActionCreateFlag"); }

// Mission.cs:2532
std::vector<VesselSituation*> Mission::GetAllVesselSituations() {
    std::vector<VesselSituation*> list;
    for (MENode* n : nodes.GetListEnumerator()) {
        for (auto& am : n->actionModules) {
            if (am->TypeName() == "ActionCreateVessel") {
                if (auto* actionCreateVessel = dynamic_cast<IActionCreateVessel*>(am.get())) list.push_back(actionCreateVessel->GetVesselSituation());
            }
        }
    }
    return list;
}

// Mission.cs:2558
DictionaryValueList<VesselSituation*, Guid> Mission::GetAllVesselSituationsGuid() {
    DictionaryValueList<VesselSituation*, Guid> dictionaryValueList;
    for (MENode* n : nodes.GetListEnumerator()) {
        for (auto& am : n->actionModules) {
            if (am->TypeName() == "ActionCreateVessel") {
                if (auto* actionCreateVessel = dynamic_cast<IActionCreateVessel*>(am.get())) dictionaryValueList.Add(actionCreateVessel->GetVesselSituation(), n->id());
            }
        }
    }
    return dictionaryValueList;
}

// Mission.cs:2646
bool Mission::MissionHasLaunchSite(const std::string& name) {
    for (MENode* n : nodes.GetListEnumerator()) {
        if (n->IsDockedToStartNode() && n->IsLaunchPadNode()) {
            auto* actionCreateLaunchSite = dynamic_cast<IActionCreateLaunchSite*>(n->actionModules[0].get());
            if (actionCreateLaunchSite != nullptr && actionCreateLaunchSite->GetLaunchSiteName() == name) return true;
        }
    }
    return false;
}

// Mission.cs:2668
MEBannerEntry* Mission::GetBanner(MEBannerType bannerType) {
    switch (bannerType) {
        case MEBannerType::Menu: return &bannerMenu;
        case MEBannerType::Success: return &bannerSuccess;
        case MEBannerType::Fail: return &bannerFail;
    }
    return nullptr;
}

// Mission.cs:2686
void Mission::SetBanner(const MEBannerEntry& newBanner, MEBannerType bannerType) {
    switch (bannerType) {
        case MEBannerType::Menu: bannerMenu = newBanner; break;
        case MEBannerType::Success: bannerSuccess = newBanner; break;
        case MEBannerType::Fail: bannerFail = newBanner; break;
    }
}

// MENode ownership (Mission.Spawn parents the node GameObject under the mission's)
MENode* Mission::AdoptNode(std::unique_ptr<MENode> node) {
    ownedNodes_.push_back(std::move(node));
    return ownedNodes_.back().get();
}

}  // namespace ksp::Expansions::Missions
