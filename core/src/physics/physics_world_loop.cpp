// WP-20a: PhysicsWorld <-> PlayerLoop (WP-00). Separate from physics_world.cpp, which is built without RTTI like Jolt.
#include <memory>

#include "ksp/core/delegate.hpp"
#include "ksp/core/timing_manager.hpp"
#include "ksp/physics/physics_world.hpp"

namespace ksp::physics {

// PlayerLoop::physicsStep is FixedUpdate.PhysicsFixedUpdate, the slot where Unity ran the PhysX scene step.
void PhysicsWorld::Attach(PlayerLoop& loop) {
    if (loopHook_) return;
    auto hook = std::make_shared<Delegate<void()>>(Delegate<void()>::Lambda(this, [this] { FixedStep(); }));
    loop.physicsStep += *hook;
    loopHook_ = hook;
}

void PhysicsWorld::Detach(PlayerLoop& loop) {
    if (!loopHook_) return;
    loop.physicsStep -= *static_cast<Delegate<void()>*>(loopHook_.get());
    loopHook_.reset();
}

}  // namespace ksp::physics
