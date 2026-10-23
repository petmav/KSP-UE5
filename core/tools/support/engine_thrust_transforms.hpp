// WP-17b: the real thrust transforms of the stock engines (core/data/engine_thrust_transforms.json, written by
// tools/wp17/mu_transforms.py from the .mu models through PartReader.cs / PartLoader.CompileModel), installed on the live
// ModuleEngines of an editor ship. Header-only; shared by the golden runner (ds_deltav_stages.cpp) and tests/test_deltav.cpp.
#pragma once

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "ksp/dotnet/mono_float.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "nlohmann/json.hpp"

namespace wp17b {

struct EngineThrustTransform {
    std::string name;
    ksp::Quaternionf partSpaceRotation;  // world rotation relative to the part (right-nested product of the chain, model first)
    ksp::Vector3f partSpacePosition;
    std::vector<ksp::Quaternionf> chainRotations;  // local rotations, model first
    std::vector<std::string> chainNames;            // WP-20f: the chain's transform names, local positions and scales (model first)
    std::vector<ksp::Vector3f> chainPositions, chainScales;
    ksp::Vector3f partSpaceForward;      // as written by the python tool (independent computation, cross-checked by the unit test)
};

struct EngineThrustPart {
    std::vector<std::string> thrustVectorTransformName;
    std::vector<EngineThrustTransform> transforms;  // FindModelTransforms order (pre-order of the model hierarchy)
    std::string muOrigin, muSha256, cfgOrigin, cfgSha256;
};

// The editor ship's Unity transform hierarchy as ShipConstruct.LoadShip builds it: every part is created parentless and given its world
// rotation (ShipConstruct.cs:473 `part.transform.rotation = rot`), then re-parented with the world pose kept
// (:644 `partByCraftID.transform.parent = part2.transform`), which derives the child's local rotation as
// inverse(parent world) * child world. Transform.rotation of a part is the right-nested product of the local rotations from its root
// (native Quaternion multiplication, UnityNative::Multiply). Unity stores every rotation normalised (the setters run Normalize): that is
// applied to the world rotation set from the craft file and to the local rotation derived at parenting time. These two Normalize points
// and the multiplication order are not documented in the managed code: they are the combination that reproduces the captured stage
// delta-v of the 17 crafts (STATUS.md "WP-17b"); without the second one ComSat Lx is off by 467 ULP.
struct EditorHierarchy {
    std::map<const ksp::Part*, ksp::Quaternionf> local;
    std::map<const ksp::Part*, const ksp::Part*> parent;

    // EMPIRICAL (not documented in the decompile, UNVERIFIED against Unity native): the right-nesting over part hierarchy AND model chain fixed
    // Kerbal X in capture.deltav_stages. Documented: ShipConstruct.cs:473 (`part.transform.rotation = rot`) and :644 (`transform.parent = ...`).
    // `tail` (optional): a rotation below the part (the model chain of a thrust transform); Unity composes the whole chain from the leaf
    // upward (rot = parentLocal * rot), i.e. right-nested over the part hierarchy AND the model chain, not (part world) * (chain).
    ksp::Quaternionf World(const ksp::Part* p, const ksp::Quaternionf* tail = nullptr) const {
        ksp::Quaternionf rot = local.at(p);
        if (tail) rot = ksp::UnityNative::Multiply(rot, *tail);
        for (auto it = parent.find(p); it != parent.end() && it->second != nullptr; it = parent.find(it->second))
            rot = ksp::UnityNative::Multiply(local.at(it->second), rot);
        return rot;
    }

    // `parts` in file order, `Part::children` in link order = the loops of ShipConstruct.LoadShip (:625-646)
    static EditorHierarchy Build(const std::vector<ksp::Part*>& parts) {
        EditorHierarchy h;
        for (const ksp::Part* p : parts) {
            h.local[p] = ksp::UnityNative::Normalize(p->partTransform.rotation);
            h.parent[p] = nullptr;
        }
        for (const ksp::Part* p2 : parts)
            for (const ksp::Part* c : p2->children) {
                const ksp::Quaternionf worldChild = h.World(c);
                h.local[c] = ksp::UnityNative::Normalize(ksp::UnityNative::Multiply(ksp::UnityNative::Inverse(h.World(p2)), worldChild));
                h.parent[c] = p2;
            }
        return h;
    }
};

class EngineThrustTransforms {
public:
    // path = <project root>/core/data/engine_thrust_transforms.json
    static EngineThrustTransforms Load(const std::filesystem::path& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) throw std::runtime_error("engine thrust transforms: cannot open " + path.string());
        nlohmann::json j = nlohmann::json::parse(f);
        EngineThrustTransforms out;
        for (auto& [name, p] : j.at("parts").items()) {
            EngineThrustPart ep;
            for (auto& n : p.at("thrustVectorTransformName")) ep.thrustVectorTransformName.push_back(n.get<std::string>());
            ep.muOrigin = p.at("mu").at("origin").get<std::string>();
            ep.muSha256 = p.at("mu").at("sha256").get<std::string>();
            ep.cfgOrigin = p.at("cfg").at("origin").get<std::string>();
            ep.cfgSha256 = p.at("cfg").at("sha256").get<std::string>();
            for (auto& t : p.at("thrustTransforms")) {
                EngineThrustTransform et;
                et.name = t.at("name").get<std::string>();
                for (auto& c : t.at("chain")) {
                    const auto cr = Bits(c.at("rotation"), 4);
                    et.chainRotations.push_back(ksp::Quaternionf(cr[0], cr[1], cr[2], cr[3]));
                    et.chainNames.push_back(c.at("name").get<std::string>());
                    const auto cp = Bits(c.at("position"), 3);
                    et.chainPositions.push_back(ksp::Vector3f(cp[0], cp[1], cp[2]));
                    const auto cs = Bits(c.at("scale"), 3);
                    et.chainScales.push_back(ksp::Vector3f(cs[0], cs[1], cs[2]));
                }
                const auto q = Bits(t.at("partSpaceRotation"), 4);
                et.partSpaceRotation = ksp::Quaternionf(q[0], q[1], q[2], q[3]);
                const auto pos = Bits(t.at("partSpacePosition"), 3);
                et.partSpacePosition = ksp::Vector3f(pos[0], pos[1], pos[2]);
                const auto fwd = Bits(t.at("partSpaceForward"), 3);
                et.partSpaceForward = ksp::Vector3f(fwd[0], fwd[1], fwd[2]);
                ep.transforms.push_back(et);
            }
            out.parts_[name] = std::move(ep);
        }
        return out;
    }

    const std::map<std::string, EngineThrustPart>& Parts() const { return parts_; }

    // ModuleEngines.cs:665 (OnLoad) / :1714 (OnStart): thrustTransforms = part.FindModelTransforms(thrustVectorTransformName), then
    // the multiplier rule of OnLoad (:668-697, no `transformMultipliers` node in any staged cfg): 1 / count each. The transforms
    // are the part's children, so their world rotation is the part's rotation times the part-space chain rotation (Unity composes
    // the rotations of the chain in float, parent * child), the forward is `rotation * Vector3.forward`.
    // Throws (never falls back) if the part or the transform name is not one of the staged engines.
    //
    // `missing` (optional): collect the names of unstaged parts instead of throwing, so a caller can report all of them at once.
    void Install(ksp::Part& part, std::vector<std::string>* missing = nullptr, const EditorHierarchy* hierarchy = nullptr) const {
        for (size_t i = 0; i < part.Modules().Count(); i++) {
            auto* engine = dynamic_cast<ksp::ModuleEngines*>(part.Modules().GetModule(i));
            if (engine == nullptr) continue;
            const auto it = parts_.find(part.gameObjectName);
            if (it == parts_.end()) {
                if (missing != nullptr) {
                    missing->push_back(part.gameObjectName);
                    continue;
                }
                throw std::runtime_error("engine thrust transforms: part '" + part.gameObjectName + "' is not one of the staged engine models");
            }
            const std::string tname = engine->Get<std::string>("thrustVectorTransformName");
            bool known = false;
            for (const auto& n : it->second.thrustVectorTransformName) known = known || n == tname;
            if (!known)
                throw std::runtime_error("engine thrust transforms: part '" + part.gameObjectName + "' thrustVectorTransformName '" + tname + "' not staged");
            std::vector<ksp::PartTransform> list;
            for (const auto& t : it->second.transforms) {
                if (t.name != tname) continue;
                ksp::PartTransform w;
                w.position = part.partTransform.TransformPoint(t.partSpacePosition);
                const ksp::Quaternionf chain = ChainRotation(t);
                w.rotation = hierarchy ? hierarchy->World(&part, &chain) : ksp::UnityNative::Multiply(part.partTransform.rotation, chain);
                list.push_back(w);
            }
            if (list.empty())
                throw std::runtime_error("engine thrust transforms: part '" + part.gameObjectName + "' has no transform named '" + tname + "'");
            engine->thrustTransforms = list;
            engine->thrustTransformMultipliers.clear();
            const float item = ksp::f32store(ksp::fx(1.0f) / ksp::fx(static_cast<float>(list.size())));
            for (size_t k = list.size(); k-- > 0;) engine->thrustTransformMultipliers.push_back(item);
        }
    }

    // Transform.rotation of the thrust transform relative to the part: localRot(leaf), then rot = localRot(parent) * rot upward
    static ksp::Quaternionf ChainRotation(const EngineThrustTransform& t) {
        ksp::Quaternionf rot = t.chainRotations.back();
        for (size_t i = t.chainRotations.size() - 1; i-- > 0;) rot = ksp::UnityNative::Multiply(t.chainRotations[i], rot);
        return rot;
    }

private:
    static std::vector<float> Bits(const nlohmann::json& a, size_t n) {
        std::vector<float> v;
        if (a.size() != n) throw std::runtime_error("engine thrust transforms: bad vector length");
        for (auto& s : a) {
            const uint32_t b = static_cast<uint32_t>(std::stoul(s.get<std::string>(), nullptr, 16));
            float f;
            std::memcpy(&f, &b, 4);
            v.push_back(f);
        }
        return v;
    }
    std::map<std::string, EngineThrustPart> parts_;
};

}  // namespace wp17b
