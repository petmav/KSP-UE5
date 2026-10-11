#include "jolt_world.hpp"

#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>

#include <cstdarg>
#include <cstdio>

namespace spike {

namespace {

namespace BPL {
constexpr JPH::BroadPhaseLayer STATIC(0);
constexpr JPH::BroadPhaseLayer MOVING(1);
constexpr unsigned NUM = 2;
}  // namespace BPL

class BPLayers final : public JPH::BroadPhaseLayerInterface {
public:
    unsigned GetNumBroadPhaseLayers() const override { return BPL::NUM; }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer l) const override { return l == Layers::STATIC ? BPL::STATIC : BPL::MOVING; }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer l) const override { return l == BPL::STATIC ? "STATIC" : "MOVING"; }
#endif
};

class ObjVsBP final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer l, JPH::BroadPhaseLayer b) const override {
        return l == Layers::STATIC ? b == BPL::MOVING : true;
    }
};

class ObjPair final : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override {
        return a == Layers::STATIC ? b == Layers::MOVING : true;
    }
};

void TraceImpl(const char* fmt, ...) {
    va_list list;
    va_start(list, fmt);
    std::vfprintf(stderr, fmt, list);
    va_end(list);
    std::fputc('\n', stderr);
}

}  // namespace

void JoltInit() {
    JPH::RegisterDefaultAllocator();
    JPH::Trace = TraceImpl;
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
}

void JoltShutdown() {
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

World::World(const WorldConfig& cfg) : cfg_(cfg) {
    temp_ = std::make_unique<JPH::TempAllocatorImpl>(256u * 1024u * 1024u);
    jobs_ = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, cfg.threads > 1 ? cfg.threads - 1 : 0);
    bpl_ = std::make_unique<BPLayers>();
    ovb_ = std::make_unique<ObjVsBP>();
    olp_ = std::make_unique<ObjPair>();
    system_ = std::make_unique<JPH::PhysicsSystem>();
    system_->Init(cfg.maxBodies, 0, cfg.maxBodyPairs, cfg.maxContacts, *bpl_, *ovb_, *olp_);
    system_->SetGravity(JPH::Vec3::sZero());  // KSP: Physics.gravity = 0 [runtime]; gravity is a per-vessel force (VesselPrecalculate)
    JPH::PhysicsSettings s = system_->GetPhysicsSettings();
    s.mNumVelocitySteps = static_cast<unsigned>(cfg.velocitySteps);
    s.mNumPositionSteps = static_cast<unsigned>(cfg.positionSteps);
    system_->SetPhysicsSettings(s);
    filter_ = new SameVesselGroupFilter();
}

World::~World() = default;

void World::Step(float dt, int collisionSteps) { system_->Update(dt, collisionSteps, temp_.get(), jobs_.get()); }

}  // namespace spike
