// WP-12d: UpgradeableSlave.cs and the KSC facility / destruction host (see ksp/physics/ksc_facilities.hpp).
#include "ksp/physics/ksc_facilities.hpp"

#include <algorithm>

#include "ksp/career/career_environment.hpp"
#include "ksp/core/coroutine_host.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp::Upgradeables {

using GE = events::GameEvents;

UpgradeableSlave::~UpgradeableSlave() {
    if (listening_) OnOnDestroy();
}

// UpgradeableSlave.cs:20
void UpgradeableSlave::CompileIDs() {
    neighbourIDs.assign(neighbours.size(), std::string());
    size_t num = neighbours.size();
    while (num-- > 0)
        if (neighbours[num] != nullptr) neighbourIDs[num] = neighbours[num]->id;
}

// UpgradeableSlave.cs:33
void UpgradeableSlave::OnAwake() {
    GE::OnUpgradeableObjLevelChange.Add(this, &UpgradeableSlave::OnObjectLevelChange);
    listening_ = true;
}

// UpgradeableSlave.cs:38
void UpgradeableSlave::OnStart() {
    CareerEnvironment::Current().Scheduler().StartCoroutine(
        CallbackUtil::DelayedCallback(3, Callback<>::Lambda(this, [this] { UpdateLevel(); })));
}

// UpgradeableSlave.cs:46
void UpgradeableSlave::OnOnDestroy() {
    if (listening_) GE::OnUpgradeableObjLevelChange.Remove(this, &UpgradeableSlave::OnObjectLevelChange);
    listening_ = false;
}

// UpgradeableSlave.cs:51
void UpgradeableSlave::OnObjectLevelChange(UpgradeableObject* upObj, int) {
    auto* upgradeableFacility = dynamic_cast<UpgradeableFacility*>(upObj);
    if (upgradeableFacility == nullptr) return;
    size_t num = neighbourIDs.size();
    do {
        if (num-- <= 0) return;
    } while (!(neighbourIDs[num] == upgradeableFacility->id));
    UpdateLevel();
}

// UpgradeableSlave.cs:70 (float accumulation stored per statement: the Mono rule)
void UpgradeableSlave::UpdateLevel() {
    neighbourAvgNorm = 0.0f;
    size_t num = neighbourIDs.size();
    while (num-- > 0) {
        const std::string& facilityId = neighbourIDs[num];
        neighbourAvgNorm = f32store(fx(neighbourAvgNorm) + fx(ScenarioUpgradeableFacilities::GetFacilityLevel(facilityId)));
    }
    if (neighbourIDs.size() > 1) neighbourAvgNorm = f32store(fx(neighbourAvgNorm) / static_cast<double>(static_cast<float>(neighbourIDs.size())));
    facilityLevel_ = Mathf::FloorToInt(f32store(fx(neighbourAvgNorm) * static_cast<double>(static_cast<float>(upgradeLevels_.size()))));
    facilityLevel_ = Mathf::Clamp(facilityLevel_, 0, static_cast<int>(upgradeLevels_.size()) - 1);
    setLevel(facilityLevel_);
}

}  // namespace ksp::Upgradeables

namespace ksp::physics {

KSCFacilityHost::KSCFacilityHost(SceneryCityHost& city, Options options) : city_(city), options_(std::move(options)) {
    const SceneryCity& c = city_.city();
    std::map<std::string, int> levels;
    for (const SceneryObjectSpec& o : c.objects)
        if (!o.levelGroup.empty()) levels[o.levelGroup] = std::max(levels[o.levelGroup], o.level + 1);
    for (const auto& [id, n] : levels) {
        Group_ g;
        g.id = id;
        g.levels = n;
        const auto sl = options_.slaveNeighbours.find(id);
        if (sl != options_.slaveNeighbours.end()) {
            auto s = std::make_unique<Upgradeables::UpgradeableSlave>();
            s->neighbourIDs = sl->second;
            g.object = std::move(s);
        } else {
            g.object = std::make_unique<Upgradeables::UpgradeableFacility>();
        }
        Upgradeables::UpgradeableObject& u = *g.object;
        u.id = id;
        u.compiledId = id;
        u.preCompiledId = true;
        u.name = id.substr(id.rfind('/') + 1);
        for (int l = 0; l < n; ++l) {
            Upgradeables::UpgradeLevel lv;
            lv.hasFacilityPrefab = true;
            const std::string gid = id;
            // UpgradeLevel.Spawn destroys the GameObject facilityTransform pointed at (the level shown so far: Unity's null check then
            // makes that level's Spawned false) and puts the new instance in its place
            lv.onSpawn = [this, gid, l](Upgradeables::UpgradeLevel&) {
                Upgradeables::UpgradeableObject* u = Group(gid);
                auto& lvls = u->UpgradeLevels();
                for (size_t k = 0; k < lvls.size(); ++k) {
                    if (static_cast<int>(k) == l || !lvls[k].facilityInstance) continue;
                    lvls[k].facilityInstance = false;
                    Despawned(gid, static_cast<int>(k));
                }
                Spawned(gid, l);
            };
            lv.onDespawn = [this, gid, l](Upgradeables::UpgradeLevel&) { Despawned(gid, l); };
            u.UpgradeLevels().push_back(std::move(lv));
        }
        groups_.push_back(std::move(g));
    }
    // the level group and level of every destructible record (its nearest ancestor that is a level root)
    for (const SceneryDestructible& d : c.destructibles) {
        std::pair<std::string, int> gl{"", -1};
        for (int k = static_cast<int>(d.object); k >= 0; k = c.objects[static_cast<size_t>(k)].parent) {
            const SceneryObjectSpec& o = c.objects[static_cast<size_t>(k)];
            if (!o.levelGroup.empty()) {
                gl = {o.levelGroup, o.level};
                break;
            }
        }
        recordLevel_.push_back(gl);
    }
}

KSCFacilityHost::~KSCFacilityHost() {
    for (Live& l : live_) Retire(*l.building);
    live_.clear();
    for (Group_& g : groups_) {
        for (Upgradeables::UpgradeLevel& lv : g.object->UpgradeLevels()) lv.onSpawn = lv.onDespawn = nullptr;
        if (auto* f = dynamic_cast<Upgradeables::UpgradeableFacility*>(g.object.get())) f->UnregisterInstance();
        g.object->OnDestroy();
    }
}

void KSCFacilityHost::Awake() {
    for (Group_& g : groups_) g.object->Awake();
    // records outside any level group live from the start (none in the stock KSC)
    if (std::any_of(recordLevel_.begin(), recordLevel_.end(), [](const auto& gl) { return gl.second < 0; })) Spawned(std::string(), -1);
}

void KSCFacilityHost::Start() {
    for (Group_& g : groups_) g.object->Start();
}

// UpgradeLevel.Spawn (UpgradeableObject.cs:39): the level prefab enters the scene with its DestructibleBuilding components (Awake, Start)
void KSCFacilityHost::Spawned(const std::string& group, int level) {
    if (!group.empty()) city_.SetFacilityLevel(group, level);
    const SceneryCity& c = city_.city();
    for (size_t r = 0; r < recordLevel_.size(); ++r) {
        if (recordLevel_[r].first != group || recordLevel_[r].second != level) continue;
        const SceneryDestructible& d = c.destructibles[r];
        Live l;
        l.record = r;
        l.building = std::make_unique<DestructibleBuilding>();
        DestructibleBuilding& b = *l.building;
        b.id = d.id;
        b.compiledId = d.id;
        b.preCompiledId = true;
        b.tag = c.objects[d.object].tag;
        b.owner = NextOwner();   // its coroutines stop with it (Unity stops a destroyed MonoBehaviour's coroutines)
        for (const SceneryCollapsible& sc : d.collapsibles) {
            DestructibleBuilding::CollapsibleObject co;
            co.hasCollapseObject = true;
            co.collapseObjectActive = c.objects[sc.collapse].active;
            co.hasReplacementObject = sc.replacement >= 0;
            co.replacementObjectActive = sc.replacement >= 0 && c.objects[static_cast<size_t>(sc.replacement)].active;
            co.collapseBehaviour = static_cast<DestructibleBuilding::CollapsibleObject::Behaviour>(sc.collapseBehaviour);
            co.collapseOffset = sc.collapseOffset;
            co.collapseDuration = options_.collapseDuration;
            co.repairDuration = options_.repairDuration;
            co.replaceDelay = options_.replaceDelay;
            co.collapseTiltMax = options_.collapseTiltMax;
            co.localPosition = c.objects[sc.collapse].position;
            co.localRotation = c.objects[sc.collapse].rotation;
            b.CollapsibleObjects.push_back(co);
            l.lastCollapse.push_back(co.collapseObjectActive);
            l.lastReplacement.push_back(co.replacementObjectActive);
            l.lastPos.push_back(co.localPosition);
            l.lastRot.push_back(co.localRotation);
        }
        b.Awake();
        b.Start();
        live_.push_back(std::move(l));
    }
    Sync();   // a saved destroyed state (Load -> SetDestroyed) applies at once
}

// UpgradeLevel.Despawn (UpgradeableObject.cs:61): Object.Destroy of the level instance, its DestructibleBuildings with it
void KSCFacilityHost::Despawned(const std::string& group, int level) {
    const SceneryCity& c = city_.city();
    for (size_t i = live_.size(); i-- > 0;) {
        const auto& gl = recordLevel_[live_[i].record];
        if (gl.first != group || gl.second != level) continue;
        // the instance is gone: its objects' runtime state does not survive (the next Spawn instantiates the prefab again)
        const SceneryDestructible& d = c.destructibles[live_[i].record];
        for (const SceneryCollapsible& sc : d.collapsibles) {
            city_.SetObjectActiveSelf(sc.collapse, c.objects[sc.collapse].active);
            city_.SetObjectLocalPose(sc.collapse, c.objects[sc.collapse].position, c.objects[sc.collapse].rotation);
            if (sc.replacement >= 0) city_.SetObjectActiveSelf(static_cast<size_t>(sc.replacement), c.objects[static_cast<size_t>(sc.replacement)].active);
        }
        Retire(*live_[i].building);
        live_.erase(live_.begin() + static_cast<std::ptrdiff_t>(i));
    }
}

int KSCFacilityHost::NextOwner() {
    static int next = 0x12D00000;
    return ++next;
}

// Object.Destroy of a DestructibleBuilding: OnDestroy, and its coroutines (LoadOnSceneStart, the collapse / repair) stop
void KSCFacilityHost::Retire(DestructibleBuilding& b) {
    HighLogic::Scheduler().StopAllCoroutines(b.owner);
    b.OnDestroy();
}

void KSCFacilityHost::Sync() {
    const SceneryCity& c = city_.city();
    for (Live& l : live_) {
        const SceneryDestructible& d = c.destructibles[l.record];
        for (size_t k = 0; k < d.collapsibles.size(); ++k) {
            const DestructibleBuilding::CollapsibleObject& co = l.building->CollapsibleObjects[k];
            const SceneryCollapsible& sc = d.collapsibles[k];
            if (!co.localPosition.Equals(l.lastPos[k]) || !(co.localRotation.x == l.lastRot[k].x && co.localRotation.y == l.lastRot[k].y &&
                                                            co.localRotation.z == l.lastRot[k].z && co.localRotation.w == l.lastRot[k].w)) {
                city_.SetObjectLocalPose(sc.collapse, co.localPosition, co.localRotation);
                l.lastPos[k] = co.localPosition;
                l.lastRot[k] = co.localRotation;
            }
            if (static_cast<bool>(l.lastCollapse[k]) != co.collapseObjectActive) {
                city_.SetObjectActiveSelf(sc.collapse, co.collapseObjectActive);
                l.lastCollapse[k] = co.collapseObjectActive;
            }
            if (sc.replacement >= 0 && static_cast<bool>(l.lastReplacement[k]) != co.replacementObjectActive) {
                city_.SetObjectActiveSelf(static_cast<size_t>(sc.replacement), co.replacementObjectActive);
                l.lastReplacement[k] = co.replacementObjectActive;
            }
        }
    }
}

Upgradeables::UpgradeableObject* KSCFacilityHost::Group(const std::string& id) const {
    for (const Group_& g : groups_)
        if (g.id == id) return g.object.get();
    return nullptr;
}

Upgradeables::UpgradeableFacility* KSCFacilityHost::Facility(const std::string& id) const {
    return dynamic_cast<Upgradeables::UpgradeableFacility*>(Group(id));
}

Upgradeables::UpgradeableSlave* KSCFacilityHost::Slave(const std::string& id) const { return dynamic_cast<Upgradeables::UpgradeableSlave*>(Group(id)); }

DestructibleBuilding* KSCFacilityHost::Building(const std::string& id) const {
    for (const Live& l : live_)
        if (l.building->id == id) return l.building.get();
    return nullptr;
}

std::vector<std::string> KSCFacilityHost::GroupIds() const {
    std::vector<std::string> out;
    for (const Group_& g : groups_) out.push_back(g.id);
    return out;
}

size_t KSCFacilityHost::LiveBuildings() const { return live_.size(); }

}  // namespace ksp::physics
