// WP-52 unit tests: the robotics controller (src/serenity/robotic_controller.cpp, controlled_axis.cpp): the sequencer against the independent
// Python reference (tools/wp52/servo_reference.py), 0 ULP. See tests/test_robotics.cpp for the gate description and the data caveat.
#include "robotics_fixture.hpp"

using namespace wp52t;

namespace {

struct ControllerLog {
    std::vector<std::string> ev;
    void Played(ModuleRoboticController*) { ev.push_back("played"); }
    void Stopped(ModuleRoboticController*) { ev.push_back("stopped"); }
    void Dir(ModuleRoboticController*, ModuleRoboticController_SequenceDirectionOptions d) { ev.push_back("dir:" + std::to_string(static_cast<int>(d))); }
    void Loop(ModuleRoboticController*, ModuleRoboticController_SequenceLoopOptions l) { ev.push_back("loop:" + std::to_string(static_cast<int>(l))); }
    ControllerLog() {
        GameEvents::onRoboticControllerSequencePlayed.Add(this, &ControllerLog::Played);
        GameEvents::onRoboticControllerSequenceStopped.Add(this, &ControllerLog::Stopped);
        GameEvents::onRoboticControllerSequenceDirectionChanged.Add(this, &ControllerLog::Dir);
        GameEvents::onRoboticControllerSequenceLoopModeChanged.Add(this, &ControllerLog::Loop);
    }
    ~ControllerLog() {
        GameEvents::onRoboticControllerSequencePlayed.Remove(this, &ControllerLog::Played);
        GameEvents::onRoboticControllerSequenceStopped.Remove(this, &ControllerLog::Stopped);
        GameEvents::onRoboticControllerSequenceDirectionChanged.Remove(this, &ControllerLog::Dir);
        GameEvents::onRoboticControllerSequenceLoopModeChanged.Remove(this, &ControllerLog::Loop);
    }
};

void RunControllerScenario(Rig& rig, const json& sc, size_t idx, Diff& d, long& ticks) {
    const std::string name = "controller" + std::to_string(idx);
    INFO(name);
    auto one = rig.MakeOne();
    Part& p = *one->part;
    ModuleRoboticController* ctl = AddController(p);
    ModuleRoboticController* ctl2 = AddController(p);
    ModuleRoboticServoHinge* hinge = AddServo<ModuleRoboticServoHinge>(p, "ModuleRoboticServoHinge");
    ctl->Set<float>("sequenceLength", J(sc["length"]));
    ctl->Set<float>("sequencePosition", J(sc["position"]));
    ctl->Set<float>("sequencePlaySpeed", J(sc["speed"]));
    ctl->Set<int32_t>("sequencePlayingIndex", sc["playing"].get<bool>() ? 0 : 1);
    ctl->Set<int32_t>("sequenceDirectionIndex", sc["direction"].get<int>());
    ctl->Set<int32_t>("sequenceLoopIndex", sc["loop"].get<int>());
    ctl->Set<bool>("controllerEnabled", sc["enabled"].get<bool>());
    ctl->hasEnoughResources = true;
    ctl->sequenceAxisPositionAxis = dynamic_cast<BaseAxisField*>(ctl->Fields()["sequencePosition"]);
    Time::deltaTime = J(sc["dt"]);
    // actions: plain BaseActions that record their index
    std::vector<int> fired;
    std::vector<std::unique_ptr<BaseAction>> actions;
    for (size_t i = 0; i < sc["actions"].size(); i++) {
        actions.push_back(std::make_unique<BaseAction>(nullptr, "act" + std::to_string(i), KSPActionAttribute{}));
        const int id = static_cast<int>(i);
        actions.back()->onEvent = [&fired, id](KSPActionParam&) { fired.push_back(id); };
        auto ca = std::make_shared<ControlledAction>(&p, nullptr, actions.back().get(), ctl);
        for (const json& t : sc["actions"][i]) ca->times.push_back(J(t));
        ctl->controlledActions.push_back(ca);
    }
    // axes: a hinge axis field (multiplier 1), or the sequencePlaySpeed / sequencePosition of the second controller (multipliers 100 / its length)
    const char* hingeFields[] = {"hingeDamping", "traverseVelocity", "targetAngle"};
    int nHinge = 0;
    std::vector<BaseAxisField*> targets;
    for (const json& ax : sc["axes"]) {
        BaseAxisField* f = nullptr;
        PartModule* m = hinge;
        if (ax["mult"].is_null()) {
            f = dynamic_cast<BaseAxisField*>(hinge->Fields()[hingeFields[nHinge++]]);
        } else if (J(ax["mult"]) == 100.0f) {
            f = dynamic_cast<BaseAxisField*>(ctl2->Fields()["sequencePlaySpeed"]);
            m = ctl2;
        } else {
            f = dynamic_cast<BaseAxisField*>(ctl2->Fields()["sequencePosition"]);
            ctl2->Set<float>("sequenceLength", J(ax["mult"]));
            m = ctl2;
        }
        REQUIRE(f != nullptr);
        auto ca = std::make_shared<ControlledAxis>(&p, m, f, ctl);
        const float mt = J(ax["maxTime"]);
        ca->timeValue = FloatCurve(std::vector<Keyframe>{Keyframe(0.0f, 0.0f, 1.0f, 1.0f), Keyframe(mt, mt, 1.0f, 1.0f)});
        ctl->controlledAxes.push_back(ca);
        targets.push_back(f);
    }
    ControllerLog log;
    const int nTicks = sc["ticks"].get<int>();
    for (int t = 0; t < nTicks; t++) {
        for (const json& op : sc["ops"]) {
            if (op[0].get<int>() != t) continue;
            const std::string k = op[1].get<std::string>();
            const float arg = J(op[2]);
            if (k == "toggle_dir") ctl->ToggleDirection();
            else if (k == "cycle_loop") ctl->CycleLoopMode();
            else if (k == "play") ctl->SequencePlay();
            else if (k == "stop") ctl->SequenceStop();
            else if (k == "set_pos") ctl->SetSequencePosition(arg);
            else if (k == "set_len") ctl->SetLength(arg);
            else if (k == "set_speed") ctl->SetPlaySpeed(arg);
            else if (k == "start") ctl->SetSequencePositionStart();
            else if (k == "end") ctl->SetSequencePositionEnd();
            else if (k == "forward") ctl->SetDirection(ModuleRoboticController_SequenceDirectionOptions::Forward);
            else if (k == "reverse") ctl->SetDirection(ModuleRoboticController_SequenceDirectionOptions::Reverse);
            else FAIL("unknown op " << k);
        }
        ctl->OnUpdate();
        const json& e = sc["expected"][t];
        const std::string w = name + " t" + std::to_string(t) + " ";
        d.CheckF(ctl->SequencePosition(), J(e["position"]), w + "position");
        d.CheckF(ctl->SequenceLength(), J(e["length"]), w + "length");
        d.CheckF(static_cast<float>(static_cast<int>(ctl->SequenceDirection())), J(e["direction"]), w + "direction");
        d.CheckF(static_cast<float>(static_cast<int>(ctl->SequenceLoop())), J(e["loop"]), w + "loop");
        d.CheckF(B(ctl->SequenceIsPlaying()), J(e["playing"]), w + "playing");
        d.CheckF(ctl->actionCheck1StartTime, J(e["a1s"]), w + "a1s");
        d.CheckF(ctl->actionCheck1EndTime, J(e["a1e"]), w + "a1e");
        d.CheckF(ctl->actionCheck2StartTime, J(e["a2s"]), w + "a2s");
        d.CheckF(ctl->actionCheck2EndTime, J(e["a2e"]), w + "a2e");
        d.CheckF(ctl->SequencePlaySpeed(), J(e["speed"]), w + "speed");
        d.CheckS(std::to_string(fired.size()), std::to_string(e["fired"].size()), w + "fired count");
        for (size_t i = 0; i < std::min(fired.size(), e["fired"].size()); i++) d.CheckS(std::to_string(fired[i]), std::to_string(e["fired"][i].get<int>()), w + "fired");
        d.CheckS(std::to_string(log.ev.size()), std::to_string(e["events"].size()), w + "event count");
        for (size_t i = 0; i < std::min(log.ev.size(), e["events"].size()); i++) d.CheckS(log.ev[i], e["events"][i].get<std::string>(), w + "event");
        for (size_t i = 0; i < sc["axes"].size(); i++) {
            if (e["axisvals"][i].is_null()) continue;
            bool overwritten = false;  // a later axis of the same field wins (SetValue order)
            for (size_t j = i + 1; j < targets.size(); j++) overwritten = overwritten || targets[j] == targets[i];
            if (overwritten) continue;
            // the curve is the identity (keys (0,0,1,1) and (maxTime,maxTime,1,1)), so Evaluate(t) == t up to the native cubic's own rounding (<= 4 ULP;
            // AnimationCurve.Evaluate is native and not part of the 0-ULP gate) and the field gets F(t * multiplier)
            const float tv = J(e["axisvals"][i][0]), mult = J(e["axisvals"][i][1]);
            const float want = f32store(fx(tv) * fx(mult));
            const float got = std::get<float>(targets[i]->GetValue());
            d.CheckB(std::llabs(UlpDiff(got, want)) <= 4, true, w + "axis" + std::to_string(i) + " got " + std::to_string(got) + " want " + std::to_string(want));
        }
        fired.clear();
        log.ev.clear();
        ticks++;
    }
}

}  // namespace

TEST_CASE("WP-52 controller sequencer over 60 scripted runs (loop modes, direction, actions, axes) vs the independent Python reference, 0 ULP") {
    const json& ref = LoadRef52();
    REQUIRE(GameAvailable());
    Rig rig;
    TimeWarp::fetch = &rig.env.tw;
    rig.env.tw.current_rate_index = 0;
    Diff d;
    long ticks = 0;
    size_t idx = 0;
    for (const json& sc : ref["controller"]) RunControllerScenario(rig, sc, idx++, d, ticks);
    INFO(d.first);
    CHECK(d.bad == 0);
    CHECK(d.total > 20000);
    CHECK(ticks == 60 * 60);
}
