// Vessel: position, altitude, kinematics, mass properties and the atmosphere mirror (Vessel.cs 2652-3130, 3422-3460, 3678-3740;
// VesselPrecalculate.cs 387-498; FlightIntegrator.cs 621-623, 861-914).
#include <cmath>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_atmosphere.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/physics/krakensbane.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace M = dotnet::Math;
using GE = events::GameEvents;

namespace {

// Quaternion / Matrix4x4 native maths (UnityPlayer): plain per-operation float arithmetic, not checked against a capture.
Quaternionf QInverse(const Quaternionf& q) { return {0.0f - q.x, 0.0f - q.y, 0.0f - q.z, q.w}; }  // unit rotations: the conjugate

Matrix4x4f TRSRotation(const Quaternionf& q) {  // Matrix4x4.TRS(Vector3.zero, q, Vector3.one)
    Matrix4x4f m = Matrix4x4f::identity();
    const float x = q.x, y = q.y, z = q.z, w = q.w;
    m.m00 = 1.0f - 2.0f * (y * y + z * z);
    m.m01 = 2.0f * (x * y - z * w);
    m.m02 = 2.0f * (x * z + y * w);
    m.m10 = 2.0f * (x * y + z * w);
    m.m11 = 1.0f - 2.0f * (x * x + z * z);
    m.m12 = 2.0f * (y * z - x * w);
    m.m20 = 2.0f * (x * z - y * w);
    m.m21 = 2.0f * (y * z + x * w);
    m.m22 = 1.0f - 2.0f * (x * x + y * y);
    return m;
}

Matrix4x4f Mul(const Matrix4x4f& a, const Matrix4x4f& b) {  // Matrix4x4.operator*
    Matrix4x4f r;
    r.m00 = a.m00 * b.m00 + a.m01 * b.m10 + a.m02 * b.m20 + a.m03 * b.m30;
    r.m01 = a.m00 * b.m01 + a.m01 * b.m11 + a.m02 * b.m21 + a.m03 * b.m31;
    r.m02 = a.m00 * b.m02 + a.m01 * b.m12 + a.m02 * b.m22 + a.m03 * b.m32;
    r.m03 = a.m00 * b.m03 + a.m01 * b.m13 + a.m02 * b.m23 + a.m03 * b.m33;
    r.m10 = a.m10 * b.m00 + a.m11 * b.m10 + a.m12 * b.m20 + a.m13 * b.m30;
    r.m11 = a.m10 * b.m01 + a.m11 * b.m11 + a.m12 * b.m21 + a.m13 * b.m31;
    r.m12 = a.m10 * b.m02 + a.m11 * b.m12 + a.m12 * b.m22 + a.m13 * b.m32;
    r.m13 = a.m10 * b.m03 + a.m11 * b.m13 + a.m12 * b.m23 + a.m13 * b.m33;
    r.m20 = a.m20 * b.m00 + a.m21 * b.m10 + a.m22 * b.m20 + a.m23 * b.m30;
    r.m21 = a.m20 * b.m01 + a.m21 * b.m11 + a.m22 * b.m21 + a.m23 * b.m31;
    r.m22 = a.m20 * b.m02 + a.m21 * b.m12 + a.m22 * b.m22 + a.m23 * b.m32;
    r.m23 = a.m20 * b.m03 + a.m21 * b.m13 + a.m22 * b.m23 + a.m23 * b.m33;
    r.m30 = a.m30 * b.m00 + a.m31 * b.m10 + a.m32 * b.m20 + a.m33 * b.m30;
    r.m31 = a.m30 * b.m01 + a.m31 * b.m11 + a.m32 * b.m21 + a.m33 * b.m31;
    r.m32 = a.m30 * b.m02 + a.m31 * b.m12 + a.m32 * b.m22 + a.m33 * b.m32;
    r.m33 = a.m30 * b.m03 + a.m31 * b.m13 + a.m32 * b.m23 + a.m33 * b.m33;
    return r;
}

Quaternionf ToFloat(const QuaternionD& q) { return q.ToQuaternionf(); }  // implicit operator Quaternion(QuaternionD)

}  // namespace

// ================================================================== PartRigidbodyState

// rb.worldCenterOfMass = TransformPoint(rb.centerOfMass): Part.CoMOffset is the part's centre of mass in part space
PartRigidbodyState PartRigidbodyState::FromPart(const Part& p) {
    PartRigidbodyState s;
    s.present = p.hasRigidbody;
    s.physicsMass = p.fields.physicsMass;
    s.mass = static_cast<float>(p.fields.physicsMass);
    s.position = p.partTransform.position;
    s.rotation = p.partTransform.rotation;
    s.worldCenterOfMass = p.partTransform.position + p.partTransform.rotation * p.fields.CoMOffset;
    return s;
}

// ================================================================== mass

// Vessel.cs:3422. `part.mass + part.GetResourceMass()` is a float expression widened into the double sum (Mono: no rounding).
float Vessel::GetTotalMass() {
    if (!loaded) {
        totalMass = GetUnloadedVesselMass();
    } else {
        totalMass = 0.0;
        for (Part* part : parts) totalMass += fx(part->fields.mass) + fx(part->GetResourceMass());
    }
    return static_cast<float>(totalMass);
}

namespace {
// FlightIntegrator.cs:700 GetPhysicslessChildMass (a part without a rigid body adds its mass to its parent chain)
float FIGetPhysicslessChildMass(Part& part) {
    float num = 0.0f;
    int count = static_cast<int>(part.children.size());
    while (count-- > 0) {
        Part* part2 = part.children[static_cast<size_t>(count)];
        if (!part2->hasRigidbody)
            num = f32store(fx(num) + (fx(part2->fields.mass) + fx(part2->fields.resourceMass) + fx(FIGetPhysicslessChildMass(*part2))));
    }
    return num;
}
}  // namespace

// FlightIntegrator.cs:452 UpdateMassStats (mass half)
void Vessel::UpdateMassStats() {
    int index = static_cast<int>(parts.size());
    while (index-- > 0) {
        Part* part = parts[static_cast<size_t>(index)];
        part->fields.resourceMass = part->GetResourceMass();
    }
    int index2 = static_cast<int>(parts.size());
    while (index2-- > 0) {
        Part* part = parts[static_cast<size_t>(index2)];
        if (part->hasRigidbody) {
            float num = f32store(fx(part->fields.mass) + fx(part->fields.resourceMass) + fx(FIGetPhysicslessChildMass(*part)));
            const float minMass = part->partInfo->MinimumMass();
            const float absNum = num < 0.0f ? 0.0f - num : num;  // Mathf.Abs
            if (num < minMass) num = minMass;                     // Mathf.Clamp(num, MinimumMass, Abs(num))
            else if (num > absNum) num = absNum;
            part->fields.physicsMass = num;
        } else {
            part->fields.physicsMass = 0.0;
        }
    }
}

// Vessel.cs:3441
double Vessel::GetUnloadedVesselMass() {
    double num = 0.0;
    for (const auto& pps : protoVessel->protoPartSnapshots) {
        num += static_cast<double>(pps->mass);
        for (const auto& r : pps->resources)
            if (r && r->definition != nullptr) num += r->amount * static_cast<double>(r->definition->density());
    }
    return num;
}

// VesselPrecalculate.cs:387
void Vessel::CalculatePhysicsStats(const std::function<PartRigidbodyState(Part&)>& rbOfArg) {
    const std::function<PartRigidbodyState(Part&)>& rbOf = rbOfArg ? rbOfArg : env.partRigidbody;  // WP-19a: the seam when no override
    bool flag = true;
    if (loaded) {
        PartTransform& referenceTransform = ReferenceTransform();
        const int count = static_cast<int>(parts.size());
        const QuaternionD quaternionD = QuaternionD::Inverse(QuaternionD(referenceTransform.rotation));
        Vector3d zero, zero2, zero3;
        double num = 0.0;
        std::vector<PartRigidbodyState> states(static_cast<size_t>(count));
        for (int i = 0; i < count; i++)
            states[static_cast<size_t>(i)] = rbOf ? rbOf(*parts[static_cast<size_t>(i)]) : PartRigidbodyState::FromPart(*parts[static_cast<size_t>(i)]);
        int index = count;
        while (index-- > 0) {
            const PartRigidbodyState& rb = states[static_cast<size_t>(index)];
            if (rb.present) {
                const double physicsMass = rb.physicsMass;
                zero += Vector3d(rb.worldCenterOfMass) * physicsMass;
                const Vector3d vector3d = Vector3d(rb.velocity) * physicsMass;
                zero2 += vector3d;
                zero3 += (quaternionD * Vector3d(rb.angularVelocity)) * physicsMass;
                num += physicsMass;
            }
        }
        if (num > 0.0) {
            flag = false;
            totalMass = num;
            const double num2 = 1.0 / num;
            CoMD = zero * num2;
            rb_velocityD = zero2 * num2;
            velocityD = rb_velocityD + FrameVelocity();
            CoM = CoMD.ToVector3f();
            localCoM = vesselTransform.InverseTransformPoint(CoM);
            rb_velocity = rb_velocityD.ToVector3f();
            angularVelocityD = zero3 * num2;
            angularVelocity = angularVelocityD.ToVector3f();
            if (!(angularVelocityD != Vector3d()) && packed) {
                // VesselPrecalculate.cs:421-424: `MOI.Zero()` / `angularMomentum.Zero()` on Vector3 fields are no-ops (see below)
            } else {
                Matrix4x4f left;  // Matrix4x4.zero
                Matrix4x4f m = Matrix4x4f::identity(), m2 = Matrix4x4f::identity(), m3 = Matrix4x4f::identity();
                const Quaternionf quaternion = ToFloat(quaternionD);
                for (int i = 0; i < count; i++) {
                    const PartRigidbodyState& rb = states[static_cast<size_t>(i)];
                    if (!rb.present) continue;
                    KSPUtil::ToDiagonalMatrix2(rb.inertiaTensor, m);
                    const Quaternionf quaternion2 = quaternion * rb.rotation * rb.inertiaTensorRotation;
                    const Quaternionf q = QInverse(quaternion2);
                    const Matrix4x4f matrix4x = TRSRotation(quaternion2);
                    const Matrix4x4f matrix4x2 = TRSRotation(q);
                    Matrix4x4f right = Mul(Mul(matrix4x, m), matrix4x2);
                    KSPUtil::Add(left, right);
                    const Vector3f vector = referenceTransform.InverseTransformDirection((Vector3d(rb.position) - CoMD).ToVector3f());
                    KSPUtil::ToDiagonalMatrix2(rb.mass * vector.sqrMagnitude(), m2);
                    KSPUtil::Add(left, m2);
                    KSPUtil::OuterProduct2(vector, vector * (0.0f - rb.mass), m3);
                    KSPUtil::Add(left, m3);
                }
                MOI = KSPUtil::Diag(left).ToVector3f();
                angularMomentum.x = static_cast<float>(static_cast<double>(MOI.x) * angularVelocityD.x);
                angularMomentum.y = static_cast<float>(static_cast<double>(MOI.y) * angularVelocityD.y);
                angularMomentum.z = static_cast<float>(static_cast<double>(MOI.z) * angularVelocityD.z);
            }
        }
    }
    if (flag) {
        if (packed) {
            if (LandedOrSplashed()) {
                // VesselPrecalculate.cs:475: worldSurfacePos + worldSurfaceRot * vessel.localCoM (SetLandedPosRot's placement, WP-19a)
                CoMD = precalc.worldSurfacePos + precalc.worldSurfaceRot * Vector3d(localCoM);
            } else {
                if (orbitDriver != nullptr && !orbitDriver->Ready()) {
                    orbitDriver->orbit->Init();
                    orbitDriver->updateFromParameters(false);
                }
                CoMD = mainBody()->position + orbitDriver->pos;
            }
        } else {
            CoMD = Vector3d(vesselTransform.TransformPoint(localCoM));
        }
        CoM = CoMD.ToVector3f();
        Vector3f pointVelocity, rootAngularVelocity;
        if (rootPart != nullptr && env.rootRigidbody && env.rootRigidbody(*this, CoM, pointVelocity, rootAngularVelocity)) {
            // VesselPrecalculate.cs:491-497: rootPart.rb.GetPointVelocity(CoM), Quaternion.Inverse(ReferenceTransform.rotation) * rb.angularVelocity
            rb_velocity = pointVelocity;
            rb_velocityD = Vector3d(rb_velocity);
            velocityD = Vector3d(rb_velocity) + FrameVelocity();
            angularVelocity = QInverse(ReferenceTransform().rotation) * rootAngularVelocity;
            angularVelocityD = Vector3d(angularVelocity);
        } else {
            // VesselPrecalculate.cs:499-505. `rb_velocity.Zero()`, `angularVelocity.Zero()` (Vector3 fields) are the VectorExtensions
            // extension on a COPY in the C#, i.e. no-ops: only the Vector3d fields are cleared.
            rb_velocityD.Zero();
            velocityD.Zero();
            angularVelocityD.Zero();
        }
        // VesselPrecalculate.cs:508-509: `vessel.MOI.Zero()` / `vessel.angularMomentum.Zero()` are the same no-ops on Vector3 fields
    }
    precalc.firstStatsRunComplete = true;
}

// Krakensbane.GetFrameVelocity() (WP-20a): env.krakensbaneFrameVelocity when a host overrides it
Vector3d Vessel::FrameVelocity() const {
    return env.krakensbaneFrameVelocity ? env.krakensbaneFrameVelocity() : Krakensbane::GetFrameVelocity();
}

// ================================================================== position, altitude

// Vessel.cs:2721
double Vessel::PQSAltitude() {
    if (env.pqsSurfaceHeight && mainBody() != nullptr)
        return env.pqsSurfaceHeight(*mainBody(), mainBody()->GetRelSurfaceNVector(latitude, longitude)) - mainBody()->Radius;
    return -1.0;
}

// Vessel.cs:2714
double Vessel::getCorrectedLandedAltitude(double lat, double lon, double alt, CelestialBody& body) {
    if (!env.pqsSurfaceHeight) return alt;
    const double val = env.pqsSurfaceHeight(body, body.GetRelSurfaceNVector(lat, lon)) - body.Radius;
    return M::Max(alt, val);
}

// Vessel.cs:2652 (Physics.Raycast down the up axis: env.heightFromTerrainProbe)
float Vessel::GetHeightFromTerrain(bool overRidePacked) {
    if (loaded && (overRidePacked || !packed)) {
        heightFromTerrain = -1.0f;
        const Vector3f vector = FlightMath::getUpAxis(*mainBody(), Vector3d(vesselTransform.position)).ToVector3f();
        terrainNormal = vesselTransform.InverseTransformDirection(vector);
        float num = static_cast<float>(FlightMath::getAltitudeAtPos(Vector3d(vesselTransform.position), *mainBody()));
        if (num < 0.0f) num = 0.0f - static_cast<float>(PQSAltitude());
        num = f32store(fx(num) + 600.0);
        if (env.heightFromTerrainProbe) {
            Vector3f hitNormal;
            if (std::optional<float> d = env.heightFromTerrainProbe(*this, num, hitNormal)) {
                heightFromTerrain = *d;
                terrainNormal = vesselTransform.InverseTransformDirection(hitNormal);
            }
        }
    }
    return heightFromTerrain;
}

// Vessel.cs:3683
void Vessel::SetPosition(const Vector3d& position, bool usePristineCoords) {
    if (!loaded) {
        vesselTransform.position = position.ToVector3f();
        return;
    }
    if (usePristineCoords) {
        const QuaternionD quaternionD(vesselTransform.rotation);
        for (Part* part : parts) part->partTransform.position = (position + quaternionD * Vector3d(part->fields.orgPos)).ToVector3f();
        SyncVesselTransform();
        return;
    }
    const Vector3d vector3d = position - Vector3d(vesselTransform.position);
    for (Part* part : parts)
        if (part->fields.physicalSignificance == static_cast<int>(0))  // Part.PhysicalSignificance.FULL
            part->partTransform.position = (Vector3d(part->partTransform.position) + vector3d).ToVector3f();
    SyncVesselTransform();
}

// Vessel.cs:3678
void Vessel::SetPosition(const Vector3d& position) { SetPosition(position, packed); }

// Vessel.cs:3713
void Vessel::Translate(const Vector3d& offset) { SetPosition(offset + Vector3d(vesselTransform.position)); }

// Vessel.cs:3718
void Vessel::SetRotation(const Quaternionf& rotation) { SetRotation(rotation, true); }

// Vessel.cs:3723
void Vessel::SetRotation(const Quaternionf& rotation, bool setPos) {
    if (!loaded) {
        vesselTransform.rotation = rotation;
        return;
    }
    for (Part* part : parts) part->partTransform.rotation = rotation * part->fields.orgRot;
    SyncVesselTransform();
    if (setPos) SetPosition(Vector3d(vesselTransform.position), true);
}

void Vessel::SyncVesselTransform() {
    if (loaded && rootPart != nullptr) {
        vesselTransform.position = rootPart->partTransform.position;
        vesselTransform.rotation = rootPart->partTransform.rotation;
    }
}

// ================================================================== kinematics

// Vessel.cs:2845-2865 (the srf_velocity block of UpdatePosVel)
void Vessel::UpdateSurfaceSpeeds() {
    const double sqrMagnitude = srf_velocity.sqrMagnitude();
    if (sqrMagnitude > 0.0) {
        srfSpeed = M::Sqrt(sqrMagnitude);
        srf_vel_direction = srf_velocity / srfSpeed;
        const double num = sqrMagnitude - verticalSpeed * verticalSpeed;
        if (!(num <= 0.0) && !std::isnan(num)) horizontalSrfSpeed = M::Sqrt(num);
        else horizontalSrfSpeed = 0.0;
    } else {
        srfSpeed = 0.0;
        horizontalSrfSpeed = 0.0;
        srf_vel_direction.Zero();
    }
}

// Vessel.cs:2786
void Vessel::UpdatePosVel() {
    CelestialBody& body = *mainBody();
    if (!LandedOrSplashed() || !packed) {
        if (loaded) body.GetLatLonAlt(Vector3d(vesselTransform.position), latitude, longitude, altitude);
        else body.GetLatLonAltOrbital(orbit().pos, latitude, longitude, altitude);
        srfRelRotation = QInverse(ToFloat(body.rotation)) * vesselTransform.rotation;
    }
    if (loaded && !packed) GetHeightFromTerrain();
    else if (!LandedOrSplashed()) heightFromTerrain = -1.0f;
    if (heightFromTerrain != -1.0f) {
        terrainAltitude = altitude - static_cast<double>(heightFromTerrain);
        pqsAltitude = terrainAltitude;
        if (!(altitude < 0.0) && static_cast<double>(heightFromTerrain) >= altitude) radarAltitude = altitude;
        else radarAltitude = heightFromTerrain;
    } else if (!GameSettings::RADAR_ALTIMETER_EXTENDED_CALCS &&
               (!(altitude < static_cast<double>(body.inverseRotThresholdAltitude)) || altitude >= body.minOrbitalDistance - body.Radius)) {
        radarAltitude = altitude;
    } else {
        pqsAltitude = PQSAltitude();
        terrainAltitude = pqsAltitude;
        radarAltitude = altitude - terrainAltitude;
        heightFromTerrain = static_cast<float>(altitude);
        if (pqsAltitude < 0.0) radarAltitude = altitude;
    }
    if (!FlightRegistry::ready) return;
    obt_velocity = orbit().GetRelativeVel();
    obt_speed = obt_velocity.magnitude();
    const CelestialBody* activeRef = FlightRegistry::ActiveVessel != nullptr ? FlightRegistry::ActiveVessel->mainBody() : nullptr;
    srf_velocity = orbit().GetVel(activeRef) - body.getRFrmVelOrbit(orbit());
    upAxis = FlightMath::getUpAxis(body, CoMD);
    verticalSpeed = Vector3d::Dot(obt_velocity, upAxis);
    UpdateSurfaceSpeeds();
    up = (Vector3d(CoM) - body.position).normalized();
    const Vector3f bodyUpRadius = body.transformUp.ToVector3f() * static_cast<float>(body.Radius);
    north = Vector3d::Exclude(up, body.position + Vector3d(bodyUpRadius) - Vector3d(CoM)).normalized();
    east = body.getRFrmVel(Vector3d(CoM)).normalized();
    UpdateDistanceTraveled();
    initialPosVelSet = true;
}

// Vessel.cs:2873
void Vessel::UpdateDistanceTraveled() {
    double num = 0.0;
    switch (situation) {
        default: num = obt_speed; break;
        case Situations::FLYING: num = srfSpeed; break;
        case Situations::LANDED:
        case Situations::SPLASHED:
        case Situations::PRELAUNCH: return;
    }
    distanceTraveled += num * static_cast<double>(env.deltaTime);
}

// Vessel.cs:2892
void Vessel::UpdateAcceleration(double fdtRecip, bool /*fromUpdate*/) {
    if (!FlightRegistry::ready) return;
    const bool flag = mainBody()->rotates && mainBody()->inverseRotation;
    if (loaded && !packed) {
        if (ignoreGeeforceFrames_ == 0) {
            if (!(lastBody != orbit().referenceBody) && (!orbit().referenceBody->rotates || frameWasRotating == flag)) {
                Vessel* eVALadderVessel = EVALadderVessel();
                if (eVALadderVessel == this) {
                    acceleration_immediate = (accelSmoothing_[static_cast<size_t>(accelSmoothingCursor_++)] = (obt_velocity - lastVel) * fdtRecip);
                    if (accelSmoothingCursor_ >= static_cast<int>(accelSmoothing_.size())) accelSmoothingCursor_ = 0;
                    acceleration.Zero();
                    int num = accelSmoothingLength_;
                    while (num-- > 0) acceleration += accelSmoothing_[static_cast<size_t>(num)];
                    acceleration *= accelSmoothingLengthRecip_;
                    perturbation = acceleration - graviticAcceleration;
                    perturbation_immediate = acceleration_immediate - graviticAcceleration;
                    geeForce = perturbation.magnitude() * (1.0 / PhysicsGlobals::GravitationalAcceleration);
                    if (state == State::ACTIVE && ctrlState.mainThrottle > 0.0f)
                        specificAcceleration = geeForce * PhysicsGlobals::GravitationalAcceleration / static_cast<double>(ctrlState.mainThrottle);
                    geeForce_immediate = ((obt_velocity - lastVel) * fdtRecip - graviticAcceleration).magnitude() *
                                         (1.0 / PhysicsGlobals::GravitationalAcceleration);
                } else {
                    int num2 = accelSmoothingLength_;
                    while (num2-- > 0) acceleration = eVALadderVessel->accelSmoothing_[static_cast<size_t>(num2)];
                    perturbation = eVALadderVessel->perturbation;
                    perturbation_immediate = eVALadderVessel->perturbation_immediate;
                    geeForce = eVALadderVessel->geeForce;
                    geeForce_immediate = eVALadderVessel->geeForce_immediate;
                }
            }
        } else {
            ignoreGeeforceFrames_--;
        }
    } else {
        if (LandedOrSplashed()) {
            perturbation = perturbation_immediate = graviticAcceleration * (-1.0 / PhysicsGlobals::GravitationalAcceleration);
            geeForce = geeForce_immediate = perturbation.magnitude();
        } else {
            geeForce = 0.0;
            geeForce_immediate = 0.0;
            perturbation.Zero();
            perturbation_immediate.Zero();
        }
        acceleration_immediate.Zero();
        acceleration.Zero();
        if (ignoreGeeforceFrames_ > 0) ignoreGeeforceFrames_--;
    }
    lastVel = obt_velocity;
    frameWasRotating = flag;
    krakensbaneAcc.Zero();
    lastBody = orbit().referenceBody;
}

// Vessel.cs:2975
void Vessel::IgnoreGForces(int frames) {
    if (ignoreGeeforceFrames_ < frames) ignoreGeeforceFrames_ = frames;
}

// Vessel.cs:2983
void Vessel::IgnoreSpeed(int frames) {
    if (ignoreSpeedFrames_ < frames) ignoreSpeedFrames_ = frames;
}

// Vessel.cs:3104
void Vessel::UpdateCaches() {
    const int count = static_cast<int>(parts.size());
    if ((loaded && crewCachedPartCount_ != count) || (!loaded && crewCachedPartCount_ != static_cast<int>(protoVessel->protoPartSnapshots.size())))
        RebuildCrewList();
    if (resourcesDirty) UpdateResourceSets();
    if (oldPartCount_ == count) return;
    oldPartCount_ = count;
    dockingPorts.clear();
    int index = count;
    while (index-- > 0) {
        Part* part = parts[static_cast<size_t>(index)];
        if (part->fields.PermanentGroundContact) permanentGroundContact = true;
        dockingPorts.insert(dockingPorts.end(), part->dockingPorts.begin(), part->dockingPorts.end());
    }
}

// ================================================================== atmosphere (FlightIntegrator.cs)

// FlightIntegrator.cs:621-623 and CalculateConstantsAtmosphere / CalculateConstantsVacuum (:861-914): the values the integrator
// copies onto its vessel
void Vessel::ApplyFlightIntegratorAtmosphere(const FlightIntegratorAtmosphere& fi) {
    speed = fi.spd;
    staticPressurekPa = fi.staticPressurekPa;
    indicatedAirSpeed = fi.spd * fi.staticPressureAtm;
    if (fi.staticPressurekPa > 0.0) {
        solarFlux = fi.solarFlux;
        atmosphericTemperature = fi.atmosphericTemperature;
        atmDensity = fi.density;
        dynamicPressurekPa = fi.dynamicPressurekPa;
        speedOfSound = fi.speedOfSound;
        mach = fi.mach;
        externalTemperature = fi.externalTemperature;
        convectiveCoefficient = fi.convectiveCoefficient;
    } else {
        atmosphericTemperature = PhysicsGlobals::SpaceTemperature();
        externalTemperature = PhysicsGlobals::SpaceTemperature();
        convectiveCoefficient = 0.0;
        convectiveMachFlux = 0.0;
        atmDensity = 0.0;
        dynamicPressurekPa = 0.0;
        mach = 0.0;
        speedOfSound = 0.0;
        solarFlux = fi.solarFlux;
    }
}

}  // namespace ksp
