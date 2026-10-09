// WP-06 unit tests: the module host (C# constant evaluator, KSPField/KSPEvent/KSPAction registries, BaseFieldList.Load
// semantics, PartModule lifecycle and Save/Load, PartModuleList, mass/cost modifiers, VesselModule, ScenarioRunner). The
// capture.part_database gate covers the 32,911 module_fields rows of the real parts; these cases pin the host's API.
#include <cstring>
#include <limits>

#include "doctest/doctest.h"
#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/modules/base_field.hpp"
#include "ksp/modules/cs_expr.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/modules/vessel_module.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/util/ksp_util.hpp"

using namespace ksp;

namespace {

struct EnglishLocalizer {
    EnglishLocalizer() {
        Localizer::Shutdown();
        Localizer::InitWithBuiltInEnglish();
    }
    ~EnglishLocalizer() {
        Localizer::Shutdown();
        KSPUtil::CacheLocalStrings();
    }
};

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}

std::shared_ptr<PartModule> NewModule(const char* cls, PartPrefab* part = nullptr) {
    const ModuleClassInfo* c = GetPartModuleClass(cls);
    REQUIRE(c != nullptr);
    auto m = ModuleFactory::Create(*c);
    m->part = part;
    m->Awake();
    return m;
}

ConfigNode::NodePtr MakeNode(const char* text) {
    auto n = ConfigNode::Parse(text);  // a root holding the one top-level node
    REQUIRE(n);
    REQUIRE(n->nodes().Count() == 1);
    return n->nodes().Shared(0);
}

}  // namespace

TEST_CASE("WP-06 cs::EvaluateExpression: the C# constants of the module schemas") {
    cs::CsValue v;
    REQUIRE(cs::EvaluateExpression("1f / 17f", v));
    CHECK(v.kind == cs::CsValue::Kind::Float);
    CHECK(Bits(v.f) == Bits(f32store(fx(1.0f) / fx(17.0f))));
    REQUIRE(cs::EvaluateExpression("1E-05f", v));
    CHECK(Bits(v.f) == Bits(1E-05f));
    REQUIRE(cs::EvaluateExpression("float.MinValue", v));
    CHECK(v.f == -std::numeric_limits<float>::max());
    REQUIRE(cs::EvaluateExpression("uint.MaxValue", v));
    CHECK(v.i == 4294967295LL);
    REQUIRE(cs::EvaluateExpression("-Vector3.one", v));
    CHECK(v.kind == cs::CsValue::Kind::Vector);
    CHECK(v.vec == std::vector<float>{-1, -1, -1});
    REQUIRE(cs::EvaluateExpression("new Vector2(-15f, 15f)", v));
    CHECK(v.vec == std::vector<float>{-15, 15});
    REQUIRE(cs::EvaluateExpression("\"a\\\"b\"", v));
    CHECK(v.s == "a\"b");
    REQUIRE(cs::EvaluateExpression("TransformDir.Z", v));
    CHECK(v.kind == cs::CsValue::Kind::Member);
    CHECK(v.typeName == "TransformDir");
    CHECK(v.s == "Z");
    CHECK_FALSE(cs::EvaluateExpression("Foo.Bar(1)", v));  // outside the grammar
    CHECK_FALSE(cs::EvaluateExpression("1 +", v));
    REQUIRE(cs::EvaluateExpression("double.NaN", v));
    CHECK(v.d != v.d);
}

TEST_CASE("WP-06 cs::ParseAttributeArguments") {
    std::vector<cs::AttributeArg> a;
    REQUIRE(cs::ParseAttributeArguments("guiActive = true, guiName = \"#a, b\", unfocusedRange = 5f", a));
    REQUIRE(a.size() == 3);
    CHECK(a[0].name == "guiActive");
    CHECK(a[0].value.b);
    CHECK(a[1].value.s == "#a, b");  // comma inside a string
    CHECK(a[2].value.f == 5.0f);
    REQUIRE(cs::ParseAttributeArguments("\"#autoLOC_6001380\", KSPActionGroup.Stage, activeEditor = false", a));
    REQUIRE(a.size() == 3);
    CHECK(a[0].name.empty());
    CHECK(a[1].value.kind == cs::CsValue::Kind::Member);
    CHECK(a[2].name == "activeEditor");
}

TEST_CASE("WP-06 field initialisers: C# defaults per type") {
    EnglishLocalizer loc;
    auto gimbal = NewModule("ModuleGimbal");
    CHECK(gimbal->Get<float>("gimbalRange") == 10.0f);
    CHECK(gimbal->Get<float>("gimbalRangeXP") == -1.0f);
    CHECK(gimbal->Get<bool>("stagingEnabled"));  // PartModule's own [KSPField]s are part of every module
    CHECK(gimbal->Get<std::string>("stagingEnableText").empty());
    auto engines = NewModule("ModuleEngines");
    CHECK(engines->Get<float>("throttleResponseRate") == -1.0f);
    CHECK(engines->IsNull("atmosphereCurve") == false);  // OnAwake creates it
    CHECK(NewModule("ModuleControlSurface")->FieldInfos().size() == GetPartModuleClass("ModuleControlSurface")->Fields().size());
    // a C# `new` field hides the base class's: both are in the list, the first match wins
    auto cs = NewModule("ModuleControlSurface");
    int n = 0;
    for (const ModuleFieldInfo* f : cs->FieldInfos())
        if (std::string_view(f->name) == "transformName") n++;
    CHECK(n == 2);
    CHECK(cs->Get<std::string>("transformName") == "obj_ctrlSrf");
}

TEST_CASE("WP-06 BaseFieldList.Load: typed ReadPvt, last assignment, unparseable values keep the default") {
    EnglishLocalizer loc;
    auto m = NewModule("ModuleGimbal");
    auto node = MakeNode("MODULE\n{\n name = ModuleGimbal\n gimbalRange = 25.5\n gimbalRangeXP = oops\n gimbalLock = True\n"
                               " gimbalRange = 12\n unknownField = 1\n}\n");
    m->Load(*node);
    CHECK(m->Get<float>("gimbalRange") == 12.0f);                       // the later line wins
    CHECK(m->LastLoadTrace().size() == 4);                              // gimbalRange, gimbalRangeXP, gimbalLock, gimbalRange
    CHECK_FALSE(m->LastLoadTrace()[1].parsed);                          // "oops": error logged, field unchanged
    // OnLoad: gimbalRangeXP stays < 0 so it takes gimbalRange; XN follows XP
    CHECK(m->Get<float>("gimbalRangeXP") == 12.0f);
    CHECK(m->Get<float>("gimbalRangeXN") == 12.0f);
    CHECK(m->Get<bool>("gimbalLock"));
    CHECK(m->isEnabled);
}

TEST_CASE("WP-06 BaseFieldList.Load: Vector, enum and FloatCurve fields") {
    EnglishLocalizer loc;
    auto m = NewModule("ModuleDeployableSolarPanel");
    auto node = MakeNode("MODULE\n{\n name = ModuleDeployableSolarPanel\n trackingMode = NONE\n alignType = PIVOT\n"
                               " temperatureEfficCurve\n {\n key = 0 2 0 0\n key = 1 1 0 0\n }\n}\n");
    m->Load(*node);
    CHECK(m->EnumName("trackingMode") == "NONE");
    const auto* fc = std::get_if<FloatCurve>(m->Value("temperatureEfficCurve"));
    REQUIRE(fc != nullptr);
    CHECK(fc->Evaluate(0.0f) == 2.0f);
    CHECK(fc->Evaluate(1.0f) == 1.0f);
    // the node replaced the OnAwake default curve (5 keys)
    CHECK(fc->Curve().keys().size() == 2);
    // subPartName went through Localizer.Format in OnLoad
    CHECK(m->Get<std::string>("subPartName") == Localizer::Format("#autoLOC_235328"));
}

TEST_CASE("WP-06 BaseField: guiName, persistence, UI controls, string forms") {
    EnglishLocalizer loc;
    auto m = NewModule("ModuleEngines");
    BaseField* thrust = m->Fields()["thrustPercentage"];
    REQUIRE(thrust != nullptr);
    CHECK(thrust->guiActive);
    CHECK(thrust->guiName == Localizer::Format("#autoLOC_6001363"));
    REQUIRE(thrust->uiControlFlight != nullptr);
    CHECK(thrust->uiControlFlight->Is("UI_FloatRange"));
    CHECK(thrust->uiControlFlight->requireFullControl);
    REQUIRE(thrust->uiControlFlight->Param("maxValue") != nullptr);
    CHECK(thrust->uiControlFlight->Param("maxValue")->f == 100.0f);
    CHECK(thrust->uiControlFlight->Param("stepIncrement")->f == 0.5f);
    CHECK(thrust->GetStringValue(false).value() == "100");
    auto* axis = dynamic_cast<BaseAxisField*>(thrust);  // [KSPAxisField]: AxisGroupsManager.BuildBaseAxisFields
    REQUIRE(axis != nullptr);
    CHECK(axis->maxValue == 100.0f);
    CHECK(axis->incrementalSpeed == 20.0f);
    CHECK(axis->axisIncremental == (BaseAxisField::MaxAxisGroup * 2 - 1) - 64);  // incremental, MainThrottle removed
    CHECK(axis->overrideGroups.size() == 4);
    BaseField* staging = m->Fields()["stagingEnabled"];
    CHECK(staging->isPersistant);
    CHECK(staging->GetStringValue(false).value() == "True");
    // float "G9" and double "G17" (BaseField.cs:590)
    thrust->SetValue(ModuleValue(0.1f));
    CHECK(thrust->GetStringValue(false).value() == "0.100000001");
    // SetValue rejects a value of the wrong C# type
    CHECK_FALSE(thrust->SetValue(ModuleValue(std::string("x"))));
    CHECK(m->Get<float>("thrustPercentage") == 0.1f);
}

TEST_CASE("WP-06 PartModule Save / Load round trip") {
    EnglishLocalizer loc;
    auto a = NewModule("ModuleGimbal");
    a->Set<bool>("stagingEnabled", false);
    a->isEnabled = false;
    ConfigNode saved("MODULE");
    a->Save(saved);
    CHECK(saved.GetValue("name") == "ModuleGimbal");
    CHECK(saved.GetValue("isEnabled") == "False");
    CHECK(saved.GetValue("stagingEnabled") == "False");
    CHECK(saved.HasNode("EVENTS"));
    CHECK(saved.HasNode("ACTIONS"));
    CHECK(saved.HasNode("UPGRADESAPPLIED"));
    CHECK_FALSE(saved.HasValue("gimbalRange"));  // not isPersistant
    auto b = NewModule("ModuleGimbal");
    b->Load(saved);
    CHECK_FALSE(b->isEnabled);
    CHECK_FALSE(b->Get<bool>("stagingEnabled"));
}

TEST_CASE("WP-06 BaseEventList / BaseActionList: reflected attributes, Send, persistence") {
    EnglishLocalizer loc;
    auto m = NewModule("ModuleEngines");
    BaseEvent* activate = m->Events()["Activate"];
    REQUIRE(activate != nullptr);
    CHECK(activate->guiActive);
    CHECK(activate->guiName() == Localizer::Format("#autoLOC_6001382"));
    CHECK(activate->id() == MonoStringHash("Activate"));
    CHECK(m->Events()["ToggleStaging"] != nullptr);  // PartModule's own event (inherited)
    int calls = 0;
    CHECK(m->Events().Bind("Activate", BaseEvent::Handler([&] { calls++; })));
    CHECK(m->Events().Send("Activate"));
    CHECK(calls == 1);
    m->isEnabled = false;
    CHECK_FALSE(m->Events().Send("Activate"));  // a disabled module does not receive events
    CHECK(calls == 1);
    m->isEnabled = true;
    activate->active = false;
    CHECK_FALSE(m->Events().Send("Activate"));
    // BaseEvent.OnSave writes `category` from guiIcon (sic)
    activate->isPersistent = true;
    ConfigNode n("EVENTS");
    m->Events().OnSave(n);
    ConfigNode* en = n.GetNode("Activate");
    REQUIRE(en != nullptr);
    CHECK(en->GetValue("category") == en->GetValue("guiIcon"));
    CHECK(en->GetValue("active") == "False");
    // actions
    BaseAction* onAction = m->Actions()["OnAction"];
    REQUIRE(onAction != nullptr);
    CHECK_FALSE(onAction->activeEditor);
    CHECK(onAction->guiName() == Localizer::Format("#autoLOC_6001380"));
    CHECK(onAction->overrideGroups.size() == 4);
    onAction->actionGroup = 1;  // Stage
    ConfigNode an("ACTIONS");
    m->Actions().OnSave(an);
    CHECK(an.GetNode("OnAction")->GetValue("actionGroup") == "Stage");
    CHECK(m->Actions().Contains(1));
}

TEST_CASE("WP-06 PartModuleList: lookup, Contains, Send, modifier sums") {
    EnglishLocalizer loc;
    PartModuleList list;
    auto gimbal = NewModule("ModuleGimbal");
    auto fireworks = NewModule("ModulePartFirework");
    list.Add(gimbal);
    list.Add(fireworks);
    list.Add(gimbal);  // already in the list: ignored
    CHECK(list.Count() == 2);
    CHECK(list["ModuleGimbal"] == gimbal.get());
    CHECK(list.Contains("ModulePartFirework"));
    CHECK_FALSE(list.Contains("ModuleEngines"));
    CHECK(list.IndexOf(fireworks.get()) == 1);
    CHECK(list.GetModule("ModuleGimbal") == gimbal.get());
    // Part.GetModuleMass: IPartMassModifier modules only; defaultMass + shots * shellMass * 0.001f (Mono float rule)
    const float defaultMass = 0.1f;
    const float expect = f32store(fx(defaultMass) + fx(32.0f) * fx(0.1f) * fx(0.001f));
    CHECK(Bits(PartGetModuleMass(list, defaultMass)) == Bits(f32store(fx(0.0f) + fx(expect))));
    CHECK(PartGetModuleCosts(list, 5.0f) == 0.0f);
    int sent = 0;
    gimbal->Events().Bind("ToggleStaging", BaseEvent::Handler([&] { sent++; }));
    CHECK(list.Send("ToggleStaging"));
    CHECK(sent == 1);
    CHECK_FALSE(list.Send("NoSuchEvent"));
}

TEST_CASE("WP-06 module behaviours: OnLoad effects on Part prefab data and fields") {
    EnglishLocalizer loc;
    PartPrefab part;
    part.gameObjectName = "testPart";
    part.fields.fuelCrossFeed = false;
    auto xf = NewModule("ModuleToggleCrossfeed", &part);
    xf->Load(*MakeNode("MODULE\n{\n name = ModuleToggleCrossfeed\n crossfeedStatus = True\n}\n"));
    CHECK(part.fields.fuelCrossFeed);  // ModuleToggleCrossfeed.OnLoad -> UpdateCrossfeed
    // ModuleCargoPart.OnLoad: stackableQuantity = 1 when the part has resources
    auto cargo = NewModule("ModuleCargoPart", &part);
    cargo->Load(*MakeNode("MODULE\n{\n name = ModuleCargoPart\n stackableQuantity = 4\n}\n"));
    CHECK(cargo->Get<int32_t>("stackableQuantity") == 4);
    part.Resources.emplace_back();
    cargo->Load(*MakeNode("MODULE\n{\n name = ModuleCargoPart\n stackableQuantity = 4\n}\n"));
    CHECK(cargo->Get<int32_t>("stackableQuantity") == 1);
    // BaseConverter.OnLoad: status text; OnAwake: default curves
    auto conv = NewModule("ModuleResourceConverter", &part);
    CHECK_FALSE(conv->IsNull("ThermalEfficiency"));
    conv->Load(*MakeNode("MODULE\n{\n name = ModuleResourceConverter\n}\n"));
    CHECK(conv->Get<std::string>("status") == Localizer::Format("#autoLOC_257112"));
    // modifiers: ModuleComet clamps to Mathf.Epsilon
    auto comet = NewModule("ModuleComet", &part);
    IPartMassModifier* mm = comet->AsMassModifier();
    REQUIRE(mm != nullptr);
    CHECK(Bits(mm->GetModuleMass(5.0f, ModifierStagingSituation::CURRENT)) == 1u);  // 1.4e-45
}

TEST_CASE("WP-06 ModuleFactory: unregistered classes are generic, unknown classes absent") {
    CHECK(GetPartModuleClass("NoSuchModule") == nullptr);
    CHECK_FALSE(ModuleFactory::IsRegistered("ModuleLight"));
    CHECK(ModuleFactory::IsRegistered("ModuleGimbal"));
    EnglishLocalizer loc;
    auto light = NewModule("ModuleLight");  // generic PartModule: schema fields, events, no behaviour
    CHECK(light->Get<std::string>("animationName") == "LightAnimation");
}

TEST_CASE("WP-06 VesselModuleManager: order, activation, wrappers") {
    VesselModuleManager::CompileModules();
    const auto ordered = VesselModuleManager::GetModules(true, true);
    REQUIRE(!ordered.empty());
    CHECK(std::string_view(ordered.front().type->name) == "FlightIntegrator");  // GetOrder() == 0
    CHECK(ordered.front().order == 0);
    CHECK(VesselModuleManager::GetWrapper("AxisGroupsModule") != nullptr);
    VesselModuleManager::SetWrapperActive("AxisGroupsModule", false);
    bool found = false;
    for (const auto& w : VesselModuleManager::GetModules(true, false))
        if (std::string_view(w.type->name) == "AxisGroupsModule") found = true;
    CHECK_FALSE(found);
    VesselModuleManager::SetWrapperActive("AxisGroupsModule", true);
    // AxisGroupsModule.GetActivation: FlightScene | LoadedVessels
    auto axes = VesselModuleFactory::Create(*GetVesselModuleClass("AxisGroupsModule"));
    CHECK(axes->ShouldBeActive(true, true));
    CHECK_FALSE(axes->ShouldBeActive(false, true));
    CHECK_FALSE(axes->ShouldBeActive(true, false));
    // module list for a vessel
    std::vector<std::shared_ptr<VesselModule>> mods;
    int vessel = 0;
    VesselModuleManager::AddModulesToVessel(&vessel, mods, nullptr, true, true);
    CHECK(mods.size() == ordered.size());
    CHECK(mods.front()->vessel == &vessel);
    // VesselModule fields: CometVessel.radius default 115f, persisted through Save/Load
    auto comet = VesselModuleFactory::Create(*GetVesselModuleClass("CometVessel"));
    comet->Awake();
    CHECK(comet->Get<float>("radius") == 115.0f);
    comet->Set<float>("radius", 200.0f);
    ConfigNode n("VESSELMODULE");
    comet->Save(n);
    CHECK(n.GetValue("radius") == "200");
    auto comet2 = VesselModuleFactory::Create(*GetVesselModuleClass("CometVessel"));
    comet2->Awake();
    comet2->Load(n);
    CHECK(comet2->Get<float>("radius") == 200.0f);
}

TEST_CASE("WP-06 ScenarioRunner: modules from proto modules, generic fields, snapshots") {
    EnglishLocalizer loc;
    SaveEnvironment& env = SaveEnvironment::Current();
    const GameScenes saved = env.loadedScene;
    env.loadedScene = GameScenes::SPACECENTER;
    {
        ScenarioRunner runner;
        CHECK(ScenarioRunner::Instance() == &runner);
        auto node = MakeNode("SCENARIO\n{\n name = SentinelScenario\n scene = 5, 6\n NextSpawnTime = 123.5\n}\n");
        auto proto = std::make_shared<ProtoScenarioModule>(*node);
        CHECK(proto->moduleName == "SentinelScenario");
        CHECK(proto->targetScenes.size() == 2);
        CHECK(static_cast<int>(proto->targetScenes[0]) == 5);
        ScenarioRunner::SetProtoModules(std::vector<std::shared_ptr<ProtoScenarioModule>>{proto});
        // scene 5 (SPACECENTER) is a target scene: the module was created, Load'ed and linked to its snapshot
        REQUIRE(ScenarioRunner::GetLoadedModules().size() == 1);
        ScenarioModule* m = ScenarioRunner::GetLoadedModules()[0].get();
        CHECK(m->ClassName() == "SentinelScenario");
        CHECK(m->snapshot == proto.get());
        CHECK(proto->moduleRef == m);
        REQUIRE(m->Store() != nullptr);
        CHECK(m->Store()->Get<double>("NextSpawnTime") == 123.5);
        // Save: name, scene, persistent fields
        ConfigNode out("SCENARIO");
        m->Save(out);
        CHECK(out.GetValue("name") == "SentinelScenario");
        CHECK(out.GetValue("scene") == "5, 6");
        CHECK(out.GetValue("NextSpawnTime") == "123.5");
        // GetUpdatedProtoModules re-snapshots the live module
        auto& updated = ScenarioRunner::GetUpdatedProtoModules();
        REQUIRE(updated.size() == 1);
        CHECK(updated[0].get() != proto.get());
        CHECK(updated[0]->GetData().GetValue("NextSpawnTime") == "123.5");
        // a scene load request destroys the modules
        GameEvents::onGameSceneLoadRequested.Fire(GameScenes::FLIGHT);
        CHECK(ScenarioRunner::GetLoadedModules().empty());
        // a module of an unknown type is refused
        CHECK(runner.AddModule("NoSuchScenario") == nullptr);
    }
    CHECK(ScenarioRunner::Instance() == nullptr);
    env.loadedScene = saved;
}

namespace {
class FakeScalarModule : public PartModule, public IScalarModule {
public:
    FakeScalarModule(const ModuleClassInfo& c, std::string id) : PartModule(c), id_(std::move(id)) {}
    IScalarModule* AsScalarModule() override { return this; }
    std::string ScalarModuleID() const override { return id_; }
    float GetScalar() const override { return scalar; }
    bool CanMove() const override { return true; }
    void SetScalar(float t) override { scalar = t; }
    void SetUIRead(bool) override {}
    void SetUIWrite(bool) override {}
    bool IsMoving() override { return false; }
    float scalar = 0.0f;

private:
    std::string id_;
};
}  // namespace

TEST_CASE("WP-06 ScalarModuleSetHandler / BaseEventDetails") {
    EnglishLocalizer loc;
    PartModuleList list;
    const ModuleClassInfo* c = GetPartModuleClass("PartModule");
    REQUIRE(c != nullptr);
    auto a = std::make_shared<FakeScalarModule>(*c, "fxA");
    auto b = std::make_shared<FakeScalarModule>(*c, "fxB");
    a->Awake();
    b->Awake();
    list.Add(a);
    list.Add(b);
    CHECK(list.GetScalarModule("fxB") == b.get());
    ScalarModuleSetHandler h;
    CHECK_THROWS_AS(h.Save(*ConfigNode::Create()), NullReferenceException);  // fxModuleNames is null until Load
    auto node = MakeNode("MODULE\n{\n scalarModules = fxA \n scalarModules = fxB\n}\n");
    h.Load(*node);
    h.Initialize(list);
    CHECK(h.fxModules.size() == 2);
    h.SetFXModules(0.25f);
    CHECK(a->scalar == 0.25f);
    CHECK(b->scalar == 0.25f);
    ConfigNode out("X");
    h.Save(out);
    CHECK(out.GetValues("moduleID").size() == 2);

    BaseEventDetails d(BaseEventDetails::Sender::GUI);
    d.Set("amount", 3);
    d.Set("label", std::string("hi"));
    CHECK(d.GetInt("amount") == 3);
    CHECK(d.GetString("label") == "hi");
    CHECK(d.GetString("missing").empty());  // logs "Invalid key: missing"
    CHECK_THROWS(d.Set("amount", 4));       // Hashtable.Add on a duplicate key
}
