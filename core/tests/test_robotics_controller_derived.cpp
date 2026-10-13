// WP-52 derived cases, part 3 (labelled): the robotics controller's axis / action bookkeeping, symmetry transfer, the manager's priority queue, persistence and
// the curve editing helpers. Expectations are derived by hand from the cited C# lines; every controller / servo value is a SYNTHETIC fixture.
#include "robotics_fixture.hpp"

#include "ksp/input/control_types.hpp"
#include "ksp/input/input_lock_manager.hpp"

using namespace wp52t;

namespace {

float F32(double v) { return f32store(v); }

struct Flight {
    Rig rig;
    std::unique_ptr<Rig::One> one;
    Flight() {
        TimeWarp::fetch = &rig.env.tw;
        rig.env.tw.current_rate_index = 0;
        Time::fixedDeltaTime = 0.02f;
        Time::deltaTime = 0.02f;
        one = rig.MakeOne();
        SaveEnvironment::Current().SetStockExpansionInstalled("Serenity", true);
    }
    Part& part() { return *one->part; }
};

BaseAxisField* Axis(PartModule& m, const char* name) { return dynamic_cast<BaseAxisField*>(m.Fields()[name]); }

// registers a part's persistentId in the FlightGlobals-like registry (ControlledBase.AssignReferenceVars -> FlightGlobals.FindLoadedPart)
void RegisterPart(Part& p) { SaveEnvironment::Current().persistentIds.persistentLoadedPartIds.Add(p.fields.persistentId, &p); }

struct AxisEventLog {
    std::vector<std::string> ev;
    void Adding(ModuleRoboticController*, ControlledAxis*) { ev.push_back("axisAdding"); }
    void Removing(ModuleRoboticController*, ControlledAxis*) { ev.push_back("axisRemoving"); }
    void Changed(ModuleRoboticController*) { ev.push_back("axesChanged"); }
    void AAdding(ModuleRoboticController*, ControlledAction*) { ev.push_back("actionAdding"); }
    void ARemoving(ModuleRoboticController*, ControlledAction*) { ev.push_back("actionRemoving"); }
    void AChanged(ModuleRoboticController*) { ev.push_back("actionsChanged"); }
    AxisEventLog() {
        GameEvents::onRoboticControllerAxesAdding.Add(this, &AxisEventLog::Adding);
        GameEvents::onRoboticControllerAxesRemoving.Add(this, &AxisEventLog::Removing);
        GameEvents::onRoboticControllerAxesChanged.Add(this, &AxisEventLog::Changed);
        GameEvents::onRoboticControllerActionsAdding.Add(this, &AxisEventLog::AAdding);
        GameEvents::onRoboticControllerActionsRemoving.Add(this, &AxisEventLog::ARemoving);
        GameEvents::onRoboticControllerActionsChanged.Add(this, &AxisEventLog::AChanged);
    }
    ~AxisEventLog() {
        GameEvents::onRoboticControllerAxesAdding.Remove(this, &AxisEventLog::Adding);
        GameEvents::onRoboticControllerAxesRemoving.Remove(this, &AxisEventLog::Removing);
        GameEvents::onRoboticControllerAxesChanged.Remove(this, &AxisEventLog::Changed);
        GameEvents::onRoboticControllerActionsAdding.Remove(this, &AxisEventLog::AAdding);
        GameEvents::onRoboticControllerActionsRemoving.Remove(this, &AxisEventLog::ARemoving);
        GameEvents::onRoboticControllerActionsChanged.Remove(this, &AxisEventLog::AChanged);
    }
};

}  // namespace

TEST_CASE("WP-52 controller bookkeeping: adding an axis collects the symmetry partners, a partner's axis is refused, UpdateFieldValue drives both") {
    Flight f;
    Part& p1 = f.part();
    Part& p2 = *f.rig.AddPart(*f.one);
    p1.symmetryCounterparts = {&p2};
    p2.symmetryCounterparts = {&p1};
    auto* h1 = AddServo<ModuleRoboticServoHinge>(p1, "ModuleRoboticServoHinge");
    auto* h2 = AddServo<ModuleRoboticServoHinge>(p2, "ModuleRoboticServoHinge");
    auto* ctl = AddController(p1);
    BaseAxisField* a1 = Axis(*h1, "targetAngle");
    BaseAxisField* a2 = Axis(*h2, "targetAngle");
    REQUIRE(a1 != nullptr);
    REQUIRE(a2 != nullptr);
    AxisEventLog log;
    ctl->AddPartAxis(&p1, h1, a1);
    REQUIRE(ctl->controlledAxes.size() == 1);
    CHECK(log.ev == std::vector<std::string>{"axisAdding", "axesChanged"});
    ControlledAxis& ax = *ctl->controlledAxes[0];
    CHECK(ax.axisName == "targetAngle");
    CHECK(ax.partId == p1.fields.persistentId);
    CHECK(ax.moduleId == h1->PersistentId());
    REQUIRE(ax.SymmetryFields.size() == 1);
    CHECK(ax.SymmetryFields[0] == a2);
    CHECK(ctl->HasPartAxisField(&p1, a1));
    CHECK(!ctl->HasPartSymmetryAxisField(&p1, a1));
    CHECK(ctl->HasPartSymmetryAxisField(&p2, a2));
    CHECK(!ctl->HasPartAxisField(&p2, a2));  // TryGetPartAxisField hides the symmetry hit
    CHECK(ctl->HasPart(&p1));
    CHECK(!ctl->HasPart(&p2));
    CHECK(ctl->HasPart(std::vector<Part*>{&p2, &p1}));
    // a symmetry partner's axis is not added again
    log.ev.clear();
    ctl->AddPartAxis(&p2, h2, a2);
    CHECK(ctl->controlledAxes.size() == 1);
    CHECK(log.ev.empty());
    // the default axis curve is flat at the field minimum: (0, min) (1, min)
    a1->minValue = 5.0f;
    ControlledAxis other(&p1, h1, a1, ctl);
    CHECK(other.timeValue.Curve().length() == 2);
    CHECK(other.timeValue.Curve()[0].value == 5.0f);
    CHECK(other.timeValue.Curve()[1].time == 1.0f);
    // UpdateFieldValue without a manager sets the field and the partner's field to curve(time) * multiple
    ax.timeValue = FloatCurve(std::vector<Keyframe>{Keyframe(0.0f, 0.0f, 10.0f, 10.0f), Keyframe(1.0f, 10.0f, 10.0f, 10.0f)});
    ax.UpdateFieldValue(0.5f, 2.0f);
    const float want = F32(fx(ax.timeValue.Evaluate(0.5f)) * fx(2.0f));
    CHECK(h1->targetAngle() == want);
    CHECK(h2->targetAngle() == want);
    CHECK(want == doctest::Approx(10.0f).epsilon(1e-5));  // 5 * 2
    ax.UpdateFieldValue(0.25f, 1.0f, /*updateSymmetryPartners*/ false);
    CHECK(h1->targetAngle() == doctest::Approx(2.5f).epsilon(1e-5));
    CHECK(h2->targetAngle() == want);
    // deleting the part transfers the axis to its symmetry partner on the same vessel (ModuleRoboticController.cs:1057-1069): the old row goes, a copy of the
    // curve is controlled on the partner
    log.ev.clear();
    ctl->PartDeleted(&p1);
    REQUIRE(ctl->controlledAxes.size() == 1);
    CHECK(ctl->controlledAxes[0]->PartPtr() == &p2);
    CHECK(ctl->controlledAxes[0]->AxisField == a2);
    CHECK(ctl->controlledAxes[0]->timeValue.Curve().keys().size() == 2);
    CHECK(ctl->controlledAxes[0]->timeValue.Curve()[1].value == 10.0f);
    CHECK(log.ev == std::vector<std::string>{"axesChanged"});  // the transfer path fires no Removing event; Changed once
    // an axis row of a part with no partner on the vessel is removed with the Removing event
    log.ev.clear();
    p2.symmetryCounterparts.clear();
    ctl->PartDeleted(&p2);
    CHECK(ctl->controlledAxes.empty());
    CHECK(log.ev == std::vector<std::string>{"axisRemoving", "axesChanged"});
    h1->OnDestroy();
    h2->OnDestroy();
}

TEST_CASE("WP-52 controller bookkeeping: actions, the removal condition, other-vessel cleanup, id changes") {
    Flight f;
    Part& p1 = f.part();
    auto other = f.rig.MakeOne();  // another vessel (its resource host replaces the first: this test never ticks a servo)
    Part& p3 = *other->part;
    p3.fields.persistentId = 9300;
    auto* h1 = AddServo<ModuleRoboticServoHinge>(p1, "ModuleRoboticServoHinge");
    auto* h3 = AddServo<ModuleRoboticServoHinge>(p3, "ModuleRoboticServoHinge");
    auto* ctl = AddController(p1);
    AxisEventLog log;
    BaseAction* act = h1->Actions()["MaximumAngleAction"];
    REQUIRE(act != nullptr);
    ctl->AddPartAction(&p1, h1, act);
    REQUIRE(ctl->controlledActions.size() == 1);
    CHECK(log.ev == std::vector<std::string>{"actionAdding", "actionsChanged"});
    CHECK(ctl->controlledActions[0]->actionName == "MaximumAngleAction");
    CHECK(ctl->HasPartAction(&p1, act));
    ControlledAction* found = nullptr;
    CHECK(ctl->TryGetPartAction(&p1, act, found));
    CHECK(found == ctl->controlledActions[0].get());
    // the copy constructor makes a new row on the same action with a copy of the times
    ctl->controlledActions[0]->times = {0.25f, 0.75f};
    ControlledAction copy(*ctl->controlledActions[0]);
    CHECK(copy.times == std::vector<float>{0.25f, 0.75f});
    CHECK(copy.Action == act);
    // ReverseTimes: 1 - t, in reverse order (ControlledAction.cs:48)
    copy.ReverseTimes();
    CHECK(copy.times == std::vector<float>{F32(1.0 - 0.75), F32(1.0 - 0.25)});
    // RescaleTimes: a ratio above 1 clamps the tail by minSpace per remaining row (:60)
    copy.times = {0.5f, 0.8f, 0.9f};
    copy.RescaleTimes(2.0f, 0.01f);
    CHECK(copy.times[0] == F32(std::min(fx(F32(0.5 * 2.0)), 1.0 - fx(0.01f) * 2.0)));
    CHECK(copy.times[2] == F32(std::min(fx(F32(fx(0.9f) * 2.0)), 1.0 - fx(0.01f) * 0.0)));
    copy.times = {0.5f, 0.8f};
    copy.RescaleTimes(0.5f);
    CHECK(copy.times[0] == 0.25f);
    CHECK(copy.times[1] == F32(fx(0.8f) * 0.5));
    // removal of the part's rows with no symmetry partner: the action row goes, with the Removing event
    log.ev.clear();
    ctl->PartDeleted(&p1);
    CHECK(ctl->controlledActions.empty());
    CHECK(log.ev == std::vector<std::string>{"actionRemoving", "actionsChanged"});
    // rows on another vessel are removed when the vessel is modified, unless GameSettings.SERENITY_CONTROLLER_IGNORES_VESSEL
    ctl->AddPartAxis(&p3, h3, Axis(*h3, "targetAngle"));
    ctl->AddPartAction(&p3, h3, h3->Actions()["MaximumAngleAction"]);
    CHECK(ctl->controlledAxes.size() == 1);
    CHECK(ctl->controlledActions.size() == 1);
    SceneScope sc(GameScenes::FLIGHT);
    GameSettings::SERENITY_CONTROLLER_IGNORES_VESSEL = true;
    ctl->VesselModified(p1.vessel);
    CHECK(ctl->controlledAxes.size() == 1);
    GameSettings::SERENITY_CONTROLLER_IGNORES_VESSEL = false;
    ctl->VesselModified(other->v);  // not this controller's vessel: nothing
    CHECK(ctl->controlledAxes.size() == 1);
    ctl->VesselModified(p1.vessel);
    CHECK(ctl->controlledAxes.empty());
    CHECK(ctl->controlledActions.empty());
    // persistent id changes (ModuleRoboticController.cs:930, :957)
    ctl->AddPartAxis(&p1, h1, Axis(*h1, "targetAngle"));
    RegisterPart(p1);
    const uint32_t oldId = p1.fields.persistentId;
    p1.fields.persistentId = 7777;
    RegisterPart(p1);
    ctl->PartPersistentIdChanged(1, oldId, 7777);
    CHECK(ctl->controlledAxes[0]->partId == 7777);
    ctl->UpdatePersistentIDs({{7777, 8888}, {1, 2}});
    CHECK(ctl->controlledAxes[0]->partId == 8888);
    h1->OnDestroy();
    h3->OnDestroy();
}

TEST_CASE("WP-52 RoboticControllerManager: the highest priority wins, equal priorities are averaged, the queue resets every frame") {
    Flight f;
    Part& p = f.part();
    auto* h = AddServo<ModuleRoboticServoHinge>(p, "ModuleRoboticServoHinge");
    auto* c1 = AddController(p);
    auto* c2 = AddController(p);
    auto* c3 = AddController(p);
    for (auto* c : {c1, c2, c3}) c->Set<float>("priorityField", 3.0f);
    c2->Set<float>("priorityField", 5.0f);
    c1->PriorityChanged();
    c2->PriorityChanged();
    c3->PriorityChanged();
    CHECK(c1->Priority() == 3);
    CHECK(c2->Priority() == 5);
    c3->Set<float>("priorityField", 99.0f);
    c3->PriorityChanged();
    CHECK(c3->Priority() == 5);  // clamped to 1..5 (cs:923)
    c3->Set<float>("priorityField", 3.0f);
    c3->PriorityChanged();
    BaseAxisField* field = Axis(*h, "targetAngle");
    RoboticControllerManager mgr;
    mgr.Awake();
    REQUIRE(RoboticControllerManager::Instance == &mgr);
    // a second manager is a usurper; Serenity absent destroys the manager
    RoboticControllerManager usurper;
    LogCapture logs;
    usurper.Awake();
    CHECK(usurper.destroyed);
    CHECK(logs.Has(log::Level::Error, "Only one instance"));
    CHECK(RoboticControllerManager::Instance == &mgr);
    // two priority-3 writers and a priority-5 one
    mgr.QueueFieldUpdate(field, 10.0f, c1->Priority());
    mgr.QueueFieldUpdate(field, 20.0f, c3->Priority());
    mgr.Update();
    CHECK(h->targetAngle() == 15.0f);  // (10 + 20) / 2
    mgr.QueueFieldUpdate(field, 10.0f, 3);
    mgr.QueueFieldUpdate(field, 60.0f, 5);
    mgr.QueueFieldUpdate(field, 20.0f, 3);  // a lower priority after a higher one is ignored
    mgr.Update();
    CHECK(h->targetAngle() == 60.0f);
    // nothing queued: the field is left alone (values == 0 after the reset)
    h->Set<float>("targetAngle", 33.0f);
    mgr.Update();
    CHECK(h->targetAngle() == 33.0f);
    // an axis row of a controller routes through the manager when one exists
    ControlledAxis ax(&p, h, field, c1);
    ax.timeValue = FloatCurve(std::vector<Keyframe>{Keyframe(0.0f, 1.0f, 0.0f, 0.0f), Keyframe(1.0f, 1.0f, 0.0f, 0.0f)});
    ax.UpdateFieldValue(0.5f);
    CHECK(h->targetAngle() == 33.0f);  // queued, not applied yet
    mgr.Update();
    CHECK(h->targetAngle() == 1.0f);
    mgr.OnSceneLoad(GameScenes::FLIGHT);
    CHECK(mgr.QueueSize() == 0);
    mgr.OnDestroy();
    CHECK(RoboticControllerManager::Instance == nullptr);  // a destroyed UnityEngine.Object reads as null
    h->OnDestroy();
}

TEST_CASE("WP-52 controller persistence: OnSave / OnLoad round trip of the sequence state, the axis curves and the action times") {
    Flight f;
    Part& p1 = f.part();
    Part& p2 = *f.rig.AddPart(*f.one);
    p1.symmetryCounterparts = {&p2};
    p2.symmetryCounterparts = {&p1};
    RegisterPart(p1);
    RegisterPart(p2);
    auto* h1 = AddServo<ModuleRoboticServoHinge>(p1, "ModuleRoboticServoHinge");
    auto* h2 = AddServo<ModuleRoboticServoHinge>(p2, "ModuleRoboticServoHinge");
    auto* ctl = AddController(p1);
    ctl->Set<float>("sequenceLength", 8.0f);
    ctl->Set<int32_t>("sequencePlayingIndex", 0);
    ctl->Set<int32_t>("sequenceDirectionIndex", 1);
    ctl->Set<int32_t>("sequenceLoopIndex", 2);
    ctl->AddPartAxis(&p1, h1, Axis(*h1, "targetAngle"));
    ctl->controlledAxes[0]->timeValue = FloatCurve(std::vector<Keyframe>{Keyframe(0.0f, 1.0f, 0.0f, 2.0f), Keyframe(0.5f, 3.0f, 2.0f, -1.0f), Keyframe(1.0f, 0.25f, -1.0f, 0.0f)});
    ctl->controlledAxes[0]->SetPartNickName("Elbow");
    ctl->AddPartAction(&p1, h1, h1->Actions()["ToggleHingeAction"]);
    ctl->controlledActions[0]->times = {0.1f, 0.6f};
    ConfigNode node("MODULE");
    ctl->Save(node);
    CHECK(node.GetValue("sequenceIsPlaying").value() == "True");
    CHECK(node.GetValue("sequenceDirection").value() == "Reverse");
    CHECK(node.GetValue("sequenceLoopMode").value() == "PingPong");
    ConfigNode* axes = nullptr;
    REQUIRE(node.TryGetNode("CONTROLLEDAXES", axes));
    REQUIRE(axes->nodes().Count() == 1);
    CHECK(axes->nodes()[0]->name == "AXIS");
    CHECK(axes->nodes()[0]->GetValue("axisName").value() == "targetAngle");
    CHECK(axes->nodes()[0]->GetValue("partNickName").value() == "Elbow");
    ConfigNode* sym = nullptr;
    REQUIRE(axes->nodes()[0]->TryGetNode("SYMPARTS", sym));  // the partner part's id
    CHECK(sym->GetValue("symPersistentId").value() == std::to_string(p2.fields.persistentId));
    ConfigNode* curve = nullptr;
    REQUIRE(axes->nodes()[0]->TryGetNode("timeValueCurve", curve));
    CHECK(curve->GetValuesList("key").size() == 3);
    // a loaded copy with an out-of-range time added to the TIMES node: ControlledAction.OnLoad drops times outside 0..1
    ConfigNode* acts = nullptr;
    REQUIRE(node.TryGetNode("CONTROLLEDACTIONS", acts));
    ConfigNode* times = nullptr;
    REQUIRE(acts->nodes()[0]->TryGetNode("TIMES", times));
    times->AddValue("time", "1.5");
    times->AddValue("time", "-0.25");
    auto* ctl2 = AddController(p1);
    ctl2->Load(node);
    CHECK(ctl2->SequenceIsPlaying());
    CHECK(ctl2->SequenceDirection() == ModuleRoboticController_SequenceDirectionOptions::Reverse);
    CHECK(ctl2->SequenceLoop() == ModuleRoboticController_SequenceLoopOptions::PingPong);
    CHECK(ctl2->SequenceLength() == 8.0f);
    REQUIRE(ctl2->controlledAxes.size() == 1);
    REQUIRE(ctl2->controlledActions.size() == 1);
    CHECK(ctl2->controlledActions[0]->times == std::vector<float>{0.1f, 0.6f});
    const auto& want = ctl->controlledAxes[0]->timeValue.Curve().keys();
    const auto& got = ctl2->controlledAxes[0]->timeValue.Curve().keys();
    REQUIRE(got.size() == want.size());
    for (size_t i = 0; i < got.size(); i++) {
        CHECK(Same(got[i].time, want[i].time));
        CHECK(Same(got[i].value, want[i].value));
        CHECK(Same(got[i].inTangent, want[i].inTangent));
        CHECK(Same(got[i].outTangent, want[i].outTangent));
    }
    CHECK(ctl2->controlledAxes[0]->Controller == ctl2);
    CHECK(ctl2->controlledAxes[0]->PartNickName() == "Elbow");
    // OnStart binds the rows by id (AssignReferenceVars): the field of the part found by persistentId, the module by its persistent id
    CHECK(ctl2->controlledAxes[0]->AssignReferenceVars());
    CHECK(ctl2->controlledAxes[0]->AxisField == Axis(*h1, "targetAngle"));
    CHECK(ctl2->controlledAxes[0]->Module() == h1);
    CHECK(ctl2->controlledAxes[0]->SymmetryFields.size() == 1);
    CHECK(ctl2->controlledActions[0]->AssignReferenceVars());
    CHECK(ctl2->controlledActions[0]->Action == h1->Actions()["ToggleHingeAction"]);
    // unbindable rows are dropped by OnStart: a row whose part is gone
    ctl2->controlledAxes[0]->partId = 424242;
    LogCapture logs;
    ctl2->AssignReferenceVars_Debug();
    CHECK(ctl2->controlledAxes.empty());
    CHECK(logs.Has(log::Level::Warning, "Removing the axis we couldnt bind"));
    h1->OnDestroy();
    h2->OnDestroy();
}

TEST_CASE("WP-52 controller: SetLength rescales the position, quirks of SequencePlay / SequenceStop events, NextKey's action loop") {
    Flight f;
    Part& p = f.part();
    auto* ctl = AddController(p);
    ctl->hasEnoughResources = true;
    ctl->sequenceAxisPositionAxis = Axis(*ctl, "sequencePosition");
    ctl->Set<float>("sequenceLength", 10.0f);
    ctl->Set<float>("sequencePosition", 4.0f);
    ctl->SetLength(20.0f);
    CHECK(ctl->SequencePosition() == F32(fx(4.0f) / fx(10.0f) * fx(20.0f)));
    CHECK(ctl->sequenceAxisPositionAxis->maxValue == 20.0f);
    ctl->SetLength(0.0f);  // below 0.01 -> 0.01
    CHECK(ctl->SequenceLength() == 0.01f);
    ctl->Set<float>("sequenceLength", 10.0f);
    // the played / stopped events fire when the state did NOT change (cs:557-580)
    int played = 0, stopped = 0;
    struct L {
        int *p, *s;
        void P(ModuleRoboticController*) { (*p)++; }
        void S(ModuleRoboticController*) { (*s)++; }
    } l{&played, &stopped};
    GameEvents::onRoboticControllerSequencePlayed.Add(&l, &L::P);
    GameEvents::onRoboticControllerSequenceStopped.Add(&l, &L::S);
    ctl->SequencePlay();   // was stopped: no event
    CHECK(played == 0);
    ctl->SequencePlay();   // was playing: event
    CHECK(played == 1);
    ctl->SequenceStop();   // was playing: no event
    CHECK(stopped == 0);
    ctl->SequenceStop();   // was stopped: event
    CHECK(stopped == 1);
    GameEvents::onRoboticControllerSequencePlayed.Remove(&l, &L::P);
    GameEvents::onRoboticControllerSequenceStopped.Remove(&l, &L::S);
    // OnceRestart restarts from the beginning when played at the end
    ctl->Set<int32_t>("sequenceLoopIndex", 3);
    ctl->Set<float>("sequencePosition", 10.0f);
    ctl->SequencePlay();
    CHECK(ctl->SequencePosition() == 0.0f);
    // SetSequencePositionNextKey: the axis keys use `> position`, the ACTION loop uses `< position` (sic, cs:858)
    auto* h = AddServo<ModuleRoboticServoHinge>(p, "ModuleRoboticServoHinge");
    ctl->AddPartAxis(&p, h, Axis(*h, "targetAngle"));
    ctl->controlledAxes[0]->timeValue = FloatCurve(std::vector<Keyframe>{Keyframe(0.0f, 0.0f), Keyframe(0.5f, 1.0f), Keyframe(1.0f, 0.0f)});
    ctl->AddPartAction(&p, h, h->Actions()["MaximumAngleAction"]);
    ctl->controlledActions[0]->times = {0.2f, 0.9f};
    ctl->Set<float>("sequencePosition", 3.0f);
    ctl->SetSequencePositionNextKey();
    // axis key at 5.0 is the first one after 3.0; the action times (2.0 and 9.0): 2.0 < 3.0 lowers `a` to min(a, 2.0) = 2.0
    CHECK(ctl->SequencePosition() == 2.0f);
    // the previous-key search takes the largest key below the position among both lists
    ctl->Set<float>("sequencePosition", 6.0f);
    ctl->SetSequencePositionPrevKey();
    CHECK(ctl->SequencePosition() == 5.0f);
    h->OnDestroy();
}

TEST_CASE("WP-52 controller: CheckAndFireActions honours active, requireFullControl locks and the editor scene") {
    Flight f;
    Part& p = f.part();
    auto* ctl = AddController(p);
    ctl->hasEnoughResources = true;
    ctl->sequenceAxisPositionAxis = Axis(*ctl, "sequencePosition");
    std::vector<int> fired;
    std::vector<std::unique_ptr<BaseAction>> acts;
    for (int i = 0; i < 4; i++) {
        acts.push_back(std::make_unique<BaseAction>(nullptr, "a" + std::to_string(i), KSPActionAttribute{}));
        acts.back()->onEvent = [&fired, i](KSPActionParam&) { fired.push_back(i); };
        auto ca = std::make_shared<ControlledAction>(&p, nullptr, acts.back().get(), ctl);
        ca->times = {0.5f};
        ctl->controlledActions.push_back(ca);
    }
    acts[1]->active = false;
    acts[2]->requireFullControl = true;
    acts[3]->activeEditor = false;
    ctl->Set<float>("sequenceLength", 10.0f);
    auto fire = [&] {
        fired.clear();
        ctl->Set<float>("sequencePosition", 4.0f);
        ctl->SetSequencePosition(6.0f);  // the window [4, 6) contains 0.5 * 10
        ctl->CheckAndFireActions();
    };
    fire();
    // reverse list order (cs:504-511); the inactive action never fires; full control not locked -> 2 fires
    CHECK(fired == std::vector<int>{3, 2, 0});
    {
        InputLockManager::SetControlLock(ControlTypes::TWEAKABLES_FULLONLY, "test");
        fire();
        CHECK(fired == std::vector<int>{3, 0});  // requireFullControl action blocked
        InputLockManager::RemoveControlLock("test");
    }
    {
        SceneScope editor(GameScenes::EDITOR);
        fire();
        CHECK(fired == std::vector<int>{2, 0});  // activeEditor == false action skipped in the editor
    }
    // a window that does not contain the time fires nothing; Once mode also fires on the exact end time
    ctl->Set<float>("sequencePosition", 0.0f);
    fired.clear();
    ctl->SetSequencePosition(3.0f);
    ctl->CheckAndFireActions();
    CHECK(fired.empty());
    ctl->Set<float>("sequencePosition", 3.0f);
    ctl->SetSequencePosition(5.0f);  // end == 0.5 * 10 exactly: Once -> fires
    ctl->CheckAndFireActions();
    CHECK(fired == std::vector<int>{3, 2, 0});
    fired.clear();
    ctl->Set<int32_t>("sequenceLoopIndex", 1);  // Repeat: the half-open window excludes its end
    ctl->Set<float>("sequencePosition", 3.0f);
    ctl->SetSequencePosition(5.0f);
    ctl->CheckAndFireActions();
    CHECK(fired.empty());
}

TEST_CASE("WP-52 ControlledAxis curve helpers (ControlledAxis.cs:121-198) against hand-computed keys") {
    Flight f;
    Part& p = f.part();
    auto* h = AddServo<ModuleRoboticServoHinge>(p, "ModuleRoboticServoHinge");
    auto* ctl = AddController(p);
    BaseAxisField* field = Axis(*h, "hingeDamping");
    field->minValue = 0.0f;
    field->maxValue = 10.0f;
    ControlledAxis ax(&p, h, field, ctl);
    ax.timeValue = FloatCurve(std::vector<Keyframe>{Keyframe(0.0f, 1.0f, 0.0f, 2.0f), Keyframe(1.0f, 3.0f, 1.0f, 0.0f), Keyframe(2.0f, 5.0f, 3.0f, 4.0f)});
    // ReverseCurve: time = maxTime - t, tangents swap and negate
    ax.ReverseCurve();
    {
        const auto& k = ax.timeValue.Curve().keys();
        REQUIRE(k.size() == 3);
        CHECK(Same(k[0].time, 0.0f));
        CHECK(Same(k[0].value, 5.0f));
        CHECK(Same(k[0].inTangent, -4.0f));
        CHECK(Same(k[0].outTangent, -3.0f));
        CHECK(Same(k[1].time, 1.0f));
        CHECK(Same(k[1].value, 3.0f));
        CHECK(Same(k[1].inTangent, 0.0f));  // 0f - 0f is +0
        CHECK(Same(k[1].outTangent, -1.0f));
        CHECK(Same(k[2].time, 2.0f));
        CHECK(Same(k[2].value, 1.0f));
        CHECK(Same(k[2].inTangent, -2.0f));
        CHECK(Same(k[2].outTangent, 0.0f));
    }
    // InvertCurve: value -> max - (value - min), tangents negated
    ax.timeValue = FloatCurve(std::vector<Keyframe>{Keyframe(0.0f, 1.0f, 0.0f, 2.0f), Keyframe(1.0f, 3.0f, 1.0f, 0.0f), Keyframe(2.0f, 5.0f, 3.0f, 4.0f)});
    ax.InvertCurve();
    {
        const auto& k = ax.timeValue.Curve().keys();
        CHECK(Same(k[0].value, 9.0f));
        CHECK(Same(k[1].value, 7.0f));
        CHECK(Same(k[2].value, 5.0f));
        CHECK(Same(k[0].outTangent, -2.0f));
        CHECK(Same(k[2].inTangent, -3.0f));
    }
    // AlignCurveEnds: the last key takes the first key's value and out tangent as its in tangent
    ax.AlignCurveEnds();
    {
        const auto& k = ax.timeValue.Curve().keys();
        CHECK(Same(k[2].value, k[0].value));
        CHECK(Same(k[2].inTangent, k[0].outTangent));
    }
    // ClampAllPointValues / ClampPointValue to the axis range
    ax.timeValue = FloatCurve(std::vector<Keyframe>{Keyframe(0.0f, -4.0f, 0.0f, 0.0f), Keyframe(1.0f, 4.0f, 0.0f, 0.0f), Keyframe(2.0f, 40.0f, 0.0f, 0.0f)});
    ax.ClampPointValue(0);
    CHECK(Same(ax.timeValue.Curve()[0].value, 0.0f));
    CHECK(Same(ax.timeValue.Curve()[2].value, 40.0f));
    ax.ClampAllPointValues();
    CHECK(Same(ax.timeValue.Curve()[2].value, 10.0f));
    ax.ClampPointValue(-1);  // out of range: ignored
    CHECK_THROWS(ax.ClampPointValue(3));  // sic: `index <= length` lets index == length through to an IndexOutOfRange
    // RescaleCurveTime, ratio > 1: times scale, capped below maxTime by minSpace per remaining key
    ax.timeValue = FloatCurve(std::vector<Keyframe>{Keyframe(0.0f, 0.0f, 0.0f, 0.0f), Keyframe(1.0f, 1.0f, 0.0f, 0.0f), Keyframe(2.0f, 2.0f, 0.0f, 0.0f)});
    ax.RescaleCurveTime(1.5f, 0.01f);
    {
        const auto& k = ax.timeValue.Curve().keys();
        REQUIRE(k.size() == 3);
        CHECK(Same(k[1].time, 1.5f));
        CHECK(Same(k[2].time, F32(std::min(fx(F32(2.0 * 1.5)), 2.0 - fx(0.01f) * 0.0))));  // 3.0 capped to maxTime 2.0
    }
    // RescaleCurveTime, ratio < 1: times shrink and a closing key at the old maxTime is added with flat end tangents
    ax.timeValue = FloatCurve(std::vector<Keyframe>{Keyframe(0.0f, 0.0f, 1.0f, 1.0f), Keyframe(1.0f, 1.0f, 1.0f, 1.0f), Keyframe(2.0f, 2.0f, 1.0f, 1.0f)});
    ax.RescaleCurveTime(0.5f, 0.01f);
    {
        const auto& k = ax.timeValue.Curve().keys();
        REQUIRE(k.size() == 4);
        CHECK(Same(k[1].time, 0.5f));
        CHECK(Same(k[2].time, 1.0f));
        CHECK(Same(k[3].time, 2.0f));
        CHECK(Same(k[3].value, 2.0f));
        CHECK(Same(k[2].outTangent, 0.0f));
        CHECK(Same(k[3].inTangent, 0.0f));
    }
    // PresetFlat: a flat curve at half the soft range (the hinge limits entry of targetAngle) over the old maxTime
    ControlledAxis ta(&p, h, Axis(*h, "targetAngle"), ctl);
    ta.timeValue = FloatCurve(std::vector<Keyframe>{Keyframe(0.0f, 0.0f), Keyframe(7.0f, 0.0f)});
    ta.PresetFlat();
    CHECK(Same(ta.timeValue.Curve()[0].value, F32((fx(177.0f) - fx(0.0f)) / 2.0)));  // soft limits (0, 177)
    CHECK(Same(ta.timeValue.Curve()[1].time, 7.0f));
    // a preset with neither a supplied nor a cached reference logs and leaves the curve alone
    LogCapture logs;
    ControlledAxis fresh(&p, h, Axis(*h, "hingeDamping"), ctl);
    const auto before = fresh.timeValue.Curve().keys();
    fresh.UpdatePreset(2.0f, 0.0f);
    CHECK(logs.Has(log::Level::Error, "Unable to Generate curve"));  // lastUsedRefName is "" (not stockFlat): GenerateCurveFromReference logs
    CHECK(fresh.timeValue.Curve().keys().size() == before.size());
    h->OnDestroy();
}
