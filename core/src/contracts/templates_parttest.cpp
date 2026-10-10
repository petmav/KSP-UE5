// WP-37: Contracts.Templates.PartTest (Assembly-CSharp/Contracts/Templates/PartTest.cs).
#include <algorithm>
#include <cmath>
#include <limits>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/agents.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/contracts/module_test_subject.hpp"
#include "ksp/contracts/templates.hpp"
#include "ksp/contracts/text_gen.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/science/science.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/random.hpp"

namespace ksp::Contracts::Templates {

using GE = events::GameEvents;
using KSP::Localization::Localizer;
using Sit = Vessel::Situations;

namespace {
float Def(const char* sub, const char* key) { return ContractDefsLite::F("Test", sub, key); }
std::string Fmt1(const char* tag, const std::string& a) { return Localizer::Format(tag, {a}); }
std::string Fmt2(const char* tag, const std::string& a, const std::string& b) { return Localizer::Format(tag, {a, b}); }
template <class T>
bool Has(const std::vector<T>& v, const T& x) {
    return std::find(v.begin(), v.end(), x) != v.end();
}
template <class T>
void RemoveOne(std::vector<T>& v, const T& x) {
    auto it = std::find(v.begin(), v.end(), x);
    if (it != v.end()) v.erase(it);
}
// Templates/PartTest.cs:1010 RemoveSituations(list, params remove): the removals walk `remove` from the end
void RemoveSituations(std::vector<Sit>& list, std::initializer_list<Sit> remove) {
    std::vector<Sit> r(remove);
    int num = static_cast<int>(r.size());
    while (num-- > 0) RemoveOne(list, r[static_cast<size_t>(num)]);
}
// Templates/PartTest.cs:1018 CombineSituations(list, check)
void CombineSituations(std::vector<Sit>& list, const std::vector<Sit>& check) {
    int count = static_cast<int>(list.size());
    while (count-- > 0)
        if (!Has(check, list[static_cast<size_t>(count)])) list.erase(list.begin() + count);
}
ModuleTestSubject* TestSubjectOf(AvailablePart* ap) {
    return ap->partPrefab ? dynamic_cast<ModuleTestSubject*>(ap->partPrefab->FindModuleOfClass("ModuleTestSubject")) : nullptr;
}
}  // namespace

// Templates/PartTest.cs:35
bool PartTest::MeetRequirements() { return ProgressTracking::Instance()->NodeComplete({"FirstLaunch"}); }

// Templates/PartTest.cs:49
std::string PartTest::GetDescription() {
    return TextGen::GenerateBackStories(TypeName(), Agent()->Title(), PartName, "Parts" + PartName, MissionSeed(), true, false, true);
}

// Templates/PartTest.cs:54
std::string PartTest::GetSynopsys() {
    if (hauled) return Fmt2("#autoLOC_6100002", PartName, GetSituationString(tgtSituation, destination, ""));
    return Fmt2("#autoLOC_6100003", PartName, GetSituationString(tgtSituation, destination, ""));
}

// Templates/PartTest.cs:63
std::string PartTest::GetTitle() {
    if (hauled) return Fmt2("#autoLOC_6100004", PartName, GetSituationString(tgtSituation, destination, ""));
    return Fmt2("#autoLOC_6100005", PartName, GetSituationString(tgtSituation, destination, ""));
}

// Templates/PartTest.cs:72
std::string PartTest::MessageCompleted() {
    if (hauled) return Localizer::Format("#autoLOC_273203");
    return Localizer::Format("#autoLOC_6003090");
}

// Templates/PartTest.cs:81
std::string PartTest::GetSituationString(Sit sit, CelestialBody* body, const std::string& biomeName) {
    const std::string dn = body->displayName();
    if (biomeName.empty()) {
        switch (sit) {
            case Sit::FLYING: return hauled ? Fmt1("#autoLOC_6100008", dn) : Fmt1("#autoLOC_6100009", dn);
            case Sit::LANDED:
                if (hauled) return body->ocean ? Fmt1("#autoLOC_6100010", dn) : Fmt1("#autoLOC_6100011", dn);
                return Fmt1("#autoLOC_6100012", dn);
            case Sit::SPLASHED: return hauled ? Fmt1("#autoLOC_6100015", dn) : Fmt1("#autoLOC_6100016", dn);
            case Sit::PRELAUNCH: return hauled ? Localizer::Format("#autoLOC_6100006") : Localizer::Format("#autoLOC_6100007");
            default: return "No situation report available";
            case Sit::ESCAPING: return hauled ? Fmt1("#autoLOC_6100019", dn) : Fmt1("#autoLOC_6100020", dn);
            case Sit::ORBITING: return hauled ? Fmt1("#autoLOC_6100013", dn) : Fmt1("#autoLOC_6100014", dn);
            case Sit::SUB_ORBITAL: return hauled ? Fmt1("#autoLOC_6100017", dn) : Fmt1("#autoLOC_6100018", dn);
        }
    }
    switch (sit) {
        case Sit::FLYING: return hauled ? Fmt2("#autoLOC_6100023", dn, biomeName) : Fmt2("#autoLOC_6100024", dn, biomeName);
        case Sit::LANDED:
            if (hauled) return body->ocean ? Fmt2("#autoLOC_6100025", dn, biomeName) : Fmt2("#autoLOC_6100026", dn, biomeName);
            return Fmt2("#autoLOC_6100027", dn, biomeName);
        case Sit::SPLASHED: return hauled ? Fmt2("#autoLOC_6100030", dn, biomeName) : Fmt2("#autoLOC_6100031", dn, biomeName);
        case Sit::PRELAUNCH: return hauled ? Localizer::Format("#autoLOC_6100021") : Localizer::Format("#autoLOC_6100022");
        default: return "No situation report available";
        case Sit::ESCAPING: return hauled ? Fmt2("#autoLOC_6100034", dn, biomeName) : Fmt2("#autoLOC_6100035", dn, biomeName);
        case Sit::ORBITING: return hauled ? Fmt2("#autoLOC_6100028", dn, biomeName) : Fmt2("#autoLOC_6100029", dn, biomeName);
        case Sit::SUB_ORBITAL: return hauled ? Fmt2("#autoLOC_6100032", dn, biomeName) : Fmt2("#autoLOC_6100033", dn, biomeName);
    }
}

// Templates/PartTest.cs:191
void PartTest::OnLoad(ConfigNode& node) {
    if (node.HasValue("part")) {
        const auto& get = SaveEnvironment::Current().getPartInfoByName;
        tgtPart = get ? get(*node.GetValue("part")) : nullptr;
        PartName = (tgtPart != nullptr) ? tgtPart->title : std::string("Part");
    }
    if (node.HasValue("haul")) {
        bool b = false;
        if (!dotnet::TryParseBool(*node.GetValue("haul"), b)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
        hauled = b;
    }
    if (node.HasValue("dest")) destination = FlightRegistry::Bodies().at(static_cast<size_t>(dotnet::ParseInt32(*node.GetValue("dest"))));
    if (node.HasValue("sit")) tgtSituation = dotnet::EnumParseFull<Sit>(*node.GetValue("sit"));
    if (node.HasValue("exp")) {
        bool b = false;
        if (!dotnet::TryParseBool(*node.GetValue("exp"), b)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
        PartIsExperimental = b;
    }
}

// Templates/PartTest.cs:216
void PartTest::OnSave(ConfigNode& node) {
    node.AddValue("part", (tgtPart != nullptr) ? tgtPart->name : std::string("Part"));
    node.AddValue("haul", hauled);
    int idx = -1;
    const auto& bodies = FlightRegistry::Bodies();
    for (size_t i = 0; i < bodies.size(); ++i)
        if (bodies[i] == destination) idx = static_cast<int>(i);
    node.AddValue("dest", static_cast<int32_t>(idx));
    node.AddValue("sit", dotnet::EnumToString(tgtSituation));
    node.AddValue("exp", PartIsExperimental);
}

// Templates/PartTest.cs:225
bool PartTest::Generate() {
    std::vector<PartTest*> currentContracts = ContractSystem::Instance->GetCurrentContracts<PartTest>();
    if (static_cast<int>(currentContracts.size()) >= ContractDefsLite::I("Test", "", "MaximumExistent")) return false;
    std::vector<ModuleTestSubject*> list;
    std::vector<std::string> list2;
    int num = static_cast<int>(currentContracts.size());
    for (int i = 0; i < num; ++i) list2.push_back(currentContracts[static_cast<size_t>(i)]->PartName);
    std::vector<std::shared_ptr<ProtoTechNode>> nextUnavailableNodes = RDTechTree::GetNextUnavailableNodes();
    PartLoader* loader = PartLoader::Instance();
    num = loader != nullptr ? static_cast<int>(loader->loadedParts.size()) : 0;
    for (int j = 0; j < num; ++j) {
        AvailablePart* availablePart = loader->loadedParts[static_cast<size_t>(j)].get();
        if (availablePart->TechHidden || Has(list2, availablePart->title)) continue;
        int count = static_cast<int>(nextUnavailableNodes.size());
        while (count-- > 0) {
            if (!(nextUnavailableNodes[static_cast<size_t>(count)]->techID != availablePart->TechRequired)) {
                ModuleTestSubject* moduleTestSubject = TestSubjectOf(availablePart);
                if (moduleTestSubject != nullptr) list.push_back(moduleTestSubject);
                break;
            }
        }
    }
    if (list.size() < 5 || Prestige() == ContractPrestige::Trivial) {
        num = loader != nullptr ? static_cast<int>(loader->loadedParts.size()) : 0;
        for (int k = 0; k < num; ++k) {
            AvailablePart* availablePart2 = loader->loadedParts[static_cast<size_t>(k)].get();
            if (!Has(list2, availablePart2->title) && ResearchAndDevelopment::GetTechnologyState(availablePart2->TechRequired) == RDTech_State::Available) {
                ModuleTestSubject* moduleTestSubject2 = TestSubjectOf(availablePart2);
                if (moduleTestSubject2 != nullptr && !Has(list, moduleTestSubject2)) list.push_back(moduleTestSubject2);
            }
        }
    }
    if (list.empty()) return false;
    ModuleTestSubject* moduleTestSubject3 = list[static_cast<size_t>(UnityRandom::Global().Range(0, static_cast<int32_t>(list.size())))];
    Part* subjectPart = dynamic_cast<Part*>(moduleTestSubject3->part);
    tgtPart = subjectPart->partInfo;
    PartName = (tgtPart != nullptr) ? tgtPart->title : std::string("Part");
    if (ResearchAndDevelopment::GetTechnologyState((tgtPart != nullptr) ? tgtPart->TechRequired : std::string("Technology")) != RDTech_State::Available) PartIsExperimental = true;
    std::vector<Sit> list3;
    for (Sit s : {Sit::PRELAUNCH, Sit::LANDED, Sit::SPLASHED, Sit::FLYING, Sit::SUB_ORBITAL, Sit::ORBITING, Sit::ESCAPING, Sit::DOCKED})
        if (moduleTestSubject3->SituationAvailable(s)) list3.push_back(s);
    std::vector<PartTestConstraint> constraints;
    num = static_cast<int>(moduleTestSubject3->constraints.size());
    for (int l = 0; l < num; ++l) {
        const PartTestConstraint& partTestConstraint = moduleTestSubject3->constraints[static_cast<size_t>(l)];
        if (partTestConstraint.PrestigeAvailable(Prestige())) constraints.push_back(partTestConstraint);
    }
    CelestialBody* const home = Planetarium::fetch->Home;
    CelestialBody* const sun = Planetarium::fetch->Sun;
    std::vector<CelestialBody*> list4;
    if (moduleTestSubject3->useProgressForBodies()) {
        switch (prestige) {
            default: list4 = {home}; break;
            case ContractPrestige::Exceptional: {
                list4 = Lite::ProgressUtilities::GetBodiesReached(true, [home](CelestialBody* cb) { return cb != home; });
                std::vector<CelestialBody*> next = Lite::ProgressUtilities::GetNextUnreached(1, [home](CelestialBody* cb) { return cb != home; });
                list4.insert(list4.end(), next.begin(), next.end());
                break;
            }
            case ContractPrestige::Significant: list4 = Lite::ProgressUtilities::GetBodiesReached(true, [sun](CelestialBody* cb) { return cb != sun; }); break;
        }
    } else {
        list4 = FlightRegistry::Bodies();
    }
    if (list4.size() >= 1) {
        std::vector<CelestialBody*> list5;
        num = static_cast<int>(list4.size());
        for (int m = 0; m < num; ++m) {
            CelestialBody* celestialBody = list4[static_cast<size_t>(m)];
            const int count2 = static_cast<int>(moduleTestSubject3->constraints.size());
            for (int n = 0; n < count2; ++n) {
                const PartTestConstraint& partTestConstraint2 = moduleTestSubject3->constraints[static_cast<size_t>(n)];
                if (!partTestConstraint2.BodyAvailable(celestialBody)) continue;
                switch (partTestConstraint2.type) {
                    case PartTestConstraint::ConstraintType::ATMOSPHERE:
                        if (!partTestConstraint2.Test(celestialBody->atmosphere) && !Has(list5, celestialBody)) list5.push_back(celestialBody);
                        break;
                    case PartTestConstraint::ConstraintType::OXYGEN:
                        if ((!partTestConstraint2.Test(celestialBody->atmosphereContainsOxygen) || !partTestConstraint2.Test(celestialBody->atmosphere)) &&
                            !Has(list5, celestialBody))
                            list5.push_back(celestialBody);
                        break;
                    default: break;
                }
            }
        }
        num = static_cast<int>(list5.size());
        for (int n2 = 0; n2 < num; ++n2) RemoveOne(list4, list5[static_cast<size_t>(n2)]);
        if (list4.empty()) return false;
        destination = WeightedBodyChoice(list4);
        constraints.erase(std::remove_if(constraints.begin(), constraints.end(), [this](const PartTestConstraint& c) { return !c.BodyAvailable(destination); }),
                          constraints.end());
        list3.erase(std::remove_if(list3.begin(), list3.end(), [&](Sit s) { return !PartTestConstraint::SituationAvailable(constraints, s); }), list3.end());
        if (!destination->atmosphere) RemoveOne(list3, Sit::FLYING);
        if (!destination->ocean) RemoveOne(list3, Sit::SPLASHED);
        if (!destination->hasSolidSurface) {
            RemoveOne(list3, Sit::LANDED);
            RemoveOne(list3, Sit::SPLASHED);
        }
        if (!destination->isHomeWorld) RemoveOne(list3, Sit::PRELAUNCH);
        if (list3.size() < 1) return false;
        UnityRandom& rnd = UnityRandom::Global();
        if (moduleTestSubject3->usePrestigeForSit()) {
            if (Prestige() == ContractPrestige::Trivial) {
                std::vector<Sit> list6;
                if (destination->isHomeWorld) {
                    list6.push_back(Sit::PRELAUNCH);
                    list6.push_back(Sit::LANDED);
                    if (destination->atmosphere) list6.push_back(Sit::FLYING);
                    if (destination->ocean) list6.push_back(Sit::SPLASHED);
                } else {
                    list6.push_back(Sit::ORBITING);
                    list6.push_back(Sit::ESCAPING);
                    if (destination->atmosphere) list6.push_back(Sit::FLYING);
                }
                CombineSituations(list6, list3);
                if (list6.empty()) return false;
                tgtSituation = list6[static_cast<size_t>(rnd.Range(0, static_cast<int32_t>(list6.size())))];
                if (!ProgressTracking::Instance()->reachSpace->IsComplete()) {
                    if (!destination->isHomeWorld) return false;
                    if (tgtSituation == Sit::SUB_ORBITAL) {
                        if (!Has(list3, Sit::FLYING)) return false;
                        tgtSituation = Sit::FLYING;
                    }
                }
            } else if (Prestige() == ContractPrestige::Significant) {
                std::vector<Sit> list7 = {Sit::FLYING, Sit::SUB_ORBITAL, Sit::ORBITING, Sit::ESCAPING};
                if (destination == sun) RemoveSituations(list7, {Sit::LANDED, Sit::SPLASHED, Sit::FLYING, Sit::SUB_ORBITAL, Sit::ESCAPING});
                CombineSituations(list7, list3);
                if (list7.empty()) return false;
                tgtSituation = list7[static_cast<size_t>(rnd.Range(0, static_cast<int32_t>(list7.size())))];
            } else {
                std::vector<Sit> list8 = {Sit::LANDED, Sit::SPLASHED, Sit::FLYING, Sit::SUB_ORBITAL, Sit::ORBITING, Sit::ESCAPING};
                if (destination == sun) RemoveSituations(list8, {Sit::LANDED, Sit::SPLASHED, Sit::FLYING, Sit::SUB_ORBITAL, Sit::ESCAPING});
                if (destination == home || !destination->hasSolidSurface) RemoveSituations(list8, {Sit::LANDED, Sit::SPLASHED});
                if (!destination->atmosphere) RemoveSituations(list8, {Sit::FLYING});
                if (!destination->ocean) RemoveSituations(list8, {Sit::SPLASHED});
                CombineSituations(list8, list3);
                if (list8.empty()) return false;
                tgtSituation = list8[static_cast<size_t>(rnd.Range(0, static_cast<int32_t>(list8.size())))];
            }
        } else {
            tgtSituation = list3[static_cast<size_t>(rnd.Range(0, static_cast<int32_t>(list3.size())))];
        }
        constraints.erase(std::remove_if(constraints.begin(), constraints.end(), [this](const PartTestConstraint& c) { return !c.SituationAvailable(tgtSituation); }),
                          constraints.end());
        double num3 = 0.0;
        double num4 = std::numeric_limits<double>::max();
        bool flag = false;
        double num5 = 0.0;
        double num6 = std::numeric_limits<double>::max();
        bool flag2 = false;
        double num7 = 0.0;
        double num8 = std::numeric_limits<double>::max();
        double num9 = 0.0;
        double num10 = std::numeric_limits<double>::max();
        double num11 = std::numeric_limits<double>::max();
        double num12 = 0.0;
        bool flag3 = false;
        double num13 = 0.0;
        double num14 = 0.0;
        PartTestConstraint::TestRepeatability testRepeatability = PartTestConstraint::TestRepeatability::ONCEPERPART;
        if (destination->atmosphere && tgtSituation == Sit::FLYING)
            num4 = destination->atmosphereDepth;
        else if (tgtSituation == Sit::SUB_ORBITAL || tgtSituation == Sit::ORBITING || tgtSituation == Sit::ESCAPING)
            num3 = destination->minOrbitalDistance - destination->Radius;
        using CT = PartTestConstraint::ConstraintType;
        using TT = PartTestConstraint::ConstraintTest;
        num = static_cast<int>(constraints.size());
        for (int num15 = 0; num15 < num; ++num15) {
            const PartTestConstraint& c3 = constraints[static_cast<size_t>(num15)];
            switch (c3.type) {
                case CT::SPEED:
                    if (c3.test == TT::GT) num5 = dotnet::Math::Max(num5, c3.valueD);
                    else if (c3.test == TT::LT) num6 = dotnet::Math::Min(num6, c3.valueD);
                    flag2 = true;
                    break;
                case CT::SPEEDENV:
                    if (c3.test == TT::GT) num9 = dotnet::Math::Max(num9, c3.valueD);
                    else if (c3.test == TT::LT) num10 = dotnet::Math::Min(num10, c3.valueD);
                    break;
                case CT::ALTITUDE:
                    if (c3.test == TT::GT) num3 = dotnet::Math::Max(num3, c3.valueD);
                    else if (c3.test == TT::LT) num4 = dotnet::Math::Min(num4, c3.valueD);
                    flag = true;
                    break;
                case CT::ALTITUDEENV:
                    if (c3.test == TT::GT) num7 = dotnet::Math::Max(num7, c3.valueD);
                    else if (c3.test == TT::LT) num8 = dotnet::Math::Min(num8, c3.valueD);
                    break;
                case CT::DENSITY: {
                    const double altitudeForDensity = Lite::CelestialUtilities::GetAltitudeForDensity(destination, c3.valueD);
                    if (c3.test == TT::GT) num4 = dotnet::Math::Min(num4, altitudeForDensity);
                    else if (c3.test == TT::LT) num3 = dotnet::Math::Max(num3, altitudeForDensity);
                    flag = true;
                    break;
                }
                case CT::DYNAMICPRESSURE:
                    if (c3.test == TT::GT) num12 = dotnet::Math::Max(num12, c3.valueD);
                    else if (c3.test == TT::LT) num11 = dotnet::Math::Min(num11, c3.valueD);
                    flag3 = true;
                    break;
                case CT::REPEATABILITY: testRepeatability = c3.valueR; break;
                default: break;
            }
        }
        if (!(num3 > num4) && num5 <= num6) {
            const int subjectsToRepeat = ContractDefsLite::I("Test", "", "SubjectsToRepeat");
            if (static_cast<int>(list.size()) < subjectsToRepeat && testRepeatability < PartTestConstraint::TestRepeatability::ONCEPERPART)
                testRepeatability = PartTestConstraint::TestRepeatability::ONCEPERPART;
            if (static_cast<int>(list.size()) < subjectsToRepeat * 2 && testRepeatability < PartTestConstraint::TestRepeatability::BODYANDSITUATION)
                testRepeatability = PartTestConstraint::TestRepeatability::BODYANDSITUATION;
            if (num4 - num3 < num8) num8 = num4 - num3;
            if (num8 < num7) num7 = num8;
            if (tgtSituation != Sit::LANDED && tgtSituation != Sit::SPLASHED && tgtSituation != Sit::PRELAUNCH) {
                if (tgtSituation == Sit::FLYING) {
                    double density = destination->GetDensity(destination->GetPressure(num4), destination->GetTemperature(num4));
                    double density2 = destination->GetDensity(destination->GetPressure(num3), destination->GetTemperature(num3));
                    double num16 = 0.005 * density2 * num6 * num6;
                    if (num6 == std::numeric_limits<double>::max()) num16 = std::numeric_limits<double>::max();
                    double num17 = 0.005 * density * num5 * num5;
                    if (num16 < num12) return false;
                    if (num17 > num11) return false;
                    if (flag) {
                        num13 = std::ceil(static_cast<double>(rnd.Range(static_cast<float>(num7), static_cast<float>(num8))) * 0.001) * 1000.0;
                        num3 = dotnet::Math::Round(static_cast<double>(rnd.Range(static_cast<float>(num3), static_cast<float>(num4 - num13))) * 0.001) * 1000.0;
                        num4 = num3 + num13;
                    }
                    if (flag3) {
                        density = destination->GetDensity(destination->GetPressure(num4), destination->GetTemperature(num4));
                        density2 = destination->GetDensity(destination->GetPressure(num3), destination->GetTemperature(num3));
                        num16 = 0.005 * density2 * num6 * num6;
                        num17 = 0.005 * density * num5 * num5;
                        if (num6 == std::numeric_limits<double>::max()) num16 = std::numeric_limits<double>::max();
                        if (num16 < num12) num6 = std::sqrt(num12 / (0.005 * density2));
                        if (num17 > num11) num5 = std::sqrt(num11 / (0.005 * density));
                        if (num6 < num5) return false;
                    }
                    if (flag2) {
                        if (num6 - num5 < num10) num10 = num6 - num5;
                        if (num10 < num9) num9 = num10;
                        num14 = std::ceil(static_cast<double>(rnd.Range(static_cast<float>(num9), static_cast<float>(num10))) * 0.1) * 10.0;
                        num5 = dotnet::Math::Round(static_cast<double>(rnd.Range(static_cast<float>(num5), static_cast<float>(num6 - num14))) * 0.1) * 10.0;
                        num6 = num5 + num14;
                    }
                } else {
                    if (flag) {
                        const double sphereOfInfluence = destination->sphereOfInfluence;
                        if (num4 > sphereOfInfluence) num4 = sphereOfInfluence;
                        num13 = std::ceil(static_cast<double>(rnd.Range(static_cast<float>(num7), static_cast<float>(num8))) * 0.0001) * 10000.0;
                        num3 = dotnet::Math::Round(static_cast<double>(rnd.Range(static_cast<float>(num3), static_cast<float>(num4 - num13))) * 0.0001) * 10000.0;
                        num4 = num3 + num13;
                    }
                    if (flag2) {
                        if (num6 - num5 < num10) num10 = num6 - num5;
                        if (num10 < num9) num9 = num10;
                        num14 = dotnet::Math::Round(static_cast<double>(rnd.Range(static_cast<float>(num9), static_cast<float>(num10))) * 0.01) * 100.0;
                        num5 = dotnet::Math::Round(static_cast<double>(rnd.Range(static_cast<float>(num5), static_cast<float>(num6 - num14))) * 0.01) * 100.0;
                        num6 = num5 + num14;
                    }
                }
            }
            KSPRandom kSPRandom(MissionSeed());
            hauled = false;
            if (ContractDefsLite::B("Test", "", "AllowHauls") &&
                (!destination->isHomeWorld || (tgtSituation != Sit::PRELAUNCH && tgtSituation != Sit::LANDED && tgtSituation != Sit::SPLASHED))) {
                int num18 = 0;
                switch (prestige) {
                    default: num18 = ContractDefsLite::I("Test", "", "ExceptionalHaulChance"); break;
                    case ContractPrestige::Significant: num18 = ContractDefsLite::I("Test", "", "SignificantHaulChance"); break;
                    case ContractPrestige::Trivial: num18 = ContractDefsLite::I("Test", "", "TrivialHaulChance"); break;
                }
                if (kSPRandom.Next(0, 100) < num18) hauled = true;
            }
            agent = Agents::AgentList::Instance->GetPartManufacturer(subjectPart->partInfo->manufacturer);
            std::optional<std::string> notes;
            if (!hauled && moduleTestSubject3->HasTestNotes()) notes = moduleTestSubject3->GetTestNotes();
            ContractParameter* contractParameter = AddParameter(std::make_unique<Parameters::PartTest>(
                subjectPart->partInfo, notes, static_cast<int>(testRepeatability), destination, tgtSituation, dotnet::FormatDoubleG(kSPRandom.NextDouble(), 17), hauled));
            IContractParameterHost* contractParameterHost3 = !hauled ? static_cast<IContractParameterHost*>(contractParameter) : static_cast<IContractParameterHost*>(this);
            contractParameterHost3->AddParameter(std::make_unique<Parameters::ReachDestination>(destination, ""))->SetDisableOnStateChange(false);
            contractParameterHost3->AddParameter(std::make_unique<Parameters::ReachSituation>(tgtSituation, ""))->SetDisableOnStateChange(false);
            if (num13 != 0.0)
                contractParameterHost3->AddParameter(std::make_unique<Parameters::ReachAltitudeEnvelope>(static_cast<float>(num3), static_cast<float>(num4)))->SetDisableOnStateChange(false);
            if (num14 != 0.0)
                contractParameterHost3->AddParameter(std::make_unique<Parameters::ReachSpeedEnvelope>(static_cast<float>(num5), static_cast<float>(num6)))->SetDisableOnStateChange(false);
            const float num19 = f32store(fx(GameVariables::Instance->ScoreSituation(tgtSituation, destination)) * fx(hauled ? 0.75f : 1.0f));
            SetExpiry(ContractDefsLite::I("Test", "Expiration", "MinimumExpireDays"), ContractDefsLite::I("Test", "Expiration", "MaximumExpireDays"));
            SetDeadlineDays(static_cast<float>(ContractDefsLite::I("Test", "Expiration", "DeadlineDays")), destination);
            const auto rnd1 = [&](const char* sub, const char* key) { return Mathf::Round(f32store(fx(Def(sub, key)) * fx(num19))); };
            SetFunds(rnd1("Funds", "BaseAdvance"), rnd1("Funds", "BaseReward"), rnd1("Funds", "BaseFailure"), destination);
            SetScience(rnd1("Science", "BaseReward"));
            SetReputation(rnd1("Reputation", "BaseReward"), rnd1("Reputation", "BaseFailure"));
            const float num20 = (tgtPart != nullptr) ? tgtPart->cost : 0.0f;
            const int num21 = (tgtPart != nullptr) ? tgtPart->entryCost : 0;
            if (FundsAdvance < static_cast<double>(num20))
                FundsAdvance = static_cast<double>(Mathf::Round(f32store(fx(num20) * fx(GameVariables::Instance->GetContractFundsAdvanceFactor(Prestige())))));
            if (FundsFailure < static_cast<double>(num20))
                FundsFailure = static_cast<double>(Mathf::Round(f32store(fx(num20) * fx(GameVariables::Instance->GetContractFundsFailureFactor(Prestige())))));
            if (FundsCompletion < static_cast<double>(num21))
                FundsCompletion = static_cast<double>(Mathf::Round(f32store(fx(static_cast<float>(num21)) * fx(GameVariables::Instance->GetContractFundsCompletionFactor(Prestige())))));
            return true;
        }
        return false;
    }
    return false;
}

// Templates/PartTest.cs:813
void PartTest::OnRegister() { GE::OnPartPurchased.Add(this, &PartTest::OnPartResearched); }
// Templates/PartTest.cs:818
void PartTest::OnUnregister() { GE::OnPartPurchased.Remove(this, &PartTest::OnPartResearched); }

// Templates/PartTest.cs:823
void PartTest::OnAccepted() {
    if (tgtPart != nullptr) {
        PartIsExperimental = !ResearchAndDevelopment::PartModelPurchased(tgtPart) || ResearchAndDevelopment::IsExperimentalPart(tgtPart);
        if (PartIsExperimental) ResearchAndDevelopment::AddExperimentalPart(tgtPart);
    }
}

// Templates/PartTest.cs:835
void PartTest::OnPartResearched(AvailablePart* ap) {
    if (tgtPart != nullptr && !(ap->title != PartName) && PartIsExperimental) {
        log::Info("[PartTest]: Contract for " + ap->title + " is no longer an experimental test. Part was researched.");
        ResearchAndDevelopment::RemoveExperimentalPart(tgtPart);
        PartIsExperimental = false;
    }
}

// Templates/PartTest.cs:845
void PartTest::OnFinished() {
    if (tgtPart != nullptr && PartIsExperimental) ResearchAndDevelopment::RemoveExperimentalPart(tgtPart);
}

}  // namespace ksp::Contracts::Templates
