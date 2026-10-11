// Crew data model of the save: FlightLog (FlightLog.cs), Experience.ExperienceSystemConfig / ExperienceTraitConfig /
// ExperienceEffectConfig (Experience/*.cs, the GameDatabase EXPERIENCE_TRAIT configs), ProtoCrewMember
// (ProtoCrewMember.cs, data side) and KerbalRoster (KerbalRoster.cs, data side).
//
// Reference semantics: a ProtoCrewMember is shared (std::shared_ptr) between the roster, part snapshots and vessels.
// Quirks kept: FlightLog.Load removes "flight" from the node it reads (so loading a KERBAL mutates the save tree);
// FlightLog.GetEntries always returns an empty array (it iterates the new list); KerbalRoster's constructor appends the
// experience-type tables again on every construction (static lists grow, lookups search from the end); AddCrewMember
// refuses a kerbal whose first name is a SUBSTRING of an existing roster key; APPLICANTS/RECRUIT and CREW nodes of old
// saves load as KERBAL entries and are saved as KERBAL; a duplicate kerbal name re-adds the entry at the end of the
// roster (DictionaryValueList.Add).
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/util/collections.hpp"

namespace ksp {

class Game;
class GameParameters;
class CelestialBody;
class ModuleInventoryPart;
class ProtoCrewMember;
class Part;
class VesselCrewManifest;
namespace Experience { class ExperienceTrait; class ExperienceEffect; }

// ===================================================== FlightLog ====================================================

class FlightLog {
public:
    using EntryType = FlightLog_EntryType;

    // FlightLog.cs:28
    struct Entry {
        int flight = 0;
        std::string type;
        std::string target;
        Entry(int flight_, std::string type_, std::optional<std::string> target_ = std::nullopt)
            : flight(flight_), type(std::move(type_)), target(target_ ? std::move(*target_) : std::string()) {}
        Entry(int flight_, EntryType type_, std::optional<std::string> target_ = std::nullopt);
    };

    const std::vector<Entry>& Entries() const { return entries_; }
    std::vector<Entry>& Entries() { return entries_; }
    int Flight() const { return flight_; }
    int Count() const { return static_cast<int>(entries_.size()); }
    const Entry& operator[](int index) const { return entries_.at(static_cast<size_t>(index)); }

    void AddFlight() { ++flight_; }                // FlightLog.cs:77
    void Load(ConfigNode* node);                   // FlightLog.cs:82 (removes "flight" from node -- sic)
    void Save(ConfigNode& node) const;             // FlightLog.cs:112
    FlightLog CreateCopy() const;                  // FlightLog.cs:123 (the copy's flight counter is 0 -- sic)
    void MergeWith(const FlightLog& log);          // FlightLog.cs:134
    void Clear() { entries_.clear(); }             // FlightLog.cs:142
    void AddEntry(const Entry& entry);             // FlightLog.cs:147
    void AddEntry(EntryType type, std::optional<std::string> target = std::nullopt);
    void AddEntry(const std::string& type, std::optional<std::string> target = std::nullopt);
    void AddEntryUnique(const Entry& entry);       // FlightLog.cs:162
    void AddEntryUnique(EntryType type, std::optional<std::string> target = std::nullopt);
    const Entry* Last() const;                     // FlightLog.cs:184
    std::vector<Entry> GetEntries(EntryType type, const std::string& target) const;  // FlightLog.cs:193 (always empty)
    std::vector<Entry> GetEntries(EntryType type) const;
    std::vector<Entry> GetEntries(const std::string& type) const;
    std::vector<Entry> GetEntries(const std::string& type, const std::string& target) const;
    bool HasEntry(EntryType type) const;           // FlightLog.cs:233
    bool HasEntry(EntryType type, const std::string& target) const;
    bool HasEntry(const std::string& type) const;
    bool HasEntry(const std::string& type, const std::optional<std::string>& target) const;
    std::vector<std::string> GetDistinctTypes() const;                          // FlightLog.cs:283
    std::vector<std::string> GetDistinctTypes(const std::string& target) const;
    std::vector<std::string> GetDistinctTargets(EntryType type) const;          // FlightLog.cs:311
    std::vector<std::string> GetDistinctTargets(const std::string& type) const;
    std::vector<std::string> GetDistinctTargets() const;
    std::vector<FlightLog> GetFlights() const;                                  // FlightLog.cs:344

private:
    void AddEntryUnique(int flight, const std::string& type, const std::optional<std::string>& target);
    std::vector<Entry> entries_;
    int flight_ = 0;
};

// ================================================= Experience configs ===============================================

namespace Experience {

// Experience/ExperienceEffectConfig.cs
class ExperienceEffectConfig {
public:
    static std::shared_ptr<ExperienceEffectConfig> Create(const ConfigNode& node);  // nullptr when name is empty
    const std::string& Name() const { return name_; }
    const ConfigNode::NodePtr& Config() const { return config_; }

private:
    std::string name_;
    ConfigNode::NodePtr config_;
};

// Experience/ExperienceTraitConfig.cs (the icon texture is not loaded: IconString only)
class ExperienceTraitConfig {
public:
    static std::shared_ptr<ExperienceTraitConfig> Create(const ConfigNode& node);  // nullptr when name is missing
    const std::string& Name() const { return name_; }
    const std::optional<std::string>& Title() const { return title_; }
    const std::optional<std::string>& Description() const { return description_; }
    const std::optional<std::string>& IconString() const { return iconString_; }
    std::vector<std::shared_ptr<ExperienceEffectConfig>>& Effects() { return effects_; }
    const std::vector<std::shared_ptr<ExperienceEffectConfig>>& Effects() const { return effects_; }

private:
    std::string name_;
    std::optional<std::string> title_, description_, iconString_;
    std::vector<std::shared_ptr<ExperienceEffectConfig>> effects_;
};

// Experience/ExperienceSystemConfig.cs, fed with the GameDatabase EXPERIENCE_TRAIT nodes
class ExperienceSystemConfig {
public:
    ExperienceSystemConfig() = default;
    explicit ExperienceSystemConfig(const std::vector<const ConfigNode*>& experienceTraitNodes) {
        LoadTraitConfigs(experienceTraitNodes);
    }
    // ExperienceSystemConfig.cs:23
    void LoadTraitConfigs(const std::vector<const ConfigNode*>& experienceTraitNodes);
    const std::vector<std::shared_ptr<ExperienceTraitConfig>>& Categories() const { return categories_; }
    const std::vector<std::string>& TraitNames() const { return traitNames_; }
    const std::vector<std::string>& TraitNamesNoTourist() const { return traitNamesNoTourist_; }
    ExperienceTraitConfig* GetExperienceTraitConfig(std::string_view name) const;     // ExperienceSystemConfig.cs:72
    std::vector<std::string> GetTraitsWithEffect(std::string_view effectName) const;  // :90 (titles -- sic)
    std::vector<std::string> GetTraitsNamesWithEffect(std::string_view effectName) const;

    // kspcore: the stock GameData/Squad/Experience/Traits.cfg (Pilot, Engineer, Scientist, Tourist) for tools without
    // GameData
    static std::shared_ptr<ExperienceSystemConfig> StockBuiltIn();

private:
    std::vector<std::shared_ptr<ExperienceTraitConfig>> categories_;
    std::vector<std::string> traitNames_;
    std::vector<std::string> traitNamesNoTourist_;
};

}  // namespace Experience

// ================================================== ProtoCrewMember =================================================

class ProtoCrewMember : public std::enable_shared_from_this<ProtoCrewMember> {
public:
    using RosterStatus = ProtoCrewMember_RosterStatus;
    using KerbalType = ProtoCrewMember_KerbalType;
    using Gender = ProtoCrewMember_Gender;
    using KerbalSuit = ProtoCrewMember_KerbalSuit;
    struct CopyOf {};

    // ProtoCrewMember.cs:423 / 430 (SetDefaultInventory)
    explicit ProtoCrewMember(KerbalType type);
    ProtoCrewMember(KerbalType type, std::string newName);
    // ProtoCrewMember.cs:438 ProtoCrewMember(ProtoCrewMember copyOf)
    ProtoCrewMember(CopyOf, const ProtoCrewMember& copyOf);
    // ProtoCrewMember.cs:482 (the node is not const: FlightLog.Load removes "flight" from CAREER_LOG / FLIGHT_LOG)
    ProtoCrewMember(Game_Modes mode, ConfigNode& node, KerbalType crewType = KerbalType::Crew);
    ProtoCrewMember(const ProtoCrewMember&) = delete;
    ProtoCrewMember& operator=(const ProtoCrewMember&) = delete;
    ~ProtoCrewMember();

    // ProtoCrewMember.cs:688
    void Save(ConfigNode& node);

    // ---- fields (ProtoCrewMember.cs:63-171) ----
    float stupidity = 0.0f;
    float courage = 0.0f;
    bool isBadass = false;
    bool hasHelmetOn = false;
    bool hasNeckRingOn = false;
    bool hasVisorDown = false;
    float lightR = 1.0f;
    float lightG = 0.5176f;
    float lightB = 0.0f;
    bool completedFirstEVA = false;
    double inactiveTimeEnd = 0.0;
    bool veteran = false;
    bool hasToured = false;
    double UTaR = 0.0;
    uint32_t persistentID = 0;
    int seatIdx = -1;
    double geeForce = 0.0;
    double gExperienced = 0.0;
    double gIncrement = 0.0;
    bool outDueToG = false;
    FlightLog careerLog;
    FlightLog flightLog;
    float experience = 0.0f;
    int experienceLevel = 0;
    std::shared_ptr<Experience::ExperienceTrait> experienceTrait;  // ProtoCrewMember.cs:167 (WP-31: kerbals/experience.hpp)
    std::optional<std::string> trait;
    bool isHero = false;

    // ---- properties ----
    const std::optional<std::string>& name() const { return name_; }
    std::string displayName() const;  // ProtoCrewMember.cs:175
    std::string nameWithGender() const;  // :193 _name.LocalizeName(_gender): LingoonaGrammarExtensions.cs:12 ("^M" / "^F" suffix)
    bool inactive() const { return inactive_; }
    void set_inactive(bool value);    // fires onKerbalInactiveChange when it changes
    RosterStatus rosterStatus() const { return rosterStatus_; }
    void set_rosterStatus(RosterStatus value);  // fires onKerbalStatusChange / onKerbalStatusChanged
    KerbalType type() const { return type_; }
    void set_type(KerbalType value);            // fires onKerbalTypeChange / onKerbalTypeChanged
    Gender gender() const { return gender_; }
    void set_gender(Gender value) { gender_ = value; }
    KerbalSuit suit();                          // ProtoCrewMember.cs:257 (resets to Default when not installed)
    void set_suit(KerbalSuit value);            // ignored when the suit's DLC is not installed
    std::optional<std::string> SuitTexturePath, NormalTexturePath, SpritePath, ComboId;
    bool UseStockTexture = false;
    ConfigNode::NodePtr ChuteNode;              // chuteNode
    ConfigNode::NodePtr inventoryNode;          // the field (InventoryNode() creates the default when null)
    ConfigNode::NodePtr InventoryNode();        // ProtoCrewMember.cs:374
    float ExtraExperience() const { return extraExperience_; }
    void SetExtraExperience(float value);       // ProtoCrewMember.cs:392 (UpdateExperience)
    float ExperienceLevelDelta() const;         // ProtoCrewMember.cs:405

    // ---- methods ----
    bool SetInactive(double time, bool additive = true);  // ProtoCrewMember.cs:627
    bool CheckActive(double ut);                          // :642
    bool ChangeName(const std::string& newName);          // :652
    void SetDefaultInventory();                           // :810 (empty INVENTORY; contents need the part database)
    void SaveInventory(ModuleInventoryPart* inventory);   // :831 (WP-33: inventory/inventory_ui.cpp)
    std::string GetKerbalEVAPartName();                   // :1008
    void Die();                                           // :1205 (onCrewKilled / KerbalRef.die need the flight scene)
    void CheckRespawnTimer(double UT, const GameParameters& gameParameters);  // :1221
    void StartRespawnPeriod(double timeToRespawn = -1.0);                     // :1239
    void SetTimeForRespawn(double UTforRespawn);                              // :1248
    void ArchiveFlightLog();                                                  // :1254
    void UpdateExperience();                                                  // :1267
    float CalculateExperiencePoints(const Game& game) const;                  // :1278
    float CalculateExperiencePoints(const GameParameters& parameters, Game_Modes mode) const;  // :1283
    std::string ToString() const { return name_.value_or(""); }               // :1353

    // ---- WP-31 ----
    std::string GetLocalizedType() const;                     // :673 (the enum Description: the raw localization tag)
    std::string GetLocalizedStatus() const;                   // :678
    std::string GetLocalizedTrait() const;                    // :683
    std::string GetKerbalIVAName();                           // :1054
    std::string GetKerbalIconSuitSuffix();                    // :1100
    std::string GetKerbalIconSuitPath();                      // :1123
    void RegisterExperienceTraits(Part& part);                // :1296
    void UnregisterExperienceTraits(Part& part);              // :1304
    bool HasEffect(const std::string& effect);                // :1317
    Experience::ExperienceEffect* GetEffect(const std::string& effect);  // :1339 (last match first; NullReference without a trait)
    // GetEffect<T> / HasEffect<T> (:1309 / :1327): the last effect of the trait that is a T
    template <class T>
    T* GetEffect();
    template <class T>
    bool HasEffect() { return GetEffect<T>() != nullptr; }
    bool Equals(const ProtoCrewMember& o) const;              // :1358 (sic: `hasToured.Equals(hasToured)`)
    int32_t GetHashCode() const;                              // :1371
    double GExperiencedNormalized() const;                    // :149 expression property

    // ---- g-force and per-frame callbacks (:81-98, :856-980) ----
    using voidDelegate_PCM_Part = std::function<void(ProtoCrewMember&, Part&)>;
    using voidDelegate_PCM_UT = std::function<void(ProtoCrewMember&, double)>;
    using doubleDelegatePCM = std::function<double(ProtoCrewMember&)>;
    static inline voidDelegate_PCM_Part CallbackActiveFixedUpdate, CallbackActiveUpdate, CallbackOnPartPack, CallbackOnPartUnpack,
        ApplyGCallback;
    static inline voidDelegate_PCM_UT CallbackUpdate;
    static double _GToleranceMult(ProtoCrewMember& pcm);   // :856
    static double _MaxSustainedG(ProtoCrewMember& pcm);    // :871
    static inline doubleDelegatePCM GToleranceMult = _GToleranceMult;
    static inline doubleDelegatePCM MaxSustainedG = _MaxSustainedG;
    static inline bool doStockGCalcs = true;
    // ScreenMessages.PostScreenMessage(text, duration) (UI; unset: dropped)
    static inline std::function<void(const std::string&, float)> PostScreenMessage;
    void ActiveFixedUpdate(Part& p);                       // :876
    void ActiveUpdate(Part& p);                            // :960
    void Update(double UT);                                // :968
    void OnPartUnpack(Part& p);                            // :840
    void OnPartPack(Part& p);                              // :848
    // ModuleInventoryPart-backed values (kerbalModule is a WP-29/inventory object): hooks, unset = no module
    static inline std::function<float(ProtoCrewMember&)> kerbalModuleCost, kerbalModuleMass, kerbalPrefabResourceMass;
    float InventoryCosts();                                // :761 (0 for tourists; hook otherwise)
    float InventoryMass();                                 // :775
    float ResourceMass();                                  // :789

    // used by KerbalRoster
    void SetNameInternal(std::optional<std::string> n) { name_ = std::move(n); }

private:
    void OnGameStateCreated(Game* game);  // ProtoCrewMember.cs:745
    std::optional<std::string> name_;
    bool inactive_ = false;
    RosterStatus rosterStatus_ = RosterStatus::Available;
    KerbalType type_ = KerbalType::Crew;
    Gender gender_ = Gender::Male;
    KerbalSuit suit_ = KerbalSuit::Default;
    float extraExperience_ = 0.0f;
    bool listeningGameStateCreated_ = false;
};

// =================================================== KerbalRoster ===================================================

class KerbalRoster {
public:
    using PCM = std::shared_ptr<ProtoCrewMember>;

    static inline std::string pilotTrait = "Pilot";
    static inline std::string engineerTrait = "Engineer";
    static inline std::string scientistTrait = "Scientist";
    static inline std::string touristTrait = "Tourist";

    // KerbalRoster.cs:146 / 154 (the node is not const, see ProtoCrewMember)
    explicit KerbalRoster(Game_Modes mode);
    KerbalRoster(ConfigNode& node, Game_Modes mode);

    Game_Modes GameMode() const { return mode_; }
    // KerbalRoster.cs:50 / 62 indexers (nullptr when missing)
    PCM operator[](int index) const;
    PCM operator[](const std::string& name) const;
    int Count() const { return kerbals_.Count(); }
    std::vector<PCM> Applicants() const;  // KerbalRoster.cs:74
    std::vector<PCM> Crew() const;
    std::vector<PCM> Tourist() const;
    std::vector<PCM> Unowned() const;

    static std::shared_ptr<KerbalRoster> GenerateInitialCrewRoster(Game_Modes mode);  // KerbalRoster.cs:188
    void Init(const Game& st);                                                          // :250
    void CheckRosterRespawn(double universalTime, const GameParameters& parameters);   // :258
    void Save(ConfigNode& node) const;                                                  // :272
    void RepopulateCrewRoster(int rosterSize);                                          // :234
    PCM GetNextOrNewKerbal(ProtoCrewMember_KerbalType type = ProtoCrewMember_KerbalType::Crew);  // :281
    PCM GetNewKerbal(ProtoCrewMember_KerbalType type = ProtoCrewMember_KerbalType::Crew);        // :291
    void CheckRosterRespawn();                                                          // :267
    PCM GetNextAvailableKerbal(ProtoCrewMember_KerbalType type = ProtoCrewMember_KerbalType::Crew);                     // :302 (Planetarium UT)
    bool AddCrewMember(const PCM& crew);                                                // :325
    bool ChangeNameCalledFromPCM(const PCM& crew, const std::string& oldName, const std::string& newName);  // :349
    bool ChangeName(const std::string& oldName, const std::string& newName);           // :366
    bool Exists(const std::string& name) const;                                         // :376
    bool Remove(const std::string& name);                                               // :381
    bool Remove(const PCM& crew);                                                        // :393
    void Remove(int i);                                                                  // :398
    // :491 (implemented in kerbals/vessel_crew_manifest.cpp; the funding / facility / kerbal-seat inputs are hooks)
    std::shared_ptr<VesselCrewManifest> DefaultCrewForVessel(const ConfigNode& vesselNode, VesselCrewManifest* previous = nullptr,
                                                              bool autohire = true, bool usePreviousVCMToFill = false);
    void ValidateAssignments(Game& st);                                                  // :403
    int IndexOf(const PCM& pcm) const;                                                   // :616
    std::vector<PCM> Kerbals(ProtoCrewMember_KerbalType type,
                             std::initializer_list<ProtoCrewMember_RosterStatus> status = {}) const;  // :621
    std::vector<PCM> Kerbals(std::initializer_list<ProtoCrewMember_RosterStatus> status) const;                      // :651
    int GetActiveCrewCount() const;    // :678
    int GetAssignedCrewCount() const;  // :693
    int GetAvailableCrewCount() const; // :708
    int GetLostCrewCount() const;      // :723
    int GetMissingCrewCount() const;   // :738
    int GetKIACrewCount() const;       // :753
    PCM GetNextApplicant() const;      // :768
    void PCMUpdate(double UT);         // :788
    void Update(double UT);            // :797
    // AddApplicant (:837, private) is exposed for the tests as AddApplicant
    void AddApplicant(double UT);
    void UpdateExperience();           // :829
    void HireApplicant(const PCM& ap); // :851
    void SackAvailable(const PCM& ap); // :864
    void RemoveMIA(const PCM& ap);     // :877
    void RemoveDead(const PCM& ap);    // :890

    // ---- experience (static) ----
    static void SetExperienceTrait(ProtoCrewMember& pcm, std::optional<std::string> traitName = std::nullopt);  // :982
    static bool TryGetExperienceTraitConfig(const std::string& traitName, Experience::ExperienceTraitConfig*& traitConfig);
    // :1048 / :1053 without a home body name: FlightGlobals.GetHomeBody().name (SaveEnvironment::localBodies, isHomeWorld);
    // the first needs HighLogic.CurrentGame
    static void SetExperienceLevel(ProtoCrewMember& pcm, int level);
    static void SetExperienceLevel(ProtoCrewMember& pcm, int level, const GameParameters& parameters, Game_Modes mode);
    static void SetExperienceLevel(ProtoCrewMember& pcm, int level, const GameParameters& parameters, Game_Modes mode,
                                   const std::string& homeBodyName);  // :1053 (FlightGlobals.GetHomeBody().name)
    static std::string GetHomeBodyName();  // FlightGlobals.GetHomeBody().name (NullReferenceException when none)
    static float GetExperienceLevelRequirement(int level);           // :1088
    static int GetExperienceMaxLevel();                              // :1101
    static int CalculateExperienceLevel(float xp);                   // :1106
    static float CalculateExperience(const std::vector<const FlightLog*>& logs);  // :1126
    // :1159 GenerateExperienceLog; :1188 gettargetdisplayName; :1372 CheatExperience (HighLogic.CurrentGame.CrewRoster)
    static std::string GenerateExperienceLog(const FlightLog& log);
    static void CheatExperience();
    static std::string GetLocalizedExperienceTraitName(const std::optional<std::string>& queriedTrait);  // :1207
    static void AddExperienceType(const std::string& type, const std::string& typeName, float notHomeValue,
                                  float homeValue = 0.0f);  // :1241
    // CrewGenerator.cs:138 / 145
    static std::optional<std::string> CrewGenerator_GetLastName();
    static std::string CrewGenerator_RemoveLastName(const std::string& fullKerbalName);

    const DictionaryValueList<std::string, PCM>& kerbals() const { return kerbals_; }

private:
    static void SetExperienceLevelImpl(ProtoCrewMember& pcm, int level, const GameParameters& parameters, Game_Modes mode,
                                       const std::function<std::string()>& homeBodyName);
    static void InitializeExperienceTypes();  // :1225
    static bool ExperienceAddFlight(const FlightLog& flight, std::vector<std::string>& targets,
                                    std::vector<std::string>& types, std::vector<float>& targetXP);  // :1257
    static bool ExperienceBodyValue(const std::string& bodyName, const std::string& entryType, std::string& type,
                                    float& value);  // :1297
    static bool ExperienceHomeTypeValue(const std::string& typeName, float baseValue, std::string& type, float& value);
    static bool ExperienceNotHomeTypeValue(const std::string& typeName, float baseValue, std::string& type, float& value);

    DictionaryValueList<std::string, PCM> kerbals_;
    Game_Modes mode_;
};

}  // namespace ksp
