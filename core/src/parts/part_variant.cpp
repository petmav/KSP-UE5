// WP-13: PartVariant (PartVariant.cs). The variant's Materials (List<Material>) and the TEXTURE commands applied to them are
// model data: the TEXTURE nodes are kept in `textures` for the UE import. Everything else is ported.
#include "ksp/core/log.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/cs_parse.hpp"

namespace ksp {

// PartVariant.cs:191 PartVariant(string name, string displayName, List<AttachNode> attachNodes, AttachNode srfAttachNode)
// (and :186, which passes a null srf node)
PartVariant::PartVariant(std::string name_, std::string displayName_, const std::vector<AttachNode>& attachNodes_,
                         std::shared_ptr<AttachNode> srfAttachNode_)
    : name(std::move(name_)), displayName(std::move(displayName_)) {
    srfAttachNode = std::move(srfAttachNode_);
    mass = 0.0f;
    cost = 0.0f;
    for (const AttachNode& n : attachNodes_) attachNodes.push_back(AttachNode::Clone(n));
}

// PartVariant.cs:213 PartVariant(PartVariant baseVariant) (modelTransform and the Material copies are the model's)
PartVariant PartVariant::FromBase(const PartVariant& baseVariant) {
    PartVariant v;
    v.name = baseVariant.name;
    v.displayName = baseVariant.displayName;
    v.primaryColor = baseVariant.primaryColor;
    v.secondaryColor = baseVariant.secondaryColor;
    v.sizeGroup = baseVariant.sizeGroup;
    v.mass = baseVariant.mass;
    v.cost = baseVariant.cost;
    v.srfAttachNode = baseVariant.srfAttachNode;  // shared reference (sic)
    for (const AttachNode& n : baseVariant.attachNodes) v.attachNodes.push_back(AttachNode::Clone(n));
    v.disabledAnimations = baseVariant.disabledAnimations;
    v.disabledEvents = baseVariant.disabledEvents;
    v.themeName = baseVariant.themeName;
    v.attachRules = baseVariant.attachRules;
    return v;
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
            // ParseNode (:515): null unless the name starts with "node"; fewer than 6 values logs and returns null
            if (!dotnet::StartsWith(v.name, "node")) continue;
            AttachNode an;
            if (!ParseNodeValue(v, 1.0f, false, an)) continue;
            if (an.id != "attach" && an.id != "srfAttach") {
                const int idx = GetAttachedNodeIndex(an.id);
                if (idx != -1) attachNodes[static_cast<size_t>(idx)] = an;
            } else {
                srfAttachNode = std::make_shared<AttachNode>(an);
            }
        }
    }
    for (ConfigNode* t : node.GetNodes("TEXTURE")) textures.push_back(t->CreateCopy());  // PartMaterialInfo.Load + UpdateMaterial
    if (const auto da = node.GetValue("disabledAnimations"))
        for (const std::string& s : dotnet::Split(*da, ",", false)) disabledAnimations.push_back(s);
    if (const auto de = node.GetValue("disabledEvents"))
        for (const std::string& s : dotnet::Split(*de, ",", false)) disabledEvents.push_back(s);
    std::string tn;
    if (node.TryGetValue("themeName", tn)) themeName = tn;
    std::string ar;
    if (node.TryGetValue("attachRules", ar)) attachRules = AttachRules::Parse(ar);
}

// PartVariant.cs:358
int PartVariant::GetAttachedNodeIndex(const std::string& id) const {
    for (size_t i = 0; i < attachNodes.size(); i++)
        if (attachNodes[i].id == id) return static_cast<int>(i);
    return -1;
}

// PartVariant.cs:502
std::string PartVariant::GetExtraInfoValue(const std::string& name_) const {
    std::string result;
    for (const PartRawInfo& e : extraInfo)
        if (e.Name() == name_) result = e.Value();
    return result;
}

// PartVariant.cs:471 UpdateModel: (GameObject name, active) for every GAMEOBJECTS entry
std::vector<std::pair<std::string, bool>> PartVariant::ModelVisibility() const {
    std::vector<std::pair<std::string, bool>> r;
    for (const PartGameObjectInfo& g : infoGameObjects) r.emplace_back(g.Name(), g.Status());
    return r;
}

}  // namespace ksp
