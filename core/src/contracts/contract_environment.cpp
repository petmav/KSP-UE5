// WP-37: ContractsEnvironment seams and ContractDefsLite (the Contracts.cfg values the WP-37 files read).
#include "ksp/contracts/contract_environment.hpp"

#include <chrono>

#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/fineprint/contract_defs.hpp"

namespace ksp::Contracts {

// ---- ContractDefsLite ------------------------------------------------------------------------------------------------

namespace {
ConfigNode::NodePtr& DefsRoot() {
    static ConfigNode::NodePtr root;
    return root;
}
}  // namespace

// C++ helper (see FinePrint/ContractDefs.cs:2072) LoadConfig (the node handed to it is the "Contracts" node of the GameDatabase). WP-38 slice 3:
// the typed members of FinePrint::ContractDefs are reset to the C# initialisers and LoadConfig runs over the node; the accessors below answer from
// them.
void ContractDefsLite::Load(const ConfigNode& contractsNode) {
    DefsRoot() = contractsNode.CreateCopy();
    FinePrint::ContractDefs::ResetToDefaults();
    FinePrint::ContractDefs::config = DefsRoot();
    FinePrint::ContractDefs::LoadConfig();
}

void ContractDefsLite::Load(const std::string& contractsCfgPath) {
    ConfigNode::NodePtr file = ConfigNode::Load(contractsCfgPath);
    if (!file) throw KeyNotFoundException("Contracts.cfg not found: " + contractsCfgPath);
    ConfigNode* node = file->GetNode("Contracts");
    if (node == nullptr) throw KeyNotFoundException("Contracts.cfg has no Contracts node: " + contractsCfgPath);
    Load(*node);
}

void ContractDefsLite::Clear() {
    DefsRoot().reset();
    FinePrint::ContractDefs::config.reset();
    FinePrint::ContractDefs::ResetToDefaults();
}
bool ContractDefsLite::Loaded() { return static_cast<bool>(DefsRoot()); }

// the typed member of FinePrint::ContractDefs behind (section, sub, key) when it has the requested C# type, else nullptr
namespace {
template <class T>
T* Typed(const std::string& section, const std::string& sub, const std::string& key) {
    if (!DefsRoot()) return nullptr;
    const FinePrint::ContractDefs::Entry* e = FinePrint::ContractDefs::Find(section, sub, key);
    if (e == nullptr) return nullptr;
    T* const* p = std::get_if<T*>(&e->member);
    return p != nullptr ? *p : nullptr;
}
}  // namespace

ConfigNode* ContractDefsLite::Node(const std::string& section, const std::string& sub) {
    if (!DefsRoot()) throw NullReferenceException("ContractDefs is not loaded");
    ConfigNode* n = DefsRoot().get();
    if (!section.empty()) {
        n = n->GetNode(section);
        if (n == nullptr) return nullptr;
    }
    if (!sub.empty()) n = n->GetNode(sub);
    return n;
}

bool ContractDefsLite::Has(const std::string& section, const std::string& sub, const std::string& key) {
    ConfigNode* n = Node(section, sub);
    return n != nullptr && n->HasValue(key);
}

std::string ContractDefsLite::S(const std::string& section, const std::string& sub, const std::string& key) {
    if (const std::string* p = Typed<std::string>(section, sub, key)) return *p;
    ConfigNode* n = Node(section, sub);
    if (n == nullptr || !n->HasValue(key)) throw KeyNotFoundException("ContractDefs " + section + "/" + sub + "/" + key);
    return *n->GetValue(key);
}

float ContractDefsLite::F(const std::string& section, const std::string& sub, const std::string& key) {
    if (const float* p = Typed<float>(section, sub, key)) return *p;
    return dotnet::ParseSingle(S(section, sub, key));
}
double ContractDefsLite::D(const std::string& section, const std::string& sub, const std::string& key) {
    if (const double* p = Typed<double>(section, sub, key)) return *p;
    return dotnet::ParseDouble(S(section, sub, key));
}
int ContractDefsLite::I(const std::string& section, const std::string& sub, const std::string& key) {
    if (const int* p = Typed<int>(section, sub, key)) return *p;
    int v = 0;
    const std::string s = S(section, sub, key);
    if (!dotnet::TryParseInt32(s, v)) throw dotnet::FormatException("Input string was not in a correct format.");
    return v;
}
bool ContractDefsLite::B(const std::string& section, const std::string& sub, const std::string& key) {
    if (const bool* p = Typed<bool>(section, sub, key)) return *p;
    bool v = false;
    if (!dotnet::TryParseBool(S(section, sub, key), v)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
    return v;
}

// ---- ContractsEnvironment ---------------------------------------------------------------------------------------------

ContractsEnvironment& ContractsEnvironment::Current() {
    static ContractsEnvironment env;
    return env;
}

void ContractsEnvironment::Reset() { Current() = ContractsEnvironment{}; }

double ContractsEnvironment::GameTime() const {
    if (gameTime) return gameTime();
    if (Planetarium::fetch != nullptr) return Planetarium::fetch->time;
    return 0.0;
}

Guid ContractsEnvironment::NewGuid() {
    if (newGuid) return newGuid();
    Guid g;
    uint64_t v = ++guidCounter_;
    for (int i = 0; i < 8; ++i) g.bytes[i] = static_cast<uint8_t>(v >> (8 * i));
    g.bytes[15] = 0x4B;
    return g;
}

int64_t ContractsEnvironment::NowTicks() const {
    if (nowTicks) return nowTicks();
    return static_cast<int64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
}

int32_t ContractsEnvironment::NewRandomSeed() const {
    if (newRandomSeed) return newRandomSeed();
    return static_cast<int32_t>(std::chrono::steady_clock::now().time_since_epoch().count() & 0x7FFFFFFF);
}

}  // namespace ksp::Contracts
