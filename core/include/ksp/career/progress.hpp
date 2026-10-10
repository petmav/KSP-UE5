// WP-36: progress tracking. ProgressNode.cs, ProgressTree.cs, ProgressTracking.cs and the KSPAchievements/ subtree (every
// achievement node: records, the per-body subtree, points of interest ...).
//
// Ownership: a ProgressTree owns its nodes (std::unique_ptr); ProgressTracking keeps typed raw pointers to the nodes it created
// (the C# public fields). Node ids are the save keys (`Progress { FirstLaunch { ... } Kerbin { Orbit { ... } } }`).
//
// Seams (CareerEnvironment): ProgressUtilities rewards / FindNextRecord and ContractDefs.Progression (WP-38), MessageSystem
// (WP-46), AnalyticsUtil (WP-59, dropped), the R&D tech tree (WP-35), the coroutine host (Vessel.StartCoroutine).
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/career/career_environment.hpp"
#include "ksp/career/currency.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"

namespace ksp {

class Vessel;
class ProtoVessel;
class CelestialBody;
class ProgressTree;

namespace KSPAchievements {
// ReturnFrom.cs:3
enum class ReturnFrom : int { FlyBy, Orbit, SubOrbit, Flight, Surface };
class CelestialBodySubtree;
}  // namespace KSPAchievements

// ProgressNode.cs:9
class ProgressNode {
public:
    ProgressNode(std::string id, bool startReached);  // :41
    virtual ~ProgressNode() = default;
    ProgressNode(const ProgressNode&) = delete;
    ProgressNode& operator=(const ProgressNode&) = delete;

    const std::string& Id() const { return id_; }            // :29
    bool IsReached() const { return reached_; }              // :31
    bool IsComplete() const { return complete_; }            // :33
    bool IsCompleteManned() const { return IsCompleteManned_; }      // :35
    bool IsCompleteUnmanned() const { return IsCompleteUnmanned_; }  // :37
    ProgressTree& Subtree() { return *subtree_; }            // :39
    void Reach();                                            // :47
    void Achieve();                                          // :55
    void Complete();                                         // :60
    void CheatComplete();                                    // :72
    void Load(const ConfigNode& node);                       // :82
    void Save(ConfigNode& node);                             // :116
    std::string GetNodeSummary(const std::string& baseID);   // :145

    std::function<void()> OnDeploy;                          // :21
    std::function<void()> OnStow;                            // :23
    std::function<void(Vessel*)> OnIterateVessels;           // :25
    std::function<std::string(const std::string&)> OnGenerateSummary;  // :27
    double AchieveDate = 0.0;                                // :19 (protected in C#; tests read it)

protected:
    virtual void OnLoad(const ConfigNode& node) { (void)node; }   // :112
    virtual void OnSave(ConfigNode& node) { (void)node; }         // :141
    void CrewSensitiveComplete(Vessel* v);                   // :154
    void CrewSensitiveComplete(ProtoVessel* pv);             // :170
    void AwardProgress(const std::string& description, float funds = 0.0f, float science = 0.0f, float reputation = 0.0f,
                       CelestialBody* body = nullptr);       // :186
    void AddOrAppendWorldFirstMessage(const std::string& title, const std::string& body);  // :217
    void AwardProgressStandard(const std::string& description, FinePrint::Utilities::ProgressType progress,
                               CelestialBody* body = nullptr);   // :247
    void AwardProgressInterval(const std::string& description, int currentInterval, int totalIntervals,
                               FinePrint::Utilities::ProgressType progress, CelestialBody* body = nullptr);  // :255
    void AwardProgressRandomTech(const std::string& description, int seed);  // :263
    bool IsCompleteManned_ = false;
    bool IsCompleteUnmanned_ = false;

private:
    std::string id_;
    bool reached_;
    bool complete_ = false;
    std::unique_ptr<ProgressTree> subtree_;
};

// ProgressTree.cs:6
class ProgressTree {
public:
    ProgressTree();
    ~ProgressTree();
    int Count() const { return static_cast<int>(nodes_.size()); }          // :11
    ProgressNode* operator[](int i) const { return nodes_.at(static_cast<size_t>(i)).get(); }  // :13
    ProgressNode* operator[](const std::string& s) const;                   // :15 (nullptr when absent)
    template <class T>
    T* AddNode(std::unique_ptr<T> node) {                                   // :45 (returns the added node)
        T* p = node.get();
        nodes_.push_back(std::move(node));
        return p;
    }
    bool Contains(const ProgressNode* node) const;                          // :50
    bool AllComplete() const;                                               // :55
    void Load(const ConfigNode& node);                                      // :74
    void Save(ConfigNode& node);                                            // :87
    void Deploy();                                                          // :101
    void IterateVessels(Vessel* v);                                         // :114
    void Stow();                                                            // :127
    std::string GetTreeSummary(const std::string& baseID);                  // :140

private:
    std::vector<std::unique_ptr<ProgressNode>> nodes_;
};

namespace KSPAchievements {

// VesselRef.cs
class VesselRef {
public:
    VesselRef() = default;                       // :33
    std::string Name, FlagURL;                   // :9, :21
    static VesselRef FromVessel(Vessel* v);      // :39
    void Load(const ConfigNode& node);           // :65
    void Save(ConfigNode& node) const;           // :77
};

// CrewRef.cs
class CrewRef {
public:
    CrewRef() = default;                                // :23
    std::vector<std::shared_ptr<ProtoCrewMember>> Crews;  // :9
    bool HasAny() const { return !Crews.empty(); }      // :21
    static CrewRef FromVessel(Vessel* v);               // :28
    static CrewRef FromProtoVessel(ProtoVessel* pv);    // :46
    void Load(const ConfigNode& node);                  // :57
    void Save(ConfigNode& node) const;                  // :88
};

// FirstLaunch.cs:7
class FirstLaunch : public ProgressNode {
public:
    FirstLaunch();   // :9
    void OnVesselSituationChange(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> vs);  // :22
private:
    Enumerator TestFlight(Vessel* v);  // :30
};

// BaseConstruction.cs:6
class BaseConstruction : public ProgressNode {
public:
    explicit BaseConstruction(CelestialBody* cb);  // :12
    void onPartsCoupled(events::GameEvents::FromToAction<Part*, Part*> pa);  // :27
protected:
    void OnLoad(const ConfigNode& node) override;  // :42
    void OnSave(ConfigNode& node) override;        // :50
private:
    CelestialBody* body_;
    VesselRef firstBase_;
};

// CelestialBodyEscape.cs:6
class CelestialBodyEscape : public ProgressNode {
public:
    explicit CelestialBodyEscape(CelestialBody* cb);  // :10
    void OnSOIEscaped(events::GameEvents::HostedFromToAction<Vessel*, CelestialBody*> vcs);  // :24
private:
    CelestialBody* body_;
};

// CelestialBodyFlight.cs:6
class CelestialBodyFlight : public ProgressNode {
public:
    explicit CelestialBodyFlight(CelestialBody* cb);  // :14
    CelestialBody* body;                               // :8
    VesselRef firstVessel;                             // :10
    CrewRef firstCrew;                                 // :12
    void onVesselSitChange(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> vs);  // :30
protected:
    void OnSave(ConfigNode& node) override;        // :47
    void OnLoad(const ConfigNode& node) override;  // :56
};

// CelestialBodyFlyby.cs:6
class CelestialBodyFlyby : public ProgressNode {
public:
    explicit CelestialBodyFlyby(CelestialBody* cb);  // :10
    void OnVesselSOIChange(events::GameEvents::HostedFromToAction<Vessel*, CelestialBody*> vcb);  // :24
private:
    CelestialBody* body_;
};

// CelestialBodyLanding.cs:6
class CelestialBodyLanding : public ProgressNode {
public:
    explicit CelestialBodyLanding(CelestialBody* cb);  // :14
    void OnVesselSituationChange(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> vs);  // :30
protected:
    void OnLoad(const ConfigNode& node) override;  // :64
    void OnSave(ConfigNode& node) override;        // :76
private:
    bool IsValidVessel(Vessel* v);  // :51
    CelestialBody* body_;
    VesselRef firstVessel_;
    CrewRef firstCrew_;
};

// CelestialBodyOrbit.cs
class CelestialBodyOrbit : public ProgressNode {
public:
    explicit CelestialBodyOrbit(CelestialBody* cb);  // :14
    CelestialBody* body;
    VesselRef firstVessel;
    CrewRef firstCrew;
    void onVesselSitChange(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> vs);  // :30
protected:
    void OnSave(ConfigNode& node) override;        // :48
    void OnLoad(const ConfigNode& node) override;  // :57
};

// CelestialBodyReturn.cs
class CelestialBodyReturn : public ProgressNode {
public:
    CelestialBodyReturn(CelestialBody* cb, ReturnFrom level);  // :16
    ReturnFrom Level;                                          // :10
    VesselRef firstVessel;
    CrewRef firstCrew;
    void OnVesselSituationChange(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> vs);  // :33
protected:
    void OnLoad(const ConfigNode& node) override;  // :93
    void OnSave(ConfigNode& node) override;        // :105
private:
    CelestialBody* body_;
};

// CelestialBodyScience.cs
class CelestialBodyScience : public ProgressNode {
public:
    explicit CelestialBodyScience(CelestialBody* cb);  // :14
    // ScienceSubject.IsFromBody(body) is WP-35's: the `subjectFromBody` seam answers it (OnScienceRecieved passes the subject
    // pointer; unset: the subject is not from the body)
    static inline std::function<bool(ScienceSubject*, CelestialBody*)> subjectIsFromBody;
    void OnScience(float science, ScienceSubject* subject, ProtoVessel* pv, bool reverseEngineered);  // :32
    void OnTriggeredScience(ScienceData* data, Vessel* origin, bool xmitAborted);                    // :62
    // ScienceData.dataAmount (WP-35 seam; unset: 0)
    static inline std::function<float(ScienceData*)> dataAmount;
protected:
    void OnLoad(const ConfigNode& node) override;  // :78
    void OnSave(ConfigNode& node) override;        // :90
private:
    CelestialBody* body_;
    VesselRef firstVessel_;
    CrewRef firstCrew_;
};

// CelestialBodySplashdown.cs
class CelestialBodySplashdown : public ProgressNode {
public:
    explicit CelestialBodySplashdown(CelestialBody* cb);  // :10
    void OnVesselSituationChange(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> vs);  // :24
private:
    bool IsValidVessel(Vessel* v);  // :42
    CelestialBody* body_;
};

// CelestialBodySuborbit.cs
class CelestialBodySuborbit : public ProgressNode {
public:
    explicit CelestialBodySuborbit(CelestialBody* cb);  // :14
    CelestialBody* body;
    VesselRef firstVessel;
    CrewRef firstCrew;
    void onVesselSitChange(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> vs);  // :30
protected:
    void OnSave(ConfigNode& node) override;        // :46
    void OnLoad(const ConfigNode& node) override;  // :55
};

// CelestialBodyTransfer.cs
class CelestialBodyTransfer : public ProgressNode {
public:
    explicit CelestialBodyTransfer(CelestialBody* cb);  // :10
    void OnCrewTransfer(events::GameEvents::HostedFromToAction<ProtoCrewMember*, Part*> fp);  // :24
protected:
    void OnLoad(const ConfigNode& node) override;  // :37
    void OnSave(ConfigNode& node) override;        // :41
private:
    CelestialBody* body_;
};

// CrewRecovery.cs
class CrewRecovery : public ProgressNode {
public:
    CrewRecovery();  // :10
    void onVesselRecovered(ProtoVessel* pv, bool quick);  // :24
protected:
    void OnLoad(const ConfigNode& node) override;  // :38
    void OnSave(ConfigNode& node) override;        // :46
private:
    CrewRef firstCrew_;
};

// Docking.cs
class Docking : public ProgressNode {
public:
    explicit Docking(CelestialBody* cb);  // :10
    void OnPartsCoupled(events::GameEvents::FromToAction<Part*, Part*> pa);  // :24
private:
    CelestialBody* body_;
};

// FlagPlant.cs
class FlagPlant : public ProgressNode {
public:
    explicit FlagPlant(CelestialBody* cb);  // :12
    void OnFlagPlant(Vessel* v);            // :27
protected:
    void OnLoad(const ConfigNode& node) override;  // :41
    void OnSave(ConfigNode& node) override;        // :49
private:
    CelestialBody* body_;
    CrewRef firstKerbal_;
};

// PointOfInterest.cs:7
class PointOfInterest : public ProgressNode {
public:
    PointOfInterest(std::string body, std::string name, std::string text, bool uplifting);  // :23
    PointOfInterest(std::string body, std::string name, std::string text, bool uplifting, bool launchSite);  // :43
    const std::string& body() const { return body_; }   // :13
    const std::string& name() const { return name_; }   // :15
    const std::string& text() const { return text_; }   // :17
    bool uplifting() const { return uplifting_; }       // :19
    bool launchSite() const { return launchSite_; }     // :21
    void OnAnomalyLoaded(CelestialBody* anomalyBody, std::string anomalyName);  // :50
    // PQSCity.GetPQSCitySeed(anomalyName, bodyName) (WP-42; unset: 0)
    static inline std::function<int(const std::string&, const std::string&)> pqsCitySeed;
protected:
    void OnLoad(const ConfigNode& node) override;  // :136
    void OnSave(ConfigNode& node) override;        // :148
    std::string GetUpliftText(int seed);           // :157
private:
    VesselRef firstVessel_;
    CrewRef firstCrew_;
    std::string body_, name_, text_;
    bool uplifting_, launchSite_;
};

// ReachSpace.cs
class ReachSpace : public ProgressNode {
public:
    ReachSpace();  // :12
    VesselRef firstVessel;
    CrewRef firstCrew;
    void OnVesselSituationChange(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> vs);  // :27
protected:
    void OnSave(ConfigNode& node) override;        // :44
    void OnLoad(const ConfigNode& node) override;  // :53
};

// RecordsAltitude.cs:7
class RecordsAltitude : public ProgressNode {
public:
    RecordsAltitude();  // :37
    double record = 0.0;  // :9
    void iterateVessels(Vessel* v);  // :43
protected:
    void OnLoad(const ConfigNode& node) override;  // :93
    void OnSave(ConfigNode& node) override;        // :109
private:
    double maxAltitude();  // :19
    Vessel* recordHolder_ = nullptr;
    double rewardThreshold_ = 0.0;
    int rewardInterval_ = 1;
    double atmosphereDepth_ = -1.0;
};

// RecordsDepth.cs:8
class RecordsDepth : public ProgressNode {
public:
    RecordsDepth();  // :37
    double record = 0.0;
    void iterateVessels(Vessel* v);  // :43
protected:
    void OnLoad(const ConfigNode& node) override;  // :94
    void OnSave(ConfigNode& node) override;        // :110
private:
    double maxDepth();  // :20
    Vessel* recordHolder_ = nullptr;
    double rewardThreshold_ = 0.0;
    int rewardInterval_ = 1;
    double depthCeiling_ = -1.0;
};

// RecordsDistance.cs:8
class RecordsDistance : public ProgressNode {
public:
    RecordsDistance();  // :37
    double record = 0.0;
    void iterateVessels(Vessel* v);  // :43
protected:
    void OnLoad(const ConfigNode& node) override;  // :113
    void OnSave(ConfigNode& node) override;        // :129
private:
    double maxDistance();  // :20
    Vessel* recordHolder_ = nullptr;
    double rewardThreshold_ = 0.0;
    int rewardInterval_ = 1;
    double distanceCeiling_ = -1.0;
};

// RecordsSpeed.cs:7
class RecordsSpeed : public ProgressNode {
public:
    RecordsSpeed();  // :36
    double record = 0.0;
    void iterateVessels(Vessel* v);  // :42
protected:
    void OnLoad(const ConfigNode& node) override;  // :106
    void OnSave(ConfigNode& node) override;        // :122
private:
    double maxSpeed();  // :19
    Vessel* recordHolder_ = nullptr;
    double rewardThreshold_ = 0.0;
    int rewardInterval_ = 1;
    double speedCeiling_ = -1.0;
};

// Rendezvous.cs:8
class Rendezvous : public ProgressNode {
public:
    explicit Rendezvous(CelestialBody* cb);  // :14
    void OnVesselLoaded(Vessel* v);          // :28
private:
    Enumerator TrackCandidateVessel(Vessel* candidate, Vessel* reference);  // :36
    CelestialBody* body_;
    double maxClosingSpeed_ = 50.0;
};

// Spacewalk.cs
class Spacewalk : public ProgressNode {
public:
    explicit Spacewalk(CelestialBody* cb);  // :12
    void OnGoEVA(events::GameEvents::FromToAction<Part*, Part*> fv);  // :27
protected:
    void OnLoad(const ConfigNode& node) override;  // :41
    void OnSave(ConfigNode& node) override;        // :49
private:
    CelestialBody* body_;
    CrewRef firstKerbal_;
};

// StationConstruction.cs
class StationConstruction : public ProgressNode {
public:
    explicit StationConstruction(CelestialBody* cb);  // :12
    void onPartsCoupled(events::GameEvents::FromToAction<Part*, Part*> pa);  // :27
protected:
    void OnLoad(const ConfigNode& node) override;  // :42
    void OnSave(ConfigNode& node) override;        // :50
private:
    CelestialBody* body_;
    VesselRef firstStation_;
};

// SurfaceEVA.cs
class SurfaceEVA : public ProgressNode {
public:
    explicit SurfaceEVA(CelestialBody* cb);  // :12
    void OnGoEVA(events::GameEvents::FromToAction<Part*, Part*> fv);  // :27
protected:
    void OnLoad(const ConfigNode& node) override;  // :41
    void OnSave(ConfigNode& node) override;        // :49
private:
    CelestialBody* body_;
    CrewRef firstKerbal_;
};

// TargetedLanding.cs
class TargetedLanding : public ProgressNode {
public:
    TargetedLanding(std::string targetTag, ReturnFrom level);  // :12
    void OnVesselSituationChange(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> vs);  // :27
private:
    Enumerator WaitAndCheckLandedTag(Vessel* host, std::string tgtTag);  // :35
    std::string targetTag_;
    ReturnFrom Level_;
};

// TowerBuzz.cs:8
class TowerBuzz : public ProgressNode {
public:
    TowerBuzz();  // :20
    void OnFlightReady();                                   // :39
    void OnFlightGlobalsReady(bool ready);                  // :44
    void OnFacilityUpgrade(Upgradeables::UpgradeableFacility* facility, int level);  // :52
    // the Upgradeables name of a facility (UpgradeableFacility.name), see Upgradeables::UpgradeableObject::name
protected:
    void OnLoad(const ConfigNode& node) override;  // :76
    void OnSave(ConfigNode& node) override;        // :88
private:
    void iterateVessels(Vessel* v);   // :60
    void SetActive(bool active);      // :97
    void Initialize();                // :110
    void Terminate();                 // :139
    bool active_ = false;
    double latitude_ = 0.0, longitude_ = 0.0;
    VesselRef firstVessel_;
    CrewRef firstCrew_;
};

// CelestialBodySubtree.cs:8
class CelestialBodySubtree : public ProgressNode {
public:
    explicit CelestialBodySubtree(CelestialBody* body);  // :56
    CelestialBody* Body;                                  // :10
    CelestialBodyFlyby* flyBy = nullptr;                  // :12
    CelestialBodyOrbit* orbit = nullptr;                  // :14
    CelestialBodyEscape* escape = nullptr;                // :16
    CelestialBodySuborbit* suborbit = nullptr;            // :18
    CelestialBodyFlight* flight = nullptr;                // :20
    CelestialBodyLanding* landing = nullptr;              // :22
    CelestialBodySplashdown* splashdown = nullptr;        // :24
    CelestialBodyTransfer* crewTransfer = nullptr;        // :26
    CelestialBodyScience* science = nullptr;              // :28
    Rendezvous* rendezvous = nullptr;                     // :30
    Docking* docking = nullptr;                           // :32
    Spacewalk* spacewalk = nullptr;                       // :34
    SurfaceEVA* surfaceEVA = nullptr;                     // :36
    FlagPlant* flagPlant = nullptr;                       // :38
    BaseConstruction* baseConstruction = nullptr;         // :40
    StationConstruction* stationConstruction = nullptr;   // :42
    CelestialBodyReturn* returnFromFlyby = nullptr;       // :44
    CelestialBodyReturn* returnFromOrbit = nullptr;       // :46
    CelestialBodyReturn* returnFromSurface = nullptr;     // :48
    CelestialBodySubtree* parentTree = nullptr;           // :51
    std::vector<CelestialBodySubtree*> childTrees;        // :54
    ~CelestialBodySubtree() override;
    void OnAchievementAchieve(ProgressNode* node);        // :119
    void LinkBodyHome(const std::vector<CelestialBodySubtree*>& trees);  // :150
    void DebugBodyTree();                                 // :201
protected:
    void OnSave(ConfigNode& node) override;        // :135
    void OnLoad(const ConfigNode& node) override;  // :140
private:
    std::string generateSubtreeSummary(const std::string& baseID);  // :145
};

}  // namespace KSPAchievements

// ProgressTracking.cs:14 ([KSPScenario(AddToNewGames, EDITOR, FLIGHT, TRACKSTATION, SPACECENTER)])
class ProgressTracking : public ScenarioModule {
public:
    ProgressTracking();
    ~ProgressTracking() override;
    static ProgressTracking* Instance() { return instance_; }   // :148
    std::unique_ptr<ProgressTree> achievementTree;              // :16
    KSPAchievements::FirstLaunch* firstLaunch = nullptr;        // :18
    KSPAchievements::RecordsAltitude* altitudeRecords = nullptr;  // :20
    KSPAchievements::RecordsDepth* depthRecords = nullptr;      // :22
    KSPAchievements::RecordsSpeed* speedRecords = nullptr;      // :24
    KSPAchievements::RecordsDistance* distanceRecords = nullptr;  // :26
    KSPAchievements::CrewRecovery* firstCrewToSurvive = nullptr;  // :28
    KSPAchievements::TowerBuzz* towerBuzz = nullptr;            // :30
    KSPAchievements::TargetedLanding* KSCLanding = nullptr;     // :32
    KSPAchievements::TargetedLanding* runwayLanding = nullptr;  // :34
    KSPAchievements::TargetedLanding* launchpadLanding = nullptr;  // :36
    KSPAchievements::ReachSpace* reachSpace = nullptr;          // :38
    // :40-:140 the PointOfInterest fields in declaration order (every one is also a node of the tree)
    std::vector<KSPAchievements::PointOfInterest*> pointsOfInterest;
    KSPAchievements::PointOfInterest* POIKerbinKSC2 = nullptr;
    KSPAchievements::PointOfInterest* POIKerbinIslandAirfield = nullptr;
    KSPAchievements::PointOfInterest* POIKerbinUFO = nullptr;
    KSPAchievements::PointOfInterest* POIKerbinPyramids = nullptr;
    KSPAchievements::PointOfInterest* POIKerbinMonolith00 = nullptr;
    KSPAchievements::PointOfInterest* POIKerbinMonolith01 = nullptr;
    KSPAchievements::PointOfInterest* POIKerbinMonolith02 = nullptr;
    KSPAchievements::PointOfInterest* POIKerbinDessertAirfield = nullptr;
    KSPAchievements::PointOfInterest* POIKerbinWoomerang = nullptr;
    KSPAchievements::PointOfInterest* POIKerbinDiscoverableLaunchsite01 = nullptr;
    KSPAchievements::PointOfInterest* POIKerbinDiscoverableLaunchsite02 = nullptr;
    KSPAchievements::PointOfInterest* POIKerbinDiscoverableLaunchsite03 = nullptr;
    KSPAchievements::PointOfInterest* POIKerbinDiscoverableLaunchsite04 = nullptr;
    KSPAchievements::PointOfInterest* POIMunArmstrongMemorial = nullptr;
    KSPAchievements::PointOfInterest* POIMunUFO = nullptr;
    KSPAchievements::PointOfInterest* POIMunRockArch00 = nullptr;
    KSPAchievements::PointOfInterest* POIMunRockArch01 = nullptr;
    KSPAchievements::PointOfInterest* POIMunRockArch02 = nullptr;
    KSPAchievements::PointOfInterest* POIMunMonolith00 = nullptr;
    KSPAchievements::PointOfInterest* POIMunMonolith01 = nullptr;
    KSPAchievements::PointOfInterest* POIMunMonolith02 = nullptr;
    KSPAchievements::PointOfInterest* POIDunaPyramid = nullptr;
    KSPAchievements::PointOfInterest* POIDunaMSL = nullptr;
    KSPAchievements::PointOfInterest* POIDunaFace = nullptr;
    KSPAchievements::PointOfInterest* POIMinmusMonolith00 = nullptr;
    KSPAchievements::PointOfInterest* POITyloCave = nullptr;
    KSPAchievements::PointOfInterest* POIVallIcehenge = nullptr;
    KSPAchievements::PointOfInterest* POIBopDeadKraken = nullptr;
    KSPAchievements::PointOfInterest* POIMohoRandolith = nullptr;
    KSPAchievements::PointOfInterest* POIEveRandolith = nullptr;
    KSPAchievements::PointOfInterest* POIGillyRandolith = nullptr;
    KSPAchievements::PointOfInterest* POIKerbinRandolith = nullptr;
    KSPAchievements::PointOfInterest* POIMunRandolith = nullptr;
    KSPAchievements::PointOfInterest* POIMinmusRandolith = nullptr;
    KSPAchievements::PointOfInterest* POIDunaRandolith = nullptr;
    KSPAchievements::PointOfInterest* POIIkeRandolith = nullptr;
    KSPAchievements::PointOfInterest* POIDresRandolith = nullptr;
    KSPAchievements::PointOfInterest* POILaytheRandolith = nullptr;
    KSPAchievements::PointOfInterest* POIVallRandolith = nullptr;
    KSPAchievements::PointOfInterest* POITyloRandolith = nullptr;
    KSPAchievements::PointOfInterest* POIBopRandolith = nullptr;
    KSPAchievements::PointOfInterest* POIPolRandolith = nullptr;
    KSPAchievements::PointOfInterest* POIEelooRandolith = nullptr;
    KSPAchievements::PointOfInterest* POIAnniversary1 = nullptr;
    KSPAchievements::PointOfInterest* POIAnniversary2 = nullptr;
    KSPAchievements::PointOfInterest* POIAnniversary3 = nullptr;
    KSPAchievements::PointOfInterest* POIAnniversary4 = nullptr;
    KSPAchievements::PointOfInterest* POIAnniversary5 = nullptr;
    KSPAchievements::PointOfInterest* POIAnniversary6 = nullptr;
    KSPAchievements::PointOfInterest* POIAnniversary7 = nullptr;
    std::vector<KSPAchievements::CelestialBodySubtree*> celestialBodyNodes;  // :143
    KSPAchievements::CelestialBodySubtree* celestialBodyHome = nullptr;      // :146

    void OnAwake() override;                       // :150
    void OnLoad(ConfigNode& node) override;        // :302
    void OnSave(ConfigNode& node) override;        // :310
    void Start();                                  // :315 (MonoBehaviour Start: Deploy)
    void Update();                                 // :320
    void OnDestroy();                              // :331
    std::string GenerateSummary();                 // :367
    bool NodeReached(const std::vector<std::string>& search);          // :372
    bool NodeComplete(const std::vector<std::string>& search);         // :381
    bool NodeCompleteManned(const std::vector<std::string>& search);   // :390
    bool NodeCompleteUnmanned(const std::vector<std::string>& search); // :399
    ProgressNode* FindNode(const std::vector<std::string>& search);    // :408
    KSPAchievements::CelestialBodySubtree* GetBodyTree(const std::string& bodyName);  // :435
    KSPAchievements::CelestialBodySubtree* GetBodyTree(CelestialBody* body);          // :455
    KSPAchievements::CelestialBodySubtree* GetBodyTree();                             // :475
    void CheatProgression();                       // :514
    void CheatEarlyProgression();                  // :523
    static void Register();
    void SetNonCheatablePOIs(std::vector<KSPAchievements::PointOfInterest*> v) { nonCheatablePOIs_ = std::move(v); }  // :232

private:
    ProgressTree* generateAchievementsTree();      // :157
    ProgressNode* FindNodeRecurse(ProgressTree& tree, int depth, const std::vector<std::string>& search);  // :417
    void RecurseCheatProgression(ProgressNode* node);  // :480
    void OnAchievementComplete(ProgressNode* node);    // :341
    static inline ProgressTracking* instance_ = nullptr;
    std::vector<KSPAchievements::PointOfInterest*> nonCheatablePOIs_;  // :66
};

}  // namespace ksp
