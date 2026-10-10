// WP-28 unit tests: the resource-processing modules (BaseConverter and its descendants, drills, generator, alternator, jettison, drain,
// space-object info / analysis, scanners, surveyor). No golden or capture exists for these modules' runtime behaviour, so every expectation
// is derived: from the decompiled C# (the derivation is in the comment above the check), from the game's own cfg files (read straight from
// the MODULE nodes of every stock part), or from tools/wp28/processing_reference.py (an independent Python implementation of the converter
// catch-up maths and of ModuleSpaceObjectInfo.SetupSpaceObjectResources).
#include <fstream>

#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/modules/resource_processing.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::support;

namespace {

// the resource store the Python reference models (tools/wp28/processing_reference.py): amounts live in the host part's resources
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

nlohmann::json LoadRef() {
    std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp28_processing_reference.json", std::ios::binary);
    REQUIRE(f.good());
    return nlohmann::json::parse(f);
}

PartModule* AddModuleTo(Part& part, const char* cls) {
    PartModuleData* md = part.AddModule(cls, true);
    REQUIRE(md != nullptr);
    REQUIRE(md->module != nullptr);
    return md->module.get();
}

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}

ConfigNode::NodePtr ParseOne(const char* text) {
    auto n = ConfigNode::Parse(text);
    REQUIRE(n);
    REQUIRE(n->nodes().Count() == 1);
    return n->nodes().Shared(0);
}

// every test starts and ends with a clean environment
struct Env {
    Env() { ResourceProcessingEnvironment::Reset(); }
    ~Env() {
        ResourceProcessingEnvironment::Reset();
        ResourceEnvironment::tickCount = nullptr;
    }
};

std::string Loc(const char* id) { return Localizer::Format(id); }

float F32(double v) { return static_cast<float>(v); }
int32_t ParseI(const std::string& s) {
    int32_t v = 0;
    REQUIRE(dotnet::TryParseInt32(s, v));
    return v;
}

template <class T>
T* As(PartModule* m) {
    T* t = dynamic_cast<T*>(m);
    REQUIRE(t != nullptr);
    return t;
}

}  // namespace

// ============================================== the stock cfg: every WP-28 module of every stock part ============================

TEST_CASE("WP-28 stock cfg: every converter / harvester / drill / generator / alternator / scanner module equals its MODULE node") {
    if (!GameAvailable()) return;
    Env env;
    Game15 g;
    std::map<std::string, size_t> perClass;
    size_t ratios = 0;
    for (const auto& ap : g.data.parts.loadedParts) {
        if (!ap->partPrefab || ap->codeBuilt || !ap->prefabCompiled) continue;
        for (const PartModuleData& md : ap->partPrefab->ModuleData) {
            if (!md.module || !md.config) continue;
            CAPTURE(ap->name);
            CAPTURE(md.moduleName);
            const std::string cls = md.module->ClassName();
            if (auto* bc = dynamic_cast<BaseConverter*>(md.module.get())) {
                perClass[cls]++;
                // BaseConverter.OnLoad (BaseConverter.cs:342): the three lists come from the INPUT / OUTPUT / REQUIRED_RESOURCE nodes (a node
                // without `ResourceName` is skipped); Ratio is `float.Parse` widened to double
                std::vector<ResourceRatio> in, out, req;
                for (int i = 0; i < md.config->nodes().Count(); i++) {
                    ConfigNode* c = md.config->nodes()[i];
                    if (!c->HasValue("ResourceName") && c->name.size() >= 9 && c->name.substr(c->name.size() - 9) == "_RESOURCE") continue;
                    ResourceRatio r;
                    r.FlowMode = ResourceFlowMode::NULL_;
                    r.ResourceName = *c->GetValue("ResourceName");
                    if (c->HasValue("Ratio")) r.Ratio = static_cast<double>(dotnet::ParseSingle(*c->GetValue("Ratio")));
                    if (c->HasValue("DumpExcess")) r.DumpExcess = (*c->GetValue("DumpExcess") == "True" || *c->GetValue("DumpExcess") == "true");
                    if (c->HasValue("FlowMode")) r.FlowMode = AsFlowMode(static_cast<int32_t>(kEnum_ResourceFlowMode.Parse(*c->GetValue("FlowMode"))));
                    if (c->name == "INPUT_RESOURCE") in.push_back(r);
                    else if (c->name == "OUTPUT_RESOURCE") out.push_back(r);
                    else if (c->name == "REQUIRED_RESOURCE") req.push_back(r);
                }
                auto same = [&](const std::vector<ResourceRatio>& a, const std::vector<ResourceRatio>& b) {
                    REQUIRE(a.size() == b.size());
                    for (size_t i = 0; i < a.size(); i++) {
                        CHECK(a[i].ResourceName == b[i].ResourceName);
                        CHECK(a[i].Ratio == b[i].Ratio);
                        CHECK(a[i].DumpExcess == b[i].DumpExcess);
                        CHECK(a[i].FlowMode == b[i].FlowMode);
                        ratios++;
                    }
                };
                same(bc->inputList, in);
                same(bc->outputList, out);
                same(bc->reqList, req);
                CHECK(bc->Get<std::string>("status") == Loc("#autoLOC_257112"));
                // the cfg's own scalar fields
                if (md.config->HasValue("ConverterName")) CHECK(bc->Get<std::string>("ConverterName") == *md.config->GetValue("ConverterName"));
                if (md.config->HasValue("EfficiencyBonus")) CHECK(bc->Get<float>("EfficiencyBonus") == dotnet::ParseSingle(*md.config->GetValue("EfficiencyBonus")));
                // ThermalEfficiency: the cfg's `key = t v ..` points evaluate to their values at their own times; the default (OnAwake) is
                // the single key (0, 1)
                FloatCurve te = bc->Get<FloatCurve>("ThermalEfficiency");
                ConfigNode* teNode = md.config->GetNode("ThermalEfficiency");
                if (teNode == nullptr) {
                    CHECK(te.Evaluate(0.0f) == 1.0f);
                    CHECK(te.Evaluate(1000.0f) == 1.0f);
                } else {
                    for (const std::string& k : teNode->GetValues("key")) {
                        std::vector<std::string> parts = dotnet::Split(k, " ", true);
                        REQUIRE(parts.size() >= 2);
                        CHECK(te.Evaluate(dotnet::ParseSingle(parts[0])) == dotnet::ParseSingle(parts[1]));
                    }
                }
                if (auto* rc = dynamic_cast<ModuleResourceConverter*>(bc)) {
                    // ModuleResourceConverter.LoadRecipe (:25): the lists copied, ConvertByMass divides by the density
                    auto recipe = rc->LoadRecipe();
                    if (!rc->Get<bool>("ConvertByMass")) {
                        REQUIRE(recipe->Inputs().size() == in.size());
                        for (size_t i = 0; i < in.size(); i++) CHECK(recipe->Inputs()[i].Ratio == in[i].Ratio);
                        CHECK(recipe->Outputs().size() == out.size());
                        CHECK(recipe->Requirements().size() == req.size());
                    }
                    CHECK(rc->IsSituationValid());
                }
                if (auto* hv = dynamic_cast<ModuleResourceHarvester*>(bc)) {
                    // ModuleResourceHarvester.LoadRecipe (:321): the inputs + one output of ResourceName at the harvest rate; the
                    // electric charge input becomes eInput
                    hv->LoadRecipe(0.25);
                    REQUIRE(hv->recipe.Outputs().size() == 1);
                    CHECK(hv->recipe.Outputs()[0].ResourceName == *md.config->GetValue("ResourceName"));
                    CHECK(hv->recipe.Outputs()[0].Ratio == 0.25);
                    CHECK(hv->recipe.Outputs()[0].DumpExcess == (hv->Get<int32_t>("HarvesterType") == 2));
                    CHECK(hv->recipe.Outputs()[0].FlowMode == ResourceFlowMode::NULL_);
                    bool hasEc = false;
                    for (const auto& r : in) hasEc = hasEc || r.ResourceName == "ElectricCharge";
                    CHECK(hv->hasEInput == hasEc);
                    CHECK(hv->Get<int32_t>("HarvesterType") == ParseI(*md.config->GetValue("HarvesterType")));
                    CHECK(hv->GetConsumedResources().size() == 1);
                }
            } else if (cls == "ModuleGenerator" || cls == "ModuleAlternator") {
                perClass[cls]++;
                // the output list of the handler (WP-15) carries the rate as a double
                int idx = 0;
                for (int i = 0; i < md.config->nodes().Count(); i++) {
                    ConfigNode* c = md.config->nodes()[i];
                    if (c->name != "OUTPUT_RESOURCE") continue;
                    REQUIRE(static_cast<size_t>(idx) < md.module->resHandler.outputResources.size());
                    CHECK(md.module->resHandler.outputResources[static_cast<size_t>(idx)]->rate == dotnet::ParseDouble(*c->GetValue("rate")));
                    idx++;
                    ratios++;
                }
                CHECK(md.module->resHandler.moduleResourceBasedPrimaryIsInput == (cls == "ModuleGenerator"));
                if (cls == "ModuleGenerator") CHECK(md.module->Get<bool>("isAlwaysActive") == md.config->HasValue("isAlwaysActive"));
            } else if (cls == "ModuleResourceScanner" || cls == "ModuleOrbitalScanner" || cls == "ModuleOrbitalSurveyor" || cls == "ModuleBiomeScanner" ||
                       cls == "ModuleFuelJettison" || cls == "ModuleResourceDrain" || cls == "ModuleAsteroidInfo" || cls == "ModuleCometInfo" ||
                       cls == "ModuleAsteroidResource" || cls == "ModuleCometResource" || cls == "ModuleAnalysisResource" ||
                       cls == "ModuleAsteroidAnalysis" || cls == "ModuleCometAnalysis") {
                perClass[cls]++;
                if (cls == "ModuleResourceScanner") {
                    CHECK(md.module->Get<int32_t>("ScannerType") == ParseI(*md.config->GetValue("ScannerType")));
                    CHECK(md.module->Get<std::string>("ResourceName") == *md.config->GetValue("ResourceName"));
                }
                if (cls == "ModuleAsteroidResource" || cls == "ModuleCometResource") {
                    // ModuleSpaceObjectResource.OnLoad (:28): the flow mode of the resource definition unless FlowMode names one
                    auto* sr = dynamic_cast<ModuleSpaceObjectResource*>(md.module.get());
                    REQUIRE(sr != nullptr);
                    const PartResourceDefinition* d = PartResourceLibrary::Instance()->GetDefinition(*md.config->GetValue("resourceName"));
                    if (!md.config->HasValue("FlowMode") && d) CHECK(static_cast<int>(sr->_flowMode) == d->resourceFlowMode());
                }
            }
        }
    }
    for (const auto& [c, n] : perClass) MESSAGE(c << ": " << n << " stock module(s)");
    // the stock game has converters (FuelCell, FuelCellArray, ISRU, MiniISRU), harvesters (MiniDrill, RadialDrill), the RTG, alternators on
    // every liquid / jet engine, scanners, a surveyor and the potato / comet info modules
    CHECK(perClass["ModuleResourceConverter"] == 10);  // FuelCell, FuelCellArray, MiniISRU / ISRU (several converters each)
    CHECK(perClass["ModuleResourceHarvester"] == 2);
    CHECK(perClass["ModuleAsteroidDrill"] == 2);
    CHECK(perClass["ModuleCometDrill"] == 2);
    CHECK(perClass["ModuleResourceDrain"] == 1);
    CHECK(perClass["ModuleOrbitalSurveyor"] == 1);
    CHECK(perClass["ModuleOrbitalScanner"] == 1);
    CHECK(perClass["ModuleResourceScanner"] == 2);
    CHECK(perClass["ModuleAlternator"] > 15);
    CHECK(perClass["ModuleGenerator"] >= 2);
    CHECK(ratios > 30);
}

// ============================================== converter catch-up maths against the Python reference ==============================

TEST_CASE("WP-28 BaseConverter / ModuleResourceConverter FixedUpdate: catch-up, efficiency and ProcessRecipe equal the independent Python reference") {
    if (!GameAvailable()) return;
    Env env;
    Game15 g;
    const nlohmann::json ref = LoadRef();
    ResourceScenario scenario;
    scenario.Awake();
    REQUIRE(ResourceScenario::Instance() == &scenario);
    SceneScope scope(GameScenes::FLIGHT);
    Time::fixedDeltaTime = 0.02f;
    ResourceProcessingEnvironment::flightReady = true;
    ResourceProcessingEnvironment::timeSinceLevelLoad = 10.0f;
    // the densities the reference assumes are the game's (float densities widened)
    for (const auto& [n, d] : std::map<std::string, double>{{"Ore", 0.01}, {"LiquidFuel", 0.005}, {"Oxidizer", 0.005}, {"MonoPropellant", 0.004}, {"ElectricCharge", 0.0}})
        CHECK(static_cast<double>(PartResourceLibrary::Instance()->GetDefinition(n)->density()) == static_cast<double>(static_cast<float>(d)));

    size_t scenarios = 0, steps = 0, caught = 0, withRetries = 0, converted = 0;
    for (const auto& sc : ref.at("scenarios")) {
        CAPTURE(sc.at("name").get<std::string>());
        auto host = g.New("batteryBank");
        host->fields.persistentId = 1;
        for (const auto& [name, t] : sc.at("tanks").items()) {
            PartResource* r = host->GetResource(name);
            if (r == nullptr) r = host->Resources.Add(name, 0.0, 1.0, true, true, false, true, PartResourceFlowMode::Both);
            REQUIRE(r != nullptr);
            r->maxAmount = t.at("max").get<double>();
            r->amount = t.at("amount").get<double>();
        }
        TestVessel vessel;
        vessel.Adopt({host.get()});
        ProcessingVessel pv;
        for (const auto& c : sc.at("crew")) {
            ProcessingCrewMember m;
            for (const auto& e : c.at("effects")) m.effects.insert(e.get<std::string>());
            m.experienceLevel = c.at("level").get<int>();
            pv.crew.push_back(m);
        }
        ResourceProcessingEnvironment::vesselOf = [&](const Part&) { return &pv; };
        double ut = 0.0;
        ResourceProcessingEnvironment::universalTime = [&] { return ut; };
        scenario.gameSettings()->MaxDeltaTime = sc.at("maxDeltaTime").get<int>();

        auto* conv = As<ModuleResourceConverter>(AddModuleTo(*host, "ModuleResourceConverter"));
        TankBroker broker(host.get());
        conv->_resBroker = &broker;
        auto load = [](const nlohmann::json& a) {
            std::vector<ResourceRatio> r;
            for (const auto& x : a) {
                ResourceRatio q(x.at("name").get<std::string>(), x.at("ratio").get<double>(), x.at("dump").get<bool>(), ResourceFlowMode::NULL_);
                r.push_back(q);
            }
            return r;
        };
        conv->inputList = load(sc.at("inputs"));
        conv->outputList = load(sc.at("outputs"));
        conv->reqList = load(sc.at("reqs"));
        conv->Set<bool>("ConvertByMass", sc.at("convertByMass").get<bool>());
        conv->Set<bool>("AlwaysActive", true);
        conv->Set<float>("EfficiencyBonus", sc.at("efficiencyBonus").get<float>());
        conv->Set<float>("FillAmount", sc.at("fillAmount").get<float>());
        conv->Set<float>("TakeAmount", sc.at("takeAmount").get<float>());
        conv->Set<bool>("UseSpecialistBonus", sc.at("useSpecialistBonus").get<bool>());
        conv->Set<float>("SpecialistBonusBase", sc.at("specialistBonusBase").get<float>());
        conv->Set<float>("SpecialistEfficiencyFactor", sc.at("specialistEfficiencyFactor").get<float>());
        FloatCurve te;
        te.Add(0.0f, sc.at("thermalEfficiency").get<float>());
        conv->Set<FloatCurve>("ThermalEfficiency", te);
        conv->OnStart(StartState::Flying);
        for (const auto& m : sc.at("modifiers")) conv->EfficiencyModifiers.emplace_back("mod" + std::to_string(conv->EfficiencyModifiers.size()), m.get<float>());
        conv->TallyEfficiencyModifiers();
        // the host's flow: IsActivated is set by AlwaysActive inside FixedUpdate
        for (const auto& e : sc.at("expected")) {
            ut = e.at("ut").get<double>();
            conv->FixedUpdate();
            CHECK(conv->lastUpdateTime == e.at("lastUpdateTime").get<double>());
            CHECK(conv->catchupRetries == e.at("catchupRetries").get<int>());
            CHECK(conv->lastTimeFactor == e.at("lastTimeFactor").get<double>());
            for (const auto& [name, amt] : e.at("amounts").items()) CHECK(host->GetResource(name)->amount == amt.get<double>());
            if (e.contains("efficiency")) CHECK(conv->GetEfficiencyMultiplier() == e.at("efficiency").get<double>());
            steps++;
        }
        if (sc.at("convertByMass").get<bool>()) converted++;
        if (conv->catchupRetries > 0) withRetries++;
        for (const auto& e : sc.at("expected"))
            if (e.contains("timeFactor") && e.at("timeFactor").get<double>() > 0) {
                caught++;
                break;
            }
        scenarios++;
        conv->_resBroker = nullptr;
    }
    MESSAGE(scenarios << " scenarios, " << steps << " FixedUpdate steps, " << caught << " with production, " << withRetries << " used catch-up retries, " << converted << " by mass");
    CHECK(scenarios == 48);
    CHECK(caught > 20);
    CHECK(withRetries > 5);
}

// ============================================== ModuleSpaceObjectInfo against the Python reference =================================

TEST_CASE("WP-28 ModuleSpaceObjectInfo.SetupSpaceObjectResources equals the independent Python reference (KSPRandom, float32 stores)") {
    if (!GameAvailable()) return;
    Env env;
    Game15 g;
    const nlohmann::json ref = LoadRef();
    SceneScope scope(GameScenes::FLIGHT);
    size_t cases = 0, present = 0;
    for (const auto& c : ref.at("spaceObjects")) {
        auto part = g.New("batteryBank");
        auto* info = As<ModuleAsteroidInfo>(AddModuleTo(*part, "ModuleAsteroidInfo"));
        std::vector<ModuleSpaceObjectResource*> res;
        for (const auto& r : c.at("resources")) {
            auto* m = As<ModuleSpaceObjectResource>(AddModuleTo(*part, "ModuleAsteroidResource"));
            m->Set<std::string>("resourceName", r.at("name").get<std::string>());
            m->Set<int32_t>("presenceChance", r.at("presenceChance").get<int>());
            m->Set<int32_t>("lowRange", r.at("lowRange").get<int>());
            m->Set<int32_t>("highRange", r.at("highRange").get<int>());
            res.push_back(m);
        }
        const int32_t seed = c.at("seed").get<int32_t>();
        ResourceEnvironment::tickCount = [seed] { return seed; };
        info->set_currentMassVal(c.at("currentMass").get<double>());
        info->SetupSpaceObjectResources(res);
        const auto& e = c.at("expected");
        CAPTURE(seed);
        CHECK(info->totalResourceMass == e.at("totalResourceMass").get<float>());
        CHECK(info->totalResourcePercent == e.at("totalResourcePercent").get<double>());
        CHECK(info->currentMassVal() == e.at("currentMass").get<double>());
        CHECK(info->massThresholdVal() == e.at("massThreshold").get<double>());
        for (size_t i = 0; i < res.size(); i++) {
            const auto& a = e.at("abundances")[i];
            if (a.is_null()) {
                CHECK(res[i]->Get<float>("abundance") == 0.0f);
            } else {
                CHECK(Bits(res[i]->Get<float>("abundance")) == Bits(a.get<float>()));
                CHECK(res[i]->Get<float>("displayAbundance") == res[i]->Get<float>("abundance"));
                present++;
            }
        }
        cases++;
    }
    MESSAGE(cases << " cases, " << present << " abundances set");
    CHECK(cases == 40);
    CHECK(present > 40);
}
