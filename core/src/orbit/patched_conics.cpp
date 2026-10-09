// PatchedConics (PatchedConics.cs); see include/ksp/orbit/patched_conics.hpp.
#include "ksp/orbit/patched_conics.hpp"

#include <cmath>
#include <exception>
#include <limits>
#include <utility>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

namespace {
namespace M = dotnet::Math;
using PT = Orbit::PatchTransitionType;
using ESL = Orbit::EncounterSolutionLevel;

bool IsInfinity(double d) { return std::isinf(d); }

// catch (Exception value) { if (!Thread.CurrentThread.IsBackground) Console.WriteLine(value); }
void PrintException(const std::exception& e) {
    if (!PatchedConics::IsBackgroundThread) log::Info(e.what());
}
}  // namespace

thread_local bool PatchedConics::IsBackgroundThread = false;

PatchedConics::GetClosestApproachDelegate PatchedConics::GetClosestApproach = &PatchedConics::_GetClosestApproach;
PatchedConics::EncountersBodyDelegate PatchedConics::EncountersBody = &PatchedConics::_EncountersBody;
PatchedConics::CheckEncounterDelegate PatchedConics::CheckEncounter = &PatchedConics::_CheckEncounter;
PatchedConics::CalculatePatchDelegate PatchedConics::CalculatePatch = &PatchedConics::_CalculatePatch;

// PatchedConics.cs:99
double PatchedConics::_GetClosestApproach(Orbit& p, const Orbit& s, double startEpoch, double dT, SolverParameters& pars) {
    double num = Orbit::SynodicPeriod(p, s);
    if (std::isnan(num)) {
        const double tA = (!IsInfinity(p.referenceBody->sphereOfInfluence))
                              ? p.TrueAnomalyAtRadius(p.referenceBody->sphereOfInfluence)
                              : ((!(s.eccentricity < 1.0)) ? p.TrueAnomalyAtRadius(pars.outerReaches)
                                                           : p.TrueAnomalyAtRadius(s.semiMajorAxis * 3.0));
        num = 2.0 * p.GetUTforTrueAnomaly(tA, 0.0);
    }
    double num2;
    double maxUT;
    if (num > 0.0) {
        num2 = M::Max(p.UTappr - num / 2.0, startEpoch);
        maxUT = num2 + num;
    } else {
        if (p.timeToPe < 0.0) {  // dead stores in KSP (overwritten below)
            num2 = startEpoch;
            maxUT = num2 - num;
        }
        num2 = M::Max(p.UTappr + num, startEpoch);
        maxUT = num2 - num * 2.0;
    }
    dT = M::Abs(num);
    return Orbit::SolveClosestApproach(p, s, p.UTappr, dT, 0.0, num2, maxUT, pars.epsilon, pars.maxTimeSolverIterations,
                                       pars.TimeSolverIterations1);
}

// PatchedConics.cs:127
bool PatchedConics::_EncountersBody(Orbit& p, const Orbit& s, Orbit& nextPatch, OrbitDriver& sec, double startEpoch,
                                    SolverParameters& pars) {
    if (p.ClAppr < sec.celestialBody->sphereOfInfluence && p.ClAppr != -1.0) {
        p.UTsoi = p.UTappr;
        Orbit::SolveSOI(p, s, p.UTsoi, (p.UTappr - startEpoch) * 0.5, sec.celestialBody->sphereOfInfluence, startEpoch,
                        p.UTappr, pars.epsilon, pars.maxTimeSolverIterations, pars.TimeSolverIterations1);
        Vector3d pos;
        Vector3d vel;
        p.GetOrbitalStateVectorsAtUT(p.UTsoi, pos, vel);
        Vector3d pos2;
        Vector3d vel2;
        s.GetOrbitalStateVectorsAtUT(p.UTsoi, pos2, vel2);
        const Vector3d lhs = pos - pos2;
        const Vector3d rhs = vel - vel2;
        if (Vector3d::Dot(lhs, rhs) >= 0.0) return false;
        if (p.referenceBody == nullptr) throw NullReferenceException("EncountersBody: p.referenceBody");  // _ = p.referenceBody.position
        nextPatch.UpdateFromOrbitAtUT(p, p.UTsoi, sec.celestialBody);
        p.StartUT = startEpoch;
        p.EndUT = p.UTsoi;
        p.patchEndTransition = PT::ENCOUNTER;
        return true;
    }
    return false;
}

// PatchedConics.cs:148
bool PatchedConics::_CheckEncounter(Orbit& p, Orbit& nextPatch, double startEpoch, OrbitDriver& sec, CelestialBody* targetBody,
                                    SolverParameters& pars, bool logErrors) {
    try {
        const Orbit& orbit = *sec.orbit;
        const double num = 1.1;
        if ((!GameSettings::ALWAYS_SHOW_TARGET_APPROACH_MARKERS || !(sec.celestialBody == targetBody)) &&
            !Orbit::PeApIntersects(p, orbit, sec.celestialBody->sphereOfInfluence * num))
            return false;
        if (p.closestEncounterLevel < ESL::ORBIT_INTERSECT) {
            p.closestEncounterLevel = ESL::ORBIT_INTERSECT;
            p.closestEncounterBody = sec.celestialBody;
        }
        double CD = p.ClEctr1;
        double CCD = p.ClEctr2;
        double FFp = p.FEVp;
        double FFs = p.FEVs;
        double SFp = p.SEVp;
        double SFs = p.SEVs;
        int num2 = Orbit::FindClosestPoints(p, orbit, CD, CCD, FFp, FFs, SFp, SFs, 0.0001, pars.maxGeometrySolverIterations,
                                            pars.GeoSolverIterations);
        if (num2 < 1) {
            if (logErrors && !IsBackgroundThread) log::Info("CheckEncounter: failed to find any intercepts at all");
            return false;
        }
        double a = p.GetDTforTrueAnomaly(FFp, 0.0);
        double b = p.GetDTforTrueAnomaly(SFp, 0.0);
        double a2 = a + startEpoch;
        double b2 = b + startEpoch;
        if (IsInfinity(a2) && IsInfinity(b2)) {
            if (logErrors && !IsBackgroundThread) log::Info("CheckEncounter: both intercept UTs are infinite");
            return false;
        }
        if ((!(a2 < p.StartUT) && !(a2 > p.EndUT)) || (!(b2 < p.StartUT) && b2 <= p.EndUT)) {
            if (b2 < a2 || a2 < p.StartUT || a2 > p.EndUT) {
                std::swap(FFp, SFp);  // UtilMath.SwapValues
                std::swap(FFs, SFs);
                std::swap(CD, CCD);
                std::swap(a, b);
                std::swap(a2, b2);
            }
            if (b2 < p.StartUT || b2 > p.EndUT || IsInfinity(b2)) num2 = 1;
            p.numClosePoints = num2;
            p.FEVp = FFp;
            p.FEVs = FFs;
            p.SEVp = SFp;
            p.SEVs = SFs;
            p.ClEctr1 = CD;
            p.ClEctr2 = CCD;
            p.UTappr = a2;
            if (M::Min(p.ClEctr1, p.ClEctr2) > sec.celestialBody->sphereOfInfluence) {
                if ((GameSettings::ALWAYS_SHOW_TARGET_APPROACH_MARKERS || IsBackgroundThread) && sec.celestialBody == targetBody) {
                    p.ClAppr = GetClosestApproach(p, orbit, startEpoch, p.nearestTT * 0.5, pars);
                    p.closestTgtApprUT = p.UTappr;
                }
                return false;
            }
            if (p.closestEncounterLevel < ESL::SOI_INTERSECT_1) {
                p.closestEncounterLevel = ESL::SOI_INTERSECT_1;
                p.closestEncounterBody = sec.celestialBody;
            }
            p.timeToTransition1 = a;
            p.secondaryPosAtTransition1 = orbit.getPositionAtUT(a2);
            p.timeToTransition2 = b;
            p.secondaryPosAtTransition2 = orbit.getPositionAtUT(b2);
            p.nearestTT = p.timeToTransition1;
            p.nextTT = p.timeToTransition2;
            if (std::isnan(p.nearestTT) && !IsBackgroundThread && logErrors)
                log::Info("nearestTT is NaN! t1: " + dotnet::FormatDoubleG(p.timeToTransition1) + ", t2: " +
                          dotnet::FormatDoubleG(p.timeToTransition2) + ", FEVp: " + dotnet::FormatDoubleG(p.FEVp) +
                          ", SEVp: " + dotnet::FormatDoubleG(p.SEVp));
            p.ClAppr = GetClosestApproach(p, orbit, startEpoch, p.nearestTT * 0.5, pars);
            if (EncountersBody(p, orbit, nextPatch, sec, startEpoch, pars)) return true;
            if (sec.celestialBody == targetBody) p.closestTgtApprUT = p.UTappr;
            return false;
        }
        return false;
    } catch (const std::exception& e) {
        PrintException(e);
        return false;
    }
}

// PatchedConics.cs:265
bool PatchedConics::_CalculatePatch(Orbit& p, Orbit& nextPatch, double startEpoch, SolverParameters& pars,
                                    CelestialBody* targetBody) {
    try {
        p.activePatch = true;
        p.nextPatch = &nextPatch;
        p.patchEndTransition = PT::FINAL;
        p.closestEncounterLevel = ESL::NONE;
        p.numClosePoints = 0;
        std::vector<OrbitDriver*>* orbits = Planetarium::Orbits();
        if (orbits == nullptr) throw NullReferenceException("PatchedConics.CalculatePatch: Planetarium.Orbits");
        const size_t count = orbits->size();
        for (size_t i = 0; i < count; ++i) {
            OrbitDriver* orbitDriver = (*orbits)[i];
            if (orbitDriver->orbit.get() != &p && orbitDriver->celestialBody != nullptr) {
                if (targetBody == nullptr || orbitDriver->celestialBody == targetBody) p.closestTgtApprUT = 0.0;
                if (p.referenceBody != nullptr && orbitDriver->referenceBody() == p.referenceBody)
                    CheckEncounter(p, nextPatch, startEpoch, *orbitDriver, targetBody, pars, true);
            }
        }
        if (p.patchEndTransition == PT::FINAL && !pars.debug_disableEscapeCheck) {
            if (!(p.ApR() > p.referenceBody->sphereOfInfluence) && p.eccentricity < 1.0) {
                p.UTsoi = -1.0;
                p.StartUT = startEpoch;
                p.EndUT = startEpoch + p.period;
                p.patchEndTransition = PT::FINAL;
            } else if (IsInfinity(p.referenceBody->sphereOfInfluence)) {
                p.FEVp = M::Acos(0.0 - 1.0 / p.eccentricity);
                p.SEVp = 0.0 - p.FEVp;
                p.StartUT = startEpoch;
                p.EndUT = std::numeric_limits<double>::infinity();
                p.UTsoi = std::numeric_limits<double>::infinity();
                p.patchEndTransition = PT::FINAL;
            } else {
                p.FEVp = p.TrueAnomalyAtRadius(p.referenceBody->sphereOfInfluence);
                p.SEVp = 0.0 - p.FEVp;
                p.timeToTransition1 = p.GetDTforTrueAnomaly(p.FEVp, 0.0);
                p.timeToTransition2 = p.GetDTforTrueAnomaly(p.SEVp, 0.0);
                p.UTsoi = startEpoch + p.timeToTransition1;
                nextPatch.UpdateFromOrbitAtUT(p, p.UTsoi, p.referenceBody->referenceBody());
                p.StartUT = startEpoch;
                p.EndUT = p.UTsoi;
                p.patchEndTransition = PT::ESCAPE;
            }
        }
        nextPatch.StartUT = p.EndUT;
        nextPatch.EndUT = (nextPatch.eccentricity < 1.0) ? (nextPatch.StartUT + nextPatch.period) : nextPatch.period;
        nextPatch.patchStartTransition = p.patchEndTransition;
        nextPatch.previousPatch = &p;
        return p.patchEndTransition != PT::FINAL;
    } catch (const std::exception& e) {
        PrintException(e);
        return false;
    }
}

// PatchedConics.cs:444
double PatchedConics::AngleWrap(double a) {
    const double num = M::Floor(a / (UtilMath::PI * 2.0) + 0.5);
    return a - num * 2.0 * UtilMath::PI;
}

// PatchedConics.cs:450
bool PatchedConics::TAIsWithinPatchBounds(double tA, const Orbit& patch) {
    if ((patch.eccentricity > 1.0 || patch.patchEndTransition != PT::FINAL) && patch.fromV != 0.0 && patch.toV != 0.0) {
        const double num = AngleWrap(tA);
        const double num2 = AngleWrap(patch.fromV);
        const double num3 = AngleWrap(patch.toV);
        if (num2 < num3) {
            if (num < num2 || num > num3) return false;
        } else if (num < num2 && num > num3) {
            return false;
        }
    }
    return true;
}

}  // namespace ksp
