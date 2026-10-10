// PartCrewManifest.cs and VesselCrewManifest.cs (WP-31): the editor's / launch dialog's crew assignment.
//
// VesselCrewManifest replaces the WP-18 seam class of the same name that lived in save/ship_construct.hpp: ShipConstruct,
// ShipConstruction and the tests keep using the virtual members (CrewCount, GetCrewInventoryCosts/Mass, GetCrewResourceMass,
// AssignCrewToVessel). PartCrewManifest keeps a raw back pointer to its VesselCrewManifest (C# reference): a manifest must
// outlive the part manifests that point at it, or they must be re-parented with SetPartManifest.
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/save/crew.hpp"

namespace ksp {

class Part;
struct AvailablePart;
class ShipConstruct;
class VesselCrewManifest;

namespace Expansions::Missions {
// Expansions/Missions/Crew.cs (only the data VesselCrewManifest.AddCrewMembers reads / writes)
struct Crew {
    uint32_t partPersistentID = 0;
    uint32_t vesselPersistentID = 0;
    std::vector<std::string> crewNames;
};
}  // namespace Expansions::Missions

// PartCrewManifest.cs
class PartCrewManifest : public std::enable_shared_from_this<PartCrewManifest> {
public:
    // create with std::make_shared (AddCrewToSeat / RemoveCrewFromSeat use shared_from_this)
    using PCM = std::shared_ptr<ProtoCrewMember>;

    VesselCrewManifest* vcm = nullptr;     // :8
    std::vector<std::string> partCrew;     // :10 (empty string = empty seat)

    explicit PartCrewManifest(VesselCrewManifest* v) : vcm(v) {}  // :20
    const AvailablePart* PartInfo() const { return partInfo_; }    // :14
    uint32_t PartID() const { return partID_; }                    // :16

    // :25 ("part" = name_craftID; nullptr when the part is unknown)
    static std::shared_ptr<PartCrewManifest> FromConfigNode(const ConfigNode& node, VesselCrewManifest* v);
    // :44
    static std::shared_ptr<PartCrewManifest> CloneOf(const PartCrewManifest& original, VesselCrewManifest* vcm, bool blank);
    void AddCrewToSeat(const PCM& crew, int seatIndex);   // :71
    void RemoveCrewFromSeat(int seatIndex);               // :98
    void RemoveCrew(const std::string& name);             // :118
    std::vector<PCM> GetPartCrew();                        // :127
    void GetPartCrew(std::vector<PCM>& crewArray);         // :134
    bool Contains(const PCM& crew);                        // :155
    int GetCrewSeat(const ProtoCrewMember& crew);          // :160
    int GetCrewSeat(const std::string& name);              // :165
    bool AnySeats() const { return !partCrew.empty(); }    // :183
    bool NoSeats() const { return partCrew.empty(); }      // :188
    bool AllSeatsEmpty();                                  // :193
    bool AllSeatsEmpty(KerbalRoster& roster);              // :198
    bool AnySeatTaken();                                   // :221
    bool AnySeatTaken(KerbalRoster& roster);               // :226
    int CountCrewType(ProtoCrewMember_KerbalType type);    // :248
    int CountCrewType(ProtoCrewMember_KerbalType type, KerbalRoster& roster);   // :253
    int CountCrewNotType(ProtoCrewMember_KerbalType type);                      // :272
    int CountCrewNotType(ProtoCrewMember_KerbalType type, KerbalRoster& roster);  // :277

    // PartLoader.getPartInfoByName (SaveEnvironment::getPartInfoByName, else PartLoader::Instance())
    static const AvailablePart* GetPartInfoByName(const std::string& name);

private:
    friend class VesselCrewManifest;
    const AvailablePart* partInfo_ = nullptr;
    uint32_t partID_ = 0;
};

// VesselCrewManifest.cs
class VesselCrewManifest {
public:
    using PCM = std::shared_ptr<ProtoCrewMember>;
    using PartManifest = std::shared_ptr<PartCrewManifest>;

    VesselCrewManifest() = default;       // :21
    virtual ~VesselCrewManifest() = default;
    VesselCrewManifest(const VesselCrewManifest&) = delete;
    VesselCrewManifest& operator=(const VesselCrewManifest&) = delete;

    std::vector<PartManifest>& PartManifests() { return partManifests_; }  // :16
    virtual int CrewCount() const { return static_cast<int>(crewLookup_.size()); }  // :18
    float CrewCountOptimizedForFloat() const { return crewCountOptimizedForFloat_; }  // :20
    int PartCount() const { return static_cast<int>(partLookup_.size()); }  // :22

    static std::shared_ptr<VesselCrewManifest> FromConfigNode(const ConfigNode& craftNode);   // :31
    static std::shared_ptr<VesselCrewManifest> CloneOf(VesselCrewManifest& original, bool blank);  // :47
    // :59
    static void MergeInto(VesselCrewManifest& m1, VesselCrewManifest& m2,
                          const std::function<bool(PartCrewManifest&)>& inclusionFilter = nullptr);
    std::vector<PCM> GetAllCrew(bool includeNulls);                         // :86
    std::vector<PCM> GetAllCrew(bool includeNulls, KerbalRoster& roster);   // :91
    virtual float GetCrewInventoryCosts() { return GetCrewInventoryCosts(nullptr); }  // :112 (roster = null)
    float GetCrewInventoryCosts(KerbalRoster* roster);
    virtual float GetCrewInventoryMass() { return GetCrewInventoryMass(nullptr); }    // :126
    float GetCrewInventoryMass(KerbalRoster* roster);                                  // :131
    virtual float GetCrewResourceMass() { return GetCrewResourceMass(nullptr); }      // :145
    float GetCrewResourceMass(KerbalRoster* roster);                                   // :150
    bool Contains(const PCM& crew) const;                                  // :164
    std::vector<PartManifest> GetCrewableParts();                          // :169
    void SetPartManifestNoOverwrite(uint32_t id, const PartManifest& newPCMtoUse);  // :182
    void SetPartManifest(uint32_t id, const PartManifest& newPCMtoUse);    // :240
    void UpdatePartManifest(uint32_t id, const PartManifest& referencePCM);  // :296
    void RemovePartManifest(const PartManifest& pcm);                      // :331
    void SetCrewMember(const std::string& cName, const PartManifest& pcm);  // :345
    void RemoveCrewMember(const std::string& cName);                       // :360
    PartManifest GetPartCrewManifest(uint32_t id);                         // :373
    PartManifest GetPartForCrew(const ProtoCrewMember& crew);             // :382
    void DebugManifest();                                                   // :391
    void Filter(const std::function<bool(PartCrewManifest&)>& inclusionFilter);  // :411
    bool HasAnyCrew();                                                      // :424
    bool HasAnyCrew(KerbalRoster& roster);                                  // :429
    bool AnyCrewInState(ProtoCrewMember_RosterStatus state, bool notInState = false);   // :453
    bool AnyCrewInState(ProtoCrewMember_RosterStatus state, KerbalRoster& roster, bool notInState = false);
    bool AnyCrewWithTrait(const std::string& trait, bool noCrewWithTrait = false);      // :493
    bool AnyCrewWithTrait(const std::string& trait, KerbalRoster& roster, bool noCrewWithTrait = false);
    virtual void AssignCrewToVessel(ShipConstruct& ship);                   // :523
    void AssignCrewToVessel(ShipConstruct& ship, KerbalRoster& roster);
    std::shared_ptr<VesselCrewManifest> UpdateCrewForVessel(
        const ConfigNode& vesselNode, const std::function<bool(PartCrewManifest&)>& persistFilter = nullptr);  // :566
    void AddCrewMembers(std::vector<Expansions::Missions::Crew>& vesselCrew, KerbalRoster& roster);  // :582 (internal)

private:
    // Dictionary<uint, PartCrewManifest> partLookup / Dictionary<string, PartCrewManifest> crewLookup (enumeration order is
    // never observed here: DebugManifest walks partManifests)
    std::map<uint32_t, PartManifest> partLookup_;
    std::map<std::string, PartManifest> crewLookup_;
    std::vector<PartManifest> partManifests_;
    float crewCountOptimizedForFloat_ = 0.0f;
};

// Inputs of KerbalRoster.DefaultCrewForVessel that live in other systems (all unset = no funds system, nothing affordable
// logic): GameVariables.GetActiveCrewLimit(AstronautComplex level), GameVariables.GetRecruitHireCost, Funding.CanAfford,
// Funding.Instance != null, ScreenMessages
struct CrewAssignmentEnvironment {
    static inline std::function<int()> activeCrewLimit;                      // unset: 0
    static inline std::function<float(int currentActive)> recruitHireCost;   // unset: 2000 * ... see GameVariables.cs:463 (0)
    static inline std::function<bool(float cost)> canAfford;                 // unset: true
    static inline std::function<bool()> fundingPresent;                      // unset: false
    static inline std::function<void(const std::string& text, float duration)> postScreenMessage;  // unset: dropped
};

}  // namespace ksp
