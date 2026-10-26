// See include/ksp/ship/ship_construction.hpp. Spec: Assembly-CSharp/ShipConstruction.cs
#include "ksp/kerbals/crew_manifest.hpp"
#include "ksp/ship/ship_construction.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_geometry_util.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_assembly.hpp"

namespace ksp {

using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;
namespace fs = std::filesystem;

namespace {

std::string AppRoot() { return KSPUtil::ApplicationRootPath(); }

// `value.Substring(0, value.IndexOf('_'))` (IndexOf == -1 is an ArgumentOutOfRangeException)
std::string PartNameBeforeUnderscore(const std::string& value) {
    const size_t i = value.find('_');
    if (i == std::string::npos) throw ArgumentException("Length cannot be less than zero. (Parameter 'length')");
    return value.substr(0, i);
}

const AvailablePart* PartInfoByName(const std::string& name) {
    SaveEnvironment& env = SaveEnvironment::Current();
    return env.getPartInfoByName ? env.getPartInfoByName(name) : nullptr;
}

// Path.GetFullPath / DirectoryInfo.FullName without a trailing separator
std::string FullName(const std::string& path) {
    fs::path p = fs::absolute(dotnet::PathFromUtf8(path)).lexically_normal();
    std::u8string s = p.u8string();
    while (s.size() > 1 && (s.back() == u8'/' || s.back() == u8'\\')) s.pop_back();
    return std::string(s.begin(), s.end());
}

std::string ParentOf(const std::string& fullName) {  // DirectoryInfo.Parent.FullName ("" when there is none)
    const fs::path p = dotnet::PathFromUtf8(fullName);
    if (!p.has_parent_path() || p.parent_path() == p) return "";
    const std::u8string s = p.parent_path().u8string();
    return std::string(s.begin(), s.end());
}

std::string NameOf(const std::string& fullName) {  // DirectoryInfo.Name
    const std::u8string s = dotnet::PathFromUtf8(fullName).filename().u8string();
    return std::string(s.begin(), s.end());
}

// Retains ProtoVessels removed from a FlightState: in C# the vessel's `protoVessel` reference keeps the object alive
std::vector<std::shared_ptr<ProtoVessel>>& RemovedProtoVessels() {
    static std::vector<std::shared_ptr<ProtoVessel>> removed;
    return removed;
}
std::shared_ptr<ProtoVessel> TakeOut(FlightState& state, ProtoVessel* pv) {
    for (auto it = state.protoVessels.begin(); it != state.protoVessels.end(); ++it) {
        if (it->get() == pv) {
            std::shared_ptr<ProtoVessel> out = std::move(*it);
            state.protoVessels.erase(it);
            return out;
        }
    }
    return nullptr;
}
}  // namespace

// ---- backups (ShipConstruction.cs:28-72) ----------------------------------------------------------------------------------------------

// ShipConstruction.cs:28
void ShipConstruction::ClearBackups() { backups.clear(); }

// ShipConstruction.cs:33
void ShipConstruction::CreateBackup(ShipConstruct& ship) {
    ShipConfig = ship.SaveShip();
    ShipType = ship.shipFacility;
    backups.push_back(ShipConfig);
}

// ShipConstruction.cs:40
void ShipConstruction::ShiftAndCreateBackup(ShipConstruct& ship) {
    for (int i = 0; i < static_cast<int>(backups.size()) - 1; i++) backups[static_cast<size_t>(i)] = backups[static_cast<size_t>(i) + 1];
    ShipConfig = ship.SaveShip();
    ShipType = ship.shipFacility;
    if (backups.empty()) throw ArgumentException("Index was out of range. Must be non-negative and less than the size of the collection.");
    backups.back() = ShipConfig;
}

// ShipConstruction.cs:51 (the SetLayerRecursive of each part is Unity)
std::unique_ptr<ShipConstruct> ShipConstruction::RestoreBackup(int index) {
    if (index < 0 || index >= static_cast<int>(backups.size()))
        throw ArgumentException("Index was out of range. Must be non-negative and less than the size of the collection.");
    ShipConfig = backups[static_cast<size_t>(index)];
    auto shipConstruct = std::make_unique<ShipConstruct>();
    shipConstruct->LoadShipWithParts(*ShipConfig);
    ShipType = shipConstruct->shipFacility;
    return shipConstruct;
}

// ShipConstruction.cs:64
void ShipConstruction::DebugBackup() {
    for (const auto& b : backups) log::Info(b->ToString());
}

// ---- load / save (ShipConstruction.cs:73-165) ---------------------------------------------------------------------------------------------

// ShipConstruction.cs:73 (AutoGenerateThumbnail is a UI coroutine)
std::unique_ptr<ShipConstruct> ShipConstruction::LoadShip(const std::string& filePath) {
    ConfigNode::NodePtr configNode = ConfigNode::Load(filePath);
    if (!configNode) {
        log::Error("File '" + filePath + "' not found!");
        return nullptr;
    }
    auto shipConstruct = std::make_unique<ShipConstruct>();
    if (!shipConstruct->LoadShipWithParts(*configNode)) {
        log::Error("Ship file error!");
        return nullptr;
    }
    if (shipConstruct->steamPublishedFileId == 0 && env.steamIdFromSteamFolder)
        shipConstruct->steamPublishedFileId = env.steamIdFromSteamFolder(filePath);
    ShipConfig = configNode;
    return shipConstruct;
}

// ShipConstruction.cs:96
std::unique_ptr<ShipConstruct> ShipConstruction::LoadShip() {
    if (!ShipConfig) {
        log::Error("Backup ship config not found!");
        return nullptr;
    }
    auto shipConstruct = std::make_unique<ShipConstruct>();
    if (!shipConstruct->LoadShipWithParts(*ShipConfig)) {
        log::Error("Ship file error!");
        return nullptr;
    }
    return shipConstruct;
}

// ShipConstruction.cs:112
std::string ShipConstruction::SaveShip(ShipConstruct& ship, const std::string& shipFilename) {
    const std::string text = AppRoot() + "saves/" + env.SaveFolder + "/Ships/" + GetShipsSubfolderFor(env.editorFacility) + "/" + shipFilename + ".craft";
    ship.SaveShip()->Save(text);
    CaptureThumbnail(&ship, "thumbs", env.SaveFolder + "_" + GetShipsSubfolderFor(env.editorFacility) + "_" + shipFilename);
    return text;
}

// ShipConstruction.cs:120
std::string ShipConstruction::SaveShip(const std::string& shipFilename) {
    if (!ShipConfig) return "";
    const std::string savePath = GetSavePath(shipFilename);
    ShipConfig->Save(savePath);
    CaptureThumbnail(env.editorShip, "thumbs", env.SaveFolder + "_" + GetShipsSubfolderFor(env.editorFacility) + "_" + shipFilename);
    return savePath;
}

// ShipConstruction.cs:132
std::string ShipConstruction::SaveShipToPath(std::string shipName, const std::string& path) {
    if (!ShipConfig) return "";
    shipName = KSPUtil::SanitizeString(shipName, '_', true);
    ShipConfig->Save(path + "/" + shipName + ".craft");
    const std::string playerCraftThumbnailName = GetPlayerCraftThumbnailName(path, shipName);
    std::string text = "thumbs";
    if (env.CurrentGame == nullptr) throw NullReferenceException();
    if (env.CurrentGame->Mode == Game_Modes::MISSION_BUILDER) text += "/test_missions";
    else if (env.CurrentGame->Mode == Game_Modes::MISSION) text += "/missions";
    CaptureThumbnail(env.editorShip, text, playerCraftThumbnailName);
    return path;
}

// ShipConstruction.cs:154
std::string ShipConstruction::SaveShipToPath(const std::string& gameFolder, EditorFacility editorFacility, const std::string& localPath,
                                             std::string shipName) {
    if (!ShipConfig) return "";
    shipName = KSPUtil::SanitizeString(shipName, '_', true);
    const std::string text = GetShipsPathFor(gameFolder, editorFacility) + "/" + localPath;
    ShipConfig->Save(text + "/" + shipName + ".craft");
    const std::string playerCraftThumbnailName = GetPlayerCraftThumbnailName(text, shipName);
    CaptureThumbnail(env.editorShip, "thumbs", playerCraftThumbnailName);
    return text;
}

// ShipConstruction.cs:168
std::string ShipConstruction::GetPlayerCraftThumbnailName(const std::string& fullpath, std::string craftFileName) {
    try {
        craftFileName = KSPUtil::SanitizeString(craftFileName, '_', true);
        const std::string fileFullName = FullName(fullpath + "/" + craftFileName + ".craft");  // new FileInfo(...).FullName
        const EditorFacility facility = CheckCraftFileType(fileFullName);
        std::string empty, empty2;
        if (env.CurrentGame == nullptr) throw NullReferenceException();
        if (env.CurrentGame->IsMissionMode()) {
            if (!env.mission) return "";  // !MissionSystem.HasMissions
            empty = env.mission->folderName;
            empty2 = "/" + GetShipsSubfolderFor(facility);
        } else {
            const std::string root = FullName(GetRootCraftSavePath());
            std::string directoryInfo2 = FullName(fullpath);
            std::string directoryInfo3;
            bool have3 = false;
            while (directoryInfo2 != root) {
                directoryInfo3 = directoryInfo2;
                have3 = true;
                directoryInfo2 = ParentOf(directoryInfo2);
                if (directoryInfo2.empty()) throw NullReferenceException();  // DirectoryInfo.Parent == null
            }
            if (!have3) throw NullReferenceException();
            empty = NameOf(directoryInfo3);
            const std::string parent = ParentOf(FullName(GetShipsPathFor(empty, facility)));
            const std::string dir = ParentOf(fileFullName);  // fileInfo.Directory.FullName
            if (dir.size() < parent.size()) throw ArgumentException("startIndex cannot be larger than length of string.");
            empty2 = dir.substr(parent.size());
        }
        return GetPlayerCraftThumbnailName(empty, empty2, craftFileName);
    } catch (...) {
        return "";
    }
}

// ShipConstruction.cs:208
std::string ShipConstruction::GetPlayerCraftThumbnailName(const std::string& gameName, std::string subDirectoryPath, std::string shipName) {
    const std::string text = KSPUtil::SanitizeFilename(gameName);
    subDirectoryPath = dotnet::Replace(subDirectoryPath, "\\n", "/n");
    subDirectoryPath = dotnet::Replace(subDirectoryPath, "\n", "/n");
    subDirectoryPath = dotnet::Replace(subDirectoryPath, "\\t", "/t");
    subDirectoryPath = dotnet::Replace(subDirectoryPath, "\t", "/t");
    const std::string text2 = KSPUtil::SanitizeFilename(subDirectoryPath);
    shipName = KSPUtil::SanitizeString(shipName, '_', true);
    return text + text2 + "_" + shipName;
}

// ShipConstruction.cs:218
std::string ShipConstruction::GetSavePath(std::string shipName) {
    if (shipName.empty()) return "";
    shipName = KSPUtil::SanitizeString(shipName, '_', true);
    SaveEnvironment& se = SaveEnvironment::Current();
    if (env.CurrentGame != nullptr && env.CurrentGame->Mode == Game_Modes::MISSION && se.IsExpansionInstalled("MakingHistory") && env.mission)
        return AppRoot() + "saves/" + env.SaveFolder + "/Ships/" + GetShipsSubfolderFor(env.editorFacility) + "/" + shipName + ".craft";
    if (env.CurrentGame != nullptr && env.CurrentGame->Mode == Game_Modes::MISSION_BUILDER && se.IsExpansionInstalled("MakingHistory") && env.mission)
        return env.mission->FolderPath + "Ships/" + GetShipsSubfolderFor(env.editorFacility) + "/" + shipName + ".craft";
    return env.DefaultCraftSavePath + "/" + shipName + ".craft";
}

// ShipConstruction.cs:236
std::string ShipConstruction::GetCurrentGameShipsPathFor(EditorFacility facility) { return GetShipsPathFor(env.SaveFolder, facility); }

// ShipConstruction.cs:241
std::string ShipConstruction::GetShipsPathFor(const std::string& gameName, EditorFacility facility) {
    return GetShipsPathFor(gameName) + "Ships/" + GetShipsSubfolderFor(facility) + "/";
}

// ShipConstruction.cs:246
std::string ShipConstruction::GetShipsPathFor(const std::string& gameName) { return GetRootCraftSavePath() + "/" + gameName + "/"; }

// ShipConstruction.cs:251
std::string ShipConstruction::GetRootCraftSavePath() { return AppRoot() + "saves"; }

// ShipConstruction.cs:256
EditorFacility ShipConstruction::CheckCraftFileType(const std::string& filePath) {
    ConfigNode::NodePtr configNode = ConfigNode::Load(filePath);
    if (!configNode) return EditorFacility::None;
    if (!configNode->HasValue("type")) return EditorFacility::None;
    const std::string& value = *configNode->GetValuePtr("type");
    if (value == "SPH") return EditorFacility::SPH;
    return EditorFacility::VAB;
}

// ShipConstruction.cs:278
std::string ShipConstruction::GetShipsSubfolderFor(EditorFacility facility) {
    switch (facility) {
        case EditorFacility::VAB: return "VAB";
        case EditorFacility::SPH: return "SPH";
        default: return "";
    }
}

// ---- AssembleForLaunch (ShipConstruction.cs:291-434) ------------------------------------------------------------------------------------

// ShipConstruction.cs:291
std::unique_ptr<Vessel> ShipConstruction::AssembleForLaunch(ShipConstruct& ship, const std::string& landedAt, const std::string& displaylandedAt,
                                                            const std::string& flagURL, Game& sceneState, VesselCrewManifest* crewManifest,
                                                            const LaunchPlacement& placement) {
    return AssembleForLaunch(ship, landedAt, displaylandedAt, flagURL, sceneState, crewManifest, true, true, true, false, nullptr, false, false,
                             placement);
}

// ShipConstruction.cs:296. The Vessel component, orbit driver, naming, landed state, flight ids, crew list and reference transform are
// the shared AssembleVesselFromParts (vessel/vessel_assembly.cpp, which WP-14 wrote for the craft-file route). Not ported:
// PSystemSetup.GetSpaceCenterFacility / IsStockLaunchSite (a lookup whose result is discarded), AnalyticsUtil.LogVesselLaunched.
std::unique_ptr<Vessel> ShipConstruction::AssembleForLaunch(ShipConstruct& ship, const std::string& landedAt, const std::string& displaylandedAt,
                                                            const std::string& flagURL, Game& sceneState, VesselCrewManifest* crewManifest,
                                                            bool fromShipAssembly, bool setActiveVessel, bool isLanded, bool preCreate,
                                                            std::shared_ptr<Orbit> orbit, bool orbiting, bool isSplashed,
                                                            const LaunchPlacement& placement) {
    if (ship.liveParts.empty()) throw ArgumentException("Index was out of range. Must be non-negative and less than the size of the collection.");
    if (ship.ownedParts.size() != ship.liveParts.size())
        throw dotnet::InvalidOperationException("AssembleForLaunch: the ship does not own its parts (the Vessel takes them over)");
    if (crewManifest == nullptr) throw NullReferenceException();
    if (env.CurrentGame == nullptr) throw NullReferenceException();
    AssembleOptions o;
    o.rootPosition = placement.rootPosition;
    o.rootRotation = placement.rootRotation;
    o.landedAt = landedAt;
    o.displaylandedAt = displaylandedAt;
    o.flagURL = flagURL;
    o.isLanded = isLanded;
    o.isSplashed = isSplashed;
    o.setActiveVessel = setActiveVessel;
    o.fromShipAssembly = fromShipAssembly;
    o.preCreate = preCreate;
    o.orbiting = orbiting;
    o.orbit = std::move(orbit);
    o.launchID = env.CurrentGame->launchID++;  // :349 HighLogic.CurrentGame.launchID++
    o.flightState = sceneState.flightState.get();
    o.crewManifestAssign = [&ship, crewManifest]() { crewManifest->AssignCrewToVessel(ship); };
    // the parts leave the ship: Part.ship must not dangle
    for (Part* p : ship.liveParts)
        if (p->ship == &ship) p->ship = nullptr;
    std::vector<std::unique_ptr<Part>> owned = std::move(ship.ownedParts);
    ship.ownedParts.clear();
    return AssembleVesselFromParts(std::move(owned), ship, o);
}

// ShipConstruction.cs:436
uint32_t ShipConstruction::GetUniqueFlightID(const FlightState* flightState) {
    uint32_t hashCode;
    do {
        hashCode = static_cast<uint32_t>(SaveEnvironment::Current().NewGuidHashCode());
    } while (flightState != nullptr && flightState->ContainsFlightID(hashCode) && hashCode != 0);
    return hashCode;
}

// ShipConstruction.cs:447
Part* ShipConstruction::findFirstCrewablePart(Part* part) {
    if (part->fields.CrewCapacity > 0 && !part->protoModuleCrew.empty() && part->fields.isControlSource > 0) return part;
    for (Part* c : part->children)
        if (Part* p = findFirstCrewablePart(c)) return p;
    return nullptr;
}

// ShipConstruction.cs:473
Part* ShipConstruction::findFirstPod_Placeholder(Part* part) {
    if (part->fields.CrewCapacity > 0 && part->fields.isControlSource > 0) return part;
    for (Part* c : part->children)
        if (Part* p = findFirstPod_Placeholder(c)) return p;
    return nullptr;
}

// ShipConstruction.cs:499
Part* ShipConstruction::findFirstControlSource(Vessel& v) {
    for (Part* p : v.parts)
        if (p->fields.isControlSource > 0) return p;
    return nullptr;
}

// ShipConstruction.cs:521. The collider bounds are Part::colliderBounds (world space, the enabled non-layer-21 colliders of the part's
// own model, filled by the model reader); the descendants' colliders count for the part (GetComponentsInChildren). Transform.Translate /
// RotateAround are native Unity: plain float32, not verified against a capture. Moves every part of the ship (the children hang under
// the root transform in Unity).
void ShipConstruction::PutShipToGround(ShipConstruct& ship, const SpawnPoint& spawnPoint) {
    PartHeightQuery partHeightQuery(3.40282347E+38f);  // float.MaxValue
    std::vector<Part*>& parts = ship.liveParts;
    std::function<float(Part&)> lowestOf = [&lowestOf](Part& p) {
        float lowest = 3.40282347E+38f;
        for (const Bounds& b : p.colliderBounds) lowest = Mathf::Min(lowest, b.min().y);
        for (Part* c : p.children) lowest = Mathf::Min(lowest, lowestOf(*c));
        return lowest;
    };
    const int count = static_cast<int>(parts.size());
    for (int i = 0; i < count; i++) {
        Part* part = parts[static_cast<size_t>(i)];
        partHeightQuery.lowestOnParts[part] = 3.40282347E+38f;
        for (const Bounds& b : part->colliderBounds) {
            partHeightQuery.lowestPoint = Mathf::Min(partHeightQuery.lowestPoint, b.min().y);
            partHeightQuery.lowestOnParts[part] = Mathf::Min(partHeightQuery.lowestOnParts[part], b.min().y);
        }
        for (Part* c : part->children) {
            const float l = lowestOf(*c);
            partHeightQuery.lowestPoint = Mathf::Min(partHeightQuery.lowestPoint, l);
            partHeightQuery.lowestOnParts[part] = Mathf::Min(partHeightQuery.lowestOnParts[part], l);
        }
    }
    if (env.onPutToGround)
        for (int k = 0; k < count; k++) env.onPutToGround(*parts[static_cast<size_t>(k)], partHeightQuery);
    log::Info("putting ship to ground: " + dotnet::FormatSingleG(partHeightQuery.lowestPoint));
    // Quaternion.ToAngleAxis (native): angle = 2 acos(w) in degrees, axis = xyz / sqrt(1 - w^2) (x axis when the angle is ~0)
    const Quaternionf& q = spawnPoint.rotation;
    float angle = f32store(fx(f32store(2.0 * fx(Mathf::Acos(q.w)))) * fx(Mathf::Rad2Deg));
    Vector3f axis{1.0f, 0.0f, 0.0f};
    if (!(Mathf::Abs(angle) < 1e-6f)) {
        const float div = f32store(1.0 / fx(Mathf::Sqrt(f32store(1.0 - fx(q.w) * fx(q.w)))));
        axis = Vector3f{f32store(fx(q.x) * fx(div)), f32store(fx(q.y) * fx(div)), f32store(fx(q.z) * fx(div))};
    }
    Vector3f translation{0.0f, f32store(0.0 - fx(partHeightQuery.lowestPoint)), 0.0f};
    translation = translation + spawnPoint.position;
    Part* root = parts[0]->localRoot();
    const Quaternionf rot = UnityNative::AngleAxis(angle, axis);
    for (Part* p : parts) {
        if (p->localRoot() != root) continue;
        p->partTransform.position = p->partTransform.position + translation;  // Translate(translation, Space.World)
        const Vector3f v2 = rot * (p->partTransform.position - spawnPoint.position);  // RotateAround(spawnPoint.position, axis, angle)
        p->partTransform.position = spawnPoint.position + v2;
        p->partTransform.rotation = rot * p->partTransform.rotation;
    }
}

// ---- craft size, centre, MOI (ShipConstruction.cs:555-652) --------------------------------------------------------------------------------

// ShipConstruction.cs:555
Vector3f ShipConstruction::CalculateCraftSize(ShipConstruct& ship) {
    if (ship.liveParts.empty()) return Vector3f::zero();
    return CalculateCraftSize(ship.liveParts, ship.liveParts[0]);
}

// ShipConstruction.cs:564 (the renderer bounds are the model reader's: parts without them add nothing)
Vector3f ShipConstruction::CalculateCraftSize(const std::vector<Part*>& parts, Part* rootPart) {
    if (parts.empty() || rootPart == nullptr) return Vector3f::zero();
    std::vector<Bounds> list;
    for (Part* part : parts) {
        if (part->FindModuleOfClass("LaunchClamp") != nullptr) continue;
        std::vector<Bounds> partRendererBounds = PartGeometryUtil::GetPartRendererBounds(*part);
        for (Bounds item : partRendererBounds) {
            item.set_size(item.size() * part->fields.boundsMultiplier);
            const Vector3f size = item.size();
            item.Expand(part->GetModuleSize(size));
            list.push_back(item);
        }
    }
    if (list.empty()) return Vector3f::zero();
    // rootPart.transform.root: the root part's own transform (the root of the ship hierarchy)
    Part* top = rootPart->localRoot();
    return PartGeometryUtil::MergeBounds(list, top->partTransform.position).size();
}

// ShipConstruction.cs:599
Vector3f ShipConstruction::CalculateCraftSize(const ShipTemplate& ship) {
    if (ship.shipSize != Vector3f::zero()) return ship.shipSize;
    return Vector3f::zero();
}

// ShipConstruction.cs:608
Vector3f ShipConstruction::FindCraftCenter(ShipConstruct& ship) { return FindCraftCenter(ship, false); }

// ShipConstruction.cs:613
Vector3f ShipConstruction::FindCraftCenter(ShipConstruct& ship, bool excludeClamps) {
    std::vector<Bounds> list;
    for (Part* p : ship.liveParts) {
        if (!excludeClamps || p->FindModuleOfClass("LaunchClamp") == nullptr) {
            std::vector<Bounds> b = PartGeometryUtil::GetPartRendererBounds(*p);
            list.insert(list.end(), b.begin(), b.end());
        }
    }
    if (list.empty() && !ship.liveParts.empty()) {
        std::vector<Bounds> b = PartGeometryUtil::GetPartRendererBounds(*ship.liveParts[0]);
        list.insert(list.end(), b.begin(), b.end());
    }
    return PartGeometryUtil::FindBoundsCentroid(list, nullptr);
}

// ShipConstruction.cs:631
Vector3f ShipConstruction::FindCraftMOI(Part& rootPart, const Vector3f& CoM) { return FindCraftMOI(rootPart, CoM, Vector3f::zero()); }

// ShipConstruction.cs:636. Part.WCoM without a rigidbody is `partTransform.position + partTransform.rotation * CoMOffset`
// (Part.cs:1014); `localRoot.GetReferenceTransform().rotation` is the local root's transform rotation (Quaternion.Inverse is native: the
// conjugate of a unit rotation).
Vector3f ShipConstruction::FindCraftMOI(Part& part, const Vector3f& CoM, Vector3f MOI) {
    const Quaternionf& r = part.localRoot()->partTransform.rotation;
    const Quaternionf inv(0.0f - r.x, 0.0f - r.y, 0.0f - r.z, r.w);
    const Vector3f wcom = part.partTransform.position + part.partTransform.rotation * part.fields.CoMOffset;
    const Vector3f vector = inv * (wcom - CoM);
    const float num = f32store(fx(part.fields.mass) + fx(part.GetResourceMass()));
    const double vx = vector.x, vy = vector.y, vz = vector.z;
    const Vector3f term{f32store(vy * vy + vz * vz), f32store(vx * vx + vz * vz), f32store(vx * vx + vy * vy)};
    MOI = MOI + term * num;
    for (Part* part2 : part.children) {
        if (part2->isAttached() && part2->State() != PartStates::DEAD) MOI = FindCraftMOI(*part2, CoM, MOI);
    }
    return MOI;
}

// ---- launch-site queries on the saved flight state (ShipConstruction.cs:654-777) -----------------------------------------------------------------

// ShipConstruction.cs:654
bool ShipConstruction::CheckLaunchSiteClear(FlightState* flightState, const std::string& launchSiteName, bool doCleanup) {
    int firstVesselOnPadIndex;
    return CheckLaunchSiteClear(flightState, launchSiteName, doCleanup, firstVesselOnPadIndex);
}

// ShipConstruction.cs:661
bool ShipConstruction::CheckLaunchSiteClear(FlightState* flightState, const std::string& launchSiteName, bool doCleanup, int& firstVesselOnPadIndex) {
    bool flag = true;
    firstVesselOnPadIndex = -1;
    if (flightState == nullptr) {
        firstVesselOnPadIndex = -1;
        return true;
    }
    std::vector<ProtoVessel*> list;
    VesselType vesselType = VesselType::Debris;
    const int count = static_cast<int>(flightState->protoVessels.size());
    for (int i = 0; i < count; i++) {
        ProtoVessel* protoVessel = flightState->protoVessels[static_cast<size_t>(i)].get();
        if (protoVessel->landedAt.find(launchSiteName) != std::string::npos) {
            if (static_cast<int>(protoVessel->vesselType) > static_cast<int>(vesselType) || firstVesselOnPadIndex == -1) {
                vesselType = protoVessel->vesselType;
                firstVesselOnPadIndex = i;  // flightState.protoVessels.IndexOf(protoVessel)
            }
            list.push_back(protoVessel);
            flag = false;
        }
    }
    if (!flag && doCleanup) {
        log::Info("[Ship Assembly]: WARNING! - " + launchSiteName + " has vessels on it. - Removing before something stupid happens.");
        for (ProtoVessel* pv : list) {
            GE::onVesselRecovered.Fire(pv, false);
            if (auto out = TakeOut(*flightState, pv)) RemovedProtoVessels().push_back(std::move(out));
        }
    }
    return flag;
}

// ShipConstruction.cs:701
std::vector<ProtoVessel*> ShipConstruction::FindVesselsLandedAt(const FlightState* flightState, const std::string& landedAt) {
    std::vector<ProtoVessel*> list;
    if (flightState != nullptr) {
        for (const auto& pv : flightState->protoVessels)
            if (pv->landedAt.find(landedAt) != std::string::npos) list.push_back(pv.get());
    }
    return list;
}

// ShipConstruction.cs:718
void ShipConstruction::FindVesselsLandedAt(const FlightState* flightState, const std::string& landedAt, int& count, std::string& name, int& idx,
                                           VesselType& vType) {
    count = 0;
    name = "";
    idx = 0;
    vType = VesselType::Debris;
    bool flag = true;
    if (flightState == nullptr) return;
    const int count2 = static_cast<int>(flightState->protoVessels.size());
    for (int i = 0; i < count2; i++) {
        const ProtoVessel* protoVessel = flightState->protoVessels[static_cast<size_t>(i)].get();
        if (protoVessel->landedAt.find(landedAt) != std::string::npos) {
            count++;
            if (flag || static_cast<int>(protoVessel->vesselType) > static_cast<int>(VesselType::Debris)) {
                vType = protoVessel->vesselType;
                name = protoVessel->vesselName;
                idx = i;
                flag = false;
            }
        }
    }
}

// ShipConstruction.cs:748 [Obsolete("Use FindVesselsLandedAt instead.")]: the body is `return FindVesselsAtLaunchSite(flightState, launchSiteName);`
// -- unbounded recursion, a StackOverflowException in the game, which ends the process
std::vector<ProtoVessel*> ShipConstruction::FindVesselsAtLaunchSite(const FlightState*, const std::string&) {
    throw Exception("StackOverflowException: ShipConstruction.FindVesselsAtLaunchSite calls itself");
}

// ShipConstruction.cs:753
void ShipConstruction::RecoverVesselFromFlight(ProtoVessel& vessel, FlightState& fromState) { RecoverVesselFromFlight(vessel, fromState, false); }

// ShipConstruction.cs:758
void ShipConstruction::RecoverVesselFromFlight(ProtoVessel& vessel, FlightState& fromState, bool quick) {
    GE::onVesselRecovered.Fire(&vessel, quick);
    if (auto out = TakeOut(fromState, &vessel)) RemovedProtoVessels().push_back(std::move(out));  // protoVessels.Remove(vessel)
    if (vessel.vesselRef != nullptr) {
        if (vessel.vesselRef->loaded) {
            if (vessel.vesselRef == FlightRegistry::ActiveVessel) {
                log::Error("[Vessel Removal]: " + vessel.GetDisplayName() + " is the active vessel. Cannot remove.");
                return;
            }
            log::Warning("[Vessel Removal]: " + vessel.GetDisplayName() + " is loaded. Unloading before removal. ");
            vessel.vesselRef->Unload();
        }
        if (destroyVesselGameObject) destroyVesselGameObject(*vessel.vesselRef);  // Object.DestroyImmediate(vesselRef.gameObject)
    }
    log::Info("Vessel " + vessel.GetDisplayName() + " recovered");
}

// ---- templates, subassemblies (ShipConstruction.cs:937-985, 1113-1195) ---------------------------------------------------------------------------

// ShipConstruction.cs:829 (HighlightRecursive and the camera angles are UI)
void ShipConstruction::CaptureStockThumbnail(ShipConstruct* ship, const std::string& craftPath, bool expansion) {
    if (env.captureStockThumbnail) env.captureStockThumbnail(ship, craftPath, expansion);
}

// ShipConstruction.cs:846
void ShipConstruction::CaptureThumbnail(ShipConstruct* ship, const std::string& baseFolder, const std::string& thumbURL) {
    if (env.captureThumbnail) env.captureThumbnail(ship, baseFolder, thumbURL);
}

// ShipConstruction.cs:937
std::unique_ptr<ShipTemplate> ShipConstruction::LoadTemplate(const std::string& shipFilename) {
    ConfigNode::NodePtr configNode = ConfigNode::Load(shipFilename);
    if (!configNode) {
        log::Error("File '" + shipFilename + "' not found!");
        return nullptr;
    }
    auto shipTemplate = std::make_unique<ShipTemplate>();
    shipTemplate->LoadShip(shipFilename, configNode);
    return shipTemplate;
}

// ShipConstruction.cs:950
std::string ShipConstruction::SaveSubassembly(ShipConstruct& ship, const std::string& shipFilename) {
    const std::string text = AppRoot() + "saves/" + env.SaveFolder + "/Subassemblies/";
    std::error_code ec;
    fs::create_directories(dotnet::PathFromUtf8(text), ec);
    const std::string text2 = text + shipFilename + ".craft";
    ship.SaveShip()->Save(text2);
    return text2;
}

// ShipConstruction.cs:959
bool ShipConstruction::SubassemblyExists(const std::string& shipFilename) {
    const std::string text = AppRoot() + "saves/" + env.SaveFolder + "/Subassemblies/";
    std::error_code ec;
    fs::create_directories(dotnet::PathFromUtf8(text), ec);
    return fs::exists(dotnet::PathFromUtf8(text + shipFilename + ".craft"), ec);
}

// ShipConstruction.cs:966
std::unique_ptr<ShipConstruct> ShipConstruction::LoadSubassembly(const std::string& shipFilename) {
    ConfigNode::NodePtr configNode = ConfigNode::Load(shipFilename);
    if (!configNode) {
        log::Error("File '" + shipFilename + "' not found!");
        return nullptr;
    }
    auto shipConstruct = std::make_unique<ShipConstruct>();
    if (!shipConstruct->LoadShipWithParts(*configNode)) {
        log::Error("Ship file error!");
        return nullptr;
    }
    if (shipConstruct->steamPublishedFileId == 0 && env.steamIdFromSteamFolder)
        shipConstruct->steamPublishedFileId = env.steamIdFromSteamFolder(shipFilename);
    return shipConstruct;
}

// ShipConstruction.cs:1113
void ShipConstruction::CreateConstructFromTemplate(ShipTemplate& tmpl, const std::function<void(std::unique_ptr<ShipConstruct>)>& onComplete) {
    KSPUpgradePipeline::Process(
        tmpl.config, tmpl.shipName, SaveUpgradePipeline::LoadContext::Craft,
        [&tmpl, &onComplete](const ConfigNode::NodePtr& n) { OnPipelineFinished(n, tmpl, onComplete); },
        [&tmpl, &onComplete](KSPUpgradePipeline::UpgradeFailOption opt, const ConfigNode::NodePtr& n) { OnPipelineFailed(opt, n, tmpl, onComplete); });
}

// ShipConstruction.cs:1124
void ShipConstruction::OnPipelineFailed(KSPUpgradePipeline::UpgradeFailOption opt, const ConfigNode::NodePtr& n, ShipTemplate& tmpl,
                                        const std::function<void(std::unique_ptr<ShipConstruct>)>& onComplete) {
    if (opt != KSPUpgradePipeline::UpgradeFailOption::Cancel && opt == KSPUpgradePipeline::UpgradeFailOption::LoadAnyway)
        OnPipelineFinished(n, tmpl, onComplete);
}

// ShipConstruction.cs:1132 (the template's file is rewritten when the pipeline changed the node; the original is kept as .original)
void ShipConstruction::OnPipelineFinished(const ConfigNode::NodePtr& node, ShipTemplate& tmpl,
                                          const std::function<void(std::unique_ptr<ShipConstruct>)>& onComplete) {
    if (node != tmpl.config) {
        tmpl.config->Save(tmpl.filename + ".original");
        node->Save(tmpl.filename);
        tmpl.config = node;
    }
    auto shipConstruct = std::make_unique<ShipConstruct>();
    if (!shipConstruct->LoadShipWithParts(*tmpl.config)) {
        onComplete(nullptr);
        return;
    }
    for (Part* part : shipConstruct->liveParts) part->fields.craftID = part->instanceID;  // (uint)part.GetInstanceID()
    onComplete(std::move(shipConstruct));
}

// ShipConstruction.cs:1155 (FindObjectsOfType<Part>() = Part::allParts: every live Part is active and enabled)
void ShipConstruction::SanitizeCraftIDs(const std::vector<Part*>& parts, bool preserveIDsOnGivenParts) {
    std::vector<Part*> list(Part::allParts.begin(), Part::allParts.end());
    int num = static_cast<int>(list.size()) - 1;
    int count2 = static_cast<int>(parts.size());
    while (count2-- > 0) {
        Part* part = parts[static_cast<size_t>(count2)];
        for (int num2 = num; num2 >= 0; num2--) {
            Part* part2 = list[static_cast<size_t>(num2)];
            if (part != part2 && part->fields.craftID == part2->fields.craftID) {
                if (preserveIDsOnGivenParts) part2->fields.craftID = part2->instanceID;
                else part->fields.craftID = part->instanceID;
            }
        }
    }
}

// ---- part costs (ShipConstruction.cs:1197-1303) -----------------------------------------------------------------------------------------------

// ShipConstruction.cs:1197
float ShipConstruction::GetPartCosts(ProtoPartSnapshot& protoPart, const AvailablePart* aP, float& dryCost, float& fuelCost) {
    return GetPartCosts(protoPart, true, aP, dryCost, fuelCost);
}

// ShipConstruction.cs:1202 (aP.cost is read before the null test: a null aP is a NullReferenceException, as in the C#)
float ShipConstruction::GetPartCosts(ProtoPartSnapshot& protoPart, bool includeModuleCosts, const AvailablePart* aP, float& dryCost, float& fuelCost) {
    if (aP == nullptr) throw NullReferenceException();
    dryCost = includeModuleCosts ? f32store(fx(aP->cost) + fx(protoPart.moduleCosts)) : aP->cost;
    fuelCost = 0.0f;
    double num = dryCost;
    double num2 = 0.0;
    PartResourceLibrary* lib = PartResourceLibrary::Instance();
    for (const auto& snap : protoPart.resources) {
        const PartResourceDefinition* definition = lib != nullptr && snap->resourceName ? lib->GetDefinition(*snap->resourceName) : nullptr;
        if (definition != nullptr) {
            num -= static_cast<double>(definition->unitCost()) * snap->maxAmount;
            num2 += static_cast<double>(definition->unitCost()) * snap->amount;
        } else {
            log::Error("[ShipTemplate]: No Resource definition found for " + snap->resourceName.value_or(std::string()));
        }
    }
    dryCost = static_cast<float>(num);
    fuelCost = static_cast<float>(num2);
    return static_cast<float>(num + num2);
}

// ShipConstruction.cs:1238 (float locals: Mono rule; float.Parse = dotnet::ParseSingle)
float ShipConstruction::GetPartCostsAndMass(const ConfigNode& partNode, const AvailablePart* aP, float& dryCost, float& fuelCost, float& dryMass,
                                            float& fuelMass) {
    dryCost = 0.0f;
    fuelCost = 0.0f;
    dryMass = 0.0f;
    fuelMass = 0.0f;
    if (aP == nullptr) {
        const std::string* v = partNode.GetValuePtr("part");
        if (v == nullptr) throw NullReferenceException();
        aP = PartInfoByName(PartNameBeforeUnderscore(*v));
        if (aP == nullptr) {
            log::Error("[ShipConstruct]: No AvailablePart found for " + PartNameBeforeUnderscore(*v));
            return 0.0f;
        }
    }
    dryCost = aP->cost;
    if (aP->partPrefab != nullptr) dryMass = aP->partPrefab->fields.mass;
    if (partNode.HasValue("modCost")) dryCost = f32store(fx(dryCost) + fx(dotnet::ParseSingle(*partNode.GetValuePtr("modCost"))));
    if (partNode.HasValue("modMass")) dryMass = f32store(fx(dryMass) + fx(dotnet::ParseSingle(*partNode.GetValuePtr("modMass"))));
    PartResourceLibrary* lib = PartResourceLibrary::Instance();
    int count = partNode.nodes().Count();
    while (count-- > 0) {
        const ConfigNode& configNode = *partNode.nodes()[count];
        if (configNode.name != "RESOURCE") continue;
        const std::string* rn = configNode.GetValuePtr("name");
        if (rn == nullptr) throw ArgumentNullException("key");
        const PartResourceDefinition* definition = lib != nullptr ? lib->GetDefinition(*rn) : nullptr;
        if (definition != nullptr && configNode.HasValue("amount")) {
            const float num = dotnet::ParseSingle(*configNode.GetValuePtr("amount"));
            const std::string* maxv = configNode.GetValuePtr("maxAmount");
            if (maxv == nullptr) throw ArgumentNullException("s");
            const float num2 = dotnet::ParseSingle(*maxv);
            dryCost = f32store(fx(dryCost) - fx(definition->unitCost()) * fx(num2));
            fuelCost = f32store(fx(fuelCost) + fx(definition->unitCost()) * fx(num));
            fuelMass = f32store(fx(fuelMass) + fx(definition->density()) * fx(num));
        } else {
            log::Error("[ShipTemplate]: No Resource definition found for " + configNode.name);
        }
    }
    return f32store(fx(dryCost) + fx(fuelCost));
}

// ShipConstruction.cs:1292 (the cost is raised to the resources' value when the part costs less than its fuel)
void ShipConstruction::SanitizePartCosts(AvailablePart& aP, const ConfigNode& partNode) {
    float dryCost = aP.cost;
    float fuelCost = 0.0f;
    float dm = 0.0f, fm = 0.0f;
    GetPartCostsAndMass(partNode, &aP, dryCost, fuelCost, dm, fm);
    if (dryCost < 0.0f) {
        log::Warning("[ShipConstruct for " + aP.name + "]: part cost (" + dotnet::FormatSingleF(aP.cost, 1) + ") is less than the cost of its resources (" +
                     dotnet::FormatSingleF(fuelCost, 1) + ")");
        aP.cost = fuelCost;
    }
}

// ShipConstruction.cs:1304 (EditorLogic.fetch.ship.parts.Find)
Part* ShipConstruction::FindPartWithCraftID(uint32_t craftID) {
    if (env.editorShip == nullptr) throw NullReferenceException();
    for (Part* p : env.editorShip->liveParts)
        if (p->fields.craftID == craftID) return p;
    return nullptr;
}

// ShipConstruction.cs:1309
bool ShipConstruction::AllPartsFound(const ConfigNode* root, std::string* error) {
    if (root != nullptr && error != nullptr) {
        SaveEnvironment& se = SaveEnvironment::Current();
        auto doesExist = [&se](const std::string& n) { return se.partExists ? se.partExists(n) : false; };
        auto hasReplacement = [&se](const std::string& n) {
            for (const auto& r : se.partReplacements)
                if (r.first == n) return true;
            return false;
        };
        auto replacementName = [&se](const std::string& n) {
            for (const auto& r : se.partReplacements)
                if (r.first == n) return r.second;
            return std::string();
        };
        bool result = true;
        for (ConfigNode* node : root->GetNodes("PART")) {
            const std::string* v = node->GetValuePtr("part");
            if (v != nullptr) {
                const std::string name = PartNameBeforeUnderscore(*v);
                if (!doesExist(name) && !hasReplacement(name) && !doesExist(replacementName(name))) {
                    const std::string* ship = root->GetValuePtr("ship");
                    *error += Localizer::Format("#autoLOC_8004243", {ship != nullptr ? *ship : std::string(), name});
                    result = false;
                }
            }
        }
        return result;
    }
    return false;
}

// ShipConstruction.cs:1335
bool ShipConstruction::IsCompatible(const ConfigNode* root, std::string* error) {
    if (root != nullptr && error != nullptr) {
        const std::string* version = root->GetValuePtr("version");
        if (KSPUtil::CheckVersion(version != nullptr ? *version : std::string(), ShipConstruct::lastCompatibleMajor, ShipConstruct::lastCompatibleMinor,
                                  ShipConstruct::lastCompatibleRev) != VersionCompareResult::COMPATIBLE) {
            const std::string* ship = root->GetValuePtr("ship");
            *error += Localizer::Format("#autoLOC_8004244", {ship != nullptr ? *ship : std::string()});
            return false;
        }
        return true;
    }
    return false;
}

}  // namespace ksp
