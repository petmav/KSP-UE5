// WP-20e: CollisionEnhancer.cs (see ksp/physics/collision_enhancer.hpp). C# paths are relative to decompiled/KSP-Decompiled/Assembly-CSharp/.
// Float statements follow the Mono rule (managed Vector3 operators of ksp/unity/vector.hpp).
#include "ksp/physics/collision_enhancer.hpp"

#include "ksp/core/log.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::physics {

using GE = events::GameEvents;

// CollisionEnhancer.cs:32 Start (rb = the part's Rigidbody or its parent's: the env's velocity hooks resolve it)
void CollisionEnhancer::Start(Part& p, const Env& env) {
    part = &p;
    lastPos = env.position ? env.position(p) : p.partTransform.position;
    if (part == nullptr && OnTerrainPunchThrough == CollisionEnhancerBehaviour::EXPLODE) OnTerrainPunchThrough = CollisionEnhancerBehaviour::TRANSLATE_BACK;
}

// CollisionEnhancer.cs:47
void CollisionEnhancer::FixedUpdate(const Env& env) {
    lastLinecast = lastHit = lastExploded = false;
    Part& p = *part;
    const Vector3f position = env.position(p);
    if (env.packed(p)) {
        lastPos = position;
        wasPacked = true;
        return;
    }
    if (framesToSkip > 0) {
        lastPos = position;
        framesToSkip--;
        return;
    }
    if (!wasPacked) lastPos = lastPos - env.worldShift().ToVector3f();  // :63 lastPos -= FloatingOrigin.Offset (header comment)
    wasPacked = false;
    if (!bypass && p.State() != PartStates::DEAD &&
        fx((lastPos - position).sqrMagnitude()) > fx(instanceMinDistSqr > 0.0f ? instanceMinDistSqr : minDistSqr)) {
        bool flag = false;
        const double num = 0.0;
        lastLinecast = true;
        const LinecastHit hit = env.linecast(lastPos, position);
        if (hit.hit) {
            const Vector3f rbVelocity = env.velocity(p);
            log::Info("[" + p.gameObjectName + "] Collision Enhancer Punch Through - vel: " + std::to_string(rbVelocity.magnitude()));
            const Vector3f vector = env.upAxis(hit.point);
            const Vector3f frameV3f = env.frameVelocity().ToVector3f();  // Krakensbane.GetFrameVelocityV3f()
            // Vector3.Reflect((v + F).normalized, n * Sign(Dot(n, up))).normalized * (v + F).magnitude * factor - F
            auto translateBackVelocity = [&]() {
                const Vector3f vf = rbVelocity + frameV3f;
                return Vector3f::Reflect(vf.normalized(), hit.normal * Mathf::Sign(Vector3f::Dot(hit.normal, vector))).normalized() *
                           (rbVelocity + frameV3f).magnitude() * translateBackVelocityFactor -
                       frameV3f;
            };
            auto setVelocity = [&](const Vector3f& v) { env.setVelocity(p, VectorExtensions::IsInvalid(v) ? Vector3f() : v); };
            switch (OnTerrainPunchThrough) {
                case CollisionEnhancerBehaviour::EXPLODE:
                    if (!flag && hit.hasRigidbody) {
                        if (fx((rbVelocity - hit.rigidbodyVelocity).magnitude()) * fx(hit.rigidbodyMass) > fx(p.fields.crashTolerance) &&
                            !PartContactEnvironment::noCrashDamage) {
                            EventReport r(FlightEvents::COLLISION, &p, p.partInfo != nullptr ? p.partInfo->title : std::string(), hit.rigidbodyName);
                            GE::onCollision.Fire(&r);
                            lastExploded = true;
                            p.explode();
                            break;
                        }
                        if (!VectorExtensions::IsInvalid(hit.point)) env.setPosition(p, hit.point + vector * upFactor);
                        setVelocity(translateBackVelocity());
                    } else if (fx(rbVelocity.sqrMagnitude()) > fx(p.fields.crashTolerance) * fx(p.fields.crashTolerance) &&
                               !PartContactEnvironment::noCrashDamage) {
                        EventReport r(FlightEvents::COLLISION, &p, p.partInfo != nullptr ? p.partInfo->title : std::string(),
                                      Localizer::Format("#autoLOC_204427"));
                        GE::onCollision.Fire(&r);
                        lastExploded = true;
                        p.explode();
                    } else if (!flag) {
                        if (!VectorExtensions::IsInvalid(hit.point)) env.setPosition(p, hit.point + vector * upFactor);
                        setVelocity(translateBackVelocity());
                    } else {
                        env.setPosition(p, position + (p.vessel->upAxis * num).ToVector3f());
                        env.setVelocity(p, (p.vessel->upAxis * static_cast<double>(translateBackVelocityFactor) - env.frameVelocity()).ToVector3f());
                    }
                    break;
                case CollisionEnhancerBehaviour::TRANSLATE_BACK:
                    if (!flag) {
                        if (!VectorExtensions::IsInvalid(hit.point)) env.setPosition(p, hit.point + vector * upFactor);
                        setVelocity(translateBackVelocity());
                    } else {
                        env.setPosition(p, position + (p.vessel->upAxis * num).ToVector3f());
                        env.setVelocity(p, (p.vessel->upAxis * static_cast<double>(translateBackVelocityFactor) - env.frameVelocity()).ToVector3f());
                    }
                    break;
                case CollisionEnhancerBehaviour::TRANSLATE_BACK_SPLAT:
                    if (!flag) {
                        if (!VectorExtensions::IsInvalid(hit.point)) env.setPosition(p, hit.point + vector * upFactor);
                    } else {
                        env.setPosition(p, position + (p.vessel->upAxis * num).ToVector3f());
                    }
                    env.setVelocity(p, Vector3f());
                    break;
                default:
                    break;
            }
            flag = true;
        }
        if (flag) {
            lastHit = true;
            UnityEngine::RaycastHit rh;
            rh.point = hit.point;
            rh.normal = hit.normal;
            GE::OnCollisionEnhancerHit.Fire(&p, rh);
            GE::onPartExplodeGroundCollision.Fire(&p);
        }
    }
    lastPos = env.position(p);
}

}  // namespace ksp::physics
