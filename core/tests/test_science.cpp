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
