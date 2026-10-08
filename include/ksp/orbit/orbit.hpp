// KSP's Orbit class (Assembly-CSharp/Orbit.cs, 1,854 lines), ported operation for operation in double precision.
//
// Frames. Orbit keeps two frames, exactly like KSP:
//   * the "world-like" frame of OrbitFrame / an / pos-argument of UpdateFromFixedVectors: right-handed, z = north
//     (orbit normal of an equatorial prograde orbit), built by CelestialFrame.OrbitalFrame(LAN, inc, argPe);
//   * the "zup local" frame returned by getRelativePositionAtUT / getOrbitalVelocityAtUT / pos / vel / h / eccVec:
//     PlanetariumFrames::Zup().WorldToLocal(world-like vector). Zup = PlanetaryFrame(0, 90, InverseRotAngle) is the
//     identity up to ~6e-17 cross terms while InverseRotAngle = 0.
//   Unity world coordinates are the .xzy swizzle of zup-local vectors (getPositionAtT etc. return
//   referenceBody.position + local.xzy). UpdateFromStateVectors takes zup-local vectors; UpdateFromFixedVectors takes
//   world-like ones.
// Units: metres, seconds; inclination / LAN / argumentOfPeriapsis in degrees; anomalies in radians; semiMajorAxis < 0
// for hyperbolic orbits.
//
// Runtime semantics: every System.Math call goes through ksp::dotnet::Math (x87 Sin/Cos, VS2013 CRT libm) and
// Math.Min/Max keep .NET's NaN rules. Debug.Log / Console.WriteLine go to ksp::log (KSP's
// `!Thread.CurrentThread.IsBackground` guard is treated as "main thread": messages are always written).
//
// KSP quirks kept (marked "KSP quirk" in orbit.cpp):
//   * solveEccentricAnomaly ignores its `ecc` argument: the branch and the value handed to the solvers are the
//     eccentricity FIELD (e < 0.8 Newton with |dE| <= maxError, unbounded; 0.8 <= e < 1 eight fixed Laguerre-type
//     iterations; e >= 1 hyperbolic Newton with |dE| <= maxError). Math.Sign(NaN) in the 0.8..1 solver throws in
//     C#; KSP catches it and returns NaN (so does this port).
//   * GetTimeToPeriapsis returns timeToPe - period when the patch is not FINAL and StartUT + timeToPe > EndUT, which is
//     always the case for a fresh Orbit (patchEndTransition = INITIAL, StartUT = EndUT = 0).
//   * UpdateFromStateVectors' NaN-argPe fallback stores Math.Acos(...) in RADIANS into argumentOfPeriapsis.
//   * GetRadiusAtPhaseAngle / PhaseAngle use the float Deg2Rad (0.01745329238474369).
//   * Init() does not set timeToPe / timeToAp / radius / pos / vel (only UpdateFromUT / UpdateFromFixedVectors do).
//
// Not ported: DrawOrbit (Unity Debug.DrawLine gizmos), State.DumpData (empty in KSP). Targeting-based
// FindClosestPoints (Orbit.cs:1385, needs Targeting.cs) is reached through Orbit::FindClosestPoints, which defaults to
// FindClosestPointsDefault (see there).
#pragma once

#include <string>

#include "ksp/math/util_math.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/orbit/celestial_body_ref.hpp"
#include "ksp/orbit/planetarium_frames.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Orbit {
public:
    // Orbit.cs:10
    enum class ObjectType { VESSEL, SPACE_DEBRIS, CELESTIAL_BODIES, UNKNOWN_MISC, KERBAL };
    // Orbit.cs:108
    enum class EncounterSolutionLevel { NONE, ESCAPE, ORBIT_INTERSECT, SOI_INTERSECT_2, SOI_INTERSECT_1 };
    // Orbit.cs:117
    enum class PatchTransitionType { INITIAL, FINAL, ENCOUNTER, ESCAPE, MANEUVER, IMPACT };

    // Orbit.cs:21 Orbit.State (world-like frame, not zup-local)
    struct State {
        Vector3d pos;
        Vector3d vel;
        Vector3d acc;
        Vector3d jrk;
        double w = 0.0;
        void DumpData(const std::string& /*name*/, double /*UT*/) const {}  // Orbit.cs:33 (empty in KSP)
    };

    // Orbit.cs:38 Orbit.CASolutionState (closest-approach solver state)
    struct CASolutionState {
        const Orbit* p = nullptr;
        const Orbit* s = nullptr;
        State pstate;
        State sstate;
        State rstate;
        double rdv = 0.0;
        double drdv = 0.0;
        double ddrdv = 0.0;
        double MaxDT = 0.0;
        double maxdt = 0.0;

        // Orbit.cs:60
        bool targetAhead() const { return Vector3d::Dot(rstate.pos, pstate.vel) < 0.0; }
        // Orbit.cs:62
        CASolutionState(const Orbit* p_, const Orbit* s_, double MaxDT_);
        // Orbit.cs:81
        void Update(double t, int& iter, bool dump = false);
        // Orbit.cs:90
        double Halley_dt() const;
        // Orbit.cs:95
        double Clamp_dt(double dt) const;
    };

    // Orbit.cs:19 / 102 / 104 / 106 delegate types; the static members below are settable like KSP's.
    using FindClosestPointsDelegate = int (*)(const Orbit& p, const Orbit& s, double& CD, double& CCD, double& FFp,
                                              double& FFs, double& SFp, double& SFs, double epsilon, int maxIterations,
                                              int& iterationCount);
    using SolveClosestApproachDelegate = double (*)(const Orbit& p, const Orbit& s, double& UT, double dT,
                                                    double threshold, double MinUT, double MaxUT, double epsilon,
                                                    int maxIterations, int& iterationCount);
    using SolveSOI_Delegate = bool (*)(const Orbit& p, const Orbit& s, double& UT, double dT, double Rsoi, double MinUT,
                                       double MaxUT, double epsilon, int maxIterations, int& iterationCount);
    using SolveSOI_BSPDelegate = SolveSOI_Delegate;

    // ---- fields (Orbit.cs:126-306, same names and order) ---------------------------------------------------------
    CelestialBodyRef* referenceBody = nullptr;
    double inclination = 0.0;
    double eccentricity = 0.0;
    double semiMajorAxis = 0.0;
    double LAN = 0.0;
    double argumentOfPeriapsis = 0.0;
    double epoch = 0.0;
    static constexpr double Rad2Deg = 180.0 / 3.141592653589793;  // Orbit.cs:140
    static constexpr double Deg2Rad = 3.141592653589793 / 180.0;  // Orbit.cs:142
    CelestialFrame OrbitFrame;
    Vector3d pos;
    Vector3d vel;
    double orbitalEnergy = 0.0;
    double meanAnomaly = 0.0;
    double trueAnomaly = 0.0;
    double eccentricAnomaly = 0.0;
    double radius = 0.0;
    double altitude = 0.0;
    double orbitalSpeed = 0.0;
    double orbitPercent = 0.0;
    double ObT = 0.0;
    double ObTAtEpoch = 0.0;
    double timeToPe = 0.0;
    double timeToAp = 0.0;
    ObjectType objectType = ObjectType::UNKNOWN_MISC;  // [Obsolete] in KSP
    Vector3d h;
    Vector3d eccVec;
    Vector3d an;
    double meanAnomalyAtEpoch = 0.0;
    double meanMotion = 0.0;
    double period = 0.0;
    Vector3f OrbitFrameX;  // float copies (UnityEngine.Vector3) written by UpdateFromFixedVectors
    Vector3f OrbitFrameY;
    Vector3f OrbitFrameZ;
    Vector3f debugPos;
    Vector3f debugVel;
    Vector3f debugH;
    Vector3f debugAN;
    Vector3f debugEccVec;
    double mag = 0.0;
    int numClosePoints = 0;
    double FEVp = 0.0;
    double FEVs = 0.0;
    double SEVp = 0.0;
    double SEVs = 0.0;
    double UTappr = 0.0;
    double UTsoi = 0.0;
    double ClAppr = 0.0;
    double CrAppr = 0.0;
    double ClEctr1 = 0.0;
    double ClEctr2 = 0.0;
    double timeToTransition1 = 0.0;
    double timeToTransition2 = 0.0;
    double nearestTT = 0.0;
    double nextTT = 0.0;
    Vector3d secondaryPosAtTransition1;
    Vector3d secondaryPosAtTransition2;
    double closestTgtApprUT = 0.0;
    // Patched-conic bookkeeping (data only here; PatchedConics / PatchedConicSolver are WP-08).
    double StartUT = 0.0;
    double EndUT = 0.0;
    bool activePatch = false;
    Orbit* closestEncounterPatch = nullptr;
    CelestialBodyRef* closestEncounterBody = nullptr;
    EncounterSolutionLevel closestEncounterLevel = EncounterSolutionLevel::NONE;
    PatchTransitionType patchStartTransition = PatchTransitionType::INITIAL;
    PatchTransitionType patchEndTransition = PatchTransitionType::INITIAL;
    Orbit* nextPatch = nullptr;
    Orbit* previousPatch = nullptr;
    double fromE = 0.0;
    double toE = 0.0;
    double sampleInterval = 0.0;
    double E = 0.0;
    double V = 0.0;
    double fromV = 0.0;
    double toV = 0.0;
    bool debug_returnFullEllipseTrajectory = false;

    // Orbit.cs:210-216 static delegates (process-wide, settable). Defaults: the KSP implementations below.
    static FindClosestPointsDelegate FindClosestPoints;
    static SolveClosestApproachDelegate SolveClosestApproach;
    static SolveSOI_Delegate SolveSOI;
    static SolveSOI_BSPDelegate SolveSOI_BSP;
    // Stand-in for GameSettings.LEGACY_ORBIT_TARGETING (read by FindClosestPointsDefault; KSP default false).
    static bool LegacyOrbitTargeting;

    // ---- construction ---------------------------------------------------------------------------------------------
    // Orbit.cs:312 (all fields default). Copying an Orbit copies every field (C# MemberwiseClone semantics, including
    // the patch pointers); KSP's element-only copy constructor is Orbit::CopyOf.
    Orbit() = default;
    // Orbit.cs:316 -> SetOrbit -> Init
    Orbit(double inc, double e, double sma, double lan, double argPe, double mEp, double t, CelestialBodyRef* body);
    // Orbit.cs:321 `new Orbit(orbit)`: copies the seven elements and the reference body, then Init().
    static Orbit CopyOf(const Orbit& orbit);

    // ---- derived elements -----------------------------------------------------------------------------------------
    double semiMinorAxis() const;      // Orbit.cs:282
    double semiLatusRectum() const;    // Orbit.cs:294
    double PeR() const;                // Orbit.cs:296
    double ApR() const;                // Orbit.cs:298
    double PeA() const;                // Orbit.cs:300
    double ApA() const;                // Orbit.cs:302

    double GetMeanMotion(double sma) const;  // Orbit.cs:334
    void SetOrbit(double inc, double e, double sma, double lan, double argPe, double mEp, double t,
                  CelestialBodyRef* body);   // Orbit.cs:340
    void Init();                             // Orbit.cs:353
    static double SafeAcos(double c);        // Orbit.cs:380

    // ---- updates --------------------------------------------------------------------------------------------------
    void UpdateFromOrbitAtUT(const Orbit& orbit, double UT, CelestialBodyRef* toBody);              // Orbit.cs:393
    void UpdateFromStateVectors(Vector3d pos, Vector3d vel, CelestialBodyRef* refBody, double UT);  // Orbit.cs:423
    void UpdateFromFixedVectors(Vector3d pos, Vector3d vel, CelestialBodyRef* refBody, double UT);  // Orbit.cs:448
    void UpdateFromUT(double UT);                                                                   // Orbit.cs:541

    // ---- time <-> anomaly -----------------------------------------------------------------------------------------
    double GetDTforTrueAnomalyAtUT(double tA, double UT) const;               // Orbit.cs:575
    double GetDTforTrueAnomaly(double tA, double wrapAfterSeconds) const;     // Orbit.cs:599
    double GetUTforTrueAnomaly(double tA, double wrapAfterSeconds) const;     // Orbit.cs:624
    double getObtAtUT(double UT) const;                                       // Orbit.cs:649
    double getObTAtMeanAnomaly(double M) const;                               // Orbit.cs:676
    double getObtAtTrueAnomaly(double tA) const;                              // Orbit.cs:681
    double GetTimeToPeriapsis() const;                                        // Orbit.cs:746
    double GetEccentricAnomaly(double tA) const;                              // Orbit.cs:808
    double GetMeanAnomaly(double E) const;                                    // Orbit.cs:828
    double RadiusAtTrueAnomaly(double tA) const;                              // Orbit.cs:841
    double TrueAnomalyAtRadiusSimple(double R) const;                         // Orbit.cs:846 (internal)
    double TrueAnomalyAtRadius(double R) const;                               // Orbit.cs:851
    double TrueAnomalyAtUT(double UT) const;                                  // Orbit.cs:863
    double TrueAnomalyAtT(double T) const;                                    // Orbit.cs:868
    double EccentricAnomalyAtUT(double UT) const;                             // Orbit.cs:879
    double EccentricAnomalyAtObT(double T) const;                             // Orbit.cs:884
    double solveEccentricAnomaly(double M, double ecc, double maxError = 1E-07, int maxIterations = 8) const;  // :890
    double GetTrueAnomaly(double E) const;                                    // Orbit.cs:959
    double GetTrueAnomalyOfZupVector(const Vector3d& vector) const;           // Orbit.cs:980
    double TimeOfTrueAnomaly(double tA, double UT) const;                     // Orbit.cs:1010
    double getUTAtMeanAnomaly(double M, double UT) const;                     // Orbit.cs:1015
    double getMeanAnomalyAtUT(double UT) const;                               // Orbit.cs:1026
    double GetNextPeriapsisTime(double UT) const;                             // Orbit.cs:1286
    double GetNextApoapsisTime(double UT) const;                              // Orbit.cs:1295
    double GetNextTimeOfRadius(double UT, double radius) const;               // Orbit.cs:1304

    // ---- positions and velocities ---------------------------------------------------------------------------------
    Vector3d getPositionAtUT(double UT) const;                                // Orbit.cs:629 (world: body.position + .xzy)
    Vector3d getTruePositionAtUT(double UT) const;                            // Orbit.cs:634
    Vector3d getRelativePositionAtUT(double UT) const;                        // Orbit.cs:639 (zup local)
    Vector3d getOrbitalVelocityAtUT(double UT) const;                         // Orbit.cs:644 (zup local)
    Vector3d GetOrbitNormal() const;                                          // Orbit.cs:687
    Vector3d GetEccVector() const;                                            // Orbit.cs:692
    Vector3d GetANVector() const;                                             // Orbit.cs:697
    // Orbit.cs:702 GetVel: KSP subtracts FlightGlobals.ActiveVessel.orbitDriver.referenceBody.GetFrameVel() when there
    // is an active vessel; the caller passes that body (nullptr = no active vessel -> Vector3d.zero).
    Vector3d GetVel(const CelestialBodyRef* activeVesselReferenceBody = nullptr) const;
    Vector3d GetRelativeVel() const;                                          // Orbit.cs:708
    Vector3d GetRotFrameVel(const CelestialBodyRef& refBody) const;           // Orbit.cs:713
    Vector3d GetRotFrameVelAtPos(const CelestialBodyRef& refBody, const Vector3d& refPos) const;  // Orbit.cs:722
    Vector3d GetFrameVel() const;                                             // Orbit.cs:731
    Vector3d GetFrameVelAtUT(double UT) const;                                // Orbit.cs:736
    // Orbit.cs:741 (activeVesselReferenceBody: see GetVel)
    Vector3d GetWorldSpaceVel(const CelestialBodyRef* activeVesselReferenceBody = nullptr) const;
    double GetRadiusAtUT(double time) const;                                  // Orbit.cs:755
    double GetRadiusAtPhaseAngle(double phaseAngle) const;                    // Orbit.cs:761
    double PhaseAngle(double time, double longitude) const;                   // Orbit.cs:767
    double PhaseAngle(double time) const;                                     // Orbit.cs:776
    double GetRelativeInclination(const Orbit& otherOrbit) const;             // Orbit.cs:789 (degrees)
    double TrueAnomalyFromVector(const Vector3d& vec) const;                  // Orbit.cs:794
    Vector3d getPositionAtT(double T) const;                                  // Orbit.cs:986
    Vector3d getRelativePositionAtT(double T) const;                          // Orbit.cs:991
    Vector3d getPositionFromMeanAnomaly(double M) const;                      // Orbit.cs:999
    Vector3d getRelativePositionFromMeanAnomaly(double M) const;              // Orbit.cs:1004
    Vector3d getPositionFromEccAnomaly(double E) const;                       // Orbit.cs:1036
    Vector3d getRelativePositionFromEccAnomaly(double E) const;               // Orbit.cs:1041
    Vector3d getPositionFromEccAnomalyWithSemiMinorAxis(double E, double semiMinorAxis) const;          // :1064
    Vector3d getRelativePositionFromEccAnomalyWithSemiMinorAxis(double E, double semiMinorAxis) const;  // :1069
    Vector3d getPositionFromTrueAnomaly(double tA) const;                     // Orbit.cs:1092
    Vector3d getRelativePositionFromTrueAnomaly(double tA) const;             // Orbit.cs:1097
    Vector3d getPositionFromTrueAnomaly(double tA, bool worldToLocal) const;  // Orbit.cs:1102
    double getOrbitalSpeedAt(double time) const;                              // Orbit.cs:1114
    double getOrbitalSpeedAtRelativePos(const Vector3d& relPos) const;        // Orbit.cs:1119
    double getOrbitalSpeedAtPos(const Vector3d& pos) const;                   // Orbit.cs:1124
    double getOrbitalSpeedAtDistance(double d) const;                         // Orbit.cs:1129
    Vector3d getOrbitalVelocityAtObT(double ObT) const;                       // Orbit.cs:1134
    Vector3d getOrbitalVelocityAtTrueAnomaly(double tA) const;                // Orbit.cs:1139
    Vector3d getOrbitalVelocityAtTrueAnomaly(double tA, bool worldToLocal) const;  // Orbit.cs:1144
    double GetOrbitalStateVectorsAtUT(double UT, Vector3d& pos, Vector3d& vel) const;                   // :1159
    double GetOrbitalStateVectorsAtObT(double ObT, double UT, Vector3d& pos, Vector3d& vel) const;      // :1164
    double GetOrbitalStateVectorsAtTrueAnomaly(double tA, double UT, Vector3d& pos, Vector3d& vel) const;  // :1169
    double GetOrbitalStateVectorsAtTrueAnomaly(double tA, double UT, bool worldToLocal, Vector3d& pos,
                                               Vector3d& vel) const;          // Orbit.cs:1174
    double GetOrbitalStateVectorsAtUT(double UT, State& state) const;         // Orbit.cs:1200
    double GetOrbitalStateVectorsAtObT(double ObT, State& state) const;       // Orbit.cs:1205
    double GetOrbitalStateVectorsAtTrueAnomaly(double tA, State& state) const;  // Orbit.cs:1216
    double GetOrbitalCurvatureAtTrueAnomaly(double ta) const;                 // Orbit.cs:1244
    Vector3d Prograde(double UT) const;                                       // Orbit.cs:1261 (world .xzy)
    Vector3d Radial(double UT) const;                                         // Orbit.cs:1266
    Vector3d Normal(double UT) const;                                         // Orbit.cs:1271
    Vector3d Up(double UT) const;                                             // Orbit.cs:1276
    Vector3d Horizontal(double UT) const;                                     // Orbit.cs:1281

    // ---- static helpers -------------------------------------------------------------------------------------------
    // Orbit.cs:1365 (pos/vel in Unity world coordinates)
    static Orbit OrbitFromStateVectors(const Vector3d& pos, const Vector3d& vel, CelestialBodyRef* body, double UT);
    static bool PeApIntersects(const Orbit& primary, const Orbit& secondary, double threshold);  // Orbit.cs:1372
    // Orbit.cs:1385 _FindClosestPoints. With LegacyOrbitTargeting it runs FindClosestPoints_old and returns 2 (as KSP).
    // The default path (Targeting.Conic / Targeting.Intercepts, Targeting.cs) is not ported yet: it throws
    // std::logic_error. Install another implementation through Orbit::FindClosestPoints if needed.
    static int FindClosestPointsDefault(const Orbit& p, const Orbit& s, double& CD, double& CCD, double& FFp,
                                        double& FFs, double& SFp, double& SFs, double epsilon, int maxIterations,
                                        int& iterationCount);
    // Orbit.cs:1430
    static void FindClosestPoints_old(const Orbit& p, const Orbit& s, double& CD, double& CCD, double& FFp, double& FFs,
                                      double& SFp, double& SFs, double epsilon, int maxIterations, int& iterationCount);
    // Orbit.cs:1495 (returns Dot(rstate.vel, rstate.pos))
    static double RelativeStateAtUT(const Orbit& p, const Orbit& s, double UT, State& pstate, State& sstate,
                                    State& rstate, bool dump = false);
    static double SynodicPeriod(const Orbit& o1, const Orbit& o2);           // Orbit.cs:1513
    // Orbit.cs:1530 _SolveClosestApproach
    static double SolveClosestApproachDefault(const Orbit& p, const Orbit& s, double& UT, double dT, double threshold,
                                              double MinUT, double MaxUT, double epsilon, int maxIterations,
                                              int& iterationCount);
    // Orbit.cs:1648 _SolveSOI
    static bool SolveSOIDefault(const Orbit& p, const Orbit& s, double& UT, double dT, double Rsoi, double MinUT,
                                double MaxUT, double epsilon, int maxIterations, int& iterationCount);
    // Orbit.cs:1680 _SolveSOI_BSP
    static bool SolveSOI_BSPDefault(const Orbit& p, const Orbit& s, double& UT, double dT, double Rsoi, double MinUT,
                                    double MaxUT, double epsilon, int maxIterations, int& iterationCount);
    static double NextCloseApproachTime(const Orbit& p, const Orbit& s, double UT);  // Orbit.cs:1731
    static double AscendingNodeTrueAnomaly(const Orbit& p, const Orbit& s);           // Orbit.cs:1766
    static double DescendingNodeTrueAnomaly(const Orbit& p, const Orbit& s);          // Orbit.cs:1773
    static double RelativeInclination(const Orbit& p, const Orbit& s);                // Orbit.cs:1778 (degrees)
    // Orbit.cs:1783-1845: UnityEngine.Random draws come from UnityRandom::Global().
    static Orbit CreateRandomOrbitAround(CelestialBodyRef* body);
    static Orbit CreateRandomOrbitAround(CelestialBodyRef* body, double minAltitude, double maxAltitude);
    static Orbit CreateRandomOrbitNearby(const Orbit& baseOrbit);
    // Orbit.cs:1828/1835: KSP reads Planetarium.GetUniversalTime(); the caller passes it as universalTime.
    static Orbit CreateRandomOrbitFlyBy(const CelestialBodyRef& tgtBody, double daysToClosestApproach,
                                        double universalTime);
    static Orbit CreateRandomOrbitFlyBy(const Orbit& targetOrbit, double timeToPeriapsis, double periapsis,
                                        double deltaVatPeriapsis, double universalTime);
    static Vector3d Swizzle(Vector3d vec);                                    // Orbit.cs:1847

private:
    double solveEccentricAnomalyStd(double M, double ecc, double maxError = 1E-07) const;    // Orbit.cs:903
    double solveEccentricAnomalyExtremeEcc(double M, double ecc, int iterations = 8) const;  // Orbit.cs:916
    double solveEccentricAnomalyHyp(double M, double ecc, double maxError = 1E-07) const;    // Orbit.cs:942
    double trueAnomalyAtRadiusExtreme(double R) const;                                      // Orbit.cs:856
    static double SolveClosestBSP(double& Fp, double& Fs, double Ir, double dF, const Orbit& p, const Orbit& s,
                                  double epsilon, int maxIterations, int& iterationCount);  // Orbit.cs:1460
    static double Separation(const Orbit& p, const Orbit& s, double UT);                    // Orbit.cs:1761

    double drawResolution = 15.0;  // Orbit.cs:208 (DrawOrbit only)
};

// Enum names as C# ToString() writes them (save files, golden tables).
const char* ToString(Orbit::PatchTransitionType t);
const char* ToString(Orbit::EncounterSolutionLevel l);
const char* ToString(Orbit::ObjectType t);

}  // namespace ksp
