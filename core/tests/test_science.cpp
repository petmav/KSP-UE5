// WP-35 unit tests: science data (ScienceDefs.cfg, TechTree.cfg), the subject / science value maths, ResearchAndDevelopment (the scenario,
// OnLoad / OnSave round trip, tech state, parts), the lab page value, ModuleScienceConverter (formulas and the FixedUpdate catch-up), the science
// part modules.
//
// No golden or capture of these classes exists (harness/golden/MANIFEST.json and harness/captures/out*/ carry no science data), so every
// expectation is derived: from the decompiled C# (the derivation is in the comment above the check), from the game's own data
// (ScienceDefs.cfg, TechTree.cfg, the lab part cfg, the part cfgs of every science module, the body science values of the PSystem prefab) or from
// tools/wp35/science_reference.py (an independent numpy float32 implementation; tests/data/wp35_reference.json). The save in
// core/testdata/saves carries no ResearchAndDevelopment SCENARIO node (see STATUS.md WP-35), so the round trip is built through the API from
// the game's own experiments and tech tree.
#include <cstring>
#include <fstream>
#include <map>
#include <set>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/science_modules.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/science/science.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/module_trip_logger.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_assembly.hpp"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::support;
using GE = events::GameEvents;
using json = nlohmann::json;
using PCM = std::shared_ptr<ProtoCrewMember>;
using Loc35 = KSP::Localization::Localizer;

namespace {

json LoadRef35() {
    std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp35_reference.json", std::ios::binary);
    REQUIRE(f.good());
    return json::parse(f);
}

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
float FromBits(uint32_t u) {
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}
int64_t UlpF(float a, float b) {
    int32_t x, y;
    std::memcpy(&x, &a, 4);
    std::memcpy(&y, &b, 4);
    if (x < 0) x = INT32_MIN - x;
    if (y < 0) y = INT32_MIN - y;
    return x > y ? static_cast<int64_t>(x) - y : static_cast<int64_t>(y) - x;
}
struct Tally {
    int64_t maxUlp = 0;
    long values = 0;
    void Add(float got, float expected) {
        maxUlp = std::max(maxUlp, UlpF(got, expected));
        values++;
    }
    void Bits32(float got, uint32_t expectedBits) { Add(got, FromBits(expectedBits)); }
};

// Every test starts and ends with a clean science environment, wired to the stock game
struct SciGuard {
    SciGuard() { Reset(); }
    ~SciGuard() { Reset(); }
    static void Reset() {
        ScienceEnvironment::Reset();
        ScienceModuleEnvironment::Reset();
        ResearchAndDevelopment::ResetStatics();
        RDTechTree::Reset();
        ResourceProcessingEnvironment::Reset();
        ResourceFlowEnvironment::partActionUIContains = nullptr;
    }
    void Wire(Game15& g) {
        Game15* gp = &g;
        ScienceEnvironment::getConfigNodes = [gp](std::string_view t) { return gp->data.database.GetConfigNodes(t); };
        ScienceEnvironment::loadedParts = [gp] {
            std::vector<const AvailablePart*> v;
            for (const auto& p : gp->data.parts.loadedParts) v.push_back(p.get());
            return v;
        };
    }
};

const CelestialBody& BodyOf(const std::string& name) {
    for (const auto& b : KerbolSystemPrefab().bodies)
        if (b.body.bodyName == name) return b.body;
    FAIL("no body " << name);
    return KerbolSystemPrefab().bodies[0].body;
}

ExperimentSituations SitOf(const std::string& n) { return dotnet::EnumParse<ExperimentSituations>(n); }

// the Scientist trait carries ScienceSkill (Squad Traits.cfg); the Squad traits are loaded by the stock game
struct CrewWorld {
    Game game;
    explicit CrewWorld(Game15& g) {
        SaveEnvironment& env = SaveEnvironment::Current();
        std::vector<const ConfigNode*> traits;
        for (ConfigNode* n : g.data.database.GetConfigNodes("EXPERIENCE_TRAIT")) traits.push_back(n);
        REQUIRE(traits.size() == 4);
        env.experienceConfigs = std::make_shared<Experience::ExperienceSystemConfig>(traits);
        game.Mode = Game_Modes::SANDBOX;
        game.CrewRoster = std::make_shared<KerbalRoster>(Game_Modes::SANDBOX);
        env.currentGame = &game;
    }
    ~CrewWorld() { SaveEnvironment::Current().currentGame = nullptr; }
    PCM Kerbal(const char* trait, int level) {
        PCM pcm = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Testy Kerman");
        pcm->trait = trait;
        for (const auto& cfg : SaveEnvironment::Current().experienceConfigs->Categories())
            if (cfg->Name() == trait) pcm->experienceTrait = Experience::ExperienceTrait::Create(cfg->Name(), cfg.get(), pcm.get());
        REQUIRE(pcm->experienceTrait);
        pcm->experienceLevel = level;
        return pcm;
    }
};

// the resource store the Python reference models: amounts live in the host part's resources
class TankBroker : public IResourceBroker {
public:
    explicit TankBroker(Part* p) : host(p) {}
    Part* host;
    PartResource& R(const std::string& n) {
        PartResource* r = host->GetResource(n);
        REQUIRE(r != nullptr);
        return *r;
    }
    static std::string NameOf(int id) { return PartResourceLibrary::Instance()->GetDefinition(id)->name(); }
    double AmountAvailable(Part*, const std::string& n, double, ResourceFlowMode) override { return R(n).amount; }
    double AmountAvailable(Part* p, int id, double dt, ResourceFlowMode m) override { return AmountAvailable(p, NameOf(id), dt, m); }
    double RequestResource(Part*, const std::string& n, double amt, double, ResourceFlowMode) override {
        PartResource& r = R(n);
        const double take = amt > 0 ? std::min(amt, r.amount) : 0.0;
        r.amount -= take;
        return take;
    }
    double RequestResource(Part* p, int id, double a, double dt, ResourceFlowMode m) override { return RequestResource(p, NameOf(id), a, dt, m); }
    double StorageAvailable(Part*, const std::string& n, double, ResourceFlowMode, double fill) override {
        PartResource& r = R(n);
        return std::max(0.0, r.maxAmount * fill - r.amount);
    }
    double StorageAvailable(Part* p, int id, double dt, ResourceFlowMode m, double f) override { return StorageAvailable(p, NameOf(id), dt, m, f); }
    double StoreResource(Part*, const std::string& n, double amt, double, ResourceFlowMode) override {
        PartResource& r = R(n);
        const double put = amt > 0 ? std::min(amt, r.maxAmount - r.amount) : 0.0;
        r.amount += put;
        return put;
    }
    double StoreResource(Part* p, int id, double a, double dt, ResourceFlowMode m) override { return StoreResource(p, NameOf(id), a, dt, m); }
};

template <class T>
T* ModuleOf(Part& p, const char* cls) {
    T* t = dynamic_cast<T*>(p.FindModuleOfClass(cls));
    REQUIRE(t != nullptr);
    return t;
}

}  // namespace

// ============================================================================================== ScienceDefs.cfg / TechTree.cfg

// ResearchAndDevelopment.loadExperiments (:157) over the game's ScienceDefs.cfg, ScienceExperiment.Load (ScienceExperiment.cs:38); the expected values
// are the independent parse in tools/wp35/science_reference.py (every EXPERIMENT_DEFINITION, float values stored as float32).
TEST_CASE("WP-35 ScienceDefs.cfg: every EXPERIMENT_DEFINITION loads with its fields and RESULTS; the recovery experiment is added") {
    if (!GameAvailable()) return;
    SciGuard guard;
    Game15 g;
    guard.Wire(g);
    const json ref = LoadRef35();
    std::vector<std::string> ids = ResearchAndDevelopment::GetExperimentIDs();
    REQUIRE(ids.size() == ref.at("experiments").size() + 1);
    size_t withResults = 0;
    for (const auto& re : ref.at("experiments")) {
        const std::string id = re.at("id").get<std::string>();
        INFO(id);
        ScienceExperiment* e = ResearchAndDevelopment::GetExperiment(id);
        REQUIRE(e != nullptr);
        CHECK(*e->id == id);
        CHECK(e->baseValue == static_cast<float>(re.at("baseValue").get<double>()));
        CHECK(e->scienceCap == static_cast<float>(re.at("scienceCap").get<double>()));
        CHECK(e->dataScale == static_cast<float>(re.at("dataScale").get<double>()));
        CHECK(e->requiredExperimentLevel == static_cast<float>(re.at("requiredExperimentLevel").get<double>()));
        CHECK(e->situationMask == re.at("situationMask").get<uint32_t>());
        CHECK(e->biomeMask == re.at("biomeMask").get<uint32_t>());
        CHECK(e->requireAtmosphere == re.at("requireAtmosphere").get<bool>());
        CHECK(e->requireNoAtmosphere == re.at("requireNoAtmosphere").get<bool>());
        CHECK(e->applyScienceScale == re.at("applyScienceScale").get<bool>());
        // RESULTS: duplicate keys get '*' appended (ScienceExperiment.cs:61), in file order
        const auto& rr = re.at("results");
        REQUIRE(e->Results().Count() == static_cast<int>(rr.size()));
        int i = 0;
        for (const auto& kv : e->Results()) {
            CHECK(kv.first == rr[static_cast<size_t>(i)][0].get<std::string>());
            // the game database localises cfg values on load (GameDatabase / Localizer.TranslateBranch): the raw cfg holds the #autoLOC tag
            const std::string raw = rr[static_cast<size_t>(i)][1].get<std::string>();
            CHECK(kv.second == (raw.rfind("#autoLOC", 0) == 0 ? Loc35::Format(raw) : raw));
            i++;
        }
        if (!rr.empty()) withResults++;
        // Save / Load round trip (ScienceExperiment.cs:69 / :38)
        ConfigNode n("EXPERIMENT_DEFINITION");
        e->Save(n);
        ScienceExperiment e2;
        e2.Load(n);
        CHECK(*e2.id == id);
        CHECK(Bits(e2.baseValue) == Bits(e->baseValue));
        CHECK(Bits(e2.scienceCap) == Bits(e->scienceCap));
        CHECK(e2.situationMask == e->situationMask);
        CHECK(e2.applyScienceScale == e->applyScienceScale);
        // unlocked by the default facility level (GameVariables.GetExperimentLevel is the identity, level 1)
        CHECK(e->IsUnlocked() == (e->requiredExperimentLevel <= 1.0f));
    }
    CHECK(withResults >= 10);
    ScienceExperiment* rec = ResearchAndDevelopment::GetExperiment("recovery");
    REQUIRE(rec != nullptr);
    CHECK(rec->scienceCap == 0.0f);
    CHECK(rec->dataScale == 0.0f);
    CHECK(rec->experimentTitle == Loc35::Format("#autoLOC_300438"));
    CHECK(ResearchAndDevelopment::GetExperiment("noSuchExperiment") == nullptr);
    // ExperimentSituations masks (ScienceExperiment.cs:84, :105)
    ScienceExperiment* goo = ResearchAndDevelopment::GetExperiment("mysteryGoo");
    REQUIRE(goo != nullptr);
    for (const auto& b : KerbolSystemPrefab().bodies)
        for (ExperimentSituations s : {ExperimentSituations::SrfLanded, ExperimentSituations::SrfSplashed, ExperimentSituations::FlyingLow,
                                       ExperimentSituations::FlyingHigh, ExperimentSituations::InSpaceLow, ExperimentSituations::InSpaceHigh}) {
            bool expected = (goo->situationMask & static_cast<uint32_t>(s)) != 0;
            if (goo->requireAtmosphere) expected = expected && b.body.atmosphere;
            if (goo->requireNoAtmosphere) expected = expected && !b.body.atmosphere;
            CHECK(goo->IsAvailableWhile(s, b.body) == expected);
            CHECK(goo->BiomeIsRelevantWhile(s) == ((goo->biomeMask & static_cast<uint32_t>(s)) != 0));
        }
    MESSAGE(ids.size() << " experiments (" << withResults << " with RESULTS)");
}

// RDTechTree.LoadTechTree / ProtoRDNode / ProtoTechNode over the game's TechTree.cfg (RDTechTree.cs:223, ProtoRDNode.cs:20, ProtoTechNode.cs:19)
TEST_CASE("WP-35 TechTree.cfg: every RDNode loads as a ProtoRDNode with its cost, links and title; tree queries") {
    if (!GameAvailable()) return;
    SciGuard guard;
    Game15 g;
    guard.Wire(g);
    std::vector<ConfigNode*> trees = g.data.database.GetConfigNodes("TechTree");
    REQUIRE(!trees.empty());
    std::vector<ConfigNode*> rdnodes;
    for (ConfigNode* t : trees)
        for (ConfigNode* n : t->GetNodes("RDNode")) rdnodes.push_back(n);
    REQUIRE(rdnodes.size() > 30);
    auto techs = RDTechTree::GetTreeTechs();
    auto nodes = RDTechTree::GetTreeNodes();
    REQUIRE(techs.size() == rdnodes.size());
    REQUIRE(nodes.size() == rdnodes.size());
    std::map<std::string, ProtoRDNode*> byId;
    for (auto& n : nodes) byId[n->tech->techID] = n.get();
    size_t links = 0;
    for (size_t i = 0; i < rdnodes.size(); i++) {
        const ConfigNode& cn = *rdnodes[i];
        INFO(*cn.GetValue("id"));
        // GetTreeTechs is indexed like the file (ReLoad fills it from the end, one to one)
        CHECK(techs[i]->techID == *cn.GetValue("id"));
        CHECK(techs[i]->scienceCost == dotnet::ParseInt32(*cn.GetValue("cost")));
        CHECK(techs[i]->state == RDTech_State::Unavailable);  // no `state` value in TechTree.cfg: the enum default
        CHECK(techs[i]->partsPurchased.empty());
        ProtoRDNode* n = nodes[i].get();
        CHECK(n->iconRef == (cn.HasValue("icon") ? *cn.GetValue("icon") : std::string()));
        CHECK(n->AnyParentToUnlock == (cn.HasValue("anyToUnlock") && *cn.GetValue("anyToUnlock") == "True"));
        std::vector<std::string> expectedParents;
        for (ConfigNode* p : cn.GetNodes("Parent"))
            if (p->HasValue("parentID") && byId.count(*p->GetValue("parentID"))) expectedParents.push_back(*p->GetValue("parentID"));
        REQUIRE(n->parents.size() == expectedParents.size());
        for (size_t k = 0; k < expectedParents.size(); k++) {
            CHECK(n->parents[k]->tech->techID == expectedParents[k]);
            // the parent lists the child (ProtoRDNode.cs:161)
            CHECK(std::find(n->parents[k]->children.begin(), n->parents[k]->children.end(), n) != n->parents[k]->children.end());
            links++;
        }
    }
    // titles (RDTechTree.LoadTechTitles :267)
    dotnet::Dictionary<std::string, std::string> titles;
    RDTechTree::LoadTechTitles("GameData/Squad/Resources/TechTree.cfg", titles, true);
    CHECK(static_cast<size_t>(titles.Count()) == rdnodes.size());
    for (ConfigNode* cn : rdnodes) CHECK(titles.At(*cn->GetValue("id")) == *cn->GetValue("title"));
    CHECK(ResearchAndDevelopment::GetTechnologyTitle("basicRocketry") == titles.At("basicRocketry"));
    CHECK(ResearchAndDevelopment::GetTechnologyTitle("noSuchTech").empty());
    // queries with an initial-state R&D (only `start` researched)
    ResearchAndDevelopment rd;
    rd.Awake();
    ConfigNode empty("ResearchAndDevelopment");
    rd.OnLoad(empty);
    std::vector<std::string> directChildren;
    for (auto& n : nodes)
        for (auto* p : n->parents)
            if (p->tech->techID == "start") directChildren.push_back(n->tech->techID);
    auto next = RDTechTree::GetNextUnavailableNodes();
    REQUIRE(next.size() == directChildren.size());
    for (size_t i = 0; i < next.size(); i++) CHECK(next[i]->techID == directChildren[i]);
    int minCost = INT32_MAX;
    for (auto& t : techs)
        if (t->techID != "start") minCost = std::min(minCost, t->scienceCost);
    auto cheapest = RDTechTree::GetCheapestUnavailableNodes(0);
    CHECK(!cheapest.empty());
    for (auto& t : cheapest) CHECK(t->scienceCost <= minCost);
    CHECK(RDTechTree::FindTech("basicRocketry") != nullptr);
    CHECK(RDTechTree::FindTech("noSuchTech") == nullptr);
    MESSAGE(rdnodes.size() << " tech nodes, " << links << " parent links");
}

// ============================================================================================== subject / science value maths vs the reference

TEST_CASE("WP-35 ScienceSubject ids and science values (GetReferenceDataValue / GetScienceValue / GetNextScienceValue / SubmitScienceData) equal the Python reference (0 ULP)") {
    if (!GameAvailable()) return;
    SciGuard guard;
    Game15 g;
    guard.Wire(g);
    const json ref = LoadRef35();
    ResearchAndDevelopment rd;
    rd.Awake();
    REQUIRE(ResearchAndDevelopment::Instance == &rd);
    // the body science values the reference read from the harness golden equal the prefab's
    for (const auto& rb : ref.at("bodies")) {
        const CelestialBody& b = BodyOf(rb.at("name").get<std::string>());
        const auto& sv = rb.at("sv");
        CHECK(Bits(b.scienceValues.LandedDataValue) == sv.at("LandedDataValue").get<uint32_t>());
        CHECK(Bits(b.scienceValues.InSpaceHighDataValue) == sv.at("InSpaceHighDataValue").get<uint32_t>());
        CHECK(Bits(b.scienceValues.RecoveryValue) == sv.at("RecoveryValue").get<uint32_t>());
        CHECK(b.atmosphere == rb.at("atmosphere").get<bool>());
        std::vector<std::string> names;
        if (b.BiomeMap) for (const auto& a : b.BiomeMap->Attributes) names.push_back(a.name);
        CHECK(names == rb.at("biomes").get<std::vector<std::string>>());
    }
    Tally t;
    size_t cases = 0, steps = 0, idMismatch = 0;
    for (const auto& c : ref.at("subjectCases")) {
        const std::string expId = c.at("exp").get<std::string>();
        ScienceExperiment* exp = ResearchAndDevelopment::GetExperiment(expId);
        REQUIRE(exp != nullptr);
        const CelestialBody& body = BodyOf(c.at("body").get<std::string>());
        auto subj = ResearchAndDevelopment::GetExperimentSubject(*exp, SitOf(c.at("sit").get<std::string>()), body, c.at("biome").get<std::string>(), "");
        if (subj->id != c.at("id").get<std::string>()) {
            idMismatch++;
            CHECK(subj->id == c.at("id").get<std::string>());
        }
        t.Bits32(subj->subjectValue, c.at("subjectValue").get<uint32_t>());
        t.Bits32(subj->scienceCap, c.at("scienceCap").get<uint32_t>());
        t.Bits32(subj->dataScale, c.at("dataScale").get<uint32_t>());
        CHECK(subj->scientificValue == 1.0f);
        CHECK(subj->science == 0.0f);
        CHECK(ResearchAndDevelopment::GetSubjectByID(subj->id) == subj);
        const float data = FromBits(c.at("data").get<uint32_t>());
        for (const auto& st : c.at("steps")) {
            const float x = FromBits(st.at("x").get<uint32_t>());
            const float ratio = FromBits(st.at("ratio").get<uint32_t>());
            t.Bits32(ResearchAndDevelopment::GetReferenceDataValue(data, *subj), st.at("refValue").get<uint32_t>());
            t.Bits32(ResearchAndDevelopment::GetScienceValue(data, ratio, *subj, x), st.at("scienceValue").get<uint32_t>());
            t.Bits32(ResearchAndDevelopment::GetNextScienceValue(data, *subj, x), st.at("nextValue").get<uint32_t>());
            const float got = rd.SubmitScienceData(data, ratio, *subj, x);
            t.Bits32(got, st.at("submitted").get<uint32_t>());
            t.Bits32(subj->science, st.at("science").get<uint32_t>());
            t.Bits32(subj->scientificValue, st.at("scientificValue").get<uint32_t>());
            steps++;
        }
        cases++;
        if (cases % 200 == 0) ScienceEnvironment::screenMessages.clear();
    }
    CHECK(t.maxUlp == 0);
    CHECK(idMismatch == 0);
    CHECK(cases >= 1000);
    // the same subject asked again is the registered one with its title / cap / value refreshed (getScienceSubject :677)
    MESSAGE(cases << " subjects, " << steps << " submissions, " << t.values << " floats, max ULP " << t.maxUlp);
    // the science points are the sum of the submissions (AddScience :53), never negative
    CHECK(rd.Science() > 0.0f);
}

// ExperimentResultDialogPage.UpdatePageLabValue (:148): SurfaceBonus / ContextBonus / homeworldMultiplier and Math.Round
TEST_CASE("WP-35 science lab page value (SurfaceBonus, ContextBonus, homeworldMultiplier, Math.Round) equals the Python reference (0 ULP)") {
    const json ref = LoadRef35();
    const auto& lab = ref.at("lab");
    const float sb = FromBits(lab.at("SurfaceBonus").get<uint32_t>());
    const float cb = FromBits(lab.at("ContextBonus").get<uint32_t>());
    const float hm = FromBits(lab.at("homeworldMultiplier").get<uint32_t>());
    Tally t;
    for (const auto& c : ref.at("labCases")) {
        const float v = ExperimentResultDialogPage::LabValueMaths(FromBits(c.at("ref").get<uint32_t>()), c.at("landed").get<bool>(), c.at("splashed").get<bool>(),
                                                                  c.at("context").get<bool>(), c.at("home").get<bool>(), sb, cb, hm);
        t.Bits32(v, c.at("labValue").get<uint32_t>());
    }
    CHECK(t.maxUlp == 0);
    MESSAGE(t.values << " lab values, max ULP " << t.maxUlp);
}

// ============================================================================================== the lab and its converter

TEST_CASE("WP-35 stock lab part: ModuleScienceLab / ModuleScienceConverter fields equal the cfg and the reference's reading of it; formulas equal the reference (0 ULP)") {
    if (!GameAvailable()) return;
    SciGuard guard;
    Game15 g;
    guard.Wire(g);
    CrewWorld w(g);
    const json ref = LoadRef35();
    const auto& lab = ref.at("lab");
    auto host = g.New("Large.Crewed.Lab");
    auto* labm = ModuleOf<ModuleScienceLab>(*host, "ModuleScienceLab");
    auto* conv = ModuleOf<ModuleScienceConverter>(*host, "ModuleScienceConverter");
    CHECK(Bits(labm->Get<float>("crewsRequired")) == lab.at("crewsRequired").get<uint32_t>());
    CHECK(Bits(labm->Get<float>("SurfaceBonus")) == lab.at("SurfaceBonus").get<uint32_t>());
    CHECK(Bits(labm->Get<float>("ContextBonus")) == lab.at("ContextBonus").get<uint32_t>());
    CHECK(Bits(labm->Get<float>("homeworldMultiplier")) == lab.at("homeworldMultiplier").get<uint32_t>());
    CHECK(Bits(labm->Get<float>("dataStorage")) == lab.at("dataStorage").get<uint32_t>());
    CHECK(Bits(conv->Get<float>("dataProcessingMultiplier")) == lab.at("dataProcessingMultiplier").get<uint32_t>());
    CHECK(Bits(conv->Get<float>("scientistBonus")) == lab.at("scientistBonus").get<uint32_t>());
    CHECK(conv->Get<int>("researchTime") == lab.at("researchTime").get<int>());
    CHECK(conv->Get<int>("scienceMultiplier") == lab.at("scienceMultiplier").get<int>());
    CHECK(conv->Get<int>("scienceCap") == lab.at("scienceCap").get<int>());
    CHECK(Bits(conv->Get<float>("powerRequirement")) == lab.at("powerRequirement").get<uint32_t>());
    CHECK(conv->Lab() == labm);
    CHECK(labm->Converter() == conv);
    const int day = KSPUtil::dateTimeFormatter().Day();
    size_t checked = 0;
    Tally t;
    for (const auto& c : ref.at("converterFormulas")) {
        if (c.at("day").get<int>() != day) continue;
        host->protoModuleCrew.clear();
        for (int lvl : c.at("levels").get<std::vector<int>>()) host->protoModuleCrew.push_back(w.Kerbal("Scientist", lvl));
        const float data = FromBits(c.at("data").get<uint32_t>());
        labm->Set<float>("dataStored", FromBits(c.at("stored").get<uint32_t>()));
        t.Bits32(conv->GetScientists(), c.at("scientists").get<uint32_t>());
        CHECK(conv->CalculateScienceAmount(data) == c.at("amount").get<double>());
        CHECK(conv->CalculateScienceRate(data) == c.at("rate").get<double>());
        if (!c.at("researchTime").is_null()) CHECK(conv->CalculateResearchTime(data) == c.at("researchTime").get<double>());
        checked++;
    }
    CHECK(t.maxUlp == 0);
    CHECK(checked >= 100);
    // non-scientists add nothing (HasEffect<ScienceSkill>)
    host->protoModuleCrew.clear();
    host->protoModuleCrew.push_back(w.Kerbal("Pilot", 5));
    CHECK(conv->GetScientists() == 0.0f);
    CHECK(!labm->IsOperational());
    host->protoModuleCrew.push_back(w.Kerbal("Scientist", 0));
    CHECK(labm->IsOperational());
    MESSAGE(checked << " formula cases (Day = " << day << ")");
}

// BaseConverter.FixedUpdate (:500) for ModuleScienceConverter: GetDeltaTime / GetBestDeltaTime catch-up, ProcessRecipe on the electric charge
// tank, PostProcess (ModuleScienceConverter.cs:118). The reference models the same simple tank store.
TEST_CASE("WP-35 ModuleScienceConverter FixedUpdate: catch-up, electric charge, data to science, caps, stop without scientists equal the Python reference") {
    if (!GameAvailable()) return;
    SciGuard guard;
    Game15 g;
    guard.Wire(g);
    CrewWorld w(g);
    const json ref = LoadRef35();
    ResourceScenario scenario;
    scenario.Awake();
    SceneScope scope(GameScenes::FLIGHT);
    Time::fixedDeltaTime = 0.02f;
    ResourceProcessingEnvironment::flightReady = true;
    ResourceProcessingEnvironment::timeSinceLevelLoad = 10.0f;
    size_t scenarios = 0, steps = 0, produced = 0, retries = 0;
    for (const auto& sc : ref.at("converterScenarios")) {
        CAPTURE(sc.at("name").get<std::string>());
        auto host = g.New("Large.Crewed.Lab");
        host->fields.persistentId = 1;
        PartResource* ec = host->GetResource("ElectricCharge");
        if (ec == nullptr) ec = host->Resources.Add("ElectricCharge", 0.0, 1.0, true, true, false, true, PartResourceFlowMode::Both);
        REQUIRE(ec != nullptr);
        ec->maxAmount = 1e12;
        ec->amount = sc.at("ec").get<double>();
        TestVessel vessel;
        vessel.Adopt({host.get()});
        ProcessingVessel pv;
        ResourceProcessingEnvironment::vesselOf = [&](const Part&) { return &pv; };
        double ut = 0.0;
        ResourceProcessingEnvironment::universalTime = [&] { return ut; };
        scenario.gameSettings()->MaxDeltaTime = sc.at("maxDeltaTime").get<int>();
        for (int lvl : sc.at("levels").get<std::vector<int>>()) host->protoModuleCrew.push_back(w.Kerbal("Scientist", lvl));
        auto* labm = ModuleOf<ModuleScienceLab>(*host, "ModuleScienceLab");
        auto* conv = ModuleOf<ModuleScienceConverter>(*host, "ModuleScienceConverter");
        TankBroker broker(host.get());
        conv->_resBroker = &broker;
        labm->Set<float>("dataStored", static_cast<float>(sc.at("dataStored").get<double>()));
        labm->Set<float>("storedScience", static_cast<float>(sc.at("storedScience").get<double>()));
        conv->OnStart(StartState::Flying);
        conv->Set<bool>("IsActivated", true);
        // the reference assumes EfficiencyBonus * heat throttle * modifiers * crew bonus == 1
        CHECK(conv->GetEfficiencyMultiplier() == 1.0);
        for (const auto& e : sc.at("expected")) {
            ut = e.at("ut").get<double>();
            conv->FixedUpdate();
            CHECK(conv->lastUpdateTime == e.at("lastUpdateTime").get<double>());
            CHECK(conv->catchupRetries == e.at("catchupRetries").get<int>());
            CHECK(conv->lastTimeFactor == e.at("lastTimeFactor").get<double>());
            CHECK(host->GetResource("ElectricCharge")->amount == e.at("ec").get<double>());
            CHECK(Bits(labm->Get<float>("dataStored")) == e.at("dataStored").get<uint32_t>());
            CHECK(Bits(labm->Get<float>("storedScience")) == e.at("storedScience").get<uint32_t>());
            CHECK(conv->Get<bool>("IsActivated") == e.at("activated").get<bool>());
            const int status = e.at("status").get<int>();
            if (status == 1) CHECK(conv->Get<std::string>("status") == Loc35::Format("#autoLOC_237911"));
            if (status == 2) CHECK(conv->Get<std::string>("status") == Loc35::Format("#autoLOC_237928"));
            if (status == 3) CHECK(conv->Get<std::string>("status") == Loc35::Format("#autoLOC_237931"));
            steps++;
        }
        if (labm->Get<float>("storedScience") > static_cast<float>(sc.at("storedScience").get<double>())) produced++;
        if (conv->catchupRetries > 0) retries++;
        scenarios++;
        conv->_resBroker = nullptr;
        host->protoModuleCrew.clear();
    }
    CHECK(scenarios == ref.at("converterScenarios").size());
    CHECK(produced >= 3);
    CHECK(retries >= 1);
    MESSAGE(scenarios << " scenarios, " << steps << " FixedUpdate steps, " << produced << " producing science, " << retries << " using catch-up retries");
}

// ============================================================================================== ResearchAndDevelopment

// OnLoad (:117) without Tech nodes assumes the initial state (getInitialTech :240); OnSave (:211); PartTechAvailable / PartModelPurchased (:494, :511);
// AddScience (:53); the experimental parts (:286-:320); UnlockProtoTechNode (:1004). The expectations are derived from the C# and the game's own
// part cfgs (TechRequired) and TechTree.cfg (costs).
TEST_CASE("WP-35 ResearchAndDevelopment: initial tech, part availability, science points, OnSave / OnLoad round trip of tech, subjects and experimental parts") {
    if (!GameAvailable()) return;
    SciGuard guard;
    Game15 g;
    guard.Wire(g);
    Game game;
    game.Mode = Game_Modes::CAREER;
    game.Parameters->Career.StartingScience = 12.5f;
    SaveEnvironment::Current().currentGame = &game;
    SaveEnvironment::Current().loadedSceneIsGame = true;
    struct Restore {
        ~Restore() {
            SaveEnvironment::Current().currentGame = nullptr;
            SaveEnvironment::Current().loadedSceneIsGame = false;
        }
    } restore;

    ResearchAndDevelopment rd;
    rd.Awake();
    CHECK(rd.Science() == 12.5f);  // OnAwake: Career.StartingScience
    ConfigNode empty("ResearchAndDevelopment");
    rd.OnLoad(empty);
    CHECK(empty.HasNode("Tech"));  // getInitialTech's node was added to the loaded node
    std::vector<const AvailablePart*> startParts, otherParts;
    for (const auto& ap : g.data.parts.loadedParts) (ap->TechRequired == "start" ? startParts : otherParts).push_back(ap.get());
    REQUIRE(!startParts.empty());
    REQUIRE(!otherParts.empty());
    REQUIRE(rd.ProtoTechNodes().Count() == 1);
    auto start = rd.GetTechState("start");
    REQUIRE(start != nullptr);
    CHECK(start->state == RDTech_State::Available);
    CHECK(start->partsPurchased == startParts);  // `part` values in loadedParts order
    CHECK(start->scienceCost == 0);              // TechTree.cfg: start costs 0
    CHECK(ResearchAndDevelopment::GetTechnologyState("start") == RDTech_State::Available);
    CHECK(ResearchAndDevelopment::GetTechnologyState("basicRocketry") == RDTech_State::Unavailable);
    for (const AvailablePart* ap : startParts) {
        CHECK(ResearchAndDevelopment::PartTechAvailable(ap));
        CHECK(ResearchAndDevelopment::PartModelPurchased(ap));
    }
    const AvailablePart* locked = nullptr;
    for (const AvailablePart* ap : otherParts)
        if (!ap->TechRequired.empty() && ap->TechRequired != "start") {
            locked = ap;
            break;
        }
    REQUIRE(locked != nullptr);
    CHECK(!ResearchAndDevelopment::PartTechAvailable(locked));
    CHECK(!ResearchAndDevelopment::PartModelPurchased(locked));
    // an experimental part is available (and its model purchased) while the stock lasts (:301-:320, :494-:526)
    ResearchAndDevelopment::AddExperimentalPart(locked);
    ResearchAndDevelopment::AddExperimentalPart(locked);
    CHECK(ResearchAndDevelopment::IsExperimentalPart(locked));
    CHECK(ResearchAndDevelopment::PartTechAvailable(locked));
    CHECK(ResearchAndDevelopment::PartModelPurchased(locked));
    ResearchAndDevelopment::RemoveExperimentalPart(locked);
    CHECK(ResearchAndDevelopment::IsExperimentalPart(locked));
    ResearchAndDevelopment::RemoveExperimentalPart(locked);
    CHECK(!ResearchAndDevelopment::IsExperimentalPart(locked));
    CHECK(!ResearchAndDevelopment::PartTechAvailable(locked));
    ResearchAndDevelopment::AddExperimentalPart(locked);

    // AddScience (:53): a float add, clamped at 0 unless AllowNegativeCurrency; OnScienceChanged fires with the new total
    std::vector<std::pair<float, int>> changes;
    auto handler = events::EventData<float, TransactionReasons>::OnEvent::Lambda(&rd, [&](float s, TransactionReasons r) { changes.emplace_back(s, static_cast<int>(r)); });
    GE::OnScienceChanged.Add(handler);
    rd.AddScience(7.25f, science_reason::ScienceTransmission());
    CHECK(rd.Science() == 19.75f);
    rd.AddScience(-100.0f, science_reason::Cheating());
    CHECK(rd.Science() == 0.0f);
    rd.SetScience(3.5f, science_reason::Cheating());
    CHECK(rd.Science() == 3.5f);
    game.Parameters->CustomParams<GameParameters::AdvancedParams>().AllowNegativeCurrency = true;
    rd.AddScience(-10.0f, science_reason::Cheating());
    CHECK(rd.Science() == -6.5f);
    game.Parameters->CustomParams<GameParameters::AdvancedParams>().AllowNegativeCurrency = false;
    rd.SetScience(40.0f, science_reason::Cheating());
    GE::OnScienceChanged.Remove(handler);
    REQUIRE(changes.size() == 5);
    CHECK(changes[0] == std::make_pair(19.75f, 0x400));
    CHECK(changes[1] == std::make_pair(0.0f, 0x10000));
    CHECK(ResearchAndDevelopment::CanAfford(40.0f));
    CHECK(!ResearchAndDevelopment::CanAfford(40.1f));

    // unlock a tech node (:1004): Funding is absent (sandbox seam) so the parts of the node are purchased
    auto br = RDTechTree::FindTech("basicRocketry");
    REQUIRE(br != nullptr);
    rd.UnlockProtoTechNode(*br);
    auto brState = rd.GetTechState("basicRocketry");
    REQUIRE(brState != nullptr);
    CHECK(brState->state == RDTech_State::Available);
    CHECK(brState->scienceCost == br->scienceCost);
    for (const auto& ap : g.data.parts.loadedParts) {
        const bool inNode = ap->TechRequired == "basicRocketry";
        CHECK((std::find(brState->partsPurchased.begin(), brState->partsPurchased.end(), ap.get()) != brState->partsPurchased.end()) == inNode);
        if (inNode) CHECK(ResearchAndDevelopment::PartTechAvailable(ap.get()));
    }

    // subjects (via the experiment API) and a submission
    ScienceExperiment* goo = ResearchAndDevelopment::GetExperiment("mysteryGoo");
    ScienceExperiment* thermo = ResearchAndDevelopment::GetExperiment("temperatureScan");
    REQUIRE(goo != nullptr);
    REQUIRE(thermo != nullptr);
    auto s1 = ResearchAndDevelopment::GetExperimentSubject(*goo, ExperimentSituations::SrfLanded, BodyOf("Kerbin"), "Shores", "");
    auto s2 = ResearchAndDevelopment::GetExperimentSubject(*thermo, ExperimentSituations::InSpaceLow, BodyOf("Mun"), "Craters", "");
    CHECK(s1->id == "mysteryGoo@KerbinSrfLandedShores");
    CHECK(s2->id == "temperatureScan@MunInSpaceLowCraters");
    rd.SubmitScienceData(goo->baseValue * goo->dataScale, *s1, 0.5f);
    CHECK(s1->science > 0.0f);
    // the same id again is the registered subject, refreshed (:677)
    s1->title = "stale";
    auto again = ResearchAndDevelopment::GetExperimentSubject(*goo, ExperimentSituations::SrfLanded, BodyOf("Kerbin"), "Shores", "");
    CHECK(again == s1);
    CHECK(again->title != "stale");
    CHECK(ResearchAndDevelopment::GetSubjects().size() == 2);

    // OnSave -> OnLoad -> OnSave is the identity
    ConfigNode saved("SCENARIO");
    rd.OnSave(saved);
    CHECK(saved.GetValue("sci") == dotnet::FormatSingleR(rd.Science()));
    CHECK(saved.GetNodes("Tech").size() == 2);      // start + basicRocketry (both Available)
    CHECK(saved.GetNodes("Science").size() == 2);
    REQUIRE(saved.HasNode("ExpParts"));
    CHECK(saved.GetNode("ExpParts")->GetValue(locked->name) == "1");
    ResearchAndDevelopment::ResetStatics();
    ResearchAndDevelopment rd2;  // a second instance: Instance now points to rd2 (the C# logs an error and keeps the newest)
    rd2.Awake();
    auto copy = saved.CreateCopy();
    rd2.OnLoad(*copy);
    ConfigNode saved2("SCENARIO");
    rd2.OnSave(saved2);
    CHECK(saved2.SaveToString() == saved.SaveToString());
    CHECK(rd2.Science() == rd.Science());
    CHECK(ResearchAndDevelopment::GetTechnologyState("basicRocketry") == RDTech_State::Available);
    auto s1b = ResearchAndDevelopment::GetSubjectByID("mysteryGoo@KerbinSrfLandedShores");
    REQUIRE(s1b != nullptr);
    CHECK(Bits(s1b->science) == Bits(s1->science));
    CHECK(Bits(s1b->scientificValue) == Bits(s1->scientificValue));
    CHECK(Bits(s1b->subjectValue) == Bits(s1->subjectValue));
    CHECK(Bits(s1b->scienceCap) == Bits(s1->scienceCap));
    CHECK(s1b->applyScienceScale == s1->applyScienceScale);
    CHECK(ResearchAndDevelopment::IsExperimentalPart(locked));
    ResearchAndDevelopment::RemoveExperimentalPart(locked);
}

// ScienceSubject / ScienceData Save / Load (ScienceSubject.cs:143/:163, ScienceData.cs:220/:250), the explicit-argument constructors and CopyOf
TEST_CASE("WP-35 ScienceSubject and ScienceData: constructors, Save / Load round trips, titles from the subject id") {
    if (!GameAvailable()) return;
    SciGuard guard;
    Game15 g;
    guard.Wire(g);
    std::vector<CelestialBody*> bodies;
    for (const auto& b : KerbolSystemPrefab().bodies) bodies.push_back(const_cast<CelestialBody*>(&b.body));
    FlightGlobals::bodies = &bodies;
    struct R {
        ~R() { FlightGlobals::bodies = nullptr; }
    } r;
    ScienceSubject plain("rec@x", "Title", 2.0f, 3.0f, 9.0f);
    CHECK(plain.applyScienceScale);
    CHECK(plain.scientificValue == 1.0f);
    CHECK(plain.science == 0.0f);
    ScienceExperiment* exp = ResearchAndDevelopment::GetExperiment("barometerScan");
    REQUIRE(exp != nullptr);
    const CelestialBody& kerbin = BodyOf("Kerbin");
    // the asteroid / comet constructor (:91): sourceUid != "" appends "_<uid>" after the biome; subjectValue 1
    ScienceSubject ast(*exp, ExperimentSituations::InSpaceLow, "Ast-ABC-123", "Ast. ABC-123", kerbin, "", "");
    CHECK(ast.id == "barometerScan@KerbinInSpaceLow_Ast-ABC-123");
    CHECK(ast.subjectValue == 1.0f);
    CHECK(Bits(ast.scienceCap) == Bits(exp->scienceCap));
    ScienceSubject nouid(*exp, ExperimentSituations::FlyingLow, "", "", kerbin, "", "");
    CHECK(nouid.id == "barometerScan@KerbinFlyingLow");
    CHECK(nouid.IsFromBody(kerbin));
    CHECK(!nouid.IsFromBody(BodyOf("Mun")));
    CHECK(nouid.IsFromSituation(ExperimentSituations::FlyingLow));
    CHECK(!nouid.IsFromSituation(ExperimentSituations::SrfLanded));
    CHECK(nouid.HasPartialIDstring("Kerbin"));
    // the id forms parse back
    std::string bodyName, biome, sit;
    ScienceUtil::GetExperimentFieldsFromScienceID("mysteryGoo@KerbinSrfLandedShores", bodyName, sit, biome);
    CHECK(bodyName == "Kerbin");
    CHECK(sit == "SrfLanded");
    CHECK(biome == "Shores");
    ExperimentSituations es;
    ScienceUtil::GetExperimentFieldsFromScienceID("mysteryGoo@MunInSpaceHigh", bodyName, es, biome);
    CHECK(bodyName == "Mun");
    CHECK(es == ExperimentSituations::InSpaceHigh);
    CHECK(biome.empty());
    CHECK(ScienceUtil::GetExperimentBodyName("mysteryGoo@KerbinSrfLandedShores") == "Kerbin");
    CHECK(ScienceUtil::GetExperimentBodyName("nonsense").empty());
    // titles: GenerateLocalizedTitle(id) is GenerateScienceSubjectTitle of the parsed fields (ScienceUtil.cs:129)
    ScienceExperiment* goo = ResearchAndDevelopment::GetExperiment("mysteryGoo");
    CHECK(ScienceUtil::GenerateLocalizedTitle("mysteryGoo@KerbinSrfLandedShores") ==
          ScienceUtil::GenerateScienceSubjectTitle(*goo, ExperimentSituations::SrfLanded, kerbin, "Shores"));
    CHECK(ScienceUtil::GenerateLocalizedTitle("mysteryGoo@KerbinFlyingHigh") == ScienceUtil::GenerateScienceSubjectTitle(*goo, ExperimentSituations::FlyingHigh, kerbin));
    CHECK(ScienceUtil::GenerateLocalizedTitle("mysteryGoo@KerbinFlyingHigh") ==
          Loc35::Format("#autoLOC_301735", std::vector<std::string>{goo->experimentTitle, kerbin.bodyDisplayName}));
    CHECK(ScienceUtil::GenerateLocalizedTitle("garbage").empty());
    CHECK(ScienceUtil::GenerateLocalizedTitle("noSuchExperiment@KerbinSrfLanded").empty());
    // the recovery experiment's ids (ScienceUtil.generateRecoveryLocalizedTitle :190)
    CHECK(ScienceUtil::GenerateLocalizedTitle("recovery@KerbinSurfaced") ==
          Loc35::Format("#autoLOC_6001096", std::vector<std::string>{Loc35::Format("#autoLOC_7001301", std::vector<std::string>{kerbin.bodyDisplayName})}));
    // ScienceSubject.Save / Load
    ScienceSubject a(*goo, ExperimentSituations::SrfLanded, kerbin, "Shores", "");
    a.science = 4.25f;
    a.scientificValue = 0.625f;
    ConfigNode n("Science");
    a.Save(n);
    ScienceSubject b(n);
    CHECK(b.id == a.id);
    CHECK(b.title == a.title);
    CHECK(Bits(b.science) == Bits(a.science));
    CHECK(Bits(b.scientificValue) == Bits(a.scientificValue));
    CHECK(Bits(b.subjectValue) == Bits(a.subjectValue));
    CHECK(Bits(b.scienceCap) == Bits(a.scienceCap));
    CHECK(Bits(b.dataScale) == Bits(a.dataScale));
    CHECK(b.applyScienceScale == a.applyScienceScale);
    // ScienceData
    ScienceData d(10.0f, 0.75f, 0.5f, 0.25f, "mysteryGoo@KerbinSrfLandedShores", "Goo", true, 1234u, "extra");
    ConfigNode dn("ScienceData");
    d.Save(dn);
    CHECK(dn.GetValue("data") == "10");
    CHECK(dn.GetValue("extraResultString") == "extra");
    ScienceData d2(dn);
    CHECK(d2.subjectID == d.subjectID);
    CHECK(d2.dataAmount == 10.0f);
    CHECK(d2.scienceValueRatio == 0.75f);
    CHECK(d2.baseTransmitValue == 0.5f);
    CHECK(d2.transmitBonus == 0.25f);
    CHECK(d2.triggered);
    CHECK(d2.container == 1234u);
    CHECK(d2.extraResultString == "extra");
    CHECK(d2.title == ScienceUtil::GenerateLocalizedTitle(d.subjectID, true));  // the title is regenerated from the id
    auto cp = ScienceData::CopyOf(&d);
    CHECK(cp != nullptr);
    CHECK(cp->container == d.container);
    CHECK(ScienceData::CopyOf(nullptr) == nullptr);
    ScienceData noExtra(1.0f, 1.0f, 1.0f, 0.0f, "temperatureScan@KerbinFlyingLow", "t");
    ConfigNode ne("ScienceData");
    noExtra.Save(ne);
    CHECK(!ne.HasValue("extraResultString"));
    // a bare experiment id (no '@') takes the experiment title
    ConfigNode be("ScienceData");
    be.AddValue("subjectID", "mysteryGoo");
    be.AddValue("data", "5");
    ScienceData bare(be);
    CHECK(bare.title == goo->experimentTitle);
    CHECK(bare.scienceValueRatio == 1.0f);
    // KSP quirk: ScienceData.Load of an unknown bare id dereferences a null experiment
    ConfigNode bad("ScienceData");
    bad.AddValue("subjectID", "noSuchExperiment");
    CHECK_THROWS(ScienceData{bad});
}

// ============================================================================================== the stock cfg: every science module of every stock part

TEST_CASE("WP-35 stock cfg: every ModuleScienceExperiment / Container / Lab / Converter / EnviroSensor / SampleContainer is the real class with its cfg state") {
    if (!GameAvailable()) return;
    SciGuard guard;
    Game15 g;
    guard.Wire(g);
    size_t experiments = 0, containers = 0, labs = 0, converters = 0, sensors = 0, samples = 0, evaSits = 0;
    auto cfgF = [](const PartModuleData& d, const char* k, float def) {
        const std::optional<std::string> v = d.config->GetValue(k);
        return v ? dotnet::ParseSingle(*v) : def;
    };
    auto cfgI = [](const PartModuleData& d, const char* k, int def) {
        const std::optional<std::string> v = d.config->GetValue(k);
        return v ? dotnet::ParseInt32(*v) : def;
    };
    auto cfgB = [](const PartModuleData& d, const char* k, bool def) {
        const std::optional<std::string> v = d.config->GetValue(k);
        return v ? (*v == "True" || *v == "true") : def;
    };
    for (const auto& apu : g.data.parts.loadedParts) {
        const AvailablePart* ap = apu.get();
        if (!ap->partPrefab) continue;
        for (const PartModuleData& d : ap->partPrefab->ModuleData) {
            INFO(ap->name);
            if (!d.module) continue;
            if (d.module->Class().Is("ModuleScienceExperiment")) {
                auto* m = dynamic_cast<ModuleScienceExperiment*>(d.module.get());
                REQUIRE(m != nullptr);
                experiments++;
                std::string id = *d.config->GetValue("experimentID");
                while (!id.empty() && id.back() == ' ') id.pop_back();
                CHECK(m->Get<std::string>("experimentID") == id);
                CHECK(m->Get<int>("usageReqMaskInternal") == cfgI(d, "usageReqMaskInternal", 1));
                CHECK(m->Get<int>("usageReqMaskExternal") == cfgI(d, "usageReqMaskExternal", -1));
                CHECK(m->Get<bool>("rerunnable") == cfgB(d, "rerunnable", false));
                CHECK(m->Get<bool>("resettable") == cfgB(d, "resettable", true));
                CHECK(m->Get<bool>("useStaging") == cfgB(d, "useStaging", false));
                CHECK(Bits(m->Get<float>("xmitDataScalar")) == Bits(cfgF(d, "xmitDataScalar", 0.5f)));
                CHECK(Bits(m->Get<float>("scienceValueRatio")) == Bits(cfgF(d, "scienceValueRatio", 1.0f)));
                CHECK(m->IsStageable() == m->Get<bool>("useStaging"));
                CHECK(m->GetScienceCount() == 0);
                CHECK(m->IsRerunnable() == m->Get<bool>("rerunnable"));
                const size_t nEva = d.config->GetNodes("EVASITUATION").size();
                CHECK(m->evaSituations.has_value() == (nEva > 0));
                if (m->evaSituations) {
                    // every stock kerbalAction (PlayGolf, Banana, Dzhanibekov) is a KerbalEVA method: all nodes are kept
                    CHECK(m->evaSituations->size() == nEva);
                    evaSits += nEva;
                }
            } else if (d.module->Class().Is("ModuleScienceContainer")) {
                auto* m = dynamic_cast<ModuleScienceContainer*>(d.module.get());
                REQUIRE(m != nullptr);
                containers++;
                CHECK(m->Get<int>("capacity") == cfgI(d, "capacity", 0));
                CHECK(Bits(m->Get<float>("storageRange")) == Bits(cfgF(d, "storageRange", 1.0f)));
                CHECK(m->Get<bool>("evaOnlyStorage") == cfgB(d, "evaOnlyStorage", true));
                CHECK(m->Get<bool>("canBeTransferredToInVessel") == cfgB(d, "canBeTransferredToInVessel", false));
                CHECK(m->Get<bool>("allowRepeatedSubjects") == cfgB(d, "allowRepeatedSubjects", false));
                CHECK(m->Get<bool>("dataIsRecoverable") == cfgB(d, "dataIsRecoverable", true));
                CHECK(m->GetScienceCount() == 0);
                CHECK(m->IsRerunnable());
            } else if (d.module->Class().Is("ModuleScienceLab")) {
                auto* m = dynamic_cast<ModuleScienceLab*>(d.module.get());
                REQUIRE(m != nullptr);
                labs++;
                CHECK(Bits(m->Get<float>("dataStorage")) == Bits(cfgF(d, "dataStorage", 500.0f)));
                CHECK(Bits(m->Get<float>("crewsRequired")) == Bits(cfgF(d, "crewsRequired", 0.0f)));
                CHECK(m->GetScienceCount() == 0);
                CHECK(!m->IsRerunnable());
                // the RESOURCE_PROCESS nodes of the prefab (loaded by OnLoad)
                CHECK(m->processResources.size() == d.config->GetNodes("RESOURCE_PROCESS").size());
            } else if (d.module->Class().Is("ModuleScienceConverter")) {
                auto* m = dynamic_cast<ModuleScienceConverter*>(d.module.get());
                REQUIRE(m != nullptr);
                converters++;
                CHECK(m->Get<int>("scienceCap") == cfgI(d, "scienceCap", 500));
                CHECK(m->IsSituationValid());
            } else if (d.module->Class().Is("ModuleEnviroSensor")) {
                auto* m = dynamic_cast<ModuleEnviroSensor*>(d.module.get());
                REQUIRE(m != nullptr);
                sensors++;
                const std::optional<std::string> t = d.config->GetValue("sensorType");
                CHECK(m->EnumName("sensorType") == (t ? *t : "TEMP"));
                // OnLoad (:80): an ElectricCharge input at powerConsumption (default 0.0075) when the node names no input of its own
                REQUIRE(!m->resHandler.inputResources.empty());
            } else if (d.module->Class().Is("ModuleSampleContainer")) {
                REQUIRE(dynamic_cast<ModuleSampleContainer*>(d.module.get()) != nullptr);
                samples++;
            }
        }
    }
    CHECK(experiments >= 25);
    CHECK(containers >= 15);
    CHECK(labs >= 1);
    CHECK(converters >= 1);
    CHECK(sensors >= 4);
    CHECK(evaSits >= 3);
    MESSAGE(experiments << " experiments, " << containers << " containers, " << labs << " labs, " << converters << " converters, " << sensors << " sensors, "
                        << samples << " sample containers, " << evaSits << " EVA situations");
}

// ============================================================================================== ModuleScienceContainer

namespace {
// An IScienceDataContainer that is not a ModuleScienceContainer (an experiment stand-in whose behaviour is exactly the interface)
class FakeContainer : public IScienceDataContainer {
public:
    std::vector<std::shared_ptr<ScienceData>> data;
    std::vector<std::string> dumped;
    std::vector<std::shared_ptr<ScienceData>> GetData() override { return data; }
    void ReturnData(const std::shared_ptr<ScienceData>& d) override { data.push_back(d); }
    void DumpData(const std::shared_ptr<ScienceData>& d) override {
        dumped.push_back(d->subjectID);
        data.erase(std::remove(data.begin(), data.end(), d), data.end());
    }
    void ReviewData() override {}
    void ReviewDataItem(const std::shared_ptr<ScienceData>&) override {}
    int GetScienceCount() override { return static_cast<int>(data.size()); }
    bool IsRerunnable() override { return false; }
    void Add(const std::string& id) { data.push_back(std::make_shared<ScienceData>(5.0f, 1.0f, 0.5f, 0.0f, id, "t " + id)); }
};
}  // namespace

TEST_CASE("WP-35 ModuleScienceContainer: StoreData (capacity, repeated subjects, order), AddData, HasData, ReturnData, OnSave / OnLoad") {
    if (!GameAvailable()) return;
    SciGuard guard;
    Game15 g;
    guard.Wire(g);
    auto pod = g.New("mk1pod.v2");
    pod->fields.flightID = 777;
    auto* cont = ModuleOf<ModuleScienceContainer>(*pod, "ModuleScienceContainer");
    cont->OnStart(StartState::PreLaunch);
    cont->Set<int>("capacity", 3);
    FakeContainer a, b, c;
    a.Add("mysteryGoo@KerbinSrfLanded");
    a.Add("temperatureScan@KerbinSrfLanded");
    b.Add("barometerScan@KerbinFlyingLow");
    std::vector<IScienceDataContainer*> list{&a, &b};
    // ModuleScienceContainer.sortContainerList (:398): ascending science count among the non-containers, then the containers
    CHECK(cont->StoreData(list, false));
    REQUIRE(cont->dataStore.size() == 3);
    CHECK(cont->dataStore[0]->subjectID == "barometerScan@KerbinFlyingLow");  // b (1 item) before a (2 items)
    CHECK(cont->dataStore[1]->subjectID == "mysteryGoo@KerbinSrfLanded");
    CHECK(cont->dataStore[2]->subjectID == "temperatureScan@KerbinSrfLanded");
    for (auto& d : cont->dataStore) CHECK(d->container == 777u);  // AddData: data.container = part.flightID
    CHECK(a.data.empty());
    CHECK(b.data.empty());
    CHECK(a.dumped == std::vector<std::string>{"mysteryGoo@KerbinSrfLanded", "temperatureScan@KerbinSrfLanded"});
    CHECK(cont->GetScienceCount() == 3);
    CHECK(cont->HasData(ScienceData(1.0f, 1.0f, 1.0f, 0.0f, "temperatureScan@KerbinSrfLanded", "x")));
    CHECK(!cont->HasData(ScienceData(1.0f, 1.0f, 1.0f, 0.0f, "atmosphereAnalysis@KerbinFlyingLow", "x")));
    // full: a container with 1 more item does not fit (scienceCount + dataStore.Count > capacity)
    c.Add("gravityScan@MunInSpaceLow");
    std::vector<IScienceDataContainer*> l2{&c};
    CHECK(!cont->StoreData(l2, false));
    CHECK(c.data.size() == 1);
    CHECK(cont->dataStore.size() == 3);
    // ReturnData / AddData respect the capacity
    CHECK(!cont->AddData(c.data[0]));
    cont->ReturnData(c.data[0]);
    CHECK(cont->dataStore.size() == 3);
    // unlimited: a repeated subject is refused (dumpRepeats false keeps it in the source), or dumped from the source (dumpRepeats true)
    cont->Set<int>("capacity", 0);
    FakeContainer r;
    r.Add("mysteryGoo@KerbinSrfLanded");
    std::vector<IScienceDataContainer*> l3{&r};
    CHECK(!cont->StoreData(l3, false));
    CHECK(r.data.size() == 1);
    std::vector<IScienceDataContainer*> l4{&r};
    CHECK(cont->StoreData(l4, true));
    CHECK(r.data.empty());
    CHECK(r.dumped == std::vector<std::string>{"mysteryGoo@KerbinSrfLanded"});
    CHECK(cont->dataStore.size() == 3);
    // repeated subjects allowed
    cont->Set<bool>("allowRepeatedSubjects", true);
    FakeContainer r2;
    r2.Add("mysteryGoo@KerbinSrfLanded");
    std::vector<IScienceDataContainer*> l5{&r2};
    CHECK(cont->StoreData(l5, false));
    CHECK(cont->dataStore.size() == 4);
    cont->ReturnData(std::make_shared<ScienceData>(1.0f, 1.0f, 1.0f, 0.0f, "mysteryGoo@KerbinSrfLanded", "again"));
    CHECK(cont->dataStore.size() == 5);
    cont->Set<bool>("allowRepeatedSubjects", false);
    cont->ReturnData(std::make_shared<ScienceData>(1.0f, 1.0f, 1.0f, 0.0f, "mysteryGoo@KerbinSrfLanded", "again"));
    CHECK(cont->dataStore.size() == 5);
    // a container among the sources is processed after the plain ones (priority false), and excludeScienceContainers drops it
    auto pod2 = g.New("mk1pod.v2");
    auto* cont2 = ModuleOf<ModuleScienceContainer>(*pod2, "ModuleScienceContainer");
    cont2->dataStore.clear();
    cont2->dataStore.push_back(std::make_shared<ScienceData>(1.0f, 1.0f, 1.0f, 0.0f, "seismicScan@KerbinSrfLanded", "seismicScan@KerbinSrfLanded"));
    FakeContainer f1;
    f1.Add("infraredTelescope@KerbinInSpaceHigh");
    f1.Add("surfaceSample@KerbinSrfLanded");
    auto pod3 = g.New("mk1pod.v2");
    auto* cont3 = ModuleOf<ModuleScienceContainer>(*pod3, "ModuleScienceContainer");
    std::vector<IScienceDataContainer*> l6{cont2, &f1};
    CHECK(cont3->StoreData(l6, false));
    REQUIRE(cont3->dataStore.size() == 3);
    CHECK(cont3->dataStore[0]->subjectID == "infraredTelescope@KerbinInSpaceHigh");  // plain containers first (:437)
    CHECK(cont3->dataStore[1]->subjectID == "surfaceSample@KerbinSrfLanded");
    CHECK(cont3->dataStore[2]->subjectID == "seismicScan@KerbinSrfLanded");
    auto pod4 = g.New("mk1pod.v2");
    auto* cont4 = ModuleOf<ModuleScienceContainer>(*pod4, "ModuleScienceContainer");
    cont2->dataStore.push_back(std::make_shared<ScienceData>(1.0f, 1.0f, 1.0f, 0.0f, "magnetometer@KerbinInSpaceLow", "magnetometer@KerbinInSpaceLow"));
    FakeContainer f2;
    f2.Add("crewReport@KerbinFlyingHigh");
    std::vector<IScienceDataContainer*> l7{cont2, &f2};
    CHECK(cont4->StoreData(l7, false, true));
    REQUIRE(cont4->dataStore.size() == 1);  // the container was excluded
    CHECK(cont4->dataStore[0]->subjectID == "crewReport@KerbinFlyingHigh");
    CHECK(l7.size() == 1);
    // RemoveData / DumpData / GetData
    auto first = cont->dataStore[0];
    CHECK(cont->GetData().size() == cont->dataStore.size());
    cont->RemoveData(first);
    CHECK(cont->dataStore.size() == 4);
    auto second = cont->dataStore[0];
    cont->DumpData(second);
    CHECK(cont->dataStore.size() == 3);
    // OnSave names the nodes by dataIsRecoverable; OnLoad reads both (ScienceData first, then CommsData)
    ConfigNode n1("MODULE");
    cont->OnSave(n1);
    CHECK(n1.GetNodes("ScienceData").size() == 3);
    CHECK(n1.GetNodes("CommsData").empty());
    cont->Set<bool>("dataIsRecoverable", false);
    ConfigNode n2("MODULE");
    cont->OnSave(n2);
    CHECK(n2.GetNodes("CommsData").size() == 3);
    ConfigNode mixed("MODULE");
    n1.GetNodes("ScienceData")[0]->CopyTo(*mixed.AddNode("ScienceData"));
    n2.GetNodes("CommsData")[1]->CopyTo(*mixed.AddNode("CommsData"));
    auto pod5 = g.New("mk1pod.v2");
    auto* cont5 = ModuleOf<ModuleScienceContainer>(*pod5, "ModuleScienceContainer");
    cont5->OnLoad(mixed);
    REQUIRE(cont5->dataStore.size() == 2);
    CHECK(cont5->dataStore[0]->subjectID == cont->dataStore[0]->subjectID);
    CHECK(cont5->dataStore[1]->subjectID == cont->dataStore[1]->subjectID);
    // ExperimentTransfer.IsValidPart (:112): a part with a container that canBeTransferredToInVessel
    CHECK(ExperimentTransfer::IsValidPart(*pod) == cont->Get<bool>("canBeTransferredToInVessel"));
    CHECK(!ExperimentTransfer::IsSemiValidPart(*pod));
    auto goo = g.New("GooExperiment");
    CHECK(!ExperimentTransfer::IsValidPart(*goo));  // no container module
}

// ============================================================================================== science flow on a real vessel

namespace {
struct World35V {
    GameData data;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    World35V() {
        SaveEnvironment::Reset();
        FlightRegistry::Clear();
        Vessel::SpawnedVessels().clear();
        GameData::Options o;
        o.gameRoot = ksp::testing::GameRoot();
        data.Load(o);
        SaveEnvironment& env = SaveEnvironment::Current();
        env.getPartInfoByName = [this](std::string_view n) { return data.parts.getPartInfoByName(n); };
        env.partExists = [this](std::string_view n) { return data.parts.getPartInfoByName(n) != nullptr; };
        env.newGuidHashCode = [] {
            static uint32_t s = 4242u;
            s = s * 1664525u + 1013904223u;
            return static_cast<int32_t>(s);
        };
        support::InstallUpgradeHooks(data);
        pg.physicsDatabaseFilename = ksp::testing::GameRoot() + "/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        env.localBodies = sys->SpawnOrder();
        Vessel::env = VesselEnvironment{};
        VesselModuleManager::CompileModules();
        env.loadedScene = GameScenes::FLIGHT;
        ResourceSetup::database = &data.database;
        ResourceSetup::ResetConfig();
    }
    ~World35V() {
        Vessel::SpawnedVessels().clear();
        FlightRegistry::Clear();
        FlightRegistry::ActiveVessel = nullptr;
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
        ResourceSetup::database = nullptr;
        ResourceSetup::ResetConfig();
        SaveEnvironment::Reset();
        Vessel::env = VesselEnvironment{};
    }
    CelestialBody& Kerbin() { return *sys->GetBodyByName("Kerbin"); }
    Vector3f PlaceKerbinAtPad() {
        CelestialBody& k = Kerbin();
        k.position = Vector3d();
        const Vector3d pad = k.GetWorldSurfacePosition(-0.0972077486779459, -74.5576813534722, 75.0);
        k.position = Vector3d() - pad;
        return Vector3f();
    }
    std::unique_ptr<Vessel> Stack(std::initializer_list<const char*> names, uint32_t base) {
        std::vector<std::unique_ptr<Part>> owned;
        uint32_t id = base;
        for (const char* n : names) {
            const AvailablePart* ap = data.parts.getPartInfoByName(n);
            REQUIRE_MESSAGE(ap != nullptr, n);
            auto p = Part::Instantiate(*ap->partPrefab);
            p->partInfo = ap;
            p->gameObjectName = ap->name;
            p->fields.flightID = id++;
            p->fields.persistentId = id * 7;
            p->UpdateMass();
            if (!owned.empty()) p->setParent(owned.back().get());
            owned.push_back(std::move(p));
        }
        auto v = std::make_unique<Vessel>();
        Part* root = owned.front().get();
        root->partTransform.position = PlaceKerbinAtPad();
        v->vesselTransform = root->partTransform;
        v->AdoptParts(std::move(owned), root);
        v->vesselName = "Test vessel";
        REQUIRE(v->Initialize(true, false, false, false));
        v->loaded = true;
        if (std::find(FlightRegistry::VesselsLoaded.begin(), FlightRegistry::VesselsLoaded.end(), v.get()) == FlightRegistry::VesselsLoaded.end()) FlightRegistry::AddVessel(v.get());
        v->packed = false;
        for (Part* p : v->parts) p->fields.packed = false;
        return v;
    }
};

// ModuleDataTransmitter stand-in (WP-39 owns the real one): records what it is asked to transmit
class FakeTransmitter : public IScienceDataTransmitter {
public:
    float DataRate() override { return 2.0f; }
    double DataResourceCost() override { return 0.0; }
    bool CanTransmit() override { return true; }
    bool IsBusy() override { return false; }
    void TransmitData(std::vector<std::shared_ptr<ScienceData>>& q) override {
        for (auto& d : q) sent.push_back(d);
    }
    std::vector<std::shared_ptr<ScienceData>> sent;
};

void RunFrames(int n) {
    for (int i = 0; i < n; i++) {
        ScienceEnvironment::Scheduler().AdvanceFrame(0.02f);
        ScienceEnvironment::Scheduler().RunDelayedFixedFrameRate();
        ScienceEnvironment::Scheduler().RunDelayedDynamicFrameRate();
        ScienceEnvironment::Scheduler().RunEndOfFrame();
    }
}
}  // namespace

TEST_CASE("WP-35 ModuleScienceExperiment on a vessel: deploy, data creation, review page, transmit / lab / container, inoperable, reset") {
    if (!GameAvailable()) return;
    SciGuard guard;
    World35V w;
    Game15* none = nullptr;
    (void)none;
    ScienceEnvironment::getConfigNodes = [&w](std::string_view t) { return w.data.database.GetConfigNodes(t); };
    ScienceEnvironment::loadedParts = [&w] {
        std::vector<const AvailablePart*> v;
        for (const auto& p : w.data.parts.loadedParts) v.push_back(p.get());
        return v;
    };
    double ut = 100.0;
    ScienceEnvironment::universalTime = [&ut] { return ut; };
    CelestialBody& kerbin = w.Kerbin();
    REQUIRE(kerbin.BiomeMap.has_value());
    REQUIRE(kerbin.BiomeMap->Attributes.size() > 3);
    ScienceEnvironment::biomeAtt = [&kerbin](const CelestialBody& b, double, double) -> const CBAttributeMapSO::MapAttribute* {
        return &b.BiomeMap->Attributes[3];
    };
    ResearchAndDevelopment rd;
    rd.Awake();
    auto v = w.Stack({"mk1pod.v2", "GooExperiment", "Large.Crewed.Lab"}, 1000);
    FlightRegistry::ActiveVessel = v.get();
    Part* pod = v->parts[0];
    Part* gooPart = v->parts[1];
    Part* labPart = v->parts[2];
    auto* exp = ModuleOf<ModuleScienceExperiment>(*gooPart, "ModuleScienceExperiment");
    auto* podCont = ModuleOf<ModuleScienceContainer>(*pod, "ModuleScienceContainer");
    auto* lab = ModuleOf<ModuleScienceLab>(*labPart, "ModuleScienceLab");
    exp->OnStart(StartState::PreLaunch);
    podCont->OnStart(StartState::PreLaunch);
    lab->OnStart(StartState::PreLaunch);
    REQUIRE(exp->experiment != nullptr);
    CHECK(*exp->experiment->id == "mysteryGoo");
    CHECK(exp->hasContainer == podCont->Get<bool>("canBeTransferredToInVessel"));
    const ExperimentSituations expectedSit = ScienceUtil::GetExperimentSituation(*v);
    CHECK((v->situation == VesselSituations::PRELAUNCH || v->situation == VesselSituations::LANDED));
    CHECK(expectedSit == ExperimentSituations::SrfLanded);

    // RequiredUsageInternalAvailable (ScienceUtil.cs:388): the goo needs vessel control (mask 1) and nothing else
    std::string msg;
    CHECK(ScienceUtil::RequiredUsageInternalAvailable(*v, *gooPart, ExperimentUsageReqs::Always, *exp->experiment, msg));
    CHECK(!ScienceUtil::RequiredUsageInternalAvailable(*v, *gooPart, ExperimentUsageReqs::CrewInVessel, *exp->experiment, msg));
    CHECK(msg == Loc35::Format("#autoLOC_302075", std::vector<std::string>{exp->experiment->experimentTitle}));
    CHECK(!ScienceUtil::RequiredUsageInternalAvailable(*v, *gooPart, ExperimentUsageReqs::CrewInPart, *exp->experiment, msg));
    CHECK(msg == Loc35::Format("#autoLOC_302084", std::vector<std::string>{exp->experiment->experimentTitle, gooPart->partInfo->title}));
    CHECK(!ScienceUtil::RequiredUsageInternalAvailable(*v, *gooPart, ExperimentUsageReqs::ScientistCrew, *exp->experiment, msg));
    CHECK(msg == Loc35::Format("#autoLOC_302109", std::vector<std::string>{exp->experiment->experimentTitle}));
    CHECK(!ScienceUtil::RequiredUsageInternalAvailable(*v, *gooPart, ExperimentUsageReqs::Never, *exp->experiment, msg));
    CHECK(msg.empty());

    // deploy: the goo needs no control or crew here (mask 0 for the test)
    exp->Set<int>("usageReqMaskInternal", 0);
    ScienceEnvironment::screenMessages.clear();
    exp->DeployExperiment();
    CHECK(exp->Get<bool>("Deployed"));
    REQUIRE(exp->HasExperimentData());
    CHECK(exp->subject != nullptr);
    CHECK(exp->subject->id.rfind("mysteryGoo@KerbinSrfLanded", 0) == 0);
    CHECK(exp->subject == ResearchAndDevelopment::GetSubjectByID(exp->subject->id));
    CHECK(exp->experimentData->subjectID == exp->subject->id);
    CHECK(exp->experimentData->title == exp->subject->title);
    CHECK(exp->experimentData->dataAmount == 10.0f);  // baseValue 10 * dataScale 1 (ScienceDefs.cfg)
    CHECK(exp->experimentData->container == gooPart->fields.flightID);
    CHECK(exp->experimentData->baseTransmitValue == exp->Get<float>("xmitDataScalar"));
    CHECK(exp->experimentData->scienceValueRatio == 1.0f);
    CHECK(exp->GetScienceCount() == 1);
    CHECK(ScienceEnvironment::shownPages.empty());  // the dialog waits for UT > dialogTimer
    ut = 100.5;
    RunFrames(1);
    REQUIRE(ScienceEnvironment::shownPages.size() == 1);
    auto page = ScienceEnvironment::shownPages[0];
    CHECK(exp->resultsDialog == page);
    // the page values (ExperimentResultDialogPage.cs:64): derived from GetScienceValue and the career multiplier (1)
    ScienceSubject& subj = *exp->subject;
    CHECK(page->dataSize == 10.0f);
    CHECK(Bits(page->refValue) == Bits(ResearchAndDevelopment::GetReferenceDataValue(10.0f, subj)));
    CHECK(Bits(page->scienceValue) == Bits(ResearchAndDevelopment::GetScienceValue(10.0f, 1.0f, subj)));
    CHECK(Bits(page->valueAfterRecovery) == Bits(ResearchAndDevelopment::GetNextScienceValue(10.0f, subj)));
    CHECK(page->showTransmitWarning);  // !rerunnable
    CHECK(page->title == exp->experiment->experimentTitle);
    CHECK(page->resultText.rfind(exp->experimentData->title + ":\n\n", 0) == 0);
    // transmitting without a transmitter keeps the data and says so (autoLOC_238507: CommNet is off)
    ScienceEnvironment::screenMessages.clear();
    page->OnTransmitData(page->pageData);
    CHECK(exp->HasExperimentData());
    CHECK(exp->resultsDialog == nullptr);
    REQUIRE(!ScienceEnvironment::screenMessages.empty());
    CHECK(ScienceEnvironment::screenMessages.back() == Loc35::Format("#autoLOC_238507"));
    CHECK(!exp->Get<bool>("Inoperable"));
    // keep it: the review event shows the page again
    exp->ReviewDataEvent();
    REQUIRE(ScienceEnvironment::shownPages.size() == 2);
    auto page2 = ScienceEnvironment::shownPages[1];
    // with a transmitter the data is handed over, the experiment becomes inoperable and the data is dumped (not resettable-by-itself: endExperiment)
    FakeTransmitter tx;
    ScienceEnvironment::commNetBestTransmitter = [&tx](Vessel&, IScienceDataTransmitter*& r) {
        r = &tx;
        return true;
    };
    auto sentData = exp->experimentData;
    page2->OnTransmitData(page2->pageData);
    REQUIRE(tx.sent.size() == 1);
    CHECK(tx.sent[0] == sentData);
    CHECK(exp->Get<bool>("Inoperable"));
    CHECK(exp->Get<bool>("Deployed"));
    CHECK(!exp->HasExperimentData());  // resettable && !Inoperable is false -> dumpData
    CHECK(!exp->Events()["DeployExperiment"]->active);
    CHECK(!exp->Events()["ResetExperiment"]->active);  // Inoperable
    // a lab cleaning (CleanUpExperiment) or the Kerbal's ScienceResetSkill makes it operable again: Inoperable=false then ResetExperiment
    exp->Set<bool>("Inoperable", false);
    exp->ResetExperiment();
    RunFrames(2);
    CHECK(!exp->Get<bool>("Deployed"));
    CHECK(!exp->Get<bool>("Inoperable"));
    CHECK(exp->Events()["DeployExperiment"]->active);

    // deploy again: the same subject, the R&D entry is reused
    ut = 200.0;
    exp->DeployExperiment();
    ut = 200.5;
    RunFrames(1);
    REQUIRE(exp->HasExperimentData());
    CHECK(exp->subject == ResearchAndDevelopment::GetSubjectByID(exp->subject->id));
    // send to a lab: no manned lab -> the error is posted and the data stays
    REQUIRE(ScienceEnvironment::shownPages.size() == 3);
    auto page3 = ScienceEnvironment::shownPages[2];
    ScienceEnvironment::screenMessages.clear();
    page3->OnSendToLab(page3->pageData);
    CHECK(exp->HasExperimentData());
    CHECK(ScienceEnvironment::screenMessages.back() ==
          Loc35::Format("#autoLOC_239631", std::vector<std::string>{[&] {
                            std::string e = Loc35::Format("#autoLOC_6001034");  // one lab, none manned (ScienceLabSearch.ErrorString)
                            std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                            return e;
                        }()}));
    // man the lab (a Scientist: ScienceSkill) and send again
    ScienceLabSearch noLab(v.get(), exp->experimentData);
    CHECK(noLab.Error == ScienceLabSearch::SearchError::NO_MANNED);
    CHECK(noLab.LabsTotal == 1);
    CHECK(noLab.LabsManned == 0);
    CrewWorld* crewWorld = nullptr;
    (void)crewWorld;
    {
        std::vector<const ConfigNode*> traits;
        for (ConfigNode* n : w.data.database.GetConfigNodes("EXPERIENCE_TRAIT")) traits.push_back(n);
        SaveEnvironment::Current().experienceConfigs = std::make_shared<Experience::ExperienceSystemConfig>(traits);
        PCM sci = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Sci Kerman");
        sci->trait = "Scientist";
        for (const auto& cfg : SaveEnvironment::Current().experienceConfigs->Categories())
            if (cfg->Name() == "Scientist") sci->experienceTrait = Experience::ExperienceTrait::Create(cfg->Name(), cfg.get(), sci.get());
        REQUIRE(sci->experienceTrait);
        labPart->protoModuleCrew.push_back(sci);
    }
    ScienceLabSearch yesLab(v.get(), exp->experimentData);
    CHECK(yesLab.NextLabForDataFound());
    CHECK(yesLab.NextLabForData == lab);
    CHECK(yesLab.Error == ScienceLabSearch::SearchError::NONE);
    const std::string subjectId = exp->experimentData->subjectID;
    page3->OnSendToLab(page3->pageData);
    REQUIRE(lab->ExperimentData.size() == 1);
    CHECK(lab->ExperimentData[0] == subjectId);
    CHECK(Bits(lab->Get<float>("dataStored")) == Bits(0.0f));  // labValue is set by the page (UpdatePageLabValue); 0 here
    CHECK(exp->Get<bool>("Inoperable"));
    // the same subject cannot be stored twice (IsStorable :237)
    ScienceData again(10.0f, 1.0f, 0.5f, 0.0f, subjectId, "again");
    CHECK(!lab->IsStorable(again));
    ScienceLabSearch used(v.get(), std::make_shared<ScienceData>(again));
    CHECK(used.Error == ScienceLabSearch::SearchError::ALL_RESEARCHED);
    // the lab storage limit
    ScienceData huge(10.0f, 1.0f, 0.5f, 0.0f, "other@x", "huge");
    huge.labValue = 100000.0f;
    CHECK(!lab->IsStorable(huge));

    // the container route: collect into the pod container (TransferToContainer -> StoreData -> DumpData of the experiment)
    exp->Set<bool>("Inoperable", false);
    exp->ResetExperiment();
    RunFrames(2);
    ut = 300.0;
    exp->DeployExperiment();
    ut = 300.5;
    RunFrames(1);
    REQUIRE(exp->HasExperimentData());
    auto held = exp->experimentData;
    exp->TransferToContainer(podCont, "pod");
    REQUIRE(podCont->dataStore.size() == 1);
    CHECK(podCont->dataStore[0] == held);
    CHECK(held->container == pod->fields.flightID);
    CHECK(!exp->HasExperimentData());
    CHECK(exp->Get<bool>("Inoperable"));  // DumpData: !rerunnable -> SetInoperable
    // no container at all
    ScienceEnvironment::screenMessages.clear();
    exp->TransferToContainer(nullptr, "nowhere");
    CHECK(ScienceEnvironment::screenMessages.back() == Loc35::Format("#autoLOC_238572", std::vector<std::string>{gooPart->partInfo->title, "nowhere"}));
}

// ============================================================================================== ScienceUtil.GetExperimentSituation, RnDCommsStream, recovery

TEST_CASE("WP-35 ScienceUtil.GetExperimentSituation: the vessel situation and the body altitude thresholds") {
    if (!GameAvailable()) return;
    SciGuard guard;
    World35V w;
    auto v = w.Stack({"mk1pod.v2"}, 5000);
    const CelestialBody& kerbin = BodyOf("Kerbin");
    const double fly = kerbin.scienceValues.flyingAltitudeThreshold;
    const double space = kerbin.scienceValues.spaceAltitudeThreshold;
    CHECK(fly == 18000.0);
    CHECK(space == 250000.0);
    struct Row {
        VesselSituations s;
        double alt;
        ExperimentSituations expected;
    };
    const Row rows[] = {{VesselSituations::LANDED, 70.0, ExperimentSituations::SrfLanded},
                        {VesselSituations::PRELAUNCH, 70.0, ExperimentSituations::SrfLanded},
                        {VesselSituations::SPLASHED, 0.0, ExperimentSituations::SrfSplashed},
                        {VesselSituations::FLYING, fly - 1.0, ExperimentSituations::FlyingLow},
                        {VesselSituations::FLYING, fly, ExperimentSituations::FlyingHigh},
                        {VesselSituations::FLYING, fly + 1.0, ExperimentSituations::FlyingHigh},
                        {VesselSituations::SUB_ORBITAL, space - 1.0, ExperimentSituations::InSpaceLow},
                        {VesselSituations::SUB_ORBITAL, space, ExperimentSituations::InSpaceHigh},
                        {VesselSituations::ORBITING, 100000.0, ExperimentSituations::InSpaceLow},
                        {VesselSituations::ORBITING, 1e7, ExperimentSituations::InSpaceHigh},
                        {VesselSituations::ESCAPING, 1e9, ExperimentSituations::InSpaceHigh},
                        {VesselSituations::DOCKED, 100.0, ExperimentSituations::InSpaceLow}};
    for (const Row& r : rows) {
        v->situation = r.s;
        v->altitude = r.alt;
        CHECK(ScienceUtil::GetExperimentSituation(*v) == r.expected);
    }
}

// RnDCommsStream (:222): the data streamed in is submitted when the file is complete, or when the stream times out (xmitIncomplete)
TEST_CASE("WP-35 RnDCommsStream: complete file, timeout with and without xmitIncomplete") {
    if (!GameAvailable()) return;
    SciGuard guard;
    Game15 g;
    guard.Wire(g);
    ResearchAndDevelopment rd;
    rd.Awake();
    ScienceExperiment* goo = ResearchAndDevelopment::GetExperiment("mysteryGoo");
    REQUIRE(goo != nullptr);
    auto subj = ResearchAndDevelopment::GetExperimentSubject(*goo, ExperimentSituations::SrfLanded, BodyOf("Kerbin"), "", "");
    // the whole file at once
    RnDCommsStream full(subj, 10.0f, 5.0f, 0.5f, true, &rd);
    const float want = ResearchAndDevelopment::GetScienceValue(10.0f, 1.0f, *subj, 0.5f);
    Time::time = 10.0f;
    full.StreamData(10.0f, nullptr);
    CHECK(full.dataIn() == 10.0f);
    CHECK(full.dataOut() == 10.0f);
    CHECK(Bits(subj->science) == Bits(want));
    // partial, then silence: xmitIncomplete submits what arrived once Time.time > last + timeout
    auto subj2 = ResearchAndDevelopment::GetExperimentSubject(*goo, ExperimentSituations::SrfLanded, BodyOf("Mun"), "", "");
    RnDCommsStream part(subj2, 10.0f, 5.0f, 0.5f, true, &rd);
    const float want2 = ResearchAndDevelopment::GetScienceValue(4.0f, 1.0f, *subj2, 0.5f);
    Time::time = 20.0f;
    part.StreamData(4.0f, nullptr);
    CHECK(part.dataIn() == 4.0f);
    CHECK(!part.timedOut());
    CHECK(subj2->science == 0.0f);
    Time::time = 22.0f;
    RunFrames(1);  // the WaitForSeconds(1) of the timeout coroutine (the scheduler clock advances 0.02 s per RunFrames step)
    ScienceEnvironment::Scheduler().AdvanceFrame(2.0f);
    ScienceEnvironment::Scheduler().RunDelayedDynamicFrameRate();
    CHECK(subj2->science == 0.0f);  // 22 is not > 20 + 5
    Time::time = 26.0f;
    ScienceEnvironment::Scheduler().AdvanceFrame(2.0f);
    ScienceEnvironment::Scheduler().RunDelayedDynamicFrameRate();
    CHECK(part.timedOut());
    CHECK(part.dataOut() == 4.0f);
    CHECK(Bits(subj2->science) == Bits(want2));
    // partial and not xmitIncomplete: the stream is reset and nothing is submitted
    auto subj3 = ResearchAndDevelopment::GetExperimentSubject(*goo, ExperimentSituations::SrfLanded, BodyOf("Duna"), "", "");
    RnDCommsStream lost(subj3, 10.0f, 5.0f, 0.5f, false, &rd);
    Time::time = 40.0f;
    lost.StreamData(4.0f, nullptr);
    Time::time = 50.0f;
    ScienceEnvironment::Scheduler().AdvanceFrame(2.0f);
    ScienceEnvironment::Scheduler().RunDelayedDynamicFrameRate();
    CHECK(lost.dataIn() == 0.0f);
    CHECK(lost.dataOut() == 0.0f);
    CHECK(subj3->science == 0.0f);
}

// ResearchAndDevelopment.onVesselRecoveryProcessing (:331) over a vessel of the game's own save, plus ModuleScienceLab.RecoverScienceLabs (:426)
TEST_CASE("WP-35 recovery: the ScienceData of a recovered vessel is submitted, stored lab science is added, the trip log earns recovery subjects") {
    if (!GameAvailable()) return;
    SciGuard guard;
    Game15 g;
    guard.Wire(g);
    std::vector<CelestialBody*> bodies;
    for (const auto& b : KerbolSystemPrefab().bodies) bodies.push_back(const_cast<CelestialBody*>(&b.body));
    FlightGlobals::bodies = &bodies;
    struct R {
        ~R() { FlightGlobals::bodies = nullptr; }
    } r;
    ConfigNode::NodePtr save = ConfigNode::Load(std::string(KSPCORE_TESTDATA_DIR) + "/saves/physprofile/persistent.sfs");
    REQUIRE(save);
    ConfigNode* flight = save->GetNode("GAME")->GetNode("FLIGHTSTATE");
    REQUIRE(flight != nullptr);
    ConfigNode* vnode = nullptr;
    for (ConfigNode* n : flight->GetNodes("VESSEL"))
        for (ConfigNode* part : n->GetNodes("PART"))
            for (ConfigNode* mod : part->GetNodes("MODULE"))
                if (vnode == nullptr && mod->GetValue("name") == "ModuleScienceContainer") vnode = n;
    REQUIRE(vnode != nullptr);  // the second vessel of the physprofile save carries a ModuleScienceContainer
    Game game;
    ProtoVessel pv(*vnode, &game);
    ResearchAndDevelopment rd;
    rd.Awake();
    ScienceExperiment* goo = ResearchAndDevelopment::GetExperiment("mysteryGoo");
    auto subj = ResearchAndDevelopment::GetExperimentSubject(*goo, ExperimentSituations::SrfLanded, BodyOf("Kerbin"), "Shores", "");
    // find a ModuleScienceContainer snapshot in the vessel and store one ScienceData in it (the node a real flight would have saved)
    ProtoPartModuleSnapshot* snap = nullptr;
    for (ProtoPartSnapshot* ps : pv.GetAllProtoPartsIncludingCargo())
        for (auto& m : ps->modules)
            if (m->moduleName && *m->moduleName == "ModuleScienceContainer" && snap == nullptr) snap = m.get();
    REQUIRE(snap != nullptr);
    ScienceData sd(10.0f, 0.5f, 0.5f, 0.0f, subj->id, subj->title);
    sd.Save(*snap->moduleValues->AddNode("ScienceData"));
    // a lab part holding 12.5 stored science
    auto labNode = ConfigNode::Create("MODULE");
    labNode->AddValue("name", "ModuleScienceLab");
    labNode->AddValue("storedScience", 12.5f);
    labNode->AddValue("dataStored", 3.0f);
    pv.protoPartSnapshots[0]->modules.push_back(std::make_unique<ProtoPartModuleSnapshot>(*labNode));
    const float expectedData = ResearchAndDevelopment::GetScienceValue(10.0f, 0.5f, *subj, 1.0f);
    VesselTripLog trip = VesselTripLog::FromProtoVessel(pv);
    rd.onVesselRecoveryProcessing(&pv, nullptr, 0.0f);
    CHECK(Bits(subj->science) == Bits(expectedData));
    const bool flewHome = trip.Log.HasEntry(FlightLog::EntryType::Flight, "Kerbin") || trip.Log.HasEntry(FlightLog::EntryType::Orbit, "Kerbin") ||
                          trip.Log.HasEntry(FlightLog::EntryType::Suborbit, "Kerbin");
    // the science points: the data's value (x1 multiplier), the lab's stored science, plus the reverse-engineered recovery subject if any
    float recoveryScience = 0.0f;
    size_t recoverySubjects = 0;
    for (const auto& s : ResearchAndDevelopment::GetSubjects()) {
        if (s->id.rfind("recovery@", 0) == 0) {
            recoverySubjects++;
            recoveryScience = f32store(fx(recoveryScience) + fx(s->science));
            // ScienceSubject("recovery@...", title, 1, RecoveryValue * subValue, RecoveryValue * subValue * 1.2): cap is 1.2 x the value
            CHECK(Bits(s->scienceCap) == Bits(f32store(fx(s->subjectValue) * fx(1.2f))));
        }
    }
    CHECK((recoverySubjects > 0) == (flewHome || trip.Log.HasEntry(FlightLog::EntryType::Land, "Kerbin")));
    float expectedTotal = 0.0f;
    expectedTotal = f32store(fx(expectedTotal) + fx(expectedData));
    expectedTotal = f32store(fx(expectedTotal) + fx(recoveryScience));
    expectedTotal = f32store(fx(expectedTotal) + 12.5);
    // order of the C#: data (+= each), reverse engineering, then labs (one AddScience(12.5)); float adds in that order
    float order = 0.0f;
    order = f32store(fx(order) + fx(expectedData));
    order = f32store(fx(order) + fx(recoveryScience));
    order = f32store(fx(order) + 12.5);
    CHECK(Bits(rd.Science()) == Bits(order));
    ScienceEnvironment::screenMessages.clear();
}

// ============================================================================================== sensors, samples, the lab cleaning

TEST_CASE("WP-35 ModuleEnviroSensor and ModuleSampleContainer: toggling, the readout, the electric charge draw, sample transfer and discard") {
    if (!GameAvailable()) return;
    SciGuard guard;
    Game15 g;
    guard.Wire(g);
    Time::fixedDeltaTime = 0.02f;
    auto host = g.New("sensorThermometer");
    auto* sensor = ModuleOf<ModuleEnviroSensor>(*host, "ModuleEnviroSensor");
    CHECK(sensor->EnumName("sensorType") == "TEMP");
    REQUIRE(sensor->resHandler.inputResources.size() == 1);
    CHECK(sensor->resHandler.inputResources[0]->name == "ElectricCharge");
    CHECK(sensor->resHandler.inputResources[0]->rate == static_cast<double>(0.0075f));
    REQUIRE(sensor->GetConsumedResources().size() == 1);
    // ToggleAction (:45): Activate -> on, Deactivate -> off, Toggle flips
    KSPActionParam act(0, KSPActionType::Activate), deact(0, KSPActionType::Deactivate), tog(0, KSPActionType::Toggle);
    CHECK(!sensor->Get<bool>("sensorActive"));
    sensor->ToggleAction(act);
    CHECK(sensor->Get<bool>("sensorActive"));
    sensor->ToggleAction(tog);
    CHECK(!sensor->Get<bool>("sensorActive"));
    sensor->ToggleAction(tog);
    CHECK(sensor->Get<bool>("sensorActive"));
    sensor->ToggleAction(deact);
    CHECK(!sensor->Get<bool>("sensorActive"));
    sensor->Toggle();
    CHECK(sensor->Get<bool>("sensorActive"));
    // FixedUpdate (:97): off -> "Off"
    const std::string off = Loc35::Format("#autoLOC_237153");
    sensor->Set<bool>("sensorActive", false);
    sensor->FixedUpdate();
    CHECK(sensor->Get<std::string>("readoutInfo") == off);
    // on, started, with electric charge and the part in the PAW: the temperature with "0.##" and the unit
    PartResource* ec = host->GetResource("ElectricCharge");
    if (ec == nullptr) ec = host->Resources.Add("ElectricCharge", 100.0, 100.0, true, true, false, true, PartResourceFlowMode::Both);
    ec->maxAmount = 100.0;
    ec->amount = 100.0;
    TestVessel vessel;
    vessel.Adopt({host.get()});
    host->fields.started = true;
    host->fields.temperature = 293.15;
    ScienceModuleEnvironment::partActionUIContains = [](const Part*) { return true; };
    sensor->Set<bool>("sensorActive", true);
    sensor->FixedUpdate();
    CHECK(sensor->Get<std::string>("readoutInfo") == "293.15 " + Loc35::Format("#autoLOC_7001406"));
    CHECK(ec->amount < 100.0);  // the module resource input drew charge (rate 0.0075/s)
    host->fields.temperature = 300.0;
    sensor->FixedUpdate();
    CHECK(sensor->Get<std::string>("readoutInfo") == "300 " + Loc35::Format("#autoLOC_7001406"));  // "0.##" drops the trailing zeros
    // not in the PAW: the readout keeps its last text
    ScienceModuleEnvironment::partActionUIContains = [](const Part*) { return false; };
    host->fields.temperature = 310.0;
    sensor->FixedUpdate();
    CHECK(sensor->Get<std::string>("readoutInfo") == "300 " + Loc35::Format("#autoLOC_7001406"));
    // no charge: "Off"
    ec->amount = 0.0;
    sensor->FixedUpdate();
    CHECK(sensor->Get<std::string>("readoutInfo") == off);
    // the pressure sensor and its "0.00##" formats
    auto baro = g.New("sensorBarometer");
    auto* bs = ModuleOf<ModuleEnviroSensor>(*baro, "ModuleEnviroSensor");
    CHECK(bs->EnumName("sensorType") == "PRES");

    // ModuleSampleContainer (no stock part carries it: added in code)
    auto pod = g.New("mk1pod.v2");
    PartModuleData* md = pod->AddModule("ModuleSampleContainer", true);
    REQUIRE(md != nullptr);
    auto* sc = dynamic_cast<ModuleSampleContainer*>(md->module.get());
    REQUIRE(sc != nullptr);
    CHECK(sc->Get<std::string>("heldSampleName") == "N/A");
    sc->OnStart(StartState::Landed);
    CHECK(!sc->Events()["Discard"]->active);
    auto sample = std::make_shared<PlanetarySample>();
    sample->sampleName = "Kerbin soil";
    sample->sampleMass = 2.5f;
    sc->TransferSample(sample);
    CHECK(sc->Get<bool>("sampleHeld"));
    CHECK(sc->Get<std::string>("heldSampleName") == "Kerbin soil");
    CHECK(sc->Get<float>("heldSampleMass") == 2.5f);
    CHECK(sc->Events()["Discard"]->active);
    sc->Discard();
    CHECK(!sc->Get<bool>("sampleHeld"));
    CHECK(sc->Get<std::string>("heldSampleName") == "N/A");
    CHECK(sc->Get<float>("heldSampleMass") == 0.0f);
    CHECK(sc->sample == nullptr);
    sc->Set<bool>("sampleHeld", true);
    sc->OnStart(StartState::Flying);
    CHECK(sc->Events()["Discard"]->active);
}

// ModuleScienceLab.CleanUpExperiment (:303): the lab draws processResources (ElectricCharge amount x resourceResetCost x baseValue) frame by frame,
// then the experiment is operable and reset. The lab is manned by nobody here (cleaning does not need crew).
TEST_CASE("WP-35 ModuleScienceLab.CleanUpExperiment: resource gathering over frames, shortage, reset of an inoperable experiment") {
    if (!GameAvailable()) return;
    SciGuard guard;
    Game15 g;
    guard.Wire(g);
    ResearchAndDevelopment rd;
    rd.Awake();
    auto labPart = g.New("Large.Crewed.Lab");
    auto gooPart = g.New("GooExperiment");
    TestVessel vessel;
    vessel.Adopt({labPart.get(), gooPart.get()});
    auto* lab = ModuleOf<ModuleScienceLab>(*labPart, "ModuleScienceLab");
    auto* exp = ModuleOf<ModuleScienceExperiment>(*gooPart, "ModuleScienceExperiment");
    PartResource* ec = labPart->GetResource("ElectricCharge");
    if (ec == nullptr) ec = labPart->Resources.Add("ElectricCharge", 0.0, 1.0, true, true, false, true, PartResourceFlowMode::Both);
    REQUIRE(lab->processResources.size() == 1);
    CHECK(lab->processResources[0]->name == "ElectricCharge");
    CHECK(lab->processResources[0]->amount == 10.0);  // largeCrewedLab.cfg: RESOURCE_PROCESS amount = 10
    exp->experiment = ResearchAndDevelopment::GetExperiment("mysteryGoo");
    exp->Set<bool>("hideUIwhenUnavailable", false);  // the fake vessel has no body to ask
    exp->Set<bool>("Inoperable", true);
    exp->Set<bool>("Deployed", true);
    ScienceEnvironment::deltaTime = 1.0f;  // one request of `amount` per frame
    // not enough charge: stops with the shortage message and nothing is reset
    ec->maxAmount = 1000.0;
    ec->amount = 25.0;
    ScienceEnvironment::screenMessages.clear();
    auto co = ScienceEnvironment::Scheduler().StartCoroutine(lab->CleanUpExperiment(exp));
    RunFrames(10);
    CHECK(!lab->allResourcesAvailable);
    CHECK(exp->Get<bool>("Inoperable"));
    CHECK(ec->amount == 0.0);
    CHECK(co->Finished());
    // enough charge: required = amount (10) * resourceResetCost (1) * baseValue (10) = 100; 10 per frame
    ec->amount = 500.0;
    lab->processResources[0]->currentAmount = 0.0;
    auto co2 = ScienceEnvironment::Scheduler().StartCoroutine(lab->CleanUpExperiment(exp));
    RunFrames(30);
    CHECK(co2->Finished());
    CHECK(lab->allResourcesAvailable);
    CHECK(!exp->Get<bool>("Inoperable"));
    CHECK(ec->amount == 400.0);  // 10 requests of 10 charge
    CHECK(lab->processResources[0]->currentAmount == 90.0);  // `currentAmount -= amount * resourceResetCost` (:322) after 100 were gathered
}
