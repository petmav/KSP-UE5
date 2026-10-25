// WP-29d: the stock wheel / gear / leg model library. See ksp/physics/wheel_models.hpp.
#include "ksp/physics/wheel_models.hpp"

#include <fstream>
#include <stdexcept>

#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/mu_part_colliders.hpp"
#include "nlohmann/json.hpp"

namespace ksp::physics {

namespace {
using nlohmann::json;

Vector3f Vec3(const json& j) { return Vector3f(j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>()); }
}  // namespace

std::string WheelModelLibrary::DefaultDirectory() {
#ifdef KSPCORE_DATA_DIR
    return std::string(KSPCORE_DATA_DIR) + "/wheel_models";
#else
    return "core/data/wheel_models";
#endif
}

WheelModelLibrary::WheelModelLibrary(const std::string& dir) {
    std::ifstream f(dir + "/MANIFEST.json", std::ios::binary);
    if (!f) throw std::runtime_error("wheel models: cannot open " + dir + "/MANIFEST.json");
    const json man = json::parse(f);
    std::map<std::string, std::shared_ptr<const mu::Model>> cache;
    for (const auto& [name, pj] : man.at("parts").items()) {
        PartEntry e;
        e.cfgOrigin = pj.at("cfg").get<std::string>();
        for (const json& ij : pj.at("instances")) {
            Inst in;
            const std::string file = ij.at("file").get<std::string>();
            auto it = cache.find(file);
            if (it == cache.end()) it = cache.emplace(file, std::make_shared<const mu::Model>(mu::ReadModelFile(dir + "/" + file))).first;
            in.model = it->second;
            in.fromModelNode = ij.at("fromModelNode").get<bool>();
            in.parent = ij.value("parent", std::string());
            if (ij.contains("name")) {
                in.hasName = true;
                in.name = ij["name"].get<std::string>();
            }
            if (ij.contains("scale")) {
                in.hasScale = true;
                in.scale = Vec3(ij["scale"]);
            }
            if (ij.contains("position")) {
                in.hasPosition = true;
                in.position = Vec3(ij["position"]);
            }
            if (ij.contains("rotation")) {
                in.hasRotation = true;
                in.rotation = Vec3(ij["rotation"]);
            }
            e.instances.push_back(std::move(in));
        }
        parts_.emplace(name, std::move(e));
    }
}

std::vector<std::string> WheelModelLibrary::PartNames() const {
    std::vector<std::string> out;
    for (const auto& kv : parts_) out.push_back(kv.first);
    return out;
}

const std::string& WheelModelLibrary::CfgOrigin(const std::string& partName) const { return parts_.at(partName).cfgOrigin; }

std::vector<mu::ModelInstance> WheelModelLibrary::Instances(const std::string& partName) const {
    std::vector<mu::ModelInstance> out;
    auto it = parts_.find(partName);
    if (it == parts_.end()) return out;
    for (const Inst& in : it->second.instances) {
        mu::ModelInstance mi;
        mi.model = in.model.get();
        mi.fromModelNode = in.fromModelNode;
        mi.parent = in.parent;
        if (in.hasName) mi.name = in.name;
        if (in.hasScale) mi.scale = in.scale;
        if (in.hasPosition) mi.position = in.position;
        if (in.hasRotation) mi.rotationEuler = in.rotation;
        out.push_back(std::move(mi));
    }
    return out;
}

std::shared_ptr<mu::PartModelRig> WheelModelLibrary::BuildRig(const std::string& partName, float rescaleFactor) const {
    if (!Has(partName)) return nullptr;
    auto rig = std::make_shared<mu::PartModelRig>(mu::PartModelRig::Build(rescaleFactor, Instances(partName)));
    return rig->Valid() ? rig : nullptr;
}

std::function<bool(Part&, const std::string&, PartTransform&)> WheelModelLibrary::ModelTransformLookup() const {
    auto rigs = std::make_shared<std::map<std::pair<std::string, float>, std::shared_ptr<mu::PartModelRig>>>();
    return [this, rigs](Part& part, const std::string& name, PartTransform& local) {
        local = PartTransform{};
        const std::string partName = NameOf(part);
        if (!Has(partName)) return true;
        const float rescale = static_cast<float>(part.fields.rescaleFactor);
        auto key = std::make_pair(partName, rescale);
        auto it = rigs->find(key);
        if (it == rigs->end()) it = rigs->emplace(key, BuildRig(partName, rescale)).first;
        if (!it->second) return true;
        const int node = it->second->Find(name);
        if (node < 0) return false;
        const mu::RigPose pose = it->second->PartSpace(node);
        local.position = pose.position;
        local.rotation = pose.rotation;
        return true;
    };
}

std::function<std::vector<ColliderProxy>(const Part&)> WheelModelLibrary::ColliderHook() const {
    return [this](const Part& part) {
        const std::string name = NameOf(part);
        if (Has(name)) {
            std::vector<ColliderProxy> out =
                ProxiesFromShapes(mu::BuildPartColliders(static_cast<float>(part.fields.rescaleFactor), Instances(name)));
            if (!out.empty()) return out;
        }
        return DragCubeColliderProxy(part);
    };
}

std::string WheelModelLibrary::NameOf(const Part& part) { return part.partInfo != nullptr ? part.partInfo->name : std::string(); }

}  // namespace ksp::physics
