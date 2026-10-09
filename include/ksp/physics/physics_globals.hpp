// PhysicsGlobals (PhysicsGlobals.cs, 3,875 lines): every global physics constant and curve of KSP.
//
// In game the object is a MonoBehaviour on the level0 prefab. Its life cycle, reproduced here:
//   1. constructor + Unity deserialization of the level0 prefab  -> the member initialisers below
//      (physics_globals_members.inc, generated from harness/golden/serialized/physicsglobals.serialized.json, whose
//      origin is decompiled/KSP-dev/KSP_x64_Data/level0)
//   2. Awake  (PhysicsGlobals.cs:2736): instance = this; physicsDatabaseFilename = ApplicationRootPath + "Physics.cfg";
//      SurfaceCurves = {DragCurveTail, dragCurveSurface, dragCurveMultiplier, dragCurveTip}
//   3. Start  (PhysicsGlobals.cs:2752): if (!LoadDatabase()) SaveDatabase(); bodyLiftCurve = GetLiftingSurfaceCurve("BodyLift")
//      (Physics.cfg = decompiled/KSP-dev/Physics.cfg in this build)
//   4. OnFlightGlobalsReady -> CalculateValues (PhysicsGlobals.cs:2717): solarLuminosity from the home body's orbit.
//
// KSP reads the values through static properties (PhysicsGlobals.SpaceTemperature, ...). They are mirrored as static
// functions with the same names (PhysicsGlobals::SpaceTemperature(), setters SetSpaceTemperature(v)) acting on the
// process-wide instance (like KSP's singleton; not thread-safe). The fields themselves are public members named exactly
// as in C# (they are private there).
#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ksp/curves/float_curve.hpp"
#include "ksp/curves/serialized_float_curve.hpp"
#include "ksp/physics/vessel_ranges.hpp"
#include "ksp/unity/color.hpp"

namespace ksp {

class ConfigNode;
class CelestialBody;

// UnityEngine.PhysicMaterialCombine
enum class PhysicMaterialCombine : int32_t { Average = 0, Multiply = 1, Minimum = 2, Maximum = 3 };
// Part.ShowRigidAttachmentOption (Part.cs:83)
enum class ShowRigidAttachmentOption : int32_t { Never = 0, Editor = 1, Always = 2 };
// VesselTargetModes.cs
enum class VesselTargetModes : int32_t { None = 0, Direction = 1, DirectionAndVelocity = 2, DirectionVelocityAndOrientation = 3 };

// Enum.ToString() / Enum.Parse(typeof(E), s) for the enums above (Parse: trimmed, case-sensitive name or integer text;
// throws std::invalid_argument like C#'s ArgumentException).
const char* ToString(PhysicMaterialCombine v);
const char* ToString(ShowRigidAttachmentOption v);
const char* ToString(VesselTargetModes v);
template <class E>
E EnumParse(const std::string& text);

// UnityEngine.Gradient exactly as serialized (8 colour keys with their alpha in .a, 16-bit key times, mode, key counts).
// Gradient.Evaluate is native Unity code and is used only for the thermal glow colour (rendering): not ported.
struct Gradient {
    Color key[8];
    uint16_t ctime[8];
    uint16_t atime[8];
    int32_t mode = 0;
    int32_t numColorKeys = 2;
    int32_t numAlphaKeys = 2;
};

class PhysicsGlobals {
public:
    // PhysicsGlobals.cs:8 SurfaceCurvesList: references to the instance's drag curves (shared, as in C#). (no default member initializers: Clang rejects a static inline member of this type otherwise; always value-initialise)
    struct SurfaceCurvesList {
        FloatCurve* dragCurveTail;
        FloatCurve* dragCurveSurface;
        FloatCurve* dragCurveMultiplier;
        FloatCurve* dragCurveTip;
    };

    // PhysicsGlobals.cs:19 LiftingSurfaceCurve (a reference type in C#: held by std::shared_ptr)
    class LiftingSurfaceCurve {
    public:
        std::string name;
        FloatCurve liftCurve;
        FloatCurve liftMachCurve;
        FloatCurve dragCurve;
        FloatCurve dragMachCurve;
        void Load(const ConfigNode& node);  // PhysicsGlobals.cs:32
        void Save(ConfigNode& node) const;  // PhysicsGlobals.cs:61
    };
    // Dictionary<string, LiftingSurfaceCurve>: insertion-ordered (C# Dictionary enumeration order without removals),
    // `dict[name] = v` replaces in place.
    class LiftingSurfaceCurveMap {
    public:
        using Entry = std::pair<std::string, std::shared_ptr<LiftingSurfaceCurve>>;
        std::shared_ptr<LiftingSurfaceCurve> Find(const std::string& name) const;
        void Set(const std::string& name, std::shared_ptr<LiftingSurfaceCurve> value);
        void Clear() { entries_.clear(); }
        size_t Count() const { return entries_.size(); }
        const std::vector<Entry>& Entries() const { return entries_; }

    private:
        std::vector<Entry> entries_;
    };

    // Kind of a field in Fields() (the C# reflection view the golden harness used).
    enum class FieldKind { Double, Float, Int, Bool, String, Enum, Curve, Ranges, LiftingSurfaces, Other };
    struct FieldInfo {
        const char* name;
        bool isStatic;
        bool isConst;
        FieldKind kind;
        const char* csType;  // C# type name as Type.Name reports it ("double", "VesselTargetModes", ...)
        const void* (*get)(const PhysicsGlobals&);
    };
    static const std::vector<FieldInfo>& Fields();  // C# declaration order
    static int32_t EnumValue(const FieldInfo& f, const PhysicsGlobals& pg);
    static const char* EnumName(const FieldInfo& f, const PhysicsGlobals& pg);

    PhysicsGlobals() = default;
    PhysicsGlobals(const PhysicsGlobals&) = delete;
    PhysicsGlobals& operator=(const PhysicsGlobals&) = delete;
    ~PhysicsGlobals();

    // The in-game start-up sequence. applicationRootPath = KSPUtil.ApplicationRootPath (the game folder, with a trailing
    // separator), e.g. "C:/Projects/KSP-UE5/decompiled/KSP-dev/".
    void Awake(const std::string& applicationRootPath);  // PhysicsGlobals.cs:2736 (Shader.PropertyToID omitted)
    void Start();  // PhysicsGlobals.cs:2752: LoadDatabase or SaveDatabase, bodyLiftCurve (GameSettings/EVA parts: shell)
    // PhysicsGlobals.cs:2717 CalculateValues: `bodies` = FlightGlobals.Bodies, `sun` = Planetarium.fetch.Sun (or
    // FlightGlobals.Bodies[0] when there is no Planetarium).
    void CalculateValues(const std::vector<CelestialBody*>& bodies, const CelestialBody* sun);

    bool LoadDatabase();                      // PhysicsGlobals.cs:2809 (file physicsDatabaseFilename)
    void LoadDatabase(const ConfigNode& node);  // PhysicsGlobals.cs:2825 (private in C#)
    void SaveDatabase();                      // PhysicsGlobals.cs:3592 (writes physicsDatabaseFilename)
    void SaveDatabase(ConfigNode& configNode) const;  // the node SaveDatabase() writes
    void LoadLiftingSurfaceCurves(const ConfigNode& subNode);  // PhysicsGlobals.cs:3760
    void LoadDefaultLiftingSurfaceCurves();                    // PhysicsGlobals.cs:3772

    // ---- static API (PhysicsGlobals.cs:2676-2716) ----
    static PhysicsGlobals* Instance() { return instance; }  // KSP: FindObjectOfType when unset
    static void SetInstance(PhysicsGlobals* pg) { instance = pg; }
    static float DragCurveValue(const SurfaceCurvesList& cuves, float dotNormalized, float mach);  // :2692 (Mono float rule)
    static std::shared_ptr<LiftingSurfaceCurve> GetLiftingSurfaceCurve(const std::string& name);  // :2707
    // KpaToAtmospheres (PhysicsGlobals.cs:790): 1 / Planetarium.fetch.Home.atmospherePressureSeaLevel, or 0.009869...
    static double KpaToAtmospheres();
    // JointForce (PhysicsGlobals.cs:1816): +Infinity unless jointForce > 0
    static float JointForce() { return !(Instance()->jointForce > 0.0f) ? std::numeric_limits<float>::infinity() : Instance()->jointForce; }
    static void SetJointForce(float value) { Instance()->jointForce = value; }
    // AeroDataDisplay / AeroGUIDisplay (PhysicsGlobals.cs:707/733): the setters also write GameSettings (shell side).
    static bool AeroDataDisplay() { return Instance()->aeroDataDisplay; }
    static void SetAeroDataDisplay(bool value) { Instance()->aeroDataDisplay = value; }
    static bool AeroGUIDisplay() { return Instance()->aeroGUIDisplay; }
    static void SetAeroGUIDisplay(bool value) { Instance()->aeroGUIDisplay = value; }
    // KerbalEVADragCube (PhysicsGlobals.cs:2084): DRAG_CUBE { cube = KerbalEVADragCubeString }
    static std::shared_ptr<ConfigNode> KerbalEVADragCube();

    // Builds a FloatCurve from Unity-serialized keyframes (used by the prefab initialisers).
    static FloatCurve PrefabCurve(const std::vector<SerializedKeyframe>& keys, int preInfinity, int postInfinity,
                                  float minTime, float maxTime) {
        return MakeSerializedFloatCurve(keys, preInfinity, postInfinity, minTime, maxTime);
    }

    // ---- fields (generated; see header comment of the .inc) ----
#include "ksp/physics/physics_globals_members.inc"

    // ---- static properties (generated) ----
#include "ksp/physics/physics_globals_properties.inc"
};

}  // namespace ksp
