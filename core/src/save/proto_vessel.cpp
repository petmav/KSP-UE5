// ProtoVessel and the part snapshots (data side); see include/ksp/save/proto_vessel.hpp.
#include "ksp/save/proto_vessel.hpp"

#include <algorithm>

#include "ksp/config/parse_extensions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/util/ksp_util.hpp"
#include "save_util.hpp"

namespace ksp {

using save_detail::AddNullableValue;
using save_detail::EnumName;
using save_detail::ParseBool;
using save_detail::ParseDouble;
using save_detail::ParseEnum;
using save_detail::ParseFloat;
using save_detail::ParseInt;
using save_detail::ParseUInt;
using GE = events::GameEvents;

namespace save_detail {
void AddNullableValue(ConfigNode& node, std::string_view name, const std::optional<std::string>& value) {
    if (!value)
        log::Error("Input is null for field '" + std::string(name) + "' in config node '" + node.name + "'");
    node.AddValue(name, value.value_or(""));
}
ConfigNode::NodePtr FirstChildShared(const ConfigNode& node, std::string_view name) {
    for (int i = 0; i < node.nodes().Count(); ++i)
        if (node.nodes()[i]->name == name) return node.nodes().Shared(i);
    return nullptr;
}
}  // namespace save_detail

// ================================================== ProtoTargetInfo =================================================

ProtoTargetInfo::ProtoTargetInfo(const ConfigNode& node) {
    targetType = Type::Null;
    Load(node);
}

// ProtoTargetInfo.cs:79
void ProtoTargetInfo::Load(const ConfigNode& node) {
    if (node.HasValue("type")) targetType = ParseEnum<Type>(*node.GetValuePtr("type"));
    if (node.HasValue("vesselID")) vesselId = Guid::Parse(*node.GetValuePtr("vesselID"));
    if (node.HasValue("partID")) partUID = ParseUInt(*node.GetValuePtr("partID"));
    if (node.HasValue("moduleIdx")) partModuleIndex = ParseInt(*node.GetValuePtr("moduleIdx"));
    if (node.HasValue("tgtId")) uniqueName = *node.GetValuePtr("tgtId");
}

// ProtoTargetInfo.cs:103
void ProtoTargetInfo::Save(ConfigNode& node) const {
    if (targetType == Type::Null) return;
    node.AddValue("type", EnumName(targetType));
    switch (targetType) {
    case Type::Vessel: node.AddValue("vesselID", vesselId.ToString()); break;
    case Type::PartModule:
        node.AddValue("vesselID", vesselId.ToString());
        node.AddValue("partID", partUID);
        node.AddValue("moduleIdx", static_cast<int32_t>(partModuleIndex));
        break;
    case Type::Part:
        node.AddValue("vesselID", vesselId.ToString());
        node.AddValue("partID", partUID);
        break;
    case Type::CelestialBody:
    case Type::Generic: AddNullableValue(node, "tgtId", uniqueName); break;
    default: break;
    }
}

// ================================================= ProtoWaypointInfo ================================================

ProtoWaypointInfo::ProtoWaypointInfo(const ConfigNode& node) {
    waypointType = Type::Null;
    Load(node);
}

// ProtoWaypointInfo.cs:49
void ProtoWaypointInfo::Load(const ConfigNode& node) {
    if (node.HasValue("type")) waypointType = ParseEnum<Type>(*node.GetValuePtr("type"));
    if (node.HasValue("navigationID")) navigationId = Guid::Parse(*node.GetValuePtr("navigationID"));
}

// ProtoWaypointInfo.cs:61
void ProtoWaypointInfo::Save(ConfigNode& node) const {
    if (waypointType == Type::Null) return;
    node.AddValue("type", EnumName(waypointType));
    node.AddValue("navigationID", navigationId.ToString());
}

// ================================================ AttachNodeSnapshot ================================================

// AttachNodeSnapshot.cs:33
AttachNodeSnapshot::AttachNodeSnapshot(std::string_view attachNodeString) {
    const std::vector<std::string> array = dotnet::Split(attachNodeString, ",", false);
    id = dotnet::Trim(array.at(0));
    partIdx = ParseInt(dotnet::Trim(array.at(1)));
    if (array.size() > 2) srfAttachMeshName = array[2];
    else srfAttachMeshName = "";
}

// AttachNodeSnapshot.cs:48
std::string AttachNodeSnapshot::Save() const {
    std::string text = id + ", " + dotnet::FormatInt64(partIdx);
    if (srfAttachMeshName && !srfAttachMeshName->empty()) text += "," + *srfAttachMeshName;
    return text;
}

// =================================================== VesselNaming ===================================================

// VesselNaming.cs:18 (the field initialiser leaves vesselType = Debris; TryGetEnum then defaults to Ship)
VesselNaming::VesselNaming(const ConfigNode& node) {
    vesselType = VesselType::Debris;
    Load(node);
}

// VesselNaming.cs:23
void VesselNaming::Load(const ConfigNode& node) {
    if (const std::string* v = node.GetValuePtr("name")) vesselName = *v;
    save_detail::TryGetEnum(node, "type", vesselType, VesselType::Ship);
    int32_t p = 0;
    if (node.TryGetValue("priority", p)) namingPriority = p;
}

// VesselNaming.cs:30
void VesselNaming::Save(ConfigNode& node) const {
    AddNullableValue(node, "name", vesselName);
    node.AddValue("type", EnumName(vesselType));
    node.AddValue("priority", static_cast<int32_t>(namingPriority));
}

// ============================================= ProtoPartResourceSnapshot ============================================

// ProtoPartResourceSnapshot.cs:30
ProtoPartResourceSnapshot::ProtoPartResourceSnapshot(const ConfigNode& node) {
    resourceValues_ = std::make_shared<ConfigNode>("RESOURCE");
    node.CopyTo(*resourceValues_);
    if (const std::string* t = node.GetValuePtr("name")) resourceName = *t;
    if (const std::string* t = node.GetValuePtr("amount")) amount = ParseDouble(*t);
    if (const std::string* t = node.GetValuePtr("maxAmount")) maxAmount = ParseDouble(*t);
    if (const std::string* t = node.GetValuePtr("flowState")) flowState = ParseBool(*t);
    // ProtoPartResourceSnapshot.cs:51 (WP-14: Vessel.GetUnloadedVesselMass reads definition.density)
    if (resourceName && !resourceName->empty()) {
        const PartResourceLibrary* lib = PartResourceLibrary::Instance();
        definition = lib ? lib->GetDefinition(*resourceName) : nullptr;
    }
}

// ProtoPartResourceSnapshot.cs:55
void ProtoPartResourceSnapshot::Save(ConfigNode& node) const {
    resourceValues_->CopyTo(node);
    node.SetValue("flowState", flowState, true);
    node.SetValue("amount", amount, true);
    node.SetValue("maxAmount", maxAmount, true);
}

// ProtoPartResourceSnapshot.cs:63
void ProtoPartResourceSnapshot::UpdateConfigNodeAmounts() {
    resourceValues_->SetValue("amount", amount, true);
    resourceValues_->SetValue("maxAmount", maxAmount, true);
}

// ============================================== ProtoPartModuleSnapshot =============================================

// ProtoPartModuleSnapshot.cs:34
ProtoPartModuleSnapshot::ProtoPartModuleSnapshot(const ConfigNode& node) {
    GE::onProtoPartModuleSnapshotLoad.Fire(
        GE::FromToAction<ProtoPartModuleSnapshot*, ConfigNode*>(this, const_cast<ConfigNode*>(&node)));
    moduleValues = std::make_shared<ConfigNode>("MODULE");
    node.CopyTo(*moduleValues);
    for (const ConfigNode::Value& v : node.values())
        if (v.name == "name") moduleName = v.value;
}

// ProtoPartModuleSnapshot.cs:48 (EnsureModuleValuesIsInitialized needs a live module: nothing without moduleRef)
void ProtoPartModuleSnapshot::Save(ConfigNode& node) {
    EnsureModuleValuesIsInitialized();
    moduleValues->CopyTo(node);
    GE::onProtoPartModuleSnapshotSave.Fire(GE::FromToAction<ProtoPartModuleSnapshot*, ConfigNode*>(this, &node));
}

// ProtoPartModuleSnapshot.cs:156
void ProtoPartModuleSnapshot::RemovePartModuleAdjuster(const Guid& removeAdjusterID) {
    if (!SaveEnvironment::Current().IsExpansionInstalled("MakingHistory")) return;
    ConfigNode::NodePtr node = std::make_shared<ConfigNode>();
    if (auto n = save_detail::FirstChildShared(*moduleValues, "ADJUSTERS")) node = n;
    const std::vector<ConfigNode*> nodes = node->GetNodes();
    ConfigNode::NodePtr node2 = std::make_shared<ConfigNode>("ADJUSTERSTOREMOVE");
    if (auto n = save_detail::FirstChildShared(*moduleValues, "ADJUSTERSTOREMOVE")) node2 = n;
    for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
        if (nodes[static_cast<size_t>(i)]->HasValue("adjusterID") &&
            Guid::Parse(*nodes[static_cast<size_t>(i)]->GetValuePtr("adjusterID")) == removeAdjusterID)
            node2->AddNode(node->nodes().Shared(i));
    moduleValues->SetNode("ADJUSTERS", node2, true);
    GE::onProtoPartModuleAdjusterRemoved.Fire(this);
}

// ProtoPartModuleSnapshot.cs:197
void ProtoPartModuleSnapshot::ProtoPartModuleRepair() {
    if (!SaveEnvironment::Current().IsExpansionInstalled("MakingHistory")) return;
    ConfigNode::NodePtr node = std::make_shared<ConfigNode>();
    if (auto n = save_detail::FirstChildShared(*moduleValues, "ADJUSTERS")) node = n;
    ConfigNode::NodePtr node2 = std::make_shared<ConfigNode>("ADJUSTERSTOREMOVE");
    if (auto n = save_detail::FirstChildShared(*moduleValues, "ADJUSTERSTOREMOVE")) node2 = n;
    for (int i = 0; i < node->nodes().Count(); ++i) node2->AddNode(node->nodes().Shared(i));
    moduleValues->SetNode("ADJUSTERSTOREMOVE", node2, true);
    GE::onProtoPartModuleRepaired.Fire(this);
}

// ================================================= ProtoPartSnapshot ================================================

// ProtoPartSnapshot.cs:248
ProtoPartSnapshot::ProtoPartSnapshot(const ConfigNode& node, ProtoVessel* protoVessel, Game* st) {
    GE::onProtoPartSnapshotLoad.Fire(GE::FromToAction<ProtoPartSnapshot*, ConfigNode*>(this, const_cast<ConfigNode*>(&node)));
    pVesselRef = protoVessel;
    customPartData = "";
    flagURL = "";
    refTransformName = "";
    moduleCosts = 0.0f;
    moduleMass = 0.0f;
    moduleVariantName = "";
    moduleCargoStackableQuantity = 1;
    SaveEnvironment& env = SaveEnvironment::Current();
    for (int i = 0; i < node.values().Count(); ++i) {
        const ConfigNode::Value& value = node.values()[i];
        const std::string& n = value.name;
        const std::string& v = value.value;
        if (n == "launchID") launchID = ParseUInt(v);
        else if (n == "srfN") srfAttachNode = AttachNodeSnapshot(v);
        else if (n == "moduleCargoStackableQuantity") moduleCargoStackableQuantity = ParseInt(v);
        else if (n == "rotation") rotation = KSPUtil::ParseQuaternion(v);
        else if (n == "tempExt") skinTemperature = ParseDouble(v);
        else if (n == "resPri") resourcePriorityOffset = ParseInt(v);
        else if (n == "sidx") inStageIndex = ParseInt(v);
        else if (n == "attm") attachMode = ParseInt(v);
        else if (n == "sepI") separationIndex = ParseInt(v);
        else if (n == "symMethod") symMethod = ParseEnum<SymmetryMethod>(v);
        else if (n == "mirror") mirror = KSPUtil::ParseVector3d(v).ToVector3f();  // implicit Vector3d -> Vector3
        else if (n == "rigidAttachment") {
            bool r = false;
            if (dotnet::TryParseBool(v, r)) rigidAttachment = r;
        } else if (n == "attN") attachNodes.emplace_back(v);
        else if (n == "tempExtUnexp") skinUnexposedTemperature = ParseDouble(v);
        else if (n == "sameVesselCollision") sameVesselCollision = ParseBool(v);
        else if (n == "uid") flightID = ParseUInt(v);
        else if (n == "modCost") moduleCosts = ParseFloat(v);
        else if (n == "dstg") defaultInverseStage = ParseInt(v);
        else if (n == "state") state = ParseInt(v);
        else if (n == "persistentId") {
            uint32_t result3 = 0;
            if (!dotnet::TryParseUInt32(v, result3) &&
                !dotnet::TryParseUInt32(dotnet::Trim(dotnet::Split(v, ",", false)[0]), result3))
                result3 = 0;
            persistentId = result3;
        } else if (n == "cData") customPartData = v;
        else if (n == "modMass") moduleMass = ParseFloat(v);
        else if (n == "sqor") seqOverride = ParseInt(v);
        else if (n == "position") position = KSPUtil::ParseVector3d(v);
        else if (n == "crew") {
            int32_t result2 = -1;
            if (dotnet::TryParseInt32(v, result2)) protoCrewIndicesBackup.push_back(result2);
            else protoCrewNames.push_back(v);
        } else if (n == "name") {
            partName = v;
            if (env.getPartInfoByName) {  // PartLoader.getPartInfoByName (WP-13)
                partInfo = env.getPartInfoByName(v);
                partInfoFound = partInfo != nullptr;
            } else {
                partInfoFound = env.partExists ? env.partExists(v) : false;
            }
        } else if (n == "autostrutMode") autostrutMode = ParseEnum<Part_AutoStrutMode>(v);
        else if (n == "attached") attached = ParseBool(v);
        else if (n == "shielded") {
            bool r = false;
            dotnet::TryParseBool(v, r);
            shielded = r;
        } else if (n == "flag") flagURL = v;
        else if (n == "sym") symLinkIdxs.push_back(ParseInt(v));
        else if (n == "istg") inverseStageIndex = ParseInt(v);
        else if (n == "mid") missionID = ParseUInt(v);
        else if (n == "temp") temperature = ParseDouble(v);
        else if (n == "staticPressureAtm") staticPressureAtm = ParseDouble(v);
        else if (n == "mass") mass = ParseFloat(v);
        else if (n == "expt") explosionPotential = ParseFloat(v);
        else if (n == "parent") parentIdx = ParseInt(v);
        else if (n == "cid") craftID = ParseUInt(v);
        else if (n == "partRendererBoundsIgnore") {
            for (const std::string& s : dotnet::Split(v, ",", false)) partRendererBoundsIgnore.push_back(dotnet::Trim(s));
        } else if (n == "moduleVariantName") moduleVariantName = v;
        else if (n == "PreFailState") {
            int32_t result = 0;
            if (!dotnet::TryParseInt32(v, result) && !dotnet::TryParseInt32(dotnet::Trim(dotnet::Split(v, ",", false)[0]), result))
                result = 0;
            PreFailState = result;
        } else if (n == "rTrf") refTransformName = v;
        else if (n == "connected") {
        } else {
            if (partStateValues.ContainsKey(n))
                throw ArgumentException("An item with the same key has already been added. Key: " + n);
            partStateValues.Add(n, KSPParseable(v));
        }
    }
    persistentId = env.persistentIds.CheckProtoPartSnapShotpersistentId(
        persistentId, this, false, true, pVesselRef != nullptr ? pVesselRef->persistentId : 0u);
    for (int k = 0; k < node.nodes().Count(); ++k) {
        const ConfigNode& child = *node.nodes()[k];
        const std::string& n = child.name;
        if (n == "MODULE") modules.push_back(std::make_unique<ProtoPartModuleSnapshot>(child));
        else if (n == "ACTIONS") {
            partActions = std::make_shared<ConfigNode>();
            child.CopyTo(*partActions);
        } else if (n == "PARTDATA") {
            partData = std::make_shared<ConfigNode>();
            child.CopyTo(*partData);
        } else if (n == "RESOURCE") resources.push_back(std::make_unique<ProtoPartResourceSnapshot>(child));
        else if (n == "VESSELNAMING") vesselNaming = std::make_unique<VesselNaming>(child);
        else if (n == "EVENTS") {
            partEvents = std::make_shared<ConfigNode>();
            child.CopyTo(*partEvents);
        } else if (n == "EFFECTS") {
            partEffects = std::make_shared<ConfigNode>();
            child.CopyTo(*partEffects);
        }
    }
    if (st != nullptr) {
        if (!st->CrewRoster) throw NullReferenceException("ProtoPartSnapshot: Game.CrewRoster is null");
        for (int idx : protoCrewIndicesBackup) {
            auto pcm = (*st->CrewRoster)[idx];
            if (!pcm) throw NullReferenceException("ProtoPartSnapshot: crew index " + dotnet::FormatInt64(idx) + " not in roster");
            protoCrewNames.push_back(pcm->name().value_or(""));
        }
        for (const std::string& crewName : protoCrewNames) {
            PCM pcm = (*st->CrewRoster)[crewName];
            protoModuleCrew.push_back(pcm);
            if (protoVessel == nullptr) throw NullReferenceException("ProtoPartSnapshot: protoVessel is null");
            protoVessel->AddCrew(pcm);
        }
    }
}

// ProtoPartSnapshot.cs:525
void ProtoPartSnapshot::Save(ConfigNode& node) {
    const SaveEnvironment& env = SaveEnvironment::Current();
    AddNullableValue(node, "name", partName);
    node.AddValue("cid", craftID);
    node.AddValue("uid", flightID);
    node.AddValue("mid", missionID);
    node.AddValue("persistentId", persistentId);
    node.AddValue("launchID", launchID);
    node.AddValue("parent", static_cast<int32_t>(parentIdx));
    node.AddValue("position", KSPUtil::WriteVector(position));
    node.AddValue("rotation", KSPUtil::WriteQuaternion(rotation));
    node.AddValue("mirror", KSPUtil::WriteVector(mirror));
    node.AddValue("symMethod", EnumName(symMethod));
    node.AddValue("istg", static_cast<int32_t>(inverseStageIndex));
    node.AddValue("resPri", static_cast<int32_t>(resourcePriorityOffset));
    node.AddValue("dstg", static_cast<int32_t>(defaultInverseStage));
    node.AddValue("sqor", static_cast<int32_t>(seqOverride));
    node.AddValue("sepI", static_cast<int32_t>(separationIndex));
    node.AddValue("sidx", static_cast<int32_t>(inStageIndex));
    node.AddValue("attm", static_cast<int32_t>(attachMode));
    node.AddValue("sameVesselCollision", sameVesselCollision);
    if (!customPartData.empty()) node.AddValue("cData", customPartData);
    for (int s : symLinkIdxs) node.AddValue("sym", static_cast<int32_t>(s));
    if (srfAttachNode) node.AddValue("srfN", srfAttachNode->Save());
    else node.AddValue("srfN", "None, -1");
    for (const AttachNodeSnapshot& a : attachNodes) node.AddValue("attN", a.Save());
    node.AddValue("mass", mass);
    node.AddValue("shielded", shielded);
    node.AddValue("temp", temperature);
    node.AddValue("tempExt", skinTemperature);
    node.AddValue("tempExtUnexp", skinUnexposedTemperature);
    node.AddValue("staticPressureAtm", staticPressureAtm);
    node.AddValue("expt", explosionPotential);
    node.AddValue("state", static_cast<int32_t>(state));
    if (env.IsExpansionInstalled("MakingHistory")) node.AddValue("PreFailState", static_cast<int32_t>(PreFailState));
    node.AddValue("attached", attached);
    node.AddValue("autostrutMode", EnumName(autostrutMode));
    node.AddValue("rigidAttachment", rigidAttachment);
    node.AddValue("flag", flagURL);
    node.AddValue("rTrf", refTransformName);
    node.AddValue("modCost", moduleCosts);
    node.AddValue("modMass", moduleMass);
    node.AddValue("moduleVariantName", moduleVariantName);
    node.AddValue("moduleCargoStackableQuantity", static_cast<int32_t>(moduleCargoStackableQuantity));
    if (!partRendererBoundsIgnore.empty()) {
        std::string joined;
        for (size_t i = 0; i < partRendererBoundsIgnore.size(); ++i) {
            if (i > 0) joined += ",";
            joined += partRendererBoundsIgnore[i];
        }
        node.AddValue("partRendererBoundsIgnore", joined);
    }
    for (const auto& [key, value] : partStateValues) node.AddValue(key, value.Save());
    for (const std::string& c : protoCrewNames) node.AddValue("crew", c);
    if (partEvents) partEvents->CopyTo(*node.AddNode("EVENTS"));
    if (partActions) partActions->CopyTo(*node.AddNode("ACTIONS"));
    if (partEffects) partEffects->CopyTo(*node.AddNode("EFFECTS"));
    if (partData) partData->CopyTo(*node.AddNode("PARTDATA"));
    for (auto& m : modules) m->Save(*node.AddNode("MODULE"));
    for (auto& r : resources) r->Save(*node.AddNode("RESOURCE"));
    if (vesselNaming) vesselNaming->Save(*node.AddNode("VESSELNAMING"));
    GE::onProtoPartSnapshotSave.Fire(GE::FromToAction<ProtoPartSnapshot*, ConfigNode*>(this, &node));
}

// ProtoPartSnapshot.cs:1131
bool ProtoPartSnapshot::HasCrew(const std::string& name) const {
    for (const PCM& c : protoModuleCrew)
        if (c && c->name() == name) return true;
    return false;
}
// ProtoPartSnapshot.cs:1151
ProtoPartSnapshot::PCM ProtoPartSnapshot::GetCrew(const std::string& name) const {
    for (const PCM& c : protoModuleCrew)
        if (c && c->name() == name) return c;
    return nullptr;
}
// ProtoPartSnapshot.cs:1171
ProtoPartSnapshot::PCM ProtoPartSnapshot::GetCrew(int index) const { return protoModuleCrew.at(static_cast<size_t>(index)); }
// ProtoPartSnapshot.cs:1176
int ProtoPartSnapshot::GetCrewIndex(const std::string& name) const {
    for (size_t i = 0; i < protoModuleCrew.size(); ++i)
        if (protoModuleCrew[i] && protoModuleCrew[i]->name() == name) return static_cast<int>(i);
    return -1;
}
// ProtoPartSnapshot.cs:1196
void ProtoPartSnapshot::RemoveCrew(const std::string& name) {
    for (size_t i = 0; i < protoModuleCrew.size(); ++i)
        if (protoModuleCrew[i] && protoModuleCrew[i]->name() == name) {
            protoModuleCrew.erase(protoModuleCrew.begin() + static_cast<std::ptrdiff_t>(i));
            protoCrewNames.erase(protoCrewNames.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
}
// ProtoPartSnapshot.cs:1217
void ProtoPartSnapshot::RemoveCrew(int index) {
    protoModuleCrew.erase(protoModuleCrew.begin() + index);
    protoCrewNames.erase(protoCrewNames.begin() + index);
}
// ProtoPartSnapshot.cs:1223
void ProtoPartSnapshot::RemoveCrew(const PCM& pcm) {
    for (size_t i = 0; i < protoModuleCrew.size(); ++i)
        if (protoModuleCrew[i] == pcm) {
            protoModuleCrew.erase(protoModuleCrew.begin() + static_cast<std::ptrdiff_t>(i));
            protoCrewNames.erase(protoCrewNames.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
}
// ProtoPartSnapshot.cs:1244
ProtoPartModuleSnapshot* ProtoPartSnapshot::FindModule(const std::string& moduleName) const {
    for (const auto& m : modules)
        if (m->moduleName == moduleName) return m.get();
    return nullptr;
}

// ==================================================== ProtoVessel ===================================================

// ProtoVessel.cs:254
ProtoVessel::ProtoVessel(const ConfigNode& node, Game* st) {
    discoveryInfo = std::make_shared<ConfigNode>();
    flightPlan = std::make_shared<ConfigNode>();
    ctrlState = std::make_shared<ConfigNode>();
    vesselModules = nullptr;
    GE::onProtoVesselLoad.Fire(GE::FromToAction<ProtoVessel*, ConfigNode*>(this, const_cast<ConfigNode*>(&node)));
    bool haveOverrideDefault = false, haveOverrideAction = false, haveOverrideAxis = false, haveOverrideNames = false;
    for (int i = 0; i < node.values().Count(); ++i) {
        const ConfigNode::Value& value = node.values()[i];
        const std::string& n = value.name;
        const std::string& v = value.value;
        if (n == "cln") autoClean = ParseBool(v);
        else if (n == "ctrl") wasControllable = ParseBool(v);
        else if (n == "PQSMin") PQSminLevel = ParseInt(v);
        else if (n == "root") rootIndex = ParseInt(v);
        else if (n == "alt") altitude = ParseDouble(v);
        else if (n == "PQSMax") PQSmaxLevel = ParseInt(v);
        else if (n == "launchedFrom") launchedFrom = v;
        else if (n == "vesselSpawning") vesselSpawning = ParseBool(v);
        else if (n == "lon") longitude = ParseDouble(v);
        else if (n == "nrm") normal = KSPUtil::ParseVector3(v);
        else if (n == "rot") rotation = KSPUtil::ParseQuaternion(v);
        else if (n == "ref") refTransform = ParseUInt(v);
        else if (n == "type") vesselType = ParseEnum<VesselType>(v);
        else if (n == "lat") latitude = ParseDouble(v);
        else if (n == "lct") launchTime = ParseDouble(v);
        else if (n == "GroupOverride") GroupOverride = ParseInt(v);
        else if (n == "pos") position = KSPUtil::ParseVector3d(v);
        else if (n == "persistentId") {
            uint32_t r = 0;
            dotnet::TryParseUInt32(v, r);
            persistentId = r;
        } else if (n == "OverrideGroupNames") {
            OverrideGroupNames.clear();
            for (const std::string& s : ParseExtensions::ParseArray(v, ",", false)) OverrideGroupNames.emplace_back(s);
            save_detail::ArrayMinSize(NumOverrideGroups, OverrideGroupNames);
            haveOverrideNames = true;
        } else if (n == "pid") vesselID = Guid::Parse(v);
        else if (n == "clnRsn") autoCleanReason = v;
        else if (n == "CoM") CoM = KSPUtil::ParseVector3(v);
        else if (n == "skipGroundPositioningForDroppedPart") skipGroundPositioningForDroppedPart = ParseBool(v);
        else if (n == "OverrideAxisControl") {
            save_detail::TryParseEnumIntArray(v, OverrideAxisControl);
            save_detail::ArrayMinSize(NumOverrideGroups, OverrideAxisControl);
            haveOverrideAxis = true;
        } else if (n == "distanceTraveled") distanceTraveled = ParseDouble(v);
        else if (n == "landedAt") landedAt = v;
        else if (n == "name") vesselName = v;
        else if (n == "landed") landed = ParseBool(v);
        else if (n == "skipGroundPositioning") skipGroundPositioning = ParseBool(v);
        else if (n == "stg") stage = ParseInt(v);
        else if (n == "altDispState")
            save_detail::TryGetEnum(node, "altDispState", altimeterDisplayState, AltimeterDisplayState::DEFAULT);
        else if (n == "sit") situation = ParseEnum<VesselSituations>(v);
        else if (n == "splashed") splashed = ParseBool(v);
        else if (n == "displaylandedAt") displaylandedAt = v;
        else if (n == "met") missionTime = ParseDouble(v);
        else if (n == "lastUT") lastUT = ParseDouble(v);
        else if (n == "OverrideActionControl") {
            save_detail::TryParseEnumIntArray(v, OverrideActionControl);
            save_detail::ArrayMinSize(NumOverrideGroups, OverrideActionControl);
            haveOverrideAction = true;
        } else if (n == "hgt") height = ParseFloat(v);
        else if (n == "OverrideDefault") {
            ParseExtensions::TryParseBoolArray(v, OverrideDefault);
            save_detail::ArrayMinSize(NumOverrideGroups, OverrideDefault);
            haveOverrideDefault = true;
        } else if (n == "prst") persistent = ParseBool(v);
        else {
            if (vesselStateValues.ContainsKey(n))
                throw ArgumentException("An item with the same key has already been added. Key: " + n);
            vesselStateValues.Add(n, KSPParseable(v));
        }
    }
    if (!haveOverrideDefault) OverrideDefault.assign(static_cast<size_t>(NumOverrideGroups), false);
    if (!haveOverrideAction) OverrideActionControl.assign(static_cast<size_t>(NumOverrideGroups), KSPActionGroup::None);
    if (!haveOverrideAxis) OverrideAxisControl.assign(static_cast<size_t>(NumOverrideGroups), KSPAxisGroup::None);
    if (!haveOverrideNames) OverrideGroupNames.assign(static_cast<size_t>(NumOverrideGroups), std::nullopt);
    persistentId = SaveEnvironment::Current().persistentIds.CheckVesselpersistentId(persistentId, nullptr, false, true);
    for (int j = 0; j < node.nodes().Count(); ++j) {
        const ConfigNode::NodePtr& child = node.nodes().Shared(j);
        const std::string& n = child->name;
        if (n == "PART") protoPartSnapshots.push_back(std::make_unique<ProtoPartSnapshot>(*child, this, st));
        else if (n == "WAYPOINT") waypointInfo = ProtoWaypointInfo(*child);
        else if (n == "VESSELMODULES") vesselModules = child;
        else if (n == "DISCOVERY") discoveryInfo = child;
        else if (n == "CTRLSTATE") ctrlState = child;
        else if (n == "FLIGHTPLAN") flightPlan = child;
        else if (n == "TARGET") targetInfo = ProtoTargetInfo(*child);
        else if (n == "ACTIONGROUPS") actionGroups = child;
        else if (n == "ORBIT") orbitSnapShot.emplace(*child);
    }
    RebuildCrewCounts();
}

// ProtoVessel.cs:453
void ProtoVessel::verifyCrew() {
    int count = static_cast<int>(crew_.size());
    while (count-- > 0)
        if (!crew_[static_cast<size_t>(count)]) crew_.erase(crew_.begin() + count);
}

const std::vector<ProtoVessel::PCM>& ProtoVessel::GetVesselCrew() {
    verifyCrew();
    return crew_;
}

// ProtoVessel.cs:471 (partPrefab.CrewCapacity through SaveEnvironment::partCrewCapacity)
void ProtoVessel::RebuildCrewCounts() {
    crewableParts = 0;
    crewedParts = 0;
    const SaveEnvironment& env = SaveEnvironment::Current();
    int count = static_cast<int>(protoPartSnapshots.size());
    while (count-- > 0) {
        const ProtoPartSnapshot& pps = *protoPartSnapshots[static_cast<size_t>(count)];
        if (!pps.protoModuleCrew.empty()) {
            ++crewableParts;
            ++crewedParts;
        } else if (pps.partInfoFound && env.partCrewCapacity && pps.partName &&
                   env.partCrewCapacity(*pps.partName) > 0) {
            ++crewableParts;
        }
    }
}

// ProtoVessel.cs:496
void ProtoVessel::AddCrew(const PCM& pcm) {
    crew_.push_back(pcm);
    RebuildCrewCounts();
}

// ProtoVessel.cs:502
bool ProtoVessel::RemoveCrew(const PCM& pcm) {
    RebuildCrewCounts();
    auto it = std::find(crew_.begin(), crew_.end(), pcm);
    if (it == crew_.end()) return false;
    crew_.erase(it);
    return true;
}

// ProtoVessel.cs:508
std::string ProtoVessel::GetDisplayName() const { return KSP::Localization::Localizer::Format(vesselName); }

// ProtoVessel.cs:513
void ProtoVessel::Save(ConfigNode& node) {
    node.AddValue("pid", vesselID.ToString('N'));
    node.AddValue("persistentId", persistentId);
    node.AddValue("name", vesselName);
    node.AddValue("type", EnumName(vesselType));
    node.AddValue("sit", EnumName(situation));
    node.AddValue("landed", landed);
    node.AddValue("skipGroundPositioning", skipGroundPositioning);
    node.AddValue("skipGroundPositioningForDroppedPart", skipGroundPositioningForDroppedPart);
    node.AddValue("vesselSpawning", vesselSpawning);
    node.AddValue("launchedFrom", launchedFrom);
    node.AddValue("landedAt", landedAt);
    node.AddValue("displaylandedAt", displaylandedAt);
    node.AddValue("splashed", splashed);
    node.AddValue("met", missionTime);
    node.AddValue("lct", launchTime);
    node.AddValue("lastUT", lastUT);
    node.AddValue("distanceTraveled", distanceTraveled);
    if (autoClean) {
        node.AddValue("cln", true);
        if (!autoCleanReason.empty()) node.AddValue("clnRsn", autoCleanReason);
    }
    node.AddValue("root", static_cast<int32_t>(rootIndex));
    node.AddValue("lat", latitude);
    node.AddValue("lon", longitude);
    node.AddValue("alt", altitude);
    node.AddValue("hgt", height);
    node.AddValue("nrm", KSPUtil::WriteVector(normal));
    node.AddValue("rot", KSPUtil::WriteQuaternion(rotation));
    node.AddValue("CoM", KSPUtil::WriteVector(CoM));
    node.AddValue("stg", static_cast<int32_t>(stage));
    node.AddValue("prst", persistent);
    node.AddValue("ref", refTransform);
    node.AddValue("ctrl", wasControllable);
    node.AddValue("PQSMin", static_cast<int32_t>(PQSminLevel));
    node.AddValue("PQSMax", static_cast<int32_t>(PQSmaxLevel));
    node.AddValue("GroupOverride", static_cast<int32_t>(GroupOverride));
    node.AddValue("OverrideDefault", save_detail::WriteBoolArray(OverrideDefault));
    node.AddValue("OverrideActionControl", save_detail::WriteEnumIntArray(OverrideActionControl));
    node.AddValue("OverrideAxisControl", save_detail::WriteEnumIntArray(OverrideAxisControl));
    node.AddValue("OverrideGroupNames", save_detail::WriteStringArray(OverrideGroupNames));
    node.AddValue("altDispState", EnumName(altimeterDisplayState));
    for (const auto& [key, value] : vesselStateValues) node.AddValue(key, value.value);
    if (!orbitSnapShot) throw NullReferenceException("ProtoVessel.Save: orbitSnapShot is null (no ORBIT node)");
    orbitSnapShot->Save(*node.AddNode("ORBIT"));
    for (auto& p : protoPartSnapshots) p->Save(*node.AddNode("PART"));
    ConfigNode* node2 = node.AddNode("ACTIONGROUPS");
    if (!actionGroups) throw NullReferenceException("ProtoVessel.Save: actionGroups is null (no ACTIONGROUPS node)");
    actionGroups->CopyTo(*node2);
    discoveryInfo->CopyTo(*node.AddNode("DISCOVERY"));
    flightPlan->CopyTo(*node.AddNode("FLIGHTPLAN"));
    auto target = std::make_shared<ConfigNode>("TARGET");
    targetInfo.Save(*target);
    if (target->HasData()) node.AddNode(target);
    if (!(waypointInfo.navigationId == Guid::Empty())) {
        auto node5 = std::make_shared<ConfigNode>("WAYPOINT");
        waypointInfo.Save(*node5);
        node.AddNode(node5);
    }
    ctrlState->CopyTo(*node.AddNode("CTRLSTATE"));
    SaveVesselModules();
    vesselModules->CopyTo(*node.AddNode("VESSELMODULES"));
    GE::onProtoVesselSave.Fire(GE::FromToAction<ProtoVessel*, ConfigNode*>(this, &node));
}

// ProtoVessel.cs:597
void ProtoVessel::SaveVesselModules() {
    if (!vesselModules) vesselModules = std::make_shared<ConfigNode>("VESSELMODULES");
    SaveLiveVesselModules();  // WP-14: the vesselRef != null branch (vessel/vessel_proto.cpp)
}

// ProtoVessel.cs:1061
std::vector<ProtoPartSnapshot*> ProtoVessel::GetAllProtoPartsIncludingCargo() {
    std::vector<ProtoPartSnapshot*> list;
    const SaveEnvironment& env = SaveEnvironment::Current();
    for (auto& ppsPtr : protoPartSnapshots) {
        ProtoPartSnapshot* pps = ppsPtr.get();
        if (!pps->partInfoFound || !pps->partName)
            throw NullReferenceException("ProtoVessel.GetAllProtoPartsIncludingCargo: partInfo is null");
        if (!(env.partExists && env.partExists(*pps->partName)) || *pps->partName == "kerbalEVA") continue;
        ConfigNode* configNode = nullptr;
        for (const auto& m : pps->modules)
            if (m->moduleName == "ModuleInventoryPart") {
                configNode = m->moduleValues.get();
                break;
            }
        list.push_back(pps);
        if (configNode == nullptr) continue;
        ConfigNode* stored = configNode->GetNode("STOREDPARTS");
        if (stored == nullptr) continue;
        for (ConfigNode* item : stored->GetNodes()) {
            int32_t quantity = 1;
            item->TryGetValue("quantity", quantity);
            ConfigNode* partNode = item->GetNode("PART");
            if (partNode == nullptr) continue;
            cargoSnapshots_.push_back(std::make_unique<ProtoPartSnapshot>(*partNode, this, nullptr));
            for (int l = 0; l < quantity; ++l) list.push_back(cargoSnapshots_.back().get());
        }
    }
    for (ProtoPartSnapshot* p : GetAllProtoPartsFromCrew()) list.push_back(p);
    return list;
}

// ProtoVessel.cs:1121
std::vector<ProtoPartSnapshot*> ProtoVessel::GetAllProtoPartsFromCrew() {
    std::vector<ProtoPartSnapshot*> list;
    for (const PCM& c : crew_) {
        if (!c) throw NullReferenceException("ProtoVessel.GetAllProtoPartsFromCrew: null crew");
        ConfigNode::NodePtr inventory = c->InventoryNode();
        if (!inventory) continue;
        ConfigNode* stored = inventory->GetNode("STOREDPARTS");
        if (stored == nullptr) continue;
        for (ConfigNode* item : stored->GetNodes()) {
            int32_t quantity = 1;
            item->TryGetValue("quantity", quantity);
            ConfigNode* partNode = item->GetNode("PART");
            if (partNode == nullptr) continue;
            cargoSnapshots_.push_back(std::make_unique<ProtoPartSnapshot>(*partNode, this, nullptr));
            for (int k = 0; k < quantity; ++k) list.push_back(cargoSnapshots_.back().get());
        }
    }
    return list;
}

// ProtoVessel.cs:968
ConfigNode::NodePtr ProtoVessel::CreateOrbitNode(const Orbit& orbit, const std::vector<CelestialBody*>& bodies) {
    auto node = std::make_shared<ConfigNode>("ORBIT");
    node->AddValue("SMA", orbit.semiMajorAxis);
    node->AddValue("ECC", orbit.eccentricity);
    node->AddValue("INC", orbit.inclination);
    node->AddValue("LPE", orbit.argumentOfPeriapsis);
    node->AddValue("LAN", orbit.LAN);
    node->AddValue("MNA", orbit.meanAnomalyAtEpoch);
    node->AddValue("EPH", orbit.epoch);
    const auto it = std::find(bodies.begin(), bodies.end(), orbit.referenceBody);
    node->AddValue("REF", static_cast<int32_t>(it == bodies.end() ? -1 : it - bodies.begin()));
    return node;
}

}  // namespace ksp
