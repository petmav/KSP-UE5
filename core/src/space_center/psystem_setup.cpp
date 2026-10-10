// WP-42: PSystemSetup.cs.
#include "ksp/space_center/psystem_setup.hpp"

#include <algorithm>

#include "ksp/career/career_environment.hpp"
#include "ksp/career/scenario_misc.hpp"
#include "ksp/career/upgradeables.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/log.hpp"
#include "ksp/editor/preflight_tests.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/space_center/pqs_city.hpp"
#include "ksp/space_center/space_center.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/util/enum_extensions.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;
using space_center::PQSInfo;
using space_center::SceneGraph;

// ---- SpaceCenterFacility ----------------------------------------------------------------------------------------------------

// PSystemSetup.cs:28
void PSystemSetup::SpaceCenterFacility::SpawnPoint::Setup(SpaceCenterFacility* hostFacility) { host = hostFacility; }

// PSystemSetup.cs:33
std::optional<Vector3d> PSystemSetup::SpaceCenterFacility::SpawnPoint::GetSpawnPointTransform() {
    if (!transformFound && SceneGraph::FindChild) {
        const auto found = SceneGraph::FindChild(host->pqsName, host->facilityTransformName, spawnTransformURL);
        if (found) {
            transformFound = true;
            transformPosition = *found;
        }
    }
    if (!transformFound) {
        log::Info("[PSystemSetup::SpaceCenterFacility::SpawnPoint]: Cannot find a transform named '" + spawnTransformURL + "' on '" + name + "'");
        return std::nullopt;
    }
    return transformPosition;
}

// PSystemSetup.cs:44
void PSystemSetup::SpaceCenterFacility::SpawnPoint::GetSpawnPointLatLonAlt(double& lat, double& lon, double& alt) const {
    lat = latitude;
    lon = longitude;
    alt = altitude;
}

// PSystemSetup.cs:50
void PSystemSetup::SpaceCenterFacility::SpawnPoint::SetSpawnPointLatLonAlt() {
    if (SceneGraph::FindChild) {
        const auto found = SceneGraph::FindChild(host->pqsName, host->facilityTransformName, spawnTransformURL);
        transformFound = found.has_value();
        if (found) transformPosition = *found;
    }
    if (!transformFound) {
        log::Info("[PSystemSetup::SpaceCenterFacility::SpawnPoint]: Cannot find a transform named '" + spawnTransformURL + "' on '" + name + "'");
        latlonaltSet = false;
    } else if (host->hostBody != nullptr) {
        host->hostBody->GetLatLonAlt(transformPosition, latitude, longitude, altitude);
        latlonaltSet = true;
    } else {
        log::Info("[PSystemSetup::SpaceCenterFacility::SpawnPoint]: (" + name + ") Unable to set Latitude, Longitude, Altitude");
        latlonaltSet = false;
    }
}

// PSystemSetup.cs:84
void PSystemSetup::SpaceCenterFacility::Setup(const std::vector<PQSInfo>& PQSs, const std::function<CelestialBody*(const PQSInfo&)>& bodyOf) {
    for (const PQSInfo& pQS : PQSs) {
        if (pQS.gameObjectName == pqsName) {
            facilityPQS = pQS.pqs;
            hostBody = bodyOf ? bodyOf(pQS) : nullptr;
            facilityTransformFound = false;
            if (SceneGraph::FindInPQS) {
                const auto found = SceneGraph::FindInPQS(pqsName, facilityTransformName);
                if (found) {
                    facilityTransformFound = true;
                    facilityTransformPosition = *found;
                }
            }
            const size_t slash = facilityTransformName.find_last_of('/');
            facilityName = slash == std::string::npos ? facilityTransformName : facilityTransformName.substr(slash + 1);
            if (facilityTransformFound) break;
        }
    }
    if (!facilityTransformFound) {
        log::Info("[PSystemSetup::SpaceCenterFacility]: Cannot find facility named '" + facilityTransformName + "' on pqs '" + pqsName + "'");
    } else {
        for (SpawnPoint& sp : spawnPoints) sp.Setup(this);
    }
}

// PSystemSetup.cs:112
void PSystemSetup::SpaceCenterFacility::SetSpawnPointsLatLonAlt() {
    for (size_t i = 0; i < spawnPoints.size(); i++) spawnPoints[i].SetSpawnPointLatLonAlt();
}

// PSystemSetup.cs:120
PSystemSetup::SpaceCenterFacility::SpawnPoint* PSystemSetup::SpaceCenterFacility::GetSpawnPoint(const std::string& spawnPointName) {
    for (SpawnPoint& sp : spawnPoints)
        if (sp.name == spawnPointName) return &sp;
    log::Info("[PSystemSetup::SpaceCenterFacility]: Cannot find spawnpoint named '" + spawnPointName + "'");
    return nullptr;
}

// PSystemSetup.cs:146
float PSystemSetup::SpaceCenterFacility::GetFacilityDamage() { return ScenarioDestructibles::GetFacilityDamage(facilityName); }

// PSystemSetup.cs:150
float PSystemSetup::SpaceCenterFacility::GetFacilityLevel() { return ScenarioUpgradeableFacilities::GetFacilityLevel(facilityName); }

// PSystemSetup.cs:154
bool PSystemSetup::SpaceCenterFacility::IsLaunchFacility() const { return !spawnPoints.empty(); }

// ---- PSystemSetup -----------------------------------------------------------------------------------------------------------

PSystemSetup::PSystemSetup() = default;
PSystemSetup::~PSystemSetup() { OnDestroy(); }

// PSystemSetup.cs:207
std::vector<LaunchSite*> PSystemSetup::NonStockLaunchSites() const {
    std::vector<LaunchSite*> list;
    for (size_t i = 0; i < launchsites.size(); i++) {
        bool flag = false;
        for (size_t j = 0; j < stocklaunchsites.size(); j++) {
            if (stocklaunchsites[j]->name == launchsites[i]->name) {
                flag = true;
                break;
            }
        }
        if (!flag) list.push_back(launchsites[i]);
    }
    return list;
}

// PSystemSetup.cs:238
void PSystemSetup::Awake() {
    if (Instance != nullptr && Instance != this) return;   // DestroyImmediate(gameObject)
    Instance = this;
}

// PSystemSetup.cs:248 (SceneManager.sceneLoaded and PSystemManager.OnPSystemReady are the host's)
void PSystemSetup::Start() {
    GE::onGameSceneLoadRequested.Add(this, &PSystemSetup::OnSceneChange);
    GE::OnUpgradeableObjLevelChange.Add(this, &PSystemSetup::onUpgradeableObjLevelChange);
    spaceCenterFacilityLaunchSites.clear();
    launchsites.clear();
}

// PSystemSetup.cs:265
void PSystemSetup::OnDestroy() {
    GE::onGameSceneLoadRequested.Remove(this, &PSystemSetup::OnSceneChange);
    GE::OnUpgradeableObjLevelChange.Remove(this, &PSystemSetup::onUpgradeableObjLevelChange);
    if (Instance != nullptr && Instance == this) Instance = nullptr;
}

// PSystemSetup.cs:276
void PSystemSetup::onUpgradeableObjLevelChange(Upgradeables::UpgradeableObject* upgradeObject, int level) {
    (void)level;
    for (size_t i = 0; i < spaceCenterFacilityLaunchSites.size(); i++) {
        SpaceCenterFacility* f = spaceCenterFacilityLaunchSites[i];
        if (f->facilityTransformFound) {
            if (f->upgradeableObject == nullptr && facilityUpgradeableObject) f->upgradeableObject = facilityUpgradeableObject(*f);
            if (f->upgradeableObject != nullptr && upgradeObject == f->upgradeableObject) f->SetSpawnPointsLatLonAlt();
        }
    }
}

// PSystemSetup.cs:378
void PSystemSetup::SetupFacilities() {
    for (SpaceCenterFacility& spaceCenterFacility : facilities) {
        spaceCenterFacility.Setup(pqsArray, bodyOfPQS);
        if (spaceCenterFacility.IsLaunchFacility()) spaceCenterFacilityLaunchSites.push_back(&spaceCenterFacility);
    }
}

namespace {
const PQSInfo* FindPQSInfo(const std::vector<PQSInfo>& arr, const std::string& name) {
    for (const PQSInfo& p : arr)
        if (p.gameObjectName == name) return &p;
    return nullptr;
}
}  // namespace

// PSystemSetup.cs:387 (FloatingOrigin.SetOffset / ForceInitPQS / PQS.SetTarget / ResetModList are `sceneHook` calls)
Enumerator PSystemSetup::SetupLaunchSites() {
    std::vector<PQS*> spherestoSetup;
    for (size_t lsI = 0; lsI < stocklaunchsites.size(); lsI++) {
        LaunchSite& launchSite = *stocklaunchsites[lsI];
        if (launchSite.BundleName == "makinghistory_assets" && !SaveEnvironment::Current().IsExpansionInstalled("MakingHistory")) {
            log::Info("PSystemSetup: Cannot check launchsite named " + launchSite.name + "- Making History expansion is not installed.");
            continue;
        }
        const PQSInfo* gameObject = FindPQSInfo(pqsArray, launchSite.pqsName);
        if (gameObject != nullptr) {
            PQSCity* component = findPQSCity ? findPQSCity(launchSite.pqsName, launchSite.launchSiteTransformURL) : nullptr;
            if (component != nullptr) {
                launchSite.pqsCity = component;
                component->launchSite = &launchSite;
            }
            PQSCity2* component2 = findPQSCity2 ? findPQSCity2(launchSite.pqsName, launchSite.launchSiteTransformURL) : nullptr;
            if (component2 != nullptr) {
                launchSite.pqsCity2 = component2;
                component2->launchSite = &launchSite;
            }
        }
        bool flag = false;
        bool forcePQSPosition = false;
        if (launchSite.pqsCity != nullptr) {
            if (launchSite.Setup(launchSite.pqsCity, pqsArray)) {
                launchsites.push_back(&launchSite);
                launchSite.pqsCity->launchSite = &launchSite;
                flag = true;
            }
        } else if (launchSite.pqsCity2 != nullptr) {
            if (launchSite.Setup(launchSite.pqsCity2, pqsArray)) {
                launchsites.push_back(&launchSite);
                launchSite.pqsCity2->launchSite = &launchSite;
                flag = true;
            }
            if (sceneHook && flag) sceneHook("SpawnAdditionalPrefabs:" + launchSite.name, nullptr);
        } else {
            if (!launchSite.prefabPath.empty() && !launchSite.BundleName.empty() && gameObject != nullptr && instantiateBundlePrefab) {
                if (instantiateBundlePrefab(launchSite, launchSite.pqsName)) {
                    bool ok = false;
                    if (launchSite.pqsCity != nullptr) ok = launchSite.Setup(launchSite.pqsCity, pqsArray);
                    else if (launchSite.pqsCity2 != nullptr) ok = launchSite.Setup(launchSite.pqsCity2, pqsArray);
                    if (ok) {
                        launchsites.push_back(&launchSite);
                        if (launchSite.pqsCity != nullptr) launchSite.pqsCity->launchSite = &launchSite;
                        else launchSite.pqsCity2->launchSite = &launchSite;
                        flag = true;
                        forcePQSPosition = true;   // a PQSMod_FlattenArea on the instantiated prefab (scene check: the hook decides)
                    }
                }
            }
            if (sceneHook && flag) sceneHook("SpawnAdditionalPrefabs:" + launchSite.name, nullptr);
        }
        if (flag) {
            if (std::find(spherestoSetup.begin(), spherestoSetup.end(), launchSite.launchsitePQS) == spherestoSetup.end())
                spherestoSetup.push_back(launchSite.launchsitePQS);
            if (launchSite.isPQSCity()) {
                PQSCity* city = launchSite.pqsCity;
                CelestialFrame cf;
                CelestialFrame::SetFrame(0.0, 0.0, 0.0, cf);
                const Vector3d vector3d = LatLonGetSurfaceNVector(cf, city->lat, city->lon) * (city->sphere->radius + city->alt);
                if (sceneHook) sceneHook("FloatingOrigin.SetOffset", &vector3d);
                co_yield YieldInstruction::WaitForFixedUpdate();
                if (forcePQSPosition) {
                    if (sceneHook) sceneHook("ForceInitPQS", &vector3d);
                    co_yield YieldInstruction::WaitForSeconds(2.0f);
                }
                city->Orientate();
                if (forcePQSPosition && sceneHook) sceneHook("PQS.SetTarget(null)", nullptr);
            }
            if (launchSite.isPQSCity2()) {
                PQSCity2* city = launchSite.pqsCity2;
                CelestialFrame cf2;
                CelestialFrame::SetFrame(0.0, 0.0, 0.0, cf2);
                const Vector3d vector3d2 = LatLonGetSurfaceNVector(cf2, city->lat, city->lon) * (city->sphere->radius + city->alt);
                if (sceneHook) sceneHook("FloatingOrigin.SetOffset", &vector3d2);
                co_yield YieldInstruction::WaitForFixedUpdate();
                city->Reset();
                if (sceneHook) sceneHook("PositionMobileLaunchPad.ResetPositioning", nullptr);
                if (forcePQSPosition) {
                    if (sceneHook) sceneHook("ForceInitPQS", &vector3d2);
                    co_yield YieldInstruction::WaitForSeconds(2.0f);
                }
                city->Orientate();
                if (forcePQSPosition && sceneHook) sceneHook("PQS.SetTarget(null)", nullptr);
            }
        } else {
            log::Info("PSystemSetup: Cannot add Stock launchsite named '" + stocklaunchsites[lsI]->launchSiteName + "' - Launchsite is not setup.");
            co_yield YieldInstruction::Null();
        }
    }
    for (size_t i = 0; i < spherestoSetup.size(); i++)
        if (sceneHook) sceneHook("PQS.ResetModList", nullptr);
    co_yield YieldInstruction::Null();
}

// PSystemSetup.cs:608
bool PSystemSetup::IsStockLaunchSite(const std::string& name) const {
    for (size_t num = 0; num < stocklaunchsites.size(); num++)
        if (stocklaunchsites[num]->name == name || stocklaunchsites[num]->launchSiteName == name) return true;
    return false;
}

// PSystemSetup.cs:628
bool PSystemSetup::IsFacilityOrLaunchSite(const std::string& inputName, bool& isFacility, std::string& displayName) {
    isFacility = false;
    displayName = "";
    if (IsFacility(inputName, displayName)) {
        isFacility = true;
        return true;
    }
    if (IsLaunchSite(inputName, displayName)) return true;
    return false;
}

// PSystemSetup.cs:641
bool PSystemSetup::IsFacility(const std::string& inputName, std::string& displayName) {
    displayName = "";
    for (size_t num = 0; num < facilities.size(); num++) {
        if (facilities[num].name == inputName || facilities[num].facilityName == inputName) {
            displayName = facilities[num].facilityDisplayName;
            return true;
        }
    }
    return false;
}

// PSystemSetup.cs:659
bool PSystemSetup::IsLaunchSite(const std::string& inputName, std::string& displayName) {
    displayName = "";
    for (size_t num = 0; num < launchsites.size(); num++) {
        if (launchsites[num]->name == inputName || launchsites[num]->launchSiteName == inputName) {
            displayName = launchsites[num]->launchSiteName;
            return true;
        }
    }
    return false;
}

// PSystemSetup.cs:682
LaunchSite* PSystemSetup::GetLaunchSite(const std::string& name) {
    for (size_t num = 0; num < launchsites.size(); num++) {
        if (launchsites[num]->name == name || launchsites[num]->launchSiteName == name) {
            if (launchsites[num]->BundleName == "makinghistory_assets" && SaveEnvironment::Current().IsExpansionInstalled("MakingHistory")) return launchsites[num];
            if (launchsites[num]->BundleName.empty() || launchsites[num]->BundleName == "stock") return launchsites[num];
        }
    }
    return nullptr;
}

// PSystemSetup.cs:709
CelestialBody* PSystemSetup::GetLaunchSiteBody(const std::string& name) {
    SpaceCenterFacility* spaceCenterFacility = GetSpaceCenterFacility(name);
    auto bodyOfPqsPtr = [this](PQS* pqs) -> CelestialBody* {
        for (const PQSInfo& p : pqsArray)
            if (p.pqs == pqs && bodyOfPQS) return bodyOfPQS(p);
        return nullptr;
    };
    if (spaceCenterFacility != nullptr) return bodyOfPqsPtr(spaceCenterFacility->facilityPQS);
    LaunchSite* site = GetLaunchSite(name);
    if (site == nullptr) return nullptr;
    return bodyOfPqsPtr(site->launchsitePQS);
}

// PSystemSetup.cs:719
std::string PSystemSetup::GetLaunchSiteBodyName(const std::string& name) {
    SpaceCenterFacility* spaceCenterFacility = GetSpaceCenterFacility(name);
    if (spaceCenterFacility != nullptr) return spaceCenterFacility->pqsName;
    LaunchSite* site = GetLaunchSite(name);
    return site != nullptr ? site->pqsName : std::string();
}

// PSystemSetup.cs:729
std::string PSystemSetup::GetLaunchSiteDisplayName(const std::string& name) {
    SpaceCenterFacility* spaceCenterFacility = GetSpaceCenterFacility(name);
    if (spaceCenterFacility != nullptr) return Localizer::Format(spaceCenterFacility->facilityDisplayName);
    LaunchSite* launchSite = GetLaunchSite(name);
    if (launchSite != nullptr) return Localizer::Format(launchSite->launchSiteName);
    return name;
}

// PSystemSetup.cs:745 (DontDestroyOnLoad of the city objects is the engine's)
bool PSystemSetup::AddLaunchSite(LaunchSite* launchsite, bool overRide) {
    if (GetLaunchSite(launchsite->launchSiteName) == nullptr) {
        if (!overRide && !launchsite->IsSetup()) {
            log::Info("PSystemSetup: Cannot add launchsite named '" + launchsite->launchSiteName + "' - Launchsite is not setup.");
            return false;
        }
        launchsites.push_back(launchsite);
        return true;
    }
    log::Info("PSystemSetup: Cannot add launchsite named '" + launchsite->launchSiteName + "' - Already exists.");
    return false;
}

// PSystemSetup.cs:779 (PQSCity.OnSphereInactive / DestroyGameObject are the engine's)
bool PSystemSetup::RemoveLaunchSite(const std::string& name) {
    LaunchSite* launchSite = GetLaunchSite(name);
    if (launchSite != nullptr) {
        if (sceneHook) sceneHook("RemoveLaunchSite:" + name, nullptr);
        launchsites.erase(std::remove(launchsites.begin(), launchsites.end(), launchSite), launchsites.end());
        return true;
    }
    log::Info("PSystemSetup: Cannot remove launchsite named '" + name + "' - Launchsite does not exist.");
    return false;
}

// PSystemSetup.cs:813
PSystemSetup::SpaceCenterFacility* PSystemSetup::GetSpaceCenterFacility(const std::string& name) {
    for (SpaceCenterFacility& f : facilities)
        if (f.name == name || f.facilityName == name) return &f;
    return nullptr;
}

// PSystemSetup.cs:835
void PSystemSetup::LoadTestScene(GameScenes sceneBase) {
    loadTestScene = true;
    nextScene = sceneBase;
}

// PSystemSetup.cs:826
void PSystemSetup::OnSceneLoaded(int buildIndex) { OnLevelLoaded(HighLogic::GetLoadedGameSceneFromBuildIndex(buildIndex)); }

// PSystemSetup.cs:843
void PSystemSetup::OnLevelLoaded(GameScenes level) {
    switch (level) {
        case GameScenes::MISSIONBUILDER: SetMissionEditor(); break;
        case GameScenes::LOADINGBUFFER: SetDisabled(); break;
        case GameScenes::MAINMENU: SetMainMenu(); break;
        default: SetDisabled(); break;
        case GameScenes::SPACECENTER: SetSpaceCentre(); break;
        case GameScenes::EDITOR: SetEditor(); break;
        case GameScenes::FLIGHT: SetFlight(); break;
        case GameScenes::TRACKSTATION: SetTrackingStation(); break;
        case GameScenes::PSYSTEM: break;
    }
}

// PSystemSetup.cs:879
void PSystemSetup::OnSceneChange(GameScenes scene) {
    (void)scene;
    if (FlightGlobals::bodies != nullptr) {
        for (CelestialBody* celestialBody : *FlightGlobals::bodies) {
            celestialBody->inverseRotation = false;
            if (celestialBody->orbitDriver) celestialBody->orbitDriver->reverse = false;
            if (sceneHook) sceneHook("SetupShadowCasting:" + celestialBody->bodyName, nullptr);
        }
    }
    ResetTransforms();
    SetDisabled();
}

namespace {
void PlanetariumSet(bool pause) {
    if (Planetarium::fetch != nullptr) Planetarium::fetch->pause = pause;
}
}  // namespace

// PSystemSetup.cs:902
void PSystemSetup::SetMainMenu() {
    PlanetariumSet(true);
    if (sceneHook) sceneHook("SetMainMenu", nullptr);
}

// PSystemSetup.cs:925
void PSystemSetup::SetSpaceCentre() {
    if (Planetarium::fetch != nullptr) {
        Planetarium::fetch->pause = false;
        Planetarium::fetch->timeScale = 1.0;
    }
    FlightGlobals::currentMainBody = kscBody;
    if (FlightGlobals::bodies != nullptr) {
        for (CelestialBody* celestialBody : *FlightGlobals::bodies) {
            celestialBody->inverseRotation = false;
            if (celestialBody->orbitDriver) celestialBody->orbitDriver->reverse = false;
        }
    }
    if (kscBody != nullptr) {
        kscBody->inverseRotation = true;
        if (kscBody->orbitDriver) kscBody->orbitDriver->reverse = true;
    }
    if (sceneHook) sceneHook("SetSpaceCentre", nullptr);
}

// PSystemSetup.cs:969
void PSystemSetup::SetMissionEditor() {
    SetMainMenu();
    if (sceneHook) sceneHook("SetMissionEditor", nullptr);
}

// PSystemSetup.cs:976
void PSystemSetup::SetFlight() {
    PlanetariumSet(false);
    if (sceneHook) sceneHook("SetFlight", nullptr);
}

// PSystemSetup.cs:993
void PSystemSetup::SetTrackingStation() {
    if (Planetarium::fetch != nullptr) {
        Planetarium::fetch->pause = false;
        Planetarium::fetch->timeScale = 1.0;
    }
    if (sceneHook) sceneHook("SetTrackingStation", nullptr);
}

// PSystemSetup.cs:1016
void PSystemSetup::SetEditor() {
    PlanetariumSet(true);
    if (sceneHook) sceneHook("SetEditor", nullptr);
}

// PSystemSetup.cs:1040
void PSystemSetup::SetDisabled() {
    PlanetariumSet(true);
    FlightGlobals::currentMainBody = nullptr;
    if (sceneHook) sceneHook("SetDisabled", nullptr);
}

// PSystemSetup.cs:1109
void PSystemSetup::SetPQSInactive(bool keepActiveInFlight) {
    if (sceneHook) sceneHook(keepActiveInFlight ? "SetPQSInactive(keepActiveInFlight)" : "SetPQSInactive", nullptr);
}

// PSystemSetup.cs:1130
void PSystemSetup::SetPQSActive(PQS* pqs) {
    (void)pqs;
    if (sceneHook) sceneHook("SetPQSActive(pqs)", nullptr);
}

// PSystemSetup.cs:1146
void PSystemSetup::SetPQSActive() {
    if (sceneHook) sceneHook("SetPQSActive", nullptr);
}

// PSystemSetup.cs:1157
void PSystemSetup::SetPQSDisabled() {
    if (sceneHook) sceneHook("SetPQSDisabled", nullptr);
}

// PSystemSetup.cs:1167
void PSystemSetup::ResetTransforms() {
    PlanetariumSet(true);
    if (sceneHook) sceneHook("ResetTransforms", nullptr);
}

// ---- seams --------------------------------------------------------------------------------------------------------------------

void PSystemSetup::InstallSeams() {
    CareerEnvironment& env = CareerEnvironment::Current();
    env.spaceCenterBody = [] { return SpaceCenter::Instance != nullptr ? SpaceCenter::Instance->cb : nullptr; };
    env.spaceCenterGreatCircleDistance = [](double lat, double lon) {
        SpaceCenter* sc = SpaceCenter::Instance;
        if (sc == nullptr) throw NullReferenceException();
        return sc->GreatCircleDistance(sc->cb->GetRelSurfaceNVector(lat, lon));
    };
    env.launchSites = [] {
        std::vector<CareerEnvironment::LaunchSiteInfo> out;
        if (PSystemSetup::Instance == nullptr) throw NullReferenceException();
        for (LaunchSite* ls : PSystemSetup::Instance->launchsites) {
            CareerEnvironment::LaunchSiteInfo info;
            info.name = ls->name;
            info.requiresPOIVisit = ls->requiresPOIVisit;
            info.body = ls->Body();
            info.launchSiteName = ls->launchSiteName;
            info.editorFacilityDisplay = util::displayDescription(ls->editorFacility);
            out.push_back(std::move(info));
        }
        return out;
    };
    env.launchSiteFound = [](const std::string& launchSiteName) {
        if (PSystemSetup::Instance == nullptr) return;
        for (LaunchSite* ls : PSystemSetup::Instance->launchsites)
            if (ls->name == launchSiteName) {
                GE::LaunchSiteFound.Fire(ls);
                return;
            }
    };
    env.isStockLaunchSite = [](const std::string& name) { return PSystemSetup::Instance != nullptr && PSystemSetup::Instance->IsStockLaunchSite(name); };
    env.launchSiteDistance = [](const std::string& name, CelestialBody*& siteBody, double lat, double lon, double& distance) {
        LaunchSite* launchSite = PSystemSetup::Instance != nullptr ? PSystemSetup::Instance->GetLaunchSite(name) : nullptr;
        if (launchSite == nullptr) return false;
        siteBody = launchSite->Body();
        distance = launchSite->GreatCircleDistance(siteBody->GetRelSurfaceNVector(lat, lon));
        return true;
    };
    env.spaceCenterFacilities = [] {
        std::vector<CareerEnvironment::SpaceCenterFacilityInit> out;
        if (PSystemSetup::Instance == nullptr) throw NullReferenceException();
        for (auto& f : PSystemSetup::Instance->facilities) {
            CareerEnvironment::SpaceCenterFacilityInit i;
            i.facilityName = f.facilityName;
            i.upgradeableObjectId = f.upgradeableObjectInParentId;
            i.upgradeableFacilityId = f.upgradeableFacilityId;
            i.maxLevel = f.upgradeableFacilityMaxLevel;
            out.push_back(std::move(i));
        }
        return out;
    };
    PreFlightTests::PreFlightEnvironment& pf = PreFlightTests::PreFlightEnvironment::Current();
    pf.isSpaceCenterFacility = [](const std::string& n) { return PSystemSetup::Instance != nullptr && PSystemSetup::Instance->GetSpaceCenterFacility(n) != nullptr; };
    pf.facilityDamage = [](const std::string& n) -> std::optional<float> {
        PSystemSetup::SpaceCenterFacility* f = PSystemSetup::Instance != nullptr ? PSystemSetup::Instance->GetSpaceCenterFacility(n) : nullptr;
        if (f == nullptr) return std::nullopt;
        return f->GetFacilityDamage();
    };
    pf.isLaunchSite = [](const std::string& n) { return PSystemSetup::Instance != nullptr && PSystemSetup::Instance->GetLaunchSite(n) != nullptr; };
    Vessel::env.isFacilityOrLaunchSite = [](const std::string& landedAt, bool& isFacility, std::string& displayName) {
        if (PSystemSetup::Instance == nullptr) throw NullReferenceException();
        return PSystemSetup::Instance->IsFacilityOrLaunchSite(landedAt, isFacility, displayName);
    };
    ShipConstruction::env.isStockLaunchSite = [](const std::string& name) { return PSystemSetup::Instance != nullptr && PSystemSetup::Instance->IsStockLaunchSite(name); };
}

void PSystemSetup::UninstallSeams() {
    CareerEnvironment& env = CareerEnvironment::Current();
    env.spaceCenterBody = nullptr;
    env.spaceCenterGreatCircleDistance = nullptr;
    env.launchSites = nullptr;
    env.launchSiteFound = nullptr;
    env.isStockLaunchSite = nullptr;
    env.launchSiteDistance = nullptr;
    env.spaceCenterFacilities = nullptr;
    PreFlightTests::PreFlightEnvironment& pf = PreFlightTests::PreFlightEnvironment::Current();
    pf.isSpaceCenterFacility = nullptr;
    pf.facilityDamage = nullptr;
    pf.isLaunchSite = nullptr;
    Vessel::env.isFacilityOrLaunchSite = nullptr;
    ShipConstruction::env.isStockLaunchSite = nullptr;
}

}  // namespace ksp
