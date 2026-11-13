// wpfix9: CometVessel.cs start-up (see include/ksp/modules/comet_vessel.hpp).
#include "ksp/modules/comet_vessel.hpp"

#include <algorithm>

#include "ksp/career/career_environment.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/fineprint/space_objects.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

// CometVessel.cs:112 (the GameEvents subscriptions are not ported: see the header)
void CometVessel::OnAwake() {
    if (VesselPtr() != nullptr) VesselPtr()->comet = this;
}

namespace {
// CometVessel.cs:207
void SetSituationRanges(VesselRanges::Situation& sit) {
    CometManager* m = CometManager::Instance;
    if (m == nullptr) throw NullReferenceException("CometManager.Instance");
    sit.load = m->vesselRangeOrbitLoadOverride;
    sit.unload = m->vesselRangeOrbitUnloadOverride;
    sit.pack = m->vesselRangeOrbitPackOverride;
    sit.unpack = m->vesselRangeOrbitUnpackOverride;
}
}  // namespace

// CometVessel.cs:126 (part.name is the Part's gameObjectName; StartCoroutine("WaitAndUpdateVFXDimensions") at the end is the VFX side: not ported)
void CometVessel::OnStart() {
    Vessel* v = VesselPtr();
    const std::string typeName = Get<std::string>("typeName");
    uint32_t cometPartId = Get<uint32_t>("cometPartId");
    bool flag = true;
    if (!v->parts.empty()) {
        for (size_t i = 0; i < v->parts.size(); i++) {
            if (v->parts[i]->gameObjectName.find("PotatoComet") != std::string::npos) {
                if (cometPartId == 0 && !typeName.empty()) {
                    cometPartId = v->parts[i]->fields.persistentId;
                    flag = false;
                    break;
                }
                if (cometPartId == v->parts[i]->fields.persistentId) {
                    flag = false;
                    break;
                }
            }
        }
    }
    if (flag && v->protoVessel != nullptr) {
        for (size_t j = 0; j < v->protoVessel->protoPartSnapshots.size(); j++) {
            const ProtoPartSnapshot& snap = *v->protoVessel->protoPartSnapshots[j];
            if (!(snap.partName.has_value() && *snap.partName == "PotatoComet")) continue;
            if (cometPartId == 0) {
                ConfigNode* node = nullptr;
                if (v->protoVessel->vesselModules && v->protoVessel->vesselModules->TryGetNode("CometVessel", node)) {
                    std::string value;
                    if (node->TryGetValue("typeName", value) && !value.empty()) {
                        cometPartId = snap.persistentId;
                        flag = false;
                        break;
                    }
                }
            }
            if (cometPartId == snap.persistentId) {
                flag = false;
                break;
            }
        }
    }
    Set<uint32_t>("cometPartId", cometPartId);
    if (flag) {
        // vessel.vesselModules.Remove(this); vessel.Comet = null; Object.Destroy(this); return
        // (the local reference keeps this object alive until the function has returned)
        std::shared_ptr<VesselModule> self;
        auto& mods = v->vesselModules;
        const auto it = std::find_if(mods.begin(), mods.end(), [this](const std::shared_ptr<VesselModule>& m) { return m.get() == this; });
        if (it != mods.end()) {
            self = *it;
            mods.erase(it);
        }
        v->comet = nullptr;
        destroyed_ = true;
        return;
    }
    cometType_ = CometManager::GetCometOrbitType(typeName);
    if (cometType_ == nullptr) {
        log::Error("[CometVessel]: Unable to find CometType:" + typeName);
        return;
    }
    isCometTypeReady_ = true;
    lastUpdateUT_ = Planetarium::GetUniversalTime();
    // if ((bool)FlightGlobals.fetch) homeBodySMA = FlightGlobals.GetHomeBody().orbit.semiMajorAxis
    if (CelestialBody* home = CareerEnvironment::Current().HomeBody()) {
        if (home->orbit() != nullptr && home->orbit()->GetOrbit() != nullptr) homeBodySMA_ = home->orbit()->GetOrbit()->semiMajorAxis;
    }
    SetSituationRanges(v->vesselRanges.orbit);
    SetSituationRanges(v->vesselRanges.subOrbital);
    SetSituationRanges(v->vesselRanges.escaping);
    SetSituationRanges(v->vesselRanges.flying);
    SetSituationRanges(v->vesselRanges.splashed);
    SetSituationRanges(v->vesselRanges.landed);
    SetSituationRanges(v->vesselRanges.prelaunch);
}

namespace detail {
void RegisterCometVessel() {
    VesselModuleFactory::Register("CometVessel", [](const ModuleClassInfo& c) {
        return std::static_pointer_cast<VesselModule>(std::make_shared<CometVessel>(c));
    });
}
}  // namespace detail

}  // namespace ksp
