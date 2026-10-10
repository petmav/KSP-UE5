// WP-39: ModuleDataTransmitter.cs (see ksp/modules/module_data_transmitter.hpp for the scope and the seams).
#include "ksp/modules/module_data_transmitter.hpp"

#include <algorithm>
#include <cctype>

#include "ksp/commnet/comm_net_vessel.hpp"
#include "ksp/commnet/commnet_scene.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/util/misc_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::CommNet {

bool ProtoValueFlag(ProtoPartModuleSnapshot* mSnap, const char* valueName);

// ScienceData.cs:46
void TransmissionItem::Load(const ConfigNode& node) {
    node.TryGetValue("data", dataAmount);
    scienceValueRatio = 1.0f;
    node.TryGetValue("scienceValueRatio", scienceValueRatio);
    node.TryGetValue("subjectID", subjectID);
    node.TryGetValue("xmit", baseTransmitValue);
    node.TryGetValue("xmitBonus", transmitBonus);
    std::string t;
    if (node.GetValue("triggered").has_value()) {
        bool b = false;
        if (node.TryGetValue("triggered", b)) triggered = b;
    }
    node.TryGetValue("container", container);
}

// ScienceData.cs:76
void TransmissionItem::Save(ConfigNode& node) const {
    node.AddValue("data", dotnet::FormatSingleR(dataAmount));
    node.AddValue("scienceValueRatio", dotnet::FormatSingleR(scienceValueRatio));
    node.AddValue("subjectID", subjectID);
    node.AddValue("xmit", dotnet::FormatSingleR(baseTransmitValue));
    node.AddValue("xmitBonus", dotnet::FormatSingleR(transmitBonus));
    node.AddValue("triggered", triggered);
    node.AddValue("container", dotnet::FormatUInt64(container));
}

}  // namespace ksp::CommNet

namespace ksp {

using CommNet::TransmissionItem;

void TransmitterEnvironment::Reset() {
    scheduler = nullptr;
    screenMessages.clear();
    collectVesselData = nullptr;
    returnData = nullptr;
    createCommsStream = nullptr;
    onTriggeredDataTransmission = nullptr;
    applyUpgrades = nullptr;
}

namespace {
const PartResourceDefinition* LookupDefinition(const std::string& name) {
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    return lib ? lib->GetDefinition(std::string_view(name)) : nullptr;
}
void Post(const std::string& message) { TransmitterEnvironment::screenMessages.push_back(message); }  // ScreenMessages.PostScreenMessage

// string.Format of "0.0###"
std::string FormatZeroPointHash(double v) {
    std::string s = dotnet::FormatDoubleF(v, 4);
    while (s.size() > 2 && s.back() == '0' && s[s.size() - 2] != '.') s.pop_back();
    return s;
}

// CultureInfo.CurrentCulture.TextInfo.ToTitleCase (invariant English): a word that is entirely upper case is kept
std::string ToTitleCase(const std::string& s) {
    std::string r = s;
    size_t i = 0;
    while (i < r.size()) {
        while (i < r.size() && !std::isalpha(static_cast<unsigned char>(r[i]))) i++;
        size_t j = i;
        while (j < r.size() && std::isalpha(static_cast<unsigned char>(r[j]))) j++;
        if (j > i) {
            bool allUpper = j - i > 1;
            for (size_t k = i; k < j; k++) allUpper = allUpper && std::isupper(static_cast<unsigned char>(r[k]));
            if (!allUpper) {
                r[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(r[i])));
                for (size_t k = i + 1; k < j; k++) r[k] = static_cast<char>(std::tolower(static_cast<unsigned char>(r[k])));
            }
        }
        i = j;
    }
    return r;
}
Part* AsPart(PartPrefab* p) { return dynamic_cast<Part*>(p); }
}  // namespace

Vessel* ModuleDataTransmitter::VesselPtr() const {
    Part* p = AsPart(part);
    return p ? p->vessel : nullptr;
}

std::string ModuleDataTransmitter::PartTitle() const {
    Part* p = AsPart(part);
    if (p == nullptr || p->partInfo == nullptr) throw NullReferenceException();
    return p->partInfo->title;
}

ModuleDataTransmitter::~ModuleDataTransmitter() {
    if (TransmitterEnvironment::scheduler != nullptr) TransmitterEnvironment::scheduler->StopAllCoroutines(CoroutineOwner());
}

std::shared_ptr<Coroutine> ModuleDataTransmitter::StartCoroutine(Enumerator e) {
    if (TransmitterEnvironment::scheduler == nullptr) {
        log::Error("[TransmitterModule]: no coroutine scheduler (TransmitterEnvironment::scheduler)");
        return nullptr;
    }
    return TransmitterEnvironment::scheduler->StartCoroutine(std::move(e), CoroutineOwner());
}

AntennaType ModuleDataTransmitter::antennaType() const {
    const ModuleValue* v = Value("antennaType");
    const EnumValue* e = v ? std::get_if<EnumValue>(v) : nullptr;
    return e ? static_cast<AntennaType>(e->value) : AntennaType::DIRECT;
}

// ModuleDataTransmitter.cs:86
float ModuleDataTransmitter::DataRate() const { return f32store(fx(packetSize()) * (1.0 / fx(packetInterval()))); }

// ModuleDataTransmitter.cs:88
double ModuleDataTransmitter::DataResourceCost() const { return packetResourceCost() / (double)packetSize(); }

// ModuleDataTransmitter.cs:96
const DoubleCurve* ModuleDataTransmitter::CommRangeCurve() const {
    const ModuleValue* v = Value("rangeCurve");
    return v ? std::get_if<DoubleCurve>(v) : nullptr;
}

// ModuleDataTransmitter.cs:98
const DoubleCurve* ModuleDataTransmitter::CommScienceCurve() const {
    const ModuleValue* v = Value("scienceCurve");
    return v ? std::get_if<DoubleCurve>(v) : nullptr;
}

// ModuleDataTransmitter.cs:100
double ModuleDataTransmitter::CommPower() { return ApplyPowerAdjustments(antennaPower(), adjusterCache_); }

// Unity Instantiate: the public int[] members
void ModuleDataTransmitter::CopySerializedFrom(const PartModule& from) {
    PartModule::CopySerializedFrom(from);
    if (const auto* r = dynamic_cast<const ModuleDataTransmitter*>(&from)) {
        DeployFxModuleIndices = r->DeployFxModuleIndices;
        ProgressFxModuleIndices = r->ProgressFxModuleIndices;
    }
}

// ModuleDataTransmitter.cs:107
void ModuleDataTransmitter::OnAwake() {
    transmissionQueue.clear();
    DoubleCurve range;
    range.Add(0.0, 0.0, 0.0, 0.0);
    range.Add(1.0, 1.0, 0.0, 0.0);
    Set<DoubleCurve>("rangeCurve", std::move(range));
    DoubleCurve science;
    science.Add(0.0, 0.0, 0.0, 0.0);
    science.Add(0.44999998807907104, 0.07999999821186066, 0.4951019, 0.4951019);
    science.Add(0.8100000023841858, 0.3499999940395355, 0.8020515, 0.8020515);
    science.Add(1.0, 0.4000000059604645, 0.0, 0.0);
    Set<DoubleCurve>("scienceCurve", std::move(science));
    consumedResources_.clear();
    for (size_t i = 0, count = resHandler.inputResources.size(); i < count; i++)
        consumedResources_.push_back(LookupDefinition(resHandler.inputResources[i]->name));
    Events().Bind("StartTransmission", BaseEvent::Handler([this] { StartTransmission(); }));
    Events().Bind("TransmitIncompleteToggle", BaseEvent::Handler([this] { TransmitIncompleteToggle(); }));
    Events().Bind("StopTransmission", BaseEvent::Handler([this] { StopTransmission(); }));
    Actions().Bind("StartTransmissionAction", [this](KSPActionParam&) { StartTransmissionAction(); });
}

// ModuleDataTransmitter.cs:133
void ModuleDataTransmitter::OnLoad(const ConfigNode& node) {
    if (node.HasValue("DeployFxModules")) {
        DeployFxModuleIndices = KSPUtil::ParseArray<int>(*node.GetValue("DeployFxModules"), [](const std::string& t) {
            int32_t x = 0;
            if (!dotnet::TryParseInt32(t, x)) throw dotnet::FormatException("Input string was not in a correct format.");  // int.Parse
            return static_cast<int>(x);
        });
    }
    if (node.HasValue("ProgressFxModules")) {
        ProgressFxModuleIndices = KSPUtil::ParseArray<int>(*node.GetValue("ProgressFxModules"), [](const std::string& t) {
            int32_t x = 0;
            if (!dotnet::TryParseInt32(t, x)) throw dotnet::FormatException("Input string was not in a correct format.");
            return static_cast<int>(x);
        });
    }
    const std::vector<ConfigNode*> nodes = node.GetNodes("CommsData");
    for (size_t i = 0; i < nodes.size(); i++) {
        TransmissionItem item;
        item.Load(*nodes[i]);
        transmissionQueue.push_back(item);
    }
    Events()["TransmitIncompleteToggle"]->SetGuiName(xmitIncomplete() ? "#autoLOC_7001224" : "#autoLOC_7001225");
    UpdatePowerText();
    Part* p = AsPart(part);
    if (resHandler.inputResources.empty() && (p == nullptr || p->partInfo == nullptr)) {
        std::string value = "ElectricCharge";
        node.TryGetValue("requiredResource", value);
        auto moduleResource = std::make_shared<ModuleResource>();
        moduleResource->name = value;
        moduleResource->title = KSPUtil::PrintModuleName(value);
        moduleResource->id = StringExtensions::GetHashCode_Net35(value);
        moduleResource->rate = 1.0;
        resHandler.inputResources.push_back(moduleResource);
    }
}

// ModuleDataTransmitter.cs:165
void ModuleDataTransmitter::UpdatePowerText() {
    Set<std::string>("powerText", KSPUtil::PrintSI(CommPower(), "") + (antennaCombinable() ? (" " + Localizer::Format("#autoLOC_236248")) : std::string()));
}

// ModuleDataTransmitter.cs:170
void ModuleDataTransmitter::OnSave(ConfigNode& node) {
    for (size_t i = 0, count = transmissionQueue.size(); i < count; i++) transmissionQueue[i].Save(*node.AddNode("CommsData"));
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) node.AddValue("canComm", CanComm());
}

// ModuleDataTransmitter.cs:183
void ModuleDataTransmitter::OnStart(StartState) {
    deployFxModules = findFxModules(DeployFxModuleIndices, true);
    progressFxModules = findFxModules(ProgressFxModuleIndices, false);
    statusMessage.clear();
    progressMessage.clear();
    errorMessage.clear();
    const bool enabled = moduleIsEnabled();
    if (transmissionQueue.empty()) {
        Actions()["StartTransmissionAction"]->active = enabled && antennaType() != AntennaType::INTERNAL;
        Events()["StartTransmission"]->active = enabled && antennaType() != AntennaType::INTERNAL;
        Events()["StopTransmission"]->active = false;
    } else {
        StartCoroutine(transmitQueuedData(packetInterval(), packetSize()));
    }
    const bool commNet = CommNet::CommNetScenario::CommNetEnabled();
    Fields()["statusText"]->guiActive = commNet && enabled;
    BaseField* powerText = Fields()["powerText"];
    powerText->guiActive = powerText->guiActiveEditor = commNet && enabled;
    Events()["TransmitIncompleteToggle"]->active = enabled && antennaType() != AntennaType::INTERNAL;
}

// ModuleDataTransmitter.cs:207
float ModuleDataTransmitter::GetVesselSignalStrength() {
    if (!CommNet::CommNetScenario::CommNetEnabled()) return 1.0f;
    Vessel* v = VesselPtr();
    if (!(v != nullptr)) return 1.0f;
    if (v->connection == nullptr) throw NullReferenceException();
    return (float)v->connection->ControlPath().signalStrength();
}

// ModuleDataTransmitter.cs:220
double ModuleDataTransmitter::EvaluateScienceMultiplier(double signalStrength) {
    const DoubleCurve* c = CommScienceCurve();
    if (c == nullptr) return 1.0;
    return c->Evaluate(signalStrength);
}

// ModuleDataTransmitter.cs:229
std::optional<std::vector<IScalarModule*>> ModuleDataTransmitter::findFxModules(const std::optional<std::vector<int>>& indices, bool showUI) {
    if (indices.has_value()) {
        std::vector<IScalarModule*> list;
        Part* p = AsPart(part);
        if (p == nullptr) throw NullReferenceException();
        for (size_t i = 0, num = indices->size(); i < num; i++) {
            const int num2 = (*indices)[i];
            PartModule* m = p->Modules().GetModule(static_cast<size_t>(num2));  // List indexer: ArgumentOutOfRange when absent
            if (IScalarModule* sm = m->AsScalarModule()) {
                sm->SetUIWrite(showUI);
                sm->SetUIRead(showUI);
                list.push_back(sm);
            } else {
                log::Error("[TransmitterModule]: Part Module " + std::to_string(num2) + " doesn't implement IScalarModule");
            }
        }
        return list;
    }
    return std::nullopt;
}

// ModuleDataTransmitter.cs:256
void ModuleDataTransmitter::StartTransmissionAction() { StartTransmission(); }

// ModuleDataTransmitter.cs:262
void ModuleDataTransmitter::StartTransmission() {
    if (!CanSetFXModules(deployFxModules)) {
        errorMessage = Localizer::Format("#autoLOC_236367", {PartTitle()});
        Post(errorMessage);
        return;
    }
    if (!CanTransmit()) {
        errorMessage = Localizer::Format("#autoLOC_236374", {PartTitle()});
        Post(errorMessage);
        return;
    }
    Vessel* v = VesselPtr();
    if (CommNet::CommNetScenario::CommNetEnabled() && v != nullptr && v->connection != nullptr) {
        CommNet::CommNetVessel* conn = v->connection;
        std::shared_ptr<CommNet::Network::Link> first = conn->ControlPath().First();
        if (!first) throw NullReferenceException();
        double cost = first->start()->At(first->end())->cost;
        cost *= cost;
        const double power = static_cast<CommNet::CommNode*>(first->end())->antennaRelay.power;
        const bool combined = ((conn->Comm()->antennaRelay.power > conn->Comm()->antennaTransmit.power) ? conn->Comm()->antennaRelay.combined
                                                                                                          : conn->Comm()->antennaTransmit.combined);
        if (!CanScienceTo(combined, power, cost)) {
            errorMessage = Localizer::Format("#autoLOC_236387", {PartTitle()});
            Post(errorMessage);
            return;
        }
    }
    // PartItemTransfer.DismissActive(); FindPartModulesImplementing<IScienceDataContainer>, the choice dialog and queueVesselData (:336): WP-35 / UI
    std::optional<std::vector<TransmissionItem>> queue;
    if (TransmitterEnvironment::collectVesselData) queue = TransmitterEnvironment::collectVesselData(*this);
    else queue = std::vector<TransmissionItem>();
    if (queue.has_value()) onStartTransmission(std::move(*queue));
}

// ModuleDataTransmitter.cs:330
void ModuleDataTransmitter::onStartTransmission(std::vector<TransmissionItem> queue) {
    transmissionQueue = std::move(queue);
    StartCoroutine(transmitQueuedData(packetInterval(), packetSize()));
}

// ModuleDataTransmitter.cs:358
Enumerator ModuleDataTransmitter::transmitQueuedData(float transmitInterval, float dataPacketSize, Callback<>, bool sendData) {
    if (!CanSetFXModules(deployFxModules)) {
        errorMessage = Localizer::Format("#autoLOC_236475", {PartTitle()});
        Post(errorMessage);
        co_return;
    }
    Actions()["StartTransmissionAction"]->active = false;
    Events()["StartTransmission"]->active = false;
    Events()["StopTransmission"]->active = true;
    xmitOnHold_ = false;
    busy_ = true;
    statusMessage = "[" + PartTitle() + "]: " + Localizer::Format("#autoLOC_7003222");
    Post(statusMessage);
    SetFXModulesUI(deployFxModules, true, false);
    deployFxModuleStartPosition_ = GetFxModuleScalar(deployFxModules);
    co_yield YieldInstruction::Wait(StartCoroutine(SetFXModules(deployFxModules, 1.0f)));
    SetFXModulesUI(deployFxModules, false, false);
    while (!transmissionQueue.empty()) {
        TransmissionItem data = transmissionQueue[0];
        transmissionQueue.erase(transmissionQueue.begin());
        float totalData = data.dataAmount;
        int packetsToSend = Mathf::CeilToInt(f32store(fx(totalData) / fx(dataPacketSize)));
        const int totalPackets = packetsToSend;
        if (TransmitterEnvironment::createCommsStream && sendData && !data.triggered) {
            commStream_ = TransmitterEnvironment::createCommsStream(data, totalData, f32store(fx(transmitInterval) + 0.5), xmitIncomplete());
        }
        StartCoroutine(SetFXModules(progressFxModules, 0.0f));
        capacitorCharge_ = 0.0;
        while (packetsToSend > 0) {
            const double num = packetResourceCost() - capacitorCharge_;
            capacitorCharge_ += num * resHandler.UpdateModuleResourceInputs(errorStr, num / (double)TimeWarp::fixedDeltaTime(), 0.999, false, false, true);
            if (capacitorCharge_ >= packetResourceCost() * 0.95) {
                capacitorCharge_ = 0.0;
                const float dataAmount = Mathf::Min(packetSize(), totalData);
                totalData = f32store(fx(totalData) - fx(packetSize()));
                xmitOnHold_ = false;
                packetsToSend--;
                Set<std::string>("statusText", Localizer::Format("#autoLOC_236534"));
                StartCoroutine(SetFXModules(progressFxModules, f32store((fx(data.dataAmount) - fx(totalData)) / fx(data.dataAmount))));
                if (packetsToSend % static_cast<int>(TimeWarp::CurrentRate()) == 0) {
                    progressMessage = Localizer::Format("#autoLOC_236540", {PartTitle()}) + " " +
                                      dotnet::FormatSingleF(f32store((1.0 - fx((float)packetsToSend) / fx((float)totalPackets)) * 100.0), 2) + "%";
                    Post(progressMessage);
                }
                if (commStream_) commStream_->StreamData(dataAmount, VesselPtr() ? VesselPtr()->protoVessel : nullptr);
            } else {
                if (!xmitOnHold_) xmitOnHold_ = true;
                errorMessage = Localizer::Format("#autoLOC_236559", {PartTitle(), errorStr});
                Post(errorMessage);
                Set<std::string>("statusText", Localizer::Format("#autoLOC_236562", {dotnet::FormatDoubleF(capacitorCharge_, 1), dotnet::FormatDoubleF(packetResourceCost(), 1)}));
                if (!xmitIncomplete()) AbortTransmission(Localizer::Format("#autoLOC_236564"));
            }
            co_yield YieldInstruction::Wait(StartCoroutine(WaitForFixedSeconds(transmitInterval)));
            if (xmitAborted_) {
                ReturnDataToContainer(data);
                break;
            }
        }
        if (data.triggered && TransmitterEnvironment::onTriggeredDataTransmission) TransmitterEnvironment::onTriggeredDataTransmission(data, VesselPtr(), xmitAborted_);
        if (!transmissionQueue.empty()) co_yield YieldInstruction::WaitForSeconds(f32store(fx(transmitInterval) * 2.0));
        if (xmitAborted_) break;
    }
    if (!xmitAborted_) {
        Set<std::string>("statusText", Localizer::Format("#autoLOC_236592"));
        statusMessage = Localizer::Format("#autoLOC_236594", {PartTitle()});
        Post(statusMessage);
        co_yield YieldInstruction::WaitForSeconds(3.0f);
    }
    commStream_.reset();
    SetFXModulesUI(deployFxModules, true, false);
    StartCoroutine(SetFXModules(progressFxModules, 0.0f));
    co_yield YieldInstruction::Wait(StartCoroutine(SetFXModules(deployFxModules, deployFxModuleStartPosition_)));
    SetFXModulesUI(deployFxModules, true, true);
    busy_ = false;
    Set<std::string>("statusText", Localizer::Format("#autoLOC_236147"));
    xmitAborted_ = false;
    Actions()["StartTransmissionAction"]->active = true;
    Events()["StartTransmission"]->active = true;
    Events()["StopTransmission"]->active = false;
    if (!transmissionQueue.empty()) StartCoroutine(transmitQueuedData(packetInterval(), packetSize()));
}

// ModuleDataTransmitter.cs:472
Enumerator ModuleDataTransmitter::WaitForFixedSeconds(float seconds) {
    for (float curTime = 0.0f; curTime < seconds; curTime = f32store(fx(curTime) + fx(TimeWarp::fixedDeltaTime()))) co_yield YieldInstruction::WaitForFixedUpdate();
}

// ModuleDataTransmitter.cs:480
bool ModuleDataTransmitter::CanSetFXModules(const std::optional<std::vector<IScalarModule*>>& modules) {
    if (!modules.has_value()) throw NullReferenceException();
    int count = static_cast<int>(modules->size());
    do {
        if (count-- <= 0) return true;
    } while ((*modules)[static_cast<size_t>(count)]->CanMove());
    return false;
}

// ModuleDataTransmitter.cs:494
float ModuleDataTransmitter::GetFxModuleScalar(const std::optional<std::vector<IScalarModule*>>& modules) {
    if (modules.has_value() && !modules->empty()) {
        if ((*modules)[0]->GetScalar() < 0.5f) return 0.0f;
        return 1.0f;
    }
    return 0.0f;
}

// ModuleDataTransmitter.cs:507
float ModuleDataTransmitter::GetModulesScalarMax(const std::vector<IScalarModule*>& modules) {
    float num = 0.0f;
    int count = static_cast<int>(modules.size());
    while (count-- > 0) {
        const float getScalar = modules[static_cast<size_t>(count)]->GetScalar();
        if (getScalar > num) num = getScalar;
    }
    return num;
}

// ModuleDataTransmitter.cs:522
float ModuleDataTransmitter::GetModulesScalarMin(const std::vector<IScalarModule*>& modules) {
    float num = 1.0f;
    int count = static_cast<int>(modules.size());
    while (count-- > 0) {
        const float getScalar = modules[static_cast<size_t>(count)]->GetScalar();
        if (getScalar < num) num = getScalar;
    }
    return num;
}

// ModuleDataTransmitter.cs:537
Enumerator ModuleDataTransmitter::SetFXModules(std::optional<std::vector<IScalarModule*>> modules, float tgtValue) {
    if (!modules.has_value()) co_return;
    bool allFXDone = false;
    while (!allFXDone) {
        allFXDone = true;
        for (size_t i = 0, count = modules->size(); i < count; i++) {
            if ((*modules)[i]->CanMove() && !(Mathf::Abs(f32store(fx((*modules)[i]->GetScalar()) - fx(tgtValue))) < 0.01f)) {
                allFXDone = false;
                (*modules)[i]->SetScalar(tgtValue);
            }
        }
        if (!allFXDone) co_yield YieldInstruction::Null();
    }
}

// ModuleDataTransmitter.cs:563
void ModuleDataTransmitter::SetFXModulesUI(const std::optional<std::vector<IScalarModule*>>& fxModules, bool readState, bool writeState) {
    if (!fxModules.has_value()) throw NullReferenceException();
    for (size_t i = 0, count = fxModules->size(); i < count; i++) {
        (*fxModules)[i]->SetUIRead(readState);
        (*fxModules)[i]->SetUIWrite(writeState);
    }
}

// ModuleDataTransmitter.cs:574
void ModuleDataTransmitter::TransmitIncompleteToggle() {
    if (!xmitIncomplete()) {
        errorMessage = Localizer::Format("#autoLOC_236732", {PartTitle()});
        Post(errorMessage);
        Set<bool>("xmitIncomplete", true);
        Events()["TransmitIncompleteToggle"]->SetGuiName("#autoLOC_7001224");
    } else {
        Set<bool>("xmitIncomplete", false);
        Events()["TransmitIncompleteToggle"]->SetGuiName("#autoLOC_7001225");
    }
    if (commStream_) commStream_->xmitIncomplete = xmitIncomplete();
}

// ModuleDataTransmitter.cs:595
void ModuleDataTransmitter::StopTransmission() { AbortTransmission(Localizer::Format("#autoLOC_236748")); }

// ModuleDataTransmitter.cs:600
void ModuleDataTransmitter::AbortTransmission(const std::string& message) {
    statusMessage = "[" + PartTitle() + "]: " + message;
    Post(statusMessage);
    Set<std::string>("statusText", Localizer::Format("#autoLOC_236755"));
    xmitAborted_ = true;
    Events()["StopTransmission"]->active = false;
}

// ModuleDataTransmitter.cs:609
void ModuleDataTransmitter::ReturnDataToContainer(const TransmissionItem& data) {
    if (TransmitterEnvironment::returnData) TransmitterEnvironment::returnData(*this, data);
}

// ModuleDataTransmitter.cs:631
bool ModuleDataTransmitter::CanTransmit() {
    Vessel* v = VesselPtr();
    if (moduleIsEnabled() && antennaType() != AntennaType::INTERNAL &&
        (!CommNet::CommNetScenario::CommNetEnabled() || v == nullptr || v->connection == nullptr ||
         (v->connection->SignalStrengthValue() > 0.0 && v->connection->ControlPath().IsLastHopHome()))) {
        if (deployFxModules.has_value() && !deployFxModules->empty() && GetModulesScalarMin(*deployFxModules) != 1.0f) return CanSetFXModules(deployFxModules);
        return true;
    }
    return false;
}

// ModuleDataTransmitter.cs:649
void ModuleDataTransmitter::TransmitData(std::vector<TransmissionItem> dataQueue) {
    transmissionQueue.insert(transmissionQueue.end(), dataQueue.begin(), dataQueue.end());
    if (!busy_) StartCoroutine(transmitQueuedData(packetInterval(), packetSize()));
}

// ModuleDataTransmitter.cs:658
void ModuleDataTransmitter::TransmitData(std::vector<TransmissionItem> dataQueue, Callback<> callback) {
    transmissionQueue.insert(transmissionQueue.end(), dataQueue.begin(), dataQueue.end());
    if (!busy_) StartCoroutine(transmitQueuedData(packetInterval(), packetSize(), std::move(callback), false));
}

// ModuleDataTransmitter.cs:667
std::string ModuleDataTransmitter::GetInfo() {
    std::string text = ToTitleCase(Localizer::Format(antennaType() == AntennaType::INTERNAL ? "#autoLOC_6004049"
                                                      : antennaType() == AntennaType::DIRECT ? "#autoLOC_6004050"
                                                                                             : "#autoLOC_6004051"));
    std::string text2;
    const std::string unitName = Localizer::Format("#autoLOC_7001411");
    text2 += Localizer::Format("#autoLOC_7001005", {text});
    text2 += Localizer::Format("#autoLOC_7001006", {Get<std::string>("powerText")});
    CommNet::IRangeModel& range = CommNet::CommNetScenario::RangeModel();
    text2 = text2 + Localizer::Format("#autoLOC_236834") + " " + KSPUtil::PrintSI(range.GetMaximumRange(antennaPower(), CommNet::DSN::GetDSNRange(0.0f)), unitName);
    text2 = text2 + Localizer::Format("#autoLOC_236835") + " " + KSPUtil::PrintSI(range.GetMaximumRange(antennaPower(), CommNet::DSN::GetDSNRange(0.5f)), unitName);
    text2 = text2 + Localizer::Format("#autoLOC_236836") + " " + KSPUtil::PrintSI(range.GetMaximumRange(antennaPower(), CommNet::DSN::GetDSNRange(1.0f)), unitName);
    text2 += "\n";
    if (antennaType() != AntennaType::INTERNAL) {
        text2 += Localizer::Format("#autoLOC_236840", {dotnet::FormatSingleF(packetSize(), 1)});
        text2 += Localizer::Format("#autoLOC_236841", {FormatZeroPointHash(f32store(fx(packetSize()) / fx(packetInterval())))});
        text2 += Localizer::Format("#autoLOC_236842");
        // resHandler.PrintModuleResources(packetResourceCost / packetInterval): the PAW text of the resource list is not modelled (WP-15)
    } else {
        text2 += Localizer::Format("#autoLOC_236846");
    }
    if (!moduleIsEnabled()) text2 += Localizer::Format("#autoLOC_236849");
    return text2;
}

// ModuleDataTransmitter.cs:823
std::string ModuleDataTransmitter::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003034"); }

// ModuleDataTransmitter.cs:706
bool ModuleDataTransmitter::CanScienceTo(bool combined, double bPower, double sqrDistance) {
    if (!(antennaCombinable() && combined)) return CommNet::CommNetScenario::RangeModel().InRange(CommPower(), bPower, sqrDistance);
    return true;
}

// ModuleDataTransmitter.cs:715
bool ModuleDataTransmitter::CanComm() {
    if (!moduleIsEnabled() || deployFxModules.has_value()) {
        if (!deployFxModules.has_value()) throw NullReferenceException();
        if (!deployFxModules->empty() && GetModulesScalarMin(*deployFxModules) != 1.0f) return false;
    }
    return !IsDataTransmitterBroken(adjusterCache_);
}

// ModuleDataTransmitter.cs:724
bool ModuleDataTransmitter::CanCommUnloaded(ProtoPartModuleSnapshot* mSnap) {
    if (mSnap == nullptr) return true;
    std::vector<CommNet::IDataTransmitterAdjuster*> list;
    if (CommNet::CommNetModuleEnvironment::protoTransmitterAdjusters) list = CommNet::CommNetModuleEnvironment::protoTransmitterAdjusters(*mSnap);
    if (IsDataTransmitterBroken(list)) return false;
    return CommNet::ProtoValueFlag(mSnap, "canComm");
}

// ModuleDataTransmitter.cs:746
double ModuleDataTransmitter::CommPowerUnloaded(ProtoPartModuleSnapshot* mSnap) {
    double value = antennaPower();
    if (mSnap != nullptr) {
        if (ConfigNode* node = mSnap->moduleValues->GetNode("UPGRADESAPPLIED")) {
            ConfigNode configNode;
            if (TransmitterEnvironment::applyUpgrades) TransmitterEnvironment::applyUpgrades(*this, *mSnap, configNode);
            configNode.TryGetValue("antennaPower", value);
        }
    }
    if (mSnap != nullptr) {
        std::vector<CommNet::IDataTransmitterAdjuster*> list;
        if (CommNet::CommNetModuleEnvironment::protoTransmitterAdjusters) list = CommNet::CommNetModuleEnvironment::protoTransmitterAdjusters(*mSnap);
        value = ApplyPowerAdjustments(value, list);
    }
    return value;
}

// ModuleDataTransmitter.cs:773
void ModuleDataTransmitter::OnModuleAdjusterRemoved(CommNet::IDataTransmitterAdjuster* adjuster) {
    auto it = std::find(adjusterCache_.begin(), adjusterCache_.end(), adjuster);
    if (it != adjusterCache_.end()) adjusterCache_.erase(it);
}

// ModuleDataTransmitter.cs:780
double ModuleDataTransmitter::ApplyPowerAdjustments(double power, const std::vector<CommNet::IDataTransmitterAdjuster*>& adjusterList) {
    for (size_t i = 0; i < adjusterList.size(); i++) power = adjusterList[i]->ApplyPowerAdjustment(power);
    return power;
}

// ModuleDataTransmitter.cs:789
bool ModuleDataTransmitter::IsDataTransmitterBroken(const std::vector<CommNet::IDataTransmitterAdjuster*>& adjusterList) {
    size_t num = 0;
    while (true) {
        if (num < adjusterList.size()) {
            if (adjusterList[num]->IsDataTransmitterBroken()) break;
            num++;
            continue;
        }
        return false;
    }
    return true;
}

}  // namespace ksp
