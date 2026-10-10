// WP-24 unit tests: command, SAS / autopilot, reaction wheels, control state (ModuleCommand.cs, ModuleSAS.cs, ModuleReactionWheel.cs,
// ModuleRemoteController.cs, VesselAutopilot.cs, ControlPoint.cs, SteeringObject.cs, FlightCtrlState.cs).
//
// No golden or capture of these classes' runtime behaviour exists (harness/golden/MANIFEST.json and harness/captures/out*/ carry no SAS,
// command or reaction wheel data), so every expectation is derived: from the decompiled C# (the derivation is in the comment above the
// check), from the game's own data (the part cfgs: minimumCrew, requiresPilot, hibernation, the reaction wheel torques and ElectricCharge
// rates) or from tools/wp24/sas_reference.py (an independent numpy float32 implementation of the VesselSAS controller replay and of the
// reaction wheel torque / resource maths; tests/data/wp24_reference.json). The dynamic inputs of the replays (attitude trace, angular
// velocity, pilot input) are recorded in that file; the reference and the port see the same values. Quaternion.eulerAngles is native Unity
// code that kspcore does not have a capture for: both sides use the same documented formulation (STATUS.md WP-24).
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/modules/control_types.hpp"
#include "ksp/modules/module_command.hpp"
#include "ksp/modules/module_reaction_wheel.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/vessel/flight_ctrl_state.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_autopilot.hpp"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using GE = events::GameEvents;
using Localizer24 = KSP::Localization::Localizer;
using json = nlohmann::json;
using PCM = std::shared_ptr<ProtoCrewMember>;

namespace {

std::string GameDataDir24() { return ksp::testing::GameRoot() + "/GameData"; }

json LoadRef24() {
    std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp24_reference.json", std::ios::binary);
    REQUIRE(f.good());
    return json::parse(f);
}

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
int64_t UlpF(float a, float b) {
    int32_t x, y;
    std::memcpy(&x, &a, 4);
    std::memcpy(&y, &b, 4);
    if (x < 0) x = INT32_MIN - x;
    if (y < 0) y = INT32_MIN - y;
    return x > y ? static_cast<int64_t>(x) - y : static_cast<int64_t>(y) - x;
}
int64_t UlpD(double a, double b) {
    int64_t x, y;
    std::memcpy(&x, &a, 8);
    std::memcpy(&y, &b, 8);
    if (x < 0) x = INT64_MIN - x;
    if (y < 0) y = INT64_MIN - y;
    return x > y ? x - y : y - x;
}

// the max ULP distance seen over a replay
struct UlpTally {
    int64_t maxF = 0, maxD = 0;
    long values = 0;
    void F(float got, double expectedFloatValue) {
        maxF = std::max(maxF, UlpF(got, static_cast<float>(expectedFloatValue)));
        values++;
    }
    void D(double got, double expected) {
        maxD = std::max(maxD, UlpD(got, expected));
        values++;
    }
};

void LoadLocalizer24() {
    Localizer24::Shutdown();
    Localizer24::Init();
    ConfigNode::NodePtr root = ConfigNode::Load(GameDataDir24() + "/Squad/Localization/dictionary.cfg");
    REQUIRE(root);
    std::vector<ConfigNode*> nodes{root->GetNode("Localization")};
    Localizer24::Instance()->AddTagValuesForLanguage(nodes, "en-us");
}

// The stock game + the Squad Traits.cfg (the Pilot trait carries FullVesselControlSkill) + a sandbox game
struct World24 {
    Game15 g;
    Game game;
    World24() {
        FlightRegistry::Clear();
        SaveEnvironment& env = SaveEnvironment::Current();
        Vessel::env = VesselEnvironment{};
        LoadLocalizer24();
        std::vector<const ConfigNode*> traits;
        for (ConfigNode* n : g.data.database.GetConfigNodes("EXPERIENCE_TRAIT")) traits.push_back(n);
        REQUIRE(traits.size() == 4);
        env.experienceConfigs = std::make_shared<Experience::ExperienceSystemConfig>(traits);
        game.Mode = Game_Modes::SANDBOX;
        game.CrewRoster = std::make_shared<KerbalRoster>(Game_Modes::SANDBOX);
        game.Parameters->CustomParams<GameParameters::AdvancedParams>().EnableFullSASInSandbox = true;
        env.currentGame = &game;
        AutopilotEnvironment::Reset();
        CommandEnvironment::Reset();
        ReactionWheelEnvironment::Reset();
    }
    ~World24() {
        AutopilotEnvironment::Reset();
        CommandEnvironment::Reset();
        ReactionWheelEnvironment::Reset();
        SaveEnvironment::Current().currentGame = nullptr;
        FlightRegistry::Clear();
        TimeWarp::fetch = nullptr;
        Vessel::env = VesselEnvironment{};
    }
    PCM Kerbal(const char* trait, ProtoCrewMember::KerbalType type = ProtoCrewMember::KerbalType::Crew) {
        PCM pcm = std::make_shared<ProtoCrewMember>(type, "Testy Kerman");
        if (trait != nullptr) {
            pcm->trait = trait;
            for (const auto& cfg : SaveEnvironment::Current().experienceConfigs->Categories())
                if (cfg->Name() == trait) pcm->experienceTrait = Experience::ExperienceTrait::Create(cfg->Name(), cfg.get(), pcm.get());
            REQUIRE(pcm->experienceTrait);
        }
        return pcm;
    }
};

// A bare vessel owning no parts (the parts belong to the test) plus the resource sets of its parts (Vessel.UpdateResourceSets)
struct TestVesselHost final : IResourceVessel {
    Vessel* v;
    VesselResourceSets sets;
    explicit TestVesselHost(Vessel* vessel) : v(vessel) {
        ResourceFlowEnvironment::vesselHost = [this](Vessel* x) -> IResourceVessel* { return x == v ? this : nullptr; };
    }
    ~TestVesselHost() override {
        ResourceFlowEnvironment::vesselHost = nullptr;
        for (Part* p : v->parts) {
            p->crossfeedPartSet.reset();
            p->simulationCrossfeedPartSet.reset();
        }
    }
    void Update() { sets.UpdateResourceSets(v, v->parts); }
    const std::vector<Part*>& ResourceParts() const override { return v->parts; }
    double RequestResource(Part* part, int id, double demand, bool usePriority, bool simulate) override {
        return sets.RequestResource(part, id, demand, usePriority, simulate);
    }
    void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, bool pulling) override {
        sets.GetConnectedResourceTotals(id, simulate, amount, maxAmount, pulling);
    }
    void GetConnectedResourceTotals(int id, bool simulate, double& amount, double& maxAmount, double threshold, bool pulling) override {
        sets.GetConnectedResourceTotals(id, simulate, amount, maxAmount, threshold, pulling);
    }
};

struct OneVessel {
    std::unique_ptr<Vessel> v = std::make_unique<Vessel>();
    std::unique_ptr<TestVesselHost> host;
    std::vector<Part*> parts;
    explicit OneVessel(std::vector<Part*> ps) : parts(std::move(ps)) {
        for (Part* p : parts) {
            v->parts.push_back(p);
            p->vessel = v.get();
        }
        host = std::make_unique<TestVesselHost>(v.get());
        host->Update();
    }
    ~OneVessel() {
        host.reset();
        for (Part* p : parts) p->vessel = nullptr;
        v->parts.clear();
    }
};

template <class T>
T* Find(Part& p, size_t index = 0) {
    for (size_t i = 0; i < p.Modules().Count(); i++)
        if (auto* m = dynamic_cast<T*>(p.Modules()[i]))
            if (index-- == 0) return m;
    return nullptr;
}

const PartResourceDefinition* EC() { return PartResourceLibrary::Instance()->GetDefinition(std::string_view("ElectricCharge")); }

void FillEC(Part& p, double amount) {
    PartResource* r = p.GetResource("ElectricCharge");
    if (r == nullptr) return;
    r->amount = amount;
    p.ResetSimulationResources();
}

std::string PartNameOf(const std::string& cfgName) {
    std::string n = cfgName;
    std::replace(n.begin(), n.end(), '_', '.');  // PartLoader: '_' in a part name becomes '.'
    return n;
}

}  // namespace

// ============================================================ the real classes on the stock parts

TEST_CASE("WP-24 every stock ModuleCommand / ModuleReactionWheel / ModuleSAS / ModuleRemoteController is the real class with its cfg state") {
    if (!GameAvailable()) return;
    World24 w;
    int cmds = 0, wheels = 0, sass = 0, remotes = 0, withControlPoints = 0;
    for (const auto& apu : w.g.data.parts.loadedParts) {
        const AvailablePart* ap = apu.get();
        if (!ap->partPrefab) continue;
        for (const PartModuleData& d : ap->partPrefab->ModuleData) {
            INFO(ap->name);
            if (auto* c = dynamic_cast<ModuleCommand*>(d.module.get())) {
                cmds++;
                // ModuleCommand.OnAwake (ModuleCommand.cs:366): isControlSource = FULL on the prefab
                CHECK(ap->partPrefab->fields.isControlSource == static_cast<int>(ControlLevel::FULL));
                const std::optional<std::string> mc = d.config->GetValue("minimumCrew");
                CHECK(c->Get<int>("minimumCrew") == (mc ? std::stoi(*mc) : 0));
                // The prefab is Awake-d before Load (PartModule.Awake then Load): its consumedResources / controlPoints are still empty there;
                // the instances (Instantiate copies the serialized resHandler / controlPointsList first) are checked below
                const std::vector<ConfigNode*> cps = d.config->GetNodes("CONTROLPOINT");
                CHECK(c->controlPointsList.has_value() == !cps.empty());
                if (!cps.empty()) withControlPoints++;
                CHECK(c->ModuleState() == ModuleCommand::ModuleControlState::NotEnoughCrew);
            } else if (auto* r = dynamic_cast<ModuleReactionWheel*>(d.module.get())) {
                wheels++;
                CHECK(r->State() == ModuleReactionWheel::WheelState::Active);
                CHECK(r->operational);
                for (const char* t : {"PitchTorque", "YawTorque", "RollTorque"}) {
                    const std::optional<std::string> s = d.config->GetValue(t);
                    CHECK(r->Get<float>(t) == (s ? std::strtof(s->c_str(), nullptr) : 5.0f));
                }
            } else if (auto* s = dynamic_cast<ModuleSAS*>(d.module.get())) {
                sass++;
                const std::optional<std::string> l = d.config->GetValue("SASServiceLevel");
                CHECK(s->Get<int>("SASServiceLevel") == (l ? std::stoi(*l) : 0));
            } else if (dynamic_cast<ModuleRemoteController*>(d.module.get())) {
                remotes++;
            }
        }
    }
    CHECK(cmds >= 25);
    CHECK(wheels >= 20);
    CHECK(sass >= 6);
    CHECK(withControlPoints >= 1);
    // the instances: consumedResources one to one with the input resources, the control point dictionary from the cfg nodes
    int instances = 0;
    for (const auto& apu : w.g.data.parts.loadedParts) {
        const AvailablePart* ap = apu.get();
        if (!ap->partPrefab) continue;
        for (const PartModuleData& d : ap->partPrefab->ModuleData) {
            if (dynamic_cast<ModuleCommand*>(d.module.get()) == nullptr && dynamic_cast<ModuleReactionWheel*>(d.module.get()) == nullptr) continue;
            INFO(ap->name);
            auto part = w.g.New(ap->name.c_str());
            ModuleCommand* c = Find<ModuleCommand>(*part);
            if (c != nullptr) {
                instances++;
                CHECK(part->fields.isControlSource == static_cast<int>(ControlLevel::FULL));
                REQUIRE(c->consumedResources.size() == c->resHandler.inputResources.size());
                for (size_t i = 0; i < c->consumedResources.size(); i++) CHECK(c->consumedResources[i] == EC());
                for (const PartModuleData& d2 : ap->partPrefab->ModuleData) {
                    if (dynamic_cast<ModuleCommand*>(d2.module.get()) == nullptr) continue;
                    const std::vector<ConfigNode*> cps = d2.config->GetNodes("CONTROLPOINT");
                    CHECK(c->ControlPointsExist() == !cps.empty());
                    if (!cps.empty()) {
                        REQUIRE(c->controlPoints.has_value());
                        REQUIRE(c->controlPoints->Count() == static_cast<int>(cps.size()) + 1);
                        CHECK(c->controlPoints->KeysList()[0] == "_default");
                        for (size_t i = 0; i < cps.size(); i++) {
                            ControlPoint* cp = c->GetControlPoint(*cps[i]->GetValuePtr("name"));
                            REQUIRE(cp != nullptr);
                            CHECK(cp->displayName == *cps[i]->GetValuePtr("displayName"));
                            const Vector3f o = ConfigNode::ParseVector3(*cps[i]->GetValuePtr("orientation"));
                            CHECK(Bits(cp->orientation.x) == Bits(o.x));
                            CHECK(Bits(cp->orientation.y) == Bits(o.y));
                            CHECK(Bits(cp->orientation.z) == Bits(o.z));
                        }
                    }
                    break;
                }
            }
            if (ModuleReactionWheel* r = Find<ModuleReactionWheel>(*part)) {
                REQUIRE(r->consumedResources.size() == r->resHandler.inputResources.size());
                for (size_t i = 0; i < r->consumedResources.size(); i++) CHECK(r->consumedResources[i] == EC());
            }
            break;
        }
    }
    CHECK(instances >= 25);
    MESSAGE("stock modules: ModuleCommand " << cmds << " (" << withControlPoints << " with CONTROLPOINT), ModuleReactionWheel " << wheels
                                           << ", ModuleSAS " << sass << ", ModuleRemoteController " << remotes);
}

// ============================================================ ModuleCommand control source state

namespace {
struct Expect {
    ModuleCommand::ModuleControlState state;
    VesselControlState vcs;
    ControlLevel level;
};

// The decision table of ModuleCommand.UpdateControlSourceState (ModuleCommand.cs:488-612) written out for the inputs a test controls.
// crew: non-tourist crew count aboard, tourists: tourist count, pilots: crew with FullVesselControlSkill; every crew member active.
struct Scene {
    bool resources = true;
    bool hibernating = false;
    int crew = 0, tourists = 0, pilots = 0;
    bool commNet = false, connected = false, signalRequired = false;
};
Expect Oracle(ModuleCommand& c, int crewCapacity, const Scene& s) {
    using MS = ModuleCommand::ModuleControlState;
    const int minimumCrew = c.Get<int>("minimumCrew");
    const bool remoteControl = c.Get<bool>("remoteControl"), requiresTelemetry = c.Get<bool>("requiresTelemetry");
    const bool flag = c.Get<bool>("requiresPilot") && s.pilots == 0 && crewCapacity > 0;
    auto level = [](VesselControlState v) {
        if ((v & VesselControlState::Full) > VesselControlState::None) return ControlLevel::FULL;
        if ((v & VesselControlState::Partial) > VesselControlState::None)
            return (v & VesselControlState::Kerbal) > VesselControlState::None ? ControlLevel::PARTIAL_MANNED : ControlLevel::PARTIAL_UNMANNED;
        return ControlLevel::NONE;
    };
    auto make = [&](MS m, VesselControlState v) { return Expect{m, v, level(v)}; };
    if (!s.resources) return make(MS::NotEnoughResources, minimumCrew > 0 ? VesselControlState::Kerbal : VesselControlState::Probe);
    if (s.crew < minimumCrew) {
        if (s.crew + s.tourists > s.crew) return make(MS::TouristCrew, VesselControlState::Kerbal);   // totalCrewCount (non tourist) < crew list size
        return make(MS::NotEnoughCrew, VesselControlState::Kerbal);
    }
    if (s.commNet) {
        if (!remoteControl && flag) return make(MS::PartialManned, VesselControlState::KerbalPartial);
        if (!s.connected) {
            if (flag) return make(MS::PartialManned, VesselControlState::KerbalPartial);
            if (s.crew == 0) {
                if (s.signalRequired || !remoteControl) return make(MS::NoControlPoint, VesselControlState::Probe);
                if (requiresTelemetry) return make(MS::PartialProbe, VesselControlState::ProbePartial);
            }
        }
    }
    if (s.hibernating) return minimumCrew > 0 ? make(MS::PartialManned, VesselControlState::KerbalPartial) : make(MS::PartialProbe, VesselControlState::ProbePartial);
    return minimumCrew > 0 ? make(MS::Nominal, VesselControlState::KerbalFull) : make(MS::Nominal, VesselControlState::ProbeFull);
}
}  // namespace

TEST_CASE("WP-24 ModuleCommand control source state on every stock command part: resources, crew, tourists, pilots, hibernation, CommNet") {
    if (!GameAvailable()) return;
    World24 w;
    SceneScope flight(GameScenes::FLIGHT);
    int parts = 0, scenes = 0;
    for (const auto& apu : w.g.data.parts.loadedParts) {
        const AvailablePart* ap = apu.get();
        if (!ap->partPrefab || Find<ModuleCommand>(*ap->partPrefab) == nullptr && true) {
            bool has = false;
            if (ap->partPrefab)
                for (const PartModuleData& d : ap->partPrefab->ModuleData) has = has || dynamic_cast<ModuleCommand*>(d.module.get()) != nullptr;
            if (!has) continue;
        }
        INFO(ap->name);
        auto part = w.g.New(ap->name.c_str());
        ModuleCommand* c = Find<ModuleCommand>(*part);
        REQUIRE(c != nullptr);
        OneVessel ov({part.get()});
        c->OnStart(StartState::Flying);
        parts++;
        const int capacity = part->fields.CrewCapacity;
        const int minimumCrew = c->Get<int>("minimumCrew");
        const bool hasInput = !c->resHandler.inputResources.empty();
        const bool hasHibernation = c->Get<bool>("hasHibernation");

        auto run = [&](const Scene& s, bool expectProtoState = true) {
            // crew aboard
            part->protoModuleCrew.clear();
            for (int i = 0; i < s.pilots; i++) part->protoModuleCrew.push_back(w.Kerbal("Pilot"));
            for (int i = 0; i < s.crew - s.pilots; i++) part->protoModuleCrew.push_back(w.Kerbal("Engineer"));
            for (int i = 0; i < s.tourists; i++) part->protoModuleCrew.push_back(w.Kerbal("Tourist", ProtoCrewMember::KerbalType::Tourist));
            c->Set<bool>("hibernation", s.hibernating);
            FillEC(*part, s.resources ? 1000.0 : 0.0);
            const bool supplied = s.resources || !hasInput;
            Scene eff = s;
            eff.resources = supplied;
            CommandEnvironment::commNetEnabled = [&] { return s.commNet; };
            CommandEnvironment::connection = [&](Vessel&) -> std::optional<CommNetLinkState> {
                if (!s.connected) return std::nullopt;
                return CommNetLinkState{true, 1.0, 100.0};
            };
            w.game.Parameters->CustomParams<CommNet::CommNetParams>().requireSignalForControl = s.signalRequired;
            const Expect e = Oracle(*c, capacity, eff);
            c->UpdateControlState();
            CHECK(c->ModuleState() == e.state);
            CHECK(c->GetControlSourceState() == e.vcs);
            CHECK(part->fields.isControlSource == static_cast<int>(e.level));
            CHECK(c->crewCount() == (supplied ? s.crew : 0) );
            scenes++;
            (void)expectProtoState;
        };

        // resources
        if (hasInput) run({false});
        // no crew
        run({true});
        // crewed to the minimum, no pilot / pilots
        if (minimumCrew > 0 && minimumCrew <= capacity) {
            run({true, false, minimumCrew, 0, 0});
            run({true, false, minimumCrew, 0, 1});
            // too few crew; a tourist in the seat instead
            run({true, false, minimumCrew - 1, 0, 0});
            if (minimumCrew - 1 + 1 <= capacity) run({true, false, minimumCrew - 1, 1, 0});
        }
        // hibernation (the multiplier is a KSPField; hasHibernation parts show the toggle)
        if (hasHibernation) {
            run({true, true, minimumCrew, 0, 0});
            run({true, true, 0, 0, 0});
        }
        // CommNet on: no link / link, signal required or not, crewed and crewless
        for (bool connected : {false, true})
            for (bool required : {false, true}) {
                run({true, false, 0, 0, 0, true, connected, required});
                if (minimumCrew > 0 && minimumCrew <= capacity) {
                    run({true, false, minimumCrew, 0, 0, true, connected, required});
                    run({true, false, minimumCrew, 0, 1, true, connected, required});
                }
            }
        w.game.Parameters->CustomParams<CommNet::CommNetParams>().requireSignalForControl = false;
        part->protoModuleCrew.clear();
    }
    CHECK(parts >= 25);
    MESSAGE("command parts " << parts << ", scenes " << scenes);
}

TEST_CASE("WP-24 ModuleCommand resource draw: full rate, hibernation multiplier, request = rate * multiplier * fixedDeltaTime") {
    if (!GameAvailable()) return;
    World24 w;
    SceneScope flight(GameScenes::FLIGHT);
    int checked = 0;
    for (const char* name : {"probeCoreOcto.v2", "HECS2.ProbeCore", "probeCoreHex.v2", "probeStackLarge"}) {
        const AvailablePart* ap = w.g.data.parts.getPartInfoByName(name);
        if (ap == nullptr) continue;
        INFO(name);
        auto part = w.g.New(name);
        ModuleCommand* c = Find<ModuleCommand>(*part);
        REQUIRE(c != nullptr);
        REQUIRE(c->resHandler.inputResources.size() == 1);
        OneVessel ov({part.get()});
        const double rate = c->resHandler.inputResources[0]->rate;
        const double fdt = static_cast<double>(Time::fixedDeltaTime);
        FillEC(*part, 1000.0);
        const double a0 = part->GetResource("ElectricCharge")->amount;
        c->UpdateControlSourceState();
        const double a1 = part->GetResource("ElectricCharge")->amount;
        CHECK(c->resHandler.inputResources[0]->currentRequest == rate * 1.0 * fdt);
        CHECK(a0 - a1 == doctest::Approx(rate * fdt).epsilon(1e-9));
        if (c->Get<bool>("hasHibernation")) {
            c->Set<bool>("hibernation", true);
            const double mult = c->Get<double>("hibernationMultiplier");
            c->UpdateControlSourceState();
            CHECK(c->resHandler.inputResources[0]->currentRequest == rate * mult * fdt);
            CHECK(c->ModuleState() == (c->Get<int>("minimumCrew") > 0 ? ModuleCommand::ModuleControlState::PartialManned
                                                                       : ModuleCommand::ModuleControlState::PartialProbe));
        }
        checked++;
    }
    CHECK(checked >= 2);
}

TEST_CASE("WP-24 ModuleCommand IsHibernating (hibernation, hibernateOnWarp with the warp rate and Part.packed), HibernateToggle, UpdateControlState gating") {
    if (!GameAvailable()) return;
    World24 w;
    auto part = w.g.New("probeCoreOcto.v2");
    ModuleCommand* c = Find<ModuleCommand>(*part);
    REQUIRE(c != nullptr);
    CHECK_FALSE(c->IsHibernating());
    c->Set<bool>("hibernateOnWarp", true);
    CHECK_FALSE(c->IsHibernating());                       // no warp, not packed
    part->fields.packed = true;
    CHECK(c->IsHibernating());                              // ModuleCommand.cs:128: part.packed
    part->fields.packed = false;
    TimeWarp tw;
    tw.LoadStockPrefab();
    TimeWarp::fetch = &tw;
    tw.Mode = TimeWarp::Modes::HIGH;
    tw.curr_rate = 1.0f;
    CHECK_FALSE(c->IsHibernating());
    tw.curr_rate = 5.0f;
    CHECK(c->IsHibernating());
    tw.Mode = TimeWarp::Modes::LOW;
    CHECK_FALSE(c->IsHibernating());                        // physics warp is not rails warp
    TimeWarp::fetch = nullptr;
    c->Set<bool>("hibernateOnWarp", false);
    KSPActionParam param;
    c->HibernateToggle(param);
    CHECK(c->Get<bool>("hibernation"));
    CHECK(c->IsHibernating());
    c->HibernateToggle(param);
    CHECK_FALSE(c->Get<bool>("hibernation"));
    // UpdateControlState does nothing outside flight / without a vessel / while the flight is paused (ModuleCommand.cs:462)
    part->fields.isControlSource = 77;
    c->UpdateControlState();
    CHECK(part->fields.isControlSource == 77);
    OneVessel ov({part.get()});
    {
        SceneScope editor(GameScenes::EDITOR);
        c->UpdateControlState();
        CHECK(part->fields.isControlSource == 77);
    }
    SceneScope flight(GameScenes::FLIGHT);
    CommandEnvironment::flightPaused = [] { return true; };
    c->UpdateControlState();
    CHECK(part->fields.isControlSource == 77);
    CommandEnvironment::flightPaused = nullptr;
    FillEC(*part, 100.0);
    c->UpdateControlState();
    CHECK(part->fields.isControlSource == static_cast<int>(ControlLevel::FULL));
}

TEST_CASE("WP-24 ModuleCommand control points: ChangeControlPoint cycles, SetControlPoint falls back to _default, OnControlPointChanged") {
    if (!GameAvailable()) return;
    World24 w;
    std::string withPoints;
    for (const auto& apu : w.g.data.parts.loadedParts) {
        if (!apu->partPrefab || !withPoints.empty()) continue;
        for (const PartModuleData& d : apu->partPrefab->ModuleData)
            if (dynamic_cast<ModuleCommand*>(d.module.get()) != nullptr && d.config->HasNode("CONTROLPOINT")) withPoints = apu->name;
    }
    REQUIRE_FALSE(withPoints.empty());
    auto part = w.g.New(withPoints.c_str());
    ModuleCommand* c = Find<ModuleCommand>(*part);
    REQUIRE(c != nullptr);
    LogCapture logs;
    std::vector<std::string> fired;
    struct L {
        std::vector<std::string>* out;
        void Run(Part*, ControlPoint* cp) { out->push_back(cp->name); }
    } l{&fired};
    GE::OnControlPointChanged.Add(&l, &L::Run);
    SceneScope editor(GameScenes::EDITOR);
    c->OnStart(StartState::Editor);   // SetControlPoint(activeControlPointName): "_default"
    REQUIRE(fired.size() == 1);
    CHECK(fired[0] == "_default");
    CHECK(c->ActiveControlPointName() == "_default");
    CHECK(c->ActiveControlPoint() != nullptr);
    const std::vector<std::string> keys = c->controlPoints->KeysList();
    CHECK(c->Events()["ChangeControlPoint"]->active == (keys.size() > 1));
    for (size_t i = 1; i <= keys.size(); i++) {
        c->ChangeControlPoint();
        CHECK(c->ActiveControlPointName() == keys[i % keys.size()]);
    }
    c->SetControlPoint("no such point");
    CHECK(c->ActiveControlPointName() == "_default");
    CHECK(logs.Has(log::Level::Warning, "Control Point name does not exist. using _default: no such point"));
    // the control point child transforms are created in the editor / flight scene with Quaternion.Euler(orientation) (identity for _default)
    CHECK(c->GetControlPoint("_default")->hasTransform);
    CHECK_FALSE(c->GetControlPoint("_default")->isPartTransform);
    CHECK(c->GetControlPoint("_default")->localRotation.Equals(Quaternionf::identity()));
    CHECK(c->GetControlPoint("nope") == nullptr);
    GE::OnControlPointChanged.Remove(&l, &L::Run);
    // a part without CONTROLPOINT nodes: SetControlPoint only logs (ModuleCommand.cs:243)
    std::string without;
    for (const auto& apu : w.g.data.parts.loadedParts) {
        if (!apu->partPrefab || !without.empty()) continue;
        for (const PartModuleData& d : apu->partPrefab->ModuleData)
            if (dynamic_cast<ModuleCommand*>(d.module.get()) != nullptr && !d.config->HasNode("CONTROLPOINT")) without = apu->name;
    }
    REQUIRE_FALSE(without.empty());
    auto probe = w.g.New(without.c_str());
    ModuleCommand* pc = Find<ModuleCommand>(*probe);
    REQUIRE(pc != nullptr);
    CHECK_FALSE(pc->ControlPointsExist());
    LogCapture logs2;
    pc->SetControlPoint("x");
    CHECK(logs2.Has(log::Level::Error, "There are no alternate control points"));
    CHECK(pc->GetControlPoint("_default") == nullptr);
}

TEST_CASE("WP-24 ControlPoint.Load, SteeringObject.FindAlignmentAxis, ModuleRemoteController.Toggle") {
    ControlPoint cp;
    ConfigNode::NodePtr n = ConfigNode::Parse("CONTROLPOINT\n{\nname = docking\ndisplayName = #autoLOC_X\norientation = 0,0,90\n}\n");
    cp.Load(*n->GetNode("CONTROLPOINT"));
    CHECK(cp.name == "docking");
    CHECK(cp.displayName == "#autoLOC_X");
    CHECK(cp.orientation.z == 90.0f);
    ConfigNode::NodePtr bad = ConfigNode::Parse("CONTROLPOINT\n{\nname = a\n}\n");
    ControlPoint cp2;
    CHECK_THROWS_AS(cp2.Load(*bad->GetNode("CONTROLPOINT")), NullReferenceException);

    // FindAlignmentAxis (SteeringObject.cs:31): the dominant dot product picks the axis, |value| < 0.7 means no drive
    SteeringObject so;
    PartTransform ref, part;
    so.controlAxis = SteeringObject::ControlAxis::Forward;
    so.FindAlignmentAxis(ref, part);                           // identical frames: ref.forward . part.forward = 1
    CHECK(so.alignmentAxis == SteeringObject::AlignmentAxis::Forward);
    CHECK(so.wheelDriveInvert == 1.0f);
    // part turned 180 degrees about up: forward points backwards
    part.rotation = Quaternionf(0.0f, 1.0f, 0.0f, 0.0f);
    so.FindAlignmentAxis(ref, part);
    CHECK(so.alignmentAxis == SteeringObject::AlignmentAxis::Forward);
    CHECK(so.wheelDriveInvert == -1.0f);
    // part turned 90 degrees about up: forward points along ref right
    const float s = std::sqrt(0.5f);
    part.rotation = Quaternionf(0.0f, s, 0.0f, s);
    so.FindAlignmentAxis(ref, part);
    CHECK(so.alignmentAxis == SteeringObject::AlignmentAxis::Right);
    CHECK(std::fabs(so.wheelDriveInvert) == 1.0f);
    // 45 degrees: the two dot products are 0.7071 and 0.7071 (>= 0.7): still a drive axis
    // 30 degrees from ref forward: |dot| = 0.866 forward wins
    const float h = std::sin(0.5f * 0.5235988f), cz = std::cos(0.5f * 0.5235988f);
    part.rotation = Quaternionf(0.0f, h, 0.0f, cz);
    so.FindAlignmentAxis(ref, part);
    CHECK(so.alignmentAxis == SteeringObject::AlignmentAxis::Forward);
    CHECK(so.wheelDriveInvert == 1.0f);   // 0.866 >= 0.7: normalised to +-1
    // 60 degrees: forward dot 0.5, right dot 0.866: Right wins
    const float h2 = std::sin(0.5f * 1.0471976f), c2 = std::cos(0.5f * 1.0471976f);
    part.rotation = Quaternionf(0.0f, h2, 0.0f, c2);
    so.FindAlignmentAxis(ref, part);
    CHECK(so.alignmentAxis == SteeringObject::AlignmentAxis::Right);
    CHECK(so.wheelDriveInvert == 1.0f);
}

TEST_CASE("WP-24 ModuleRemoteController.Toggle flips controllerActive") {
    if (!GameAvailable()) return;
    World24 w;
    // no stock part carries the module (a Making History drone shell module): add it the way Part.AddModule(string) does
    for (const auto& apu : w.g.data.parts.loadedParts)
        if (apu->partPrefab)
            for (const PartModuleData& d : apu->partPrefab->ModuleData) CHECK(dynamic_cast<ModuleRemoteController*>(d.module.get()) == nullptr);
    auto part = w.g.New("probeCoreOcto.v2");
    PartModuleData* md = part->AddModule("ModuleRemoteController", true);
    REQUIRE(md != nullptr);
    auto* m = dynamic_cast<ModuleRemoteController*>(md->module.get());
    REQUIRE(m != nullptr);
    CHECK(m->Get<std::string>("controllerActive") == "Inactive");
    m->Events()["Toggle"]->Invoke();
    CHECK(m->Get<std::string>("controllerActive") == "Active");
    m->Toggle();
    CHECK(m->Get<std::string>("controllerActive") == "Inactive");
    m->Set<std::string>("controllerActive", "other");
    m->Toggle();
    CHECK(m->Get<std::string>("controllerActive") == "other");
}

// ============================================================ ModuleSAS

TEST_CASE("WP-24 ModuleSAS: GetValue (command-module and standalone), OnStart registrations, FixedUpdate service level and standalone state") {
    if (!GameAvailable()) return;
    World24 w;
    SceneScope flight(GameScenes::FLIGHT);
    int checked = 0, standalone = 0;
    for (const auto& apu : w.g.data.parts.loadedParts) {
        if (!apu->partPrefab) continue;
        bool has = false;
        for (const PartModuleData& d : apu->partPrefab->ModuleData) has = has || dynamic_cast<ModuleSAS*>(d.module.get()) != nullptr;
        if (!has) continue;
        INFO(apu->name);
        auto part = w.g.New(apu->name.c_str());
        ModuleSAS* sas = Find<ModuleSAS>(*part);
        REQUIRE(sas != nullptr);
        ModuleCommand* cmd = Find<ModuleCommand>(*part);
        OneVessel ov({part.get()});
        part->fields.started = true;
        const int level = sas->Get<int>("SASServiceLevel");
        sas->OnStart(StartState::Flying);
        CHECK(sas->targetSASServiceLevel == level);
        if (!sas->Get<bool>("standalone")) {
            // ModuleSAS.cs:62: the command module of the part (CommandModuleIndex or the first ModuleCommand)
            const int idx = sas->Get<int>("CommandModuleIndex");
            if (idx != -1) CHECK(sas->CommandModule() == dynamic_cast<ModuleCommand*>(part->Modules()[static_cast<size_t>(idx)]));
            else CHECK(sas->CommandModule() == cmd);
            REQUIRE(sas->CommandModule() != nullptr);
            for (bool requireCrew : {false, true}) {
                sas->Set<bool>("RequireCrew", requireCrew);
                for (auto ms : {ModuleCommand::ModuleControlState::NotEnoughCrew, ModuleCommand::ModuleControlState::NotEnoughResources,
                                ModuleCommand::ModuleControlState::PartialManned, ModuleCommand::ModuleControlState::Nominal}) {
                    // drive the real module into each state through its inputs
                    ModuleCommand* c = sas->CommandModule();
                    part->protoModuleCrew.clear();
                    const int min = c->Get<int>("minimumCrew");
                    FillEC(*part, ms == ModuleCommand::ModuleControlState::NotEnoughResources ? 0.0 : 1000.0);
                    if (ms == ModuleCommand::ModuleControlState::Nominal || ms == ModuleCommand::ModuleControlState::PartialManned)
                        for (int i = 0; i < min; i++) part->protoModuleCrew.push_back(w.Kerbal("Engineer"));
                    c->Set<bool>("hibernation", ms == ModuleCommand::ModuleControlState::PartialManned);
                    c->UpdateControlSourceState();
                    const auto st = c->ModuleState();
                    // RequireCrew ? state == Nominal : state != NotEnoughResources (ModuleSAS.cs:112)
                    const bool ok = requireCrew ? st == ModuleCommand::ModuleControlState::Nominal : st != ModuleCommand::ModuleControlState::NotEnoughResources;
                    CHECK(sas->GetValue() == (ok ? level : -1));
                    sas->Set<bool>("moduleIsEnabled", false);
                    CHECK(sas->GetValue() == -1);   // disabled and not standalone
                    sas->Set<bool>("moduleIsEnabled", true);
                }
            }
            part->protoModuleCrew.clear();
        } else {
            standalone++;
            CHECK(sas->CommandModule() == nullptr);
            CHECK_FALSE(sas->standaloneOperational);
            CHECK(sas->GetValue() == -1);
            // FixedUpdate: toggle on, autopilot off => operational without a draw; autopilot on => the resource draw decides
            sas->Set<bool>("standaloneToggle", true);
            FillEC(*part, 1000.0);
            sas->FixedUpdate();
            CHECK(sas->standaloneOperational);
            CHECK(sas->GetValue() == level);
            CHECK(sas->Get<std::string>("standaloneStateText") == Localizer24::Format("#autoLOC_6001373"));
            sas->Set<bool>("standaloneToggle", false);
            sas->FixedUpdate();
            CHECK_FALSE(sas->standaloneOperational);
            CHECK(sas->Get<std::string>("standaloneStateText") == Localizer24::Format("#autoLOC_6001374"));
            CHECK(sas->GetValue() == -1);
            sas->Set<bool>("standaloneToggle", true);
            REQUIRE(ov.v->Autopilot().Enable());
            const bool hasSupply = part->GetResource("ElectricCharge") != nullptr;
            FillEC(*part, 0.0);
            sas->FixedUpdate();
            if (!sas->resHandler.inputResources.empty()) CHECK_FALSE(sas->standaloneOperational);
            FillEC(*part, 1000.0);
            sas->FixedUpdate();
            CHECK(sas->standaloneOperational == (hasSupply || sas->resHandler.inputResources.empty()));
            ov.v->Autopilot().Disable();
        }
        // adjusters (AdjusterSASBase): the service level is the adjusted target in flight
        sas->AddAdjuster([](int l) { return l + 1; });
        sas->AddAdjuster([](int l) { return l * 2; });
        sas->FixedUpdate();
        CHECK(sas->Get<int>("SASServiceLevel") == (level + 1) * 2);
        checked++;
        // the part values see the module: AutopilotSkill / AutopilotSASSkill providers
        part->partValues.Update();
        CHECK(part->partValues.AutopilotSASSkill.value() == sas->GetValue());
    }
    CHECK(checked >= 6);
    CHECK(standalone >= 1);
}

// ============================================================ ModuleReactionWheel

TEST_CASE("WP-24 ModuleReactionWheel: state machine, actions, OnLoad / OnSave, GetPotentialTorque, mode cycling on every stock wheel") {
    if (!GameAvailable()) return;
    World24 w;
    int checked = 0;
    for (const auto& apu : w.g.data.parts.loadedParts) {
        if (!apu->partPrefab) continue;
        bool has = false;
        for (const PartModuleData& d : apu->partPrefab->ModuleData) has = has || dynamic_cast<ModuleReactionWheel*>(d.module.get()) != nullptr;
        if (!has) continue;
        INFO(apu->name);
        auto part = w.g.New(apu->name.c_str());
        ModuleReactionWheel* rw = Find<ModuleReactionWheel>(*part);
        REQUIRE(rw != nullptr);
        const Vector3f t(rw->Get<float>("PitchTorque"), rw->Get<float>("RollTorque"), rw->Get<float>("YawTorque"));
        Vector3f pos, neg;
        rw->GetPotentialTorque(pos, neg);
        CHECK((pos.x == t.x && pos.y == t.y && pos.z == t.z));
        CHECK((neg.x == t.x && neg.y == t.y && neg.z == t.z));
        // actuatorModeCycle 2 (Pilot only) offers no SAS torque, 1 (SAS only) still does (ModuleReactionWheel.cs:287)
        rw->Set<int>("actuatorModeCycle", 2);
        rw->GetPotentialTorque(pos, neg);
        CHECK((pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f && neg.x == 0.0f));
        rw->Set<int>("actuatorModeCycle", 1);
        rw->GetPotentialTorque(pos, neg);
        CHECK(pos.x == t.x);
        // CycleAction wraps 0 -> 1 -> 2 -> 0
        KSPActionParam param;
        rw->Set<int>("actuatorModeCycle", 0);
        rw->CycleAction(param);
        CHECK(rw->Get<int>("actuatorModeCycle") == 1);
        rw->CycleAction(param);
        rw->CycleAction(param);
        CHECK(rw->Get<int>("actuatorModeCycle") == 0);
        // Activate / Deactivate / Toggle / OnToggle on Active, Disabled and Broken (the Broken state ignores every action)
        using WS = ModuleReactionWheel::WheelState;
        CHECK(rw->State() == WS::Active);
        rw->Activate(param);
        CHECK(rw->State() == WS::Active);
        rw->Deactivate(param);
        CHECK(rw->State() == WS::Disabled);
        CHECK(rw->Get<std::string>("stateString") == Localizer24::Format("#autoLOC_6003062"));
        rw->GetPotentialTorque(pos, neg);
        CHECK((pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f));
        rw->Deactivate(param);
        CHECK(rw->State() == WS::Disabled);
        rw->Toggle(param);
        CHECK(rw->State() == WS::Active);
        CHECK(rw->Get<std::string>("stateString") == "Active");
        rw->OnToggle();
        CHECK(rw->State() == WS::Disabled);
        rw->OnToggle();
        rw->SetState(WS::Broken);
        CHECK(rw->Get<std::string>("stateString") == Localizer24::Format("#autoLOC_6003063"));
        rw->Activate(param);
        rw->Deactivate(param);
        rw->Toggle(param);
        CHECK(rw->State() == WS::Broken);
        rw->GetPotentialTorque(pos, neg);
        CHECK((pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f));
        // OnSave writes State.ToString(); OnLoad Enum.Parse's it back (and throws for an unknown name)
        ConfigNode node("MODULE");
        rw->OnSave(node);
        CHECK(*node.GetValuePtr("WheelState") == "Broken");
        rw->SetState(WS::Active);
        rw->OnLoad(node);
        CHECK(rw->State() == WS::Broken);
        ConfigNode disabled("MODULE");
        disabled.AddValue("WheelState", "Disabled");
        rw->OnLoad(disabled);
        CHECK(rw->State() == WS::Disabled);
        ConfigNode junk("MODULE");
        junk.AddValue("WheelState", "Sideways");
        CHECK_THROWS_AS(rw->OnLoad(junk), ArgumentException);
        ConfigNode none("MODULE");
        rw->OnLoad(none);   // no WheelState value: unchanged
        CHECK(rw->State() == WS::Disabled);
        // module not enabled: no torque
        rw->SetState(WS::Active);
        rw->Set<bool>("moduleIsEnabled", false);
        rw->GetPotentialTorque(pos, neg);
        CHECK((pos.x == 0.0f && neg.z == 0.0f));
        // OnStart: PAW state follows moduleIsEnabled
        rw->OnStart(StartState::Flying);
        CHECK_FALSE(rw->Actions()["Toggle"]->active);
        CHECK_FALSE(rw->Fields()["authorityLimiter"]->guiActive);
        CHECK_FALSE(rw->Events()["OnToggle"]->guiActive);
        rw->Set<bool>("moduleIsEnabled", true);
        rw->OnStart(StartState::Flying);
        CHECK(rw->Actions()["Toggle"]->active);
        CHECK(rw->Fields()["stateString"]->guiActiveEditor);
        checked++;
    }
    CHECK(checked >= 20);
}

TEST_CASE("WP-24 ModuleReactionWheel adjusters: a breaking adjuster stops the wheel, torque adjusters chain in order") {
    if (!GameAvailable()) return;
    World24 w;
    SceneScope flight(GameScenes::FLIGHT);
    auto part = w.g.New("probeCoreOcto.v2");
    ModuleReactionWheel* rw = Find<ModuleReactionWheel>(*part);
    REQUIRE(rw != nullptr);
    struct Scale : IReactionWheelAdjuster {
        float k;
        bool broken = false;
        explicit Scale(float kk) : k(kk) {}
        bool IsReactionWheelBroken() override { return broken; }
        Vector3f ApplyTorqueAdjustment(const Vector3f& t) override { return t * k; }
    } a(2.0f), b(0.5f);
    rw->AddAdjuster(&a);
    rw->AddAdjuster(&b);
    const Vector3f in(1.0f, -2.0f, 3.0f);
    const Vector3f out = rw->ApplyTorqueAdjustments(in);
    CHECK((out.x == 1.0f && out.y == -2.0f && out.z == 3.0f));   // *2 then *0.5
    CHECK_FALSE(rw->IsAdjusterBreakingReactionWheel());
    b.broken = true;
    CHECK(rw->IsAdjusterBreakingReactionWheel());
    OneVessel ov({part.get()});
    part->fields.started = true;
    FillEC(*part, 10.0);
    ov.v->ctrlState.pitch = 1.0f;
    rw->FixedUpdate();
    CHECK(rw->inputSum == 0.0f);   // FixedUpdate did not run ActiveUpdate
    CHECK_FALSE(rw->torqueApplied);
    rw->RemoveAdjuster(&b);
    CHECK_FALSE(rw->IsAdjusterBreakingReactionWheel());
    rw->FixedUpdate();
    CHECK(rw->inputSum > 0.0f);
    CHECK(rw->torqueApplied);
}

// ============================================================ the reference replays

namespace {
Vector3f V3(const json& a) { return Vector3f(a[0].get<float>(), a[1].get<float>(), a[2].get<float>()); }
Quaternionf Q4(const json& a) { return Quaternionf(a[0].get<float>(), a[1].get<float>(), a[2].get<float>(), a[3].get<float>()); }
}  // namespace

TEST_CASE("WP-24 ModuleReactionWheel vs tools/wp24/sas_reference.py: torque mapping, authority, Lerp, inputSum, resource draw, applied torque (every stock wheel)") {
    if (!GameAvailable()) return;
    const json ref = LoadRef24();
    World24 w;
    SceneScope flight(GameScenes::FLIGHT);
    UlpTally tally;
    long steps = 0, updated = 0, operational = 0, starved = 0;
    for (const json& cs : ref["wheels"]) {
        const json& spec = cs["spec"];
        const std::string name = PartNameOf(spec["part"].get<std::string>());
        INFO(name);
        auto part = w.g.New(name.c_str());
        ModuleReactionWheel* rw = Find<ModuleReactionWheel>(*part);
        REQUIRE(rw != nullptr);
        // the reference read the same cfg values
        CHECK(Bits(rw->Get<float>("PitchTorque")) == Bits(spec["PitchTorque"].get<float>()));
        CHECK(Bits(rw->Get<float>("YawTorque")) == Bits(spec["YawTorque"].get<float>()));
        CHECK(Bits(rw->Get<float>("RollTorque")) == Bits(spec["RollTorque"].get<float>()));
        CHECK(Bits(rw->Get<float>("torqueResponseSpeed")) == Bits(spec["torqueResponseSpeed"].get<float>()));
        if (spec["rate"].is_null()) {
            CHECK(rw->resHandler.inputResources.empty());
        } else {
            REQUIRE(rw->resHandler.inputResources.size() == 1);
            CHECK(rw->resHandler.inputResources[0]->rate == spec["rate"].get<double>());
        }
        OneVessel ov({part.get()});
        part->fields.started = true;
        PartResource* ec = part->GetResource("ElectricCharge");
        CHECK((ec != nullptr) == !spec["ec_max"].is_null());
        if (ec != nullptr) {
            CHECK(ec->maxAmount == spec["ec_max"].get<double>());
            ec->amount = spec["ec_start"].get<double>();
            part->ResetSimulationResources();
        }
        Vessel& v = *ov.v;
        Vector3f cmd;
        ReactionWheelEnvironment::activeVesselPYR = [&] { return cmd; };
        rw->Set<int>("actuatorModeCycle", 0);
        for (const json& st : cs["steps"]) {
            const json& ex = cs["expected"][static_cast<size_t>(&st - &cs["steps"][0])];
            rw->Set<int>("actuatorModeCycle", st["mode"].get<int>());
            rw->Set<float>("authorityLimiter", st["authority"].get<float>());
            // Autopilot.Enable / Disable neutralise the stick (Disable: NeutralizeStick): switch first, then set the control state
            if (st["autopilotEnabled"].get<bool>() != v.Autopilot().Enabled()) v.Autopilot().SetEnabled(st["autopilotEnabled"].get<bool>());
            const json& ctrl = st["ctrl"];
            v.ctrlState.pitch = ctrl[0].get<float>();
            v.ctrlState.roll = ctrl[1].get<float>();
            v.ctrlState.yaw = ctrl[2].get<float>();
            cmd = V3(st["cmd"]);
            v.vesselTransform.rotation = Q4(st["q"]);
            FlightRegistry::ActiveVessel = st["activeVessel"].get<bool>() ? &v : nullptr;
            rw->torqueApplied = false;
            rw->FixedUpdate();
            steps++;
            INFO("step " << steps << " mode " << st["mode"].get<int>() << " ap " << st["autopilotEnabled"].get<bool>() << " apEnabled " << v.Autopilot().Enabled()
                         << " sum " << rw->inputSum << " ec " << (ec ? ec->amount : -1.0) << " state " << rw->Get<std::string>("stateString")
                         << " expected sum " << ex["inputSum"].get<double>() << " ec " << ex["ec"].get<double>());
            if (!ex["updated"].get<bool>()) {
                CHECK_FALSE(rw->torqueApplied);
                continue;
            }
            updated++;
            const Vector3f ivt = rw->GetAppliedTorque();
            const json& e1 = ex["inputVectorTemp"];
            tally.F(ivt.x, e1[0].get<double>());
            tally.F(ivt.y, e1[1].get<double>());
            tally.F(ivt.z, e1[2].get<double>());
            const json& e2 = ex["inputVector"];
            tally.F(rw->inputVector.x, e2[0].get<double>());
            tally.F(rw->inputVector.y, e2[1].get<double>());
            tally.F(rw->inputVector.z, e2[2].get<double>());
            tally.F(rw->inputSum, ex["inputSum"].get<double>());
            CHECK(rw->operational == ex["operational"].get<bool>());
            CHECK(rw->torqueApplied == ex["operational"].get<bool>());
            if (rw->operational) {
                operational++;
                const json& t = ex["torque"];
                tally.F(rw->lastTorque.x, t[0].get<double>());
                tally.F(rw->lastTorque.y, t[1].get<double>());
                tally.F(rw->lastTorque.z, t[2].get<double>());
                CHECK(rw->Get<std::string>("stateString") == Localizer24::Format("#autoLOC_7001223"));
            } else {
                starved++;
            }
            if (ec != nullptr) tally.D(ec->amount, ex["ec"].get<double>());
        }
        ReactionWheelEnvironment::activeVesselPYR = nullptr;
        FlightRegistry::ActiveVessel = nullptr;
    }
    MESSAGE("reaction wheel replay: " << ref["wheels"].size() << " wheels, " << steps << " steps, " << updated << " updates, " << operational
                                      << " operational, " << starved << " not; values " << tally.values << ", max ULP float " << tally.maxF
                                      << " double " << tally.maxD);
    CHECK(tally.maxF == 0);
    CHECK(tally.maxD == 0);
    CHECK(operational > 100);
    CHECK(starved > 10);
}

TEST_CASE("WP-24 VesselSAS vs tools/wp24/sas_reference.py: 1100 controller steps over four scenarios (locked, target, warp, no decay)") {
    if (!GameAvailable()) return;
    const json ref = LoadRef24();
    World24 w;
    SceneScope flight(GameScenes::FLIGHT);
    UlpTally tally;
    long total = 0, damped = 0;
    for (const json& sc : ref["sas"]) {
        INFO(sc["name"].get<std::string>());
        std::vector<std::unique_ptr<Part>> owned;
        std::vector<Part*> parts;
        for (const json& pj : sc["parts"]) {
            auto p = w.g.New(PartNameOf(pj["part"].get<std::string>()).c_str());
            ModuleReactionWheel* rw = Find<ModuleReactionWheel>(*p);
            REQUIRE(rw != nullptr);
            rw->Set<int>("actuatorModeCycle", pj["actuatorModeCycle"].get<int>());
            const std::string state = pj["state"].get<std::string>();
            rw->SetState(state == "Active" ? ModuleReactionWheel::WheelState::Active : ModuleReactionWheel::WheelState::Disabled);
            parts.push_back(p.get());
            owned.push_back(std::move(p));
        }
        OneVessel ov(parts);
        Vessel& v = *ov.v;
        FlightRegistry::ActiveVessel = &v;
        v.MOI = V3(sc["moi"]);
        v.vesselTransform.rotation = Q4(sc["q0"]);
        VesselAutopilot::VesselSAS& sas = v.Autopilot().SAS();
        sas.lockedMode = sc["locked"].get<bool>();
        sas.autoTune = sc["autoTune"].get<bool>();
        sas.useDecay = sc["useDecay"].get<bool>();
        TimeWarp tw;
        if (!sc["warpRate"].is_null()) {
            tw.LoadStockPrefab();
            tw.Mode = TimeWarp::Modes::HIGH;
            tw.curr_rate = sc["warpRate"].get<float>();
            TimeWarp::fetch = &tw;
        }
        Time::deltaTime = sc["deltaTime"].get<float>();
        int kills = 0;
        AutopilotEnvironment::killAngularVelocity = [&](Vessel&) { kills++; };
        bool flags[3] = {false, false, false};
        AutopilotEnvironment::pitchInput = [&](float) { return flags[0]; };
        AutopilotEnvironment::rollInput = [&](float) { return flags[1]; };
        AutopilotEnvironment::yawInput = [&](float) { return flags[2]; };
        if (sas.lockedMode) sas.ConnectFlyByWire(true);
        else sas.SetTargetOrientation(V3(sc["target"]), true);
        REQUIRE(sas.FBWconnected());
        const json& steps = sc["steps"];
        for (size_t i = 0; i < steps.size(); i++) {
            const json& s = steps[i];
            const json& ex = sc["expected"][i];
            v.vesselTransform.rotation = Q4(s["q"]);
            v.angularVelocity = V3(s["w"]);
            v.ctrlState.pitch = s["pilot"][0].get<float>();
            v.ctrlState.roll = s["pilot"][1].get<float>();
            v.ctrlState.yaw = s["pilot"][2].get<float>();
            for (int k = 0; k < 3; k++) flags[k] = s["input"][static_cast<size_t>(k)].get<bool>();
            v.OnAutopilotUpdate(v.ctrlState);
            total++;
            const json& c = ex["ctrl"];
            tally.F(v.ctrlState.pitch, c[0].get<double>());
            tally.F(v.ctrlState.roll, c[1].get<double>());
            tally.F(v.ctrlState.yaw, c[2].get<double>());
            for (size_t k = 0; k < 3; k++) {
                tally.D(sas.sasResponse()[static_cast<int>(k)], ex["sasResponse"][k].get<double>());
                tally.F(sas.rotationDelta()[static_cast<int>(k)], ex["rotationDelta"][k].get<double>());
                tally.D(sas.angularDeltaRad()[static_cast<int>(k)], ex["angularDeltaRad"][k].get<double>());
                tally.F(sas.angularAccelerationMax()[static_cast<int>(k)], ex["angularAccelerationMax"][k].get<double>());
                tally.F(sas.decayScalar()[static_cast<int>(k)], ex["decayScalar"][k].get<double>());
                tally.F(sas.torqueVector()[static_cast<int>(k)], ex["torqueVector"][k].get<double>());
            }
            const Quaternionf lr = sas.lockedRotation;
            tally.F(lr.x, ex["lockedRotation"][0].get<double>());
            tally.F(lr.y, ex["lockedRotation"][1].get<double>());
            tally.F(lr.z, ex["lockedRotation"][2].get<double>());
            tally.F(lr.w, ex["lockedRotation"][3].get<double>());
            CHECK(sas.dampingMode == ex["dampingMode"].get<bool>());
            if (sas.dampingMode) damped++;
            tally.F(sas.pidLockedPitch.tuningScalar, ex["tuningScalar"][0].get<double>());
            tally.F(sas.pidLockedRoll.tuningScalar, ex["tuningScalar"][1].get<double>());
            tally.F(sas.pidLockedYaw.tuningScalar, ex["tuningScalar"][2].get<double>());
            tally.F(sas.pidLockedPitch.clampScalar, ex["clampScalar"][0].get<double>());
            tally.F(sas.pidLockedRoll.clampScalar, ex["clampScalar"][1].get<double>());
            tally.F(sas.pidLockedYaw.clampScalar, ex["clampScalar"][2].get<double>());
            tally.D(sas.pidLockedPitch.Output(), ex["pidOutput"][0].get<double>());
            tally.D(sas.pidLockedRoll.Output(), ex["pidOutput"][1].get<double>());
            tally.D(sas.pidLockedYaw.Output(), ex["pidOutput"][2].get<double>());
        }
        CHECK(kills == sc["killAngularVelocityCalls"].get<int>());
        sas.DisconnectFlyByWire();
        TimeWarp::fetch = nullptr;
        Time::deltaTime = 0.02f;
        AutopilotEnvironment::Reset();
        FlightRegistry::ActiveVessel = nullptr;
    }
    MESSAGE("SAS replay: " << total << " steps, " << tally.values << " values, damped steps " << damped << ", max ULP float " << tally.maxF
                           << " double " << tally.maxD);
    CHECK(tally.maxF == 0);
    CHECK(tally.maxD == 0);
    CHECK(total == 1100);
    CHECK(damped > 50);
}

// ============================================================ VesselAutopilot logic

TEST_CASE("WP-24 VesselAutopilot: Enable / Disable / SetMode / CanSetMode, the SAS action group, locks, target and maneuver orientation") {
    if (!GameAvailable()) return;
    World24 w;
    SceneScope flight(GameScenes::FLIGHT);
    auto part = w.g.New("probeCoreOcto.v2");
    OneVessel ov({part.get()});
    Vessel& v = *ov.v;
    FlightRegistry::ActiveVessel = &v;
    VesselAutopilot& ap = v.Autopilot();
    using AM = VesselAutopilot::AutopilotMode;
    CHECK_FALSE(ap.Enabled());
    CHECK(ap.Mode() == AM::StabilityAssist);

    // Enable: lockedMode follows the mode, the controller connects its callbacks to OnAutopilotUpdate
    CHECK(ap.Enable());
    CHECK(ap.Enabled());
    CHECK(ap.SAS().lockedMode);
    CHECK(ap.SAS().FBWconnected());
    // Disable neutralises the stick and disconnects
    v.ctrlState.pitch = 0.4f;
    v.ctrlState.roll = -0.2f;
    v.ctrlState.yaw = 0.3f;
    v.ctrlState.mainThrottle = 0.7f;
    CHECK(ap.Disable());
    CHECK_FALSE(ap.Enabled());
    CHECK_FALSE(ap.SAS().FBWconnected());
    CHECK((v.ctrlState.pitch == 0.0f && v.ctrlState.roll == 0.0f && v.ctrlState.yaw == 0.0f));
    CHECK(v.ctrlState.mainThrottle == 0.7f);   // NeutralizeStick keeps the throttle (FlightCtrlState.cs:170)
    CHECK(ap.SAS().targetOrientation.x == 0.0f);

    // modes needing a velocity: VectorLockInvalid(1) against FlightGlobals.GetDisplayVelocity().sqrMagnitude
    AutopilotEnvironment::displayVelocitySqrMagnitude = [](Vessel&) { return 0.5; };
    CHECK_FALSE(ap.CanSetMode(AM::Prograde));
    CHECK_FALSE(ap.CanSetMode(AM::Retrograde));
    CHECK(ap.CanSetMode(AM::Target) == false);   // no target object
    CHECK(ap.CanSetMode(AM::Maneuver) == false); // no patched conic solver
    CHECK(ap.CanSetMode(AM::StabilityAssist));
    AutopilotEnvironment::displayVelocitySqrMagnitude = [](Vessel&) { return 4.0; };
    CHECK(ap.CanSetMode(AM::Prograde));
    CHECK(ap.CanSetMode(AM::Normal));
    AutopilotEnvironment::speedDisplayMode = [] { return AutopilotSpeedMode::Surface; };
    AutopilotEnvironment::displayVelocitySqrMagnitude = [](Vessel&) { return 0.5; };
    CHECK(ap.CanSetMode(AM::Normal));   // Normal / Radial only need a velocity in Orbit mode
    AutopilotEnvironment::speedDisplayMode = nullptr;
    CHECK_FALSE(ap.CanSetMode(AM::Normal));
    AutopilotEnvironment::displayVelocitySqrMagnitude = [](Vessel&) { return 4.0; };

    // SetMode while off only records the mode (and lockedMode); while on it re-enables
    CHECK(ap.SetMode(AM::Prograde));
    CHECK(ap.Mode() == AM::Prograde);
    CHECK_FALSE(ap.SAS().lockedMode);
    CHECK_FALSE(ap.Enabled());
    v.obt_velocity = Vector3d(30.0, 40.0, 0.0);
    CHECK(ap.Enable(AM::Prograde));
    CHECK(ap.SAS().FBWconnected());
    // SetTargetOrientation(vessel.obt_velocity): the normalised float vector
    CHECK(ap.SAS().targetOrientation.x == doctest::Approx(0.6f));
    CHECK(ap.SAS().targetOrientation.y == doctest::Approx(0.8f));
    CHECK(ap.SetMode(AM::Retrograde));
    CHECK(ap.Enabled());
    CHECK(ap.SAS().targetOrientation.x == doctest::Approx(-0.6f));
    // Normal / RadialIn in Orbit mode use orbit.h (needs an orbit: skipped); in Surface mode Normal = the body rotation axis, RadialIn = upAxis
    AutopilotEnvironment::speedDisplayMode = [] { return AutopilotSpeedMode::Surface; };
    v.upAxis = Vector3d(0.0, 0.0, 2.0);
    CHECK(ap.SetMode(AM::RadialIn));
    CHECK(ap.SAS().targetOrientation.z == 1.0f);
    CHECK(ap.SetMode(AM::RadialOut));
    CHECK(ap.SAS().targetOrientation.z == -1.0f);
    CHECK(ap.SetMode(AM::Normal));
    CHECK(ap.SAS().targetOrientation.y == 1.0f);   // CelestialBody.RotationAxis = up
    CHECK(ap.SetMode(AM::Antinormal));
    CHECK(ap.SAS().targetOrientation.y == -1.0f);
    CHECK(ap.SetMode(AM::Prograde));
    v.srf_velocity = Vector3d(0.0, 3.0, 4.0);
    ap.Update();   // SAS action group off => the running autopilot is disabled
    CHECK_FALSE(ap.Enabled());
    // Target: needs a target object; the orientation is the normalised ref-to-target vector
    OneVessel other({});
    other.v->vesselTransform.position = Vector3f(0.0f, 0.0f, 100.0f);
    CHECK_FALSE(ap.SetMode(AM::Target));
    v.targetObject = other.v.get();
    CHECK(ap.SetMode(AM::Target));
    CHECK(ap.Enable(AM::Target));
    CHECK(ap.SAS().targetOrientation.z == 1.0f);
    CHECK(ap.SetMode(AM::AntiTarget));
    CHECK(ap.SAS().targetOrientation.z == -1.0f);
    v.targetObject = nullptr;
    ap.Update();   // the action group is off: disables
    CHECK_FALSE(ap.Enabled());
    // the SAS action group switches the autopilot on, and off again when the mode stops being available
    v.ActionGroups().Set(KSPActionGroup::SAS, true);
    CHECK(ap.SetMode(AM::StabilityAssist));
    ap.Update();
    CHECK(ap.Enabled());
    CHECK(ap.Mode() == AM::StabilityAssist);
    v.ActionGroups().Set(KSPActionGroup::SAS, false);
    ap.Update();
    CHECK_FALSE(ap.Enabled());
    // Enabled setter
    ap.SetEnabled(true);
    CHECK(ap.Enabled());
    ap.SetEnabled(true);
    CHECK(ap.Enabled());
    ap.SetEnabled(false);
    CHECK_FALSE(ap.Enabled());
    ap.Destroy();
    CHECK_FALSE(ap.SAS().FBWconnected());
}

TEST_CASE("WP-24 VesselSAS small rules: PID reset, tuning scalars, lock rotation bookkeeping, SAS availability by game mode") {
    if (!GameAvailable()) return;
    World24 w;
    SceneScope flight(GameScenes::FLIGHT);
    auto part = w.g.New("probeCoreOcto.v2");
    OneVessel ov({part.get()});
    Vessel& v = *ov.v;
    VesselAutopilot::VesselSAS& sas = v.Autopilot().SAS();
    // the constructor gains (VesselAutopilot.cs:113)
    CHECK(sas.pidLockedPitch.kp == 15000.0);
    CHECK(sas.pidLockedPitch.kd == 2000.0);
    CHECK(sas.pidLockedPitch.clamp == 1500.0);
    CHECK(sas.pidLockedRoll.kp == 9000.0);
    CHECK(sas.pidLockedRoll.kd == 600.0);
    CHECK(sas.pidLockedRoll.clamp == 2000.0);
    CHECK(sas.pidLockedYaw.kp == 15000.0);
    sas.SetLockYawPID(1.0f, 2.0f, 3.0f, 4.0f);
    CHECK(sas.pidLockedYaw.kp == 1.0);
    CHECK(sas.pidLockedYaw.ki == 2.0);
    CHECK(sas.pidLockedYaw.clamp == 4.0);
    // SetTuningScalar stores the scalar and resets the controllers' tuning (AutoTuneReset: kdScalar = 1)
    sas.pidLockedPitch.kdScalar = 5.0f;
    sas.SetTuningScalar(0.25f);
    CHECK(sas.sasTuningScalar() == 0.25f);
    CHECK(sas.pidLockedPitch.tuningScalar == 0.25f);
    CHECK(sas.pidLockedRoll.tuningScalar == 0.25f);
    CHECK(sas.pidLockedPitch.kdScalar == 1.0f);
    sas.TuneScalars(3.0f);
    CHECK(sas.pidLockedYaw.tuningScalar == 3.0f);
    // CanEngageSAS: sandbox with EnableFullSASInSandbox => any mode (APSkillExtensions.cs:6)
    CHECK(sas.CanEngageSAS());
    w.game.Parameters->CustomParams<GameParameters::AdvancedParams>().EnableFullSASInSandbox = false;
    // sandbox without it: the vessel needs a SAS provider (AutopilotSASSkill) unless it has no skill-bearing modules at all
    const bool sandboxNoFull = sas.CanEngageSAS();
    (void)sandboxNoFull;
    w.game.Parameters->CustomParams<GameParameters::AdvancedParams>().EnableFullSASInSandbox = true;
    // Start: lock the reference rotation and reset the controllers
    v.vesselTransform.rotation = Quaternionf(0.0f, 0.6f, 0.0f, 0.8f);
    sas.Start();
    CHECK(sas.lockedRotation.Equals(v.vesselTransform.rotation));
    CHECK(sas.storedTransform() == &v.ReferenceTransform());
    // Update in flight: stays locked while the reference transform is the stored one; a new reference part re-locks
    sas.Update();
    CHECK(sas.lockedRotation.Equals(v.vesselTransform.rotation));
    v.SetReferenceTransform(part.get());
    part->partTransform.rotation = Quaternionf(0.0f, 0.0f, 0.6f, 0.8f);
    sas.Update();
    CHECK(sas.storedTransform() == &part->partTransform);
    CHECK(sas.lockedRotation.Equals(part->partTransform.rotation));
    // VesselModified of another vessel changes nothing; of this one in free mode disconnects
    sas.ConnectFlyByWire(true);
    sas.lockedMode = false;
    OneVessel other({});
    sas.VesselModified(*other.v);
    CHECK(sas.FBWconnected());
    sas.VesselModified(v);
    CHECK_FALSE(sas.FBWconnected());
    CHECK(sas.lockedRotation.Equals(Quaternionf::identity()));   // DisconnectFlyByWire
    // UpdateVesselTorque over the vessel modules: the reaction wheel of the probe core is the torque provider (max of pos / neg, per axis)
    ModuleReactionWheel* rw = Find<ModuleReactionWheel>(*part);
    REQUIRE(rw != nullptr);
    sas.lockedMode = true;
    sas.ConnectFlyByWire(true);
    v.OnAutopilotUpdate(v.ctrlState);
    CHECK(sas.torqueVector().x == rw->Get<float>("PitchTorque"));
    CHECK(sas.torqueVector().y == rw->Get<float>("RollTorque"));
    CHECK(sas.torqueVector().z == rw->Get<float>("YawTorque"));
    rw->SetState(ModuleReactionWheel::WheelState::Disabled);
    v.OnAutopilotUpdate(v.ctrlState);
    CHECK(sas.torqueVector().x == 0.0f);   // a disabled wheel provides nothing
    sas.DisconnectFlyByWire();
}
