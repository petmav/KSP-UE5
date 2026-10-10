// WP-37: Contracts.Templates.RecoverAsset (Assembly-CSharp/Contracts/Templates/RecoverAsset.cs).
//
// Seams (TemplateEnvironment): the vessel spawning of OnAccepted (GenerateLandRecovery / GenerateOrbitalRecovery and the
// ProtoVessel node builders), Cleanup's vessel destruction, SystemUtilities.ExpungeKerbal / CheckTouristRecoveryContractKerbals,
// ResearchAndDevelopment.ResearchedValidContractObjectives, VesselUtilities.EstimatePartSize and the InternalModel seats.
#include <algorithm>
#include <cctype>

#include "ksp/career/format.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/agents.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/contracts/templates.hpp"
#include "ksp/contracts/text_gen.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/kerbals/crew_generator.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/science/science.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/unity/random.hpp"

namespace ksp::Contracts::Templates {

using KSP::Localization::Localizer;
namespace PU = Lite::ProgressUtilities;
namespace SU = Lite::StringUtilities;
using Lite::MannedStatus;
using Lite::ProgressType;

namespace {
Game* CurrentGame() { return SaveEnvironment::Current().currentGame; }
std::string Fmt(const char* tag, const std::vector<std::string>& a) { return Localizer::Format(tag, a); }
std::string ToLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
// LingoonaGrammarExtensions.cs:50 LocalizeName(string, ProtoCrewMember.Gender) -> LocalizeNoun(gender, isProperNoun: true) (as ProtoCrewMember.nameWithGender)
std::string LocalizeName(std::string input, ProtoCrewMember_Gender gender) {
    const size_t pos = input.rfind('^');
    if (pos != std::string::npos) input.erase(pos);
    return input + (gender == ProtoCrewMember_Gender::Male ? "^M" : "^F");
}
float Def(const char* sub, const char* key) { return ContractDefsLite::F("Recovery", sub, key); }
}  // namespace

void TemplateEnvironment::Reset() {
    researchedValidContractObjectives = nullptr;
    checkTouristRecoveryContractKerbals = nullptr;
    expungeKerbal = nullptr;
    alphaNumeric.clear();
    spawnRecoveryVessels = nullptr;
    destroyUnownedVesselsWithPart = nullptr;
    discoverVessel = nullptr;
    estimatePartSize = nullptr;
    partHasSeats = nullptr;
    makeVesselSystemsParameter = nullptr;
    makeVesselDestinationParameter = nullptr;
    makeLocationAndSituationParameter = nullptr;
    grandTourAccepted = nullptr;
}

// RecoverAsset.cs:120
RecoverAsset::RecoverAsset() {
    static const char* kerbalTags[10] = {"#autoLOC_6100036", "#autoLOC_6100037", "#autoLOC_6100038", "#autoLOC_6100039", "#autoLOC_6100040",
                                         "#autoLOC_6100041", "#autoLOC_6100042", "#autoLOC_6100043", "#autoLOC_6100044", "#autoLOC_6100045"};
    static const char* partTags[6] = {"#autoLOC_6100046", "#autoLOC_6100047", "#autoLOC_6100048", "#autoLOC_6100049", "#autoLOC_6100050", "#autoLOC_6100051"};
    for (int i = 0; i < 10; ++i) RecoveringKerbalDescriptionStrings[i] = Localizer::Format(kerbalTags[i]);
    for (int i = 0; i < 6; ++i) DescriptionStrings[i] = Localizer::Format(partTags[i]);
}

// RecoverAsset.cs:57
bool RecoverAsset::PartIsValid() const {
    if (partName != "kerbalEVA" && partName != "kerbalEVAfemale" && partName != "kerbalEVAVintage" && partName != "kerbalEVAfemaleVintage") {
        const auto& get = SaveEnvironment::Current().getPartInfoByName;
        return get && get(partName) != nullptr;
    }
    return false;
}

// RecoverAsset.cs:93
std::string RecoverAsset::VesselDescription() {
    KSPRandom kSPRandom(MissionSeed());
    if (RecoveringKerbal()) return RecoveringKerbalDescriptionStrings[kSPRandom.Next(0, 10)];
    return DescriptionStrings[kSPRandom.Next(0, 6)];
}

// RecoverAsset.cs:106
std::string RecoverAsset::VesselName() {
    if (!RecoveringKerbal()) return Fmt("#autoLOC_6100053", {VesselDescription(), SU::AlphaNumericDesignation(MissionSeed())});
    return Fmt("#autoLOC_6100052", {SU::ShortKerbalName(recoveryKerbal->nameWithGender()), VesselDescription()});
}

// RecoverAsset.cs:118
std::shared_ptr<ProtoCrewMember> RecoverAsset::ProtoKerbal() { return (*CurrentGame()->CrewRoster)[recoveryKerbal->name().value_or(std::string())]; }

// RecoverAsset.cs:140
bool RecoverAsset::Generate() {
    int num = 0;
    int num2 = 0;
    std::vector<RecoverAsset*> currentContracts = ContractSystem::Instance->GetCurrentContracts<RecoverAsset>();
    int num3 = static_cast<int>(currentContracts.size());
    while (num3-- > 0) {
        switch (currentContracts[static_cast<size_t>(num3)]->ContractState()) {
            case State::Active: num2++; break;
            case State::Offered: num++; break;
            default: break;
        }
    }
    if (num < ContractDefsLite::I("Recovery", "", "MaximumAvailable") && num2 < ContractDefsLite::I("Recovery", "", "MaximumActive")) {
        if (!Initialize()) return false;
        const float multiplier = GetMultiplier();
        if (RecoveringKerbal()) {
            bool flag = false;
            while (!flag) {
                recoveryKerbal = CurrentGame()->CrewRoster->GetNewKerbal(ProtoCrewMember_KerbalType::Unowned);
                const bool tourist = TemplateEnvironment::checkTouristRecoveryContractKerbals &&
                                     TemplateEnvironment::checkTouristRecoveryContractKerbals(CrewGenerator::RemoveLastName(recoveryKerbal->name().value_or(std::string())));
                if (!tourist) flag = true;
            }
            recoveryKerbal->set_rosterStatus(ProtoCrewMember_RosterStatus::Assigned);
            recoveryKerbal->seatIdx = -1;
            auto* obj = static_cast<Parameters::AcquireCrew*>(
                AddParameter(std::make_unique<Parameters::AcquireCrew>(Fmt("#autoLOC_274219", {recoveryKerbal->nameWithGender()}))));
            obj->SetFunds(f32store(fx(Def("Funds", "BaseReward")) * fx(multiplier) * 0.1f), targetBody);
            obj->SetReputation(f32store(fx(Def("Reputation", "BaseReward")) * fx(multiplier) * 0.3f));
        }
        if (RecoveringPart()) {
            auto* obj2 = static_cast<Parameters::AcquirePart*>(AddParameter(std::make_unique<Parameters::AcquirePart>(Fmt("#autoLOC_274226", {VesselName()}))));
            obj2->SetFunds(f32store(fx(Def("Funds", "BaseReward")) * fx(multiplier) * 0.1f), targetBody);
            obj2->SetReputation(f32store(fx(Def("Reputation", "BaseReward")) * fx(multiplier) * 0.3f));
        }
        if (RecoveringKerbal())
            AddParameter(std::make_unique<Parameters::RecoverKerbal>(Fmt("#autoLOC_274236", {recoveryKerbal->nameWithGender(), Planetarium::fetch->Home->displayName()})));
        if (RecoveringPart())
            AddParameter(std::make_unique<Parameters::RecoverPart>(Fmt("#autoLOC_274236", {VesselName(), Planetarium::fetch->Home->displayName()}),
                                                                   Parameters::RecoverPart::CompleteCondition::AllCandidates,
                                                                   Parameters::RecoverPart::CompleteCondition::AnyCandidate));
        SetExpiry(ContractDefsLite::I("Recovery", "Expiration", "MinimumExpireDays"), ContractDefsLite::I("Recovery", "Expiration", "MaximumExpireDays"));
        SetDeadlineDays(static_cast<float>(ContractDefsLite::I("Recovery", "Expiration", "DeadlineDays")), targetBody);
        SetFunds(f32store(fx(Def("Funds", "BaseAdvance")) * fx(multiplier)), f32store(fx(Def("Funds", "BaseReward")) * fx(multiplier) * 0.9f),
                 f32store(fx(Def("Funds", "BaseFailure")) * fx(multiplier)), targetBody);
        SetReputation(f32store(fx(Def("Reputation", "BaseReward")) * fx(multiplier) * 0.7f), f32store(fx(Def("Reputation", "BaseFailure")) * fx(multiplier) * 0.7f));
        SetScience(f32store(fx(Def("Science", "BaseReward")) * fx(multiplier)));
        return true;
    }
    return false;
}

// RecoverAsset.cs:207
void RecoverAsset::OnAccepted() {
    if (RecoveringKerbal()) {
        GetParameterT<Parameters::AcquireCrew>()->AddKerbal(recoveryKerbal->name().value_or(std::string()));
        GetParameterT<Parameters::RecoverKerbal>()->AddKerbal(recoveryKerbal->name().value_or(std::string()));
    }
    if (TemplateEnvironment::spawnRecoveryVessels) TemplateEnvironment::spawnRecoveryVessels(*this, recoveryLocation == RecoveryLocation::SURFACE);
    if (RecoveringPart()) {
        GetParameterT<Parameters::AcquirePart>()->AddPart(partID);
        GetParameterT<Parameters::RecoverPart>()->AddPartToRecover(partID);
    }
}

// RecoverAsset.cs:229
void RecoverAsset::OnParameterStateChange(ContractParameter* p) {
    if (p->State() == ParameterState::Complete) {
        if (RecoveringKerbal()) {
            std::shared_ptr<ProtoCrewMember> protoKerbal = ProtoKerbal();
            if (protoKerbal != nullptr) {
                if (dynamic_cast<Parameters::AcquireCrew*>(p) != nullptr) {
                    protoKerbal->set_type(ProtoCrewMember_KerbalType::Crew);
                    SendStateMessage(Fmt("#autoLOC_6001089", {recoveryKerbal->nameWithGender()}), Fmt("#autoLOC_6001090", {recoveryKerbal->nameWithGender()}));
                } else if (dynamic_cast<Parameters::RecoverKerbal*>(p) != nullptr) {
                    protoKerbal->set_type(ProtoCrewMember_KerbalType::Crew);
                    CheckComplete();
                }
            }
        }
        if (dynamic_cast<Parameters::RecoverPart*>(p) != nullptr) CheckComplete();
    }
    Contract::OnParameterStateChange(p);
}

// RecoverAsset.cs:258
std::string RecoverAsset::GetHashString() { return dotnet::FormatInt64(Lite::SuperSeed(*this)); }

// RecoverAsset.cs:263
std::string RecoverAsset::GetTitle() {
    const std::string text = (recoveryKerbal != nullptr) ? SU::ShortKerbalName(recoveryKerbal->nameWithGender()) : std::string();
    switch (recoveryType) {
        default:
            if (recoveryLocation == RecoveryLocation::SURFACE) return Fmt("#autoLOC_6100058", {text, ToLower(VesselDescription()), targetBody->displayName()});
            return Fmt("#autoLOC_6100059", {text, ToLower(VesselDescription()), targetBody->displayName()});
        case RecoveryType::PART:
            if (recoveryLocation == RecoveryLocation::SURFACE) return Fmt("#autoLOC_6100056", {VesselName(), targetBody->displayName()});
            return Fmt("#autoLOC_6100057", {VesselName(), targetBody->displayName()});
        case RecoveryType::KERBAL:
            if (recoveryLocation == RecoveryLocation::SURFACE) return Fmt("#autoLOC_6100054", {text, targetBody->displayName()});
            return Fmt("#autoLOC_6100055", {text, targetBody->displayName()});
    }
}

// RecoverAsset.cs:298
std::string RecoverAsset::GetDescription() {
    if (RecoveringKerbal())
        return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), SU::ShortKerbalName(recoveryKerbal->nameWithGender()), "RecoverKerbal", MissionSeed(), true, false, true);
    if (RecoveringPart()) return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), VesselName(), "RecoverPart", MissionSeed(), true, false, true);
    return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), Planetarium::fetch->Home->displayName(), "Kerbal", MissionSeed(), true, false, true);
}

// RecoverAsset.cs:311
std::string RecoverAsset::GetSynopsys() {
    const std::string home = Planetarium::fetch->Home->displayName();
    switch (recoveryType) {
        default:
            switch (recoveryLocation) {
                default: return Fmt("#autoLOC_6100068", {recoveryKerbal->nameWithGender(), targetBody->displayName(), home});
                case RecoveryLocation::SURFACE: return Fmt("#autoLOC_6100067", {recoveryKerbal->nameWithGender(), targetBody->displayName(), home});
                case RecoveryLocation::ORBITLOW: return Fmt("#autoLOC_6100066", {recoveryKerbal->nameWithGender(), targetBody->displayName(), home});
            }
        case RecoveryType::PART: {
            KSPRandom generator(MissionSeed());
            const ProtoCrewMember_Gender gender = (!Lite::CoinFlip(&generator)) ? ProtoCrewMember_Gender::Female : ProtoCrewMember_Gender::Male;
            std::string randomName = CrewGenerator::GetRandomName(gender, &generator);
            randomName = LocalizeName(randomName, gender);
            switch (recoveryLocation) {
                default: return Fmt("#autoLOC_6100065", {SU::ShortKerbalName(randomName), VesselName(), targetBody->displayName(), home});
                case RecoveryLocation::SURFACE: return Fmt("#autoLOC_6100064", {SU::ShortKerbalName(randomName), VesselName(), targetBody->displayName(), home});
                case RecoveryLocation::ORBITLOW: return Fmt("#autoLOC_6100063", {SU::ShortKerbalName(randomName), VesselName(), targetBody->displayName(), home});
            }
        }
        case RecoveryType::KERBAL:
            switch (recoveryLocation) {
                default: return Fmt("#autoLOC_6100062", {recoveryKerbal->nameWithGender(), targetBody->displayName(), home});
                case RecoveryLocation::SURFACE: return Fmt("#autoLOC_6100061", {recoveryKerbal->nameWithGender(), targetBody->displayName(), home});
                case RecoveryLocation::ORBITLOW: return Fmt("#autoLOC_6100060", {recoveryKerbal->nameWithGender(), targetBody->displayName(), home});
            }
    }
}

// RecoverAsset.cs:366
std::string RecoverAsset::GetNotes() {
    if (RecoveringPart()) {
        const auto& get = SaveEnvironment::Current().getPartInfoByName;
        const AvailablePart* partInfoByName = get ? get(partName) : nullptr;
        if (partInfoByName != nullptr) {
            const Vector3f vector = TemplateEnvironment::estimatePartSize ? TemplateEnvironment::estimatePartSize(partInfoByName) : Vector3f(0.0f, 0.0f, 0.0f);
            const std::string text = Fmt("#autoLOC_274385", {VesselName(), career::FormatNumber(partInfoByName->partPrefab->fields.mass, "N1"),
                                                             career::FormatNumber(vector.y, "N1"), career::FormatNumber(vector.x, "N1"),
                                                             career::FormatNumber(vector.z, "N1")});
            if (SaveEnvironment::Current().loadedScene != GameScenes::SPACECENTER) return text;
            return text + "\n";
        }
    }
    return std::string();
}

// RecoverAsset.cs:385
std::string RecoverAsset::MessageCompleted() {
    switch (recoveryType) {
        default: return Fmt("#autoLOC_6001092", {SU::ShortKerbalName(recoveryKerbal->nameWithGender()), ToLower(VesselDescription())});
        case RecoveryType::PART: return Fmt("#autoLOC_274401", {VesselName()});
        case RecoveryType::KERBAL: return Fmt("#autoLOC_6001091", {recoveryKerbal->nameWithGender()});
    }
}

// RecoverAsset.cs:398
std::string RecoverAsset::MessageFailed() {
    ContractParameter* parameter = GetParameterT<Parameters::AcquireCrew>();
    ContractParameter* parameter2 = GetParameterT<Parameters::AcquirePart>();
    ContractParameter* parameter3 = GetParameterT<Parameters::RecoverKerbal>();
    ContractParameter* parameter4 = GetParameterT<Parameters::RecoverPart>();
    bool flag = false;
    bool flag2 = false;
    if (parameter != nullptr && parameter->State() == ParameterState::Failed) flag = true;
    if (parameter2 != nullptr && parameter2->State() == ParameterState::Failed) flag2 = true;
    if (parameter3 != nullptr && parameter3->State() == ParameterState::Failed) flag = true;
    if (parameter4 != nullptr && parameter4->State() == ParameterState::Failed) flag2 = true;
    if (flag && flag2) return Fmt("#autoLOC_274430", {recoveryKerbal->nameWithGender(), VesselName()});
    if (flag) return Fmt("#autoLOC_274433", {recoveryKerbal->nameWithGender()});
    if (flag2) return Fmt("#autoLOC_274436", {VesselName()});
    return Localizer::Format("#autoLOC_274438");
}

// RecoverAsset.cs:437
std::string RecoverAsset::MessageCancelled() {
    switch (recoveryType) {
        default: return Fmt("#autoLOC_6001093", {SU::ShortKerbalName(recoveryKerbal->nameWithGender()), ToLower(VesselDescription())});
        case RecoveryType::PART: return Fmt("#autoLOC_274448", {VesselName()});
        case RecoveryType::KERBAL: return Fmt("#autoLOC_274446", {recoveryKerbal->nameWithGender()});
    }
}

// RecoverAsset.cs:450
std::string RecoverAsset::MessageDeadlineExpired() {
    switch (recoveryType) {
        default: return Fmt("#autoLOC_6001094", {SU::ShortKerbalName(recoveryKerbal->nameWithGender()), ToLower(VesselDescription())});
        case RecoveryType::PART: return Fmt("#autoLOC_274461", {VesselName()});
        case RecoveryType::KERBAL: return Fmt("#autoLOC_274459", {recoveryKerbal->nameWithGender()});
    }
}

// RecoverAsset.cs:463
void RecoverAsset::OnLoad(ConfigNode& node) {
    std::string value = "Jebediah Kerman";
    targetBody = Planetarium::fetch->Home;
    if (node.HasValue("targetBody")) {
        int idx = 0;
        if (dotnet::TryParseInt32(*node.GetValue("targetBody"), idx) && idx >= 0 && idx < static_cast<int>(FlightRegistry::Bodies().size()))
            targetBody = FlightRegistry::Bodies()[static_cast<size_t>(idx)];
    }
    Lite::LoadNode(node, "Template.RecoverAsset", "kerbalName", value, value);
    partName = "mk1pod.v2";
    Lite::LoadNode(node, "Template.RecoverAsset", "partName", partName, std::string("mk1pod.v2"));
    Lite::LoadNode(node, "Template.RecoverAsset", "partID", partID, 9999u);
    Lite::LoadNode(node, "Template.RecoverAsset", "recoveryLocation", recoveryLocation, RecoveryLocation::NONE);
    Lite::LoadNode(node, "Template.RecoverAsset", "recoveryType", recoveryType, RecoveryType::NONE);
    recoveryKerbal = (*CurrentGame()->CrewRoster)[value];
}

// RecoverAsset.cs:475
void RecoverAsset::OnSave(ConfigNode& node) {
    const std::string value = (recoveryKerbal != nullptr) ? recoveryKerbal->name().value_or(std::string()) : std::string("Jebediah Kerman");
    node.AddValue("targetBody", static_cast<int32_t>(targetBody->flightGlobalsIndex));
    node.AddValue("kerbalName", value);
    node.AddValue("partName", partName);
    node.AddValue("partID", partID);
    node.AddValue("recoveryLocation", static_cast<int32_t>(recoveryLocation));
    node.AddValue("recoveryType", static_cast<int32_t>(recoveryType));
}

// RecoverAsset.cs:486
bool RecoverAsset::MeetRequirements() {
    if (ProgressTracking::Instance() == nullptr) return state == State::Active;
    if (!ProgressTracking::Instance()->NodeComplete({"ReachedSpace"})) return false;
    if (PU::GetProgressLevel() < 4) return false;
    if (GameVariables::Instance->GetOrbitDisplayMode(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::TrackingStation)) <
        GameVariables::OrbitDisplayMode::AllOrbits)
        return false;
    return true;
}

// RecoverAsset.cs:512
void RecoverAsset::Cleanup() {
    if (RecoveringKerbal()) {
        std::shared_ptr<ProtoCrewMember> protoKerbal = ProtoKerbal();
        if (protoKerbal != nullptr && protoKerbal->type() == ProtoCrewMember_KerbalType::Unowned && TemplateEnvironment::expungeKerbal)
            TemplateEnvironment::expungeKerbal(*protoKerbal);
    }
    if (!PartIsValid() || partID == 0) return;
    if (TemplateEnvironment::destroyUnownedVesselsWithPart) TemplateEnvironment::destroyUnownedVesselsWithPart(partID);
}

// RecoverAsset.cs:599
bool RecoverAsset::Initialize() {
    KSPRandom kSPRandom(MissionSeed());
    const bool flag = GameVariables::Instance->UnlockedEVA(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::AstronautComplex));
    const bool num = TemplateEnvironment::researchedValidContractObjectives && TemplateEnvironment::researchedValidContractObjectives("Grapple");
    std::vector<RecoveryType> list;
    if (flag && ContractDefsLite::B("Recovery", "", "AllowKerbalRescue")) list.push_back(RecoveryType::KERBAL);
    if (num && ContractDefsLite::B("Recovery", "", "AllowPartRecovery")) {
        list.push_back(RecoveryType::PART);
        if (flag && prestige != ContractPrestige::Trivial && ContractDefsLite::B("Recovery", "", "AllowCompoundRecovery")) list.push_back(RecoveryType::COMPOUND);
    }
    if (list.empty()) return false;
    recoveryType = list[static_cast<size_t>(kSPRandom.Next(0, static_cast<int>(list.size())))];
    ChooseLocation(&kSPRandom);
    partName = ChooseVesselType(&kSPRandom);
    if (!PartIsValid()) {
        partName = "kerbalEVA";
        if (std::find(list.begin(), list.end(), RecoveryType::KERBAL) == list.end()) return false;
        recoveryType = RecoveryType::KERBAL;
    }
    return true;
}

// RecoverAsset.cs:636
void RecoverAsset::ChooseLocation(KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(MissionSeed());
        generator = own.get();
    }
    const MannedStatus manned = (recoveryType != RecoveryType::KERBAL && recoveryType != RecoveryType::COMPOUND) ? MannedStatus::ANY : MannedStatus::MANNED;
    std::vector<CelestialBody*> bodiesProgress = PU::GetBodiesProgressNoReach(ProgressType::ORBIT, true, manned);
    std::vector<CelestialBody*> bodiesProgress2 = PU::GetBodiesProgressNoReach(
        ProgressType::LANDING, true, manned, [](CelestialBody* cb) { return cb != Planetarium::fetch->Home && cb->hasSolidSurface; });
    if (prestige == ContractPrestige::Exceptional) {
        auto it = std::find(bodiesProgress.begin(), bodiesProgress.end(), Planetarium::fetch->Home);
        if (it != bodiesProgress.end()) bodiesProgress.erase(it);
    }
    if (!ContractDefsLite::B("Recovery", "", "AllowLandedVacuum"))
        bodiesProgress2.erase(std::remove_if(bodiesProgress2.begin(), bodiesProgress2.end(), [](CelestialBody* cb) { return !cb->atmosphere; }), bodiesProgress2.end());
    if (!ContractDefsLite::B("Recovery", "", "AllowLandedAtmosphere"))
        bodiesProgress2.erase(std::remove_if(bodiesProgress2.begin(), bodiesProgress2.end(), [](CelestialBody* cb) { return cb->atmosphere; }), bodiesProgress2.end());
    std::vector<RecoveryLocation> list;
    if (!bodiesProgress.empty()) {
        list.push_back(RecoveryLocation::ORBITHIGH);
        list.push_back(RecoveryLocation::ORBITLOW);
    }
    if (!bodiesProgress2.empty() && prestige == ContractPrestige::Exceptional) list.push_back(RecoveryLocation::SURFACE);
    if (prestige != ContractPrestige::Trivial && !list.empty() && bodiesProgress2.size() + bodiesProgress.size() > 0) {
        recoveryLocation = list[static_cast<size_t>(generator->Next(0, static_cast<int>(list.size())))];
        targetBody = (recoveryLocation == RecoveryLocation::SURFACE) ? WeightedBodyChoice(bodiesProgress2, generator) : WeightedBodyChoice(bodiesProgress, generator);
    } else {
        recoveryLocation = RecoveryLocation::ORBITLOW;
        targetBody = Planetarium::fetch->Home;
    }
}

// RecoverAsset.cs:679
std::string RecoverAsset::ChooseVesselType(KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(MissionSeed());
        generator = own.get();
    }
    std::vector<Part*> list;
    PartLoader* loader = PartLoader::Instance();
    const size_t count = loader != nullptr ? loader->loadedParts.size() : 0;
    for (size_t i = 0; i < count; ++i) {
        AvailablePart* availablePart = loader->loadedParts[i].get();
        if (ResearchAndDevelopment::GetTechnologyState(availablePart->TechRequired) != RDTech_State::Available) continue;
        Part* partPrefab = availablePart->partPrefab.get();
        if (RecoveringKerbal()) {
            if (PartHasSeats(Lite::GetPartName(partPrefab))) list.push_back(partPrefab);
        } else if (AcceptablePart(partPrefab)) {
            list.push_back(partPrefab);
        }
    }
    if (list.empty()) return "kerbalEVA";
    dotnet::ListSort(list, [](Part* a, Part* b) {
        const float x = a->partInfo->cost;
        const float y = b->partInfo->cost;
        return x < y ? -1 : (x > y ? 1 : (x == y ? 0 : (std::isnan(x) ? (std::isnan(y) ? 0 : -1) : 1)));
    });
    const int a = generator->Next(0, static_cast<int>(list.size()));
    const int b = generator->Next(0, static_cast<int>(list.size()));
    return Lite::GetPartName(list[static_cast<size_t>(dotnet::Math::Min(a, b))]);
}

// RecoverAsset.cs:716
bool RecoverAsset::AcceptablePart(Part* p) {
    if (!(p->fields.mass <= 0.5f) && !(p->partInfo->cost <= 0.0f) && !(p->partInfo->partSize <= 1.5f) && p->FindModuleOfClass("ModuleWheelBase") == nullptr) {
        if (p->FindModuleOfClass("ModuleDeployablePart") != nullptr) return false;
        return true;
    }
    return false;
}

// RecoverAsset.cs:846
bool RecoverAsset::PartHasSeats(const std::string& part) {
    if (TemplateEnvironment::partHasSeats) return TemplateEnvironment::partHasSeats(part);
    const auto& get = SaveEnvironment::Current().getPartInfoByName;
    const AvailablePart* info = get ? get(part) : nullptr;
    return info != nullptr && info->partPrefab != nullptr && info->partPrefab->fields.CrewCapacity > 0;
}

// RecoverAsset.cs:925
float RecoverAsset::GetMultiplier() {
    if (recoveryType == RecoveryType::NONE) return 1.0f;
    if (recoveryLocation == RecoveryLocation::SURFACE) {
        float num = static_cast<float>(targetBody->GeeASL);
        num = (!targetBody->atmosphere) ? f32store(fx(num) / 4.0) : f32store(fx(num) / 2.0);
        switch (recoveryType) {
            default: return f32store(1.0 + fx(num) + 0.1f);
            case RecoveryType::PART: return f32store(1.0 + fx(num));
            case RecoveryType::KERBAL: return f32store(1.0 + fx(num) - 0.1f);
        }
    }
    switch (recoveryType) {
        default: return 1.1f;
        case RecoveryType::PART: return 1.0f;
        case RecoveryType::KERBAL: return 0.9f;
    }
}

// RecoverAsset.cs:956
void RecoverAsset::CheckComplete() {
    auto* parameter = GetParameterT<Parameters::RecoverKerbal>();
    auto* parameter2 = GetParameterT<Parameters::RecoverPart>();
    if (recoveryType == RecoveryType::KERBAL && parameter != nullptr && parameter->State() == ParameterState::Complete) Complete();
    if (recoveryType == RecoveryType::PART && parameter2 != nullptr && parameter2->State() == ParameterState::Complete) Complete();
    if (recoveryType == RecoveryType::COMPOUND && parameter != nullptr && parameter->State() == ParameterState::Complete && parameter2 != nullptr &&
        parameter2->State() == ParameterState::Complete)
        Complete();
}

// RecoverAsset.cs:998
void RecoverAsset::DiscoverAsset(Vessel* v) {
    if (RecoveringKerbal()) {
        std::shared_ptr<ProtoCrewMember> protoKerbal = ProtoKerbal();
        if (protoKerbal != nullptr) {
            const auto& crew = v->GetVesselCrew();
            if (std::find(crew.begin(), crew.end(), protoKerbal) != crew.end() && TemplateEnvironment::discoverVessel) TemplateEnvironment::discoverVessel(v);
        }
    }
    if (!PartIsValid()) return;
    int count = static_cast<int>(v->parts.size());
    do {
        if (count-- <= 0) return;
    } while (v->parts[static_cast<size_t>(count)]->fields.flightID != partID);
    if (TemplateEnvironment::discoverVessel) TemplateEnvironment::discoverVessel(v);
}

}  // namespace ksp::Contracts::Templates
