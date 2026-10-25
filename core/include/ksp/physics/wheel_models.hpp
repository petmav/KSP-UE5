// WP-29d: the model data of the stock wheel / landing gear / landing leg parts, for the wheel host (vessel_wheels.hpp).
//
// The data is core/data/wheel_models: the 13 stock parts' .mu models reduced to the transform tree, TagAndLayer, collider chunks, the
// WheelCollider chunk and the Animation chunk (core/tools/wp29d/stage_wheel_models.py, reduce_mu.py), with MANIFEST.json holding the origin
// path (decompiled/KSP-dev/GameData/...) and sha256 of every original, the sha256 of every reduced file and PartLoader.CompileModel's view of
// the part cfg (MODEL nodes: model / parent / name / scale / position / rotation). `Rig(partName, rescaleFactor)` builds the part's
// hierarchy as CompileModel does; a host with the real GameData reads the originals through the same reader (mu::ReadModelFile) instead.
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ksp/parts/part_model_rig.hpp"
#include "ksp/physics/vessel_physics.hpp"

namespace ksp {
class Part;
struct PartTransform;
}

namespace ksp::physics {

class WheelModelLibrary {
public:
    // dir holds MANIFEST.json and the reduced models. Throws std::runtime_error when the manifest or a model cannot be read.
    explicit WheelModelLibrary(const std::string& dir);
    // the repository's core/data/wheel_models
    static std::string DefaultDirectory();

    bool Has(const std::string& partName) const { return parts_.count(partName) != 0; }
    size_t PartCount() const { return parts_.size(); }
    std::vector<std::string> PartNames() const;
    const std::string& CfgOrigin(const std::string& partName) const;

    // PartLoader.CompileModel's instances of the part; the Model objects belong to the library
    std::vector<mu::ModelInstance> Instances(const std::string& partName) const;
    // a new rig with its own mutable local transforms (nullptr when the part is not in the library). The library must outlive the rig.
    std::shared_ptr<mu::PartModelRig> BuildRig(const std::string& partName, float rescaleFactor) const;

    // WP-21b: PartModuleForces::Hooks::modelTransform over this library: Part.FindModelTransform(name) (pre-order, first match) at the prefab
    // pose, in part space. A part the library does not hold answers true with the part's own transform (identity: not modelled, the
    // part transform stands in); a part it holds without such a transform answers false. The library must outlive the hook.
    std::function<bool(Part&, const std::string& name, PartTransform& local)> ModelTransformLookup() const;

    // WP-21b: VesselPhysicsOptions::colliders over this library: the part's real .mu colliders (physics/mu_part_colliders.hpp ProxiesFromShapes of
    // mu::BuildPartColliders over the part's instances), the drag-cube fixture box for a part the library does not hold or one without a
    // non-trigger collider (MuPartColliders::Proxies' rule). The library must outlive the hook.
    std::function<std::vector<ColliderProxy>(const Part&)> ColliderHook() const;

    // partName of a Part (AvailablePart.name); empty without partInfo
    static std::string NameOf(const Part& part);

private:
    struct Inst {
        std::shared_ptr<const mu::Model> model;
        bool fromModelNode = false;
        std::string parent;
        bool hasName = false;
        std::string name;
        bool hasScale = false, hasPosition = false, hasRotation = false;
        Vector3f scale, position, rotation;
    };
    struct PartEntry {
        std::string cfgOrigin;
        std::vector<Inst> instances;
    };
    std::map<std::string, PartEntry> parts_;
};

}  // namespace ksp::physics
