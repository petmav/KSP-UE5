// WP-29c: VesselPhysics' buffered forces as a value (physics-warp substeps of wheeled vessels, the wheels' travel-limit prediction). Engine
// glue in double, no ported KSP statements. See ksp/physics/vessel_physics.hpp (BufferedForces).
#include "vessel_physics_internal.hpp"

namespace ksp::physics {

VesselPhysics::BufferedForces VesselPhysics::PeekBufferedForces() const {
    BufferedForces f;
    f.clusterForce.reserve(clusters_.size());
    f.clusterMoment.reserve(clusters_.size());
    for (const ClusterRec& c : clusters_) {
        f.clusterForce.push_back(c.force);
        f.clusterMoment.push_back(c.moment);
    }
    f.partForce = partForce_;
    f.partMoment = partMoment_;
    f.uniformAcceleration = uniformAccel_;
    return f;
}

void VesselPhysics::AddBufferedForces(const BufferedForces& f) {
    if (f.clusterForce.size() != clusters_.size() || f.partForce.size() != partForce_.size()) return;
    for (size_t i = 0; i < clusters_.size(); ++i) {
        clusters_[i].force += f.clusterForce[i];
        clusters_[i].moment += f.clusterMoment[i];
    }
    for (size_t i = 0; i < partForce_.size(); ++i) {
        partForce_[i] += f.partForce[i];
        partMoment_[i] += f.partMoment[i];
    }
    uniformAccel_ += f.uniformAcceleration;
}

}  // namespace ksp::physics
