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
#include "ksp/vessel/vessel.hpp"
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

// ============================================== BaseConverter: lifecycle, persistence, crew, heat, shutdown ==========================

namespace {

// a host part with a flight vessel record (the Part.vessel the modules read) and the resource flow of WP-15
struct Rig {
    Game15& g;
    std::unique_ptr<Part> host;
    TestVessel vessel;
    ProcessingVessel pv;
    double ut = 5000.0;
    explicit Rig(Game15& game, const char* part = "batteryBank") : g(game), host(game.New(part)) {
        host->fields.persistentId = 1;
        vessel.Adopt({host.get()});
        pv.parts = {host.get()};
        ResourceProcessingEnvironment::vesselOf = [this](const Part&) { return &pv; };
        ResourceProcessingEnvironment::universalTime = [this] { return ut; };
    }
    PartResource* Res(const char* n, double amount, double max) {
        PartResource* r = host->GetResource(n);
        if (r == nullptr) r = host->Resources.Add(n, amount, max, true, true, false, true, PartResourceFlowMode::Both);
        r->maxAmount = max;
        r->amount = amount;
        return r;
    }
};

}  // namespace

TEST_CASE("WP-28 BaseConverter: OnLoad lists and lastUpdateTime, OnSave, start / stop / toggle, animation gate, GetDeltaTime guards") {
    if (!GameAvailable()) return;
    Env env;
    Game15 g;
    SceneScope scope(GameScenes::FLIGHT);
    Rig rig(g);
    auto* conv = As<ModuleResourceConverter>(AddModuleTo(*rig.host, "ModuleResourceConverter"));
    REQUIRE(conv->startEvt != nullptr);
    REQUIRE(conv->stopEvt != nullptr);
    // BaseConverter.OnLoad (:342). The OUTPUT_RESOURCE node has no ResourceName: it is skipped (logged) but still clears the output list
    // (`node.HasNode("OUTPUT_RESOURCE")`); the INPUT node keeps its FlowMode / DumpExcess / float Ratio widened to double
    conv->outputList.push_back(ResourceRatio("Stale", 1.0, false));
    auto node = ParseOne(
        "MODULE\n{\n name = ModuleResourceConverter\n lastUpdateTime = 1234.5\n"
        " INPUT_RESOURCE\n {\n  ResourceName = Ore\n  Ratio = 0.1\n  DumpExcess = True\n  FlowMode = ALL_VESSEL\n }\n"
        " OUTPUT_RESOURCE\n {\n  Ratio = 3\n }\n"
        " REQUIRED_RESOURCE\n {\n  ResourceName = ElectricCharge\n  Ratio = 5\n }\n}\n");
    {
        LogCapture log;
        conv->OnLoad(*node);
        CHECK(log.Has(log::Level::Info, "Resource must have value 'ResourceName'"));
    }
    REQUIRE(conv->inputList.size() == 1);
    CHECK(conv->inputList[0].ResourceName == "Ore");
    CHECK(conv->inputList[0].Ratio == static_cast<double>(0.1f));  // float.Parse("0.1") widened
    CHECK(conv->inputList[0].DumpExcess);
    CHECK(conv->inputList[0].FlowMode == ResourceFlowMode::ALL_VESSEL);
    CHECK(conv->outputList.empty());
    REQUIRE(conv->reqList.size() == 1);
    CHECK(conv->reqList[0].FlowMode == ResourceFlowMode::NULL_);  // `resourceRatio.FlowMode = NULL` before Load
    CHECK(conv->lastUpdateTime == 1234.5);                        // ResourceUtilities.GetValue(node, "lastUpdateTime", ...)
    // SetupModule -> EnableModule (no ModuleAnimationGroup): isEnabled, DirtyFlag = !IsActivated, then UpdateConverterStatus
    // (DirtyFlag != IsActivated) clears it and sets the events: start active (not activated and enabled), stop inactive
    CHECK(conv->isEnabled);
    CHECK_FALSE(conv->Get<bool>("DirtyFlag"));
    CHECK(conv->startEvt->active);
    CHECK_FALSE(conv->stopEvt->active);
    CHECK(conv->Get<std::string>("status") == Loc("#autoLOC_257112"));
    // OnSave (:486)
    ConfigNode saved("MODULE");
    conv->OnSave(saved);
    CHECK(saved.GetValue("lastUpdateTime") == dotnet::FormatDoubleG(1234.5));
    // without a vessel OnLoad stops after the lists (`if (base.vessel == null) return;`): lastUpdateTime stays
    ResourceProcessingEnvironment::vesselOf = nullptr;
    conv->lastUpdateTime = 9.0;
    conv->OnLoad(*node);
    CHECK(conv->lastUpdateTime == 9.0);
    ResourceProcessingEnvironment::vesselOf = [&](const Part&) { return &rig.pv; };

    // start / stop actions and events (:173-:216)
    KSPActionParam p;
    conv->StartResourceConverterAction(p);
    CHECK(conv->Get<bool>("IsActivated"));
    CHECK(conv->ModuleIsActive());
    CHECK_FALSE(conv->CanBeDetached());
    conv->ToggleResourceConverterAction(p);  // active -> Stop
    CHECK_FALSE(conv->Get<bool>("IsActivated"));
    CHECK(conv->Get<std::string>("status") == Loc("#autoLOC_257112"));
    CHECK(conv->CanBeRotated());
    conv->ToggleResourceConverterAction(p);  // inactive -> Start
    CHECK(conv->Get<bool>("IsActivated"));
    conv->UpdateConverterStatus();  // DirtyFlag(false) != IsActivated(true): the status text is set
    CHECK(conv->Get<std::string>("status") == Loc("#autoLOC_257237"));
    CHECK_FALSE(conv->startEvt->active);
    CHECK(conv->stopEvt->active);
    conv->StopResourceConverterAction(p);
    // StartResourceConverter with an undeployed animation group posts the message and leaves the converter off (:173)
    conv->hasAnimationGroup = true;
    conv->partAnimationModule = conv;
    ResourceProcessingEnvironment::animationGroupDeployed = [](PartModule&) { return false; };
    conv->StartResourceConverter();
    CHECK_FALSE(conv->Get<bool>("IsActivated"));
    REQUIRE(ResourceProcessingEnvironment::screenMessages.size() == 1);
    CHECK(ResourceProcessingEnvironment::screenMessages[0] == "#autoLOC_6010007");
    ResourceProcessingEnvironment::animationGroupDeployed = [](PartModule&) { return true; };
    conv->StartResourceConverter();
    CHECK(conv->Get<bool>("IsActivated"));
    conv->hasAnimationGroup = false;
    // DisableModule: isEnabled false, IsActivated false (:150); EnableModule re-arms the start event
    conv->DisableModule();
    CHECK_FALSE(conv->isEnabled);
    CHECK_FALSE(conv->Get<bool>("IsActivated"));
    CHECK_FALSE(conv->startEvt->active);  // `!IsActivated && isEnabled`
    conv->EnableModule();
    CHECK(conv->startEvt->active);

    // GetDeltaTime (:226): -1 while the level is younger than one second, or the flight scene is not ready, or on the first call (which
    // only records the UT); then the elapsed time capped by the game's MaxDeltaTime
    ResourceScenario scenario;
    scenario.Awake();
    scenario.gameSettings()->MaxDeltaTime = 600;
    ResourceProcessingEnvironment::timeSinceLevelLoad = 0.5f;
    CHECK(conv->GetDeltaTime() == -1.0);
    CHECK(conv->lastUpdateTime == 9.0);
    ResourceProcessingEnvironment::timeSinceLevelLoad = 10.0f;
    ResourceProcessingEnvironment::flightReady = false;
    CHECK(conv->GetDeltaTime() == -1.0);
    ResourceProcessingEnvironment::flightReady = true;
    conv->lastUpdateTime = 0.0;
    rig.ut = 5000.0;
    CHECK(conv->GetDeltaTime() == -1.0);
    CHECK(conv->lastUpdateTime == 5000.0);
    rig.ut = 5000.01;
    const double dt1 = conv->GetDeltaTime();  // 0.01 < one fixed step: no retry, returned as is
    CHECK(dt1 == 5000.01 - 5000.0);
    CHECK(conv->lastUpdateTime == 5000.0 + dt1);
    rig.ut = 9000.0;  // 4000 s: capped at MaxDeltaTime 600 (no recipe: GetBestDeltaTime returns the input)
    CHECK(conv->GetDeltaTime() == 600.0);
    // a throwing UT source is caught and logged: 0.0 (:257)
    ResourceProcessingEnvironment::universalTime = []() -> double { throw std::runtime_error("no clock"); };
    {
        LogCapture log;
        CHECK(conv->GetDeltaTime() == 0.0);
        CHECK(log.Has(log::Level::Error, "BaseConverter_GetDeltaTime"));
    }
    // TallyEfficiencyModifiers before OnStart created the dictionary: NullReferenceException (:568)
    CHECK_THROWS_AS(conv->TallyEfficiencyModifiers(), NullReferenceException);
    // OnStart (:437): events and actions get the cfg names, the dictionary exists, DirtyFlag = !IsActivated, then SetupModule
    conv->OnStart(StartState::Flying);
    CHECK(conv->EfficiencyModifiers.empty());
    CHECK(conv->_totalEfficiencyModifiers == 1.0f);
    CHECK(conv->Fields()["status"]->guiName == conv->Get<std::string>("ConverterName"));
    CHECK(conv->startEvt->guiName() == Localizer::Format(conv->Get<std::string>("StartActionName")));
}

TEST_CASE("WP-28 BaseConverter: crew bonus, specialist search, heat throttle, core heat, shutdown, ConvertRecipeToUnits quirk") {
    if (!GameAvailable()) return;
    Env env;
    Game15 g;
    SceneScope scope(GameScenes::FLIGHT);
    Rig rig(g);
    ResourceScenario scenario;
    scenario.Awake();
    auto* conv = As<ModuleResourceConverter>(AddModuleTo(*rig.host, "ModuleResourceConverter"));
    conv->OnStart(StartState::Flying);
    // HasSpecialist (:671): from the last crew member to the first, the highest level among members with the effect; level 5 ends the search
    auto crew = [&](std::vector<std::pair<std::string, int>> c) {
        rig.pv.crew.clear();
        for (auto& [e, l] : c) {
            ProcessingCrewMember m;
            m.effects.insert(e);
            m.experienceLevel = l;
            rig.pv.crew.push_back(m);
        }
    };
    crew({});
    CHECK(conv->HasSpecialist("ConverterSkill") == -1);
    crew({{"ConverterSkill", 2}, {"ConverterSkill", 4}, {"PilotSkill", 5}});
    CHECK(conv->HasSpecialist("ConverterSkill") == 4);
    crew({{"ConverterSkill", 5}, {"ConverterSkill", 1}});
    CHECK(conv->HasSpecialist("ConverterSkill") == 5);
    // GetCrewEfficiencyBonus (:108): 1 without UseSpecialistBonus; else SpecialistBonusBase + (1 + level) * SpecialistEfficiencyFactor with one
    // float store: 0.05f + (1 + 4) * 0.1f
    CHECK(conv->GetCrewEfficiencyBonus() == 1.0f);
    conv->Set<bool>("UseSpecialistBonus", true);
    crew({{"ConverterSkill", 2}, {"ConverterSkill", 4}});
    CHECK(conv->GetCrewEfficiencyBonus() == F32(static_cast<double>(0.05f) + 5.0 * static_cast<double>(0.1f)));
    crew({});  // level -1: 0.05 + 0 * 0.1
    CHECK(conv->GetCrewEfficiencyBonus() == 0.05f);
    // GetCrewHeatBonus (:122): (1 + level) * SpecialistHeatFactor, at most 1
    CHECK(conv->GetCrewHeatBonus() == 0.0f);
    conv->Set<bool>("UseSpecialistHeatBonus", true);
    crew({{"ConverterSkill", 3}});
    CHECK(conv->GetCrewHeatBonus() == F32(4.0 * static_cast<double>(0.1f)));
    conv->Set<float>("SpecialistHeatFactor", 0.5f);
    CHECK(conv->GetCrewHeatBonus() == 1.0f);  // min(2.5, 1)

    // GetHeatThrottle (:640): 1 while inactive; active: ThermalEfficiency at the part temperature, or at the core temperature of a
    // ModuleCoreHeat on the part
    FloatCurve te;
    te.Add(0.0f, 1.0f, 0.0f, 0.0f);
    te.Add(1000.0f, 0.25f, 0.0f, 0.0f);
    conv->Set<FloatCurve>("ThermalEfficiency", te);
    rig.host->fields.temperature = 640.0;
    CHECK(conv->GetHeatThrottle() == 1.0f);
    conv->Set<bool>("IsActivated", true);
    CHECK(conv->GetHeatThrottle() == te.Evaluate(640.0f));
    CHECK_FALSE(conv->IsOverheating());
    CHECK_FALSE(conv->DisplayCoreHeat());
    CHECK(conv->GetCoreTemperature() == 640.0);
    CHECK(conv->GetGoalTemperature() == 640.0);
    PartModule* core = AddModuleTo(*rig.host, "ModuleCoreHeat");
    ResourceProcessingEnvironment::coreTemperature = [](PartModule&) { return 777.5; };
    CHECK(conv->DisplayCoreHeat());
    CHECK(conv->GetHeatThrottle() == te.Evaluate(777.5f));
    CHECK(conv->IsOverheating());  // CoreTemperature > part.temperature
    CHECK(conv->GetCoreTemperature() == 777.5);
    core->Set<double>("CoreTempGoalAdjustment", 25.0);
    CHECK(conv->GetGoalTemperature() == core->Get<double>("CoreTempGoal") + 25.0);
    // GetEfficiencyMultiplier (:493): EfficiencyBonus * heatThrottle * total * crew, a double product of float operands
    conv->Set<float>("EfficiencyBonus", 0.7f);
    conv->EfficiencyModifiers = {{"a", 0.9f}, {"b", 1.1f}};
    conv->TallyEfficiencyModifiers();
    CHECK(conv->_totalEfficiencyModifiers == F32(static_cast<double>(F32(1.0 * static_cast<double>(0.9f))) * static_cast<double>(1.1f)));  // float store per step
    crew({{"ConverterSkill", 5}});
    const double want = static_cast<double>(0.7f) * static_cast<double>(te.Evaluate(777.5f)) * static_cast<double>(conv->_totalEfficiencyModifiers) *
                        static_cast<double>(conv->GetCrewEfficiencyBonus());
    CHECK(conv->GetEfficiencyMultiplier() == want);

    // CheckForShutdown (:657): AutoShutdown && GeneratesHeat and part.temperature > maxTemp * DefaultShutoffTemp (0.8) -> stop + message
    ResourceProcessingEnvironment::coreTemperature = nullptr;
    conv->Set<bool>("AutoShutdown", true);
    conv->Set<bool>("GeneratesHeat", true);
    rig.host->fields.maxTemp = 2000.0;
    rig.host->fields.temperature = 1599.0;
    conv->CheckForShutdown();
    CHECK(conv->Get<bool>("IsActivated"));
    rig.host->fields.temperature = 1601.0;  // 2000 * (double)0.8f = 1600.0000238
    conv->CheckForShutdown();
    CHECK_FALSE(conv->Get<bool>("IsActivated"));
    REQUIRE(ResourceProcessingEnvironment::screenMessages.size() == 1);
    CHECK(ResourceProcessingEnvironment::screenMessages[0] == Loc("#autoLOC_257466"));

    // ConvertRecipeToUnits (:578): ratios divide by the (float) density when it exceeds 1E-09; the KSP quirk puts a dense REQUIREMENT into
    // the output list, only a massless one stays a requirement
    ConversionRecipe recipe;
    recipe.Inputs().push_back(ResourceRatio("Ore", 1.0, false, ResourceFlowMode::ALL_VESSEL));
    recipe.Inputs().push_back(ResourceRatio("ElectricCharge", 4.0, false));
    recipe.Outputs().push_back(ResourceRatio("LiquidFuel", 0.45, true));
    recipe.Requirements().push_back(ResourceRatio("Ore", 2.0, false));
    recipe.Requirements().push_back(ResourceRatio("ElectricCharge", 8.0, false));
    conv->ConvertRecipeToUnits(recipe);
    REQUIRE(recipe.Inputs().size() == 2);
    CHECK(recipe.Inputs()[0].Ratio == 1.0 / static_cast<double>(0.01f));
    CHECK(recipe.Inputs()[0].FlowMode == ResourceFlowMode::ALL_VESSEL);
    CHECK(recipe.Inputs()[1].Ratio == 4.0);  // density 0: unchanged
    REQUIRE(recipe.Outputs().size() == 2);
    CHECK(recipe.Outputs()[0].Ratio == 0.45 / static_cast<double>(0.005f));
    CHECK(recipe.Outputs()[0].DumpExcess);
    CHECK(recipe.Outputs()[1].ResourceName == "Ore");  // sic: the requirement became an output
    CHECK(recipe.Outputs()[1].Ratio == 2.0 / static_cast<double>(0.01f));
    REQUIRE(recipe.Requirements().size() == 1);
    CHECK(recipe.Requirements()[0].ResourceName == "ElectricCharge");
    // an unknown resource: NullReferenceException (the dictionary lookup returns null)
    ConversionRecipe bad;
    bad.Inputs().push_back(ResourceRatio("Unobtainium", 1.0, false));
    CHECK_THROWS_AS(conv->ConvertRecipeToUnits(bad), NullReferenceException);
}

// ============================================== ModuleResourceHarvester ==============================================================

namespace {

// The in-game start-up the resource map needs: PhysicsGlobals, an Awake Planetarium, the spawned Kerbol system (FlightGlobals.Bodies)
struct Kerbol28 {
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    Kerbol28() {
        pg.physicsDatabaseFilename = std::string(KSPCORE_HARNESS_DIR) + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        ResourceEnvironment::bodies = [this]() -> const std::vector<CelestialBody*>& { return sys->Bodies(); };
    }
    ~Kerbol28() {
        ResourceEnvironment::bodies = nullptr;
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
    }
};

// the lock / scan lists live in the scenario's game settings and are shared with the map's cache: start every test from empty lists
void FreshMap(ResourceScenario& scenario) {
    scenario.gameSettings()->GetBiomeLockInfo().clear();
    scenario.gameSettings()->GetPlanetScanInfo().clear();
    scenario.gameSettings()->GetDepletionInfo().clear();
    ResourceMap::Instance()->ResetCache();
}

PartModule* Find(Part& p, const char* cls) {
    PartModule* m = p.FindModuleOfClass(cls);
    REQUIRE(m != nullptr);
    return m;
}

}  // namespace

TEST_CASE("WP-28 ModuleResourceHarvester: PrepareRecipe branches, abundance rate, intake multiplier, depletion, electric charge, situation") {
    if (!GameAvailable()) return;
    Env env;
    Game15 g;
    Kerbol28 kerbol;
    ResourceScenario scenario;
    scenario.Awake();
    scenario.gameSettings()->Seed = 1985366484;  // the physprofile save's resource seed (the WP-15 reference's)
    scenario.gameSettings()->MaxDeltaTime = 600;
    ResourceEnvironment::difficultyResourceAbundance = [] { return 1.0f; };
    ResourceMap& map = *ResourceMap::Instance();
    map.ResetCache();
    Time::fixedDeltaTime = 0.02f;
    Rig rig(g, "MiniDrill");
    rig.Res("ElectricCharge", 1000.0, 1000.0);
    rig.Res("Ore", 0.0, 100.0);
    auto* hv = As<ModuleResourceHarvester>(Find(*rig.host, "ModuleResourceHarvester"));
    TankBroker broker(rig.host.get());
    hv->_resBroker = &broker;
    const float Efficiency = hv->Get<float>("Efficiency");
    REQUIRE(hv->Get<std::string>("ResourceName") == "Ore");
    REQUIRE(hv->Get<int32_t>("HarvesterType") == 0);
    REQUIRE_FALSE(hv->inputList.empty());  // the cfg's ElectricCharge input

    // outside the flight scene there is no recipe (:176)
    {
        SceneScope editor(GameScenes::EDITOR);
        CHECK(hv->PrepareRecipe(1.0) == nullptr);
    }
    SceneScope flight(GameScenes::FLIGHT);
    auto fails = [&](double dt, const char* loc) {
        hv->Set<bool>("IsActivated", true);
        CHECK(hv->PrepareRecipe(dt) == nullptr);
        CHECK(hv->Get<std::string>("status") == Loc(loc));
        CHECK_FALSE(hv->Get<bool>("IsActivated"));
    };
    // type 0 (planetary), landed and moving above 10 m/s on the active vessel, with the step shorter than MaxDeltaTime: 259762. A catch-up
    // step of MaxDeltaTime or more skips the check
    rig.pv.Landed = true;
    rig.pv.horizontalSrfSpeed = 10.5;
    rig.pv.isActiveVessel = true;
    fails(1.0, "#autoLOC_259762");
    // type 0, not landed above 1000 m of radar altitude: 259775
    rig.pv.Landed = false;
    rig.pv.radarAltitude = 1500.0;
    fails(1.0, "#autoLOC_259775");
    // type 1 (oceanic) without a splashed vessel: 259782
    hv->Set<int32_t>("HarvesterType", 1);
    fails(1.0, "#autoLOC_259782");
    hv->Set<int32_t>("HarvesterType", 0);
    rig.pv.Landed = true;
    rig.pv.horizontalSrfSpeed = 0.0;
    rig.pv.radarAltitude = 0.0;
    // no impact: ImpactTransform set and the transform exists, the raycast misses, and the step is shorter than twice the fixed step
    // (deltaTime / 2 < fixedDeltaTime): 259789; a long step goes on
    hv->Set<std::string>("ImpactTransform", "Impact");
    hv->hasImpactTransform = true;
    ResourceProcessingEnvironment::impactRaycast = [](PartModule&) { return false; };
    CHECK_FALSE(hv->CheckForImpact());
    fails(0.02, "#autoLOC_259789");
    ResourceProcessingEnvironment::impactRaycast = [](PartModule&) { return true; };
    CHECK(hv->CheckForImpact());
    hv->hasImpactTransform = false;
    // zero abundance: the Sun (body 0) has no Ore: 259808
    rig.pv.mainBody.flightGlobalsIndex = 0;
    fails(1.0, "#autoLOC_259808");
    // a place with Ore on the planetary layer: scan Kerbin / Mun / Minmus and the others for the first non-zero abundance. The request the
    // module builds is (body, lat, lon, altitude, ResourceType = HarvesterType, ResourceName, CheckForLock = false)
    int foundBody = -1;
    double foundLat = 0, foundLon = 0;
    float abundance = 0.0f;
    for (int body = 1; body < 17 && foundBody < 0; body++)
        for (double lat = -60; lat <= 60 && foundBody < 0; lat += 30)
            for (double lon = -120; lon <= 120 && foundBody < 0; lon += 60) {
                AbundanceRequest r;
                r.BodyId = body;
                r.Latitude = lat;
                r.Longitude = lon;
                r.ResourceType = HarvestTypes::Planetary;
                r.ResourceName = "Ore";
                const float a = map.GetAbundance(r);
                if (a > 0.0f) {
                    foundBody = body;
                    foundLat = lat;
                    foundLon = lon;
                    abundance = a;
                }
            }
    REQUIRE(foundBody > 0);
    rig.pv.mainBody.flightGlobalsIndex = foundBody;
    rig.pv.latitude = foundLat;
    rig.pv.longitude = foundLon;
    // an abundance below HarvestThreshold: the threshold message (`(HarvestThreshold * 100f).ToString("0.0")`) and 259819
    hv->Set<float>("HarvestThreshold", 2.0f);
    ResourceProcessingEnvironment::screenMessages.clear();
    fails(1.0, "#autoLOC_259819");
    REQUIRE(ResourceProcessingEnvironment::screenMessages.size() == 1);
    CHECK(ResourceProcessingEnvironment::screenMessages[0] == Localizer::Format("#autoLOC_6001069", {"200.0"}));
    hv->Set<float>("HarvestThreshold", 0.0f);
    // not activated: the 259826 status and no recipe, IsActivated untouched
    hv->Set<bool>("IsActivated", false);
    CHECK(hv->PrepareRecipe(1.0) == nullptr);
    CHECK(hv->Get<std::string>("status") == Loc("#autoLOC_259826"));
    // activated: num = abundance * Efficiency stored as a float; _resFlow = num; the recipe is the inputs plus one output of ResourceName
    hv->Set<bool>("IsActivated", true);
    ConversionRecipe* rec = hv->PrepareRecipe(1.0);
    REQUIRE(rec != nullptr);
    const float num = F32(static_cast<double>(abundance) * static_cast<double>(Efficiency));
    CHECK(hv->_resFlow == static_cast<double>(num));
    REQUIRE(rec->Outputs().size() == 1);
    CHECK(rec->Outputs()[0].ResourceName == "Ore");
    CHECK(rec->Outputs()[0].Ratio == static_cast<double>(num));
    CHECK_FALSE(rec->Outputs()[0].DumpExcess);
    CHECK(rec->Outputs()[0].FlowMode == ResourceFlowMode::NULL_);
    CHECK(rec->Inputs().size() == hv->inputList.size());
    CHECK(hv->hasEInput);
    CHECK(hv->eInput.ResourceName == "ElectricCharge");
    // the same request through the real FixedUpdate path: a full catch-up step (the recipe is the module's `recipe` member)
    CHECK(rec == &hv->recipe);

    // GetIntakeMultiplier (:273): vessel.atmDensity (1 for an exospheric harvester) * (srfSpeed + airSpeedStatic) * clamp01(srfVelDir . intake
    // forward); a part shielded from the airstream gives 0
    rig.pv.atmDensity = 0.5;
    rig.pv.srfSpeed = 100.0;
    rig.pv.srf_vel_direction = Vector3d(0.0, 0.0, 1.0);
    hv->Set<int32_t>("HarvesterType", 2);
    CHECK(hv->GetIntakeMultiplier() == 1.0 * (100.0 + 40.0) * 0.5);  // no intake transform: dot factor 1
    ResourceProcessingEnvironment::intakeForward = [](PartModule&) { return std::optional<Vector3f>(Vector3f(0.0f, 0.0f, 1.0f)); };
    CHECK(hv->GetIntakeMultiplier() == 1.0 * (140.0 * 0.5));
    ResourceProcessingEnvironment::intakeForward = [](PartModule&) { return std::optional<Vector3f>(Vector3f(0.0f, 0.0f, -1.0f)); };
    CHECK(hv->GetIntakeMultiplier() == 0.0);  // clamp01(-1)
    ResourceProcessingEnvironment::intakeForward = nullptr;
    hv->Set<int32_t>("HarvesterType", 3);
    CHECK(hv->GetIntakeMultiplier() == 140.0);  // exospheric: density 1
    rig.host->set_ShieldedFromAirstream(true);
    CHECK(hv->GetIntakeMultiplier() == 0.0);
    rig.host->set_ShieldedFromAirstream(false);
    // LoadRecipe for the atmospheric type dumps the excess
    hv->Set<int32_t>("HarvesterType", 2);
    hv->LoadRecipe(0.5);
    CHECK(hv->recipe.Outputs()[0].DumpExcess);
    hv->Set<int32_t>("HarvesterType", 0);

    // PostProcess (:359). CausesDepletion: the node under the vessel loses `value * (float)(DepletionRate * min(1, TimeFactor / dt))`
    hv->Set<bool>("CausesDepletion", true);
    hv->Set<float>("DepletionRate", 0.1f);
    const Vector2f node = map.GetDepletionNode(foundLat, foundLon);
    const int nx = static_cast<int>(node.x), ny = static_cast<int>(node.y);
    const float before = map.GetDepletionNodeValue(foundBody, "Ore", nx, ny);
    ConverterResults res;
    res.TimeFactor = 0.5;
    hv->Set<bool>("IsActivated", true);
    hv->PostProcess(res, 1.0);
    const float rate = F32(static_cast<double>(0.1f) * static_cast<double>(static_cast<float>(0.5)));
    CHECK(map.GetDepletionNodeValue(foundBody, "Ore", nx, ny) == F32(static_cast<double>(before) - static_cast<double>(before) * static_cast<double>(rate)));
    hv->Set<bool>("CausesDepletion", false);
    // the electric-charge guard: the input needs ratio * dt, the vessel has less -> the harvester stops with the message (:381)
    rig.Res("ElectricCharge", 0.0, 1000.0);
    ResourceProcessingEnvironment::screenMessages.clear();
    hv->LoadRecipe(0.25);
    REQUIRE(hv->hasEInput);
    hv->Set<bool>("IsActivated", true);
    res.TimeFactor = 1.0;
    hv->PostProcess(res, 1.0);
    CHECK_FALSE(hv->Get<bool>("IsActivated"));
    REQUIRE_FALSE(ResourceProcessingEnvironment::screenMessages.empty());
    CHECK(ResourceProcessingEnvironment::screenMessages.back() == Loc("#autoLOC_259933"));
    // PostUpdateCleanup (:298): inactive -> the n/a text and a displayed flow of -1
    hv->PostUpdateCleanup();
    CHECK(hv->Get<std::string>("ResourceStatus") == Loc("#autoLOC_259862"));
    CHECK(hv->_displayedResFlow == -1.0);

    // IsSituationValid (:413): planetary = LANDED, or radar altitude <= 500; oceanic = SPLASHED; atmospheric = atmDensity > 1E-9;
    // exospheric = ORBITING; any other type false
    auto situation = [&](int type, VesselSituations s, double radar, double density) {
        hv->Set<int32_t>("HarvesterType", type);
        rig.pv.situation = s;
        rig.pv.radarAltitude = radar;
        rig.pv.atmDensity = density;
        return hv->IsSituationValid();
    };
    CHECK(situation(0, VesselSituations::LANDED, 9000.0, 0.0));
    CHECK(situation(0, VesselSituations::FLYING, 500.0, 0.0));
    CHECK_FALSE(situation(0, VesselSituations::FLYING, 500.5, 0.0));
    CHECK(situation(1, VesselSituations::SPLASHED, 0.0, 0.0));
    CHECK_FALSE(situation(1, VesselSituations::LANDED, 0.0, 0.0));
    CHECK(situation(2, VesselSituations::FLYING, 0.0, 0.01));
    CHECK_FALSE(situation(2, VesselSituations::FLYING, 0.0, 1e-10));
    CHECK(situation(3, VesselSituations::ORBITING, 0.0, 0.0));
    CHECK_FALSE(situation(3, VesselSituations::SUB_ORBITAL, 0.0, 0.0));
    CHECK_FALSE(situation(7, VesselSituations::LANDED, 0.0, 1.0));
    {
        SceneScope editor(GameScenes::EDITOR);
        CHECK_FALSE(hv->IsSituationValid());
    }
    // a vessel with an asteroid (or comet) part is never valid; the answer is cached by part count (:419)
    auto rock = g.New("batteryBank");
    rock->fields.persistentId = 2;
    AddModuleTo(*rock, "ModuleAsteroid");
    rig.pv.parts = {rig.host.get(), rock.get()};
    CHECK_FALSE(situation(0, VesselSituations::LANDED, 0.0, 0.0));
    CHECK_FALSE(hv->cachedWasNotAsteroid);
    rig.pv.parts = {rig.host.get(), rig.host.get()};  // same count: the cached flag answers
    CHECK_FALSE(situation(0, VesselSituations::LANDED, 0.0, 0.0));
    rig.pv.parts = {rig.host.get()};                  // count changed: rescanned, no rock any more
    CHECK(situation(0, VesselSituations::LANDED, 0.0, 0.0));
    CHECK(hv->cachedWasNotAsteroid);
    // PreProcessing (:466): in flight isEnabled follows the situation
    hv->isEnabled = true;
    situation(0, VesselSituations::FLYING, 900.0, 0.0);
    hv->PreProcessing();
    CHECK_FALSE(hv->isEnabled);
    // OnStart (:154): the flight scene sets isEnabled from IsSituationValid and the resource status name from the definition's display name
    situation(0, VesselSituations::LANDED, 0.0, 0.0);
    hv->OnStart(StartState::Landed);
    CHECK(hv->isEnabled);
    CHECK(hv->Fields()["ResourceStatus"]->guiName == Localizer::Format("#autoLOC_6001918", {PartResourceLibrary::Instance()->GetDefinition("Ore")->displayName()}));
    hv->_resBroker = nullptr;
}

// ============================================== asteroid / comet drills ===========================================================

namespace {

struct DrillRig {
    Rig rig;
    std::unique_ptr<Part> potato;
    ModuleSpaceObjectDrillBase* drill = nullptr;
    ModuleSpaceObjectInfo* info = nullptr;
    std::vector<ModuleSpaceObjectResource*> res;
    TankBroker* broker = nullptr;
    std::unique_ptr<TankBroker> brokerOwner;
    // `comet` selects the ModuleCometDrill / ModuleComet* set
    DrillRig(Game15& g, bool comet) : rig(g, "MiniDrill") {
        potato = g.New("batteryBank");
        potato->fields.persistentId = 2;
        rig.host->setParent(potato.get());
        rig.vessel.Adopt({rig.host.get(), potato.get()});
        rig.pv.parts = {rig.host.get(), potato.get()};
        AddModuleTo(*potato, comet ? "ModuleComet" : "ModuleAsteroid");
        info = comet ? static_cast<ModuleSpaceObjectInfo*>(As<ModuleCometInfo>(AddModuleTo(*potato, "ModuleCometInfo")))
                     : static_cast<ModuleSpaceObjectInfo*>(As<ModuleAsteroidInfo>(AddModuleTo(*potato, "ModuleAsteroidInfo")));
        for (const char* n : {"Ore", "LiquidFuel"}) {
            auto* m = As<ModuleSpaceObjectResource>(AddModuleTo(*potato, comet ? "ModuleCometResource" : "ModuleAsteroidResource"));
            m->Set<std::string>("resourceName", n);
            m->_flowMode = ResourceFlowMode::NULL_;
            res.push_back(m);
        }
        drill = As<ModuleSpaceObjectDrillBase>(Find(*rig.host, comet ? "ModuleCometDrill" : "ModuleAsteroidDrill"));
        rig.Res("ElectricCharge", 50.0, 50.0);
        rig.Res("Ore", 0.0, 500.0);
        rig.Res("LiquidFuel", 0.0, 500.0);
        brokerOwner = std::make_unique<TankBroker>(rig.host.get());
        broker = brokerOwner.get();
        drill->_resBroker = broker;
        drill->OnStart(StartState::Landed);  // the efficiency modifiers' product starts at 0 until OnStart tallies it
    }
    ~DrillRig() {
        drill->_resBroker = nullptr;
        rig.host->setParent(nullptr);
        rig.vessel.parts = {rig.host.get()};  // the potato dies first: the vessel must not touch it again
        potato->vessel = nullptr;
    }
};

}  // namespace

TEST_CASE("WP-28 ModuleAsteroidDrill / ModuleCometDrill: PrepareRecipe branches, drilled mass, recipe outputs, PostProcess") {
    if (!GameAvailable()) return;
    Env env;
    Game15 g;
    SceneScope flight(GameScenes::FLIGHT);
    for (const bool comet : {false, true}) {
        CAPTURE(comet);
        DrillRig d(g, comet);
        ModuleSpaceObjectDrillBase* drill = d.drill;
        const char* noObject = comet ? "#autoLOC_6002530" : "#autoLOC_258405";
        const char* notAttached = comet ? "#autoLOC_6002531" : "#autoLOC_258412";
        auto fails = [&](const std::string& text) {
            drill->Set<bool>("IsActivated", true);
            CHECK(drill->PrepareRecipe(1.0) == nullptr);
            CHECK(drill->_status == text);
            CHECK_FALSE(drill->Get<bool>("IsActivated"));
        };
        const float Efficiency = drill->Get<float>("Efficiency");
        const float PowerConsumption = drill->Get<float>("PowerConsumption");
        d.info->set_currentMassVal(10.0);
        d.info->set_massThresholdVal(8.0);
        d.res[0]->Set<float>("abundance", 0.3f);
        d.res[1]->Set<float>("abundance", 0.2f);
        // :45 PrepareRecipe. No potato part on the vessel: the first status
        d.rig.pv.parts = {d.rig.host.get()};
        CHECK(drill->IsSituationValid() == false);
        fails(Loc(noObject));
        d.rig.pv.parts = {d.rig.host.get(), d.potato.get()};
        CHECK(drill->IsSituationValid());
        CHECK(drill->GetAttachedPotato() == d.potato.get());
        // DirectAttach: the potato must be a child or the parent of the drill part
        drill->Set<bool>("DirectAttach", true);
        d.rig.host->setParent(nullptr);
        fails(Loc(notAttached));
        d.rig.host->setParent(d.potato.get());
        drill->Set<bool>("DirectAttach", false);
        // RockOnly false needs at least one resource module on the potato: remove the modules by testing a bare potato
        {
            auto bare = d.rig.g.New("batteryBank");
            bare->fields.persistentId = 3;
            AddModuleTo(*bare, comet ? "ModuleComet" : "ModuleAsteroid");
            AddModuleTo(*bare, comet ? "ModuleCometInfo" : "ModuleAsteroidInfo");
            d.rig.pv.parts = {d.rig.host.get(), bare.get()};
            fails(Loc("#autoLOC_258419"));
            drill->Set<bool>("RockOnly", true);  // RockOnly skips that check: the next one (impact) passes (no transform), the info check runs
            drill->Set<bool>("IsActivated", true);
            // threshold 0 >= current mass 0 on the bare potato
            CHECK(drill->PrepareRecipe(1.0) == nullptr);
            CHECK(drill->_status == Loc("#autoLOC_258443"));
            drill->Set<bool>("RockOnly", false);
            d.rig.pv.parts = {d.rig.host.get(), d.potato.get()};
        }
        // the impact test (:170): ImpactTransform set and found, the raycast must hit the object
        drill->Set<std::string>("ImpactTransform", "Impact");
        drill->hasImpactTransform = true;
        ResourceProcessingEnvironment::impactRaycast = [](PartModule&) { return false; };
        fails(Loc("#autoLOC_258428"));
        ResourceProcessingEnvironment::impactRaycast = [](PartModule&) { return true; };
        // no info module: the potato without Info
        d.info->set_massThresholdVal(10.0);  // threshold >= current mass
        fails(Loc("#autoLOC_258443"));
        d.info->set_massThresholdVal(8.0);
        // not enough electric charge: AmountAvailable <= dt * PowerConsumption
        d.rig.Res("ElectricCharge", 0.5, 50.0);
        drill->Set<float>("PowerConsumption", 1.0f);
        fails(Loc("#autoLOC_258451"));
        d.rig.Res("ElectricCharge", 50.0, 50.0);
        drill->Set<float>("PowerConsumption", PowerConsumption);
        // IsActivated false after the checks: UpdateConverterStatus and no recipe, no failure text
        drill->Set<bool>("IsActivated", false);
        CHECK(drill->PrepareRecipe(1.0) == nullptr);
        // the recipe: per resource val = dt * abundance * Efficiency * efficiencyMultiplier, val2 = (current - threshold) / density, the
        // smaller one is drilled; Ratio = drilled / dt; every output dumps its excess; the electric charge input is PowerConsumption
        drill->Set<bool>("IsActivated", true);
        const double dt = 1.0;
        const double effMult = drill->GetEfficiencyMultiplier();
        ConversionRecipe* rec = drill->PrepareRecipe(dt);
        REQUIRE(rec != nullptr);
        REQUIRE(rec->Inputs().size() == 1);
        CHECK(rec->Inputs()[0].ResourceName == "ElectricCharge");
        CHECK(rec->Inputs()[0].Ratio == static_cast<double>(PowerConsumption));
        CHECK(rec->Inputs()[0].FlowMode == ResourceFlowMode::NULL_);
        REQUIRE(rec->Outputs().size() == 2);
        double drilled = 0.0;
        const char* names[2] = {"Ore", "LiquidFuel"};
        const float ab[2] = {0.3f, 0.2f};
        for (int i = 0; i < 2; i++) {
            const double density = static_cast<double>(PartResourceLibrary::Instance()->GetDefinition(names[i])->density());
            const double val = dt * static_cast<double>(ab[i]) * static_cast<double>(Efficiency) * effMult;
            const double val2 = (10.0 - 8.0) / density;
            const double num3 = val < val2 ? val : val2;
            drilled += density * num3;
            CHECK(rec->Outputs()[static_cast<size_t>(i)].ResourceName == names[i]);
            CHECK(rec->Outputs()[static_cast<size_t>(i)].Ratio == num3 / dt);
            CHECK(rec->Outputs()[static_cast<size_t>(i)].DumpExcess);
            CHECK(rec->Outputs()[static_cast<size_t>(i)].FlowMode == ResourceFlowMode::NULL_);
        }
        CHECK(drill->_drilledMass == drilled);
        // PostProcess (:260): the drilled mass leaves the object, the status is the preparation's ("Connected")
        ConverterResults cr;
        cr.TimeFactor = 1.0;
        drill->PostProcess(cr, dt);
        CHECK(d.info->currentMassVal() == 10.0 - drilled);
        CHECK(drill->Get<std::string>("status") == "Connected");
        // storage full for every resource: flag false -> the empty recipe and the 258501 status, nothing drilled
        d.rig.Res("Ore", 500.0, 500.0);
        d.rig.Res("LiquidFuel", 500.0, 500.0);
        d.info->set_currentMassVal(10.0);
        rec = drill->PrepareRecipe(dt);
        REQUIRE(rec != nullptr);
        CHECK(rec->Inputs().empty());
        CHECK(rec->Outputs().empty());
        CHECK(drill->_status == Loc("#autoLOC_258501"));
        CHECK(drill->_drilledMass == 0.0);
        // the part is a valid drill only while the object is on the vessel (OnUpdate :234)
        d.rig.pv.parts = {d.rig.host.get()};
        drill->_isValidSituation = true;
        drill->isEnabled = true;
        drill->OnUpdate();
        CHECK_FALSE(drill->isEnabled);
        ResourceProcessingEnvironment::impactRaycast = nullptr;
        // OnStart (:253): flight scene: valid situation, precalculated efficiency
        d.rig.pv.parts = {d.rig.host.get(), d.potato.get()};
        drill->OnStart(StartState::Landed);
        CHECK(drill->isEnabled);
        CHECK(drill->_preCalculateEfficiency);
    }
}

// ============================================== ModuleGenerator / ModuleAlternator / ModuleFuelJettison ==============================

TEST_CASE("WP-28 ModuleGenerator: activation, inputs, throttle, efficiency adjusters, RTG") {
    if (!GameAvailable()) return;
    Env env;
    Game15 g;
    SceneScope flight(GameScenes::FLIGHT);
    Time::fixedDeltaTime = 0.02f;
    Rig rig(g);
    PartResource* ec = rig.Res("ElectricCharge", 0.0, 1000.0);
    PartResource* ore = rig.Res("Ore", 100.0, 100.0);
    const double dt = static_cast<double>(TimeWarp::fixedDeltaTime());  // (double)0.02f
    auto* gen = As<ModuleGenerator>(AddModuleTo(*rig.host, "ModuleGenerator"));
    gen->resHandler.OnLoad(*ParseOne("MODULE\n{\n name = ModuleGenerator\n INPUT_RESOURCE\n {\n  name = Ore\n  rate = 2\n }\n"
                                     " OUTPUT_RESOURCE\n {\n  name = ElectricCharge\n  rate = 3\n }\n}\n"));
    // OnStart (:107): the events / actions follow moduleIsEnabled and isAlwaysActive; the gui names are the cfg's strings
    gen->OnStart(StartState::Flying);
    CHECK(gen->Events()["Activate"]->active);
    CHECK_FALSE(gen->Events()["Shutdown"]->active);
    CHECK(gen->Actions()["ToggleAction"]->active);
    CHECK(gen->Fields()["efficiency"]->guiActive);
    CHECK(gen->Events()["Activate"]->guiName() == Localizer::Format(gen->Get<std::string>("activateGUIName")));
    CHECK(gen->Actions()["ShutdownAction"]->guiName() == Localizer::Format(gen->Get<std::string>("shutdownGUIName")));
    // inactive (:163): efficiency 0, status "Off", display text 220477
    gen->FixedUpdate();
    CHECK(gen->Get<float>("efficiency") == 0.0f);
    CHECK(gen->Get<std::string>("status") == "Off");
    CHECK(gen->Get<std::string>("displayStatus") == Loc("#autoLOC_220477"));
    // Activate event: shutdown becomes available
    gen->Activate();
    CHECK(gen->Get<bool>("generatorIsActive"));
    CHECK(gen->Events()["Shutdown"]->active);
    CHECK_FALSE(gen->Events()["Activate"]->active);
    // active with enough Ore: the input draws rate * throttle(1) * dt, the satisfied fraction is 1, efficiency 1, the output adds
    // rate * efficiency * dt; status "Nominal" and display text 219034
    gen->FixedUpdate();
    CHECK(gen->Get<std::string>("status") == "Nominal");
    CHECK(gen->Get<std::string>("displayStatus") == Loc("#autoLOC_219034"));
    CHECK(gen->Get<float>("throttle") == 1.0f);
    CHECK(gen->inputRecieved == 1.0);
    CHECK(gen->Get<float>("efficiency") == 1.0f);
    CHECK(ore->amount == doctest::Approx(100.0 - 2.0 * dt).epsilon(1e-12));
    CHECK(ec->amount == doctest::Approx(3.0 * dt).epsilon(1e-12));
    // efficiency adjusters (the module-adjuster chain) transform the efficiency before the outputs: halve it
    gen->efficiencyAdjusters.push_back([](float e) { return e * 0.5f; });
    const double ec0 = ec->amount;
    gen->FixedUpdate();
    CHECK(gen->Get<float>("efficiency") == 0.5f);
    CHECK(ec->amount == doctest::Approx(ec0 + 3.0 * 0.5 * dt).epsilon(1e-12));
    gen->efficiencyAdjusters.clear();
    // throttle control: the vessel's main throttle (when the part is controllable); inputs and outputs scale by it; efficiency =
    // (float)(inputRecieved * throttle)
    gen->Set<bool>("isThrottleControlled", true);
    rig.pv.mainThrottle = 0.5f;
    const double ore0 = ore->amount, ec1 = ec->amount;
    gen->FixedUpdate();
    CHECK(gen->Get<float>("throttle") == 0.5f);
    CHECK(gen->Get<float>("efficiency") == 0.5f);
    CHECK(ore->amount == doctest::Approx(ore0 - 2.0 * 0.5 * dt).epsilon(1e-12));
    CHECK(ec->amount == doctest::Approx(ec1 + 3.0 * 0.5 * dt).epsilon(1e-12));
    rig.pv.isControllable = false;  // not controllable: the throttle keeps its last value
    rig.pv.mainThrottle = 0.9f;
    gen->FixedUpdate();
    CHECK(gen->Get<float>("throttle") == 0.5f);
    rig.pv.isControllable = true;
    // lacking the input: the unsatisfied input is unavailable, requiresAllInputs forces efficiency 0 and the outputs do nothing
    gen->Set<bool>("isThrottleControlled", false);
    ore->amount = 0.0;
    const double ec2 = ec->amount;
    gen->FixedUpdate();
    CHECK(gen->lackingResources);
    CHECK(gen->Get<float>("efficiency") == 0.0f);
    CHECK(gen->Get<std::string>("displayStatus") == Loc("#autoLOC_220477"));
    CHECK(gen->Get<std::string>("status") != "Nominal");
    CHECK(ec->amount == ec2);
    CHECK_FALSE(gen->currentResource->available);
    // Shutdown action / event, ToggleAction by KSPActionType (Activate 0, Deactivate 1, Toggle 2): `type != 0 && (type != Toggle ||
    // generatorIsActive)` shuts down
    gen->Shutdown();
    CHECK_FALSE(gen->Get<bool>("generatorIsActive"));
    KSPActionParam p;
    p.type = static_cast<int>(KSPActionType::Toggle);
    gen->ToggleAction(p);  // toggle while off: Activate
    CHECK(gen->Get<bool>("generatorIsActive"));
    gen->ToggleAction(p);  // toggle while on: Shutdown
    CHECK_FALSE(gen->Get<bool>("generatorIsActive"));
    p.type = static_cast<int>(KSPActionType::Deactivate);
    gen->Activate();
    gen->ToggleAction(p);
    CHECK_FALSE(gen->Get<bool>("generatorIsActive"));
    p.type = static_cast<int>(KSPActionType::Activate);
    gen->ToggleAction(p);
    CHECK(gen->Get<bool>("generatorIsActive"));
    gen->ShutdownAction(p);
    CHECK_FALSE(gen->Get<bool>("generatorIsActive"));
    gen->ActivateAction(p);
    CHECK(gen->Get<bool>("generatorIsActive"));
    // the bound handlers reach the same code (Actions().Invoke)
    gen->Actions()["ShutdownAction"]->Invoke(p);
    CHECK_FALSE(gen->Get<bool>("generatorIsActive"));
    // an always-active generator ignores the events and is switched on by OnStart outside the editor (:107)
    gen->Set<bool>("isAlwaysActive", true);
    gen->Activate();
    gen->Shutdown();
    CHECK_FALSE(gen->Get<bool>("generatorIsActive"));
    gen->OnStart(StartState::Editor);
    CHECK_FALSE(gen->Get<bool>("generatorIsActive"));
    gen->OnStart(StartState::Flying);
    CHECK(gen->Get<bool>("generatorIsActive"));
    CHECK_FALSE(gen->Actions()["ToggleAction"]->active);
    CHECK(gen->GetContractObjectiveType() == "Generator");
    CHECK(gen->CheckContractObjectiveValidity());
    gen->Set<bool>("isGroundFixture", true);
    CHECK_FALSE(gen->CheckContractObjectiveValidity());

    // a generator without inputs and with throttle control: efficiency = throttle (:210)
    auto* g2 = As<ModuleGenerator>(AddModuleTo(*rig.host, "ModuleGenerator"));
    g2->resHandler.OnLoad(*ParseOne("MODULE\n{\n name = ModuleGenerator\n OUTPUT_RESOURCE\n {\n  name = ElectricCharge\n  rate = 4\n }\n}\n"));
    g2->Set<bool>("generatorIsActive", true);
    g2->Set<bool>("isThrottleControlled", true);
    rig.pv.mainThrottle = 0.25f;
    const double ec3 = ec->amount;
    g2->FixedUpdate();
    CHECK(g2->Get<float>("efficiency") == 0.25f);
    CHECK(ec->amount == doctest::Approx(ec3 + 4.0 * 0.25 * dt).epsilon(1e-12));

    // the stock RTG (RTG.cfg): ModuleGenerator, isAlwaysActive, one OUTPUT_RESOURCE ElectricCharge 0.75, no inputs
    Rig rtgRig(g, "rtg");
    PartResource* rtgEc = rtgRig.Res("ElectricCharge", 0.0, 100.0);
    auto* rtg = As<ModuleGenerator>(Find(*rtgRig.host, "ModuleGenerator"));
    rtg->OnStart(StartState::Flying);
    CHECK(rtg->Get<bool>("generatorIsActive"));
    rtg->FixedUpdate();
    CHECK(rtg->Get<float>("efficiency") == 1.0f);
    CHECK(rtgEc->amount == doctest::Approx(0.75 * dt).epsilon(1e-12));
}

TEST_CASE("WP-28 ModuleAlternator: engine / rotor output, single tick threshold, gui fields") {
    if (!GameAvailable()) return;
    Env env;
    Game15 g;
    SceneScope flight(GameScenes::FLIGHT);
    Time::fixedDeltaTime = 0.02f;
    Rig rig(g, "liquidEngine");
    PartResource* ec = rig.Res("ElectricCharge", 0.0, 100.0);
    const double dt = static_cast<double>(TimeWarp::fixedDeltaTime());
    auto* alt = As<ModuleAlternator>(Find(*rig.host, "ModuleAlternator"));
    REQUIRE(alt->resHandler.outputResources.size() == 1);
    const double rate = alt->resHandler.outputResources[0]->rate;  // the cfg's alternator rate
    CHECK_FALSE(alt->resHandler.moduleResourceBasedPrimaryIsInput);
    // OnLoad / OnStart (:37, :44): the output field takes the module's name, units (localised) and format
    alt->OnLoad(ConfigNode("MODULE"));
    CHECK(alt->Fields()["outputRate"]->guiName == alt->Get<std::string>("outputName"));
    CHECK(alt->Fields()["outputRate"]->guiUnits == Localizer::Format(alt->Get<std::string>("outputUnits")));
    CHECK(alt->Fields()["outputRate"]->guiFormat == alt->Get<std::string>("outputFormat"));
    alt->OnStart(StartState::Flying);
    CHECK(alt->Fields()["outputRate"]->guiActive);
    // FixedUpdate (:106): with the engine at normalizedOutput 0.5 the output is rate * 0.5 for one fixed step and outputRate (float) shows the rate
    alt->engineNormalizedOutput = [] { return 0.5f; };
    alt->FixedUpdate();
    CHECK(ec->amount == doctest::Approx(rate * 0.5 * dt).epsilon(1e-12));
    CHECK(alt->Get<float>("outputRate") == static_cast<float>(rate * static_cast<double>(0.5f)));
    // below singleTickThreshold (0.0001) per step: UpdateModuleResourceOutputs does nothing and returns 0
    const double before = ec->amount;
    alt->engineNormalizedOutput = [] { return 1.0e-6f; };
    alt->FixedUpdate();
    CHECK(ec->amount == before);
    CHECK(alt->Get<float>("outputRate") == 0.0f);
    // a zero output does not call the handler at all (`num > 0f`): outputRate keeps its value
    alt->Set<float>("outputRate", 7.0f);
    alt->engineNormalizedOutput = [] { return 0.0f; };
    alt->FixedUpdate();
    CHECK(alt->Get<float>("outputRate") == 7.0f);
    // output adjusters (AdjusterAlternatorBase.ApplyOutputAdjustment)
    alt->outputAdjusters.push_back([](float o) { return o * 0.5f; });
    alt->engineNormalizedOutput = [] { return 1.0f; };
    const double before2 = ec->amount;
    alt->FixedUpdate();
    CHECK(ec->amount == doctest::Approx(before2 + rate * 0.5 * dt).epsilon(1e-12));
    alt->outputAdjusters.clear();
    // not in flight / module disabled: nothing happens
    {
        SceneScope editor(GameScenes::EDITOR);
        const double b = ec->amount;
        alt->FixedUpdate();
        CHECK(ec->amount == b);
    }
    alt->Set<bool>("moduleIsEnabled", false);
    {
        const double b = ec->amount;
        alt->FixedUpdate();
        CHECK(ec->amount == b);
    }
    alt->Set<bool>("moduleIsEnabled", true);
    // robotic rotor mode (:44): without Serenity or without a rotor module the mode is switched off; with a rotor the rotor's output counts
    alt->Set<bool>("roboticRotorMode", true);
    alt->serenityInstalled = false;
    alt->OnStart(StartState::Flying);
    CHECK_FALSE(alt->Get<bool>("roboticRotorMode"));
    alt->Set<bool>("roboticRotorMode", true);
    alt->serenityInstalled = true;
    alt->OnStart(StartState::Flying);  // no ModuleRoboticServoRotor on the part
    CHECK_FALSE(alt->Get<bool>("roboticRotorMode"));
    alt->Set<bool>("roboticRotorMode", true);
    alt->rotor = alt;  // a rotor was found
    alt->rotorNormalizedOutput = [] { return 0.25f; };
    const double before3 = ec->amount;
    alt->FixedUpdate();
    CHECK(ec->amount == doctest::Approx(before3 + rate * 0.25 * dt).epsilon(1e-12));
    // GetModuleDisplayName / the stock count (every liquid and jet engine carries one)
    CHECK(alt->GetModuleDisplayName() == Loc("#autoLoc_6003026"));
}

TEST_CASE("WP-28 ModuleFuelJettison: dump all, dump one, unknown name, null ResourceName") {
    if (!GameAvailable()) return;
    Env env;
    Game15 g;
    Rig rig(g, "SmallTank");
    PartResource* lf = rig.Res("LiquidFuel", 120.0, 180.0);
    PartResource* ox = rig.Res("Oxidizer", 80.0, 220.0);
    auto* jet = As<ModuleFuelJettison>(Find(*rig.host, "ModuleFuelJettison"));
    // the stock cfg gives no ResourceName: the prefab's field is null; a runtime (cloned) module has "" and dumps everything
    CHECK(jet->IsNull("ResourceName"));
    jet->OnStart(StartState::Flying);  // empty name: the gui names stay the attribute's
    // a named jettison dumps only that resource (:9): TransferResource(res, double.MinValue, part) empties it
    jet->Set<std::string>("ResourceName", "Oxidizer");
    jet->OnStart(StartState::Flying);
    CHECK(jet->Events()["FuelJettison"]->guiName() == Localizer::Format("#autoLOC_6001888", {"Oxidizer"}));
    CHECK(jet->Actions()["FuelJettisonAction"]->guiName() == Localizer::Format("#autoLOC_6001888", {"Oxidizer"}));
    jet->FuelJettison();
    CHECK(ox->amount == 0.0);
    CHECK(lf->amount == 120.0);
    REQUIRE(ResourceProcessingEnvironment::screenMessages.size() == 1);
    CHECK(ResourceProcessingEnvironment::screenMessages[0] == Localizer::Format("#autoLOC_6001051", {"80.00", "Oxidizer"}));
    // an unknown name does nothing
    jet->Set<std::string>("ResourceName", "Unobtainium");
    jet->FuelJettison();
    CHECK(lf->amount == 120.0);
    // the empty name (and the cloned module's "") dumps every resource of the part, in list order
    ox->amount = 50.0;
    jet->Set<std::string>("ResourceName", "");
    ResourceProcessingEnvironment::screenMessages.clear();
    KSPActionParam kp;
    jet->Actions()["FuelJettisonAction"]->Invoke(kp);
    CHECK(lf->amount == 0.0);
    CHECK(ox->amount == 0.0);
    CHECK(ResourceProcessingEnvironment::screenMessages.size() == static_cast<size_t>(rig.host->ResourceList().Count()));
}

// ============================================== ModuleResourceDrain ====================================================================

TEST_CASE("WP-28 ModuleResourceDrain: resource discovery, toggles, DrainPart amounts and forces, FX state, save / load") {
    if (!GameAvailable()) return;
    Env env;
    Game15 g;
    SceneScope flight(GameScenes::FLIGHT);
    Time::fixedDeltaTime = 0.02f;
    Time::deltaTime = 0.02f;
    Rig rig(g, "fuelTank");
    auto valve = g.New("ReleaseValve");
    valve->fields.persistentId = 2;
    valve->setParent(rig.host.get());
    rig.vessel.Adopt({rig.host.get(), valve.get()});
    PartResource* lf = rig.host->GetResource("LiquidFuel");
    PartResource* ox = rig.host->GetResource("Oxidizer");
    REQUIRE(lf != nullptr);
    REQUIRE(ox != nullptr);
    const double lfMax = lf->maxAmount;
    valve->fxGroupNames = {"gasDraining"};
    auto* dr = As<ModuleResourceDrain>(Find(*valve, "ModuleResourceDrain"));
    char dummy = 0;
    dr->snapshot = reinterpret_cast<ProtoPartModuleSnapshot*>(&dummy);  // FixedUpdate only drains a part that has a snapshot (a loaded flight vessel)
    // OnStart (:121): drainRate clamped to [1, 20]; every drainable resource of the PARENT is a candidate; Initialize creates the statuses
    dr->Set<float>("drainRate", 55.0f);
    dr->OnStart(StartState::Flying);
    CHECK(dr->Get<float>("drainRate") == 20.0f);
    dr->Set<float>("drainRate", 10.0f);
    REQUIRE(dr->resourcesAvailable.size() == 2);  // LiquidFuel and Oxidizer are drainable in ResourcesGeneric.cfg
    CHECK(dr->resourcesAvailable[0]->resourceName == lf->resourceName);
    CHECK(dr->resourceDrainStatus.size() == 2);
    CHECK(dr->drainFXGroups == std::vector<std::string>{"gasDraining"});
    CHECK(dr->Get<std::string>("noResourceAvailable") == Loc("#autoLOC_6006031"));
    CHECK_FALSE(dr->Fields()["noResourceAvailable"]->guiActive);
    CHECK(dr->Actions()["StartResourceDrainAction"]->guiName() == Loc("#autoLOC_6006003"));
    CHECK(dr->CanBeDetached());
    // nothing selected: TurnOnDrain does nothing (ResourcesToDrain() == 0)
    dr->TurnOnDrain();
    CHECK_FALSE(dr->IsDraining());
    CHECK(dr->Get<int32_t>("resourcesDraining") == 0);
    // select LiquidFuel and turn on: HasEnoughDrainingResources needs amount >= max / (100f / drainRate) * deltaTime
    dr->TogglePartResource(lf, true);
    CHECK(dr->IsResourceDraining(*lf));
    CHECK_FALSE(dr->IsResourceDraining(*ox));
    dr->TurnOnDrain();
    CHECK(dr->IsDraining());
    CHECK_FALSE(dr->CanBeOffset());
    CHECK(dr->Get<int32_t>("resourcesDraining") == 1);
    // the FX group: highest priority among draining resources with FX and something left; active, power = drainRate * 0.05
    REQUIRE(dr->highestPriorityFXGroup.has_value());
    CHECK(*dr->highestPriorityFXGroup == "gasDraining");
    CHECK(dr->fxGroupActive["gasDraining"]);
    CHECK(dr->fxGroupPower["gasDraining"] == F32(static_cast<double>(10.0f) * static_cast<double>(0.05f)));
    // DrainPart (:455): per draining resource drainAmount = maxAmount / (double)(100f / drainRate) * (double)deltaTime, requested from the
    // PARENT with NO_FLOW (flowMode off); the force is drainISP * 9.80665f * (float)(received * density) / Time.fixedDeltaTime
    const double per = static_cast<double>(F32(100.0 / 10.0));
    const double drainAmount = lfMax / per * static_cast<double>(0.02f);
    const double lf0 = lf->amount;
    dr->FixedUpdate();
    CHECK(dr->drainAmount == drainAmount);
    CHECK(lf->amount == doctest::Approx(lf0 - drainAmount).epsilon(1e-12));
    CHECK(dr->amtReceived == doctest::Approx(drainAmount).epsilon(1e-12));
    const double density = static_cast<double>(lf->info->density());
    const float vel = F32(static_cast<double>(lf->info->partResourceDrainDefinition().drainForceISP()) * static_cast<double>(9.80665f));
    REQUIRE(dr->addedForces.size() == 1);
    CHECK(dr->addedForces[0] == F32(static_cast<double>(vel) * static_cast<double>(static_cast<float>(dr->amtReceived * density)) / static_cast<double>(0.02f)));
    CHECK(ox->amount == ox->maxAmount);  // not selected: untouched
    // draining until empty: every call removes drainAmount; when nothing more can be taken (<= 1E-15) the drained-resource count equals
    // ResourcesToDrain and the drain switches off
    int ticks = 0;
    while (dr->IsDraining() && ticks++ < 5000) dr->FixedUpdate();
    CHECK_FALSE(dr->IsDraining());
    CHECK(lf->amount <= 1e-9);
    CHECK(ticks > 10);
    CHECK_FALSE(dr->fxGroupActive["gasDraining"]);
    // flow mode on: the request uses the resource's own flow mode (the whole vessel for LiquidFuel is just the tank here)
    dr->Set<bool>("flowMode", true);
    lf->amount = lfMax;
    dr->TurnOnDrain();
    CHECK(dr->IsDraining());
    const double lf1 = lf->amount;
    dr->FixedUpdate();
    CHECK(lf->amount == doctest::Approx(lf1 - drainAmount).epsilon(1e-12));
    dr->TurnOffDrain();
    CHECK_FALSE(dr->IsDraining());
    // ToggleDrain / the actions (:544, :173-:197)
    KSPActionParam p;
    dr->ToggleResourceDrainAction(p);
    CHECK(dr->IsDraining());
    dr->StopResourceDrainAction(p);
    CHECK_FALSE(dr->IsDraining());
    dr->StartResourceDrainAction(p);
    CHECK(dr->IsDraining());
    dr->ToggleResourceDrainFlowAction(p);
    CHECK_FALSE(dr->Get<bool>("flowMode"));
    dr->TurnOffDrain();
    // CheatOptions.InfinitePropellant: every resource counts as selected and drains for free
    ResourceFlowEnvironment::InfinitePropellant = true;
    CHECK(dr->ResourcesToDrain() == 2);
    const double lf2 = lf->amount;
    dr->DrainPart();
    CHECK(lf->amount == lf2);  // infinite: amtReceived = drainAmount without a request
    ResourceFlowEnvironment::InfinitePropellant = false;
    // save / load (:675, :696): the selected resources travel as a comma list; load restores the selection on the next OnStart
    ConfigNode saved("MODULE");
    dr->OnSave(saved);
    CHECK(saved.GetValue("resourcesToRelease") == std::string("LiquidFuel"));
    auto valve2 = g.New("ReleaseValve");
    valve2->fields.persistentId = 3;
    valve2->setParent(rig.host.get());
    auto* dr2 = As<ModuleResourceDrain>(Find(*valve2, "ModuleResourceDrain"));
    ConfigNode in("MODULE");
    in.AddValue("resourcesToRelease", "Oxidizer,,LiquidFuel");
    dr2->OnLoad(in);
    REQUIRE(dr2->loadedResources.has_value());
    CHECK(*dr2->loadedResources == std::vector<std::string>{"Oxidizer", "LiquidFuel"});
    dr2->OnStart(StartState::Flying);
    CHECK(dr2->ResourcesToDrain() == 2);
    dr2->UpdateResourcesToRelease({"Oxidizer"}, true);  // exclude: Oxidizer stops
    CHECK(dr2->ResourcesToDrain() == 1);
    dr2->UpdateResourcesToRelease({"Oxidizer"});        // include only: LiquidFuel is switched off, Oxidizer on
    CHECK(dr2->ResourcesToDrain() == 1);
    CHECK(dr2->IsResourceDraining(*ox));
    CHECK_FALSE(dr2->IsResourceDraining(*lf));
    // a part without a parent is inactive (:265)
    auto lone = g.New("ReleaseValve");
    lone->fields.persistentId = 4;
    auto* dr3 = As<ModuleResourceDrain>(Find(*lone, "ModuleResourceDrain"));
    dr3->LoadResources();
    CHECK(dr3->IsInactive());
    ConfigNode out3("MODULE");
    dr3->OnSave(out3);  // no resources: the loaded text is kept ("")
    CHECK(out3.GetValue("resourcesToRelease") == std::string());
    valve2->setParent(nullptr);
    valve->setParent(nullptr);
    rig.vessel.parts = {rig.host.get()};
    valve->vessel = nullptr;
}

// ============================================== space-object info / analysis ========================================================

TEST_CASE("WP-28 ModuleAsteroidInfo / ModuleCometInfo OnStart + Update, ModuleAnalysisResource, ModuleAsteroidAnalysis / ModuleCometAnalysis") {
    if (!GameAvailable()) return;
    Env env;
    Game15 g;
    SceneScope flight(GameScenes::FLIGHT);
    for (const bool comet : {false, true}) {
        CAPTURE(comet);
        Rig rig(g);
        AddModuleTo(*rig.host, comet ? "ModuleComet" : "ModuleAsteroid");
        auto* info = comet ? static_cast<ModuleSpaceObjectInfoBase*>(As<ModuleCometInfo>(AddModuleTo(*rig.host, "ModuleCometInfo")))
                           : static_cast<ModuleSpaceObjectInfoBase*>(As<ModuleAsteroidInfo>(AddModuleTo(*rig.host, "ModuleAsteroidInfo")));
        std::vector<ModuleSpaceObjectResource*> res;
        for (const char* n : {"Ore", "LiquidFuel"}) {
            auto* m = As<ModuleSpaceObjectResource>(AddModuleTo(*rig.host, comet ? "ModuleCometResource" : "ModuleAsteroidResource"));
            m->Set<std::string>("resourceName", n);
            m->Set<int32_t>("presenceChance", 100);
            m->Set<int32_t>("lowRange", 10);
            m->Set<int32_t>("highRange", 10);
            res.push_back(m);
        }
        ResourceEnvironment::tickCount = [] { return 1234; };
        // OnStart (:11): with the object module present, an unset current mass becomes the part's mass; with no threshold yet the resources are
        // rolled (flight scene); the part's mass and the object's mass module follow (float)currentMassVal
        rig.host->fields.mass = 12.5f;
        info->OnStart(StartState::Flying);
        REQUIRE(info->baseMod != nullptr);
        CHECK(info->currentMassVal() >= 12.5);  // 12.5 unless the rolled resources exceed it (then totalResourceMass * 1.01f)
        CHECK(info->massThresholdVal() > 0.0);
        CHECK(rig.host->fields.mass == static_cast<float>(info->currentMassVal()));
        IPartMassModifier* mm = info->baseMod->AsMassModifier();
        REQUIRE(mm != nullptr);
        CHECK(mm->GetModuleMass(0.0f, ModifierStagingSituation::CURRENT) == static_cast<float>(info->currentMassVal()));
        // two resources, both present (100 %) with range 10 %: each mass = current * 0.1, purity (float)10 * 0.01f
        CHECK(res[0]->Get<float>("abundance") > 0.0f);
        CHECK(res[0]->Get<float>("displayAbundance") == res[0]->Get<float>("abundance"));
        const double threshold = info->massThresholdVal();
        // a second OnStart keeps the rolled threshold (`massThresholdVal <= 1E-09` is false)
        info->OnStart(StartState::Flying);
        CHECK(info->massThresholdVal() == threshold);
        // Update (:37): the text fields in flight
        info->Update();
        CHECK(info->Get<std::string>("displayMass") == dotnet::FormatDoubleF(info->currentMassVal(), 5) + "t");
        CHECK(info->Get<std::string>("resources") ==
              Localizer::Format("#autoLOC_6001049", {dotnet::FormatDoubleF(info->currentMassVal() - threshold, 5),
                                                      dotnet::FormatDoubleF((info->currentMassVal() - threshold) / info->currentMassVal() * 100.0, 3)}));
        // the current / threshold are G17 strings: parse failure reads as 0 (double.TryParse leaves 0)
        info->Set<std::string>("currentMass", "not a number");
        CHECK(info->currentMassVal() == 0.0);
        info->set_currentMassVal(0.1);
        CHECK(info->Get<std::string>("currentMass") == "0.10000000000000001");  // G17
        // ModuleSpaceObjectResource.OnLoad (:28): flow mode from the definition, the FlowMode string overrides it, a bad name is logged
        res[0]->OnLoad(ConfigNode("MODULE"));
        CHECK(static_cast<int>(res[0]->_flowMode) == PartResourceLibrary::Instance()->GetDefinition("Ore")->resourceFlowMode());
        res[0]->Set<std::string>("FlowMode", "ALL_VESSEL");
        res[0]->OnLoad(ConfigNode("MODULE"));
        CHECK(res[0]->_flowMode == ResourceFlowMode::ALL_VESSEL);
        res[0]->Set<std::string>("FlowMode", "SOMETIMES");
        {
            LogCapture log;
            res[0]->OnLoad(ConfigNode("MODULE"));
            CHECK(log.Has(log::Level::Error, "Error parsing flow mode"));
        }
        res[1]->Set<std::string>("resourceName", "Unobtainium");
        res[1]->OnLoad(ConfigNode("MODULE"));
        CHECK(res[1]->_flowMode == ResourceFlowMode::NULL_);

        // ModuleAnalysisResource (:16-:42): a status text only for a positive abundance; the field's name is the resource's
        auto* ar = As<ModuleAnalysisResource>(AddModuleTo(*rig.host, "ModuleAnalysisResource"));
        ar->Set<std::string>("resourceName", "Ore");
        ar->OnStart(StartState::Flying);
        CHECK(ar->Fields()["status"]->guiName == "Ore");
        ar->Update();
        CHECK_FALSE(ar->statusFldGuiActive);
        ar->Set<float>("abundance", 0.5f);
        ar->Set<float>("displayAbundance", 0.123456f);
        ar->Update();
        CHECK(ar->statusFldGuiActive);
        CHECK(ar->Get<std::string>("status") == dotnet::FormatDoubleF(static_cast<double>(F32(static_cast<double>(0.123456f) * 100.0)), 4) + "%");
    }
}

TEST_CASE("WP-28 ModuleAsteroidAnalysis / ModuleCometAnalysis: finding the object, copying abundances, clearing, event wiring") {
    if (!GameAvailable()) return;
    Env env;
    Game15 g;
    SceneScope flight(GameScenes::FLIGHT);
    for (const bool comet : {false, true}) {
        CAPTURE(comet);
        Rig rig(g);  // the analysis part
        auto potato = g.New("batteryBank");
        potato->fields.persistentId = 2;
        AddModuleTo(*potato, comet ? "ModuleComet" : "ModuleAsteroid");
        auto* ore = As<ModuleSpaceObjectResource>(AddModuleTo(*potato, comet ? "ModuleCometResource" : "ModuleAsteroidResource"));
        ore->Set<std::string>("resourceName", "Ore");
        ore->Set<float>("abundance", 0.3f);
        ore->Set<float>("displayAbundance", 0.25f);
        auto* lf = As<ModuleSpaceObjectResource>(AddModuleTo(*potato, comet ? "ModuleCometResource" : "ModuleAsteroidResource"));
        lf->Set<std::string>("resourceName", "LiquidFuel");
        lf->Set<float>("abundance", 0.1f);
        auto* oreAn = As<ModuleAnalysisResource>(AddModuleTo(*rig.host, "ModuleAnalysisResource"));
        oreAn->Set<std::string>("resourceName", "Ore");
        auto* an = comet ? static_cast<ModuleSpaceObjectAnalysisBase*>(As<ModuleCometAnalysis>(AddModuleTo(*rig.host, "ModuleCometAnalysis")))
                         : static_cast<ModuleSpaceObjectAnalysisBase*>(As<ModuleAsteroidAnalysis>(AddModuleTo(*rig.host, "ModuleAsteroidAnalysis")));
        // OnStart in flight subscribes to onVesselWasModified and runs CheckForPotato: no object on the vessel yet -> cleared
        oreAn->Set<float>("abundance", 0.9f);
        an->OnStart(StartState::Flying);
        CHECK(oreAn->Get<float>("abundance") == 0.0f);
        CHECK(oreAn->Get<float>("displayAbundance") == 0.0f);
        // the object joins: the analyzer whose resourceName matches gets the abundance and display abundance; a resource without an analyzer is
        // cached as a null entry (Dictionary.Add(rName, null))
        rig.pv.parts = {rig.host.get(), potato.get()};
        events::GameEvents::onVesselWasModified.Fire(static_cast<Vessel*>(nullptr));
        CHECK(oreAn->Get<float>("abundance") == 0.3f);
        CHECK(oreAn->Get<float>("displayAbundance") == 0.25f);
        REQUIRE(an->analyzers.count("Ore") == 1);
        CHECK(an->analyzers["Ore"] == oreAn);
        REQUIRE(an->analyzers.count("LiquidFuel") == 1);
        CHECK(an->analyzers["LiquidFuel"] == nullptr);
        CHECK(an->potatoPart == potato.get());
        // FindMatchingAnalyzer answers from the cache
        CHECK(an->FindMatchingAnalyzer(*rig.host, "Ore") == oreAn);
        CHECK(an->FindMatchingAnalyzer(*rig.host, "Unknown") == nullptr);
        // the object leaves: everything is cleared again (the analyzers of the analysis part and of the old object part)
        rig.pv.parts = {rig.host.get()};
        events::GameEvents::onVesselWasModified.Fire(static_cast<Vessel*>(nullptr));
        CHECK(oreAn->Get<float>("abundance") == 0.0f);
        CHECK(an->potatoPart == nullptr);
        CHECK(an->analyzers.empty());
        // OnDestroy unsubscribes: a later change is not seen
        an->OnDestroy();
        rig.pv.parts = {rig.host.get(), potato.get()};
        events::GameEvents::onVesselWasModified.Fire(static_cast<Vessel*>(nullptr));
        CHECK(oreAn->Get<float>("abundance") == 0.0f);
    }
}

// ============================================== scanners ===============================================================================

TEST_CASE("WP-28 ModuleResourceScanner: scanning mode names, abundance request, display branches, situation cache") {
    if (!GameAvailable()) return;
    Env env;
    Game15 g;
    Kerbol28 kerbol;
    ResourceScenario scenario;
    scenario.Awake();
    scenario.gameSettings()->Seed = 1985366484;
    ResourceEnvironment::difficultyResourceAbundance = [] { return 1.0f; };
    ResourceMap& map = *ResourceMap::Instance();
    FreshMap(scenario);
    SceneScope flight(GameScenes::FLIGHT);
    Rig rig(g, "SurfaceScanner");
    auto* sc = As<ModuleResourceScanner>(Find(*rig.host, "ModuleResourceScanner"));
    REQUIRE(sc->Get<std::string>("ResourceName") == "Ore");
    sc->OnAwake();
    sc->OnStart(StartState::Flying);
    // GetScanningMode (:58): by ScannerType, with Convert.ToInt32(abbrevation) as the parameter
    sc->Set<int32_t>("ScannerType", 0);
    CHECK(sc->GetScanningMode(true) == Localizer::Format("#autoLOC_6001082", {"1"}));
    CHECK(sc->GetScanningMode(false) == Localizer::Format("#autoLOC_6001082", {"0"}));
    sc->Set<int32_t>("ScannerType", 1);
    CHECK(sc->GetScanningMode(true) == Loc("#autoLOC_260065"));
    sc->Set<int32_t>("ScannerType", 2);
    CHECK(sc->GetScanningMode(false) == Localizer::Format("#autoLOC_6001083", {"0"}));
    sc->Set<int32_t>("ScannerType", 3);
    CHECK(sc->GetScanningMode(true) == Localizer::Format("#autoLOC_6001084", {"1"}));
    sc->Set<int32_t>("ScannerType", 9);
    CHECK(sc->GetScanningMode(true) == "");
    sc->Set<int32_t>("ScannerType", 0);
    CHECK(sc->ResourceDisplayName == PartResourceLibrary::Instance()->GetDefinition("Ore")->displayName());
    CHECK(sc->GetDisplayFormat() == sc->ResourceDisplayName + "[" + sc->GetScanningMode(true) + "]");
    // OnStart sets the field name BEFORE ResourceDisplayName is looked up (:87): the first name has an empty resource name
    CHECK(sc->Fields()["abundanceDisplay"]->guiName == "[" + sc->GetScanningMode(true) + "]");
    // IsSituationValid (:200): false when the vessel carries an asteroid or a comet; cached by the part count
    CHECK(sc->IsSituationValid());
    auto rock = g.New("batteryBank");
    AddModuleTo(*rock, "ModuleComet");
    rig.pv.parts = {rig.host.get(), rock.get()};
    CHECK_FALSE(sc->IsSituationValid());
    rig.pv.parts = {rig.host.get(), rig.host.get()};
    CHECK_FALSE(sc->IsSituationValid());  // same count (2): the cached answer
    rig.pv.parts = {rig.host.get()};
    CHECK(sc->IsSituationValid());
    // Update (:101): the abundance of the module's request. The request carries CheckForLock = RequiresUnlock
    rig.pv.mainBody.flightGlobalsIndex = 1;
    rig.pv.latitude = 10.0;
    rig.pv.longitude = 20.0;
    rig.pv.altitude = 100.0;
    AbundanceRequest req;
    req.BodyId = 1;
    req.Latitude = 10.0;
    req.Longitude = 20.0;
    req.Altitude = 100.0;
    req.CheckForLock = sc->Get<bool>("RequiresUnlock");
    req.ResourceType = HarvestTypes::Planetary;
    req.ResourceName = "Ore";
    sc->Update();
    CHECK(sc->abundanceValue == static_cast<double>(map.GetAbundance(req)));
    CHECK(sc->aDispFldGuiActive);
    // display: the part window must be open for the text to be built (UIPartActionController.ItemListContains): not open, no text
    sc->Set<std::string>("abundanceDisplay", "x");
    sc->Update();
    CHECK(sc->Get<std::string>("abundanceDisplay") == "x");
    ResourceFlowEnvironment::partActionUIContains = [](const Part*) { return true; };
    sc->Set<float>("MaxAbundanceAltitude", 1000.0f);
    rig.pv.radarAltitude = 5000.0;
    rig.pv.Landed = false;
    sc->Update();  // type 0 above the altitude limit, not landed: autoLOC_260116
    CHECK(sc->Get<std::string>("abundanceDisplay") == Loc("#autoLOC_260116"));
    sc->Set<int32_t>("ScannerType", 1);
    rig.pv.Splashed = false;
    sc->Update();
    CHECK(sc->Get<std::string>("abundanceDisplay") == Loc("#autoLOC_260121"));
    sc->Set<int32_t>("ScannerType", 2);
    rig.pv.mainBody.atmosphere = false;
    sc->Update();
    CHECK(sc->Get<std::string>("abundanceDisplay") == Loc("#autoLOC_260125"));
    // DisplayAbundance (:146): the biome (here "Shores") decides locking for type 0; the text is the percentage with 2 digits above 1 %,
    // 4 digits above 1E-9, else "0%"; a locked biome wraps it in autoLOC_6002267
    sc->Set<int32_t>("ScannerType", 0);
    rig.pv.radarAltitude = 10.0;
    ResourceProcessingEnvironment::biomeAt = [](double, double, const Part&) { return std::optional<ResourceProcessingEnvironment::Biome>({"Shores", "Shores"}); };
    sc->abundanceValue = 0.5;
    sc->DisplayAbundance();
    CHECK(sc->Get<std::string>("abundanceDisplay") == Localizer::Format("#autoLOC_6002267", {"50.00%"}));
    map.UnlockBiome(1, "Shores");
    sc->DisplayAbundance();
    CHECK(sc->Get<std::string>("abundanceDisplay") == "50.00%");
    sc->abundanceValue = 0.005;
    sc->DisplayAbundance();
    CHECK(sc->Get<std::string>("abundanceDisplay") == "0.5000%");
    sc->abundanceValue = 0.0;
    sc->DisplayAbundance();
    CHECK(sc->Get<std::string>("abundanceDisplay") == "0%");
    // an unlocked-less biome with zero abundance for the locked case shows the locked text
    ResourceProcessingEnvironment::biomeAt = [](double, double, const Part&) { return std::optional<ResourceProcessingEnvironment::Biome>({"Highlands", "Highlands"}); };
    sc->DisplayAbundance();
    CHECK(sc->Get<std::string>("abundanceDisplay") == Localizer::Format("#autoLOC_6002267", {Loc("#autoLOC_260145")}));
    // the events helper (:194) and the module switches (:38-:48)
    sc->ToggleEvent("ToggleStaging", false);
    CHECK_FALSE(sc->Events()["ToggleStaging"]->active);
    sc->DisableModule();
    CHECK_FALSE(sc->ModuleIsActive());
    sc->EnableModule();
    CHECK(sc->ModuleIsActive());
    ResourceFlowEnvironment::partActionUIContains = nullptr;
}

TEST_CASE("WP-28 ModuleOrbitalScanner: overlay state machine, cutoff clamps, colour cycle, resource name text, survey / menu toggles") {
    if (!GameAvailable()) return;
    Env env;
    Game15 g;
    Kerbol28 kerbol;
    ResourceScenario scenario;
    scenario.Awake();
    ResourceMap& map = *ResourceMap::Instance();
    FreshMap(scenario);
    SceneScope flight(GameScenes::FLIGHT);
    Rig rig(g, "SurveyScanner");
    OverlayGeneratorState& ov = ResourceProcessingEnvironment::overlay;
    auto* sc = As<ModuleOrbitalScanner>(Find(*rig.host, "ModuleOrbitalScanner"));
    rig.pv.mainBody.flightGlobalsIndex = 1;
    rig.pv.mainBody.displayName = "Kerbin";
    sc->Start();
    REQUIRE(sc->switchR != nullptr);
    REQUIRE(sc->toggleO != nullptr);
    REQUIRE(sc->setCol != nullptr);
    // the planet is not scanned: FixedUpdate (:180) points the overlay at the vessel's body (clearing the old display) and hides both menus
    sc->_ShowSurveyMenu = true;
    sc->_ShowOverlayMenu = true;
    sc->FixedUpdate();
    CHECK(ov.DisplayBody == 1);
    CHECK(ov.clearDisplayCalls == 1);
    CHECK_FALSE(sc->switchR->active);
    CHECK_FALSE(sc->toggleO->active);
    CHECK_FALSE(sc->setCol->active);
    CHECK_FALSE(sc->Events()["IncreaseCutoff"]->active);
    CHECK_FALSE(sc->Events()["DecreaseCutoff"]->active);
    // toggling while the planet is unscanned does nothing; once scanned the survey menu is shown
    sc->ToggleOverlay();
    CHECK_FALSE(ov.IsActive);
    map.UnlockPlanet(1);
    sc->FixedUpdate();
    CHECK(sc->switchR->active);
    CHECK(sc->toggleO->active);
    // ToggleOverlay (:34): scanned -> the overlay turns on and is generated with the module's CheckForLock; again -> off and cleared
    sc->ToggleOverlay();
    CHECK(ov.IsActive);
    CHECK(ov.generateOverlayCalls == 1);
    CHECK(ov.lastCheckForLock == sc->Get<bool>("CheckForLock"));
    sc->FixedUpdate();
    CHECK(sc->setCol->active);  // the overlay is active: the colour / cutoff menu
    CHECK(sc->Events()["IncreaseCutoff"]->active);
    const int clears = ov.clearDisplayCalls;
    sc->ToggleOverlay();
    CHECK_FALSE(ov.IsActive);
    CHECK(ov.clearDisplayCalls == clears + 1);
    // cutoff (:99, :113): steps of 10 within [0, 100], regenerating while the overlay is active
    ov.Cutoff = 95;
    sc->IncreaseCutoff();
    CHECK(ov.Cutoff == 100);
    ov.Cutoff = 5;
    sc->DecreaseCutoff();
    CHECK(ov.Cutoff == 0);
    sc->DecreaseCutoff();
    CHECK(ov.Cutoff == 0);
    ov.IsActive = true;
    const int gens = ov.generateOverlayCalls;
    sc->IncreaseCutoff();
    CHECK(ov.Cutoff == 10);
    CHECK(ov.generateOverlayCalls == gens + 1);
    ov.IsActive = false;
    // colour (:64): the display mode advances and wraps at 4; the button text uses the localised colour name
    ov.DisplayMode = 2;
    sc->SetColor();
    CHECK(ov.DisplayMode == 3);
    CHECK(sc->setCol->guiName() == Localizer::Format("#autoLOC_6001482", {Loc("#autoLOC_6002358")}));
    sc->SetColor();
    CHECK(ov.DisplayMode == 0);
    CHECK(sc->GetOverlayColor(0) == Loc("#autoLOC_7003278"));
    CHECK(sc->GetOverlayColor(1) == Loc("#autoLOC_7003277"));
    CHECK(sc->GetOverlayColor(2) == Loc("#autoLOC_7003276"));
    CHECK(sc->GetOverlayColor(5) == "5");
    // ShowResourceName (:225): the highest and lowest abundance of the displayed resource on the displayed body; the text is (low + high) / 0.02 %
    ov.ResourceList = {{"Ore", "Ore"}};
    ov.DisplayResource = -1;
    auto& cache = ResourceCache::Instance().AbundanceCache();
    auto add = [&](int body, const char* res, float ab, HarvestTypes t) {
        auto s = std::make_shared<ResourceCache::AbundanceSummary>();
        s->BodyId = body;
        s->ResourceName = res;
        s->Abundance = ab;
        s->HarvestType = t;
        cache.push_back(s);
    };
    const auto savedCache = cache;  // the real abundance summaries of the stock configs
    cache.clear();
    add(1, "Ore", 0.2f, HarvestTypes::Planetary);
    add(1, "Ore", 0.4f, HarvestTypes::Planetary);
    add(1, "Ore", 0.9f, HarvestTypes::Oceanic);   // another type: ignored
    add(2, "Ore", 0.7f, HarvestTypes::Planetary);  // another body: ignored
    add(1, "Rock", 0.8f, HarvestTypes::Planetary); // another resource: ignored
    sc->ShowResourceName();
    CHECK(ov.DisplayResource == 0);
    const float lo = 0.2f, hi = 0.4f;
    CHECK(sc->switchR->guiName() == "Ore" + Localizer::Format("#autoLOC_6002267", {dotnet::FormatDoubleF(static_cast<double>(F32(static_cast<double>(lo) + static_cast<double>(hi))) / 0.02, 2) + "%"}));
    cache = savedCache;
    // SwitchResource (:21) asks the generator for the next resource; ActivateScanner (:127) sets up and shows the map
    ov.ResourceList = {{"Ore", "Ore"}, {"Rock", "Rock"}};
    ov.DisplayResource = 0;
    sc->SwitchResource();
    CHECK(ov.fetchNextResourceCalls == 1);
    CHECK(ov.DisplayResource == 1);
    ov.IsActive = false;
    sc->ActivateScanner();
    CHECK(ov.IsActive);
    CHECK(ov.DisplayBody == 1);
    // DisableModule clears the display of an enabled module; IsSituationValid needs a biome map (:270)
    const int clears2 = ov.clearDisplayCalls;
    sc->DisableModule();
    CHECK_FALSE(sc->isEnabled);
    CHECK(ov.clearDisplayCalls == clears2 + 1);
    sc->DisableModule();
    CHECK(ov.clearDisplayCalls == clears2 + 1);
    sc->EnableModule();
    CHECK(sc->isEnabled);
    CHECK(sc->ModuleIsActive() == ov.IsActive);
    CHECK(sc->IsSituationValid());
    rig.pv.mainBody.hasBiomeMap = false;
    CHECK_FALSE(sc->IsSituationValid());
    REQUIRE_FALSE(ResourceProcessingEnvironment::screenMessages.empty());
    CHECK(ResourceProcessingEnvironment::screenMessages.back() == Localizer::Format("#autoLOC_6001055", {"Kerbin"}));
    // UpdateGUI (:166): only while the part window is open: the colour button text with the cutoff
    ResourceFlowEnvironment::partActionUIContains = [](const Part*) { return true; };
    ov.DisplayMode = 1;
    ov.Cutoff = 30;
    sc->UpdateGUI();
    CHECK(sc->setCol->guiName() == Localizer::Format(Loc("#autoLOC_6001054"), {Loc("#autoLOC_7003277"), "30"}));
    ResourceFlowEnvironment::partActionUIContains = nullptr;
    // Start (:213) subscribes to OnResourceMapLoaded when the map is not initialised yet; the handler (:286) resets both menus and shows
    // the resource name. Here the map exists, so there is no subscription: call the handler directly
    ov.ResourceList = {{"Ore", "Ore"}};
    ov.DisplayResource = 0;
    ov.DisplayMode = 0;
    sc->OnResourceMapLoaded();
    CHECK_FALSE(sc->_ShowSurveyMenu);
    CHECK_FALSE(sc->_ShowOverlayMenu);
    CHECK_FALSE(sc->switchR->active);
    sc->OnDestroy();
}

TEST_CASE("WP-28 ModuleOrbitalSurveyor and ModuleBiomeScanner: thresholds, survey, completion, scan data, biome unlock") {
    if (!GameAvailable()) return;
    Env env;
    Game15 g;
    ResourceScenario scenario;
    scenario.Awake();
    ResourceMap& map = *ResourceMap::Instance();
    FreshMap(scenario);
    SceneScope flight(GameScenes::FLIGHT);
    Rig rig(g, "SurveyScanner");
    auto* sv = As<ModuleOrbitalSurveyor>(Find(*rig.host, "ModuleOrbitalSurveyor"));
    OverlayGeneratorState& ov = ResourceProcessingEnvironment::overlay;
    rig.pv.mainBody.flightGlobalsIndex = 1;
    rig.pv.mainBody.Radius = 600000.0;
    rig.pv.mainBody.sphereOfInfluence = 84159286.0;
    rig.pv.mainBody.bodyName = "Kerbin";
    rig.pv.mainBody.displayName = "Kerbin";
    rig.pv.mainBody.recoveryValue = 0.5f;
    sv->OnStart(StartState::Flying);
    REQUIRE(sv->perfSurv != nullptr);
    const double minT = sv->Get<double>("minThreshold"), maxT = sv->Get<double>("maxThreshold");
    // PerformSurvey (:17): a polar orbit between max(R / 10, minThreshold) and min(5 R, maxThreshold) sends the data; the amount is
    // (float)R * 0.001f / (float)ScanTime and the subject "survey@<body>"
    float sent = -1.0f;
    std::string subject;
    sv->transmit = [&](float amount, const std::string& id) {
        sent = amount;
        subject = id;
        return true;
    };
    rig.pv.situation = VesselSituations::ORBITING;
    rig.pv.inclination = 85.0;
    rig.pv.PeA = 100000.0;
    rig.pv.ApA = 200000.0;
    sv->PerformSurvey();
    CHECK(sent == F32(static_cast<double>(F32(600000.0)) * static_cast<double>(0.001f) / static_cast<double>(static_cast<float>(sv->Get<int32_t>("ScanTime")))));
    CHECK(subject == "survey@Kerbin");
    // outside the band, below 80 degrees or not orbiting: the screen message with the band in km ("0" format)
    sent = -1.0f;
    rig.pv.inclination = 79.9;
    ResourceProcessingEnvironment::screenMessages.clear();
    sv->PerformSurvey();
    CHECK(sent == -1.0f);
    const double lo = dotnet::Math::Max(600000.0 / 10.0, minT);
    const double hi = dotnet::Math::Min(600000.0 * 5.0, maxT);
    REQUIRE(ResourceProcessingEnvironment::screenMessages.size() == 1);
    CHECK(ResourceProcessingEnvironment::screenMessages[0] == Localizer::Format("#autoLOC_6001056", {dotnet::FormatDoubleF(lo / 1000.0, 0), dotnet::FormatDoubleF(hi / 1000.0, 0)}));
    rig.pv.inclination = 90.0;
    rig.pv.PeA = lo - 1.0;
    sv->PerformSurvey();
    CHECK(sent == -1.0f);
    rig.pv.PeA = 100000.0;
    rig.pv.situation = VesselSituations::SUB_ORBITAL;
    sv->PerformSurvey();
    CHECK(sent == -1.0f);
    rig.pv.situation = VesselSituations::ORBITING;
    // no transmitter: the message autoLOC_259348
    sv->transmit = nullptr;
    ResourceProcessingEnvironment::screenMessages.clear();
    sv->PerformSurvey();
    REQUIRE(ResourceProcessingEnvironment::screenMessages.size() == 1);
    CHECK(ResourceProcessingEnvironment::screenMessages[0] == Loc("#autoLOC_259348"));
    // rescaleThresholds (:44) for a tiny body (radius below minThreshold with custom thresholds): Gilly-like R = 13000, SOI = 126123
    rig.pv.mainBody.Radius = 13000.0;
    rig.pv.mainBody.sphereOfInfluence = 126123.0;
    sv->Set<bool>("useCustomThresholds", true);
    sv->Set<double>("minThreshold", 25000.0);
    sv->Set<double>("maxThreshold", 1500000.0);
    sv->OnStart(StartState::Flying);  // flight scene + Radius < minThreshold: rescale
    const double num = 126123.0 - 13000.0;
    const double num2 = dotnet::Math::Sqrt(num / 1500000.0);
    double wantMax = num;
    double wantMin = dotnet::Math::Max(3000.0, 25000.0 * num2);
    if (wantMin > 0.95 * wantMax) wantMin = 0.95 * wantMax;
    CHECK(sv->Get<double>("maxThreshold") == wantMax);
    CHECK(sv->Get<double>("minThreshold") == wantMin);
    // min above max is pulled down to max before scaling; a floor of 3000 m
    sv->Set<double>("minThreshold", 2000000.0);
    sv->Set<double>("maxThreshold", 1500000.0);
    sv->rescaleThresholds();
    CHECK(sv->Get<double>("maxThreshold") == num);
    CHECK(sv->Get<double>("minThreshold") == dotnet::Math::Min(dotnet::Math::Max(3000.0, num * dotnet::Math::Sqrt(num / 1500000.0)), 0.95 * num));
    rig.pv.mainBody.Radius = 600000.0;
    // CompleteSurvey (:86): only for this part's data and a "survey@" subject that was not aborted: science SciBonus * RecoveryValue (float),
    // the planet is unlocked, the event hidden, the overlay shown for the body and OnOrbitalSurveyCompleted fired
    float science = 0.0f;
    sv->addScience = [&](float s) { science = s; };
    sv->flightID = 77;
    sv->CompleteSurvey(78, "survey@Kerbin", false);
    sv->CompleteSurvey(77, "other@Kerbin", false);
    sv->CompleteSurvey(77, "survey@Kerbin", true);
    CHECK(science == 0.0f);
    CHECK_FALSE(map.IsPlanetScanned(1));
    CHECK(sv->surveysCompleted == 0);
    sv->perfSurv->active = true;
    sv->CompleteSurvey(77, "survey@Kerbin", false);
    CHECK(science == F32(static_cast<double>(static_cast<float>(sv->Get<int32_t>("SciBonus"))) * static_cast<double>(0.5f)));
    CHECK(map.IsPlanetScanned(1));
    CHECK_FALSE(sv->perfSurv->active);
    CHECK(ov.DisplayBody == 1);
    CHECK(ov.lastCheckForLock);
    CHECK(sv->surveysCompleted == 1);
    // Update / CheckForScanData (:119, :130): the survey button follows "not scanned yet"
    sv->Update();
    CHECK_FALSE(sv->perfSurv->active);
    scenario.gameSettings()->GetPlanetScanInfo().clear();
    map.ResetCache();
    sv->Update();
    CHECK(sv->perfSurv->active);
    CHECK(sv->IsSituationValid());
    sv->DisableModule();
    CHECK_FALSE(sv->ModuleIsActive());
    sv->EnableModule();
    CHECK(sv->ModuleIsActive());
    sv->OnDestroy();

    // ModuleBiomeScanner (:6, :32)
    auto* bs = As<ModuleBiomeScanner>(AddModuleTo(*rig.host, "ModuleBiomeScanner"));
    rig.pv.latitude = 5.0;
    rig.pv.longitude = 6.0;
    rig.pv.SituationString = "Landed at KSC";
    rig.pv.Landed = true;
    rig.pv.controlLevel = ControlLevel::NONE;
    ResourceProcessingEnvironment::screenMessages.clear();
    bs->RunAnalysis();  // no control: the message, nothing unlocked
    REQUIRE(ResourceProcessingEnvironment::screenMessages.size() == 1);
    CHECK(ResourceProcessingEnvironment::screenMessages[0] == Loc("#autoLOC_258793"));
    CHECK_FALSE(map.IsBiomeUnlocked(1, "Landed at KSC"));
    rig.pv.controlLevel = ControlLevel::PARTIAL_UNMANNED;
    bs->RunAnalysis();  // no biome: the situation string unlocks
    CHECK(map.IsBiomeUnlocked(1, "Landed at KSC"));
    ResourceProcessingEnvironment::biomeAt = [](double lat, double lon, const Part&) {
        // ResourceUtilities.Deg2Rad(clampLat / clampLon) of the vessel's position
        return std::optional<ResourceProcessingEnvironment::Biome>(lat > 0.0 && lon > 0.0 ? ResourceProcessingEnvironment::Biome{"Shores", "The Shores"} : ResourceProcessingEnvironment::Biome{});
    };
    ResourceProcessingEnvironment::screenMessages.clear();
    bs->RunAnalysis();
    CHECK(ResourceProcessingEnvironment::screenMessages.back() == Localizer::Format("#autoLOC_6001050", {"Kerbin", "The Shores"}));
    CHECK(map.IsBiomeUnlocked(1, "Shores"));
    // FixedUpdate: landed or splashed with a locked biome shows the event; unlocked or flying hides it
    BaseEvent* ev = bs->Events()["RunAnalysis"];
    rig.pv.SituationString = "Landed at Runway";
    ResourceProcessingEnvironment::biomeAt = [](double, double, const Part&) { return std::optional<ResourceProcessingEnvironment::Biome>(); };
    bs->FixedUpdate();
    CHECK(ev->active);  // "Landed at Runway" is not unlocked
    map.UnlockBiome(1, "Landed at Runway");
    bs->FixedUpdate();
    CHECK_FALSE(ev->active);
    rig.pv.SituationString = "Landed at Island";
    bs->FixedUpdate();
    CHECK(ev->active);
    rig.pv.Landed = false;
    bs->FixedUpdate();
    CHECK_FALSE(ev->active);  // flying
    {
        SceneScope editor(GameScenes::EDITOR);
        ev->active = true;
        bs->FixedUpdate();
        CHECK(ev->active);  // not flight: untouched
    }
}

TEST_CASE("WP-28 ProcessingVessel.FromVessel copies a real Vessel; ResourceProcessingEnvironment defaults") {
    Env env;
    Vessel v;
    v.Landed = true;
    v.Splashed = false;
    v.altitude = 123.0;
    v.radarAltitude = 12.0;
    v.latitude = -1.5;
    v.longitude = 3.5;
    v.atmDensity = 0.25;
    v.srfSpeed = 9.0;
    v.horizontalSrfSpeed = 8.0;
    v.solarFlux = 1360.0;
    v.situation = VesselSituations::LANDED;
    v.ctrlState.mainThrottle = 0.75f;
    const ProcessingVessel r = ResourceProcessingEnvironment::FromVessel(v);
    CHECK(r.Landed);
    CHECK_FALSE(r.Splashed);
    CHECK(r.LandedOrSplashed());
    CHECK(r.altitude == 123.0);
    CHECK(r.radarAltitude == 12.0);
    CHECK(r.latitude == -1.5);
    CHECK(r.longitude == 3.5);
    CHECK(r.atmDensity == 0.25);
    CHECK(r.srfSpeed == 9.0);
    CHECK(r.horizontalSrfSpeed == 8.0);
    CHECK(r.solarFlux == 1360.0);
    CHECK(r.situation == VesselSituations::LANDED);
    CHECK(r.mainThrottle == 0.75f);
    CHECK_FALSE(r.isActiveVessel);
    // without a vessel hook (or a Part.vessel) there is no vessel record
    CHECK(ResourceProcessingEnvironment::VesselOf(nullptr) == nullptr);
    // the UT source: explicit hook first, then the resource environment's
    ResourceProcessingEnvironment::universalTime = [] { return 42.0; };
    CHECK(ResourceProcessingEnvironment::UniversalTime() == 42.0);
    ResourceProcessingEnvironment::universalTime = nullptr;
    ResourceEnvironment::universalTime = [] { return 43.0; };
    CHECK(ResourceProcessingEnvironment::UniversalTime() == 43.0);
    ResourceEnvironment::universalTime = nullptr;
}
