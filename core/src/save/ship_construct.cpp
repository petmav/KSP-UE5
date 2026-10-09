// ShipConstruct .craft load/save (data side); see include/ksp/save/ship_construct.hpp.
#include "ksp/save/ship_construct.hpp"

#include <cmath>
#include <limits>

#include "ksp/config/kspparseable.hpp"
#include "ksp/config/parse_extensions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/util/ksp_util.hpp"
#include "save_util.hpp"

namespace ksp {

using save_detail::EnumName;
using save_detail::ParseBool;
using save_detail::ParseEnum;
using save_detail::ParseInt;
using save_detail::ParseUInt;
using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace {
std::string Loc(const char* tag) { return Localizer::Instance() ? Localizer::Format(tag) : std::string(tag); }
bool IsPosInf(float x) { return x == std::numeric_limits<float>::infinity(); }
}  // namespace

// ShipConstruct.cs:76
ShipConstruct::ShipConstruct() {
    RegisterResourceHost();
    shipSize = Vector3f{0.0f, 0.0f, 0.0f};
    persistentId = SaveEnvironment::Current().persistentIds.GetUniquepersistentId();
    rotation = Quaternionf::identity();
    OverrideDefault.assign(4, false);
    OverrideActionControl.assign(4, KSPActionGroup::None);
    OverrideAxisControl.assign(4, KSPAxisGroup::None);
    OverrideGroupNames.assign(4, std::nullopt);
}

// KSPUtil.cs:2076
int ShipConstruct::GetPartByCraftID(uint32_t id) const {
    if (id == 0) return -1;
    int count = static_cast<int>(parts.size());
    while (count-- > 0)
        if (parts[static_cast<size_t>(count)].craftID == id) return count;
    return -1;
}

bool ShipConstruct::LoadShip(const ConfigNode& root, uint32_t persistentID) {
    std::string errorString;
    return LoadShip(root, persistentID, false, errorString);
}

// ShipConstruct.cs:301
bool ShipConstruct::LoadShip(const ConfigNode& root, uint32_t persistentID, bool returnErrors, std::string& errorString) {
    SaveEnvironment& env = SaveEnvironment::Current();
    errorString = "";
    parts.clear();
    changedPersistentIds_.clear();
    std::vector<std::vector<uint32_t>> list;   // links per part
    std::vector<std::vector<uint32_t>> list2;  // symmetry per part
    std::string partName, craftID, nodeID, attnPartID, attachMeshName;
    Vector3f attnPos, attnRot, attnActualPos, attnActualRot;
    persistentId = persistentID;
    loadPersistentID_ = persistentID;
    partIdCheckDone_ = !deferPartIdCheck_;
    shipFacility = EditorFacility::None;
    for (int i = 0; i < root.values().Count(); ++i) {
        const ConfigNode::Value& value = root.values()[i];
        const std::string& n = value.name;
        const std::string& v = value.value;
        if (n == "rot") rotation = KSPUtil::ParseQuaternion(v);
        else if (n == "description") shipDescription = dotnet::Replace(v, "\xC2\xA8", "\n");
        else if (n == "size") shipSize = KSPUtil::ParseVector3(v);
        else if (n == "type") shipFacility = ParseEnum<EditorFacility>(v);
        else if (n == "version") {
            if (KSPUtil::CheckVersion(v, lastCompatibleMajor, lastCompatibleMinor, lastCompatibleRev) !=
                VersionCompareResult::COMPATIBLE) {
                if (returnErrors) errorString = Loc("#autoLOC_6002428");
                else log::Error("Load Ship Failed: " + Loc("#autoLOC_6002428"));
                return false;
            }
        } else if (n == "OverrideGroupNames") {
            OverrideGroupNames.clear();
            for (const std::string& s : ParseExtensions::ParseArray(v, ",", false)) OverrideGroupNames.emplace_back(s);
            save_detail::ArrayMinSize(4, OverrideGroupNames);
        } else if (n == "steamPublishedFileId") steamPublishedFileId = save_detail::ParseULong(v);
        else if (n == "missionFlag") missionFlag = v;
        else if (n == "OverrideAxisControl") {
            save_detail::TryParseEnumIntArray(v, OverrideAxisControl);
            save_detail::ArrayMinSize(4, OverrideAxisControl);
        } else if (n == "persistentId") {
            if (persistentId == 0) persistentId = ParseUInt(v);
        } else if (n == "ship") shipName = v;
        else if (n == "vesselType") vesselType = ParseEnum<VesselType>(v);
        else if (n == "OverrideDefault") {
            ParseExtensions::TryParseBoolArray(v, OverrideDefault);
            save_detail::ArrayMinSize(4, OverrideDefault);
        } else if (n == "OverrideActionControl") {
            save_detail::TryParseEnumIntArray(v, OverrideActionControl);
            save_detail::ArrayMinSize(4, OverrideActionControl);
        }
    }
    persistentId = env.persistentIds.CheckVesselpersistentId(persistentId, nullptr, false, false);
    // every part must exist in the part database (PartLoader.getPartInfoByName; SaveEnvironment::partExists)
    std::vector<std::string> missing;
    for (int j = 0; j < root.nodes().Count(); ++j) {
        const ConfigNode& configNode = *root.nodes()[j];
        const std::string* pv = configNode.GetValuePtr("part");
        if (pv == nullptr) throw NullReferenceException("ShipConstruct.LoadShip: PART without a part value");
        partName = KSPUtil::GetPartName(*pv);
        const bool exists = env.partExists ? env.partExists(partName) : true;
        if (!exists && std::find(missing.begin(), missing.end(), partName) == missing.end()) missing.push_back(partName);
    }
    if (!missing.empty()) {
        std::string text;
        for (const auto& m : missing) text += m + "\n";
        if (returnErrors) errorString = Loc("#autoLOC_6002425") + " " + shipName + ": " + text;
        else log::Error("Craft Loading Error: " + shipName + ": " + text);
        return false;
    }
    std::vector<uint32_t> craftIds;
    for (int k = 0; k < root.nodes().Count(); ++k) {
        const ConfigNode& configNode = *root.nodes()[k];
        PartData* part = nullptr;
        std::vector<uint32_t> list4, list5;
        PartData data;
        auto need = [&](const std::string& field) -> PartData& {
            if (part == nullptr)
                throw NullReferenceException("ShipConstruct.LoadShip: '" + field + "' before 'part' in a PART node");
            return *part;
        };
        for (int l = 0; l < configNode.values().Count(); ++l) {
            const ConfigNode::Value& value = configNode.values()[l];
            const std::string& n = value.name;
            const std::string& v = value.value;
            if (n == "attRot0") need(n).attRotation0 = KSPUtil::ParseQuaternion(v);
            else if (n == "srfN") {
                Vector3f secAxis{0.0f, 0.0f, 0.0f};
                const bool info =
                    KSPUtil::GetAttachNodeInfo(v, nodeID, attnPartID, attnPos, attachMeshName, attnRot, secAxis);
                AttachNodeData& a = need(n).srfAttachNode;
                a.id = nodeID;
                a.attachedPartId = ParseUInt(attnPartID);
                a.srfAttachMeshName = attachMeshName;
                if (info) {
                    a.originalPosition = a.position = attnPos;
                    a.orientation = a.originalOrientation = attnRot;
                    a.secondaryAxis = a.originalSecondaryAxis = secAxis;
                    a.originalOrientationKnown = a.positionKnown = a.orientationKnown = true;
                } else {
                    // GetAttachNodeInfo only parses mesh and vectors for the id "srfAttach": the live node keeps the
                    // prefab's vectors, which SaveShip wrote into this line (proxy: the line's own vectors)
                    const std::vector<std::string> f = dotnet::Split(v, ",", false);
                    if (f.size() > 5) {
                        a.position = a.originalPosition = KSPUtil::ParseVector3(dotnet::Replace(f[3], "|", ","));
                        a.orientation = a.originalOrientation = KSPUtil::ParseVector3(dotnet::Replace(f[4], "|", ","));
                    }
                }
                part->srfAttachNodeInFile = true;
            } else if (n == "link") list4.push_back(ParseUInt(KSPUtil::GetLinkID(v)));
            else if (n == "sidx") need(n).inStageIndex = ParseInt(v);
            else if (n == "attm") need(n).attachMode = ParseInt(v);
            else if (n == "pSym") list5.push_back(ParseUInt(v));
            else if (n == "sepI") need(n).separationIndex = ParseInt(v);
            else if (n == "symMethod") need(n).symMethod = ParseEnum<SymmetryMethod>(v);
            else if (n == "resPri") need(n).resourcePriorityOffset = ParseInt(v);
            else if (n == "rot") need(n).rotation = KSPUtil::ParseQuaternion(v);
            else if (n == "attN") {
                const bool info =
                    KSPUtil::GetAttachNodeInfo(v, nodeID, attnPartID, attnPos, attnRot, attnActualPos, attnActualRot);
                PartData& p = need(n);
                // The prefab's node list is the file's entries (SaveShip wrote one per prefab node, prefab values for
                // the nodes LoadShip never touched); part.FindAttachNode(nodeID) then updates the FIRST node with this
                // id (prefabs with duplicate ids keep the later ones at their prefab values).
                p.attachNodes.emplace_back();
                {
                    AttachNodeData& own = p.attachNodes.back();
                    own.id = nodeID;
                    if (info) {
                        own.originalPosition = own.position = attnPos;
                        if (!IsPosInf(attnRot.x)) own.originalOrientation = attnRot, own.originalOrientationKnown = true;
                        if (!IsPosInf(attnActualPos.x)) own.position = attnActualPos, own.positionKnown = true;
                        if (!IsPosInf(attnActualRot.x)) own.orientation = attnActualRot, own.orientationKnown = true;
                    }
                }
                AttachNodeData* attachNode = nullptr;
                for (auto& a : p.attachNodes)
                    if (a.id == nodeID) {
                        attachNode = &a;
                        break;
                    }
                attachNode->attachedPartId = ParseUInt(attnPartID);
                if (info) {
                    attachNode->originalPosition = attnPos;
                    if (!IsPosInf(attnRot.x)) {
                        attachNode->originalOrientation = attnRot;
                        attachNode->originalOrientationKnown = true;
                    }
                    if (!IsPosInf(attnActualPos.x)) {
                        attachNode->position = attnActualPos;
                        attachNode->positionKnown = true;
                    }
                    if (!IsPosInf(attnActualRot.x)) {
                        attachNode->orientation = attnActualRot;
                        attachNode->orientationKnown = true;
                    }
                    if (!attachNode->positionKnown) attachNode->position = attnPos;  // prefab position (proxy)
                }
            } else if (n == "sameVesselCollision") need(n).sameVesselCollision = ParseBool(v);
            else if (n == "rigidAttachment") need(n).rigidAttachment = ParseBool(v);
            else if (n == "cData") need(n).customPartData = v;
            else if (n == "dstg") need(n).defaultInverseStage = ParseInt(v);
            else if (n == "pos") need(n).position = KSPUtil::ParseVector3(v);
            else if (n == "sqor") need(n).manualStageOffset = ParseInt(v);
            else if (n == "persistentId") need(n).persistentId = ParseUInt(v);
            else if (n == "part") {
                KSPUtil::GetPartInfo(v, partName, craftID);
                data = PartData();
                data.partInfoName = partName;
                data.craftID = ParseUInt(craftID);
                data.symMethod = shipFacility == EditorFacility::SPH ? SymmetryMethod::Mirror : SymmetryMethod::Radial;
                data.persistentId = env.persistentIds.GetUniquepersistentId();  // the instantiated prefab's id
                part = &data;
            } else if (n == "attPos0") need(n).attPos0 = KSPUtil::ParseVector3(v);
            else if (n == "istg") need(n).inverseStage = ParseInt(v);
            else if (n == "autostrutMode") need(n).autoStrutMode = ParseEnum<Part_AutoStrutMode>(v);
            else if (n == "attRot") need(n).attRotation = KSPUtil::ParseQuaternion(v);
            else if (n == "attPos") need(n).attPos = KSPUtil::ParseVector3(v);
            else if (n == "mir") need(n).mirrorVector = KSPUtil::ParseVector3(v);
            else if (n == "sym") list5.push_back(ParseUInt(KSPUtil::GetLinkID(v)));
            // values SaveShip writes from the prefab / module host, carried from the file (see the header)
            else if (n == "partName" && part) part->partName = v;
            else if (n == "modCost" && part) {
                float f = 0.0f;
                if (dotnet::TryParseSingle(v, f)) part->modCost = f;
            } else if (n == "modMass" && part) {
                float f = 0.0f;
                if (dotnet::TryParseSingle(v, f)) part->modMass = f;
            } else if (n == "modSize" && part) {
                Vector3f size;
                if (ParseExtensions::TryParseVector3(v, size)) part->modSize = size;
            }
        }
        if (part == nullptr) throw NullReferenceException("ShipConstruct.LoadShip: PART without a part value");
        for (int m = 0; m < configNode.nodes().Count(); ++m) {
            const ConfigNode::NodePtr& child = configNode.nodes().Shared(m);
            const std::string& n = child->name;
            if (n == "MODULE") part->modules.push_back(child->CreateCopy());
            else if (n == "ACTIONS") part->actions = child->CreateCopy();
            else if (n == "PARTDATA") part->partData = child->CreateCopy();
            else if (n == "RESOURCE") part->resources.push_back(child->CreateCopy());
            else if (n == "VESSELNAMING") part->vesselNaming = std::make_unique<VesselNaming>(*child);
            else if (n == "EVENTS") part->events = child->CreateCopy();
            else if (n == "EFFECTS") part->effects = child->CreateCopy();
        }
        // ShipConstruct.cs:630-640 CheckPartpersistentId and the ids it re-issued (UpdatePersistentIDs of IShipConstructIDChanges
        // modules). The file side has no Part to pass: LoadShipWithParts defers the check to InstantiateParts, which has the real parts
        // (the registry compares the part object); here a stand-in key is registered.
        if (!deferPartIdCheck_) {
            const uint32_t num2 = part->persistentId;
            part->persistentId = env.persistentIds.CheckPartpersistentId(part->persistentId,
                                                                         reinterpret_cast<const void*>(static_cast<uintptr_t>(k + 1)),
                                                                         false, true, persistentID);
            if (part->persistentId != num2 && num2 != 0) {
                if (changedPersistentIds_.count(num2) != 0)
                    log::Warning("[ShipConstruct]: The same persistentID existed on two parts in this ship - cannot track all part changes for " +
                                 part->partInfoName + "(" + dotnet::FormatUInt64(num2) + ")");
                else
                    changedPersistentIds_.emplace(num2, part->persistentId);
            }
        }
        parts.push_back(std::move(data));
        craftIds.push_back(parts.back().craftID);
        list.push_back(list4);
        list2.push_back(list5);
    }
    for (int n = 0; n < static_cast<int>(parts.size()); ++n) {
        PartData& part2 = parts[static_cast<size_t>(n)];
        for (uint32_t id : list[static_cast<size_t>(n)]) {
            const int child = GetPartByCraftID(id);
            if (child < 0) {
                log::Error("[ShipConstruct]: Unable to load Craft. Part " + part2.partInfoName + "-" +
                           dotnet::FormatUInt64(part2.craftID) + " Contains Invalid reference to part " +
                           dotnet::FormatUInt64(id) + " File corrupted.");
                return false;
            }
            // Part.setParent: leave the previous parent's children list, join the new one
            PartData& c = parts[static_cast<size_t>(child)];
            if (c.parent >= 0) {
                auto& siblings = parts[static_cast<size_t>(c.parent)].children;
                siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());
            }
            c.parent = n;
            part2.children.push_back(child);
        }
        for (uint32_t id : list2[static_cast<size_t>(n)]) part2.symmetryCounterparts.push_back(GetPartByCraftID(id));
    }
    return true;
}

// ShipConstruct.cs:153
ConfigNode::NodePtr ShipConstruct::SaveShip() {
    if (!liveParts.empty()) return SaveShipFromParts();  // WP-18: a ship with live parts saves them (ShipConstruct.cs:153 reads Part objects)
    GE::onAboutToSaveShip.Fire(this);
    auto configNode = std::make_shared<ConfigNode>();
    configNode->AddValue("ship", shipName);
    configNode->AddValue("version", Versioning_GetVersion());
    configNode->AddValue("description", dotnet::Replace(shipDescription, "\n", "\xC2\xA8"));
    configNode->AddValue("type", EnumName(shipFacility));
    configNode->AddValue("size", KSPUtil::WriteVector(shipSize));  // ShipConstruction.CalculateCraftSize (kept)
    configNode->AddValue("steamPublishedFileId", steamPublishedFileId);
    SaveEnvironment::Current().persistentIds.CheckVesselpersistentId(persistentId, nullptr, false, false);
    configNode->AddValue("persistentId", persistentId);
    configNode->AddValue("rot", rotation);
    configNode->AddValue("missionFlag", missionFlag);
    configNode->AddValue("vesselType", EnumName(vesselType));
    configNode->AddValue("OverrideDefault", save_detail::WriteBoolArray(OverrideDefault));
    configNode->AddValue("OverrideActionControl", save_detail::WriteEnumIntArray(OverrideActionControl));
    configNode->AddValue("OverrideAxisControl", save_detail::WriteEnumIntArray(OverrideAxisControl));
    configNode->AddValue("OverrideGroupNames", save_detail::WriteStringArray(OverrideGroupNames));
    auto nameOf = [this](int idx) { return parts[static_cast<size_t>(idx)].partInfoName; };
    for (const PartData& part : parts) {
        ConfigNode& n2 = *configNode->AddNode("PART");
        n2.AddValue("part", part.partInfoName + "_" + dotnet::FormatUInt64(part.craftID));
        save_detail::AddNullableValue(n2, "partName", part.partName);
        n2.AddValue("persistentId", part.persistentId);
        n2.AddValue("pos", KSPUtil::WriteVector(part.position));
        n2.AddValue("attPos", KSPUtil::WriteVector(part.attPos));
        n2.AddValue("attPos0", KSPUtil::WriteVector(part.attPos0));
        n2.AddValue("rot", KSPUtil::WriteQuaternion(part.rotation));
        n2.AddValue("attRot", KSPUtil::WriteQuaternion(part.attRotation));
        n2.AddValue("attRot0", KSPUtil::WriteQuaternion(part.attRotation0));
        n2.AddValue("mir", KSPUtil::WriteVector(part.mirrorVector));
        n2.AddValue("symMethod", EnumName(part.symMethod));
        n2.AddValue("autostrutMode", EnumName(part.autoStrutMode));
        n2.AddValue("rigidAttachment", dotnet::FormatBool(part.rigidAttachment));
        n2.AddValue("istg", static_cast<int32_t>(part.inverseStage));
        n2.AddValue("resPri", static_cast<int32_t>(part.resourcePriorityOffset));
        n2.AddValue("dstg", static_cast<int32_t>(part.defaultInverseStage));
        n2.AddValue("sidx", static_cast<int32_t>(part.inStageIndex));
        n2.AddValue("sqor", static_cast<int32_t>(part.manualStageOffset));
        n2.AddValue("sepI", static_cast<int32_t>(part.separationIndex));
        n2.AddValue("attm", static_cast<int32_t>(part.attachMode));
        n2.AddValue("sameVesselCollision", part.sameVesselCollision);
        n2.AddValue("modCost", part.modCost.value_or(0.0f));
        n2.AddValue("modMass", part.modMass.value_or(0.0f));
        n2.AddValue("modSize", part.modSize.value_or(Vector3f{0.0f, 0.0f, 0.0f}));
        if (!part.customPartData.empty()) n2.AddValue("cData", part.customPartData);
        for (int c : part.children) n2.AddValue("link", nameOf(c) + "_" + dotnet::FormatUInt64(parts[static_cast<size_t>(c)].craftID));
        for (int s : part.symmetryCounterparts) {
            if (s < 0) throw NullReferenceException("ShipConstruct.SaveShip: null symmetry counterpart");
            n2.AddValue("sym", nameOf(s) + "_" + dotnet::FormatUInt64(parts[static_cast<size_t>(s)].craftID));
        }
        const int srfPartner = GetPartByCraftID(part.srfAttachNode.attachedPartId);
        if (srfPartner >= 0) {
            const AttachNodeData& a = part.srfAttachNode;
            std::string s = a.id + "," + nameOf(srfPartner) + "_" + dotnet::FormatUInt64(parts[static_cast<size_t>(srfPartner)].craftID);
            if (a.srfAttachMeshName && !a.srfAttachMeshName->empty()) s += "," + *a.srfAttachMeshName;
            else s += ",";
            s += "," + KSPUtil::WriteVector(a.position, "|");
            s += "," + KSPUtil::WriteVector(a.orientation, "|");
            s += "," + KSPUtil::WriteVector(a.position, "|");
            n2.AddValue("srfN", s);
        }
        for (const AttachNodeData& a : part.attachNodes) {
            const int partner = GetPartByCraftID(a.attachedPartId);
            const std::string value5 = partner >= 0 ? nameOf(partner) : "Null";
            const std::string value6 = partner >= 0 ? dotnet::FormatUInt64(parts[static_cast<size_t>(partner)].craftID) : "0";
            n2.AddValue("attN", a.id + "," + value5 + "_" + value6 + "_" + KSPUtil::WriteVector(a.originalPosition, "|") +
                                    "_" + KSPUtil::WriteVector(a.originalOrientation, "|") + "_" +
                                    KSPUtil::WriteVector(a.position, "|") + "_" + KSPUtil::WriteVector(a.orientation, "|"));
        }
        (part.events ? part.events->CopyTo(*n2.AddNode("EVENTS")) : (void)n2.AddNode("EVENTS"));
        (part.actions ? part.actions->CopyTo(*n2.AddNode("ACTIONS")) : (void)n2.AddNode("ACTIONS"));
        (part.partData ? part.partData->CopyTo(*n2.AddNode("PARTDATA")) : (void)n2.AddNode("PARTDATA"));
        for (const auto& m : part.modules) m->CopyTo(*n2.AddNode("MODULE"));
        for (const auto& r : part.resources) r->CopyTo(*n2.AddNode("RESOURCE"));
        if (part.vesselNaming) part.vesselNaming->Save(*n2.AddNode("VESSELNAMING"));
    }
    return configNode;
}

}  // namespace ksp
