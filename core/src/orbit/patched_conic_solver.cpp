// PatchedConicSolver (PatchedConicSolver.cs); see include/ksp/orbit/patched_conic_solver.hpp.
#include "ksp/orbit/patched_conic_solver.hpp"

#include <algorithm>
#include <cmath>

#include "ksp/celestial/planetarium.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

namespace {
using PT = Orbit::PatchTransitionType;

// double.CompareTo (NaN sorts first, NaN == NaN)
int CompareDouble(double a, double b) {
    if (a < b) return -1;
    if (a > b) return 1;
    if (a == b) return 0;
    if (std::isnan(a)) return std::isnan(b) ? 0 : -1;
    return 1;
}

// double.Equals(double): true for two NaNs
bool DoubleEquals(double a, double b) { return a == b || (std::isnan(a) && std::isnan(b)); }

Vessel* VesselOf(const OrbitDriver* d) { return d->vessel != nullptr ? d->vessel->AsVessel() : nullptr; }
}  // namespace

// PatchedConicSolver.cs:71 Awake
PatchedConicSolver::PatchedConicSolver(OrbitDriver& driver) : obtDriver(&driver) {
    obtDriver->OnReferenceBodyChange += OrbitDriver::CelestialBodyDelegate::Bind(this, &PatchedConicSolver::OnReferenceBodyChange);
    patchLimit = Mathf::Max(GameSettings::CONIC_PATCH_LIMIT, 1);
    patches.reserve(static_cast<size_t>(maxTotalPatches));
    for (int i = 0; i < maxTotalPatches; ++i) {
        if (i == 0)
            patches.push_back(std::shared_ptr<Orbit>(obtDriver->orbit));
        else
            patches.push_back(std::make_shared<Orbit>());
    }
    parameters = PatchedConics::SolverParameters();
    GameEvents::OnGameSettingsApplied.Add(this, &PatchedConicSolver::OnGameSettingsApplied);
}

// PatchedConicSolver.cs:96 OnDestroy
PatchedConicSolver::~PatchedConicSolver() {
    if (obtDriver != nullptr)
        obtDriver->OnReferenceBodyChange -= OrbitDriver::CelestialBodyDelegate::Bind(this, &PatchedConicSolver::OnReferenceBodyChange);
    GameEvents::OnGameSettingsApplied.Remove(this, &PatchedConicSolver::OnGameSettingsApplied);
}

Orbit& PatchedConicSolver::orbit() const { return *obtDriver->orbit; }

// PatchedConicSolver.cs:51
std::shared_ptr<Orbit> PatchedConicSolver::LastActivePatch() const {
    std::shared_ptr<Orbit> result = patches.at(0);
    for (const auto& o : patches) {
        if (!o->activePatch) break;
        result = o;
    }
    return result;
}

// PatchedConicSolver.cs:105
void PatchedConicSolver::OnReferenceBodyChange(CelestialBody* /*body*/) { updateFlightPlan = true; }

// PatchedConicSolver.cs:110
void PatchedConicSolver::OnGameSettingsApplied() {
    if (patchLimit != GameSettings::CONIC_PATCH_LIMIT) {
        patchLimit = Mathf::Max(GameSettings::CONIC_PATCH_LIMIT, 1);
        UpdateFlightPlan();
    }
}

// PatchedConicSolver.cs:116 Start (IEnumerator): `yield return null` before each loaded node is added
bool PatchedConicSolver::StartMoveNext() {
    if (startState_ == 2) return false;
    if (startState_ == 0) {
        startCount_ = static_cast<int>(flightPlanLoadList.size());
        startIndex_ = 0;
    } else {
        std::shared_ptr<ManeuverNode> maneuverNode = flightPlanLoadList.at(static_cast<size_t>(startIndex_));
        maneuverNode->solver = this;
        maneuverNodes.push_back(maneuverNode);
        ++startIndex_;
    }
    if (startIndex_ < startCount_) {
        startState_ = 1;
        return true;
    }
    if (startCount_ > 0) {
        UpdateFlightPlan();
        if (obtDriver->vessel != nullptr) {
            GameEvents::onManeuverAdded.Fire(VesselOf(obtDriver), this);
            GameEvents::onManeuversLoaded.Fire(VesselOf(obtDriver), this);
        }
    }
    startState_ = 2;
    return false;
}

// PatchedConicSolver.cs:137
void PatchedConicSolver::Update() {
    if (Planetarium::Pause() || Time::timeScale == 0.0f) return;
    patchLimit = Mathf::Max(Mathf::Min(maxTotalPatches - 1, patchLimit), 1);
    for (auto& p : patches) p->activePatch = false;
    if (obtDriver->updateMode != OrbitDriver::UpdateMode::IDLE) {
        patches.at(0) = obtDriver->orbit;
        Orbit& p0 = *patches[0];
        p0.patchStartTransition = PT::INITIAL;
        p0.StartUT = Planetarium::GetUniversalTime();
        if (p0.eccentricity < 1.0)
            p0.EndUT = p0.StartUT + p0.period;
        else
            p0.EndUT = p0.period;
        int num = 0;
        do {
            MorePatchesAhead = PatchedConics::CalculatePatch(
                *patches.at(static_cast<size_t>(num)), *patches.at(static_cast<size_t>(num + 1)),
                (num != 0) ? patches[static_cast<size_t>(num)]->epoch : Planetarium::GetUniversalTime(), parameters, targetBody);
            patchesAhead = num;
            ++num;
        } while (MorePatchesAhead &&
                 (num < patchLimit || (!maneuverNodes.empty() && FindPatchContainingUT(maneuverNodes[0]->UT) == nullptr)));
        if (updateFlightPlan) {
            UpdateFlightPlan();
            updateFlightPlan = false;
        }
    }
}

// PatchedConicSolver.cs:183
void PatchedConicSolver::UpdateFlightPlan() {
    if (maneuverNodes.size() <= 0) return;
    dotnet::ListSort(maneuverNodes, [](const std::shared_ptr<ManeuverNode>& m1, const std::shared_ptr<ManeuverNode>& m2) {
        return CompareDouble(m1->UT, m2->UT);  // SortNodesByDate (PatchedConicSolver.cs:332)
    });
    flightPlan.clear();
    std::shared_ptr<Orbit> orbit = FindPatchContainingUT(maneuverNodes[0]->UT);
    if (orbit == nullptr) {
        if (maneuverNodes[0]->UT < Planetarium::GetUniversalTime()) {
            log::Warning("Maneuver Node: removed unsolveable stale Maneuver Node");
            if (PostScreenMessage)
                PostScreenMessage("<color=orange><b>" + KSP::Localization::Localizer::Format("#autoLOC_6002379") + "</b></color>", 15.0f);
            std::shared_ptr<ManeuverNode> keep = maneuverNodes[0];
            keep->RemoveSelf();
        }
        return;
    }
    for (size_t i = 0; i < patches.size() && patches[i] != orbit; ++i) flightPlan.push_back(patches[i]);
    CheckNextManeuver(0, orbit, 1);
    if (OnManeuverEditorRefresh) OnManeuverEditorRefresh(*this);
}

// PatchedConicSolver.cs:211
std::shared_ptr<Orbit> PatchedConicSolver::FindPatchContainingUT(double UT) const {
    const double universalTime = Planetarium::GetUniversalTime();
    for (const auto& orbit : patches) {
        if (!orbit->activePatch) break;
        if ((UT < universalTime && UT <= orbit->StartUT) || (UT >= orbit->StartUT && UT <= orbit->EndUT) ||
            orbit->patchEndTransition == PT::FINAL)
            return orbit;
    }
    return nullptr;
}

// PatchedConicSolver.cs:228
int PatchedConicSolver::FindManeuverIndex(const ManeuverNode& node) const {
    for (size_t num = 0; num < maneuverNodes.size(); ++num)
        if (DoubleEquals(maneuverNodes[num]->UT, node.UT)) return static_cast<int>(num);
    return -1;
}

// PatchedConicSolver.cs:248
void PatchedConicSolver::CheckNextManeuver(int nodeIdx, const std::shared_ptr<Orbit>& nodePatch, int patchesAhead_) {
    flightPlan.push_back(nodePatch);
    const double universalTime = Planetarium::GetUniversalTime();
    std::shared_ptr<Orbit> orbit = std::make_shared<Orbit>();
    bool flag = false;
    if (nodeIdx != 0) flag = PatchedConics::CalculatePatch(*nodePatch, *orbit, nodePatch->epoch, parameters, targetBody);
    if (nodeIdx < static_cast<int>(maneuverNodes.size())) {
        ManeuverNode& maneuverNode = *maneuverNodes[static_cast<size_t>(nodeIdx)];
        if ((maneuverNode.UT < universalTime && maneuverNode.UT <= nodePatch->StartUT) ||
            (maneuverNode.UT > nodePatch->StartUT && maneuverNode.UT < nodePatch->EndUT) ||
            nodePatch->patchEndTransition == PT::FINAL) {
            maneuverNode.patch = nodePatch;
            Orbit::State state;
            nodePatch->GetOrbitalStateVectorsAtUT(maneuverNode.UT, state);
            const Vector3d xzy = state.pos.xzy();
            const Vector3d xzy2 = state.vel.xzy();
            const Vector3d vector3d = QuaternionD::LookRotation(xzy2, Vector3d::Cross(-xzy, xzy2)) * maneuverNode.DeltaV;
            maneuverNode.patch = nodePatch;
            maneuverNode.nextPatch = orbit;
            orbit->previousPatch = nodePatch.get();
            orbit->UpdateFromFixedVectors(xzy.xzy(), xzy2.xzy() + vector3d.xzy(), nodePatch->referenceBody, maneuverNode.UT);
            orbit->patchStartTransition = PT::MANEUVER;
            orbit->StartUT = maneuverNode.UT;
            orbit->EndUT = (orbit->eccentricity < 1.0) ? (maneuverNode.UT + orbit->period) : orbit->period;
            // orbit.DrawOrbit(): debug gizmos, not ported
            if (nodeIdx != 0) {
                nodePatch->patchEndTransition = PT::MANEUVER;
                nodePatch->EndUT = maneuverNode.UT;
                nodePatch->activePatch = true;
            }
            CheckNextManeuver(nodeIdx + 1, orbit, 1);
            return;
        }
    }
    if (flag && (patchesAhead_ < patchLimit || nodeIdx < static_cast<int>(maneuverNodes.size())))
        CheckNextManeuver(nodeIdx, orbit, patchesAhead_ + 1);
}

// PatchedConicSolver.cs:287
std::shared_ptr<ManeuverNode> PatchedConicSolver::AddManeuverNode(double UT) { return AddManeuverNode(UT, true); }

// PatchedConicSolver.cs:292
std::shared_ptr<ManeuverNode> PatchedConicSolver::AddManeuverNode(double UT, bool updateFlightPlan_) {
    auto maneuverNode = std::make_shared<ManeuverNode>();
    maneuverNode->solver = this;
    maneuverNode->DeltaV = Vector3d::zero();
    maneuverNode->UT = UT;
    maneuverNodes.push_back(maneuverNode);
    if (updateFlightPlan_) {
        UpdateFlightPlan();
        if (obtDriver->vessel != nullptr) GameEvents::onManeuverAdded.Fire(VesselOf(obtDriver), this);
    }
    return maneuverNode;
}

// PatchedConicSolver.cs:310
void PatchedConicSolver::RemoveManeuverNode(ManeuverNode* node) {
    auto it = std::find_if(maneuverNodes.begin(), maneuverNodes.end(),
                           [node](const std::shared_ptr<ManeuverNode>& n) { return n.get() == node; });
    if (it == maneuverNodes.end()) {
        log::Warning("Patched Conics Solver: Cannot remove a node not found on the nodes list");
        return;
    }
    const std::shared_ptr<ManeuverNode> keepAlive = *it;  // the caller may be the node itself (RemoveSelf)
    maneuverNodes.erase(it);
    if (maneuverNodes.empty()) {
        flightPlan.clear();
    } else {
        Update();
        UpdateFlightPlan();
    }
    if (obtDriver->vessel != nullptr) GameEvents::onManeuverRemoved.Fire(VesselOf(obtDriver), this);
}

// PatchedConicSolver.cs:337
void PatchedConicSolver::IncreasePatchLimit() {
    ++patchLimit;
    UpdateFlightPlan();
}

// PatchedConicSolver.cs:344
void PatchedConicSolver::DecreasePatchLimit() {
    patchLimit = std::max(1, patchLimit - 1);
    UpdateFlightPlan();
}

// PatchedConicSolver.cs:350
void PatchedConicSolver::Load(const ConfigNode& node) {
    for (ConfigNode* n : node.GetNodes("MANEUVER")) {
        auto maneuverNode = std::make_shared<ManeuverNode>();
        maneuverNode->Load(*n);
        maneuverNode->solver = this;
        flightPlanLoadList.push_back(maneuverNode);
    }
}

// PatchedConicSolver.cs:364
void PatchedConicSolver::Save(ConfigNode& node) const {
    for (const auto& n : maneuverNodes) n->Save(*node.AddNode("MANEUVER"));
}

}  // namespace ksp
