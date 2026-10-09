#include "ksp/resources/part_resource_library.hpp"

#include "ksp/core/log.hpp"
#include "ksp/database/game_database.hpp"
#include "ksp/dotnet/utf16.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/parts/ksp_enums.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

namespace {
PartResourceLibrary* g_library = nullptr;
}

// mono/mcs/class/corlib/ReferenceSources/String.cs GetHashCode (Mono 5.x)
int32_t MonoStringHash(std::string_view s) {
    const std::u16string u = dotnet::ToUtf16(s);
    uint32_t h = 0;
    size_t i = 0;
    const size_t n = u.size();
    if (n > 0) {
        for (; i + 1 < n; i += 2) {
            h = (h << 5) - h + u[i];
            h = (h << 5) - h + u[i + 1];
        }
        if (i < n) h = (h << 5) - h + u[i];
    }
    return static_cast<int32_t>(h);
}

// ---- PartResourceDrainDefinition.cs ------------------------------------------------------------------------------------
void PartResourceDrainDefinition::Load(const ConfigNode& node) {
    if (node.HasValue("isDrainable")) isDrainable_ = cs::BoolParse(*node.GetValue("isDrainable"));
    if (node.HasValue("showDrainFX")) showDrainFX_ = cs::BoolParse(*node.GetValue("showDrainFX"));
    if (node.HasValue("drainFXPriority")) drainFXPriority_ = cs::IntParse(*node.GetValue("drainFXPriority"));
    if (node.HasValue("drainForceISP")) drainForceISP_ = cs::FloatParse(*node.GetValue("drainForceISP"));
    if (node.HasValue("drainFXDefinition")) drainFXDefinition_ = *node.GetValue("drainFXDefinition");
}

void PartResourceDrainDefinition::Save(ConfigNode& node) const {
    node.AddValue("isDrainable", isDrainable_);
    node.AddValue("showDrainFX", showDrainFX_);
    node.AddValue("drainFXPriority", drainFXPriority_);
    node.AddValue("drainForceISP", drainForceISP_);
    node.AddValue("drainFXDefinition", drainFXDefinition_);
}

// ---- PartResourceDefinition.cs -----------------------------------------------------------------------------------------
PartResourceDefinition::PartResourceDefinition(std::string_view name) : name_(name), id_(MonoStringHash(name)) {}

PartResourceDefinition::PartResourceDefinition(std::string_view name, const Color& color)
    : name_(name), id_(MonoStringHash(name)), color_(color) {}

// PartResourceDefinition.cs:99
void PartResourceDefinition::Load(const ConfigNode& node) {
    const auto nm = node.GetValue("name");
    if (!nm) throw NullReferenceException("Object reference not set to an instance of an object");  // name.GetHashCode()
    name_ = *nm;
    id_ = MonoStringHash(name_);
    if (node.HasValue("displayName"))
        displayName_ = *node.GetValue("displayName");
    else
        displayName_ = KSPUtil::PrintModuleName(name_);
    if (node.HasValue("abbreviation"))
        abbreviation_ = *node.GetValue("abbreviation");
    else
        abbreviation_ = GetShortName();
    if (node.HasValue("density")) density_ = cs::FloatParse(*node.GetValue("density"));
    if (node.HasValue("volume")) volume_ = cs::FloatParse(*node.GetValue("volume"));
    if (node.HasValue("unitCost")) unitCost_ = cs::FloatParse(*node.GetValue("unitCost"));
    if (node.HasValue("hsp")) specificHeatCapacity_ = cs::FloatParse(*node.GetValue("hsp"));
    if (node.HasValue("isTweakable")) isTweakable_ = cs::BoolParse(*node.GetValue("isTweakable"));
    if (node.HasValue("isVisible")) isVisible_ = cs::BoolParse(*node.GetValue("isVisible"));
    if (node.HasValue("flowMode"))
        resourceFlowMode_ = static_cast<int32_t>(kEnum_ResourceFlowMode.Parse(*node.GetValue("flowMode")));
    if (node.HasValue("transfer"))
        resourceTransferMode_ = static_cast<int32_t>(kEnum_ResourceTransferMode.Parse(*node.GetValue("transfer")));
    if (node.HasValue("color")) color_ = ConfigNode::ParseColor(*node.GetValue("color"));
    if (node.HasNode("RESOURCE_DRAIN_DEFINITION")) drain_.Load(*node.GetNode("RESOURCE_DRAIN_DEFINITION"));
    config_ = ConfigNode::Create();
    node.CopyTo(*config_);
}

// PartResourceDefinition.cs:163 (Substring throws when the text is shorter than `length`)
std::string PartResourceDefinition::GetShortName(int length) const {
    const std::string& s = !abbreviation_.empty() ? abbreviation_ : displayName_;
    const std::u16string u = dotnet::ToUtf16(s);
    if (length < 0 || static_cast<size_t>(length) > u.size())
        throw std::out_of_range("Index and length must refer to a location within the string.");
    return dotnet::FromUtf16(std::u16string_view(u).substr(0, static_cast<size_t>(length)));
}

// PartResourceDefinition.cs:172
void PartResourceDefinition::Save(ConfigNode& node) const {
    node.AddValue("name", name_);
    node.AddValue("density", density_);
    node.AddValue("volume", volume_);
    node.AddValue("unitCost", unitCost_);
    node.AddValue("hsp", specificHeatCapacity_);
    node.AddValue("flowMode", kEnum_ResourceFlowMode.ToString(resourceFlowMode_));
    node.AddValue("transfer", kEnum_ResourceTransferMode.ToString(resourceTransferMode_));
    ConfigNode* n2 = node.AddNode("RESOURCE_DRAIN_DEFINITION");
    drain_.Save(*n2);
    if (!(color_.r == 1.0f && color_.g == 1.0f && color_.b == 1.0f && color_.a == 1.0f))
        node.AddValue("color", ConfigNode::WriteColor(color_));
}

// ---- PartResourceDefinitionList.cs -------------------------------------------------------------------------------------
const PartResourceDefinition* PartResourceDefinitionList::operator[](std::string_view name) const {
    return (*this)[MonoStringHash(name)];
}

const PartResourceDefinition* PartResourceDefinitionList::operator[](int32_t id) const {
    for (const auto& d : defs_)
        if (d->id() == id) return d.get();
    return nullptr;
}

bool PartResourceDefinitionList::Contains(std::string_view name) const { return (*this)[name] != nullptr; }

// PartResourceDefinitionList.cs:45
const PartResourceDefinition* PartResourceDefinitionList::Add(const ConfigNode& node) {
    if (!node.HasValue("name")) {
        log::Warning("Config has no name field");
        return nullptr;
    }
    const std::string value = *node.GetValue("name");
    if (Contains(value)) {
        log::Warning("PartResourceDefinition list already contains definition for '" + value + "'");
        return nullptr;
    }
    auto def = std::make_unique<PartResourceDefinition>();
    def->Load(node);
    // dict.Add(id, def): a hash collision with a different name throws ArgumentException in C#
    if ((*this)[def->id()] != nullptr) throw ArgumentException("An item with the same key has already been added.");
    defs_.push_back(std::move(def));
    return defs_.back().get();
}

// ---- PartResourceLibrary.cs --------------------------------------------------------------------------------------------
PartResourceLibrary::PartResourceLibrary() { g_library = this; }
PartResourceLibrary::~PartResourceLibrary() {
    if (g_library == this) g_library = nullptr;
}
PartResourceLibrary* PartResourceLibrary::Instance() { return g_library; }

int32_t PartResourceLibrary::ElectricityHashcode() { return MonoStringHash("ElectricCharge"); }

// PartResourceLibrary.cs:44
void PartResourceLibrary::LoadDefinitions(const GameDatabase& db) {
    resourceDefinitions.Clear();
    for (ConfigNode* n : db.GetConfigNodes("RESOURCE_DEFINITION")) {
        resourceDefinitions.Add(*n);
        if (n->HasValue("name"))
            log::Info("Resource " + *n->GetValue("name") + " added to database");
        else
            log::Info("Resource " + n->name + " added to database");
    }
}

int32_t PartResourceLibrary::GetDefaultFlowMode(std::string_view resourceName) {
    return GetDefaultFlowMode(MonoStringHash(resourceName));
}

// PartResourceLibrary.cs:77
int32_t PartResourceLibrary::GetDefaultFlowMode(int32_t resourceID) {
    const PartResourceDefinition* d = Instance() ? Instance()->GetDefinition(resourceID) : nullptr;
    if (!d) {
        log::Error("Resource System Error: Requested resource (id: " + std::to_string(resourceID) + ") does not exist");
        return 8;  // ResourceFlowMode.NULL
    }
    return d->resourceFlowMode();
}

}  // namespace ksp
