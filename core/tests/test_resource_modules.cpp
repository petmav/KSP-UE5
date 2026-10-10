// WP-15 unit tests, part 2: ModuleResource / ModuleResourceHandler, ResourceBroker / ResourceConverter, the crossfeed / fuel line /
// EVA container / auto-shift modules, and the planetary resource layer (NoiseGenerator, ResourceGameSettings, ResourceMap).
// Expectations: derived from the decompiled source (derivations in the comments), from the game's cfg files and save (read directly
// from the nodes), or from tools/wp15/abundance_reference.py (an independent Python implementation of ResourceMap.GetAbundance).
#include <fstream>

#include "ksp/celestial/celestial_system.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/core/unity_time.hpp"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::support;

namespace {

ConfigNode::NodePtr ParseOne(const char* text) {
    auto n = ConfigNode::Parse(text);
    REQUIRE(n);
    REQUIRE(n->nodes().Count() == 1);
    return n->nodes().Shared(0);
}

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}

// PartModule.resHandler of the module of class `cls` added to `part` (Part.AddModule: AddComponent + Awake)
PartModule* AddModuleTo(Part& part, const char* cls) {
    PartModuleData* md = part.AddModule(cls, true);
    REQUIRE(md != nullptr);
    REQUIRE(md->module != nullptr);
    return md->module.get();
}

}  // namespace

// ================================================== ModuleResource / ModuleResourceHandler =================================

TEST_CASE("WP-15 ModuleResource: Load / Save, flow mode default from the definition, IsDeprived") {
    if (!GameAvailable()) return;
    Game15 g;
    ModuleResource r;
    auto node = ParseOne("INPUT_RESOURCE\n{\n name = ElectricCharge\n rate = 1.5\n amount = 20\n varyTime = False\n useSI = True\n displayUnitMult = 60\n unitName = /min\n}\n");
    r.Load(*node);
    CHECK(r.name == "ElectricCharge");
    CHECK(r.id == Id("ElectricCharge"));
    CHECK(r.rate == 1.5);
    CHECK(r.amount == 20.0);
    CHECK_FALSE(r.varyTime);
    CHECK(r.useSI);
    CHECK(r.displayUnitMult == 60.0);
    CHECK(r.unitName == "/min");
    CHECK(r.flowMode == ResourceFlowMode::STAGE_PRIORITY_FLOW);  // no resourceFlowMode value: the definition's flowMode (:80)
    CHECK(r.resourceDef() == PartResourceLibrary::Instance()->GetDefinition("ElectricCharge"));
    // Save writes name / rate / amount, and resourceFlowMode only when it differs from the definition's
    ConfigNode out("INPUT_RESOURCE");
    r.Save(out);
    CHECK(out.values().Count() == 3);
    r.flowMode = ResourceFlowMode::ALL_VESSEL;
    ConfigNode out2("INPUT_RESOURCE");
    r.Save(out2);
    CHECK(out2.values().Count() == 4);
    CHECK(*out2.GetValue("resourceFlowMode") == "ALL_VESSEL");
    // an explicit resourceFlowMode wins; a bad name throws (Enum.Parse)
    ModuleResource r2;
    r2.Load(*ParseOne("OUTPUT_RESOURCE\n{\n name = Ore\n rate = 2\n resourceFlowMode = STACK_PRIORITY_SEARCH\n}\n"));
    CHECK(r2.flowMode == ResourceFlowMode::STACK_PRIORITY_SEARCH);
    ModuleResource r3;
    CHECK_THROWS(r3.Load(*ParseOne("OUTPUT_RESOURCE\n{\n name = Ore\n resourceFlowMode = SOMETIMES\n}\n")));
    // an unknown resource keeps the NULL flow mode
    ModuleResource r4;
    r4.Load(*ParseOne("OUTPUT_RESOURCE\n{\n name = Unobtainium\n rate = 1\n}\n"));
    CHECK(r4.flowMode == ResourceFlowMode::NULL_);
    // Save before the definition was read is a NullReferenceException (the C# reads the cached field, not the property)
    ModuleResource fresh;
    CHECK_THROWS_AS(fresh.Save(out), NullReferenceException);

    // IsDeprived (:143): currentAmount < currentRequest * threshold
    r.currentRequest = 10.0;
    r.currentAmount = 8.9;
    CHECK(r.IsDeprived());           // 8.9 < 10 * 0.9f (0.9f widened = 0.89999997615814209)
    r.currentAmount = 9.0;
    CHECK_FALSE(r.IsDeprived());
    CHECK(r.IsDeprived(0.95f));
}

TEST_CASE("WP-15 ModuleResourceHandler.OnLoad on every stock part: INPUT_RESOURCE / OUTPUT_RESOURCE / RESOURCE lists equal the cfg") {
    if (!GameAvailable()) return;
    Game15 g;
    size_t modulesWithResources = 0, parts = 0;
    for (const auto& ap : g.data.parts.loadedParts) {
        if (!ap->partPrefab || ap->codeBuilt || !ap->prefabCompiled) continue;
        parts++;
        for (const PartModuleData& md : ap->partPrefab->ModuleData) {
            if (!md.module || !md.config) continue;
            // ModuleResourceHandler.cs:30: INPUT/OUTPUT nodes with a name load into their list (the list is cleared on the first node);
            // the first RESOURCE node loads all RESOURCE nodes into the primary list and ends the scan
            std::vector<std::string> in, out;
            bool firstIn = true, firstOut = true;
            const bool primaryIsInput = md.module->resHandler.moduleResourceBasedPrimaryIsInput;
            std::vector<std::string> resNames;
            for (int i = 0; i < md.config->nodes().Count(); i++) {
                ConfigNode* c = md.config->nodes()[i];
                const auto nm = c->GetValue("name");
                if (c->name == "OUTPUT_RESOURCE" && nm && !nm->empty()) {
                    if (firstOut) out.clear(), firstOut = false;
                    out.push_back(*nm);
                } else if (c->name == "INPUT_RESOURCE" && nm && !nm->empty()) {
                    if (firstIn) in.clear(), firstIn = false;
                    in.push_back(*nm);
                } else if (c->name == "RESOURCE") {
                    for (int j = 0; j < md.config->nodes().Count(); j++)
                        if (md.config->nodes()[j]->name == "RESOURCE") resNames.push_back(*md.config->nodes()[j]->GetValue("name"));
                    (primaryIsInput ? in : out) = resNames;
                    break;
                }
            }
            auto names = [](const std::vector<std::shared_ptr<ModuleResource>>& l) {
                std::vector<std::string> r;
                for (const auto& m : l) r.push_back(m->name);
                return r;
            };
            // ModuleDeployableSolarPanel.OnLoad (ModuleDeployableSolarPanel.cs:184, WP-27): without module resources and with a chargeRate value
            // the panel creates its own output resource `resourceName` (default ElectricCharge)
            if (md.moduleName == "ModuleDeployableSolarPanel" && out.empty() && md.config->HasValue("chargeRate")) {
                const auto rn = md.config->GetValue("resourceName");
                out.push_back(rn ? *rn : std::string("ElectricCharge"));
            }
            // ModuleEnviroSensor.OnLoad (ModuleEnviroSensor.cs:80, WP-35): a prefab compile (partInfo / partPrefab not yet linked) without module resources
            // gives the sensor its own input resource `resourceName` (default ElectricCharge)
            if (md.moduleName == "ModuleEnviroSensor" && in.empty()) {
                const auto rn = md.config->GetValue("resourceName");
                in.push_back(rn ? *rn : std::string("ElectricCharge"));
            // ModuleDataTransmitter.OnLoad (ModuleDataTransmitter.cs:152, WP-39): a prefab module (partInfo still null) without module resources adds the
            // `requiredResource` (default ElectricCharge) as its input
            } else if (md.moduleName == "ModuleDataTransmitter" && in.empty()) {
                const auto rr = md.config->GetValue("requiredResource");
                in.push_back(rr ? *rr : std::string("ElectricCharge"));
            }
            CAPTURE(ap->name);
            CAPTURE(md.moduleName);
            CHECK(names(md.module->resHandler.inputResources) == in);
            CHECK(names(md.module->resHandler.outputResources) == out);
            if (!in.empty() || !out.empty()) modulesWithResources++;
        }
    }
    MESSAGE(parts << " parts, " << modulesWithResources << " modules with module resources");
    CHECK(modulesWithResources > 20);
    // the RTG: ModuleGenerator with OUTPUT_RESOURCE ElectricCharge rate 0.75 (RTG.cfg:38); a solar panel's RESOURCE nodes would be
    // its OUTPUT (ModuleDeployableSolarPanel.OnAwake: moduleResourceBasedPrimaryIsInput = false)
    const AvailablePart& rtg = g.Info("rtg");
    PartModule* gen = nullptr;
    for (const PartModuleData& md : rtg.partPrefab->ModuleData)
        if (md.moduleName == "ModuleGenerator") gen = md.module.get();
    REQUIRE(gen != nullptr);
    REQUIRE(gen->resHandler.outputResources.size() == 1);
    CHECK(gen->resHandler.outputResources[0]->rate == 0.75);
    CHECK(gen->resHandler.outputResources[0]->name == "ElectricCharge");
    CHECK(gen->resHandler.inputResources.empty());
    CHECK(gen->resHandler.GetAverageInput() == 0.0);
    const AvailablePart& panel = g.Info("solarPanels1");
    PartModule* sp = nullptr;
    for (const PartModuleData& md : panel.partPrefab->ModuleData)
        if (md.moduleName == "ModuleDeployableSolarPanel") sp = md.module.get();
    REQUIRE(sp != nullptr);
    CHECK_FALSE(sp->resHandler.moduleResourceBasedPrimaryIsInput);
    CHECK(sp->resHandler.inputResources.empty());
}

TEST_CASE("WP-15 ModuleResourceHandler: UpdateModuleResourceInputs / Outputs, HasEnoughResourcesToAutoStart, GetAverageInput") {
    if (!GameAvailable()) return;
    Game15 g;
    // Two batteries (ElectricCharge, STAGE_PRIORITY_FLOW: vessel-wide with priorities) and a host part carrying the module
    auto host = g.New("fuelTank");
    auto bat1 = g.New("batteryBank");
    auto bat2 = g.New("batteryBank");
    host->fields.persistentId = 1;
    bat1->fields.persistentId = 2;
    bat2->fields.persistentId = 3;
    Attach(*host, "bottom", *bat1, "top");
    Attach(*bat1, "bottom", *bat2, "top");
    bat1->fields.inverseStage = 1;
    bat2->fields.inverseStage = 2;
    TestVessel vessel;
    vessel.Adopt({host.get(), bat1.get(), bat2.get()});
    const int ec = Id("ElectricCharge");
    PartResource* e1 = bat1->GetResource("ElectricCharge");
    PartResource* e2 = bat2->GetResource("ElectricCharge");
    REQUIRE(e1 != nullptr);
    REQUIRE(e2 != nullptr);
    e1->amount = 100.0;
    e2->amount = 100.0;
    PartModule* mod = AddModuleTo(*host, "ModuleGenerator");
    ModuleResourceHandler& h = mod->resHandler;
    h.OnLoad(*ParseOne("MODULE\n{\n name = ModuleGenerator\n INPUT_RESOURCE\n {\n  name = ElectricCharge\n  rate = 2\n }\n OUTPUT_RESOURCE\n {\n  name = LiquidFuel\n  rate = 3\n }\n}\n"));
    REQUIRE(h.inputResources.size() == 1);
    REQUIRE(h.outputResources.size() == 1);
    CHECK(h.GetAverageInput() == 2.0);
    Time::fixedDeltaTime = 0.02f;  // TimeWarp.fixedDeltaTime without a TimeWarp = Time.fixedDeltaTime
    const double dt = static_cast<double>(TimeWarp::fixedDeltaTime());

    // Inputs (ModuleResourceHandler.cs:142): currentRequest = rate * rateMultiplier * dt; the request goes to Part.RequestResource(id,
    // request, flowMode) only with useFlowMode, otherwise with the definition's default flow mode. Enough in stock: satisfied -> returns 1,
    // currentAmount = request, available = true. EC is STAGE_PRIORITY_FLOW: bat2 (priority 20) is drained first.
    std::string error;
    double r = h.UpdateModuleResourceInputs(error, 1.0, 0.5, false, false, true);
    ModuleResource& in = *h.inputResources[0];
    CHECK(in.currentRequest == 2.0 * 1.0 * dt);
    CHECK(in.currentAmount == doctest::Approx(in.currentRequest).epsilon(1e-12));
    CHECK(in.available);
    CHECK(r == 1.0);
    CHECK(e2->amount == doctest::Approx(100.0 - in.currentRequest).epsilon(1e-12));
    CHECK(e1->amount == 100.0);
    // the bool overloads: (error, rateMultiplier, threshold, returnOnFirstLack, stringOps) -> result > 0
    CHECK(h.UpdateModuleResourceInputs(error, 1.0, 0.5, true, false));
    // not enough: only 0.01 EC in the whole vessel, request 0.04, threshold 0.5 => 0.01 < 0.02: unavailable. First lack:
    // num = currentAmount / currentRequest (dt is (double)0.02f), error = Localizer.Format("#autoLOC_244419", name)
    e1->amount = 0.0;
    e2->amount = 0.01;
    error.clear();
    r = h.UpdateModuleResourceInputs(error, 1.0, 0.5, false, false, true);
    CHECK_FALSE(in.available);
    CHECK(r == doctest::Approx(0.01 / (2.0 * dt)).epsilon(1e-12));  // (the drawn 0.01 is demand - (demand - 0.01) in double)
    CHECK_FALSE(error.empty());
    CHECK(e2->amount == 0.0);
    // returnOnFirstLack: returns 0 with the "#autoLOC_6001043" message
    e2->amount = 0.01;
    error.clear();
    r = h.UpdateModuleResourceInputs(error, 1.0, 0.5, true, false, true);
    CHECK(r == 0.0);
    CHECK_FALSE(error.empty());
    // average mode: a satisfied input adds 1, the mean is returned
    e1->amount = 100.0;
    e2->amount = 100.0;
    CHECK(h.UpdateModuleResourceInputs(error, 1.0, 0.5, false, true, true) == 1.0);
    // CheatOptions.InfinitePropellant: currentAmount = currentRequest, nothing is drawn
    ResourceFlowEnvironment::InfinitePropellant = true;
    const double before = e1->amount + e2->amount;
    CHECK(h.UpdateModuleResourceInputs(error, 1.0, 0.5, false, false, true) == 1.0);
    CHECK(e1->amount + e2->amount == before);
    CHECK(h.HasEnoughResourcesToAutoStart());
    ResourceFlowEnvironment::InfinitePropellant = false;
    // no input resources: 1.0
    ModuleResourceHandler empty;
    CHECK(empty.UpdateModuleResourceInputs(error, 1.0, 0.5, false, false, true) == 1.0);

    // Outputs (:279): each output with currentRequest > minAbsValue stores |RequestResource(id, -currentRequest)|; returns the largest
    // rate*rateMultiplier. LiquidFuel is STACK_PRIORITY_SEARCH: the host's crossfeed set (host - bat1 - bat2 have no LiquidFuel but the
    // host tank does).
    PartResource* hostLf = host->GetResource("LiquidFuel");
    REQUIRE(hostLf != nullptr);
    hostLf->amount = 0.0;
    hostLf->maxAmount = 180.0;
    host->ResetSimulationResources();
    CHECK(h.UpdateModuleResourceOutputs(2.0, 0.0) == 6.0);  // rate 3 * 2.0
    ModuleResource& outRes = *h.outputResources[0];
    CHECK(outRes.currentRequest == 6.0 * dt);
    CHECK(outRes.currentAmount == doctest::Approx(6.0 * dt).epsilon(1e-12));
    CHECK(hostLf->amount == doctest::Approx(6.0 * dt).epsilon(1e-12));
    CHECK(h.UpdateModuleResourceOutputs(1.0, 1000.0) == 0.0);  // request below minAbsValue: skipped

    // HasEnoughResourcesToAutoStart (:305) needs startUpHandler: a plain input never auto-starts
    CHECK_FALSE(h.HasEnoughResourcesToAutoStart());
    in.startUpHandler = true;
    in.startUpAmount = 150.0;
    CHECK(h.HasEnoughResourcesToAutoStart());  // vessel EC 200 >= 150
    in.startUpAmount = 250.0;
    CHECK_FALSE(h.HasEnoughResourcesToAutoStart());
    in.shutOffStartUpUsePercent = true;
    in.startUpPercent = 40.0f;  // 200 of 2 * max(bat) ... percent of maxAmount over the vessel
    double a, m;
    vessel.GetConnectedResourceTotals(ec, false, a, m, true);
    CHECK((40.0f <= static_cast<float>(a / m * 100.0)) == h.HasEnoughResourcesToAutoStart());
    // IsResourceBelowShutOffLimit (:348)
    PartResource* anyEC = bat1->GetResource("ElectricCharge");
    // without shutOffHandler the loop breaks (returns true) when currentAmount >= 0.01 (sic: the name says "below"); with it, true needs
    // the same-named resource and currentAmount > shutOffAmount
    in.currentAmount = 0.005;
    CHECK_FALSE(h.IsResourceBelowShutOffLimit(*anyEC));
    in.currentAmount = 1.0;
    CHECK(h.IsResourceBelowShutOffLimit(*anyEC));
    in.shutOffHandler = true;
    in.shutOffAmount = 5.0f;
    CHECK_FALSE(h.IsResourceBelowShutOffLimit(*anyEC));  // currentAmount 1 is not > 5
    in.currentAmount = 10.0;
    CHECK(h.IsResourceBelowShutOffLimit(*anyEC));
}

// ================================================== broker, converter ======================================================

TEST_CASE("WP-15 ResourceSetup, ResourceBroker and ResourceConverter") {
    if (!GameAvailable()) return;
    Game15 g;
    // ResourceDefaults.cfg: RESOURCE_CONFIGURATION { HeatEnabled = True, ECMinScale = 100, OverlayStyle = 1 }
    ResourceSetup::ResourceConfig* cfg = ResourceSetup::Instance().ResConfig();
    REQUIRE(cfg != nullptr);
    CHECK(cfg->HeatEnabled);
    CHECK(cfg->ECMinScale == 100);
    CHECK(cfg->OverlayStyle == 1);
    CHECK_FALSE(cfg->ShowDebugOptions);

    Stack3 s(g, 100, 200, 300);  // LiquidFuel 100 / 200 / 300 of 400, priorities 0 / 10 / 20; Oxidizer 220 of 220 each
    ResourceBroker broker;
    const double eps = 1e-12;
    // ResourceBroker.cs:8 AmountAvailable: Part.GetConnectedResourceTotals(id, flowMode) -- NULL means the definition's mode; ElectricCharge
    // above ECMinScale * 0.02 s (= 2 s) is scaled by deltaTime / 0.02
    CHECK(broker.AmountAvailable(s.A.get(), "LiquidFuel", 1.0, ResourceFlowMode::NULL_) == 600.0);
    CHECK(broker.AmountAvailable(s.A.get(), "LiquidFuel", 1.0, ResourceFlowMode::NO_FLOW) == 100.0);  // A's own
    CHECK(broker.AmountAvailable(s.A.get(), "LiquidFuel", 50.0, ResourceFlowMode::ALL_VESSEL) == 600.0);  // only EC is scaled
    PartResource* ecA = s.A->GetResource("ElectricCharge");
    PartResource* ecB = s.B->GetResource("ElectricCharge");
    CHECK(ecA == nullptr);  // tanks carry no charge: add some batteries' worth through the list
    s.A->Resources.Add("ElectricCharge", 30.0, 100.0, true, true, false, true, PartResourceFlowMode::Both);
    s.B->Resources.Add("ElectricCharge", 20.0, 100.0, true, true, false, true, PartResourceFlowMode::Both);
    s.vessel.UpdateResourceSets();  // (the list change already did, through the event)
    CHECK(broker.AmountAvailable(s.A.get(), "ElectricCharge", 1.0, ResourceFlowMode::NULL_) == 50.0);  // 1 s <= 2 s: unscaled
    CHECK(broker.AmountAvailable(s.A.get(), "ElectricCharge", 10.0, ResourceFlowMode::NULL_) == 50.0 * (10.0 / 0.02));
    (void)ecB;
    // RequestResource: the request is divided by deltaTime / 0.02 for ElectricCharge above the threshold; the return value is the amount
    // drawn (the divided one). 100 / 500 = 0.2 from the highest-priority battery (B, priority 10)
    CHECK(broker.RequestResource(s.A.get(), "ElectricCharge", 100.0, 10.0, ResourceFlowMode::NULL_) == doctest::Approx(100.0 / (10.0 / 0.02)).epsilon(eps));
    CHECK(s.B->GetResource("ElectricCharge")->amount == doctest::Approx(20.0 - 100.0 / (10.0 / 0.02)).epsilon(eps));
    // StorageAvailable: Part.GetConnectedResourceTotals(.., FillAmount, pulling: false) -- sum of max*fill - amount, per resource, > 1e-9 only
    // (A,B,C hold 100, 200, 300 of 400): fill 1.0 -> 300 + 200 + 100 = 600; fill 0.5 -> 100 (A) + 0 + 0
    CHECK(broker.StorageAvailable(s.A.get(), "LiquidFuel", 1.0, ResourceFlowMode::NULL_, 1.0) == 600.0);
    CHECK(broker.StorageAvailable(s.A.get(), "LiquidFuel", 1.0, ResourceFlowMode::NULL_, 0.5) == 100.0);
    // StoreResource pushes: Part.RequestResource(id, -amount, mode)
    CHECK(broker.StoreResource(s.A.get(), "LiquidFuel", 50.0, 1.0, ResourceFlowMode::NULL_) == doctest::Approx(-50.0).epsilon(eps));
    CHECK(s.Amount(*s.C) == doctest::Approx(350.0).epsilon(eps));
    // the int overloads and an unknown resource with NULL flow mode (NullReferenceException on the missing definition)
    CHECK(broker.AmountAvailable(s.A.get(), Id("LiquidFuel"), 1.0, ResourceFlowMode::STACK_PRIORITY_SEARCH) == 650.0);
    CHECK_THROWS_AS(broker.AmountAvailable(s.A.get(), Id("Unobtainium"), 1.0, ResourceFlowMode::NULL_), NullReferenceException);

    // ResourceConverter.ProcessRecipe (ResourceConverter.cs:23). Oxidizer (2 per unit time) -> LiquidFuel (1 per unit time) over 10 s.
    // Requirements none; num = 1 * efficiencyBonus; inputs: AmountAvailable(Ox) * TakeAmount = 660 >= needed 2 * 10 * 1 = 20 -> num4 stays
    // 10; outputs: StorageAvailable(LF, FillAmount 1) = 650 - ... enough for 1 * 10 -> RequestResource(Ox, 20) then StoreResource(LF, 10);
    // TimeFactor = num4 * num = 10. The event OnResourceConverterOutput fires once per output.
    s.Set(0, 0, 0);
    ConversionRecipe recipe;
    recipe.Inputs().push_back(ResourceRatio("Oxidizer", 2.0, false));
    recipe.Outputs().push_back(ResourceRatio("LiquidFuel", 1.0, false));
    ResourceConverter conv;
    struct Out {
        std::vector<std::pair<std::string, double>> seen;
        void On(PartModule*, std::string n, double a) { seen.emplace_back(n, a); }
    } out;
    GameEvents::OnResourceConverterOutput.Add(&out, &Out::On);
    ConverterResults res = conv.ProcessRecipe(10.0, recipe, s.A.get(), nullptr, 1.0f);
    CHECK(res.TimeFactor == 10.0);
    CHECK(res.Status.empty());
    REQUIRE(out.seen.size() == 1);
    CHECK(out.seen[0].first == "LiquidFuel");
    CHECK(out.seen[0].second == 10.0);
    CHECK(s.C->GetResource("Oxidizer")->amount == doctest::Approx(200.0).epsilon(eps));  // highest priority first
    CHECK(s.Amount(*s.C) == doctest::Approx(10.0).epsilon(eps));

    // a requirement below 1: num = min(amount / |ratio|) = 220 / 1000 = 0.22 for the Oxidizer of A -> TimeFactor = 10 * 0.22
    recipe.Requirements().push_back(ResourceRatio("Oxidizer", 1000.0, false));
    s.Set(0, 0, 0);
    for (Part* p : {s.A.get(), s.B.get(), s.C.get()}) p->GetResource("Oxidizer")->amount = 220.0;
    out.seen.clear();
    res = conv.ProcessRecipe(10.0, recipe, s.A.get(), nullptr, 1.0f);
    CHECK(res.TimeFactor == doctest::Approx(10.0 * (220.0 / 1000.0)).epsilon(eps));
    // a negative requirement ratio means "at most": num3 = 1 - amount / |ratio| = 1 - 0.22 = 0.78
    recipe.Requirements()[0].Ratio = -1000.0;
    for (Part* p : {s.A.get(), s.B.get(), s.C.get()}) p->GetResource("Oxidizer")->amount = 220.0;
    s.Set(0, 0, 0);
    res = conv.ProcessRecipe(10.0, recipe, s.A.get(), nullptr, 1.0f);
    CHECK(res.TimeFactor == doctest::Approx(10.0 * (1.0 - 220.0 / 1000.0)).epsilon(eps));
    recipe.Requirements().clear();
    // efficiency 0: nothing happens
    CHECK(conv.ProcessRecipe(10.0, recipe, s.A.get(), nullptr, 0.0f).TimeFactor == 0.0);
    // not enough input: Ox ratio 1000 -> needs 10000 for 10 s, 660 available: num4 = 10 * 660 / 10000 = 0.66 and everything is consumed in
    // proportion (inputs 1000 * 0.66 = 660)
    for (Part* p : {s.A.get(), s.B.get(), s.C.get()}) p->GetResource("Oxidizer")->amount = 220.0;
    s.Set(0, 0, 0);
    recipe.Inputs()[0].Ratio = 1000.0;
    res = conv.ProcessRecipe(10.0, recipe, s.A.get(), nullptr, 1.0f);
    CHECK(res.TimeFactor == doctest::Approx(0.66).epsilon(eps));
    CHECK(s.C->GetResource("Oxidizer")->amount + s.B->GetResource("Oxidizer")->amount + s.A->GetResource("Oxidizer")->amount == doctest::Approx(0.0).epsilon(1e-9).scale(1.0));
    // output full: StorageAvailable 0 < needed => num4 *= 0 -> the recipe stops (TimeFactor 0), unless the output is dumped
    recipe.Inputs()[0].Ratio = 2.0;
    for (Part* p : {s.A.get(), s.B.get(), s.C.get()}) p->GetResource("Oxidizer")->amount = 220.0;
    s.Set(400, 400, 400);
    CHECK(conv.ProcessRecipe(10.0, recipe, s.A.get(), nullptr, 1.0f).TimeFactor == 0.0);
    recipe.Outputs()[0].DumpExcess = true;
    CHECK(conv.ProcessRecipe(10.0, recipe, s.A.get(), nullptr, 1.0f).TimeFactor == 10.0);
    GameEvents::OnResourceConverterOutput.Remove(&out, &Out::On);
    // InfinitePropellant skips the input check
    ResourceFlowEnvironment::InfinitePropellant = true;
    for (Part* p : {s.A.get(), s.B.get(), s.C.get()}) p->GetResource("Oxidizer")->amount = 0.0;
    CHECK(conv.ProcessRecipe(10.0, recipe, s.A.get(), nullptr, 1.0f).TimeFactor == 10.0);
    ResourceFlowEnvironment::InfinitePropellant = false;
}

// ================================================== crossfeed toggle, fuel line, EVA container, auto shift =================

namespace {
struct CrossfeedProbe {
    std::vector<Part*> changed;
    void On(Part* p) { changed.push_back(p); }
    CrossfeedProbe() { GameEvents::onPartCrossfeedStateChange.Add(this, &CrossfeedProbe::On); }
    ~CrossfeedProbe() { GameEvents::onPartCrossfeedStateChange.Remove(this, &CrossfeedProbe::On); }
};
ModuleToggleCrossfeed* ToggleOf(Part& p) {
    return dynamic_cast<ModuleToggleCrossfeed*>(
        p.Modules().GetModule([](PartModule& m) { return dynamic_cast<ModuleToggleCrossfeed*>(&m) != nullptr; }));
}
}  // namespace

TEST_CASE("WP-15 ModuleToggleCrossfeed: toggle event / actions, symmetry propagation, OnStart, Instantiate") {
    if (!GameAvailable()) return;
    Game15 g;
    // the first stock decoupler with the module (cfg: crossfeedStatus = False)
    std::string name;
    for (const auto& ap : g.data.parts.loadedParts)
        if (ap->partPrefab && !ap->codeBuilt && ap->prefabCompiled && ToggleOf(*ap->partPrefab) != nullptr && name.empty()) name = ap->name;
    REQUIRE(!name.empty());
    CAPTURE(name);
    auto d1 = g.New(name.c_str());
    auto d2 = g.New(name.c_str());
    ModuleToggleCrossfeed* m1 = ToggleOf(*d1);
    ModuleToggleCrossfeed* m2 = ToggleOf(*d2);
    REQUIRE(m1 != nullptr);
    REQUIRE(m2 != nullptr);
    d1->symmetryCounterparts = {d1.get(), d2.get()};
    d2->symmetryCounterparts = {d1.get(), d2.get()};
    CHECK_FALSE(m1->Get<bool>("crossfeedStatus"));
    CHECK_FALSE(d1->fields.fuelCrossFeed);  // OnLoad -> UpdateCrossfeed wrote part.fuelCrossFeed = crossfeedStatus
    CHECK(m1->CrossfeedToggleableEditor());  // Decoupler_0.cfg: toggleEditor = true, toggleFlight = true
    CHECK(m1->CrossfeedToggleableFlight());
    CHECK_FALSE(m1->CrossfeedRequiresTech());
    CHECK(m1->CrossfeedHasTech());
    CHECK(m1->GetModuleDisplayName() == Localizer::Format("#autoLOC_236032"));
    // the labels follow the state: false -> enableText, true -> disableText (UpdateCrossfeed :155; Start() calls it, :107)
    m1->Start();
    m2->Start();
    CHECK(m1->Events()["ToggleEvent"]->guiName() == Localizer::Format(m1->Get<std::string>("enableText")));

    // ToggleEvent in the editor scene propagates to the symmetry counterparts first (UpdateCrossfeed(false)), then updates itself; the
    // state-change event fires for every part whose fuelCrossFeed changed (the `fireEvent` argument is not read by the C#)
    {
        SceneScope editor(GameScenes::EDITOR);
        CrossfeedProbe probe;
        m1->ToggleEvent();
        CHECK(m1->Get<bool>("crossfeedStatus"));
        CHECK(m2->Get<bool>("crossfeedStatus"));
        CHECK(d1->fields.fuelCrossFeed);
        CHECK(d2->fields.fuelCrossFeed);
        CHECK(probe.changed == std::vector<Part*>{d2.get(), d1.get()});
        CHECK(m1->Events()["ToggleEvent"]->guiName() == Localizer::Format(m1->Get<std::string>("disableText")));
    }
    // flight scene: eventPropagatesInFlight is false: only the part itself
    {
        SceneScope flight(GameScenes::FLIGHT);
        m1->ToggleEvent();
        CHECK_FALSE(m1->Get<bool>("crossfeedStatus"));
        CHECK(m2->Get<bool>("crossfeedStatus"));
    }
    // actions (bound in OnAwake): EnableAction / DisableAction set the state; ToggleAction toggles without propagation and restores the flags
    KSPActionParam param;
    {
        SceneScope editor(GameScenes::EDITOR);
        m1->Actions()["EnableAction"]->Invoke(param);
        CHECK(m1->Get<bool>("crossfeedStatus"));
        CHECK(d1->fields.fuelCrossFeed);
        m1->Actions()["DisableAction"]->Invoke(param);
        CHECK_FALSE(m1->Get<bool>("crossfeedStatus"));
        m2->Set<bool>("crossfeedStatus", false);
        m2->UpdateCrossfeed(false);
        m1->Actions()["ToggleAction"]->Invoke(param);
        CHECK(m1->Get<bool>("crossfeedStatus"));
        CHECK_FALSE(m2->Get<bool>("crossfeedStatus"));  // eventPropagatesInEditor was switched off during the action
        CHECK(m1->Get<bool>("eventPropagatesInEditor"));  // and restored
    }
    m1->ToggleAction(1);  // ToggleAction(KSPActionType.Deactivate)
    CHECK_FALSE(m1->Get<bool>("crossfeedStatus"));
    m1->ToggleAction(0);  // Activate
    CHECK(m1->Get<bool>("crossfeedStatus"));

    // OnStart (:134): the toggle is offered in the editor (toggleEditor) and in flight (toggleFlight) as the fields say; a disabled
    // module falls back to the prefab's default state
    m1->Set<bool>("toggleFlight", false);
    m1->Set<bool>("crossfeedStatus", true);
    m1->OnStart(StartState::Editor);
    CHECK(m1->Events()["ToggleEvent"]->guiActiveEditor);
    CHECK_FALSE(m1->Events()["ToggleEvent"]->guiActive);
    CHECK_FALSE(m1->Actions()["ToggleAction"]->active);
    m1->Set<bool>("toggleFlight", true);
    m1->OnStart(StartState::Flying);
    CHECK(m1->Events()["ToggleEvent"]->guiActive);
    CHECK(m1->Actions()["EnableAction"]->active);
    CHECK(m1->Actions()["DisableAction"]->active);
    CHECK(m1->Get<bool>("crossfeedStatus"));
    m1->Set<bool>("moduleIsEnabled", false);
    m1->OnStart(StartState::Flying);
    CHECK(m1->Get<bool>("crossfeedStatus") == m1->defaultCrossfeedStatus);
    CHECK_FALSE(m1->Events()["ToggleEvent"]->guiActive);
    // a required technology that is not researched disables it (career): techRequired set, CrossfeedHasTech false
    m1->Set<bool>("moduleIsEnabled", true);
    m1->Set<std::string>("techRequired", "advFuelSystems");
    CHECK(m1->CrossfeedRequiresTech());
    CHECK(m1->CrossfeedTech() == "advFuelSystems");
    CHECK(m1->CrossfeedHasTech());  // not a career game: true

    // Instantiate keeps defaultCrossfeedStatus (a public serialized field of the prefab's module)
    ModuleToggleCrossfeed* prefabModule = ToggleOf(*g.Info(name.c_str()).partPrefab);
    REQUIRE(prefabModule != nullptr);
    CHECK(m1->defaultCrossfeedStatus == prefabModule->defaultCrossfeedStatus);
    // the 8 stock parts of WP-05 note 9 / the crafts: fuelCrossFeed == crossfeedStatus for every prefab with the module
    size_t withModule = 0;
    for (const auto& ap : g.data.parts.loadedParts) {
        if (!ap->partPrefab || ap->codeBuilt || !ap->prefabCompiled) continue;
        ModuleToggleCrossfeed* t = ToggleOf(*ap->partPrefab);
        if (t == nullptr) continue;
        withModule++;
        CHECK(ap->partPrefab->fields.fuelCrossFeed == t->Get<bool>("crossfeedStatus"));
        // OnAwake runs while the prefab compiles (partInfo has no prefab yet): defaultCrossfeedStatus = the cfg's part.fuelCrossFeed,
        // before the module's OnLoad overwrote it with crossfeedStatus
        if (const auto cfgValue = ap->partConfig ? ap->partConfig->GetValue("fuelCrossFeed") : std::nullopt)
            CHECK(t->defaultCrossfeedStatus == (*cfgValue == "True" || *cfgValue == "true"));
    }
    CHECK(withModule >= 8);
}

TEST_CASE("WP-15 CModuleFuelLine: target set / lost, lookup events, GetInfo") {
    if (!GameAvailable()) return;
    Game15 g;
    auto line = g.New("fuelLine");
    auto tank = g.New("fuelTank");
    auto* fl = dynamic_cast<CModuleFuelLine*>(line->Modules().GetModule([](PartModule& m) { return dynamic_cast<CModuleFuelLine*>(&m) != nullptr; }));
    REQUIRE(fl != nullptr);
    CHECK(fl->compoundPart == line.get());
    CHECK(fl->GetModuleTitle() == "Fuel Line");
    CHECK(fl->GetPrimaryField().empty());
    struct Probe {
        std::vector<std::tuple<bool, Part*, Part*>> seen;
        void On(GameEvents::HostedFromToAction<bool, Part*> d) { seen.emplace_back(d.host, d.from, d.to); }
    } probe;
    GameEvents::onPartFuelLookupStateChange.Add(&probe, &Probe::On);
    fl->OnTargetSet(tank.get());  // CModuleFuelLine.cs:12: target.fuelLookupTargets.Add(part), event (false, target, part)
    CHECK(tank->fuelLookupTargets == std::vector<Part*>{line.get()});
    fl->compoundTarget = [&](const Part&) { return tank.get(); };
    fl->OnTargetLost();  // :19 -> CloseFuelLine :24: Remove(part), same event
    CHECK(tank->fuelLookupTargets.empty());
    REQUIRE(probe.seen.size() == 2);
    CHECK(std::get<0>(probe.seen[0]) == false);
    CHECK(std::get<1>(probe.seen[0]) == tank.get());
    CHECK(std::get<2>(probe.seen[1]) == line.get());
    GameEvents::onPartFuelLookupStateChange.Remove(&probe, &Probe::On);
    // CloseFuelLine without a target is a no-op
    CModuleFuelLine::CacheLocalStrings();
    const float maxLength = line->compoundFields.maxLength;
    const std::string info = fl->GetInfo();
    CHECK(info.find(dotnet::FormatSingleF(maxLength, 1)) != std::string::npos);
    CHECK(info == Localizer::Format("#autoLOC_216749", {dotnet::FormatSingleF(maxLength, 1)}) + Localizer::Format("#autoLOC_216750"));
    CHECK(fl->GetModuleDisplayName() == Localizer::Format("#autoLOC_216757"));
}

TEST_CASE("WP-15 EVAResourceContainer: UI state, transfer steps") {
    if (!GameAvailable()) return;
    Game15 g;
    auto crate = g.New("fuelTank");
    auto root = g.New("fuelTank");
    PartModule* mod = AddModuleTo(*crate, "EVAResourceContainer");
    auto* eva = dynamic_cast<EVAResourceContainer*>(mod);
    REQUIRE(eva != nullptr);
    CHECK(eva->Get<std::string>("resourceName") == "Supplies");  // field default
    eva->Set<std::string>("resourceName", "LiquidFuel");
    eva->OnStart(StartState::Landed);
    REQUIRE(eva->resource != nullptr);
    CHECK(eva->resource->resourceName == "LiquidFuel");
    // updateModuleUI (:49): Take active when there is something to take (180 > 0), Store active only when not full; names carry "(amount)"
    BaseEvent* take = eva->Events()["GetResourceExternalEvent"];
    BaseEvent* put = eva->Events()["PutResourceExternalEvent"];
    CHECK(take->active);
    CHECK_FALSE(put->active);  // 180 / 180: full
    CHECK(take->guiName() == "Take Resources (180)");
    CHECK(put->guiName() == "Store Resources (180)");
    CHECK(take->unfocusedRange == 2.0f);
    CHECK_FALSE(eva->Events()["StopTransfer"]->active);
    // GetResourceExternalEvent(activeVessel.rootPart): the root part needs the resource; transfer src = container, dest = root
    PartResource* rootLf = root->GetResource("LiquidFuel");
    rootLf->amount = 50.0;
    eva->GetResourceExternalEvent(root.get());
    CHECK(eva->transferring);
    CHECK(eva->transferSrc == eva->resource);
    CHECK(eva->transferDest == rootLf);
    CHECK_FALSE(take->active);
    CHECK(eva->Events()["StopTransfer"]->active);
    // one coroutine iteration: rate = min(rate, src.amount, room); src loses it, dest gains it
    double rate = 2.0;
    CHECK(EVAResourceContainer::TransferStep(*eva->resource, *rootLf, rate, 2.0f, 1.0));
    CHECK(eva->resource->amount == 178.0);
    CHECK(rootLf->amount == 52.0);
    // out of range: (distance^2 >= maxDist^2) ends the loop without transferring
    CHECK_FALSE(EVAResourceContainer::TransferStep(*eva->resource, *rootLf, rate, 2.0f, 4.0));
    // destination nearly full: the rate is clamped to the room
    rootLf->amount = 179.5;
    rootLf->maxAmount = 180.0;
    rate = 2.0;
    CHECK(EVAResourceContainer::TransferStep(*eva->resource, *rootLf, rate, 2.0f, 0.0));
    CHECK(rate == 0.5);
    CHECK(rootLf->amount == 180.0);
    CHECK_FALSE(EVAResourceContainer::TransferStep(*eva->resource, *rootLf, rate, 2.0f, 0.0));  // dest full
    eva->StopTransfer();
    CHECK_FALSE(eva->transferring);
    // Put: src = the root part's resource
    eva->PutResourceExternalEvent(root.get());
    CHECK(eva->transferSrc == rootLf);
    CHECK(eva->transferDest == eva->resource);
    // a root part without the resource: nothing starts (the screen message is UI)
    auto bare = g.New("fuelTank");
    bare->RemoveResource("LiquidFuel");
    eva->StopTransfer();
    eva->GetResourceExternalEvent(bare.get());
    CHECK_FALSE(eva->transferring);
    // OnStart on a part without the resource: updateModuleUI dereferences the missing resource (NullReferenceException)
    auto empty = g.New("fuelTank");
    empty->RemoveResource("LiquidFuel");
    auto* eva2 = dynamic_cast<EVAResourceContainer*>(AddModuleTo(*empty, "EVAResourceContainer"));
    eva2->Set<std::string>("resourceName", "LiquidFuel");
    CHECK_THROWS_AS(eva2->OnStart(StartState::Landed), NullReferenceException);
}

TEST_CASE("WP-15 ModuleResourceAutoShiftState: OnStart wiring, Modify* clamps, shut-off branch of UpdateModuleResourceInputs") {
    if (!GameAvailable()) return;
    Game15 g;
    auto host = g.New("fuelTank");
    host->fields.persistentId = 1;
    support::TestVessel vessel;
    vessel.Adopt({host.get()});
    PartResource* lf = host->GetResource("LiquidFuel");
    lf->maxAmount = 180.0;
    lf->amount = 90.0;
    host->ResetSimulationResources();
    PartModule* gen = AddModuleTo(*host, "ModuleGenerator");
    gen->resHandler.OnLoad(*ParseOne("MODULE\n{\n name = ModuleGenerator\n INPUT_RESOURCE\n {\n  name = LiquidFuel\n  rate = 1\n }\n}\n"));
    REQUIRE(gen->resHandler.inputResources.size() == 1);
    auto* shift = dynamic_cast<ModuleResourceAutoShiftState*>(AddModuleTo(*host, "ModuleResourceAutoShiftState"));
    REQUIRE(shift != nullptr);
    // an empty affectedResourceName destroys the module in OnStart
    {
        auto* idle = dynamic_cast<ModuleResourceAutoShiftState*>(AddModuleTo(*host, "ModuleResourceAutoShiftState"));
        idle->OnStart(StartState::Flying);
        CHECK(idle->destroyed);
    }
    shift->Set<std::string>("affectedResourceName", "LiquidFuel");
    shift->Set<std::string>("affectedModuleName", "ModuleGenerator");
    shift->Set<bool>("resourceShutOffHandler", true);
    shift->Set<bool>("resourceStartUpHandler", true);
    shift->Set<float>("resourceShutOffAmount", 100.0f);
    shift->Set<float>("resourceStartUpAmount", 120.0f);
    shift->Set<float>("resourceShutOffPercent", 10.0f);
    shift->Set<float>("resourceStartUpPercent", 30.0f);
    {
        SceneScope flight(GameScenes::FLIGHT);
        shift->OnStart(StartState::Flying);
    }
    CHECK_FALSE(shift->destroyed);
    REQUIRE(shift->partResource != nullptr);
    CHECK(shift->partResource == lf);
    ModuleResource* res = gen->resHandler.inputResources[0].get();
    CHECK(shift->resource == res);
    CHECK(res->autoStateShifter == shift);
    CHECK(res->shutOffHandler);
    CHECK(res->startUpHandler);
    CHECK(res->shutOffAmount == 100.0f);
    CHECK(res->startUpAmount == 120.0f);
    CHECK(res->shutOffPercent == 10.0f);
    CHECK(res->startUpPercent == 30.0f);
    CHECK(shift->Get<std::string>("moduleAutoShiftName") == Localizer::Format("#autoLOC_6005054", {lf->info->displayName()}));

    // Modify* handlers (:122-:162): keep shutOff < startUp (by 1) and push the value to the ModuleResource
    shift->Set<float>("resourceShutOffAmount", 130.0f);
    shift->ModifyShutOffAmount();  // 130 >= 120 -> 119
    CHECK(shift->Get<float>("resourceShutOffAmount") == 119.0f);
    CHECK(res->shutOffAmount == 119.0f);
    shift->Set<float>("resourceStartUpAmount", 100.0f);
    shift->ModifyStartUpAmount();  // 100 <= 119 -> 120
    CHECK(shift->Get<float>("resourceStartUpAmount") == 120.0f);
    CHECK(res->startUpAmount == 120.0f);
    shift->Set<float>("resourceShutOffPercent", 50.0f);
    shift->ModifyShutOffPercent();  // 50 >= 30 -> 29
    CHECK(shift->Get<float>("resourceShutOffPercent") == 29.0f);
    shift->Set<float>("resourceStartUpPercent", 5.0f);
    shift->ModifyStartUpPercent();  // 5 <= 29 -> 30
    CHECK(shift->Get<float>("resourceStartUpPercent") == 30.0f);
    CHECK(res->shutOffPercent == 29.0f);
    CHECK(res->startUpPercent == 30.0f);
    shift->Set<bool>("resourceShutOffStartUpUsePercent", true);
    shift->ModifyAmountPercent();
    CHECK(res->shutOffStartUpUsePercent);
    // the OnValueModified hooks of the fields call the same handlers
    shift->Set<float>("resourceShutOffAmount", 125.0f);
    for (auto& cb : shift->Fields()["resourceShutOffAmount"]->OnValueModified) cb(ModuleValue{});
    CHECK(shift->Get<float>("resourceShutOffAmount") == 119.0f);

    // UpdateModuleResourceInputs with shutOffHandler (percent mode off): vessel LiquidFuel 90 <= shutOffAmount 119 => threshold 1.1,
    // currentResourceLowerThanLayoff, the warning is shown, onPartResourceNonemptyEmpty fires with the shifter's resource; the request
    // (1 * dt) is below 1.1 * request, so the input is unavailable and (not averaging) the result is 0.
    res->shutOffStartUpUsePercent = false;
    EventLog events;
    std::string error;
    Time::fixedDeltaTime = 0.02f;
    const double r = gen->resHandler.UpdateModuleResourceInputs(error, 1.0, 0.5, false, false, true);
    CHECK(gen->resHandler.currentResourceLowerThanLayoff);
    CHECK(shift->warningVisible);
    CHECK_FALSE(res->available);
    CHECK(r == 0.0);
    CHECK(events.events == std::vector<std::string>{"NonemptyEmpty"});
    // above the limit: threshold = max(threshold, 0.001), warning hidden, input available
    lf->amount = 150.0;
    host->ResetSimulationResources();
    const double r2 = gen->resHandler.UpdateModuleResourceInputs(error, 1.0, 0.5, false, false, true);
    CHECK_FALSE(gen->resHandler.currentResourceLowerThanLayoff);
    CHECK_FALSE(shift->warningVisible);
    CHECK(res->available);
    CHECK(r2 == 1.0);
    // percent mode: 150 / 180 * 100 = 83.3 > shutOffPercent 29: unaffected; with shutOffPercent 90 the percentage (83.3) is <= 90: lower
    res->shutOffStartUpUsePercent = true;
    res->shutOffPercent = 90.0f;
    gen->resHandler.UpdateModuleResourceInputs(error, 1.0, 0.5, false, false, true);
    CHECK(gen->resHandler.currentResourceLowerThanLayoff);
}

// ================================================== planetary resources ====================================================

namespace {

// The in-game start-up the resource map needs: PhysicsGlobals, an Awake Planetarium, the spawned Kerbol system (FlightGlobals.Bodies)
struct Kerbol15 {
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    Kerbol15() {
        pg.physicsDatabaseFilename = std::string(KSPCORE_HARNESS_DIR) + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        ResourceEnvironment::bodies = [this]() -> const std::vector<CelestialBody*>& { return sys->Bodies(); };
    }
    ~Kerbol15() {
        ResourceEnvironment::bodies = nullptr;
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
    }
};

ConfigNode::NodePtr LoadSaveScenario(const char* name) {
    auto root = ConfigNode::Load(std::string(KSPCORE_TESTDATA_DIR) + "/saves/physprofile/persistent.sfs");
    REQUIRE(root);
    ConfigNode* game = root->GetNode("GAME");
    REQUIRE(game != nullptr);
    for (ConfigNode* sc : game->GetNodes("SCENARIO")) {
        if (sc->GetValue("name") == std::string(name)) {
            auto copy = ConfigNode::Create();
            sc->CopyTo(*copy);
            return copy;
        }
    }
    FAIL("scenario " << name);
    return nullptr;
}

}  // namespace

TEST_CASE("WP-15 ResourceGameSettings: the physprofile save, defaults, depletion / biome / planet records, Save round trip") {
    // testdata/saves/physprofile/persistent.sfs ResourceScenario: RESOURCE_SETTINGS { GameSeed = 1985366484; ROCMissionSeed = same; MaxDeltaTime = 21600 }
    auto scenario = LoadSaveScenario("ResourceScenario");
    ResourceGameSettings s;
    s.Load(*scenario);
    CHECK(s.Seed == 1985366484);
    CHECK(s.ROCMissionSeed == 1985366484);
    CHECK(s.MaxDeltaTime == 21600);
    CHECK(s.GetDepletionInfo().empty());
    CHECK(s.GetBiomeLockInfo().empty());
    CHECK(s.GetPlanetScanInfo().empty());
    // Save writes the three values into the (existing) RESOURCE_SETTINGS node: the node reads back as it was
    ConfigNode* settings = scenario->GetNode("RESOURCE_SETTINGS");
    REQUIRE(settings != nullptr);
    const std::string original = settings->ToString();
    auto fresh0 = ConfigNode::Create();
    s.Save(*fresh0);
    CHECK(fresh0->GetNode("RESOURCE_SETTINGS")->ToString() == original);
    CHECK(s.SettingsNode == fresh0->GetNode("RESOURCE_SETTINGS"));
    // (Save into a node that already has RESOURCE_SETTINGS appends the three values again: ResourceGameSettings.cs:109 uses AddValue)
    s.Save(*scenario);
    CHECK(scenario->GetNode("RESOURCE_SETTINGS")->values().Count() == 6);

    // GetValue (:165 / :174): int.TryParse / float.TryParse, the current value when absent or unparseable
    ConfigNode probe("X");
    probe.AddValue("a", "12");
    probe.AddValue("b", "twelve");
    probe.AddValue("f", "2.5");
    CHECK(ResourceGameSettings::GetValue(probe, "a", 7) == 12);
    CHECK(ResourceGameSettings::GetValue(probe, "b", 7) == 7);
    CHECK(ResourceGameSettings::GetValue(probe, "zz", 7) == 7);
    CHECK(ResourceGameSettings::GetValue(probe, "f", 1.0f) == 2.5f);
    CHECK(ResourceUtilities::GetValue(probe, "f", 1.0) == 2.5);

    // the three record lists: DEPLETION_DATA (+ DEPLETION_NODE), BIOME_LOCK_DATA, PLANET_SCAN_DATA load and save in this order
    auto full = ConfigNode::Parse(
        "RESOURCE_SETTINGS\n{\n GameSeed = 42\n ROCMissionSeed = 43\n MaxDeltaTime = 1000\n DEPLETION_DATA\n {\n  PlanetId = 1\n  ResourceName = Ore\n"
        "  DEPLETION_NODE\n  {\n   X = 3\n   Y = 4\n   Value = 0.5\n   LastUpdate = 1234.5\n  }\n }\n BIOME_LOCK_DATA\n {\n  PlanetId = 1\n  BiomeName = Shores\n }\n"
        " PLANET_SCAN_DATA\n {\n  PlanetId = 2\n }\n}\n");
    auto holder = ConfigNode::Create();
    holder->AddNode(full->nodes().Shared(0));
    ResourceGameSettings t;
    t.Load(*holder);
    CHECK(t.Seed == 42);
    CHECK(t.MaxDeltaTime == 1000);
    REQUIRE(t.GetDepletionInfo().size() == 1);
    CHECK(t.GetDepletionInfo()[0]->ResourceName == "Ore");
    REQUIRE(t.GetDepletionInfo()[0]->DepletionNodes.size() == 1);
    CHECK(t.GetDepletionInfo()[0]->DepletionNodes[0]->X == 3);
    CHECK(t.GetDepletionInfo()[0]->DepletionNodes[0]->Value == 0.5f);
    CHECK(t.GetDepletionInfo()[0]->DepletionNodes[0]->LastUpdate == 1234.5);
    REQUIRE(t.GetBiomeLockInfo().size() == 1);
    CHECK(t.GetBiomeLockInfo()[0]->BiomeName == "Shores");
    REQUIRE(t.GetPlanetScanInfo().size() == 1);
    CHECK(t.GetPlanetScanInfo()[0]->PlanetId == 2);
    auto outRoot = ConfigNode::Create();
    t.Save(*outRoot);
    CHECK(outRoot->GetNode("RESOURCE_SETTINGS")->ToString() == full->nodes()[0]->ToString());
    // SaveBiomeUnlockNode / SavePlanetUnlockNode are idempotent; SaveDepletionNodeValue updates the node in place and stamps LastUpdate
    t.SaveBiomeUnlockNode(1, "Shores");
    t.SaveBiomeUnlockNode(1, "Water");
    CHECK(t.GetBiomeLockInfo().size() == 2);
    t.SavePlanetUnlockNode(2);
    t.SavePlanetUnlockNode(3);
    CHECK(t.GetPlanetScanInfo().size() == 2);
    ResourceEnvironment::universalTime = [] { return 777.0; };
    t.SaveDepletionNodeValue(1, "Ore", 3, 4, 0.25f);
    t.SaveDepletionNodeValue(1, "Ore", 5, 6, 0.75f);
    t.SaveDepletionNodeValue(2, "Water", 0, 0, 1.0f);
    REQUIRE(t.GetDepletionInfo().size() == 2);
    CHECK(t.GetDepletionInfo()[0]->DepletionNodes.size() == 2);
    CHECK(t.GetDepletionInfo()[0]->DepletionNodes[0]->Value == 0.25f);
    CHECK(t.GetDepletionInfo()[0]->DepletionNodes[0]->LastUpdate == 777.0);
    ResourceEnvironment::universalTime = nullptr;

    // without a RESOURCE_SETTINGS node: MaxDeltaTime = 21600 and new seeds = KSPRandom(Environment.TickCount).Next(1, int.MaxValue); no mission
    ResourceEnvironment::tickCount = [] { return 123456; };
    ResourceGameSettings fresh;
    auto empty = ConfigNode::Create();
    fresh.Load(*empty);
    KSPRandom r(123456);
    CHECK(fresh.Seed == r.Next(1, INT32_MAX));
    CHECK(fresh.Seed >= 1);
    CHECK(fresh.MaxDeltaTime == ResourceGameSettings::defaultMaxDeltaTime);
    CHECK(fresh.GetDepletionInfo().empty());
    // MISSION game: the seeds of missions[0].situation
    ResourceGameSettings::missionSeeds = [](int& a, int& b) {
        a = 11;
        b = 22;
        return true;
    };
    ResourceGameSettings mission;
    mission.Load(*empty);
    CHECK(mission.Seed == 11);
    CHECK(mission.ROCMissionSeed == 22);
    ResourceGameSettings::missionSeeds = nullptr;
    ResourceEnvironment::tickCount = nullptr;
}

TEST_CASE("WP-15 ResourceUtilities: angle clamps and conversions, HSL2RGB") {
    // clampLat (:253) = (lat + 270) % 180 - 90 with C# `%` (fmod): 100 -> -80, -100 -> 80; clampLon = (lon + 540) % 360 - 180: 190 -> -170
    CHECK(ResourceUtilities::clampLat(100.0) == -80.0);
    CHECK(ResourceUtilities::clampLat(-100.0) == 80.0);
    CHECK(ResourceUtilities::clampLat(0.0) == 0.0);
    CHECK(ResourceUtilities::clampLon(190.0) == -170.0);
    CHECK(ResourceUtilities::clampLon(-190.0) == 170.0);
    CHECK(ResourceUtilities::Deg2Rad(180.0) == 3.141592653589793 * 180.0 / 180.0);
    // Rad2Lat / Rad2Lon (:161 / :177): wrap to [0, 2 pi), mirror, sign
    const double PI = 3.141592653589793;
    CHECK(ResourceUtilities::Rad2Lat(PI / 4.0) == doctest::Approx(45.0).epsilon(1e-12));
    CHECK(ResourceUtilities::Rad2Lat(-PI / 4.0) == doctest::Approx(-45.0).epsilon(1e-12));
    CHECK(ResourceUtilities::Rad2Lon(PI / 2.0) == doctest::Approx(90.0).epsilon(1e-12));
    CHECK(ResourceUtilities::Rad2Lon(3.0 * PI / 2.0) == doctest::Approx(-90.0).epsilon(1e-12));
    // HSL2RGB (:193): h = 0, full saturation, l = 0.5 is pure red; h = 0.5 gives cyan (6h = 3: r = num5 = 0, g = num11 = 1, b = num4 = 1)
    Color red = ResourceUtilities::HSL2RGB(0.0, 1.0, 0.5, 1.0f);
    CHECK(red.r == 1.0f);
    CHECK(red.g == 0.0f);
    CHECK(red.b == 0.0f);
    Color cyan = ResourceUtilities::HSL2RGB(0.5, 1.0, 0.5, 0.25f);
    CHECK(cyan.r == 0.0f);
    CHECK(cyan.g == 1.0f);
    CHECK(cyan.b == 1.0f);
    CHECK(cyan.a == 0.25f);
    Color grey = ResourceUtilities::HSL2RGB(0.3, 0.0, 0.4, 1.0f);  // sl = 0: num4 = l, num6 = 0... stays (l, l, l)-ish
    CHECK(grey.r == doctest::Approx(0.4f).epsilon(1e-6));
    // NoiseGenerator.GetSeed / SetSeed
    NoiseGenerator ng;
    ng.SetSeed({1, 2, 3, 4, 5, 6, 7, 8});
    CHECK(ng.GetSeed() == "1 2 3 4 5 6 7 8");
    // the same seed gives the same noise; the value is bounded (8 * 0.6^4 * |gradient| per corner, 4 corners)
    NoiseGenerator ng2;
    ng2.SetSeed({1, 2, 3, 4, 5, 6, 7, 8});
    for (float x = -3.0f; x < 3.0f; x += 0.37f)
        for (float y = -2.0f; y < 2.0f; y += 0.41f) {
            const float a = ng.noise(x, y, 0.5f);
            CHECK(Bits(a) == Bits(ng2.noise(x, y, 0.5f)));
            CHECK(std::fabs(a) < 4.0f);
        }
    // fastfloor (:173) is not floor: 2.5 -> 2, -2.5 -> -3, -2.0 -> -3, 0 -> -1 (sic) -- seen through the lattice cell of noise: the
    // value at an integer lattice point differs between the two cells it can be assigned to, but is stable per call
    CHECK(Bits(ng.noise(1.0f, 2.0f, 3.0f)) == Bits(ng2.noise(1.0f, 2.0f, 3.0f)));
    // coherentNoise with one octave, multiplier 25: noise(x/25, y/25, z/25) * amplitude 0.5
    const float c = ng.coherentNoise(10.0f, 20.0f, 30.0f);
    const float direct = ng2.noise(10.0f / 25.0f, 20.0f / 25.0f, 30.0f / 25.0f);
    CHECK(Bits(c) == Bits(static_cast<float>(static_cast<double>(direct) * 0.5)));
}

TEST_CASE("WP-15 ResourceMap.GetAbundance equals the independent Python reference; cache summaries; unlock / depletion records") {
    if (!GameAvailable()) return;
    Game15 g;
    Kerbol15 kerbol;
    // the reference: tools/wp15/abundance_reference.py (1,896 cases: 8 bodies x their biomes x 9 positions x lock / variance / difficulty)
    std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp15_abundance_reference.json", std::ios::binary);
    REQUIRE(f.good());
    const nlohmann::json ref = nlohmann::json::parse(f);
    ResourceScenario scenario;
    scenario.Awake();
    REQUIRE(ResourceScenario::Instance() == &scenario);
    REQUIRE(ResourceMap::Instance() != nullptr);
    ResourceMap& map = *ResourceMap::Instance();
    scenario.gameSettings()->Seed = ref.at("seed").get<int>();
    float difficulty = 1.0f;
    ResourceEnvironment::difficultyResourceAbundance = [&] { return difficulty; };
    map.ResetCache();
    size_t checked = 0, nonzero = 0, mismatches = 0;
    for (const auto& c : ref.at("cases")) {
        const std::string body = c.at("body");
        CelestialBody* cb = kerbol.sys->GetBodyByName(body);
        REQUIRE(cb != nullptr);
        REQUIRE(cb->flightGlobalsIndex == c.at("bodyId").get<int>());
        difficulty = c.at("difficulty").get<float>();
        // IsBiomeUnlocked reads the map's lock list: unlock (and re-lock through a cache reset) as the case says
        scenario.gameSettings()->GetBiomeLockInfo().clear();
        map.ResetCache();
        if (c.at("unlocked").get<bool>()) map.UnlockBiome(cb->flightGlobalsIndex, c.at("biome"));
        AbundanceRequest req;
        req.BodyId = cb->flightGlobalsIndex;
        req.ResourceName = "Ore";
        req.ResourceType = HarvestTypes::Planetary;
        req.BiomeName = c.at("biome");
        req.Latitude = c.at("lat");
        req.Longitude = c.at("lon");
        req.CheckForLock = c.at("checkForLock");
        req.ExcludeVariance = c.at("excludeVariance");
        const float got = map.GetAbundance(req);
        const uint32_t want = c.at("bits").get<uint32_t>();
        if (Bits(got) != want) {
            if (mismatches++ < 40) MESSAGE("mismatch " << body << " " << c.at("biome") << " lat " << c.at("lat") << " lon " << c.at("lon") << " lock " << c.at("checkForLock") << " excl " << c.at("excludeVariance") << " unl " << c.at("unlocked") << " diff " << difficulty << ": " << got << " vs " << c.at("value"));
        }
        checked++;
        if (got > 0.0f) nonzero++;
    }
    CHECK(mismatches == 0);
    CHECK(checked == ref.at("cases").size());
    CHECK(nonzero > checked / 2);
    difficulty = 1.0f;
    scenario.gameSettings()->GetBiomeLockInfo().clear();
    map.ResetCache();

    // ResourceCache.LoadAbundanceCache + ResourceMap.GetResourceByName / GetResourceItemList: per biome summaries (ExcludeVariance), float32
    // mean for a scanned planet (reference "bodyItems")
    ResourceEnvironment::difficultyResourceAbundance = [] { return 1.0f; };
    for (const auto& [bodyName, item] : ref.at("bodyItems").items()) {
        CelestialBody& body = *kerbol.sys->GetBodyByName(bodyName);
        CAPTURE(bodyName);
        // not scanned: an empty record
        CHECK(map.GetResourceByName("Ore", body, HarvestTypes::Planetary).resourceName.empty());
        CHECK(map.GetResourceItemList(HarvestTypes::Planetary, body).empty());
        map.UnlockPlanet(body.flightGlobalsIndex);
        CHECK(map.IsPlanetScanned(body.flightGlobalsIndex));
        PlanetaryResource pr = map.GetResourceByName("Ore", body, HarvestTypes::Planetary);
        CHECK(pr.resourceName == "Ore");
        CHECK(pr.resourceID == Id("Ore"));
        CHECK(Bits(pr.fraction) == item.at("fractionBits").get<uint32_t>());
        std::vector<PlanetaryResource> list = map.GetResourceItemList(HarvestTypes::Planetary, body);
        REQUIRE(list.size() == 1);
        CHECK(Bits(list[0].fraction) == item.at("fractionBits").get<uint32_t>());
        // the cache has one summary per biome in attribute order
        size_t k = 0;
        for (const auto& a : ResourceCache::Instance().AbundanceCache()) {
            if (a->BodyId != body.flightGlobalsIndex || a->HarvestType != HarvestTypes::Planetary) continue;
            REQUIRE(k < item.at("bits").size());
            CHECK(a->BiomeName == item.at("biomes")[k].get<std::string>());
            CHECK(Bits(a->Abundance) == item.at("bits")[k].get<uint32_t>());
            k++;
        }
        CHECK(k == item.at("bits").size());
    }
    // GetAbundanceFromCache: the summary, or 2 when there is none
    AbundanceRequest creq;
    creq.BodyId = kerbol.sys->GetBodyByName("Kerbin")->flightGlobalsIndex;
    creq.ResourceName = "Ore";
    creq.ResourceType = HarvestTypes::Planetary;
    CHECK(Bits(map.GetAbundanceFromCache(creq, "Water")) == ref.at("bodyItems").at("Kerbin").at("bits")[0].get<uint32_t>());
    CHECK(map.GetAbundanceFromCache(creq, "NoSuchBiome") == 2.0f);
    // FetchAllResourceNames: Ore for Planetary (GLOBAL + PLANETARY), nothing for the other harvest types in the stock data
    CHECK(map.FetchAllResourceNames(HarvestTypes::Planetary) == std::vector<std::string>{"Ore"});
    CHECK(map.FetchAllResourceNames(HarvestTypes::Oceanic).empty());
    CHECK(ResourceMap::GetDefaultSituation(HarvestTypes::Planetary) == "LANDED");
    CHECK(ResourceMap::GetDefaultSituation(HarvestTypes::Oceanic) == "SPLASHED");
    CHECK(ResourceMap::GetDefaultSituation(HarvestTypes::Atmospheric) == "FLYING");
    CHECK(ResourceMap::GetDefaultSituation(HarvestTypes::Exospheric) == "ORBITING");

    // DetermineBiomeName: an explicit BiomeName wins; without the biome-map decoder the lookup falls back to the default situation
    AbundanceRequest dreq = creq;
    dreq.BiomeName = "Shores";
    CHECK(map.DetermineBiomeName(dreq) == "Shores");
    dreq.BiomeName = "";
    CHECK(map.DetermineBiomeName(dreq) == "LANDED");
    // GetDepletionNode(lat, lon): Math.Round(radians) as float: lat 28.5 deg = 0.497 rad -> 0, lon -80 deg = -1.396 -> -1
    Vector2f dn = map.GetDepletionNode(28.5, -80.0);
    CHECK(dn.x == 0.0f);
    CHECK(dn.y == -1.0f);
    // depletion node records: default value 1, SetDepletionNodeValue updates the map and the scenario settings
    CHECK(map.GetDepletionNodeValue(1, "Ore", 3, 4) == 1.0f);
    ResourceEnvironment::universalTime = [] { return 99.0; };
    map.SetDepletionNodeValue(1, "Ore", 3, 4, 0.5f);
    CHECK(map.GetDepletionNodeValue(1, "Ore", 3, 4) == 0.5f);
    CHECK(map.GetDepletionNode(1, "Ore", 3, 4)->LastUpdate == 99.0);
    CHECK(map.GetDepletionNode(1, "Ore", 9, 9) == nullptr);
    CHECK(scenario.gameSettings()->GetDepletionInfo().size() == 1);
    ResourceEnvironment::universalTime = nullptr;
    // UnlockBiome twice: one record
    map.UnlockBiome(1, "Shores");
    map.UnlockBiome(1, "Shores");
    CHECK(map.IsBiomeUnlocked(1, "Shores"));
    CHECK_FALSE(map.IsBiomeUnlocked(1, "Water"));
    size_t locks = 0;
    for (const auto& b : map.BiomeLockInfo())
        if (b->BiomeName == "Shores") locks++;
    CHECK(locks == 1);
    // ResourceScenario.OnSave / OnLoad round trip the settings (including the records just made)
    auto saved = ConfigNode::Create();
    scenario.OnSave(*saved);
    ResourceGameSettings back;
    back.Load(*saved);
    CHECK(back.Seed == ref.at("seed").get<int>());
    CHECK(back.GetDepletionInfo().size() == 1);
    CHECK(back.GetBiomeLockInfo().size() >= 1);
    CHECK(back.GetPlanetScanInfo().size() >= 5);
    ResourceCache::DestroyInstance();
    ResourceEnvironment::difficultyResourceAbundance = nullptr;
}
