// WP-38 slice 3: FinePrint.ContractDefs (the hand-written part: registry lookup, Awake, LoadDefinitions). The writer, the reader and the
// typed members are generated (contract_defs_gen.cpp, contract_defs_members.inc).
#include "ksp/fineprint/contract_defs.hpp"

#include "ksp/localization/localizer.hpp"

namespace ksp::FinePrint {

using KSP::Localization::Localizer;

const ContractDefs::Entry* ContractDefs::Find(const std::string& section, const std::string& sub, const std::string& key) {
    for (const Entry& e : Registry()) {
        if (e.serenityOnly) continue;
        if (section == e.section && sub == e.sub && key == e.key) return &e;
    }
    return nullptr;
}

// FinePrint/ContractDefs.cs:1387
void ContractDefs::LoadDefinitions() {
    PolarOrbitName = Localizer::Format("#autoLOC_7001202");
    EquatorialOrbitName = Localizer::Format("#autoLOC_7001201");
    TundraOrbitName = Localizer::Format("#autoLOC_7001204");
    SunStationaryName = Localizer::Format("#autoLOC_7001205");
    HomeStationaryName = Localizer::Format("#autoLOC_7001206");
    OtherStationaryName = Localizer::Format("#autoLOC_7001207");
    SunSynchronousName = Localizer::Format("#autoLOC_7001208");
    HomeSynchronousName = Localizer::Format("#autoLOC_7001209");
    OtherSynchronousName = Localizer::Format("#autoLOC_7001210");
    MolniyaName = Localizer::Format("#autoLOC_7001203");
}

// FinePrint/ContractDefs.cs:1401
void ContractDefs::Awake(const std::vector<const ConfigNode*>& contractsNodes, bool cfgFileExists) {
    config = std::make_shared<ConfigNode>();
    if (!cfgFileExists) CreateDefaultConfig();
    int num = static_cast<int>(contractsNodes.size());
    while (num-- > 0) contractsNodes[static_cast<size_t>(num)]->CopyTo(*config);
    LoadConfig();
    LoadDefinitions();
}

}  // namespace ksp::FinePrint
