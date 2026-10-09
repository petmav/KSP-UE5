// PatchedConicSolver (PatchedConicSolver.cs, 377 lines): a vessel's patch list and maneuver-node flight plan.
//   Update            every frame (MonoBehaviour.Update): patches[0] = the driver's orbit from now, then
//                     PatchedConics.CalculatePatch along the chain until FINAL, patchLimit patches, or the first maneuver
//                     node's UT is covered
//   UpdateFlightPlan  sorts the nodes by UT, finds the patch containing the first node, and builds `flightPlan`
//                     (patches up to that one, then per node the post-burn orbit and its own patch chain:
//                     CheckNextManeuver)
//   patch limit       GameSettings.CONIC_PATCH_LIMIT at Awake (min 1), OnGameSettingsApplied, Increase/DecreasePatchLimit;
//                     Update clamps it to [1, maxTotalPatches - 1]
// Unity lifecycle: constructor = Awake (needs the OrbitDriver KSP gets with GetComponent), destructor = OnDestroy,
// StartMoveNext() = the Start coroutine (one call per frame: true while it yields), Update() per frame.
// UI side effects go through static hooks (no-ops when unset): ScreenMessages.PostScreenMessage and the maneuver editor
// refresh (FlightUIModeController in MANEUVER_EDIT/INFO -> ManeuverNodeEditorManager.SetManeuverNodeInitialValues).
// orbit.DrawOrbit() (debug gizmos) is not ported.
//
// The C# lists of references are vectors of shared_ptr: patches[0] is the driver's own Orbit object, patches[1..149]
// are owned by the solver, flightPlan / ManeuverNode::patch / nextPatch share the orbits CheckNextManeuver creates.
// Orbit::nextPatch / previousPatch stay raw pointers (WP-07): they point into the current patch list / flight plan.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/orbit/maneuver_node.hpp"
#include "ksp/orbit/patched_conics.hpp"

namespace ksp {

class CelestialBody;
class ConfigNode;
class Orbit;
class OrbitDriver;

class PatchedConicSolver {
public:
    explicit PatchedConicSolver(OrbitDriver& driver);  // Awake (PatchedConicSolver.cs:71)
    PatchedConicSolver(const PatchedConicSolver&) = delete;
    PatchedConicSolver& operator=(const PatchedConicSolver&) = delete;
    ~PatchedConicSolver();  // OnDestroy (PatchedConicSolver.cs:96)

    // ---- fields (PatchedConicSolver.cs:10-47) ----
    OrbitDriver* obtDriver;
    int maxTotalPatches = 150;
    std::vector<std::shared_ptr<Orbit>> patches;
    std::vector<std::shared_ptr<Orbit>> flightPlan;
    std::vector<std::shared_ptr<ManeuverNode>> maneuverNodes;
    bool MorePatchesAhead = false;
    int patchesAhead = 0;
    // Unused by KSP's code (the solver passes its own SolverParameters, created with the C# defaults); kept as fields.
    int maxGeometrySolverIterations = 25;
    int maxTimeSolverIterations = 50;
    int GeoSolverIterations = 0;
    int TimeSolverIterations1 = 0;
    int TimeSolverIterations2 = 0;
    bool debug_disableEscapeCheck = false;
    double outerReaches = 1E+20;
    CelestialBody* targetBody = nullptr;

    Orbit& orbit() const;                         // PatchedConicSolver.cs:49 => obtDriver.orbit
    std::shared_ptr<Orbit> LastActivePatch() const;  // PatchedConicSolver.cs:51
    int PatchLimit() const { return patchLimit; }   // private patchLimit (read access for hosts and tests)
    PatchedConics::SolverParameters& Parameters() { return parameters; }  // private parameters

    bool StartMoveNext();  // PatchedConicSolver.cs:116 Start() coroutine: one frame per call; false when finished
    void Update();         // PatchedConicSolver.cs:137
    void UpdateFlightPlan();  // PatchedConicSolver.cs:183
    std::shared_ptr<Orbit> FindPatchContainingUT(double UT) const;  // PatchedConicSolver.cs:211
    int FindManeuverIndex(const ManeuverNode& node) const;           // PatchedConicSolver.cs:228
    std::shared_ptr<ManeuverNode> AddManeuverNode(double UT);        // PatchedConicSolver.cs:287
    std::shared_ptr<ManeuverNode> AddManeuverNode(double UT, bool updateFlightPlan);  // PatchedConicSolver.cs:292
    void RemoveManeuverNode(ManeuverNode* node);  // PatchedConicSolver.cs:310
    void IncreasePatchLimit();                    // PatchedConicSolver.cs:337
    void DecreasePatchLimit();                    // PatchedConicSolver.cs:344
    void Load(const ConfigNode& node);            // PatchedConicSolver.cs:350
    void Save(ConfigNode& node) const;            // PatchedConicSolver.cs:364

    // UI hooks
    static inline std::function<void(const std::string& message, float duration)> PostScreenMessage;
    static inline std::function<void(PatchedConicSolver&)> OnManeuverEditorRefresh;

private:
    void OnReferenceBodyChange(CelestialBody* body);  // PatchedConicSolver.cs:105
    void OnGameSettingsApplied();                     // PatchedConicSolver.cs:110
    void CheckNextManeuver(int nodeIdx, const std::shared_ptr<Orbit>& nodePatch, int patchesAhead);  // :248

    int patchLimit = 2;
    PatchedConics::SolverParameters parameters;
    std::vector<std::shared_ptr<ManeuverNode>> flightPlanLoadList;
    bool updateFlightPlan = true;
    // Start coroutine state
    int startState_ = 0;  // 0 not started, 1 suspended at the yield, 2 finished
    int startCount_ = 0;
    int startIndex_ = 0;
};

}  // namespace ksp
