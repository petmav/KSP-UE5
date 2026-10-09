#include "ksp/physics/physics_globals.hpp"

#include <cmath>
#include <filesystem>
#include <stdexcept>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

namespace {

// bool.Parse: TryParse or FormatException
bool ParseBool(const std::string& s) {
    bool b = false;
    if (!dotnet::TryParseBool(s, b)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
    return b;
}

template <class E, size_t N>
E ParseEnumNames(const std::string& text, const std::pair<const char*, E> (&names)[N], const char* typeName) {
    // Enum.Parse(Type, string): value trimmed; a name (case-sensitive) or an integer (any value is accepted).
    const std::string t = dotnet::Trim(text);
    for (const auto& [n, v] : names)
        if (t == n) return v;
    int32_t i = 0;
    if (!t.empty() && (t[0] == '-' || t[0] == '+' || (t[0] >= '0' && t[0] <= '9')) && dotnet::TryParseInt32(t, i))
        return static_cast<E>(i);
    throw std::invalid_argument(std::string("Requested value '") + text + "' was not found. (" + typeName + ")");
}

constexpr std::pair<const char*, PhysicMaterialCombine> kCombine[] = {{"Average", PhysicMaterialCombine::Average},
                                                                     {"Multiply", PhysicMaterialCombine::Multiply},
                                                                     {"Minimum", PhysicMaterialCombine::Minimum},
                                                                     {"Maximum", PhysicMaterialCombine::Maximum}};
constexpr std::pair<const char*, ShowRigidAttachmentOption> kRigid[] = {{"Never", ShowRigidAttachmentOption::Never},
                                                                       {"Editor", ShowRigidAttachmentOption::Editor},
                                                                       {"Always", ShowRigidAttachmentOption::Always}};
constexpr std::pair<const char*, VesselTargetModes> kTarget[] = {
    {"None", VesselTargetModes::None},
    {"Direction", VesselTargetModes::Direction},
    {"DirectionAndVelocity", VesselTargetModes::DirectionAndVelocity},
    {"DirectionVelocityAndOrientation", VesselTargetModes::DirectionVelocityAndOrientation}};

template <class E, size_t N>
const char* NameOf(E v, const std::pair<const char*, E> (&names)[N]) {
    for (const auto& [n, x] : names)
        if (x == v) return n;
    return "";
}

}  // namespace

const char* ToString(PhysicMaterialCombine v) { return NameOf(v, kCombine); }
const char* ToString(ShowRigidAttachmentOption v) { return NameOf(v, kRigid); }
const char* ToString(VesselTargetModes v) { return NameOf(v, kTarget); }
template <>
PhysicMaterialCombine EnumParse<PhysicMaterialCombine>(const std::string& t) {
    return ParseEnumNames(t, kCombine, "PhysicMaterialCombine");
}
template <>
ShowRigidAttachmentOption EnumParse<ShowRigidAttachmentOption>(const std::string& t) {
    return ParseEnumNames(t, kRigid, "ShowRigidAttachmentOption");
}
template <>
VesselTargetModes EnumParse<VesselTargetModes>(const std::string& t) {
    return ParseEnumNames(t, kTarget, "VesselTargetModes");
}

// ---------------------------------------------------------------------------------------------------------------------
// PhysicsGlobals.cs:32 LiftingSurfaceCurve.Load
void PhysicsGlobals::LiftingSurfaceCurve::Load(const ConfigNode& node) {
    if (auto value = node.GetValue("name")) name = *value;
    if (const ConfigNode* n = node.GetNode("lift")) liftCurve.Load(*n);
    if (const ConfigNode* n = node.GetNode("liftMach")) liftMachCurve.Load(*n);
    if (const ConfigNode* n = node.GetNode("drag")) dragCurve.Load(*n);
    if (const ConfigNode* n = node.GetNode("dragMach")) dragMachCurve.Load(*n);
}

// PhysicsGlobals.cs:61 LiftingSurfaceCurve.Save
void PhysicsGlobals::LiftingSurfaceCurve::Save(ConfigNode& node) const {
    ConfigNode* configNode = node.AddNode("LIFTING_SURFACE");
    configNode->AddValue("name", name);
    liftCurve.Save(*configNode->AddNode(
        "lift", std::string("Converts Sin(AoA) into a lift coefficient (Cl) then multiplied by the below mach multiplier, "
                            "dynamic pressure, the wing area, and the global lift multiplier")));
    liftMachCurve.Save(*configNode->AddNode("liftMach", std::string("Converts mach number into a multiplier to Cl")));
    dragCurve.Save(*configNode->AddNode(
        "drag", std::string("Converts Sin(AoA) into a drag coefficient (Cd) then multiplied by the below mach multiplier, "
                            "dynamic pressure, the wing area, and the global lifting surface drag multiplier")));
    dragMachCurve.Save(*configNode->AddNode("dragMach", std::string("Converts mach number into a multiplier to Cd")));
}

std::shared_ptr<PhysicsGlobals::LiftingSurfaceCurve> PhysicsGlobals::LiftingSurfaceCurveMap::Find(const std::string& name) const {
    for (const auto& [k, v] : entries_)
        if (k == name) return v;
    return nullptr;
}

void PhysicsGlobals::LiftingSurfaceCurveMap::Set(const std::string& name, std::shared_ptr<LiftingSurfaceCurve> value) {
    for (auto& [k, v] : entries_)
        if (k == name) {
            v = std::move(value);
            return;
        }
    entries_.emplace_back(name, std::move(value));
}

// ---------------------------------------------------------------------------------------------------------------------
const std::vector<PhysicsGlobals::FieldInfo>& PhysicsGlobals::Fields() {
    static const std::vector<FieldInfo> fields = {
#include "physics_globals_fields.inc"
    };
    return fields;
}

int32_t PhysicsGlobals::EnumValue(const FieldInfo& f, const PhysicsGlobals& pg) {
    return *static_cast<const int32_t*>(f.get(pg));  // every enum here is int-backed
}

const char* PhysicsGlobals::EnumName(const FieldInfo& f, const PhysicsGlobals& pg) {
    const std::string t = f.csType;
    const void* p = f.get(pg);
    if (t == "PhysicMaterialCombine") return ToString(*static_cast<const PhysicMaterialCombine*>(p));
    if (t == "ShowRigidAttachmentOption") return ToString(*static_cast<const ShowRigidAttachmentOption*>(p));
    if (t == "VesselTargetModes") return ToString(*static_cast<const VesselTargetModes*>(p));
    return "";
}

// PhysicsGlobals.cs:2775 OnDestroy
PhysicsGlobals::~PhysicsGlobals() {
    if (instance == this) {
        instance = nullptr;
        SurfaceCurves = SurfaceCurvesList{};
    }
}

// PhysicsGlobals.cs:2736 Awake (temperaturePropertyID = Shader.PropertyToID("_TemperatureColor") is renderer state)
void PhysicsGlobals::Awake(const std::string& applicationRootPath) {
    instance = this;
    if (physicsDatabaseFilename.find('\\') == std::string::npos && physicsDatabaseFilename.find('/') == std::string::npos)
        physicsDatabaseFilename = applicationRootPath + physicsDatabaseFilename;
    SurfaceCurvesList surfaceCurves{};
    surfaceCurves.dragCurveTail = &DragCurveTail();
    surfaceCurves.dragCurveSurface = &dragCurveSurface;
    surfaceCurves.dragCurveMultiplier = &dragCurveMultiplier;
    surfaceCurves.dragCurveTip = &dragCurveTip;
    SurfaceCurves = surfaceCurves;
}

// PhysicsGlobals.cs:2752 Start. The GameSettings.DEBUG_AERO_* copies, the kerbalEVA drag-cube reload on loaded EVA parts
// and UpdateKerbalEVAPhysicsMaterial belong to the shell / later packages.
void PhysicsGlobals::Start() {
    if (!LoadDatabase()) SaveDatabase();
    bodyLiftCurve = GetLiftingSurfaceCurve("BodyLift");
}

// PhysicsGlobals.cs:2717 CalculateValues
void PhysicsGlobals::CalculateValues(const std::vector<CelestialBody*>& bodies, const CelestialBody* sun) {
    // FlightGlobals.GetHomeBody() (FlightGlobals.cs:1813, with FlightGlobals.fetch present): first isHomeWorld body
    const CelestialBody* celestialBody = nullptr;
    for (const CelestialBody* b : bodies)
        if (b->isHomeWorld) {
            celestialBody = b;
            break;
        }
    const CelestialBody* celestialBody2 = sun != nullptr ? sun : (bodies.empty() ? nullptr : bodies[0]);
    if (celestialBody != nullptr) {
        while (celestialBody->referenceBody() != celestialBody2 && celestialBody->referenceBody() != nullptr)
            celestialBody = celestialBody->referenceBody();
        if (!celestialBody->orbitDriver) throw NullReferenceException("CalculateValues: home root body has no orbit");
        solarLuminosity = dotnet::Math::Pow(celestialBody->orbitDriver->orbit.semiMajorAxis, 2.0) * 4.0 * UtilMath::PI *
                          solarLuminosityAtHome;
    }
}

// PhysicsGlobals.cs:2809
bool PhysicsGlobals::LoadDatabase() {
    LoadDefaultLiftingSurfaceCurves();
    std::error_code ec;
    if (std::filesystem::is_regular_file(dotnet::PathFromUtf8(physicsDatabaseFilename), ec)) {
        if (ConfigNode::NodePtr configNode = ConfigNode::Load(physicsDatabaseFilename)) {
            log::Info("PhysicsGlobals: Loading database");
            LoadDatabase(*configNode);
            return true;
        }
    }
    return false;
}

// PhysicsGlobals.cs:2825 (generated statement list, same order as C#)
void PhysicsGlobals::LoadDatabase(const ConfigNode& node) {
#include "physics_globals_load.inc"
}

// PhysicsGlobals.cs:3592
void PhysicsGlobals::SaveDatabase() {
    log::Info("PhysicsGlobals: Saving database");
    ConfigNode configNode("PHYSICS_DATABASE");
    SaveDatabase(configNode);
    configNode.Save(physicsDatabaseFilename);
}

void PhysicsGlobals::SaveDatabase(ConfigNode& configNode) const {
#include "physics_globals_save.inc"
    dragCurveTip.Save(*configNode.AddNode("DRAG_TIP", std::string("Multiplier to dragcube drag when the face points towards the "
                                                                  "velocity vector, x value is mach, y value is multiplier")));
    dragCurveSurface.Save(*configNode.AddNode("DRAG_SURFACE", std::string("Multiplier to dragcube drag when the face points "
                                                                          "orthogonal to the velocity vector, x value is mach, "
                                                                          "y value is multiplier")));
    dragCurveTail.Save(*configNode.AddNode("DRAG_TAIL", std::string("Multiplier to dragcube drag when the face points away from "
                                                                    "the velocity vector, x value is mach, y value is multiplier")));
    dragCurveMultiplier.Save(*configNode.AddNode("DRAG_MULTIPLIER", std::string("Overall multiplier to drag based on mach")));
    dragCurveCd.Save(*configNode.AddNode("DRAG_CD", std::string("The final Cd of a given facing is the drag cube Cd evalauted "
                                                                "on this curve")));
    dragCurveCdPower.Save(*configNode.AddNode("DRAG_CD_POWER", std::string("The final Cd of a given facing is then raised to this "
                                                                           "power, indexed by mach number")));
    dragCurvePseudoReynolds.Save(*configNode.AddNode("DRAG_PSEUDOREYNOLDS", std::string("Converts a pseudo-Reynolds number "
                                                                                        "(density * velocity) into a multiplier "
                                                                                        "to drag coefficient")));
    ConfigNode* node8 = configNode.AddNode("LIFTING_SURFACE_CURVES", std::string("The lifting surface curvesets available for use"));
    for (const auto& [name, curve] : liftingSurfaceCurves.Entries()) curve->Save(*node8);
    VesselRangesDefault.Save(*configNode.AddNode("VesselRanges"));
}

// PhysicsGlobals.cs:3760
void PhysicsGlobals::LoadLiftingSurfaceCurves(const ConfigNode& subNode) {
    for (ConfigNode* n : subNode.GetNodes("LIFTING_SURFACE")) {
        auto liftingSurfaceCurve = std::make_shared<LiftingSurfaceCurve>();
        liftingSurfaceCurve->Load(*n);
        liftingSurfaceCurves.Set(liftingSurfaceCurve->name, liftingSurfaceCurve);
    }
}

// PhysicsGlobals.cs:3772 LoadDefaultLiftingSurfaceCurves: the built-in curve sets, built as a ConfigNode exactly like C#.
void PhysicsGlobals::LoadDefaultLiftingSurfaceCurves() {
    liftingSurfaceCurves.Clear();
    ConfigNode configNode;
    auto keys = [](ConfigNode* n, std::initializer_list<const char*> ks) {
        for (const char* k : ks) n->AddValue("key", k);
    };
    ConfigNode* configNode2 = configNode.AddNode("LIFTING_SURFACE");
    configNode2->AddValue("name", "Default");
    keys(configNode2->AddNode("lift"), {"0 0 0 1.965926", "0.258819 0.5114774 1.990092 1.905806", "0.5 0.9026583 0.7074468 -0.7074468",
                                        "0.7071068 0.5926583 -2.087948 -1.990095", "1 0 -2.014386 -2.014386"});
    keys(configNode2->AddNode("liftMach"), {"0 1 0 0", "0.3 0.5 -1.671345 -0.8273422", "1 0.125 -0.0005291355 -0.02625772",
                                            "5 0.0625 0 0", "25 0.05 0 0"});
    keys(configNode2->AddNode("drag"), {"0 0.01 0 0", "0.3420201 0.06 0.1750731 0.1750731", "0.5 0.24 2.60928 2.60928",
                                        "0.7071068 1.7 3.349777 3.349777", "1 2.4 1.387938 0"});
    keys(configNode2->AddNode("dragMach"),
         {"0 0.35 0 -0.8463008", "0.15 0.125 0 0", "0.9 0.275 0.541598 0.541598", "1.1 0.75 0 0", "1.4 0.4 -0.3626955 -0.3626955",
          "1.6 0.35 -0.1545923 -0.1545923", "2 0.3 -0.09013031 -0.09013031", "5 0.22 0 0", "25 0.3 0.0006807274 0"});
    ConfigNode* configNode7 = configNode.AddNode("LIFTING_SURFACE");
    configNode7->AddValue("name", "BodyLift");
    keys(configNode7->AddNode("lift"), {"0 0 0 1.975376", "0.309017 0.5877852 1.565065 1.565065", "0.5877852 0.9510565 0.735902 0.735902",
                                        "0.7071068 1 0 0", "0.8910065 0.809017 -2.70827 -2.70827", "1 0 -11.06124 0"});
    keys(configNode7->AddNode("liftMach"), {"0.3 0.167 0 0", "0.8 0.167 0 -0.3904104", "1 0.125 -0.0005291355 -0.02625772",
                                            "5 0.0625 0 0", "25 0.05 0 0"});
    configNode7->AddNode("drag")->AddValue("key", "0 0 0 0");
    configNode7->AddNode("dragMach")->AddValue("key", "0 0 0 0");
    ConfigNode* configNode10 = configNode.AddNode("LIFTING_SURFACE");
    configNode10->AddValue("name", "CapsuleBottom");
    keys(configNode10->AddNode("lift"), {"0 0 0 1.975376", "0.309017 0.5877852 1.565065 1.565065", "0.5877852 0.9510565 0.735902 0.735902",
                                         "0.7071068 1 0 0", "0.8910065 0.809017 -2.70827 -2.70827", "1 0 -11.06124 0"});
    configNode10->AddNode("liftMach")->AddValue("key", "0.3 0.0625 0 0");
    configNode10->AddNode("drag")->AddValue("key", "0 0 0 0");
    configNode10->AddNode("dragMach")->AddValue("key", "0 0 0 0");
    ConfigNode* configNode12 = configNode.AddNode("LIFTING_SURFACE");
    configNode12->AddValue("name", "SpeedBrake");
    configNode12->AddNode("lift")->AddValue("key", "0 0 0 0");
    configNode12->AddNode("liftMach")->AddValue("key", "0 0 0 0");
    keys(configNode12->AddNode("drag"), {"0 0.01 0 0", "0.3420201 0.06 0.1750731 0.1750731", "0.5 0.24 2.60928 2.60928",
                                         "0.7071068 1.7 3.349777 3.349777", "1 2.4 1.387938 0"});
    keys(configNode12->AddNode("dragMach"),
         {"0 0.35 0 -0.8463008", "0.15 0.125 0 0", "0.9 0.275 0.541598 0.541598", "1.1 0.75 0 0", "1.4 0.4 -0.3626955 -0.3626955",
          "1.6 0.35 -0.1545923 -0.1545923", "2 0.3 -0.09013031 -0.09013031", "5 0.22 0 0", "25 0.3 0.0006807274 0"});
    LoadLiftingSurfaceCurves(configNode);
}

// PhysicsGlobals.cs:2692 DragCurveValue (managed float code: Mono float rule)
float PhysicsGlobals::DragCurveValue(const SurfaceCurvesList& cuves, float dotNormalized, float mach) {
    if (fx(dotNormalized) <= 0.5) {
        const float a = cuves.dragCurveTail->Evaluate(mach);
        const float b = cuves.dragCurveSurface->Evaluate(mach);
        const float num = cuves.dragCurveMultiplier->Evaluate(mach);
        return f32store(fx(Mathf::Lerp(a, b, f32store(fx(dotNormalized) * 2.0))) * fx(num));
    }
    const float a2 = cuves.dragCurveSurface->Evaluate(mach);
    const float b2 = cuves.dragCurveTip->Evaluate(mach);
    const float num2 = cuves.dragCurveMultiplier->Evaluate(mach);
    return f32store(fx(Mathf::Lerp(a2, b2, f32store((fx(dotNormalized) - 0.5) * 2.0))) * fx(num2));
}

// PhysicsGlobals.cs:2707
std::shared_ptr<PhysicsGlobals::LiftingSurfaceCurve> PhysicsGlobals::GetLiftingSurfaceCurve(const std::string& name) {
    return Instance()->liftingSurfaceCurves.Find(name);
}

// PhysicsGlobals.cs:790
double PhysicsGlobals::KpaToAtmospheres() {
    if (Planetarium::fetch != nullptr && Planetarium::fetch->Home != nullptr)
        return 1.0 / Planetarium::fetch->Home->atmospherePressureSeaLevel;
    return 0.009869232667160128;
}

// PhysicsGlobals.cs:2084
std::shared_ptr<ConfigNode> PhysicsGlobals::KerbalEVADragCube() {
    auto configNode = std::make_shared<ConfigNode>("DRAG_CUBE");
    configNode->AddValue("cube", KerbalEVADragCubeString());
    return configNode;
}

}  // namespace ksp
