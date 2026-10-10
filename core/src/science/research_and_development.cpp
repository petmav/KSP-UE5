// WP-35: ResearchAndDevelopment.cs (the ScenarioModule), ProtoTechNode.cs, ProtoRDNode.cs, RDTechTree.cs (static data side),
// ExperimentResultDialogPage.cs (data side) and RnDCommsStream.cs. See ksp/science/science.hpp for the scope and the seams.
#include <algorithm>
#include <limits>

#include "ksp/celestial/flight_globals.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/modules/science_modules.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/science/science.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/vessel/module_trip_logger.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {

using Loc = KSP::Localization::Localizer;

std::string Fmt(const char* tag, std::vector<std::string> args) { return Loc::Format(tag, args); }

GameParameters* CurrentParameters() {
    Game* g = SaveEnvironment::Current().currentGame;
    return g ? g->Parameters.get() : nullptr;
}
float ScienceGainMultiplier() {
    GameParameters* p = CurrentParameters();
    return p ? p->Career.ScienceGainMultiplier : 1.0f;
}
bool AllowNegativeCurrency() {
    GameParameters* p = CurrentParameters();
    return p ? p->CustomParams<GameParameters::AdvancedParams>().AllowNegativeCurrency : false;
}

const AvailablePart* PartInfoByName(const std::string& name) {
    auto& f = SaveEnvironment::Current().getPartInfoByName;
    return f ? f(name) : nullptr;
}

std::vector<const AvailablePart*> LoadedParts() {
    return ScienceEnvironment::loadedParts ? ScienceEnvironment::loadedParts() : std::vector<const AvailablePart*>();
}

CelestialBody* FindBodyByName(const std::string& name) {
    if (FlightGlobals::bodies == nullptr) return nullptr;
    for (CelestialBody* b : *FlightGlobals::bodies)
        if (b->bodyName == name) return b;
    return nullptr;
}

// FlightGlobals.GetHomeBody (FlightGlobals.cs:1813): the first body with isHomeWorld, else Bodies[0]
CelestialBody* HomeBody() {
    if (FlightGlobals::bodies == nullptr || FlightGlobals::bodies->empty()) throw NullReferenceException("FlightGlobals.Bodies");
    for (CelestialBody* b : *FlightGlobals::bodies)
        if (b->isHomeWorld) return b;
    return (*FlightGlobals::bodies)[0];
}

}  // namespace

// ----------------------------------------------------------------------------------------------------------------------------------
// ProtoTechNode

// ProtoTechNode.cs:19
ProtoTechNode::ProtoTechNode(const ConfigNode& node) {
    if (node.HasValue("id")) techID = *node.GetValue("id");
    if (node.HasValue("state")) state = dotnet::EnumParse<RDTech_State>(*node.GetValue("state"));
    if (node.HasValue("cost")) {
        scienceCost = dotnet::ParseInt32(*node.GetValue("cost"));
    } else {
        log::Warning("[R&D Tech " + techID + "]: Tech Node did not contain cost data. Looking for node in TechTree.cfg. This message should not appear again for this node.");
        std::vector<ConfigNode*> nodes = RDTechTree::GetTreeConfigNode()->GetNodes("RDNode");
        ConfigNode* configNode = nullptr;
        for (ConfigNode* configNode2 : nodes) {
            if (configNode2->GetValue("id") == techID) {
                configNode = configNode2;
                break;
            }
        }
        if (configNode != nullptr) {
            if (configNode->HasValue("cost")) scienceCost = dotnet::ParseInt32(*configNode->GetValue("cost"));
            else log::Error("[R&D Tech " + techID + "]: Node exists in TechTree.cfg but no science cost defined! Something is very wrong here. Strongly recommend checking TechTree.cfg file integrity");
        } else {
            log::Warning("[R&D Tech " + techID + "]: No such tech node defined in tech tree!");
            scienceCost = 0;
        }
    }
    partsPurchased.clear();
    for (const std::string& v : node.GetValues("part")) {
        const AvailablePart* partInfoByName = PartInfoByName(v);
        if (partInfoByName != nullptr) partsPurchased.push_back(partInfoByName);
    }
}

// ProtoTechNode.cs:88
void ProtoTechNode::Save(ConfigNode& node) const {
    node.AddValue("id", techID);
    node.AddValue("state", dotnet::EnumToString(state));
    node.AddValue("cost", std::to_string(scienceCost));
    for (const AvailablePart* p : partsPurchased) node.AddValue("part", p->name);
}

// ----------------------------------------------------------------------------------------------------------------------------------
// ProtoRDNode

// ProtoRDNode.cs:20
ProtoRDNode::ProtoRDNode(const ConfigNode& node) {
    if (node.HasValue("icon")) iconRef = *node.GetValue("icon");
    if (node.HasValue("anyToUnlock")) {
        const std::string v = *node.GetValue("anyToUnlock");
        if (!dotnet::TryParseBool(v, AnyParentToUnlock)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
    }
    std::string techID = "";
    if (node.HasValue("id")) techID = *node.GetValue("id");
    if (ResearchAndDevelopment::Instance != nullptr) tech = ResearchAndDevelopment::Instance->GetTechState(techID);
    if (tech == nullptr) tech = std::make_shared<ProtoTechNode>(node);
}

// ProtoRDNode.cs:147
void ProtoRDNode::LoadLinks(const ConfigNode& node, std::vector<std::shared_ptr<ProtoRDNode>>& rdNodes) {
    for (ConfigNode* configNode : node.GetNodes("Parent")) {
        if (configNode->HasValue("parentID")) {
            const std::string value = *configNode->GetValue("parentID");
            ProtoRDNode* protoRDNode = FindNodeByID(value, rdNodes);
            if (protoRDNode != nullptr) {
                parents.push_back(protoRDNode);
                protoRDNode->children.push_back(this);
            }
        }
    }
}

// ProtoRDNode.cs:167
ProtoRDNode* ProtoRDNode::FindNodeByID(const std::string& techID, std::vector<std::shared_ptr<ProtoRDNode>>& nodes) {
    for (auto& n : nodes)
        if (n->tech != nullptr && n->tech->techID == techID) return n.get();
    return nullptr;
}

// ----------------------------------------------------------------------------------------------------------------------------------
// RDTechTree (static data side)

void RDTechTree::Reset() {
    rdNodes.clear();
    rdNodesLoaded = false;
    TechTreeTechs.clear();
    TechTreeNodes.clear();
    treeNode.reset();
    backupTechTreeUrl = "GameData/Squad/Resources/TechTree.cfg";
}

// RDTechTree.cs:33
std::vector<std::shared_ptr<ProtoTechNode>> RDTechTree::GetTreeTechs() {
    PreLoad();
    return TechTreeTechs;
}

// RDTechTree.cs:39
std::vector<std::shared_ptr<ProtoRDNode>> RDTechTree::GetTreeNodes() {
    PreLoad();
    return TechTreeNodes;
}

// RDTechTree.cs:45
ConfigNode* RDTechTree::GetTreeConfigNode() {
    PreLoad();
    return treeNode.get();
}

// RDTechTree.cs:56
void RDTechTree::PreLoad() {
    if (!rdNodesLoaded) ReLoad();
}

// RDTechTree.cs:64
void RDTechTree::ReLoad() {
    GameParameters* p = CurrentParameters();
    ReLoad(p ? p->Career.TechTreeUrl : backupTechTreeUrl, true);
}

// RDTechTree.cs:69
void RDTechTree::ReLoad(std::string filePath, bool) {
    if (!rdNodesLoaded) {
        rdNodes.clear();
        rdNodesLoaded = true;
    } else {
        rdNodes.clear();
    }
    if (filePath.empty()) {
        GameParameters* p = CurrentParameters();
        filePath = p ? p->Career.TechTreeUrl : backupTechTreeUrl;
    }
    LoadTechTree(filePath, rdNodes, true);
    TechTreeTechs.assign(rdNodes.size(), nullptr);
    TechTreeNodes.assign(rdNodes.size(), nullptr);
    int count = static_cast<int>(rdNodes.size());
    while (count-- > 0) {
        TechTreeTechs[static_cast<size_t>(count)] = rdNodes[static_cast<size_t>(count)]->tech;
        TechTreeNodes[static_cast<size_t>(count)] = rdNodes[static_cast<size_t>(count)];
    }
}

namespace {
ConfigNode::NodePtr LoadConfigFile(const std::string& path) {
    return ScienceEnvironment::loadConfigFile ? ScienceEnvironment::loadConfigFile(path) : ConfigNode::Load(path);
}
}  // namespace

// RDTechTree.cs:305
bool RDTechTree::GetTechNodesFromDB(ConfigNode::NodePtr& treeNode_) {
    bool result = false;
    treeNode_ = ConfigNode::Create();
    std::vector<ConfigNode*> configNodes =
        ScienceEnvironment::getConfigNodes ? ScienceEnvironment::getConfigNodes("TechTree") : std::vector<ConfigNode*>();
    if (!configNodes.empty()) {
        result = true;
        treeNode_ = ConfigNode::Create();
        int num = static_cast<int>(configNodes.size());
        while (num-- > 0) configNodes[static_cast<size_t>(num)]->CopyTo(*treeNode_);
    }
    return result;
}

// RDTechTree.cs:223
void RDTechTree::LoadTechTree(std::string filePath, std::vector<std::shared_ptr<ProtoRDNode>>& rdNodes_, bool loadFromDatabase) {
    bool flag = false;
    if (loadFromDatabase) {
        treeNode = ConfigNode::Create();
        flag = GetTechNodesFromDB(treeNode);
    }
    if (!loadFromDatabase || !flag) {
        ConfigNode::NodePtr configNode = LoadConfigFile(filePath);
        if (configNode == nullptr || !configNode->HasNode("TechTree")) {
            log::Error("[Tech Tree]: file does not exist or has bad nodes. Loading from backup path " + backupTechTreeUrl);
            configNode = LoadConfigFile(backupTechTreeUrl);
            if (configNode == nullptr || !configNode->HasNode("TechTree")) {
                log::Info("[Tech Tree]: backup still not found or lacks nodes!");
                return;
            }
        }
        if (configNode->HasNode("TechTree")) {
            // treeNode = configNode.GetNode("TechTree") (a reference into the loaded tree): keep the tree alive as an owned copy
            treeNode = configNode->GetNode("TechTree")->CreateCopy();
        }
    }
    std::vector<ConfigNode*> nodes = treeNode->GetNodes("RDNode");
    const size_t num = nodes.size();
    for (size_t i = 0; i < num; i++) rdNodes_.push_back(std::make_shared<ProtoRDNode>(*nodes[i]));
    for (size_t j = 0; j < num; j++) rdNodes_[j]->LoadLinks(*nodes[j], rdNodes_);
}

// RDTechTree.cs:267
void RDTechTree::LoadTechTitles(std::string filePath, dotnet::Dictionary<std::string, std::string>& dict, bool loadFromDatabase) {
    bool flag = false;
    if (loadFromDatabase) {
        treeNode = ConfigNode::Create();
        flag = GetTechNodesFromDB(treeNode);
    }
    if (!loadFromDatabase || !flag) {
        ConfigNode::NodePtr configNode = LoadConfigFile(filePath);
        if (configNode == nullptr || !configNode->HasNode("TechTree")) {
            log::Error("[Tech Tree]: file does not exist or has bad nodes. Loading from backup path " + backupTechTreeUrl);
            configNode = LoadConfigFile(backupTechTreeUrl);
            if (configNode == nullptr || !configNode->HasNode("TechTree")) {
                log::Info("[Tech Tree]: backup still not found or lacks nodes!");
                return;
            }
        }
        if (configNode->HasNode("TechTree")) treeNode = configNode->GetNode("TechTree")->CreateCopy();
    }
    for (ConfigNode* configNode2 : treeNode->GetNodes("RDNode")) {
        if (configNode2->HasValue("id") && configNode2->HasValue("title")) dict.Set(*configNode2->GetValue("id"), *configNode2->GetValue("title"));
    }
}

// RDTechTree.cs:323
std::vector<std::shared_ptr<ProtoTechNode>> RDTechTree::GetCheapestUnavailableNodes(int tolerance) {
    PreLoad();
    int num = std::numeric_limits<int>::max();
    const int num2 = static_cast<int>(TechTreeTechs.size());
    int num3 = num2;
    while (num3-- > 0) {
        ProtoTechNode* protoTechNode = TechTreeTechs[static_cast<size_t>(num3)].get();
        if (protoTechNode != nullptr && ResearchAndDevelopment::GetTechnologyState(protoTechNode->techID) == RDTech_State::Unavailable &&
            num > protoTechNode->scienceCost)
            num = protoTechNode->scienceCost;
    }
    std::vector<std::shared_ptr<ProtoTechNode>> list;
    for (int i = 0; i < num2; i++) {
        auto protoTechNode = TechTreeTechs[static_cast<size_t>(i)];
        if (protoTechNode->scienceCost - tolerance <= num) list.push_back(protoTechNode);
    }
    return list;
}

// RDTechTree.cs:349
std::vector<std::shared_ptr<ProtoTechNode>> RDTechTree::GetNextUnavailableNodes() {
    PreLoad();
    std::vector<std::shared_ptr<ProtoTechNode>> list;
    ProtoRDNode* node = nullptr;
    for (auto& protoRDNode : TechTreeNodes) {
        if (protoRDNode->parents.empty()) {
            node = protoRDNode.get();
            break;
        }
    }
    recurseForNextTechs(node, list);
    return list;
}

// RDTechTree.cs:368
void RDTechTree::recurseForNextTechs(ProtoRDNode* node, std::vector<std::shared_ptr<ProtoTechNode>>& techs) {
    if (node == nullptr) throw NullReferenceException("RDTechTree.recurseForNextTechs: node");
    if (ResearchAndDevelopment::GetTechnologyState(node->tech->techID) == RDTech_State::Available) {
        for (size_t i = 0; i < node->children.size(); i++) recurseForNextTechs(node->children[i], techs);
    } else {
        techs.push_back(node->tech);
    }
}

// RDTechTree.cs:384
std::shared_ptr<ProtoTechNode> RDTechTree::FindTech(const std::string& requiredTechId) {
    PreLoad();
    for (auto& t : TechTreeTechs)
        if (t->techID == requiredTechId) return t;
    if (!requiredTechId.empty()) log::Warning("[RDTechTree]: No tech node found called " + requiredTechId);
    return nullptr;
}

// ----------------------------------------------------------------------------------------------------------------------------------
// ExperimentResultDialogPage

// ExperimentResultDialogPage.cs:64
ExperimentResultDialogPage::ExperimentResultDialogPage(Part* host_, const std::shared_ptr<ScienceData>& experimentData, float xmitBase,
                                                       float xmitBonus, bool showTransmitWarning_, std::string transmitWarningMessage_,
                                                       bool showResetOption, std::shared_ptr<ScienceLabSearch> labSearch_,
                                                       ScienceCallback onDiscardData, ScienceCallback onKeepData, ScienceCallback onTransmitData,
                                                       ScienceCallback onSendToLab) {
    host = host_;
    pageData = experimentData;
    double num = 0.0;
    if (ScienceEnvironment::commNetEnabled && ScienceEnvironment::commNetEnabled() && host_ != nullptr && host_->vessel != nullptr &&
        ScienceEnvironment::commNetScienceCurve) {
        std::optional<double> curve = ScienceEnvironment::commNetScienceCurve(*host_->vessel);
        if (curve) num = *curve;
    }
    const float mult = ScienceGainMultiplier();
    std::shared_ptr<ScienceSubject> subjectByID = ResearchAndDevelopment::GetSubjectByID(experimentData->subjectID);
    if (subjectByID != nullptr) {
        remainingScience = f32store((fx(subjectByID->scienceCap) - fx(subjectByID->science)) * fx(mult));
        refValue = f32store(fx(ResearchAndDevelopment::GetReferenceDataValue(experimentData->dataAmount, *subjectByID)) * fx(mult));
        scienceValue = f32store(fx(ResearchAndDevelopment::GetScienceValue(experimentData->dataAmount, experimentData->scienceValueRatio, *subjectByID)) * fx(mult));
        baseTransmitValue =
            f32store(fx(ResearchAndDevelopment::GetScienceValue(experimentData->dataAmount, experimentData->scienceValueRatio, *subjectByID, xmitBase)) * fx(mult));
        scienceValueRatio = experimentData->scienceValueRatio;
        if (scienceValue > 0.0f) {
            xmitBonus = static_cast<float>(1.0 + num);
            pageData->baseTransmitValue = xmitBase;
            if (fx(baseTransmitValue) * fx(xmitBonus) > fx(scienceValue)) xmitBonus = f32store(fx(scienceValue) / fx(baseTransmitValue));
            pageData->transmitBonus = xmitBonus;
            if (fx(xmitBase) * fx(xmitBonus) > 0.0 && baseTransmitValue > 0.0f && refValue > 0.0f) CommBonus = f32store(fx(xmitBonus) - 1.0);
            else CommBonus = 0.0f;
        } else {
            xmitBonus = 1.0f;
            pageData->baseTransmitValue = 0.0f;
            pageData->transmitBonus = 0.0f;
            CommBonus = 0.0f;
        }
        TransmitBonus = xmitBonus;
        valueAfterRecovery = f32store(fx(ResearchAndDevelopment::GetNextScienceValue(experimentData->dataAmount, *subjectByID)) * fx(mult));
        valueAfterTransmit = f32store(
            fx(ResearchAndDevelopment::GetNextScienceValue(experimentData->dataAmount, *subjectByID, f32store(fx(xmitBase) * fx(xmitBonus)))) * fx(mult));
        resultText = experimentData->title + ":\n\n";
        resultText += ResearchAndDevelopment::GetResults(subjectByID->id);
        if (!experimentData->extraResultString.empty()) resultText = resultText + "\n\n" + Loc::Format(experimentData->extraResultString);
    } else {
        refValue = 1.0f;
        scienceValue = 0.0f;
        baseTransmitValue = 0.0f;
        TransmitBonus = 1.0f;
        valueAfterRecovery = 0.0f;
        valueAfterTransmit = 0.0f;
        scienceValueRatio = 1.0f;
        resultText = experimentData->title + ":\n\n";
        resultText += ResearchAndDevelopment::GetResults(experimentData->subjectID);
        resultText = resultText + "\n\n" + Loc::Format(experimentData->extraResultString);
    }
    const std::string expId = experimentData->subjectID.substr(0, experimentData->subjectID.find('@'));
    ScienceExperiment* experiment = ResearchAndDevelopment::GetExperiment(expId);
    if (experiment == nullptr) throw NullReferenceException("ExperimentResultDialogPage: experiment " + expId);
    title = experiment->experimentTitle;
    dataSize = experimentData->dataAmount;
    xmitDataScalar = xmitBase;
    showTransmitWarning = showTransmitWarning_;
    transmitWarningMessage = std::move(transmitWarningMessage_);
    showReset = showResetOption;
    labSearch = std::move(labSearch_);
    OnDiscardData = std::move(onDiscardData);
    OnKeepData = std::move(onKeepData);
    OnTransmitData = std::move(onTransmitData);
    OnSendToLab = std::move(onSendToLab);
}

// ExperimentResultDialogPage.cs:148
float ExperimentResultDialogPage::UpdatePageLabValue() {
    pageData->labValue = 0.0f;
    Vessel* activeVessel = dynamic_cast<Vessel*>(FlightGlobals::ActiveVessel);
    if (activeVessel != nullptr) {
        pageData->labValue = refValue;
        auto* moduleScienceLab = dynamic_cast<ModuleScienceLab*>(activeVessel->FindPartModuleImplementingClass("ModuleScienceLab"));
        if (moduleScienceLab != nullptr) {
            if (activeVessel->Landed) pageData->labValue = f32store(fx(pageData->labValue) * (1.0 + fx(moduleScienceLab->Get<float>("SurfaceBonus"))));
            if (pageData->subjectID.find(FlightGlobals::currentMainBody->bodyName) != std::string::npos)
                pageData->labValue = f32store(fx(pageData->labValue) * (1.0 + fx(moduleScienceLab->Get<float>("ContextBonus"))));
            if ((activeVessel->Landed || activeVessel->Splashed) && activeVessel->mainBody() == HomeBody())
                pageData->labValue = f32store(fx(pageData->labValue) * fx(moduleScienceLab->Get<float>("homeworldMultiplier")));
        }
        pageData->labValue = static_cast<float>(dotnet::Math::Round(static_cast<double>(pageData->labValue)));
    }
    return pageData->labValue;
}

// ----------------------------------------------------------------------------------------------------------------------------------
// RnDCommsStream

// RnDCommsStream.cs:222
RnDCommsStream::RnDCommsStream(std::shared_ptr<ScienceSubject> subject_, float fileSize_, float timeout_, float xmitValue_, bool xmitIncomplete_,
                               ResearchAndDevelopment* RDInstance)
    : RnDCommsStream(std::move(subject_), fileSize_, timeout_, xmitValue_, 1.0f, xmitIncomplete_, RDInstance) {}

// RnDCommsStream.cs:227
RnDCommsStream::RnDCommsStream(std::shared_ptr<ScienceSubject> subject_, float fileSize_, float timeout_, float xmitValue_,
                               float scienceValueRatio_, bool xmitIncomplete_, ResearchAndDevelopment* RDInstance) {
    subject = std::move(subject_);
    fileSize = fileSize_;
    timeout = timeout_;
    xmitValue = xmitValue_;
    xmitIncomplete = xmitIncomplete_;
    host_ = RDInstance;
    scienceValueRatio = scienceValueRatio_;
    dataIn_ = 0.0f;
    dataOut_ = 0.0f;
    timedOut_ = true;
}

// RnDCommsStream.cs:241
void RnDCommsStream::StreamData(float dataAmount, ProtoVessel* source) {
    dataIn_ = Mathf::Min(fileSize, f32store(fx(dataIn_) + fx(dataAmount)));
    UTofLastTransmit_ = Time::time;
    if (dataIn_ == fileSize) {
        submitStreamData(source);
    } else if (timedOut_) {
        ScienceEnvironment::Scheduler().StartCoroutine(timeoutCoroutine(source));
    }
}

// RnDCommsStream.cs:255
Enumerator RnDCommsStream::timeoutCoroutine(ProtoVessel* source) {
    timedOut_ = false;
    while (!timedOut_ && !(dataIn_ >= fileSize)) {
        co_yield YieldInstruction::WaitForSeconds(1.0f);
        if (fx(Time::time) > fx(UTofLastTransmit_) + fx(timeout)) {
            timedOut_ = true;
            if (!xmitIncomplete && dataIn_ < fileSize) {
                dataIn_ = 0.0f;
                dataOut_ = 0.0f;
                break;
            }
            submitStreamData(source);
        }
    }
}

// RnDCommsStream.cs:275
void RnDCommsStream::submitStreamData(ProtoVessel* source) {
    float num = f32store(fx(dataIn_) - fx(dataOut_));
    if (num > 0.0f) host_->SubmitScienceData(num, scienceValueRatio, *subject, xmitValue, source);
    dataOut_ = f32store(fx(dataOut_) + fx(num));
}

// ----------------------------------------------------------------------------------------------------------------------------------
// ResearchAndDevelopment

ResearchAndDevelopment::ResearchAndDevelopment() : ScenarioModule("ResearchAndDevelopment") {}

// ResearchAndDevelopment.cs:106 OnDestroy
ResearchAndDevelopment::~ResearchAndDevelopment() {
    GameEvents::onVesselRecoveryProcessing.Remove(this, &ResearchAndDevelopment::onVesselRecoveryProcessing);
    if (Instance != nullptr && Instance == this) Instance = nullptr;
}

void ResearchAndDevelopment::ResetStatics() {
    experiments.reset();
    minibiomesbyScienceID.reset();
    minibiomesbyUnityTag.reset();
    techTitles.Clear();
}

// ResearchAndDevelopment.cs:53
void ResearchAndDevelopment::AddScience(float value, TransactionReasons reason) {
    science = f32store(fx(science) + fx(value));
    if (!AllowNegativeCurrency()) science = Mathf::Max(science, 0.0f);
    // CurrencyModifierQuery(reason, 0f, value, 0f) -> OnCurrencyModifierQuery / OnCurrencyModified (WP-36 seam)
    if (ScienceEnvironment::currencyEffectDelta) OnCurrenciesModified(ScienceEnvironment::currencyEffectDelta(reason, value), reason);
    GameEvents::OnScienceChanged.Fire(science, reason);
}

// ResearchAndDevelopment.cs:66
void ResearchAndDevelopment::SetScience(float value, TransactionReasons reason) {
    science = value;
    if (!AllowNegativeCurrency()) science = Mathf::Max(science, 0.0f);
    GameEvents::OnScienceChanged.Fire(science, reason);
}

// ResearchAndDevelopment.cs:76
bool ResearchAndDevelopment::CanAfford(float amount) {
    if (Instance != nullptr) return UtilMath::RoundToPlaces(Instance->Science(), 1) >= UtilMath::RoundToPlaces(amount, 1);
    return true;
}

// ResearchAndDevelopment.cs:85
void ResearchAndDevelopment::OnAwake() {
    if (Instance != nullptr) log::Error("[ResearchAndDevelopment Module]: Instance already exists!");
    Instance = this;
    protoTechNodes.Clear();
    scienceSubjects.Clear();
    recoveredDataWidgets.clear();
    experimentalPartsStock_.Clear();
    if (GameParameters* p = CurrentParameters()) science = p->Career.StartingScience;
    GameEvents::onVesselRecoveryProcessing.Add(this, &ResearchAndDevelopment::onVesselRecoveryProcessing);
}

// ResearchAndDevelopment.cs:117
void ResearchAndDevelopment::OnLoad(ConfigNode& node) {
    if (node.HasValue("sci")) science = dotnet::ParseSingle(*node.GetValue("sci"));
    protoTechNodes.Clear();
    techTitles.Clear();
    if (node.GetNode("Tech") == nullptr) {
        log::Info("[R&D]: No tech nodes defined. Assuming initial state");
        node.AddNode(getInitialTech());
    }
    for (ConfigNode* n : node.GetNodes("Tech")) {
        auto protoTechNode = std::make_shared<ProtoTechNode>(*n);
        protoTechNodes.Add(protoTechNode->techID, protoTechNode);
    }
    for (ConfigNode* n : node.GetNodes("Science")) {
        auto scienceSubject = std::make_shared<ScienceSubject>(*n);
        scienceSubjects.Add(scienceSubject->id, scienceSubject);
    }
    experimentalPartsStock_.Clear();
    if (node.HasNode("ExpParts")) {
        const ConfigNode::ValueList& values = node.GetNode("ExpParts")->values();
        for (int k = 0; k < values.Count(); k++) {
            const ConfigNode::Value& value = values[k];
            const AvailablePart* ap = PartInfoByName(value.name);
            if (ap == nullptr) throw ArgumentException("Value cannot be null. (Parameter 'key')");
            experimentalPartsStock_.Add(ap, dotnet::ParseInt32(value.value));
        }
    }
}

// ResearchAndDevelopment.cs:157
void ResearchAndDevelopment::loadExperiments() {
    std::vector<ConfigNode*> configNodes =
        ScienceEnvironment::getConfigNodes ? ScienceEnvironment::getConfigNodes("EXPERIMENT_DEFINITION") : std::vector<ConfigNode*>();
    for (ConfigNode* cn : configNodes) {
        ScienceExperiment scienceExperiment;
        scienceExperiment.Load(*cn);
        const std::string eid = scienceExperiment.id ? *scienceExperiment.id : std::string();
        if (!scienceExperiment.id.has_value()) throw ArgumentException("Value cannot be null. (Parameter 'key')");
        if (!experiments->ContainsKey(eid)) experiments->Add(eid, scienceExperiment);
        else log::Warning("[R&D]: Error Duplicate found adding Experiment Definition: " + eid);
    }
    if (!experiments->ContainsKey("recovery")) {
        ScienceExperiment scienceExperiment2;
        scienceExperiment2.id = std::string("recovery");
        scienceExperiment2.experimentTitle = Loc::Format("#autoLOC_300438");
        scienceExperiment2.scienceCap = 0.0f;
        scienceExperiment2.dataScale = 0.0f;
        experiments->Add("recovery", scienceExperiment2);
    }
}

// ResearchAndDevelopment.cs:185
void ResearchAndDevelopment::loadMiniBiomes() {
    minibiomesbyScienceID.emplace();
    minibiomesbyUnityTag.emplace();
    if (FlightGlobals::bodies == nullptr) return;
    for (CelestialBody* celestialBody : *FlightGlobals::bodies) {
        for (const MiniBiome& mb : celestialBody->MiniBiomes) {
            if (minibiomesbyScienceID->find(mb.TagKeyID) == minibiomesbyScienceID->end()) minibiomesbyScienceID->emplace(mb.TagKeyID, &mb);
            if (minibiomesbyUnityTag->find(mb.TagKey) == minibiomesbyUnityTag->end()) minibiomesbyUnityTag->emplace(mb.TagKey, &mb);
        }
    }
}

// ResearchAndDevelopment.cs:211
void ResearchAndDevelopment::OnSave(ConfigNode& node) {
    node.AddValue("sci", science);
    for (const auto& kv : protoTechNodes) {
        const std::shared_ptr<ProtoTechNode>& current = kv.second;
        if (current->state == RDTech_State::Available) current->Save(*node.AddNode("Tech"));
    }
    for (const auto& kv : scienceSubjects) kv.second->Save(*node.AddNode("Science"));
    if (experimentalPartsStock_.Count() > 0) {
        ConfigNode* configNode = node.AddNode("ExpParts");
        for (const auto& kv : experimentalPartsStock_) configNode->AddValue(kv.first->name, kv.second);
    }
}

// ResearchAndDevelopment.cs:240
std::shared_ptr<ConfigNode> ResearchAndDevelopment::getInitialTech() {
    auto configNode = ConfigNode::Create("Tech");
    configNode->AddValue("id", "start");
    configNode->AddValue("state", dotnet::EnumToString(RDTech_State::Available));
    const std::string text = "start";
    for (const AvailablePart* availablePart : LoadedParts()) {
        if (availablePart->TechRequired == text) configNode->AddValue("part", availablePart->name);
    }
    return configNode;
}

// ResearchAndDevelopment.cs:259
void ResearchAndDevelopment::Start() {
    // HighLogic.LoadedSceneIsEditor: the editor part list filters are UI (EditorPartList.Instance.ExcludeFilters / GreyoutFilters)
    loadMiniBiomes();
    const bool fundingExists = ScienceEnvironment::fundingInstanceExists && ScienceEnvironment::fundingInstanceExists();
    const SaveEnvironment& env = SaveEnvironment::Current();
    const bool bypass = CurrentParameters() ? CurrentParameters()->Difficulty.BypassEntryPurchaseAfterResearch : false;
    if ((!env.loadedSceneIsGame || !bypass) && fundingExists) return;
    for (const AvailablePart* availablePart : LoadedParts()) {
        std::shared_ptr<ProtoTechNode>* tn = protoTechNodes.Find(availablePart->TechRequired);
        if (tn != nullptr && (*tn)->state == RDTech_State::Available) {
            auto& purchased = (*tn)->partsPurchased;
            if (std::find(purchased.begin(), purchased.end(), availablePart) == purchased.end()) purchased.push_back(availablePart);
        }
    }
}

// ResearchAndDevelopment.cs:286
void ResearchAndDevelopment::AddExperimentalPart(const AvailablePart* ap) {
    if (Instance != nullptr) {
        int* n = Instance->experimentalPartsStock_.Find(ap);
        if (n != nullptr) (*n)++;
        else Instance->experimentalPartsStock_.Add(ap, 1);
    }
}

// ResearchAndDevelopment.cs:301
bool ResearchAndDevelopment::IsExperimentalPart(const AvailablePart* ap) {
    if (Instance != nullptr && ap != nullptr) return Instance->experimentalPartsStock_.ContainsKey(ap);
    return false;
}

// ResearchAndDevelopment.cs:310
void ResearchAndDevelopment::RemoveExperimentalPart(const AvailablePart* ap) {
    if (Instance != nullptr && Instance->experimentalPartsStock_.ContainsKey(ap)) {
        Instance->experimentalPartsStock_.At(ap)--;
        if (Instance->experimentalPartsStock_.At(ap) == 0) Instance->experimentalPartsStock_.Remove(ap);
    }
}

// ResearchAndDevelopment.cs:331
void ResearchAndDevelopment::onVesselRecoveryProcessing(ProtoVessel* pv, KSP::UI::Screens::MissionRecoveryDialog* mrDialog, float) {
    if (pv == nullptr) return;
    const bool flag = mrDialog != nullptr;
    recoveredDataWidgets.clear();
    std::vector<ProtoPartSnapshot*> allProtoPartsIncludingCargo = pv->GetAllProtoPartsIncludingCargo();
    for (ProtoPartSnapshot* protoPartSnapshot : allProtoPartsIncludingCargo) {
        const std::string partName = protoPartSnapshot->partName ? *protoPartSnapshot->partName : std::string();
        for (auto& module : protoPartSnapshot->modules) {
            ConfigNode& moduleValues = *module->moduleValues;
            if (!moduleValues.HasNode("ScienceData")) continue;
            for (ConfigNode* configNode : moduleValues.GetNodes("ScienceData")) {
                ScienceData scienceData(*configNode);
                std::shared_ptr<ScienceSubject>* sp = scienceSubjects.Find(scienceData.subjectID);
                if (sp != nullptr) {
                    std::shared_ptr<ScienceSubject> scienceSubject = *sp;
                    scienceSubject->title = scienceData.title;
                    float num3 = 0.0f;
                    float value = 1.0f;
                    if (configNode->HasValue("data")) num3 = dotnet::ParseSingle(*configNode->GetValue("data"));
                    configNode->TryGetValue("scienceValueRatio", value);
                    if (num3 != 0.0f) {
                        float num = SubmitScienceData(num3, value, *scienceSubject, 1.0f, pv);
                        if (flag) recoveredDataWidgets.push_back({scienceSubject, num3, num});
                    } else if (flag) {
                        log::Warning("Research & Development]: Experiment in " + partName + " has subject " + scienceData.subjectID +
                                     " defined, but no data was gathered.");
                    }
                } else if (flag) {
                    if (!scienceData.subjectID.empty())
                        log::Error("[Research & Development]: Experiment in " + partName + " has scienceSubject " + scienceData.subjectID +
                                   " defined, but no such subject exists.");
                    else
                        log::Info("[Research & Development]: Experiment in " + partName + " has no scienceSubject defined, it seems no science was done here.");
                }
            }
        }
    }
    reverseEngineerRecoveredVessel(pv, mrDialog);
    ModuleScienceLab::RecoverScienceLabs(*pv, mrDialog);
    // mrDialog.RnDOperational / AddDataWidget / scienceEarned: the recovery dialog is UI (the widgets are in recoveredDataWidgets)
}

// ResearchAndDevelopment.cs:413
void ResearchAndDevelopment::OnCurrenciesModified(float effectDelta, TransactionReasons reason) {
    science = f32store(fx(science) + fx(effectDelta));
    if (effectDelta != 0.0f) GameEvents::OnScienceChanged.Fire(science, reason);
}

// ResearchAndDevelopment.cs:423
void ResearchAndDevelopment::reverseEngineerRecoveredVessel(ProtoVessel* pv, KSP::UI::Screens::MissionRecoveryDialog* mrDialog) {
    VesselTripLog vesselTripLog = VesselTripLog::FromProtoVessel(*pv);
    std::vector<std::shared_ptr<ScienceSubject>> list;
    const std::string homeBodyName = HomeBody()->bodyName;
    std::vector<std::string> list2;
    std::vector<std::string> list3;
    list3.push_back(homeBodyName);
    CelestialBody* bodyByName = FindBodyByName(homeBodyName);
    using ET = FlightLog::EntryType;
    auto append = [&list](std::vector<std::shared_ptr<ScienceSubject>> v) { list.insert(list.end(), v.begin(), v.end()); };
    if (vesselTripLog.Log.HasEntry(ET::Orbit, homeBodyName))
        append(reverseEngineerPartsFrom(list3, list2, 10.0f, "Orbited", Fmt("#autoLOC_300432", {bodyByName->bodyDisplayName})));
    if (vesselTripLog.Log.HasEntry(ET::Suborbit, homeBodyName))
        append(reverseEngineerPartsFrom(list3, list2, 8.0f, "SubOrbited", Loc::Format("#autoLOC_300435")));
    if (vesselTripLog.Log.HasEntry(ET::Flight, homeBodyName))
        append(reverseEngineerPartsFrom(list3, list2, 5.0f, "Flew", Loc::Format("#autoLOC_300438")));
    if (std::find(list2.begin(), list2.end(), homeBodyName) == list2.end()) list2.push_back(homeBodyName);
    append(reverseEngineerPartsFrom(vesselTripLog.Log.GetDistinctTargets(ET::Land), list2, 15.0f, "Surfaced", "#autoLOC_6001096"));
    append(reverseEngineerPartsFrom(vesselTripLog.Log.GetDistinctTargets(ET::Flight), list2, 12.0f, "Flew", "#autoLOC_6001097"));
    append(reverseEngineerPartsFrom(vesselTripLog.Log.GetDistinctTargets(ET::Suborbit), list2, 10.0f, "SubOrbited", "#autoLOC_6001098"));
    append(reverseEngineerPartsFrom(vesselTripLog.Log.GetDistinctTargets(ET::Orbit), list2, 8.0f, "Orbited", "#autoLOC_6001099"));
    append(reverseEngineerPartsFrom(vesselTripLog.Log.GetDistinctTargets(ET::Flyby), list2, 6.0f, "FlewBy", "#autoLOC_6001100"));
    if (list.size() <= 0) return;
    std::shared_ptr<ScienceSubject> subject;
    float num = std::numeric_limits<float>::lowest();
    int count = static_cast<int>(list.size());
    while (count-- > 0) {
        std::shared_ptr<ScienceSubject> scienceSubject = list[static_cast<size_t>(count)];
        if (scienceSubject->subjectValue > num) {
            subject = scienceSubject;
            num = scienceSubject->subjectValue;
        }
    }
    std::shared_ptr<ScienceSubject> scienceSubject2 = getScienceSubject(subject);
    float scienceAmount = SubmitScienceData(1.0f, *scienceSubject2, 1.0f, pv, true);
    if (mrDialog != nullptr) recoveredDataWidgets.push_back({scienceSubject2, 1.0f, scienceAmount});
}

// ResearchAndDevelopment.cs:477
std::vector<std::shared_ptr<ScienceSubject>> ResearchAndDevelopment::reverseEngineerPartsFrom(std::vector<std::string> fromCBs,
                                                                                               std::vector<std::string>& ignoreCBs, float subValue,
                                                                                               const std::string& idVerb, const std::string& returnedFrom) {
    std::vector<std::shared_ptr<ScienceSubject>> list;
    for (const std::string& text : fromCBs) {
        CelestialBody* bodyByName = FindBodyByName(text);
        if (bodyByName != nullptr && std::find(ignoreCBs.begin(), ignoreCBs.end(), text) == ignoreCBs.end()) {
            ignoreCBs.push_back(text);
            const float rv = bodyByName->scienceValues.RecoveryValue;
            list.push_back(std::make_shared<ScienceSubject>("recovery@" + text + idVerb, Fmt(returnedFrom.c_str(), {bodyByName->bodyName}), 1.0f,
                                                            f32store(fx(rv) * fx(subValue)), f32store(fx(rv) * fx(subValue) * fx(1.2f))));
        }
    }
    return list;
}

// ResearchAndDevelopment.cs:494
bool ResearchAndDevelopment::partTechAvailable(const AvailablePart* ap) {
    if (ap->TechRequired.empty()) return false;
    if (std::shared_ptr<ProtoTechNode>* tn = protoTechNodes.Find(ap->TechRequired)) return (*tn)->state == RDTech_State::Available;
    if (experimentalPartsStock_.ContainsKey(ap)) return experimentalPartsStock_.At(ap) > 0;
    return false;
}

// ResearchAndDevelopment.cs:511
bool ResearchAndDevelopment::partModelPurchased(const AvailablePart* ap) {
    if (partTechAvailable(ap)) {
        if (std::shared_ptr<ProtoTechNode>* tn = protoTechNodes.Find(ap->TechRequired)) {
            auto& purchased = (*tn)->partsPurchased;
            if (std::find(purchased.begin(), purchased.end(), ap) != purchased.end()) return true;
        }
        if (experimentalPartsStock_.ContainsKey(ap)) return experimentalPartsStock_.At(ap) > 0;
        return false;
    }
    return false;
}

// ResearchAndDevelopment.cs:528
std::shared_ptr<ProtoTechNode> ResearchAndDevelopment::GetTechState(const std::string& techID) {
    if (std::shared_ptr<ProtoTechNode>* tn = protoTechNodes.Find(techID)) return *tn;
    return nullptr;
}

// ResearchAndDevelopment.cs:537
void ResearchAndDevelopment::SetTechState(const std::string& techID, std::shared_ptr<ProtoTechNode> techNode) {
    protoTechNodes.Set(techID, std::move(techNode));
}

// ResearchAndDevelopment.cs:549
RDTech_State ResearchAndDevelopment::GetTechnologyState(const std::string& techID) {
    if (Instance == nullptr) return RDTech_State::Available;
    if (std::shared_ptr<ProtoTechNode>* tn = Instance->protoTechNodes.Find(techID)) return (*tn)->state;
    return RDTech_State::Unavailable;
}

// ResearchAndDevelopment.cs:562
std::string ResearchAndDevelopment::GetTechnologyTitle(const std::string& techID) {
    if (techTitles.Count() == 0) {
        GameParameters* p = CurrentParameters();
        RDTechTree::LoadTechTitles(p ? p->Career.TechTreeUrl : RDTechTree::backupTechTreeUrl, techTitles, true);
    }
    std::string value;
    if (techTitles.TryGetValue(techID, value)) return value;
    return std::string();
}

// ResearchAndDevelopment.cs:575
bool ResearchAndDevelopment::PartTechAvailable(const AvailablePart* ap) {
    if (Instance != nullptr) return Instance->partTechAvailable(ap);
    return true;
}

// ResearchAndDevelopment.cs:584
bool ResearchAndDevelopment::PartModelPurchased(const AvailablePart* ap) {
    if (Instance != nullptr) return Instance->partModelPurchased(ap);
    return true;
}

// ResearchAndDevelopment.cs:593
ScienceExperiment* ResearchAndDevelopment::GetExperiment(const std::string& experimentID) {
    if (!experiments.has_value()) {
        experiments.emplace();
        loadExperiments();
    }
    if (ScienceExperiment* e = experiments->Find(experimentID)) return e;
    log::Warning("[R&D]: No Experiment definition found with id " + experimentID);
    return nullptr;
}

// ResearchAndDevelopment.cs:608
std::shared_ptr<ScienceSubject> ResearchAndDevelopment::GetExperimentSubject(const ScienceExperiment& experiment, ExperimentSituations situation,
                                                                             const CelestialBody& body, const std::string& biome,
                                                                             const std::string& displaybiome) {
    auto scienceSubject = std::make_shared<ScienceSubject>(experiment, situation, body, biome, displaybiome);
    if (!Instance) return scienceSubject;
    return Instance->getScienceSubject(scienceSubject);
}

// ResearchAndDevelopment.cs:618
std::shared_ptr<ScienceSubject> ResearchAndDevelopment::GetExperimentSubject(const ScienceExperiment& experiment, ExperimentSituations situation,
                                                                             const std::string& sourceUId, const std::string& sourceTitle,
                                                                             const CelestialBody& body, const std::string& biome,
                                                                             const std::string& displaybiome) {
    auto scienceSubject = std::make_shared<ScienceSubject>(experiment, situation, sourceUId, sourceTitle, body, biome, displaybiome);
    if (!Instance) return scienceSubject;
    return Instance->getScienceSubject(scienceSubject);
}

// ResearchAndDevelopment.cs:628
std::shared_ptr<ScienceSubject> ResearchAndDevelopment::GetSubjectByID(const std::string& subjectID) {
    if (Instance != nullptr) {
        if (std::shared_ptr<ScienceSubject>* s = Instance->scienceSubjects.Find(subjectID)) return *s;
        return nullptr;
    }
    return nullptr;
}

// ResearchAndDevelopment.cs:641
float ResearchAndDevelopment::GetReferenceDataValue(float dataAmount, ScienceSubject& subject) {
    if (GetReferenceDataValueFunc) return GetReferenceDataValueFunc(dataAmount, subject);
    // the initial value of GetReferenceDataValueFunc (:47): dataAmount / subject.dataScale * subject.subjectValue
    return f32store(fx(dataAmount) / fx(subject.dataScale) * fx(subject.subjectValue));
}

// ResearchAndDevelopment.cs:646
float ResearchAndDevelopment::GetSubjectValue(float subjectScience, ScienceSubject& subject) {
    if (GetSubjectValueFunc) return GetSubjectValueFunc(subjectScience, subject);
    // the initial value of GetSubjectValueFunc (:49): Mathf.Max(0f, 1f - subjectScience / subject.scienceCap)
    return Mathf::Max(0.0f, f32store(1.0 - fx(subjectScience) / fx(subject.scienceCap)));
}

// ResearchAndDevelopment.cs:651
float ResearchAndDevelopment::GetScienceValue(float dataAmount, ScienceSubject& subject, float xmitScalar) {
    return GetScienceValue(dataAmount, 1.0f, subject, xmitScalar);
}

// ResearchAndDevelopment.cs:656
float ResearchAndDevelopment::GetScienceValue(float dataAmount, float scienceValueRatio, ScienceSubject& subject, float xmitScalar) {
    float referenceDataValue = GetReferenceDataValue(dataAmount, subject);
    float num = Mathf::Min(f32store(fx(referenceDataValue) * fx(subject.scientificValue) * fx(xmitScalar)),
                           f32store(fx(subject.scienceCap) * fx(scienceValueRatio)));
    float b = f32store(fx(Mathf::Lerp(referenceDataValue, f32store(fx(subject.scienceCap) * fx(scienceValueRatio)), xmitScalar)) * fx(xmitScalar));
    return Mathf::Max(f32store(fx(Mathf::Min(f32store(fx(subject.science) + fx(num)), b)) - fx(subject.science)), 0.0f);
}

// ResearchAndDevelopment.cs:664
float ResearchAndDevelopment::GetNextScienceValue(float dataAmount, ScienceSubject& subject, float xmitScalar) {
    float num = f32store(fx(subject.science) + fx(GetScienceValue(dataAmount, subject, xmitScalar)));
    float num2 = subject.scientificValue;
    if (subject.applyScienceScale) num2 = GetSubjectValue(num, subject);
    float num3 = Mathf::Min(f32store(fx(GetReferenceDataValue(dataAmount, subject)) * fx(num2) * fx(xmitScalar)), subject.scienceCap);
    float b = f32store(fx(Mathf::Lerp(GetReferenceDataValue(dataAmount, subject), subject.scienceCap, xmitScalar)) * fx(xmitScalar));
    return Mathf::Max(f32store(fx(Mathf::Min(f32store(fx(num) + fx(num3)), b)) - fx(num)), 0.0f);
}

// ResearchAndDevelopment.cs:677
std::shared_ptr<ScienceSubject> ResearchAndDevelopment::getScienceSubject(const std::shared_ptr<ScienceSubject>& subject) {
    if (std::shared_ptr<ScienceSubject>* sp = scienceSubjects.Find(subject->id)) {
        std::shared_ptr<ScienceSubject> scienceSubject = *sp;
        scienceSubject->title = subject->title;
        scienceSubject->scienceCap = subject->scienceCap;
        scienceSubject->subjectValue = subject->subjectValue;
        scienceSubject->dataScale = subject->dataScale;
        return scienceSubject;
    }
    subject->scientificValue = 1.0f;
    scienceSubjects.Add(subject->id, subject);
    return subject;
}

// ResearchAndDevelopment.cs:693
float ResearchAndDevelopment::SubmitScienceData(float dataAmount, ScienceSubject& subject, float xmitScalar, ProtoVessel* source, bool reverseEngineered) {
    return SubmitScienceData(dataAmount, 1.0f, subject, xmitScalar, source, reverseEngineered);
}

// ResearchAndDevelopment.cs:698
float ResearchAndDevelopment::SubmitScienceData(float dataAmount, float scienceValueRatio, ScienceSubject& subject, float xmitScalar,
                                                ProtoVessel* source, bool reverseEngineered) {
    const float damage = ScienceEnvironment::rndFacilityDamage ? ScienceEnvironment::rndFacilityDamage() : 0.0f;
    if (damage >= 100.0f) {
        if (ScienceEnvironment::loadedSceneIsFlight) ScienceEnvironment::PostScreenMessage(Loc::Format("#autoLOC_300831"));
        return 0.0f;
    }
    float scienceValue = GetScienceValue(dataAmount, scienceValueRatio, subject, xmitScalar);
    subject.science = f32store(fx(subject.science) + fx(scienceValue));
    if (subject.applyScienceScale) subject.scientificValue = GetSubjectValue(subject.science, subject);
    scienceValue = f32store(fx(scienceValue) * fx(ScienceGainMultiplier()));
    TransactionReasons reason = (xmitScalar == 1.0f) ? science_reason::VesselRecovery() : science_reason::ScienceTransmission();
    std::string text = ScienceTransmissionRewardString(scienceValue, reason);
    log::Info("[Research & Development]: +" + dotnet::FormatSingleG(dataAmount) + " data on " + subject.title + "." + text + " Subject value is " +
              dotnet::FormatSingleF(subject.scientificValue, 2));
    AddScience(scienceValue, reason);
    if (ScienceEnvironment::loadedSceneIsFlight)
        ScienceEnvironment::PostScreenMessage(Fmt("#autoLOC_300854", {dotnet::FormatSingleG(dataAmount), subject.title, text}));
    GameEvents::OnScienceRecieved.Fire(scienceValue, &subject, source, reverseEngineered);
    return scienceValue;
}

// ResearchAndDevelopment.cs:727
std::vector<std::shared_ptr<ScienceSubject>> ResearchAndDevelopment::GetSubjects() {
    std::vector<std::shared_ptr<ScienceSubject>> list;
    if (!Instance) return list;  // C# returns null
    for (const auto& kv : Instance->scienceSubjects) list.push_back(kv.second);
    return list;
}

// ResearchAndDevelopment.cs:736
std::vector<std::string> ResearchAndDevelopment::GetExperimentIDs() {
    if (!experiments.has_value()) {
        experiments.emplace();
        loadExperiments();
    }
    std::vector<std::string> keys;
    for (const auto& kv : *experiments) keys.push_back(kv.first);
    return keys;
}

// ResearchAndDevelopment.cs:746
std::vector<std::string> ResearchAndDevelopment::GetSituationTagsDescriptions() {
    std::vector<std::string> list;
    for (const std::string& n : dotnet::EnumGetNames<ExperimentSituations>()) {
        std::optional<std::string> d = dotnet::EnumMemberDescription<ExperimentSituations>(n);
        list.push_back(d ? *d : n);
    }
    return list;
}

// ResearchAndDevelopment.cs:758
std::vector<std::string> ResearchAndDevelopment::GetSituationTags() { return dotnet::EnumGetNames<ExperimentSituations>(); }

// ResearchAndDevelopment.cs:763
std::vector<std::string> ResearchAndDevelopment::GetBiomeTagsLocalized(const CelestialBody& cb, bool includeMiniBiomes) {
    std::vector<std::string> list;
    if (cb.BiomeMap.has_value() && !cb.BiomeMap->Attributes.empty()) {
        for (const auto& a : cb.BiomeMap->Attributes) list.push_back(a.displayname);
        if (includeMiniBiomes) {
            std::vector<std::string> mini = GetMiniBiomeTagsLocalized(cb);
            list.insert(list.end(), mini.begin(), mini.end());
        }
        return list;
    }
    return list;
}

// ResearchAndDevelopment.cs:782
std::vector<std::string> ResearchAndDevelopment::GetMiniBiomeTagsLocalized(const CelestialBody& cb) {
    std::vector<std::string> list;
    for (const MiniBiome& mb : cb.MiniBiomes) {
        const std::string getDisplayName = mb.LocalizedTag.empty() ? std::string() : Loc::Format(mb.LocalizedTag);
        if (std::find(list.begin(), list.end(), getDisplayName) == list.end()) list.push_back(getDisplayName);
    }
    return list;
}

// ResearchAndDevelopment.cs:810
std::vector<std::string> ResearchAndDevelopment::GetBiomeTags(const CelestialBody& cb, bool includeMiniBiomes) {
    std::vector<std::string> list;
    if (cb.BiomeMap.has_value() && !cb.BiomeMap->Attributes.empty()) {
        for (const auto& a : cb.BiomeMap->Attributes) list.push_back(dotnet::Replace(a.name, " ", ""));
        if (includeMiniBiomes) {
            std::vector<std::string> mini = GetMiniBiomeTags(cb);
            list.insert(list.end(), mini.begin(), mini.end());
        }
        return list;
    }
    return list;
}

// ResearchAndDevelopment.cs:829
std::vector<std::string> ResearchAndDevelopment::GetMiniBiomeTags(const CelestialBody& cb) {
    std::vector<std::string> list;
    for (const MiniBiome& mb : cb.MiniBiomes) {
        if (std::find(list.begin(), list.end(), mb.TagKeyID) == list.end()) list.push_back(mb.TagKeyID);
    }
    return list;
}

// ResearchAndDevelopment.cs:857
std::string ResearchAndDevelopment::GetMiniBiomedisplayNameByScienceID(const std::string& TagID, bool formatted) {
    if (!minibiomesbyScienceID.has_value()) loadMiniBiomes();
    auto it = minibiomesbyScienceID->find(TagID);
    if (it != minibiomesbyScienceID->end()) {
        if (formatted) return it->second->LocalizedTag.empty() ? std::string() : Loc::Format(it->second->LocalizedTag);
        return it->second->LocalizedTag;
    }
    return TagID;
}

// ResearchAndDevelopment.cs:874
std::string ResearchAndDevelopment::GetMiniBiomedisplayNameByUnityTag(const std::string& TagID, bool formatted) {
    if (!minibiomesbyUnityTag.has_value()) loadMiniBiomes();
    const std::string text = Vessel::GetLandedAtString(TagID);  // MiniBiome.ConvertTagtoLandedAt
    auto show = [&](const MiniBiome* mb) { return formatted ? (mb->LocalizedTag.empty() ? std::string() : Loc::Format(mb->LocalizedTag)) : mb->LocalizedTag; };
    auto it = minibiomesbyUnityTag->find(TagID);
    if (it != minibiomesbyUnityTag->end()) return show(it->second);
    if (!text.empty()) {
        auto it2 = minibiomesbyUnityTag->find(text);
        if (it2 != minibiomesbyUnityTag->end()) return show(it2->second);
    }
    return text;
}

// ResearchAndDevelopment.cs:900
std::string ResearchAndDevelopment::GetResults(const std::string& subjectID) {
    std::vector<std::string> array;
    {
        size_t start = 0;
        for (size_t i = 0; i <= subjectID.size(); i++) {
            if (i == subjectID.size() || subjectID[i] == '@') {
                array.push_back(subjectID.substr(start, i - start));
                start = i + 1;
            }
        }
    }
    const std::string experimentID = array[0];
    if (array.size() < 2) throw std::out_of_range("ResearchAndDevelopment.GetResults: subject id without '@' (IndexOutOfRangeException)");
    const std::string text = array[1];
    ScienceExperiment* experiment = GetExperiment(experimentID);
    if (experiment != nullptr) {
        if (std::string* r = const_cast<dotnet::Dictionary<std::string, std::string>&>(experiment->Results()).Find(text)) return *r;
        std::vector<std::string> list;
        for (const auto& kv : experiment->Results()) {
            std::string current = kv.first;
            while (!current.empty() && current.back() == '*') current.pop_back();  // TrimEnd('*')
            if (text.find(current) != std::string::npos) list.push_back(kv.first);
        }
        if (!list.empty()) {
            const int hi = static_cast<int>(list.size());
            const int idx = ScienceEnvironment::unityRandomRange ? ScienceEnvironment::unityRandomRange(0, hi) : UnityRandom::Global().Range(0, hi);
            return experiment->Results().At(list[static_cast<size_t>(idx)]);
        }
        if (experiment->Results().ContainsKey("default")) return experiment->Results().At("default");
        return experiment->experimentTitle + Loc::Format("#autoLOC_6002268");
    }
    return std::string();
}

// ResearchAndDevelopment.cs:936
std::string ResearchAndDevelopment::CheckForMissingParts() {
    std::vector<const AvailablePart*> list = LoadedParts();
    if (list.empty() && !ScienceEnvironment::loadedParts) return Loc::Format("#autoLOC_300989");
    std::string text;
    bool flag = true;
    for (const AvailablePart* availablePart : list) {
        if (availablePart->TechRequired.empty()) {
            text += Fmt("#autoLOC_301005", {availablePart->name});
            flag = false;
        }
    }
    if (flag) text = Loc::Format("#autoLOC_301012");
    return text;
}

// ResearchAndDevelopment.cs:962
std::string ResearchAndDevelopment::PartAssignmentSummary() {
    std::vector<const AvailablePart*> list2 = LoadedParts();
    if (list2.empty() && !ScienceEnvironment::loadedParts) return Loc::Format("#autoLOC_301030");
    std::vector<std::string> list;
    dotnet::Dictionary<std::string, std::string> dictionary;
    for (const AvailablePart* availablePart : list2) {
        std::string text = availablePart->TechRequired.empty() ? Loc::Format("#autoLOC_301043") : availablePart->TechRequired;
        if (std::find(list.begin(), list.end(), text) == list.end()) {
            list.push_back(text);
            dictionary.Add(text, availablePart->name);
        } else {
            dictionary.Set(text, dictionary.At(text) + ", " + availablePart->name);
        }
    }
    std::string text2;
    for (const std::string& text3 : list) {
        const std::string& v = dictionary.At(text3);
        int num = 1;
        for (char c : v)
            if (c == ',') num++;
        text2 = text2 + text3 + " (" + std::to_string(num) + "): " + v + "\n\n";
    }
    return Loc::Format("#autoLOC_301069") + Fmt("#autoLOC_301070", {std::to_string(list.size())}) + Fmt("#autoLOC_301071", {text2});
}

// ResearchAndDevelopment.cs:999
void ResearchAndDevelopment::CheatAddScience(float sci) { AddScience(sci, science_reason::Cheating()); }

// ResearchAndDevelopment.cs:1004
void ResearchAndDevelopment::UnlockProtoTechNode(const ProtoTechNode& node) {
    bool bypass = CurrentParameters() ? CurrentParameters()->Difficulty.BypassEntryPurchaseAfterResearch : false;
    const bool fundingExists = ScienceEnvironment::fundingInstanceExists && ScienceEnvironment::fundingInstanceExists();
    const bool num = !fundingExists || bypass;
    std::vector<const AvailablePart*> list;
    if (num) {
        for (const AvailablePart* availablePart : LoadedParts())
            if (availablePart->TechRequired == node.techID) list.push_back(availablePart);
    }
    auto protoTechNode = std::make_shared<ProtoTechNode>();
    protoTechNode->techID = node.techID;
    protoTechNode->state = RDTech_State::Available;
    protoTechNode->scienceCost = node.scienceCost;
    protoTechNode->partsPurchased = std::move(list);
    protoTechNodes.Set(protoTechNode->techID, protoTechNode);
}

// ResearchAndDevelopment.cs:1047
void ResearchAndDevelopment::CheatTechnology() {
    std::vector<std::shared_ptr<ProtoTechNode>> treeTechs = RDTechTree::GetTreeTechs();
    int num = static_cast<int>(treeTechs.size());
    while (num-- > 0) UnlockProtoTechNode(*treeTechs[static_cast<size_t>(num)]);
    // RefreshTechTreeUI (RDTechTree MonoBehaviours) and ContractDefs.Instance.TechResearched: UI / contract seams
}

// ResearchAndDevelopment.cs:1059
std::string ResearchAndDevelopment::CountUniversalScience() {
    float num = 0.0f, num2 = 0.0f, num3 = 0.0f;
    int num4 = 0, num5 = 0, num6 = 0, num7 = 0;
    if (Instance != nullptr) {
        if (!experiments.has_value()) {
            experiments.emplace();
            loadExperiments();
        }
        num4 = experiments->Count();
        const ExperimentSituations array[] = {ExperimentSituations::SrfLanded,   ExperimentSituations::SrfSplashed, ExperimentSituations::FlyingLow,
                                              ExperimentSituations::FlyingHigh,  ExperimentSituations::InSpaceLow,  ExperimentSituations::InSpaceHigh};
        const std::vector<CelestialBody*> none;
        const std::vector<CelestialBody*>& bodies = FlightGlobals::bodies ? *FlightGlobals::bodies : none;
        for (const auto& kv : *experiments) {
            const ScienceExperiment& current = kv.second;
            for (CelestialBody* celestialBody : bodies) {
                for (ExperimentSituations experimentSituations : array) {
                    float num9 = 0.0f;
                    if ((static_cast<uint64_t>(current.situationMask) & static_cast<uint64_t>(experimentSituations)) != 0) {
                        switch (experimentSituations) {
                        case ExperimentSituations::FlyingHigh:
                            num9 = f32store(fx(num9) + fx(current.scienceCap) * fx(celestialBody->scienceValues.FlyingHighDataValue));
                            break;
                        case ExperimentSituations::SrfLanded:
                            num9 = f32store(fx(num9) + fx(current.scienceCap) * fx(celestialBody->scienceValues.LandedDataValue));
                            break;
                        case ExperimentSituations::SrfSplashed:
                            num9 = f32store(fx(num9) + fx(current.scienceCap) * fx(celestialBody->scienceValues.SplashedDataValue));
                            break;
                        case ExperimentSituations::FlyingLow:
                            num9 = f32store(fx(num9) + fx(current.scienceCap) * fx(celestialBody->scienceValues.FlyingLowDataValue));
                            break;
                        case ExperimentSituations::InSpaceHigh:
                            num9 = f32store(fx(num9) + fx(current.scienceCap) * fx(celestialBody->scienceValues.InSpaceHighDataValue));
                            break;
                        case ExperimentSituations::InSpaceLow:
                            num9 = f32store(fx(num9) + fx(current.scienceCap) * fx(celestialBody->scienceValues.InSpaceLowDataValue));
                            break;
                        }
                        num5++;
                        if ((static_cast<uint64_t>(current.biomeMask) & static_cast<uint64_t>(experimentSituations)) != 0) {
                            num6++;
                            if (celestialBody->BiomeMap.has_value())
                                num9 = f32store(fx(num9) * fx(static_cast<float>(celestialBody->BiomeMap->Attributes.size())));
                        }
                    }
                    num = f32store(fx(num) + fx(num9));
                }
            }
        }
        num2 = num;
        for (CelestialBody* celestialBody2 : bodies) {
            if (celestialBody2->BiomeMap.has_value()) num7 += static_cast<int>(celestialBody2->BiomeMap->Attributes.size());
            if (celestialBody2->isHomeWorld) {
                const float array2[3] = {5.0f, 8.0f, 10.0f};
                for (float f : array2) num3 = f32store(fx(num3) + fx(celestialBody2->scienceValues.RecoveryValue) * fx(f) * fx(1.2f));
                continue;
            }
            const float array3[5] = {6.0f, 8.0f, 10.0f, 12.0f, 15.0f};
            for (float f : array3) num3 = f32store(fx(num3) + fx(celestialBody2->scienceValues.RecoveryValue) * fx(f) * fx(1.2f));
        }
        num = f32store(fx(num) + fx(num3));
    } else {
        num = 0.0f;
    }
    using dotnet::FormatSingleF;
    return Loc::Format("#autoLOC_301288") + "\n" + Fmt("#autoLOC_301289", {FormatSingleF(num, 1)}) + "\n" + Fmt("#autoLOC_301290", {FormatSingleF(num2, 1)}) +
           "\n" + Fmt("#autoLOC_301291", {FormatSingleF(num3, 1)}) + "\n" + Fmt("#autoLOC_301292", {std::to_string(num4)}) + "\n" +
           Fmt("#autoLOC_301293", {std::to_string(num5)}) + "\n" + Fmt("#autoLOC_301294", {std::to_string(num6)}) + "\n" +
           Fmt("#autoLOC_301295", {std::to_string(num7)});
}

// ResearchAndDevelopment.cs:1162
std::string ResearchAndDevelopment::ScienceTransmissionRewardString(float amount, TransactionReasons reason) {
    // StringUtilities.FormattedCurrencies(0, amount, 0, ...) + ThisThisAndThat: WP-36 (formatScienceReward); no formatted currency = ""
    std::string list = ScienceEnvironment::formatScienceReward ? ScienceEnvironment::formatScienceReward(amount, reason) : std::string();
    if (!list.empty()) return " " + Fmt("#autoLOC_301303", {list});
    return std::string();
}

// ResearchAndDevelopment.cs:1177
bool ResearchAndDevelopment::ResearchedValidContractObjectives(std::vector<std::string> objectiveTypes, bool copy) {
    // IsValidContractObjective is a PartModule virtual of the contract package (WP-36 area); no module here overrides it
    (void)copy;
    return false;
}

}  // namespace ksp
