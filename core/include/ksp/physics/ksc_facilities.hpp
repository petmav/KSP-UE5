// WP-12d: the facility level at run time and the destruction state of the KSC scenery colliders.
//
// What KSP does: every level group of the KSC (an Upgradeables.UpgradeableFacility, or an UpgradeableSlave for the eight
// "SpaceCenter/Grounds/*" patches between buildings) keeps one level prefab instantiated (UpgradeableObject.setLevel -> UpgradeLevel.
// Despawn / Spawn, UpgradeableObject.cs:172). The level comes from ScenarioUpgradeableFacilities: the facility registers its id at the
// scene start (UpgradeableFacility.RegisterInstance -> ScenarioUpgradeableFacilities.RegisterUpgradeable, which Loads the saved "lvl"
// into setNormLevel; an id the scenario does not know gets the highest level, UpgradeableFacility.Reset). A slave takes
// FloorToInt(mean of its neighbours' normalised levels x level count) three frames after Start and on every neighbour's
// OnUpgradeableObjLevelChange (UpgradeableSlave.cs:70). The level prefab carries the DestructibleBuilding components of the shown level:
// they exist (Awake / Start: RegisterInstance with ScenarioDestructibles, Load of the saved intact state) while their level is spawned.
// A DestructibleBuilding's CollapsibleObjects switch GameObjects: collapse object off and replacement on when destroyed, the collapse
// object's transform animated down during the collapse (DestructibleBuilding.cs:84-187, WP-42's port).
//
// This host connects those WP-36 / WP-42 classes to a SceneryCityHost of the KSC (data/scenery): UpgradeLevel.onSpawn ->
// SceneryCityHost::SetFacilityLevel; the live DestructibleBuildings' collapse / replacement flags and the collapse pose ->
// SetObjectActiveSelf / SetObjectLocalPose (Sync, after the coroutine passes of a frame: a GameObject switched in a coroutine reaches the
// physics scene before the next FixedUpdate in Unity as well).
//
// Data: the level groups, level count, the destructibles (ids, collapse / replacement objects, collapseOffset, collapseBehaviour) are
// data/scenery's. Not in data/scenery (the extraction needs the game; not available to this package's session): the UpgradeableSlave
// neighbourIDs and the DestructibleBuilding prefab values (collapse / repair durations, replaceDelay, tilt, damage parameters): the host
// takes the slaves' neighbour ids as `Options::slaveNeighbours` (a group listed there is a slave) and the CollapsibleObject timings from
// `Options::collapse` (the C# field defaults when not given). STATUS.md WP-12d "Needs data".
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ksp/career/upgradeables.hpp"
#include "ksp/physics/scenery_host.hpp"
#include "ksp/space_center/destructible_building.hpp"

namespace ksp::Upgradeables {

// UpgradeableSlave.cs:5 (WP-42 listed the class as a Unity component; its level rule is engine-agnostic and ported here)
class UpgradeableSlave : public UpgradeableObject {
public:
    std::vector<UpgradeableFacility*> neighbours;   // :8
    std::vector<std::string> neighbourIDs;           // :11
    const std::vector<std::string>& NeighbourIDs() const { return neighbourIDs; }   // :17
    void CompileIDs();                               // :20
    ~UpgradeableSlave() override;
    float NeighbourAvgNorm() const { return neighbourAvgNorm; }

protected:
    void OnAwake() override;                         // :33
    void OnStart() override;                         // :38
    void OnOnDestroy() override;                     // :46

private:
    void OnObjectLevelChange(UpgradeableObject* upObj, int lvl);   // :51
    void UpdateLevel();                              // :70
    float neighbourAvgNorm = 0.0f;                   // :13
    bool listening_ = false;
};

}  // namespace ksp::Upgradeables

namespace ksp::physics {

class KSCFacilityHost {
public:
    struct Options {
        std::map<std::string, std::vector<std::string>> slaveNeighbours;   // slave group -> neighbour facility ids (UpgradeableSlave.neighbourIDs)
        // CollapsibleObject timings for every collapsible (DestructibleBuilding prefab values; the defaults are the C# field initialisers)
        float collapseDuration = 0.0f, repairDuration = 0.0f, replaceDelay = 1.0f;
        Vector3f collapseTiltMax{0.0f, 0.0f, 0.0f};
    };
    KSCFacilityHost(SceneryCityHost& city, Options options);
    KSCFacilityHost(SceneryCityHost& city) : KSCFacilityHost(city, Options()) {}
    ~KSCFacilityHost();
    KSCFacilityHost(const KSCFacilityHost&) = delete;
    KSCFacilityHost& operator=(const KSCFacilityHost&) = delete;

    // Unity's Awake / Start of the UpgradeableObject components (Awake: the highest level, UpgradeableFacility.OnAwake)
    void Awake();
    void Start();
    // the GameObjects the live DestructibleBuildings switched or moved, into the city's colliders
    void Sync();

    Upgradeables::UpgradeableObject* Group(const std::string& id) const;
    Upgradeables::UpgradeableFacility* Facility(const std::string& id) const;
    Upgradeables::UpgradeableSlave* Slave(const std::string& id) const;
    // the DestructibleBuildings of the shown levels, by id (DestructibleBuilding.id; one live per id)
    DestructibleBuilding* Building(const std::string& id) const;
    std::vector<std::string> GroupIds() const;
    size_t LiveBuildings() const;

private:
    struct Live {
        size_t record = 0;                       // index into city.destructibles
        std::unique_ptr<DestructibleBuilding> building;
        std::vector<char> lastCollapse, lastReplacement;
        std::vector<Vector3f> lastPos;
        std::vector<Quaternionf> lastRot;
    };
    struct Group_ {
        std::string id;
        std::unique_ptr<Upgradeables::UpgradeableObject> object;
        int levels = 0;
    };
    void Spawned(const std::string& group, int level);
    void Despawned(const std::string& group, int level);
    SceneryCityHost& city_;
    Options options_;
    std::vector<Group_> groups_;
    std::vector<std::pair<std::string, int>> recordLevel_;   // per destructible record: its level group and level
    std::vector<Live> live_;
};

}  // namespace ksp::physics
