// WP-mucol: real part colliders for the vessel rigid bodies (WP-20b/20c) from the parts' .mu models.
//
// VesselPhysicsOptions::colliders (vessel_physics.hpp) takes a function Part -> ColliderProxy list. `MuPartColliders::Hook()` is that
// function for the real game data: it reads the .mu file(s) of the part (Part::models[i].file->fullPath(), the files PartLoader
// resolved under GameData), builds the part-space shapes (parts/part_colliders.hpp: PartLoader.CompileModel placement, rescaleFactor,
// MODEL scale / position / rotation) and converts them:
//
//   BoxCollider      -> Box        (centre, half extents = |size * lossyScale| / 2, rotation = the node's)
//   SphereCollider   -> Sphere     (radius * largest |scale|)
//   CapsuleCollider  -> Capsule    (radius * larger non-axis |scale|, height * axis |scale|; X / Z capsules are rotated into Jolt's Y)
//   MeshCollider     -> ConvexHull (KSP forces every mesh collider convex, PartReader.cs:233 / :307: Jolt ConvexHullShape of the
//                                   part-space vertices; the hull faces, volume, centroid and inertia are Jolt's hull integrals)
//   trigger colliders are dropped (a trigger has no contact response and PhysX leaves it out of the mass properties)
//   WheelCollider    -> dropped    (enabled = false in the prefab; wheels are WP-29's)
//
// Parts without readable model data (the GameData mirror of the tests has no .mu files, a missing file, a part without a collider) get
// the labelled FIXTURE: the drag-cube box (DragCubeColliderProxy). `stats()` says how many parts took which path.
//
// Not modelled (labelled): gimbal / animated colliders do not move (a collider under a gimbal transform stays at its rest pose in the
// cluster's compound); layer / tag are kept in PartColliderShape but Jolt's two object layers do not use them (tag "NoAttach" only
// matters to the editor's attach raycasts); PhysicMaterial of the part colliders is the world's default (0.6 / 0).
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ksp/parts/mu_model.hpp"
#include "ksp/parts/part_colliders.hpp"
#include "ksp/physics/vessel_physics.hpp"

namespace ksp::physics {

// Shapes -> proxies (trigger and wheel colliders are skipped). Exposed for tests and tools.
std::vector<ColliderProxy> ProxiesFromShapes(const std::vector<mu::PartColliderShape>& shapes);
// Volume / centroid / inertia per unit mass of the convex hull of a collider mesh (Jolt's hull, physics/physics_world.hpp HullMassProperties):
// KSP mesh colliders are convex, PhysX and Unity use the hull of the vertices, not the (not always convex) triangles.
std::shared_ptr<const ColliderProxy::MassProps> HullMass(const std::vector<Vector3f>& vertices);

class MuPartColliders {
public:
    // Reads a model by file path; the default reads the file (mu::ReadModelFile). Replaceable for tests / a virtual file system.
    using ModelLoader = std::function<std::shared_ptr<const mu::Model>(const std::string& path)>;
    explicit MuPartColliders(ModelLoader loader = nullptr);

    // The colliders of `part` from its .mu data, or an empty optional-like (returns false) when it has none.
    bool Proxies(const Part& part, std::vector<ColliderProxy>& out);
    // For VesselPhysicsOptions::colliders: real colliders, else DragCubeColliderProxy (the fixture).
    std::function<std::vector<ColliderProxy>(const Part&)> Hook();

    struct Stats {
        size_t realParts = 0;       // calls answered from .mu data
        size_t fixtureNoModel = 0;  // no readable model file
        size_t fixtureNoShape = 0;  // model read, but no non-trigger collider
        size_t modelsRead = 0;      // distinct files read
        size_t modelErrors = 0;     // files that failed to parse
    };
    const Stats& stats() const { return stats_; }
    // The shapes of a part (part space), empty when it has no readable model.
    std::vector<mu::PartColliderShape> Shapes(const Part& part);

private:
    std::shared_ptr<const mu::Model> Load(const std::string& path);
    ModelLoader loader_;
    std::map<std::string, std::shared_ptr<const mu::Model>> cache_;
    Stats stats_;
    bool lastHadModel_ = false;  // the last Shapes() call read at least one model
};

}  // namespace ksp::physics
