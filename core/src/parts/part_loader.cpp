#include "ksp/parts/part_loader.hpp"
#include "ksp/modules/part_module.hpp"

#include <algorithm>
#include <filesystem>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/database/game_database.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/dotnet/utf16.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/unity/quaternion_native.hpp"

namespace ksp {




namespace {

PartLoader* g_loader = nullptr;

using KSP::Localization::Localizer;

std::string LocOr(const char* tag, const char* fallback) {
    return Localizer::Instance() ? Localizer::Format(tag) : std::string(fallback);
}

// AvailablePart.cs:282 Init
void InitAvailablePart(AvailablePart& ap) {
    ap.author = LocOr("#autoLOC_168872", "Unknown");
    ap.title = LocOr("#autoLOC_168874", "Unknown Mystery Component");
    ap.manufacturer = LocOr("#autoLOC_168875", "Found lying by the side of the road");
    ap.description = LocOr("#autoLOC_168876", "Nothing is really known about this thing. Use it at your own risk.");
    ap.name = "unknownPart";
    ap.amountAvailable = 999;
    ap.internalConfig = ConfigNode::Create();
    ap.TechRequired = "";
    ap.identicalParts = "";
    ap.entryCost = 0;
    ap.tags = "*";
    ap.mapActionsToSymmetryParts = true;
}

std::string ToLowerAscii(std::string s) {
    for (char& c : s)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return s;
}
std::string ToUpperAscii(std::string s) {
    for (char& c : s)
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    return s;
}

// Vector3 * float (UnityEngine.Vector3.operator*, managed: one multiply per component, stored as float)
Vector3f Scale(const Vector3f& v, float d) {
    return Vector3f{f32store(fx(v.x) * fx(d)), f32store(fx(v.y) * fx(d)), f32store(fx(v.z) * fx(d))};
}

Vector3f ParseVector3Strict(const std::vector<std::string>& a, size_t i) {
    return Vector3f{cs::FloatParse(a.at(i)), cs::FloatParse(a.at(i + 1)), cs::FloatParse(a.at(i + 2))};
}

const std::string& At(const std::vector<std::string>& a, size_t i) {
    if (i >= a.size()) throw std::out_of_range("Index was outside the bounds of the array.");
    return a[i];
}

// The node_* attach node grammar shared by ParsePart (PartLoader.cs:740) and PartVariant.ParseNode (PartVariant.cs:515).
// `withOneWay`: ParsePart reads an 11th value (AllowOneWayXFeed); PartVariant does not. Returns false for < 6 values.
bool ParseNodeValue(const ConfigNode::Value& v, float scale, bool withOneWay, AttachNode& n) {
    const std::vector<std::string> array = dotnet::Split(v.value, ",", false);
    const std::vector<std::string> array2 = dotnet::Split(v.name, "_", false);
    if (array.size() < 6) {
        log::Warning("ERROR: Bad node definition at " + v.name);
        return false;
    }
    n = AttachNode();
    n.position = ParseVector3Strict(array, 0);
    n.orientation = ParseVector3Strict(array, 3);
    if (withOneWay) {  // ParsePart scales by rescaleFactor x scale; PartVariant.ParseNode does not
        n.position = Scale(n.position, scale);
        n.orientation = Scale(n.orientation, scale);
    }
    n.originalPosition = n.position;
    n.originalOrientation = n.orientation;
    n.size = array.size() >= 7 ? cs::IntParse(array[6]) : 1;
    const std::string& kind = At(array2, 1);
    if (array.size() >= 8) {
        n.attachMethod = static_cast<AttachNodeMethod>(cs::IntParse(array[7]));
    } else {
        if (kind == "stack") n.attachMethod = AttachNodeMethod::FIXED_JOINT;
        if (kind == "dock") n.attachMethod = AttachNodeMethod::FIXED_JOINT;
        if (kind == "attach") n.attachMethod = AttachNodeMethod::HINGE_JOINT;
    }
    if (array.size() >= 9) n.ResourceXFeed = cs::IntParse(array[8]) > 0;
    if (array.size() >= 10) n.rigid = cs::IntParse(array[9]) > 0;
    if (withOneWay && array.size() >= 11) n.AllowOneWayXFeed = cs::IntParse(array[10]) > 0;
    if (kind == "stack" || kind == "dock") {
        if (array2.size() > 2) n.id = array2[2];
        n.nodeType = kind == "stack" ? AttachNodeType::Stack : AttachNodeType::Dock;
    }
    if (kind == "attach") {
        n.id = "srfAttach";
        n.nodeType = AttachNodeType::Surface;
    }
    return true;
}

// BasePartCategorizer.cs:246
std::vector<std::string> SearchTagSplit(std::string_view terms) {
    return dotnet::Split(ToLowerAscii(dotnet::Trim(terms)), " |;,-", true);
}

std::string GameDataRel(const std::string& full, const std::string& gameData) {
    if (full.empty()) return full;
    std::string f = full, gd = gameData + "/";
    std::replace(f.begin(), f.end(), '\\', '/');
    std::replace(gd.begin(), gd.end(), '\\', '/');
    while (gd.find("//") != std::string::npos) gd.replace(gd.find("//"), 2, "/");
    if (f.size() >= gd.size() && dotnet::EqualsOrdinalIgnoreCase(std::string_view(f).substr(0, gd.size()), gd))
        return "GameData/" + f.substr(gd.size());
    for (size_t i = 0; i + 10 <= f.size(); i++)
        if (dotnet::EqualsOrdinalIgnoreCase(std::string_view(f).substr(i, 10), "/GameData/")) return f.substr(i + 1);
    return f;
}

bool HasModuleClass(const PartPrefab& p, std::string_view cls) {
    for (const PartModuleData& m : p.Modules)
        if (m.moduleClass && m.moduleClass->Is(cls)) return true;
    return false;
}

// LoadObjectFromConfig(AvailablePart, node, 0, removeAfterUse: true): the [Persistent] value fields of AvailablePart
// (ConfigNode.cs:1184 / ReadObject :2666 / ReadValue :2803). Values are read in node order, matched names are blanked,
// then every blank-named value and node is removed.
void LoadAvailablePartPersistent(AvailablePart& ap, ConfigNode& node) {
    auto str = [](NullableString& dst, const std::string& v) { dst = v; };
    for (auto& v : node.values()) {
        const std::string& n = v.name;
        bool matched = true;
        if (n == "name") ap.name = v.value;
        else if (n == "title") ap.title = v.value;
        else if (n == "manufacturer") ap.manufacturer = v.value;
        else if (n == "author") ap.author = v.value;
        else if (n == "description") ap.description = v.value;
        else if (n == "typeDescription") str(ap.typeDescription, v.value);
        else if (n == "moduleInfo") str(ap.moduleInfo, v.value);
        else if (n == "resourceInfo") str(ap.resourceInfo, v.value);
        else if (n == "category") ap.category = static_cast<int32_t>(kEnum_PartCategories.Parse(v.value));  // ParseEnum throws
        else if (n == "TechRequired") ap.TechRequired = v.value;
        else if (n == "TechHidden") { bool b; if (cs::BoolTryParse(v.value, b)) ap.TechHidden = b; }
        else if (n == "_entryCost") { int32_t i; if (cs::IntTryParse(v.value, i)) ap.entryCost = i; }
        else if (n == "identicalParts") ap.identicalParts = v.value;
        else if (n == "amountAvailable") { int32_t i; if (cs::IntTryParse(v.value, i)) ap.amountAvailable = i; }
        else if (n == "cost") { float f; if (cs::FloatTryParse(v.value, f)) ap.cost = f; }
        else if (n == "bulkheadProfiles") str(ap.bulkheadProfiles, v.value);
        else if (n == "tags") ap.tags = v.value;
        else if (n == "showVesselNaming") { bool b; if (cs::BoolTryParse(v.value, b)) ap.showVesselNaming = b; }
        else if (n == "mapActionsToSymmetryParts") { bool b; if (cs::BoolTryParse(v.value, b)) ap.mapActionsToSymmetryParts = b; }
        else if (n == "preferredStage") ap.preferredStage = static_cast<int32_t>(kEnum_PreferredInitialStage.Parse(v.value));
        else matched = false;  // "variant" (a class) is not a value field: left in place
        if (matched) v.name.clear();
    }
    for (ConfigNode* child : node.nodes().GetNodes())
        if (child->name == "variant") {
            log::Warning("PartLoader: AvailablePart.variant node ignored by kspcore (PartVariant object read)");
            child->name.clear();
        }
    node.RemoveValues(std::string_view(""));
    node.RemoveNodes(std::string_view(""));
}

// ShipConstruction.cs:1238 GetPartCostsAndMass (dryCost / fuelCost only) + :1292 SanitizePartCosts
void SanitizePartCosts(AvailablePart& ap, const ConfigNode& partNode) {
    float dryCost = ap.cost;
    float fuelCost = 0.0f;
    if (partNode.HasValue("modCost")) dryCost = f32store(fx(dryCost) + fx(cs::FloatParse(*partNode.GetValue("modCost"))));
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    const auto children = partNode.nodes().GetNodes();
    for (size_t i = children.size(); i-- > 0;) {
        const ConfigNode& c = *children[i];
        if (c.name != "RESOURCE") continue;
        const auto rn = c.GetValue("name");
        const PartResourceDefinition* def = (lib && rn) ? lib->GetDefinition(*rn) : nullptr;
        if (def && c.HasValue("amount")) {
            const float num = cs::FloatParse(*c.GetValue("amount"));
            const auto mx = c.GetValue("maxAmount");
            if (!mx) throw ArgumentNullException("String reference not set to an instance of a String.");
            const float num2 = cs::FloatParse(*mx);
            dryCost = f32store(fx(dryCost) - fx(def->unitCost()) * fx(num2));
            fuelCost = f32store(fx(fuelCost) + fx(def->unitCost()) * fx(num));
        } else {
            log::Error("[ShipTemplate]: No Resource definition found for " + c.name);
        }
    }
    if (dryCost < 0.0f) {
        log::Warning("[ShipConstruct for " + ap.name + "]: part cost (" + dotnet::FormatSingleF(ap.cost, 1) +
                     ") is less than the cost of its resources (" + dotnet::FormatSingleF(fuelCost, 1) + ")");
        ap.cost = fuelCost;
    }
}

// Part.AddResource -> PartResourceList.Add(ConfigNode) (PartResourceList.cs:195)
void AddResource(PartPrefab& part, const ConfigNode& node) {
    const auto name = node.GetValue("name");
    if (!name) {
        log::Warning("ConfigNode has no value 'name'");
        return;
    }
    for (const PartResource& r : part.Resources)
        if (MonoStringHash(r.resourceName) == MonoStringHash(*name)) {
            log::Warning("Part already contains  resource '" + *name + "'");
            return;
        }
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    const PartResourceDefinition* def = lib ? lib->GetDefinition(*name) : nullptr;
    if (!def) {
        log::Warning("Could not create PartResource of type '" + *name);
        return;
    }
    PartResource r;
    r.info = def;
    r.resourceName = def->name();
    r.Load(node);
    part.Resources.push_back(std::move(r));
}

}  // namespace

// ModulePartVariants.cs:348 OnAwake (base variant) + :380 OnLoad (variant list). Returns the `variantIndex` the load sets
// (the index of the cfg's baseVariant, then of `selectedVariant`), -1 when it leaves the field alone.
int LoadModulePartVariants(PartPrefab& part, const ConfigNode& node) {
    PartVariant base;
    base.name = LocOr("#autoLOC_8007005", "Basic");
    base.displayName = base.name;
    base.attachNodes = part.attachNodes;
    base.srfAttachNode = part.srfAttachNode;
    part.baseVariant = base;
    part.variantList.clear();
    PartVariant& bv = *part.baseVariant;
    auto nonEmpty = [&](const char* k) -> std::optional<std::string> {
        auto v = node.GetValue(k);
        if (v && !v->empty()) return v;
        return std::nullopt;
    };
    if (auto v = nonEmpty("primaryColor")) bv.primaryColor = *v;
    if (auto v = nonEmpty("secondaryColor")) bv.secondaryColor = *v;
    if (auto v = nonEmpty("sizeGroup")) bv.sizeGroup = *v;
    if (auto v = nonEmpty("baseName")) bv.name = bv.displayName = *v;
    if (auto v = nonEmpty("baseDisplayName")) bv.displayName = *v;
    if (auto v = nonEmpty("baseMass")) { float r = 0.0f; cs::FloatTryParse(*v, r); bv.mass = r; }
    if (auto v = nonEmpty("baseCost")) { float r = 0.0f; cs::FloatTryParse(*v, r); bv.cost = r; }
    if (auto v = nonEmpty("baseThemeName")) bv.themeName = *v;
    bv.attachRules = part.fields.attachRules;
    for (ConfigNode* vn : node.GetNodes("VARIANT")) {
        PartVariant pv;
        // new PartVariant(baseVariant): copies name..sizeGroup, mass, cost, srfAttachNode, attach nodes, animations,
        // events, themeName, attachRules (PartVariant.cs:213)
        pv.name = bv.name;
        pv.displayName = bv.displayName;
        pv.primaryColor = bv.primaryColor;
        pv.secondaryColor = bv.secondaryColor;
        pv.sizeGroup = bv.sizeGroup;
        pv.mass = bv.mass;
        pv.cost = bv.cost;
        pv.srfAttachNode = bv.srfAttachNode;
        pv.attachNodes = bv.attachNodes;
        pv.disabledAnimations = bv.disabledAnimations;
        pv.disabledEvents = bv.disabledEvents;
        pv.themeName = bv.themeName;
        pv.attachRules = bv.attachRules;
        pv.Load(*vn);
        part.variantList.push_back(std::move(pv));
    }
    int variantIndex = -1;
    std::string baseVariantName;
    if (node.TryGetValue("baseVariant", baseVariantName)) {
        int index = 0;  // GetVariantIndex: 0 when not found
        for (size_t i = 0; i < part.variantList.size(); i++)
            if (part.variantList[i].name == baseVariantName) {
                index = static_cast<int>(i);
                break;
            }
        part.baseVariant = part.variantList.at(static_cast<size_t>(index));
        variantIndex = index;
    } else {
        part.variantList.insert(part.variantList.begin(), *part.baseVariant);
    }
    // OnLoad: selectedVariant -> variantIndex = GetVariantIndex(name) (0 when not found)
    std::string selected;
    node.TryGetValue("selectedVariant", selected);
    if (!selected.empty()) {
        variantIndex = 0;
        for (size_t i = 0; i < part.variantList.size(); i++)
            if (part.variantList[i].name == selected) {
                variantIndex = static_cast<int>(i);
                break;
            }
    }
    return variantIndex;
}

// PartVariant.cs:263
void PartVariant::Load(const ConfigNode& node) {
    name = node.GetValue("name").value_or(std::string());
    const auto dn = node.GetValue("displayName");
    displayName = (dn && !dn->empty()) ? *dn : name;
    primaryColor = node.GetValue("primaryColor");
    secondaryColor = node.GetValue("secondaryColor");
    sizeGroup = node.GetValue("sizeGroup");
    float r = 0.0f;
    if (const auto m = node.GetValue("mass")) cs::FloatTryParse(*m, r); else r = 0.0f;
    mass = r;
    float r2 = 0.0f;
    if (const auto c = node.GetValue("cost")) cs::FloatTryParse(*c, r2); else r2 = 0.0f;
    cost = r2;
    if (const ConfigNode* go = node.GetNode("GAMEOBJECTS")) {
        infoGameObjects.clear();
        for (const auto& v : go->values()) {
            bool b;
            if (cs::BoolTryParse(v.value, b)) infoGameObjects.emplace_back(v.name, b);
        }
    }
    if (const ConfigNode* ei = node.GetNode("EXTRA_INFO")) {
        extraInfo.clear();
        for (const auto& v : ei->values()) extraInfo.emplace_back(v.name, v.value);
    }
    if (const ConfigNode* nodes = node.GetNode("NODES")) {
        for (const auto& v : nodes->values()) {
            if (!dotnet::StartsWith(v.name, "node")) continue;
            AttachNode an;
            if (!ParseNodeValue(v, 1.0f, false, an)) continue;
            if (an.id != "attach" && an.id != "srfAttach") {
                for (AttachNode& existing : attachNodes)
                    if (existing.id == an.id) {
                        existing = an;
                        break;
                    }
            } else {
                srfAttachNode = an;
            }
        }
    }
    for (ConfigNode* t : node.GetNodes("TEXTURE")) textures.push_back(t->CreateCopy());
    if (const auto da = node.GetValue("disabledAnimations"))
        for (const std::string& s : dotnet::Split(*da, ",", false)) disabledAnimations.push_back(s);
    if (const auto de = node.GetValue("disabledEvents"))
        for (const std::string& s : dotnet::Split(*de, ",", false)) disabledEvents.push_back(s);
    std::string tn;
    if (node.TryGetValue("themeName", tn)) themeName = tn;
    std::string ar;
    if (node.TryGetValue("attachRules", ar)) attachRules = AttachRules::Parse(ar);
}

const std::vector<PartVariant>* AvailablePart::Variants() const {
    if (partPrefab && partPrefab->hasVariantsModule) return &partPrefab->variantList;
    return nullptr;
}

void AvailablePart::SetMinimumMass(float v) { minimumMass = std::max(0.0001f, v); }
void AvailablePart::SetMinimumRBMass(float v) { minimumRBMass = std::max(0.001f, v); }

// AvailablePart.cs:247
void AvailablePart::AddFileTime(const io::UrlFile* file) {
    if (!file) {
        log::Info("AddFileTime: file is null!");
        return;
    }
    fileTimes.push_back(file->fileTime().ToBinaryString());
}

// ---------------------------------------------------------------------------------------------------------------------
std::vector<std::string> PartLoader::DefaultCodeBuiltParts() {
    return {"kerbalEVA",        "kerbalEVAfemale",        "flag",
            "kerbalEVAVintage", "kerbalEVAfemaleVintage", "kerbalEVAFuture",
            "kerbalEVAfemaleFuture", "kerbalEVASlimSuit", "kerbalEVASlimSuitFemale"};
}

PartLoader::PartLoader() { g_loader = this; }
PartLoader::~PartLoader() {
    if (g_loader == this) g_loader = nullptr;
}
PartLoader* PartLoader::Instance() { return g_loader; }

const AvailablePart* PartLoader::getPartInfoByName(std::string_view name) const {
    for (const auto& ap : loadedParts)
        if (ap->name == name) return ap.get();  // APFinderByName keeps the first of a name
    return nullptr;
}

// PartLoader.cs:985
bool PartLoader::ApplyPartValue(PartPrefab& part, const ConfigNode::Value& nodeValue) {
    const std::string& n = nodeValue.name;
    if (n == "dragModelType") {
        const std::string u = ToUpperAscii(nodeValue.value);
        PartDragModel m = PartDragModel::CUBE;
        if (u == "SPHERICAL") m = PartDragModel::SPHERICAL;
        else if (u == "NONE" || u == "OVERRIDE") m = PartDragModel::NONE;
        else if (u == "CYLINDRICAL") m = PartDragModel::CYLINDRICAL;
        else if (u == "CONIC") m = PartDragModel::CONIC;
        part.fields.dragModel = static_cast<int32_t>(m);
        return true;
    }
    if (n == "partRendererBoundsIgnore") {
        std::vector<std::string> list;
        for (const std::string& s : dotnet::Split(nodeValue.value, ",", false)) list.push_back(dotnet::Trim(s));
        part.fields.partRendererBoundsIgnore = std::move(list);
        return true;
    }
    static const char* kSkip[] = {"iconCenter", "alphaCutoff", "scale", "texture", "normalmap", "name", "specPower",
                                  "rimFalloff", "vesselType", "mesh", "subcategory", "module", "exportScale"};
    for (const char* s : kSkip)
        if (n == s) return false;
    if (n == "dragModel") {
        part.fields.dragModel = static_cast<int32_t>(kEnum_Part_DragModel.Parse(nodeValue.value));
        return true;
    }
    if (dotnet::StartsWith(n, "node") || dotnet::StartsWith(n, "fx") || dotnet::StartsWith(n, "sound")) return false;
    const PartFieldInfo* field = FindPartField(n, part.className == "CompoundPart");
    if (!field) {
        log::Warning("PartLoader Warning: Variable " + n + " not found in " + part.className);
        return true;
    }
    void* addr = field->address ? field->address(part.fields, part.compoundFields) : nullptr;
    const std::string& v = nodeValue.value;
    switch (field->kind) {
        case FieldKind::String:
            *static_cast<NullableString*>(addr) = v;
            return true;
        case FieldKind::Vector3: {
            const auto a = dotnet::Split(v, ",", false);
            if (a.size() < 3) {
                log::Info("WARNING: " + n + " is nor formatted properly! proper format for Vector3s is x,y,z");
                return true;
            }
            *static_cast<Vector3f*>(addr) = ParseVector3Strict(a, 0);
            return true;
        }
        case FieldKind::Quaternion: {
            const auto a = dotnet::Split(v, ",", false);
            if (a.size() < 4) {
                log::Warning("WARNING: " + n + " is nor formatted properly! proper format for Quaternions is angle(deg),x,y,z");
                return true;
            }
            const float angle = cs::FloatParse(a[0]);
            *static_cast<Quaternionf*>(addr) = UnityNative::AngleAxis(angle, ParseVector3Strict(a, 1));
            return true;
        }
        case FieldKind::Int32:
            *static_cast<int32_t*>(addr) = cs::IntParse(v);
            return true;
        case FieldKind::Double:
            // PartLoader.cs:1124: the first `typeof(double)` branch parses with float.Parse (sic); the second, with
            // double.Parse, is unreachable
            *static_cast<double*>(addr) = static_cast<double>(cs::FloatParse(v));
            return true;
        case FieldKind::Boolean:
            *static_cast<bool*>(addr) = cs::BoolParse(v);
            return true;
        case FieldKind::Enum:
            *static_cast<int32_t*>(addr) = static_cast<int32_t>(field->enumTable->Parse(v));
            return true;
        // The generic tail: the field type's static Parse(string), via reflection
        case FieldKind::Single:  // float.Parse
            *static_cast<float*>(addr) = cs::FloatParse(v);
            return true;
        case FieldKind::UInt32: {  // uint.Parse
            uint32_t r;
            if (!dotnet::TryParseUInt32(v, r)) throw dotnet::FormatException("Input string was not in a correct format.");
            *static_cast<uint32_t*>(addr) = r;
            return true;
        }
        case FieldKind::AttachRules:  // AttachRules.Parse
            *static_cast<NullableAttachRules*>(addr) = AttachRules::Parse(v);
            return true;
        default:  // Vector3d, Color, List<string>, object references: no Parse(string) -> false
            return false;
    }
}

// PartLoader.cs:1581 (never called by KSP)
std::string PartLoader::InferBulkheadProfiles(const ConfigNode& n) {
    std::string text;
    auto append = [&](const std::string& v) { text = text.empty() ? v : text + "," + v; };  // KSPUtil.AppendValueToString
    for (const std::string& s : n.GetValuesStartsWith("node_stack")) {
        const size_t commas = static_cast<size_t>(std::count(s.begin(), s.end(), ','));
        if (commas >= 6)
            append("size" + std::to_string(cs::IntParse(dotnet::Split(s, ",", false)[6])));
        else
            append("size1");
    }
    const size_t nAttach = n.GetValuesStartsWith("node_attach").size();
    for (size_t j = 0; j < nAttach; j++) append("srf");
    return text;
}

// BasePartCategorizer.cs:311
std::string PartLoader::GeneratePartAutoTags(const AvailablePart& p) {
    static const std::vector<std::string> size0{"0.625", ")mini", "small", "tiny", "little", "micro"};
    static const std::vector<std::string> size1{"1.25", "FL-T", "regular", "standard", "average", "medium"};
    static const std::vector<std::string> size1p5{"1.875", "FL-TX", "regular", "standard", "average", "medium"};
    static const std::vector<std::string> size2{"2.5", "huge", "jumbo", "large", "big"};
    static const std::vector<std::string> size3{"3.75", "enormous", "massive", "gigantic", "giant"};
    static const std::vector<std::string> size4{"5", "enormous", "massive", "gigantic", "giant"};
    static const std::vector<std::string> srf{"surface", "attach"}, xfeed{"cross", "feed"};
    static const std::vector<std::string> manned{"(crew", "(mann", "kerbal"}, unmanned{"(uncrew", "(unmann"};
    static const std::vector<std::string> radial{"radial"}, cargo{"cargo"};
    std::vector<std::string> list;
    const auto t = SearchTagSplit(p.title);
    for (size_t i = t.size(); i-- > 0;) list.push_back((dotnet::ToUtf16(t[i]).size() > 3 ? "(" : "?") + t[i]);
    const auto m = SearchTagSplit(p.manufacturer + " " + kEnum_PartCategories.ToString(p.category));
    for (size_t i = m.size(); i-- > 0;) list.push_back("?" + m[i]);
    auto add = [&](const std::vector<std::string>& v) { list.insert(list.end(), v.begin(), v.end()); };
    if (p.bulkheadProfiles) {
        const auto a = dotnet::Split(*p.bulkheadProfiles, ",", false);
        for (size_t i = a.size(); i-- > 0;) {
            const std::string s = dotnet::Trim(a[i]);
            if (s == "size0") add(size0);
            else if (s == "size4") add(size4);
            else if (s == "size1p5") add(size1p5);
            else if (s == "size2") add(size2);
            else if (s == "size1") add(size1);
            else if (s == "srf") {
                add(srf);
                if (a.size() == 1) add(radial);
            } else if (s == "size3") add(size3);
        }
    }
    if (p.partPrefab) {
        const PartPrefab& pp = *p.partPrefab;
        if (pp.fields.fuelCrossFeed) add(xfeed);
        if (pp.fields.vesselType > 0) add(pp.fields.CrewCapacity > 0 ? manned : unmanned);  // > VesselType.Debris
        if (HasModuleClass(pp, "ModuleCargoPart")) add(cargo);
    }
    std::string out;
    for (size_t i = 0; i < list.size(); i++) out += (i ? " " : "") + list[i];
    return out;
}

// ---- PartDatabase.cfg (PartLoader.cs:220-364) --------------------------------------------------------------------------
void PartLoader::InitializePartDatabase() {
    auto create = [&] {
        partDatabase = ConfigNode::Create();
        partDatabase->AddNode("PART_DATABASE")->AddValue("version", options_.versionString);
    };
    if (!options_.partDatabasePath.empty() &&
        std::filesystem::exists(dotnet::PathFromUtf8(options_.partDatabasePath))) {
        partDatabase = ConfigNode::Load(options_.partDatabasePath);
        if (partDatabase) {
            ConfigNode* node = partDatabase->GetNode("PART_DATABASE");
            bool ok = false;
            if (node) {
                const auto v = node->GetValue("version");
                if (!v) {
                    node->AddValue("version", options_.versionString);
                    ok = true;
                } else {
                    ok = *v == options_.versionString;
                }
            }
            if (ok) {
                log.push_back("PartLoader: Loading part database");
                return;
            }
            log.push_back("PartLoader: Version strings do not match. Creating part database.");
            create();
            return;
        }
    }
    log.push_back("PartLoader: Creating part database");
    create();
}

ConfigNode* PartLoader::GetDatabaseConfig(const AvailablePart& ap) {
    ConfigNode* n = partDatabase->GetNode("PART", "url", ap.partUrl);
    auto setStamps = [&](ConfigNode& c) {
        std::string text;
        for (const std::string& t : ap.fileTimes) text += (text.empty() ? "" : ", ") + t;
        c.SetValue("timestamps", text, true);
    };
    if (!n) {
        log.push_back("PartLoader: Part '" + ap.partUrl + "' has no database record. Creating.");
        n = partDatabase->AddNode("PART");
        n->AddValue("url", ap.partUrl);
        setStamps(*n);
        return n;
    }
    // CheckPartDatabaseTimestamps (PartLoader.cs:344)
    if (const auto ts = n->GetValue("timestamps")) {
        const auto a = dotnet::Split(*ts, ", ", true);
        bool same = a.size() == ap.fileTimes.size();
        for (size_t i = 0; same && i < a.size(); i++) same = a[i] == ap.fileTimes[i];
        if (!same) {
            log.push_back("PartLoader: Timestamps for part '" + ap.partUrl + "' do not match. Resetting.");
            setStamps(*n);
            n->ClearNodes();
        }
    }
    return n;
}

ConfigNode* PartLoader::GetDatabaseConfig(const AvailablePart& ap, std::string_view nodeName) {
    ConfigNode* n = GetDatabaseConfig(ap);
    return n ? n->GetNode(nodeName) : nullptr;
}

void PartLoader::SetDatabaseConfig(const AvailablePart& ap, const ConfigNode::NodePtr& newNode) {
    ConfigNode* db = GetDatabaseConfig(ap);
    if (db->HasNode(newNode->name)) db->RemoveNode(newNode->name);
    db->AddNode(newNode);
}

// ---- modules ----------------------------------------------------------------------------------------------------------
// Part.cs:5787 AddModule(string)
PartModuleData* PartLoader::AddModule(PartPrefab& part, std::string_view moduleName) {
    const ModuleClassInfo* cls = GetPartModuleClass(moduleName);
    if (!cls) {
        log.push_back("Cannot find a PartModule of typename '" + std::string(moduleName) + "'");
        log::Error(log.back());
        return nullptr;
    }
    auto contains = [&](std::string_view c) {
        for (const PartModuleData& m : part.Modules)
            if (m.moduleClass && c == m.moduleClass->name) return true;
        return false;
    };
    if (std::string_view(cls->name) == "ModuleCargoPart" && contains("ModuleInventoryPart")) {
        log::Warning("[Part]: Cannot add " + std::string(moduleName) + " as this Part '" + part.gameObjectName +
                     "' already has a ModuleInventoryPart PartModule and they are mutually exclusive.");
        return nullptr;
    }
    if (std::string_view(cls->name) == "ModuleInventoryPart" && contains("ModuleCargoPart")) {
        for (size_t i = 0; i < part.Modules.size(); i++) {
            PartModuleData& m = part.Modules[i];
            if (!m.moduleClass || !m.moduleClass->Is("ModuleCargoPart")) continue;
            // FindModuleImplementing<ModuleCargoPart>().packedVolume (KSPField, default 0 or the cfg value)
            float packedVolume = 0.0f;
            if (m.module) packedVolume = m.module->Get<float>("packedVolume");
            if (packedVolume >= 0.0f) part.Modules.erase(part.Modules.begin() + static_cast<std::ptrdiff_t>(i));
            break;
        }
        log::Warning("[Part]: Cannot have ModuleCargoPart and ModuleInventoryPart on same Part [" + part.gameObjectName +
                     "'. Removed ModuleCargoPart");
    }
    PartModuleData m;
    m.moduleName = cls->name;  // PartModule.moduleName = ClassName
    m.moduleClass = cls;
    // gameObject.AddComponent(type): the constructor chain (KSPField initialisers) and Awake (the object is active while
    // the prefab compiles; PartLoader.cs:511 / :551 force Awake on the inactive prefab parts)
    m.module = ModuleFactory::Create(*cls);
    m.module->part = &part;
    m.module->Awake();
    part.Modules.push_back(std::move(m));
    return &part.Modules.back();
}

// Part.cs:5823 AddModule(ConfigNode)
void PartLoader::AddModule(PartPrefab& part, ConfigNode& node, bool /*forceAwake*/) {
    const auto name = node.GetValue("name");
    if (!name) {
        log::Error("Cannot add a PartModule because the ConfigNode contains no module name");
        return;
    }
    PartModuleData* m = AddModule(part, *name);
    if (!m) return;
    m->moduleName = *name;
    LoadModuleData(*m, node, &part);  // PartModule.Load: Fields.Load ... OnLoad (ModuleToggleCrossfeed, ModuleCargoPart, ...)
}

// ---- CompileModel (PartLoader.cs:1765), data side ----------------------------------------------------------------------
bool PartLoader::CompileModel(const GameDatabase& db, io::UrlConfig& cfg, ConfigNode& partCfg, AvailablePart& ap) {
    PartPrefab& part = *ap.partPrefab;
    const std::vector<ConfigNode*> nodes = partCfg.GetNodes("MODEL");
    if (!nodes.empty()) {
        bool any = false;
        for (ConfigNode* cn : nodes) {
            const auto value = cn->GetValue("model");
            const GameDatabase::ModelInfo* prefab = value ? db.GetModelPrefab(*value) : nullptr;
            if (!prefab) {
                log::Error("PartCompiler: Cannot clone model '" + value.value_or("") + "' as model does not exist");
                continue;
            }
            any = true;
            PartModelRef mr;
            mr.url = prefab->name;
            mr.file = db.GetModelFile(prefab);
            mr.fromModelNode = true;
            if (!mr.file) log::Info("AddFileTime: File '" + *value + "' is null!");
            else ap.AddFileTime(mr.file);
            mr.parent = cn->GetValue("parent").value_or("");
            mr.name = cn->GetValue("name");
            if (const auto s = cn->GetValue("scale")) mr.scale = ConfigNode::ParseVector3(*s);
            if (const auto p = cn->GetValue("position")) mr.position = ConfigNode::ParseVector3(*p);
            if (const auto r = cn->GetValue("rotation")) mr.rotation = ConfigNode::ParseVector3(*r);
            std::string ih;
            if (cn->TryGetValue("iconHidden", ih) && cs::BoolParse(ih)) mr.iconHidden = true;
            for (const std::string& tv : cn->GetValues("texture")) {
                const auto a = dotnet::Split(tv, ":,;", true);
                if (a.size() != 2) {
                    log::Error("PartCompiler: Cannot replace texture as texture replacement string is invalid. Syntax is 'origTextureName newTextureURL'");
                    continue;
                }
                const std::string text = dotnet::Trim(a[0]), text2 = dotnet::Trim(a[1]);
                if (!db.GetTextureInfoIn(&*value, &text)) {
                    log::Error("PartCompiler: Cannot replace texture as cannot find texture '" + text + "' to replace");
                    continue;
                }
                const GameDatabase::TextureInfo* ti = db.GetTextureInfo(&text2);
                if (!ti) {
                    log::Error("PartCompiler: Cannot replace texture '" + text + "' as cannot find texture '" + text2 + "' to replace with");
                    continue;
                }
                mr.textureReplacements.push_back({text, ti->name, ti->file});
            }
            part.models.push_back(std::move(mr));
        }
        if (!any) {
            log::Error("PartCompiler: Model was not compiled correctly");
            return false;
        }
        return true;
    }
    const std::string dirUrl = cfg.parent()->parent()->url();
    const GameDatabase::ModelInfo* prefab = db.GetModelPrefabIn(dirUrl);
    if (!prefab) {
        log::Error("PartCompiler: Cannot clone model from '" + dirUrl + "' directory as model does not exist");
        return false;
    }
    PartModelRef mr;
    mr.url = prefab->name;
    mr.file = db.GetModelFile(prefab);
    if (!mr.file) log::Info("AddFileTime: File '" + dirUrl + "' is null!");
    else ap.AddFileTime(mr.file);
    part.models.push_back(std::move(mr));
    return true;
}

// ---- ParsePart (PartLoader.cs:642) -------------------------------------------------------------------------------------
std::unique_ptr<AvailablePart> PartLoader::ParsePart(const GameDatabase& db, io::UrlConfig& urlConfig, ConfigNode& node) {
    if (!node.HasValue("name")) {
        log::Error("PartCompiler: Part config requires value 'name' is defined");
        return nullptr;
    }
    if (!node.HasValue("module")) {
        log::Error("PartCompiler: Part config requires value 'module' is defined");
        return nullptr;
    }
    const std::string partName = dotnet::Replace(*node.GetValue("name"), '_', '.');
    const std::string value = *node.GetValue("module");
    auto ap = std::make_unique<AvailablePart>();
    InitAvailablePart(*ap);
    ap->name = partName;
    ap->partUrl = urlConfig.url();
    ap->partUrlConfig = &urlConfig;
    ap->partConfig = urlConfig.configPtr();
    ap->configFileFullName = urlConfig.parent()->fullPath();
    ap->AddFileTime(urlConfig.parent());
    float num = 1.25f;
    if (node.HasValue("rescaleFactor")) num = cs::FloatParse(*node.GetValue("rescaleFactor"));
    float num2 = f32store(1.0 * fx(num));
    ap->partPrefab = std::make_unique<PartPrefab>();
    if (!CompileModel(db, urlConfig, node, *ap)) {
        log::Error("PartCompiler: Cannot compile model");
        return nullptr;
    }
    // CreatePart (PartLoader.cs:1340): AssemblyLoader.GetClassByName(typeof(Part), module)
    if (value != "Part" && value != "CompoundPart") {
        log::Error("PartCompiler: Cannot find Part of type '" + value + "'");
        log::Error("PartCompiler: Error parsing config");
        return nullptr;
    }
    PartPrefab& part = *ap->partPrefab;
    part.className = value;
    part.gameObjectName = partName;
    part.fields.hasHeiarchyModel = true;
    part.fields.rescaleFactor = num;
    part.fields.partRendererBoundsIgnore = std::vector<std::string>();  // Part.Awake
    // AddComponent<Part>: Unity's serializer gives every [Serializable] class-typed field of a new component a
    // default-constructed instance (the C9 capture shows a default srfAttachNode on parts without node_attach)
    part.srfAttachNode = AttachNode();
    part.fields.attachRules = AttachRules();

    LoadAvailablePartPersistent(*ap, node);
    const auto ec = node.GetValue("entryCost");
    if (ec && !ec->empty()) {
        ap->entryCost = cs::IntParse(*ec);
        node.RemoveValues(std::string_view("entryCost"));
    }
    ap->name = partName;
    if (!ap->description.empty()) ap->description = dotnet::Replace(ap->description, "\\n", "\n");
    SanitizePartCosts(*ap, node);

    for (int i = 0; i < node.values().Count(); i++) {
        const ConfigNode::Value& v = *std::next(node.values().begin(), i);
        if (ApplyPartValue(part, v)) continue;
        const std::string& n = v.name;
        if (n == "texture" || n == "normalmap") {
            part.materialOverrides.emplace_back(n, v.value);
            continue;
        }
        if (n == "alphaCutoff" || n == "specPower" || n == "rimFalloff") {
            cs::FloatParse(v.value);  // float.Parse before ReplacePartMaterialValue
            part.materialOverrides.emplace_back(n, v.value);
            continue;
        }
        if (n == "vesselType") {
            part.fields.vesselType = static_cast<int32_t>(kEnum_VesselType.Parse(v.value));
            continue;
        }
        if (n == "scale" || n == "exportScale") {
            num2 = f32store(fx(cs::FloatParse(v.value)) * fx(num));
            part.fields.scaleFactor = f32store(fx(cs::FloatParse(v.value)) / fx(num));
            continue;
        }
        if (n == "iconCenter" || n == "name" || n == "mesh" || n == "subcategory" || n == "module") continue;
        if (dotnet::StartsWith(n, "node")) {
            AttachNode an;
            if (!ParseNodeValue(v, num2, true, an)) continue;
            if (an.nodeType == AttachNodeType::Surface && dotnet::Split(n, "_", false).at(1) == "attach") {
                if (part.srfAttachNode) log::Warning("PartLoader WARNING: surface attach node is defined more than once!");
                part.srfAttachNode = an;
            } else if (an.nodeType == AttachNodeType::Stack || an.nodeType == AttachNodeType::Dock) {
                const std::string kind = dotnet::Split(n, "_", false).at(1);
                if (kind == "stack" || kind == "dock") part.attachNodes.push_back(an);
            }
        } else if (dotnet::StartsWith(n, "fx")) {
            const auto a = dotnet::Split(v.value, ",", false);
            if (a.size() < 7) {
                log::Warning("PartLoader ERROR: bad effect node definition at " + n);
                continue;
            }
            ParseVector3Strict(a, 0);
            ParseVector3Strict(a, 3);
            part.fxDefinitions.emplace_back(n, v.value);
        } else if (dotnet::StartsWith(n, "sound")) {
            part.soundDefinitions.emplace_back(n, v.value);
        }
    }
    if (part.fields.skinMaxTemp <= 0.0) part.fields.skinMaxTemp = part.fields.maxTemp;
    for (int l = 0; l < node.nodes().Count(); l++) {
        ConfigNode& cn = *node.nodes().Shared(l);
        if (cn.name == "DRAG_CUBE") {
            if (!part.DragCubes.LoadCubes(cn)) log::Error("DragCube: Invalid cubes in part.cfg for part " + part.gameObjectName);
        } else if (cn.name == "EFFECTS") {
            part.effects.push_back(cn.CreateCopy());
        } else if (cn.name == "NODE") {
            // Part.AddAttachNode needs the model transform named by `transform` (position = transform.position):
            // recorded with the transform name; position/orientation come from the model (UE import).
            const auto nm = cn.GetValue("name");
            const auto tr = cn.GetValue("transform");
            if (!nm) throw Exception("Part: Cannot add attach node. Node requires a 'name' value.");
            if (!tr) throw Exception("Part: Cannot add attach node. Node requires a 'transform' value.");
            AttachNode an;
            an.id = *nm;
            an.nodeTransform = *tr;
            int32_t size = 1;
            if (cn.HasValue("size") && !cs::IntTryParse(*cn.GetValue("size"), size)) size = 1;
            an.size = size;
            an.attachMethod = (*nm == "srfAttach" || *nm == "attach") ? AttachNodeMethod::HINGE_JOINT : AttachNodeMethod::FIXED_JOINT;
            if (cn.HasValue("method")) {
                int64_t mv;
                if (kEnum_AttachNodeMethod.TryParse(*cn.GetValue("method"), mv, true)) an.attachMethod = static_cast<AttachNodeMethod>(mv);
            }
            bool cf = true, rg = false;
            cn.TryGetValue("crossfeed", cf);
            cn.TryGetValue("rigid", rg);
            an.ResourceXFeed = cf;
            an.rigid = rg;
            an.secondaryAxis = Vector3f{};
            log::Warning("PartLoader: NODE '" + *nm + "' on " + partName + " needs model transform '" + *tr + "'");
            part.attachNodes.push_back(an);
        } else if (cn.name == "RESOURCE") {
            AddResource(part, cn);
        } else if (cn.name == "MODULE") {
            AddModule(part, cn, false);
        }
    }
    if (part.fields.vesselType > 2) AddModule(part, "ModuleTripLogger")->addedInCode = true;  // > VesselType.Unknown
    if (node.HasNode("INTERNAL")) node.GetNode("INTERNAL")->CopyTo(*ap->internalConfig);
    if (!node.TryGetValue("showVesselNaming", ap->showVesselNaming)) {
        if (HasModuleClass(part, "ModuleCommand")) ap->showVesselNaming = true;
        else if (HasModuleClass(part, "KerbalSeat")) ap->showVesselNaming = true;
    }
    float value4 = options_.partMassMin;
    node.TryGetValue("minimumMass", value4);
    ap->SetMinimumMass(value4);
    float value5 = options_.partRBMassMin;
    node.TryGetValue("minimumRBMass", value5);
    ap->SetMinimumRBMass(value5);
    node.TryGetValue("minimumCost", ap->minimumCost);
    ap->tags = ToLowerAscii(ap->tags) + " " + GeneratePartAutoTags(*ap);
    return ap;
}

// ---- prefab parts (PartLoader.cs:511) ----------------------------------------------------------------------------------
void PartLoader::ApplyPrefabOverlay(const GameDatabase& /*db*/, AvailablePart& ap, io::UrlConfig& urlConfig) {
    PartPrefab& part = *ap.partPrefab;
    ap.prefabCompiled = true;  // partPrefab.Awake()
    if (!part.fields.partRendererBoundsIgnore) part.fields.partRendererBoundsIgnore = std::vector<std::string>();
    ConfigNode& cfg = urlConfig.config();
    for (const auto& v : cfg.values()) ApplyPartValue(part, v);
    for (int j = 0; j < cfg.nodes().Count(); j++) {
        ConfigNode& cn = *cfg.nodes().Shared(j);
        if (cn.name == "EFFECTS") {
            part.effects.push_back(cn.CreateCopy());
        } else if (cn.name == "DRAG_CUBE") {
            part.DragCubes.LoadCubes(cn);
        } else if (cn.name == "RESOURCE") {
            AddResource(part, cn);
        } else if (cn.name == "MODULE") {
            const auto ov = cn.GetValue("__OVERLOAD");
            if (ov && !ov->empty() && cs::BoolParse(*ov)) {
                // Modules[name].Load(node): the module comes with the Unity prefab; kspcore records it here
                const std::string nm = cn.GetValue("name").value_or("");
                PartModuleData* m = nullptr;
                for (PartModuleData& pm : part.Modules)
                    if (pm.moduleName == nm) m = &pm;
                if (!m) {
                    PartModuleData pm;
                    pm.moduleName = nm;
                    pm.moduleClass = GetPartModuleClass(nm);
                    part.Modules.push_back(std::move(pm));
                    m = &part.Modules.back();
                }
                m->cfgFields.clear();
                LoadModuleData(*m, cn, &part);
            } else {
                AddModule(part, cn, true);
            }
        }
    }
    for (const auto& v : cfg.values())
        if (dotnet::StartsWith(v.name, "sound")) part.soundDefinitions.emplace_back(v.name, v.value);
}

// ---- CompilePartInfo (PartLoader.cs:1169), data side -------------------------------------------------------------------
void PartLoader::CompilePartInfo(AvailablePart& ap) {
    PartPrefab& part = *ap.partPrefab;
    bool flag = false;
    for (const PartModuleData& m : part.Modules) {
        if (!m.moduleClass) continue;
        if (m.moduleClass->Is("ModulePartVariants")) part.hasVariantsModule = true;  // GetModuleTitle() == "PartVariants"
        if (m.moduleClass->Is("ModuleCommand")) flag = true;
    }
    if (part.fields.applyKerbalMassModification && part.fields.CrewCapacity > 0) {
        const float per = flag ? options_.perCommandSeatReduction : options_.perSeatReduction;
        part.fields.partSeatMassReduction = f32store(fx(per) * fx(static_cast<float>(part.fields.CrewCapacity)));
        part.fields.mass = f32store(fx(part.fields.mass) - fx(part.fields.partSeatMassReduction));
    }
}

// ---- assets ------------------------------------------------------------------------------------------------------------
void PartLoader::ResolveAssets(const GameDatabase& db, AvailablePart& ap) {
    const std::string& gd = db.gameDataPath();
    ap.assets.clear();
    const std::string cfgRel = GameDataRel(ap.configFileFullName, gd);
    ap.assets.push_back({"cfg", "partUrl", ap.partUrl, cfgRel, ap.configFileFullName.empty() ? "code" : "GameData"});
    if (ap.partConfig) {
        const std::vector<ConfigNode*> nodes = ap.partConfig->GetNodes("MODEL");
        if (!nodes.empty()) {
            for (size_t i = 0; i < nodes.size(); i++) {
                const auto url = nodes[i]->GetValue("model");
                io::UrlFile* f = url ? db.GetModelFile(*url) : nullptr;
                const std::string file = f ? GameDataRel(f->fullPath(), gd) : std::string();
                ap.assets.push_back({"model", "MODEL[" + std::to_string(i) + "]", url.value_or(""), file,
                                     f ? "GameData" : "unresolved"});
                for (const std::string& tv : nodes[i]->GetValues("texture")) {
                    const auto a = dotnet::Split(tv, ":,;", true);
                    if (a.size() != 2) continue;
                    const std::string orig = dotnet::Trim(a[0]), nu = dotnet::Trim(a[1]);
                    const GameDatabase::TextureInfo* ti = db.GetTextureInfo(&nu);
                    const std::string tf = (ti && ti->file) ? GameDataRel(ti->file->fullPath(), gd) : std::string();
                    ap.assets.push_back({"texture_replacement", "MODEL[" + std::to_string(i) + "] " + orig, nu, tf,
                                         tf.empty() ? "unresolved" : "GameData"});
                }
            }
        } else if (ap.partUrlConfig && ap.partUrlConfig->parent() && ap.partUrlConfig->parent()->parent()) {
            const std::string dirUrl = ap.partUrlConfig->parent()->parent()->url();
            const GameDatabase::ModelInfo* prefab = db.GetModelPrefabIn(dirUrl);
            io::UrlFile* f = prefab ? db.GetModelFile(prefab) : nullptr;
            const std::string file = f ? GameDataRel(f->fullPath(), gd) : std::string();
            ap.assets.push_back({"model", "mesh (first model in " + dirUrl + ")", prefab ? prefab->name : std::string(),
                                 file, f ? "GameData" : "unresolved"});
        }
    }
    if (ap.internalConfig && ap.internalConfig->HasValue("name"))
        ap.assets.push_back({"internal", "INTERNAL.name", *ap.internalConfig->GetValue("name"), "", "internal"});
}

// ---- CompileAll / CompileParts (PartLoader.cs:161 / 467) ---------------------------------------------------------------
void PartLoader::CompileAll(const GameDatabase& db, const Options& options) {
    options_ = options;
    if (options_.usePhysicsGlobalsInstance && PhysicsGlobals::Instance()) {
        options_.perSeatReduction = PhysicsGlobals::PerSeatReduction();
        options_.perCommandSeatReduction = PhysicsGlobals::PerCommandSeatReduction();
        options_.partMassMin = PhysicsGlobals::PartMassMin();
        options_.partRBMassMin = PhysicsGlobals::PartRBMassMin();
    }
    loadedParts.clear();
    log.clear();
    cubesNeedRendering.clear();
    // the prefab parts (PartLoader.parts, Unity-serialized)
    for (const std::string& n : options_.codeBuiltParts) {
        auto ap = std::make_unique<AvailablePart>();
        InitAvailablePart(*ap);
        ap->name = n;
        ap->codeBuilt = true;
        ap->prefabCompiled = false;
        ap->partPrefab = std::make_unique<PartPrefab>();
        ap->partPrefab->gameObjectName = n;
        loadedParts.push_back(std::move(ap));
    }
    InitializePartDatabase();
    for (io::UrlConfig* urlConfig : db.GetConfigs("PART")) {
        log.push_back("PartLoader: Compiling Part '" + urlConfig->url() + "'");
        const auto nm = urlConfig->config().GetValue("name");
        int num = -1;
        for (size_t i = 0; nm && i < options_.codeBuiltParts.size(); i++)
            if (*nm == options_.codeBuiltParts[i]) num = static_cast<int>(i);
        if (num >= 0) {
            try {
                ApplyPrefabOverlay(db, *loadedParts[static_cast<size_t>(num)], *urlConfig);
            } catch (const std::exception& e) {
                log.push_back(std::string("PartLoader: exception applying prefab part config: ") + e.what());
                log::Error(log.back());
            }
            continue;
        }
        std::unique_ptr<AvailablePart> ap;
        try {
            ap = ParsePart(db, *urlConfig, urlConfig->config());
        } catch (const std::exception& e) {
            log.push_back(std::string("PartLoader: Encountered exception during compilation. ") + e.what());
            log::Error(log.back());
            ap.reset();
        }
        if (!ap) {
            log.push_back("PartCompiler: Cannot compile part");
            log::Error(log.back());
            continue;
        }
        PartPrefab& p = *ap->partPrefab;
        if (!p.DragCubes.Cubes().empty()) {
            SetDatabaseConfig(*ap, p.DragCubes.SaveCubes());
        } else {
            // DragCubeSystem.SetupDragCubeCoroutine (DragCubeSystem.cs:651)
            ConfigNode* dc = GetDatabaseConfig(*ap, "DRAG_CUBE");
            if (!dc || !p.DragCubes.LoadCubes(*dc)) {
                log.push_back("[DragCubeSystem]: Drag cubes not found or cannot be read for part " + ap->name +
                              ". Generating New drag cubes.");
                cubesNeedRendering.push_back(ap->name);
            }
        }
        CompilePartInfo(*ap);
        ResolveAssets(db, *ap);
        loadedParts.push_back(std::move(ap));
    }
    for (auto& ap : loadedParts) {
        if (ap->codeBuilt) {
            ap->assets.push_back({"cfg", "partUrl", "", "", "code"});
            // prefab parts never go through CompilePartInfo (PartLoader.cs:511-571)
            if (!ap->prefabCompiled) ap->loadError = "NullReferenceException: Object reference not set to an instance of an object";
        }
    }
}

}  // namespace ksp
