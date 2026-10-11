// Jolt spike: one Jolt PhysicsSystem with KSP's layer rules (PHYSICS-SPIKE.md).
//
// Layers: STATIC (terrain quads, KSC) and MOVING (vessel bodies). Same-vessel collision suppression (KSP's CollisionManager
// O(C^2) Physics.IgnoreCollision rebuild, Assembly-CSharp/CollisionManager.cs:126-168) is a Jolt GroupFilter on the body's
// CollisionGroup: group id = vessel id, so the rule costs O(1) per candidate pair and nothing on a structural event (lever 7).
#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/GroupFilter.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <memory>

namespace spike {

namespace Layers {
constexpr JPH::ObjectLayer STATIC = 0;
constexpr JPH::ObjectLayer MOVING = 1;
constexpr JPH::ObjectLayer NUM = 2;
}  // namespace Layers

// Bodies of the same vessel (same CollisionGroup group id) never collide, unless both carry sub-group flag 1
// (Part.sameVesselCollision on both, CollisionManager.cs:126-168).
class SameVesselGroupFilter final : public JPH::GroupFilter {
public:
    bool CanCollide(const JPH::CollisionGroup& a, const JPH::CollisionGroup& b) const override {
        if (a.GetGroupID() != b.GetGroupID()) return true;
        return a.GetSubGroupID() == 1 && b.GetSubGroupID() == 1;
    }
};

struct WorldConfig {
    int velocitySteps = 10;  // Jolt default; PhysX in KSP: 50 position iterations / 1 velocity iteration [runtime]
    int positionSteps = 2;
    int threads = 1;         // total threads including the calling one (JobSystemThreadPool workers = threads - 1)
    unsigned maxBodies = 65536;
    unsigned maxBodyPairs = 65536;
    unsigned maxContacts = 65536;
};

// Process-wide Jolt registration (allocator, factory, types).
void JoltInit();
void JoltShutdown();

class World {
public:
    explicit World(const WorldConfig& cfg);
    ~World();
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    JPH::PhysicsSystem& System() { return *system_; }
    JPH::BodyInterface& Bodies() { return system_->GetBodyInterfaceNoLock(); }
    const JPH::GroupFilter* VesselFilter() const { return filter_.GetPtr(); }
    void Step(float dt, int collisionSteps = 1);
    const WorldConfig& Config() const { return cfg_; }

private:
    WorldConfig cfg_;
    std::unique_ptr<JPH::TempAllocatorImpl> temp_;
    std::unique_ptr<JPH::JobSystemThreadPool> jobs_;
    std::unique_ptr<JPH::BroadPhaseLayerInterface> bpl_;
    std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> ovb_;
    std::unique_ptr<JPH::ObjectLayerPairFilter> olp_;
    std::unique_ptr<JPH::PhysicsSystem> system_;
    JPH::Ref<SameVesselGroupFilter> filter_;
};

}  // namespace spike
