// WP-35: science, R&D and the tech tree (engine-agnostic side).
//
// Ported (decompiled/KSP-Decompiled/Assembly-CSharp/): ExperimentSituations.cs, ExperimentUsageReqs.cs, ScienceExperiment.cs, ScienceSubject.cs,
// ScienceData.cs, IScienceDataContainer.cs, IScienceDataTransmitter.cs, ITechPerk.cs / TechPerk.cs, PlanetarySample.cs, KnowledgeItem.cs,
// RecoverySituations.cs, ScienceUtil.cs, ProtoTechNode.cs, ProtoRDNode.cs, RnDCommsStream.cs, ExperimentResultDialogPage.cs (the data side: the
// constructor's science values and UpdatePageLabValue), KSP/UI/Screens/RDTechTree.cs (the static ProtoRDNode / ProtoTechNode side: LoadTechTree,
// LoadTechTitles, GetTechNodesFromDB, the tree queries) and ResearchAndDevelopment.cs (the ScenarioModule: science points, subjects, tech states,
// experiments, recovery processing, OnLoad / OnSave).
//
// Seams (all in ScienceEnvironment, each documented): the funds / reputation currencies, strategies, facility levels / damage and GameVariables
// belong to WP-36 (CurrencyModifierQuery, ScenarioUpgradeableFacilities, GameVariables); CommNet (CommNetScenario, the vessel connection and its
// best transmitter, the comm node science curve) to WP-39; the biome map pixel access (CBAttributeMapSO.GetAtt) to a later package; the UI
// (ExperimentsResultDialog, PopupDialog, ScreenMessages, PartItemTransfer, RDController / RDNode rendering, the editor part list filters) is a
// recorded / hooked seam in the pattern of DeployablesEnvironment.
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/save/game.hpp"

namespace ksp {

class AvailablePart;
class Part;
class PartModule;
class ProtoVessel;
class Vessel;
class ModuleScienceLab;
class ModuleScienceConverter;
class ScienceLabSearch;
class ResearchAndDevelopment;
enum class TransactionReasons : int;
namespace KSP::UI::Screens {
class MissionRecoveryDialog;
}

// ExperimentSituations.cs:3
enum class ExperimentSituations : int {
    SrfLanded = 1,
    SrfSplashed = 2,
    FlyingLow = 4,
    FlyingHigh = 8,
    InSpaceLow = 0x10,
    InSpaceHigh = 0x20
};
// ExperimentUsageReqs.cs:1
enum class ExperimentUsageReqs : int { Always = 0, VesselControl = 1, CrewInVessel = 2, CrewInPart = 4, ScientistCrew = 8, Never = -1 };
// RecoverySituations.cs:3 (internal enum)
enum class RecoverySituations : int { Orbited, SubOrbited, Flew, Surfaced, FlewBy };
// RDTech.State (RDTech.cs:8)
enum class RDTech_State : int { Unavailable = 0, Available = 1 };

}  // namespace ksp

// Enum.ToString / GetNames / [Description] of the science enums (ExperimentSituations.cs, RecoverySituations.cs, RDTech.cs)
namespace ksp::dotnet {
template <>
struct EnumTraits<ksp::ExperimentSituations> {
    static constexpr bool flags = false;
    static constexpr EnumMember<ksp::ExperimentSituations> members[] = {
        {ksp::ExperimentSituations::SrfLanded, "SrfLanded", "#autoLOC_6002000"},
        {ksp::ExperimentSituations::SrfSplashed, "SrfSplashed", "#autoLOC_6002001"},
        {ksp::ExperimentSituations::FlyingLow, "FlyingLow", "#autoLOC_6002002"},
        {ksp::ExperimentSituations::FlyingHigh, "FlyingHigh", "#autoLOC_6002003"},
        {ksp::ExperimentSituations::InSpaceLow, "InSpaceLow", "#autoLOC_6002004"},
        {ksp::ExperimentSituations::InSpaceHigh, "InSpaceHigh", "#autoLOC_6002005"}};
};
template <>
struct EnumTraits<ksp::RecoverySituations> {
    static constexpr bool flags = false;
    static constexpr EnumMember<ksp::RecoverySituations> members[] = {
        {ksp::RecoverySituations::Orbited, "Orbited", "#autoLOC_6001099"},
        {ksp::RecoverySituations::SubOrbited, "SubOrbited", "#autoLOC_6001098"},
        {ksp::RecoverySituations::Flew, "Flew", "#autoLOC_6001097"},
        {ksp::RecoverySituations::Surfaced, "Surfaced", "#autoLOC_6001096"},
        {ksp::RecoverySituations::FlewBy, "FlewBy", "#autoLOC_6001100"}};
};
template <>
struct EnumTraits<ksp::RDTech_State> {
    static constexpr bool flags = false;
    static constexpr EnumMember<ksp::RDTech_State> members[] = {{ksp::RDTech_State::Unavailable, "Unavailable"},
                                                                {ksp::RDTech_State::Available, "Available"}};
};
}  // namespace ksp::dotnet

namespace ksp {

inline ExperimentUsageReqs operator&(ExperimentUsageReqs a, ExperimentUsageReqs b) {
    return static_cast<ExperimentUsageReqs>(static_cast<int>(a) & static_cast<int>(b));
}

// The TransactionReasons members the science package uses (TransactionReasons.cs). The enum itself is WP-36's (Funding / strategies / contracts);
// only these values are needed here.
namespace science_reason {
inline TransactionReasons VesselRecovery() { return static_cast<TransactionReasons>(0x20); }
inline TransactionReasons ScienceTransmission() { return static_cast<TransactionReasons>(0x400); }
inline TransactionReasons Cheating() { return static_cast<TransactionReasons>(0x10000); }
}  // namespace science_reason

// ---------------------------------------------------------------------------------------------------------------------------------
// ScienceExperiment.cs:5
class ScienceExperiment {
public:
    std::optional<std::string> id;  // null until Load / set
    std::string experimentTitle;
    float baseValue = 0.0f;
    float scienceCap = 0.0f;
    uint32_t situationMask = 0;
    uint32_t biomeMask = 0;
    float dataScale = 0.0f;
    bool requireAtmosphere = false;
    bool requireNoAtmosphere = false;
    float requiredExperimentLevel = 0.0f;
    bool applyScienceScale = false;

    ScienceExperiment() = default;                                       // :33
    const dotnet::Dictionary<std::string, std::string>& Results() const { return results_; }  // :31
    void Load(ConfigNode& node);                                         // :38
    void Save(ConfigNode& node) const;                                   // :69
    bool IsAvailableWhile(ExperimentSituations situation, const CelestialBody& body) const;  // :84
    bool BiomeIsRelevantWhile(ExperimentSituations situation) const;     // :105
    bool IsUnlocked() const;                                             // :110

private:
    dotnet::Dictionary<std::string, std::string> results_;
};

// ScienceSubject.cs:6
class ScienceSubject {
public:
    std::string id;
    std::string title;
    float dataScale = 0.0f;
    float scientificValue = 0.0f;
    bool applyScienceScale = false;
    float subjectValue = 0.0f;
    float scienceCap = 0.0f;
    float science = 0.0f;

    ScienceSubject(std::string id, std::string title, float dataScale, float subjectValue, float scienceCap);  // :23
    ScienceSubject(std::string id, std::string title, float dataScale, float subjectValue, float scienceCap, bool applyScienceScale);  // :28
    ScienceSubject(const ScienceExperiment& exp, ExperimentSituations sit, const CelestialBody& body, std::string biome = "",
                   std::string displaybiome = "");  // :40
    ScienceSubject(const ScienceExperiment& exp, ExperimentSituations sit, const std::string& sourceUid, const std::string& sourceTitle,
                   const CelestialBody& body, std::string biome = "", std::string displaybiome = "");  // :91
    explicit ScienceSubject(ConfigNode& node);                                                          // :123
    bool IsFromBody(const CelestialBody& cb) const;                 // :128
    bool IsFromSituation(ExperimentSituations situation) const;     // :133
    bool HasPartialIDstring(const std::string& pId) const;          // :138
    void Load(const ConfigNode& node);                              // :143
    void Save(ConfigNode& node) const;                              // :163
};

// ScienceData.cs:5
class ScienceData {
public:
    float labValue = 0.0f;
    float dataAmount = 0.0f;
    float scienceValueRatio = 0.0f;
    float baseTransmitValue = 0.0f;
    float transmitBonus = 0.0f;
    std::string subjectID;
    std::string title;
    bool triggered = false;
    uint32_t container = 0;
    std::string extraResultString;

    explicit ScienceData(const ConfigNode& node);                                                                          // :197
    ScienceData(float amount, float xmitValue, float xmitBonus, const std::string& id, const std::string& dataName, bool triggered = false,
                uint32_t container = 0u);                                                                                  // :202
    ScienceData(float amount, float scienceValueRatio, float xmitValue, float xmitBonus, const std::string& id, const std::string& dataName,
                bool triggered = false, uint32_t container = 0u, const std::string& extraResultString = "");               // :207
    void Load(const ConfigNode& node);                                                                                     // :220
    void Save(ConfigNode& node) const;                                                                                     // :250
    static std::shared_ptr<ScienceData> CopyOf(const ScienceData* src);                                                    // :266
};

// IScienceDataContainer.cs:3
class IScienceDataContainer {
public:
    virtual ~IScienceDataContainer() = default;
    virtual std::vector<std::shared_ptr<ScienceData>> GetData() = 0;
    virtual void ReturnData(const std::shared_ptr<ScienceData>& data) = 0;
    virtual void DumpData(const std::shared_ptr<ScienceData>& data) = 0;
    virtual void ReviewData() = 0;
    virtual void ReviewDataItem(const std::shared_ptr<ScienceData>& data) = 0;
    virtual int GetScienceCount() = 0;
    virtual bool IsRerunnable() = 0;
};

// IScienceDataTransmitter.cs:3 (implemented by ModuleDataTransmitter, WP-39)
class IScienceDataTransmitter {
public:
    virtual ~IScienceDataTransmitter() = default;
    virtual float DataRate() = 0;
    virtual double DataResourceCost() = 0;
    virtual bool CanTransmit() = 0;
    virtual bool IsBusy() = 0;
    virtual void TransmitData(std::vector<std::shared_ptr<ScienceData>>& dataQueue) = 0;
};

// ITechPerk.cs:1 / TechPerk.cs:5 (the C# methods throw NotImplementedException; the Texture2D icon is the UI's)
class ITechPerk {
public:
    virtual ~ITechPerk() = default;
    virtual void SetPerk(bool state) = 0;
    virtual void GetPerkState() = 0;
};
class TechPerk : public ITechPerk {
public:
    std::string perkID;
    std::string description;
    void SetPerk(bool state) override;  // :14 throws NotImplementedException
    void GetPerkState() override;       // :19 throws NotImplementedException
};

// PlanetarySample.cs:1
class PlanetarySample {
public:
    std::string sampleName;
    float sampleMass = 0.0f;
};

// KnowledgeItem.cs:3 (KnowledgeItem<T>; T is a value type: the C# `value is IFormattable` test is `formattable`, the format string is applied by
// `formatValue`, which the host maps to ToString(format, null); a non-formattable T uses `toString`)
template <class T>
class KnowledgeItem {
public:
    using UpdateDataCallback = std::function<T()>;
    // The host of the C# (DiscoveryInfo.HaveKnowledgeAbout(level)) as a predicate
    using HaveKnowledge = std::function<bool()>;

    // KnowledgeItem.cs:61
    KnowledgeItem(HaveKnowledge haveKnowledge, std::string caption, std::string valueFallback, UpdateDataCallback updateValueCallback,
                  std::string suffix = "", std::string format = "")
        : haveKnowledge_(std::move(haveKnowledge)), caption_(std::move(caption)), valueFallback_(std::move(valueFallback)),
          updateValue_(std::move(updateValueCallback)), suffix_(std::move(suffix)), formatString_(std::move(format)) {}

    const std::string& Caption() const { return caption_; }  // :25
    // :27 Value
    std::string Value() {
        if (haveKnowledge_()) {
            value_ = updateValue_();
            return Format(value_) + suffix_;
        }
        return valueFallback_;
    }
    // :44 OneLiner
    std::string OneLiner() {
        if (haveKnowledge_()) {
            value_ = updateValue_();
            return caption_ + Format(value_) + suffix_;
        }
        return valueFallback_;
    }
    // :72
    std::string GetValue(const std::string& noValueFallback) {
        if (haveKnowledge_()) {
            value_ = updateValue_();
            return Format(value_) + suffix_;
        }
        return noValueFallback;
    }
    // ToString(formatString, null) for IFormattable values, ToString() otherwise: supplied by the host (default: dotnet formatting is the
    // caller's concern; a string value is returned as is)
    static inline std::function<std::string(const T&, const std::string& format)> formatValue;

private:
    std::string Format(const T& v) const {
        if (formatValue) return formatValue(v, formatString_);
        if constexpr (std::is_convertible_v<T, std::string>) return std::string(v);
        else return std::to_string(v);
    }
    HaveKnowledge haveKnowledge_;
    std::string caption_, valueFallback_;
    T value_{};
    std::string suffix_;
    UpdateDataCallback updateValue_;
    std::string formatString_;
};

// ---------------------------------------------------------------------------------------------------------------------------------
// The science package's view of everything outside it. Every member is optional; "unset" says what happens then.
struct ScienceEnvironment {
    // ScreenMessages.PostScreenMessage texts, in order
    static inline std::vector<std::string> screenMessages;
    static void PostScreenMessage(const std::string& text) { screenMessages.push_back(text); }

    // GameDatabase.Instance.GetConfigNodes(type) (EXPERIMENT_DEFINITION, TechTree). Unset: no nodes.
    static inline std::function<std::vector<ConfigNode*>(std::string_view type)> getConfigNodes;
    // ConfigNode.Load(KSPUtil.ApplicationRootPath + path) (RDTechTree.LoadTechTree when the database has no TechTree). Unset: ConfigNode::Load(path).
    static inline std::function<ConfigNode::NodePtr(const std::string& path)> loadConfigFile;
    // PartLoader.LoadedPartsList. Unset: empty.
    static inline std::function<std::vector<const AvailablePart*>()> loadedParts;

    // WP-36: ScenarioUpgradeableFacilities.GetFacilityLevel(SpaceCenterFacility.ResearchAndDevelopment) (normalised 0..1). Unset: 1.
    static inline std::function<float()> rndFacilityLevel;
    // WP-36: GameVariables.Instance.GetExperimentLevel(level). Unset: the C# value (the identity, GameVariables.cs:677).
    static inline std::function<float(float)> experimentLevel;
    // WP-36: Funding.Instance != null (a career game). Unset: false (sandbox / science game).
    static inline std::function<bool()> fundingInstanceExists;
    // WP-36: RnDFacility.GetFacilityDamage(). Unset: 0 (undamaged).
    static inline std::function<float()> rndFacilityDamage;
    // WP-36: new CurrencyModifierQuery(reason, 0, science, 0) fired through OnCurrencyModifierQuery then OnCurrencyModified: returns
    // query.GetEffectDelta(Currency.Science) after the modifiers ran (ResearchAndDevelopment.OnCurrenciesModified adds it). Unset: no modifier
    // listens, delta 0 (and the delta handler is not run).
    static inline std::function<float(TransactionReasons reason, float science)> currencyEffectDelta;
    // WP-36: StringUtilities.FormattedCurrencies / ThisThisAndThat for ScienceTransmissionRewardString. Unset: " (+N Science)"-less text:
    // the formatted list is "" so the function returns "".
    static inline std::function<std::string(float amount, TransactionReasons reason)> formatScienceReward;

    // WP-39: CommNetScenario.CommNetEnabled. Unset: false.
    static inline std::function<bool()> commNetEnabled;
    // WP-39: v.connection != null ? v.connection.GetBestTransmitter() : not applicable. Return true (and set `result`) when the CommNet branch of
    // ScienceUtil.GetBestTransmitter applies. Unset: the part scan.
    static inline std::function<bool(Vessel&, IScienceDataTransmitter*& result)> commNetBestTransmitter;
    // WP-39: host.vessel.connection.Comm.scienceCurve.Evaluate(host.vessel.connection.ControlPath.signalStrength); nullopt = no curve / no
    // connection. Unset: no curve.
    static inline std::function<std::optional<double>(Vessel&)> commNetScienceCurve;

    // CBAttributeMapSO.GetAtt(lat, lon) on body.BiomeMap (the pixel reader is a later package). Unset: nullptr (logs a warning once the
    // body has a biome map).
    static inline std::function<const CBAttributeMapSO::MapAttribute*(const CelestialBody&, double latRad, double lonRad)> biomeAtt;

    // KerbalEVA / PopupDialog / ExperimentsResultDialog.DisplayResult(page): the user's choice is made by calling one of the page's callbacks.
    // The pages shown, in order (a page stays alive until the host drops it).
    static inline std::vector<std::shared_ptr<class ExperimentResultDialogPage>> shownPages;
    static inline std::function<void(const std::shared_ptr<ExperimentResultDialogPage>&)> displayResult;
    // ExperimentsResultDialog.Instance != null && pages.Count > 0 / PartItemTransfer.Instance != null. Unset: false.
    static inline std::function<bool()> resultDialogOpen;
    static inline std::function<bool()> partItemTransferActive;
    // The experiment overwrite popup (ModuleScienceExperiment.gatherData): called with (title, confirm, cancel). Unset: records and cancels.
    static inline std::function<void(const std::string& experimentTitle, std::function<void()> confirm, std::function<void()> cancel)> overwriteDialog;
    // GameSettings.SCIENCE_EXPERIMENT_SHOW_TRANSFER_WARNING (GameSettings.cs) and the confirm dialog UIConfirmDialog.Spawn: unset -> no warning.
    static inline bool showTransferWarning = false;
    static inline std::function<void(const std::string& text, std::function<void(bool dontShowAgain)> confirmed)> confirmDialog;

    // UnityEngine.Time.deltaTime of the frame (Lab.gatherProcessResources `mr.amount * Time.deltaTime`). Unset: TimeWarp / 0.02.
    static inline float deltaTime = 0.02f;
    // HighLogic.LoadedSceneIsFlight / LoadedSceneIsEditor. Defaults: flight.
    static inline bool loadedSceneIsFlight = true;
    static inline bool loadedSceneIsEditor = false;
    // Planetarium.GetUniversalTime(); unset: SaveUniversalTime()
    static inline std::function<double()> universalTime;
    static double UniversalTime();
    // TimeWarp.deltaTime for the experiment cooldown. Unset: Time.deltaTime.
    static inline std::function<double()> timeWarpDeltaTime;
    // System.Random.Next(min, max) of EVA result strings. Unset: min.
    static inline std::function<int(int min, int max)> systemRandomNext;
    // UnityEngine.Random.Range(int min, int max) (ResearchAndDevelopment.GetResults). Unset: UnityRandom::Global().
    static inline std::function<int(int min, int max)> unityRandomRange;

    // The coroutine scheduler the modules start their iterators on (MonoBehaviour.StartCoroutine). Unset: Scheduler() creates one.
    static inline CoroutineScheduler* scheduler = nullptr;
    static CoroutineScheduler& Scheduler();

    static void Reset();
};

// ---------------------------------------------------------------------------------------------------------------------------------
// ScienceUtil.cs:9
class ScienceUtil {
public:
    static ExperimentSituations GetExperimentSituation(Vessel& v);                                            // :11
    static std::string GetExperimentBiome(const CelestialBody& body, double lat, double lon);                 // :35
    static std::string GetExperimentBiomeLocalized(const CelestialBody& body, double lat, double lon);        // :44
    static std::string GetExperimentBodyName(const std::string& subjectID);                                   // :53
    static void GetExperimentFieldsFromScienceID(const std::string& subjectID, std::string& BodyName, ExperimentSituations& Situation,
                                                 std::string& Biome);                                         // :72
    static void GetExperimentFieldsFromScienceID(const std::string& subjectID, std::string& BodyName, std::string& Situation,
                                                 std::string& Biome);                                         // :90
    static std::string GenerateLocalizedTitle(const std::string& id);                                         // :124
    static std::string GenerateLocalizedTitle(const std::string& id, bool skipAsteroidsComets);               // :129
    static std::string GenerateScienceSubjectTitle(const ScienceExperiment& exp, ExperimentSituations sit, const CelestialBody& body,
                                                   const std::string& biome = "", const std::string& displaybiome = "");  // :230
    static std::string GenerateScienceSubjectRecoveryTitle(const ScienceExperiment& exp, RecoverySituations sit, const CelestialBody& body);  // :271
    static std::string GenerateScienceSubjectTitle(const ScienceExperiment& exp, ExperimentSituations sit, const std::string& sourceUid,
                                                   const std::string& sourceTitle, const CelestialBody& body, const std::string& biome = "",
                                                   const std::string& displaybiome = "");                     // :286
    static float GetTransmitterScore(IScienceDataTransmitter& t);                                             // :327
    static IScienceDataTransmitter* GetBestTransmitter(std::vector<IScienceDataTransmitter*>& vesselTransmitters);  // :333 [Obsolete]
    static IScienceDataTransmitter* GetBestTransmitter(Vessel& v);                                            // :354
    static float GetLabScore(ModuleScienceLab& lab);                                                          // :383
    static bool RequiredUsageInternalAvailable(Vessel& v, Part& p, ExperimentUsageReqs req, const ScienceExperiment& exp, std::string& message);  // :388
    static bool RequiredUsageExternalAvailable(Vessel& v, Vessel& vExt, ExperimentUsageReqs req, const ScienceExperiment& exp, std::string& message);  // :465
    static bool BiomeIsUnlisted(const CelestialBody* body, const std::string& biome);                         // :516
    static std::string GetBiomedisplayName(const CelestialBody* body, const std::string& biome);              // :540
    static std::string GetBiomedisplayName(const CelestialBody* body, const std::string& biome, bool formatted);  // :545

private:
    static std::string generateRecoveryLocalizedTitle(const std::string& id);                                 // :190
};

// ---------------------------------------------------------------------------------------------------------------------------------
// ProtoTechNode.cs:5
class ProtoTechNode {
public:
    std::string techID;
    RDTech_State state = RDTech_State::Unavailable;
    int scienceCost = 0;
    std::vector<const AvailablePart*> partsPurchased;

    ProtoTechNode() = default;                         // :15
    explicit ProtoTechNode(const ConfigNode& node);    // :19
    void Save(ConfigNode& node) const;                 // :88
};

// ProtoRDNode.cs:3
class ProtoRDNode {
public:
    std::vector<ProtoRDNode*> parents;
    std::vector<ProtoRDNode*> children;
    bool AnyParentToUnlock = false;
    std::shared_ptr<ProtoTechNode> tech;
    std::string iconRef;

    ProtoRDNode() = default;                           // :14
    explicit ProtoRDNode(const ConfigNode& node);      // :20
    void LoadLinks(const ConfigNode& node, std::vector<std::shared_ptr<ProtoRDNode>>& rdNodes);  // :147
    ProtoRDNode* FindNodeByID(const std::string& techID, std::vector<std::shared_ptr<ProtoRDNode>>& nodes);  // :167
    void Save(ConfigNode&) {}                          // :189
};

// KSP/UI/Screens/RDTechTree.cs:7 (the static data side; the MonoBehaviour / RDController / RDNode half is UI)
class RDTechTree {
public:
    static inline std::string backupTechTreeUrl = "GameData/Squad/Resources/TechTree.cfg";   // :15
    static inline std::vector<std::shared_ptr<ProtoRDNode>> rdNodes;                          // :17 (null until ReLoad: `rdNodesLoaded`)
    static inline bool rdNodesLoaded = false;
    static inline std::vector<std::shared_ptr<ProtoTechNode>> TechTreeTechs;                  // :19
    static inline std::vector<std::shared_ptr<ProtoRDNode>> TechTreeNodes;                    // :21
    static inline ConfigNode::NodePtr treeNode;                                               // :23

    static std::vector<std::shared_ptr<ProtoTechNode>> GetTreeTechs();                         // :33
    static std::vector<std::shared_ptr<ProtoRDNode>> GetTreeNodes();                           // :39
    static ConfigNode* GetTreeConfigNode();                                                    // :45
    static void PreLoad();                                                                     // :56
    static void ReLoad();                                                                      // :64
    static void ReLoad(std::string filePath, bool loadFromDatabase);                           // :69
    static void LoadTechTree(std::string filePath, std::vector<std::shared_ptr<ProtoRDNode>>& rdNodes, bool loadFromDatabase);  // :223
    static void LoadTechTitles(std::string filePath, dotnet::Dictionary<std::string, std::string>& dict, bool loadFromDatabase);  // :267
    static std::vector<std::shared_ptr<ProtoTechNode>> GetCheapestUnavailableNodes(int tolerance);  // :323
    static std::vector<std::shared_ptr<ProtoTechNode>> GetNextUnavailableNodes();              // :349
    static std::shared_ptr<ProtoTechNode> FindTech(const std::string& requiredTechId);         // :384
    static void Reset();

private:
    static bool GetTechNodesFromDB(ConfigNode::NodePtr& treeNode);                                  // :305
    static void recurseForNextTechs(ProtoRDNode* node, std::vector<std::shared_ptr<ProtoTechNode>>& techs);  // :368
};

// ---------------------------------------------------------------------------------------------------------------------------------
// ExperimentResultDialogPage.cs:6 (data side; the dialog widgets are UI)
class ExperimentResultDialogPage {
public:
    using ScienceCallback = std::function<void(const std::shared_ptr<ScienceData>&)>;
    std::string title;
    std::string resultText;
    float dataSize = 0.0f;
    float refValue = 0.0f;
    float scienceValueRatio = 0.0f;
    float scienceValue = 0.0f;
    float baseTransmitValue = 0.0f;
    float TransmitBonus = 0.0f;
    float remainingScience = 0.0f;
    float valueAfterRecovery = 0.0f;
    float valueAfterTransmit = 0.0f;
    float xmitDataScalar = 0.0f;
    bool showTransmitWarning = false;
    std::string transmitWarningMessage;
    bool showReset = false;
    std::shared_ptr<ScienceLabSearch> labSearch;
    float CommBonus = 0.0f;
    ScienceCallback OnDiscardData, OnKeepData, OnTransmitData, OnSendToLab;
    Part* host = nullptr;
    std::shared_ptr<ScienceData> pageData;

    ExperimentResultDialogPage(Part* host, const std::shared_ptr<ScienceData>& experimentData, float xmitBase, float xmitBonus,
                               bool showTransmitWarning, std::string transmitWarningMessage, bool showResetOption,
                               std::shared_ptr<ScienceLabSearch> labSearch, ScienceCallback onDiscardData, ScienceCallback onKeepData,
                               ScienceCallback onTransmitData, ScienceCallback onSendToLab);  // :64
    float UpdatePageLabValue();  // :148
    // The arithmetic of UpdatePageLabValue (:153-:171) for the given vessel / lab state (floats stored per the Mono rule, then Math.Round)
    static float LabValueMaths(float refValue, bool landed, bool splashed, bool subjectContainsMainBodyName, bool mainBodyIsHome, float SurfaceBonus,
                               float ContextBonus, float homeworldMultiplier);
};

// ---------------------------------------------------------------------------------------------------------------------------------
// RnDCommsStream.cs:7
class RnDCommsStream {
public:
    std::shared_ptr<ScienceSubject> subject;
    float fileSize = 0.0f;
    float timeout = 0.0f;
    float xmitValue = 0.0f;
    float scienceValueRatio = 0.0f;
    bool xmitIncomplete = false;

    RnDCommsStream(std::shared_ptr<ScienceSubject> subject, float fileSize, float timeout, float xmitValue, bool xmitIncomplete,
                   ResearchAndDevelopment* RDInstance);  // :222
    RnDCommsStream(std::shared_ptr<ScienceSubject> subject, float fileSize, float timeout, float xmitValue, float scienceValueRatio,
                   bool xmitIncomplete, ResearchAndDevelopment* RDInstance);  // :227
    // :241. Time.time is `Time.time` (ksp::Time); the timeout coroutine runs on ScienceEnvironment::Scheduler()
    void StreamData(float dataAmount, ProtoVessel* source);
    float dataIn() const { return dataIn_; }
    float dataOut() const { return dataOut_; }
    bool timedOut() const { return timedOut_; }

private:
    Enumerator timeoutCoroutine(ProtoVessel* source);  // :255
    void submitStreamData(ProtoVessel* source);        // :275
    float dataIn_ = 0.0f, dataOut_ = 0.0f, UTofLastTransmit_ = 0.0f;
    bool timedOut_ = true;
    ResearchAndDevelopment* host_ = nullptr;
};

// ---------------------------------------------------------------------------------------------------------------------------------
// ResearchAndDevelopment.cs:17 (KSPScenario creation options 3112, scenes FLIGHT / TRACKSTATION / SPACECENTER / EDITOR)
class ResearchAndDevelopment : public ScenarioModule {
public:
    static inline ResearchAndDevelopment* Instance = nullptr;  // :19

    ResearchAndDevelopment();
    ~ResearchAndDevelopment() override;

    // :47 / :49 (replaceable Func<>s; unset = the C# initial values)
    static inline std::function<float(float, ScienceSubject&)> GetReferenceDataValueFunc;
    static inline std::function<float(float, ScienceSubject&)> GetSubjectValueFunc;

    float Science() const { return science; }                                        // :51
    void AddScience(float value, TransactionReasons reason);                         // :53
    void SetScience(float value, TransactionReasons reason);                         // :66
    static bool CanAfford(float amount);                                             // :76
    void OnAwake() override;                                                         // :85
    void OnLoad(ConfigNode& node) override;                                          // :117
    void OnSave(ConfigNode& node) override;                                          // :211
    void Start();                                                                    // :259 (Unity message; the editor filters are UI)
    static void AddExperimentalPart(const AvailablePart* ap);                        // :286
    static bool IsExperimentalPart(const AvailablePart* ap);                         // :301
    static void RemoveExperimentalPart(const AvailablePart* ap);                     // :310
    void onVesselRecoveryProcessing(ProtoVessel* pv, KSP::UI::Screens::MissionRecoveryDialog* mrDialog, float recoveryScore);  // :331
    std::shared_ptr<ProtoTechNode> GetTechState(const std::string& techID);          // :528
    void SetTechState(const std::string& techID, std::shared_ptr<ProtoTechNode> techNode);  // :537
    static RDTech_State GetTechnologyState(const std::string& techID);               // :549
    static std::string GetTechnologyTitle(const std::string& techID);                // :562
    static bool PartTechAvailable(const AvailablePart* ap);                          // :575
    static bool PartModelPurchased(const AvailablePart* ap);                         // :584
    static ScienceExperiment* GetExperiment(const std::string& experimentID);        // :593
    static std::shared_ptr<ScienceSubject> GetExperimentSubject(const ScienceExperiment& experiment, ExperimentSituations situation,
                                                                const CelestialBody& body, const std::string& biome, const std::string& displaybiome);  // :608
    static std::shared_ptr<ScienceSubject> GetExperimentSubject(const ScienceExperiment& experiment, ExperimentSituations situation,
                                                                const std::string& sourceUId, const std::string& sourceTitle,
                                                                const CelestialBody& body, const std::string& biome, const std::string& displaybiome);  // :618
    static std::shared_ptr<ScienceSubject> GetSubjectByID(const std::string& subjectID);  // :628
    static float GetReferenceDataValue(float dataAmount, ScienceSubject& subject);   // :641
    static float GetSubjectValue(float subjectScience, ScienceSubject& subject);     // :646
    static float GetScienceValue(float dataAmount, ScienceSubject& subject, float xmitScalar = 1.0f);  // :651
    static float GetScienceValue(float dataAmount, float scienceValueRatio, ScienceSubject& subject, float xmitScalar = 1.0f);  // :656
    static float GetNextScienceValue(float dataAmount, ScienceSubject& subject, float xmitScalar = 1.0f);  // :664
    float SubmitScienceData(float dataAmount, ScienceSubject& subject, float xmitScalar = 1.0f, ProtoVessel* source = nullptr,
                            bool reverseEngineered = false);                         // :693
    float SubmitScienceData(float dataAmount, float scienceValueRatio, ScienceSubject& subject, float xmitScalar = 1.0f,
                            ProtoVessel* source = nullptr, bool reverseEngineered = false);  // :698
    static std::vector<std::shared_ptr<ScienceSubject>> GetSubjects();               // :727
    static std::vector<std::string> GetExperimentIDs();                              // :736
    static std::vector<std::string> GetSituationTagsDescriptions();                  // :746
    static std::vector<std::string> GetSituationTags();                              // :758
    static std::vector<std::string> GetBiomeTagsLocalized(const CelestialBody& cb, bool includeMiniBiomes);  // :763
    static std::vector<std::string> GetMiniBiomeTagsLocalized(const CelestialBody& cb);  // :782
    static std::vector<std::string> GetBiomeTags(const CelestialBody& cb, bool includeMiniBiomes);  // :810
    static std::vector<std::string> GetMiniBiomeTags(const CelestialBody& cb);       // :829
    static std::string GetMiniBiomedisplayNameByScienceID(const std::string& TagID, bool formatted);  // :857
    static std::string GetMiniBiomedisplayNameByUnityTag(const std::string& TagID, bool formatted);   // :874
    static std::string GetResults(const std::string& subjectID);                     // :900
    static std::string CheckForMissingParts();                                       // :936
    static std::string PartAssignmentSummary();                                      // :962
    void CheatAddScience(float sci);                                                 // :999
    void UnlockProtoTechNode(const ProtoTechNode& node);                             // :1004
    void CheatTechnology();                                                          // :1047
    static std::string CountUniversalScience();                                      // :1059
    static std::string ScienceTransmissionRewardString(float amount, TransactionReasons reason = science_reason::ScienceTransmission());  // :1162
    static bool ResearchedValidContractObjectives(std::vector<std::string> objectiveTypes, bool copy = true);  // :1177

    // The run-time state a test or host inspects (the C# fields are private)
    const dotnet::Dictionary<std::string, std::shared_ptr<ProtoTechNode>>& ProtoTechNodes() const { return protoTechNodes; }
    const dotnet::Dictionary<std::string, std::shared_ptr<ScienceSubject>>& ScienceSubjects() const { return scienceSubjects; }
    const dotnet::Dictionary<const AvailablePart*, int>& ExperimentalPartsStock() const { return experimentalPartsStock_; }
    // Subjects created by reverseEngineerRecoveredVessel are kept here in order (recoveredDataWidgets: amounts per subject)
    struct RecoveredWidget {
        std::shared_ptr<ScienceSubject> subject;
        float dataAmount = 0.0f, scienceAmount = 0.0f;
    };
    std::vector<RecoveredWidget> recoveredDataWidgets;
    // Statics reset (experiments cache, tech titles, minibiome caches)
    static void ResetStatics();

private:
    static void loadExperiments();                                                   // :157
    static void loadMiniBiomes();                                                    // :185
    std::shared_ptr<ConfigNode> getInitialTech();                                    // :240
    void OnCurrenciesModified(float effectDelta, TransactionReasons reason);         // :413
    void reverseEngineerRecoveredVessel(ProtoVessel* pv, KSP::UI::Screens::MissionRecoveryDialog* mrDialog);  // :423
    std::vector<std::shared_ptr<ScienceSubject>> reverseEngineerPartsFrom(std::vector<std::string> fromCBs, std::vector<std::string>& ignoreCBs,
                                                                          float subValue, const std::string& idVerb, const std::string& returnedFrom);  // :477
    bool partTechAvailable(const AvailablePart* ap);                                 // :494
    bool partModelPurchased(const AvailablePart* ap);                                // :511
    std::shared_ptr<ScienceSubject> getScienceSubject(const std::shared_ptr<ScienceSubject>& subject);  // :677

    static inline std::optional<dotnet::Dictionary<std::string, ScienceExperiment>> experiments;  // :21 (null until first use)
    static inline std::optional<std::map<std::string, const MiniBiome*>> minibiomesbyScienceID;   // :23
    static inline std::optional<std::map<std::string, const MiniBiome*>> minibiomesbyUnityTag;    // :25
    static inline dotnet::Dictionary<std::string, std::string> techTitles;                        // :29
    dotnet::Dictionary<std::string, std::shared_ptr<ProtoTechNode>> protoTechNodes;               // :27
    dotnet::Dictionary<std::string, std::shared_ptr<ScienceSubject>> scienceSubjects;             // :31
    dotnet::Dictionary<const AvailablePart*, int> experimentalPartsStock_;                        // :37 Dictionary<AvailablePart,int>
    float science = 0.0f;                                                                         // :43
};

}  // namespace ksp
