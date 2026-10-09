// PatchedConics (PatchedConics.cs, 472 lines): the patch solver. CalculatePatch closes one conic patch: it looks for SOI
// encounters with every body OrbitDriver in Planetarium.Orbits that shares the patch's reference body (CheckEncounter:
// Orbit.PeApIntersects, Orbit.FindClosestPoints, GetClosestApproach -> Orbit.SolveClosestApproach, EncountersBody ->
// Orbit.SolveSOI + UpdateFromOrbitAtUT), then for an orbit that is still FINAL the escape from the reference body's SOI;
// it fills `nextPatch` and links the two patches (patchStart/EndTransition, StartUT/EndUT, previousPatch/nextPatch).
//
// The four solver entry points are settable static delegates like KSP's (process-wide). Default: the KSP methods.
// Not ported: ScreenCast / ScreenCastWorker / Solve*TA_BSP / PatchCastHit (map-view mouse picking: PlanetariumCamera,
// ScaledSpace, PatchRendering, WP-44).
//
// Exceptions: the C# methods catch every Exception, print it (Console.WriteLine) and return false; kspcore catches
// std::exception, logs it (ksp::log::Info) and returns false.
// Thread.CurrentThread.IsBackground: `IsBackgroundThread` (thread_local, false by default) - a host that runs the solver
// on a worker thread (KSP's transfer planner / alarm clock) sets it there; it changes the logging and makes
// CheckEncounter compute the target-approach marker for a targeted body.
#pragma once

#include "ksp/orbit/orbit.hpp"

namespace ksp {

class CelestialBody;
class OrbitDriver;

class PatchedConics {
public:
    // PatchedConics.cs:8
    struct SolverParameters {
        int maxGeometrySolverIterations = 25;
        int maxTimeSolverIterations = 50;
        int GeoSolverIterations = 0;
        int TimeSolverIterations1 = 0;
        int TimeSolverIterations2 = 0;
        bool FollowManeuvers = false;
        bool debug_disableEscapeCheck = false;
        double outerReaches = 1E+20;
        double epsilon = 0.0001;
    };

    // PatchedConics.cs:31-37 delegate types (C# default argument logErrors = true is explicit here)
    using GetClosestApproachDelegate = double (*)(Orbit& p, const Orbit& s, double startEpoch, double dT,
                                                  SolverParameters& pars);
    using EncountersBodyDelegate = bool (*)(Orbit& p, const Orbit& s, Orbit& nextPatch, OrbitDriver& sec,
                                            double startEpoch, SolverParameters& pars);
    using CheckEncounterDelegate = bool (*)(Orbit& p, Orbit& nextPatch, double startEpoch, OrbitDriver& sec,
                                            CelestialBody* targetBody, SolverParameters& pars, bool logErrors);
    using CalculatePatchDelegate = bool (*)(Orbit& p, Orbit& nextPatch, double startEpoch, SolverParameters& pars,
                                            CelestialBody* targetBody);

    // PatchedConics.cs:87-93
    static GetClosestApproachDelegate GetClosestApproach;
    static EncountersBodyDelegate EncountersBody;
    static CheckEncounterDelegate CheckEncounter;
    static CalculatePatchDelegate CalculatePatch;

    static double _GetClosestApproach(Orbit& p, const Orbit& s, double startEpoch, double dT, SolverParameters& pars);  // :99
    static bool _EncountersBody(Orbit& p, const Orbit& s, Orbit& nextPatch, OrbitDriver& sec, double startEpoch,
                                SolverParameters& pars);  // :127
    static bool _CheckEncounter(Orbit& p, Orbit& nextPatch, double startEpoch, OrbitDriver& sec, CelestialBody* targetBody,
                                SolverParameters& pars, bool logErrors = true);  // :148
    static bool _CalculatePatch(Orbit& p, Orbit& nextPatch, double startEpoch, SolverParameters& pars,
                                CelestialBody* targetBody);  // :265

    static double AngleWrap(double a);                                // PatchedConics.cs:444
    static bool TAIsWithinPatchBounds(double tA, const Orbit& patch);  // PatchedConics.cs:450

    static thread_local bool IsBackgroundThread;  // Thread.CurrentThread.IsBackground
};

}  // namespace ksp
