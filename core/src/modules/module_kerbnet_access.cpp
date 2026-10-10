// WP-39: ModuleKerbNetAccess.cs (see ksp/modules/module_kerbnet_access.hpp).
#include "ksp/modules/module_kerbnet_access.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/animation_modules.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

void KerbNetEnvironment::Reset() {
    hasThisDialogOpen = nullptr;
    setFoVBounds = nullptr;
    refreshErrorState = nullptr;
    openDialog = nullptr;
    screenMessages.clear();
}

namespace {
Part* AsPart(PartPrefab* p) { return dynamic_cast<Part*>(p); }

std::string Trim(const std::string& s) {
    const char* ws = " \t\r\n\f\v";
    const size_t a = s.find_first_not_of(ws);
    if (a == std::string::npos) return std::string();
    const size_t b = s.find_last_not_of(ws);
    return s.substr(a, b - a + 1);
}

// string.Split(','): empty entries are kept
std::vector<std::string> SplitComma(const std::string& s) {
    std::vector<std::string> r;
    size_t start = 0;
    while (true) {
        const size_t i = s.find(',', start);
        if (i == std::string::npos) {
            r.push_back(s.substr(start));
            break;
        }
        r.push_back(s.substr(start, i - start));
        start = i + 1;
    }
    return r;
}

// GameDatabase.Instance.ExperienceConfigs.GetTraitsWithEffect (ExperienceSystemConfig.cs:90)
std::vector<std::string> GetTraitsWithEffect(const std::string& effect) {
    SaveEnvironment& env = SaveEnvironment::Current();
    std::shared_ptr<Experience::ExperienceSystemConfig> cfg = env.experienceConfigs ? env.experienceConfigs : Experience::ExperienceSystemConfig::StockBuiltIn();
    return cfg->GetTraitsWithEffect(effect);
}

// FinePrint/Utilities/StringUtilities.cs:226 ThisThisAndThat (connector / conjugation are localised again inside)
std::string ThisThisAndThat(const std::vector<std::string>& stringList, std::string connector, std::string conjugation = "#autoLOC_6002374") {
    if (stringList.empty()) return "(NO ELEMENTS)";
    if (stringList.size() == 1) return stringList[0];
    std::string sb;
    connector = Localizer::Format(connector);
    conjugation = Localizer::Format(conjugation);
    for (size_t i = 0; i + 2 < stringList.size(); i++) {
        sb += Localizer::Format(stringList[i]);
        sb += connector;
    }
    sb += Localizer::Format(stringList[stringList.size() - 2]);
    sb += conjugation;
    sb += Localizer::Format(stringList[stringList.size() - 1]);
    return sb;
}
}  // namespace

Vessel* ModuleKerbNetAccess::VesselPtr() const {
    Part* p = AsPart(part);
    return p ? p->vessel : nullptr;
}

// ModuleKerbNetAccess.cs:78
void ModuleKerbNetAccess::OnAwake() {
    // `if (modes == null) modes = new List<Modes>()`: a C++ vector is never null; an instance keeps the serialized lists
    Events().Bind("OpenKerbNet", BaseEvent::Handler([this] { OpenKerbNet(); }));
    Actions().Bind("OpenKerbNetAction", [this](KSPActionParam&) { OpenKerbNetAction(); });
}

// Unity Instantiate: modes, effects, isCheckingEffects, partHasEffects, partIsAnimating, isEnhanced are public (serialized)
void ModuleKerbNetAccess::CopySerializedFrom(const PartModule& from) {
    PartModule::CopySerializedFrom(from);
    if (const auto* r = dynamic_cast<const ModuleKerbNetAccess*>(&from)) {
        modes = r->modes;
        effects = r->effects;
        isCheckingEffects = r->isCheckingEffects;
        partHasEffects = r->partHasEffects;
        partIsAnimating = r->partIsAnimating;
        isEnhanced = r->isEnhanced;
    }
}

// ModuleKerbNetAccess.cs:90
void ModuleKerbNetAccess::OnLoad(const ConfigNode& node) {
    LoadDisplayModes(node);
    LoadRequiredEffects(node);
}

// ModuleKerbNetAccess.cs:96
void ModuleKerbNetAccess::LoadDisplayModes(const ConfigNode& node) {
    ConfigNode* node2 = node.GetNode("DISPLAY_MODES");
    if (node2 == nullptr) return;
    modes.clear();
    const std::vector<std::string> values = node2->GetValues("Mode");
    for (size_t i = 0; i < values.size(); i++) {
        if (!values[i].empty()) {
            const std::vector<std::string> array = SplitComma(values[i]);
            const std::string mode = Trim(array[0]);
            std::string displaymode = mode;
            if (array.size() == 2) displaymode = Localizer::Format(array[1]);
            if (std::find_if(modes.begin(), modes.end(), [&](const Modes& p) { return p.Mode == mode; }) == modes.end()) modes.emplace_back(mode, displaymode);
        }
    }
}

// ModuleKerbNetAccess.cs:126
void ModuleKerbNetAccess::LoadRequiredEffects(const ConfigNode& node) {
    ConfigNode* node2 = node.GetNode("REQUIRED_EFFECTS");
    if (node2 != nullptr) {
        effects.clear();
        const std::vector<std::string> values = node2->GetValues("Effect");
        for (size_t i = 0; i < values.size(); i++) {
            const std::string text = Trim(values[i]);
            if (!text.empty() && std::find(effects.begin(), effects.end(), text) == effects.end()) effects.push_back(text);
        }
    }
    isCheckingEffects = !effects.empty();
}

ModuleKerbNetAccess::~ModuleKerbNetAccess() {
    if (subscribed_) OnDestroy();
}

// ModuleKerbNetAccess.cs:146
void ModuleKerbNetAccess::OnStart(StartState) {
    if (subscribed_) OnDestroy();  // a second OnStart must not leave a second subscription
    Events()["OpenKerbNet"]->guiActive = ResourceFlowEnvironment::LoadedSceneIsFlight();
    GameEvents::onVesselSituationChange.Add(this, &ModuleKerbNetAccess::OnVesselSituationChanged);
    subscribed_ = true;
    Vessel* v = VesselPtr();
    const VesselSituations situation = (v != nullptr) ? v->situation : VesselSituations::PRELAUNCH;
    CheckVesselSituationFoV(situation, true);
    if (isCheckingEffects) {
        GameEvents::onVesselCrewWasModified.Add(this, &ModuleKerbNetAccess::InvokeCrewEffectCheck);
        CheckRequiredEffects();
    }
    if (RequiresAnimation()) GameEvents::OnAnimationGroupStateChanged.Add(this, &ModuleKerbNetAccess::OnAnimationGroupStateChanged);
}

// ModuleKerbNetAccess.cs:163
void ModuleKerbNetAccess::OnDestroy() {
    GameEvents::onVesselSituationChange.Remove(this, &ModuleKerbNetAccess::OnVesselSituationChanged);
    if (isCheckingEffects) {
        GameEvents::onVesselCrewWasModified.Remove(this, &ModuleKerbNetAccess::InvokeCrewEffectCheck);
        if (IsInvoking()) CancelInvoke();
    }
    if (RequiresAnimation()) GameEvents::OnAnimationGroupStateChanged.Remove(this, &ModuleKerbNetAccess::OnAnimationGroupStateChanged);
    subscribed_ = false;
}

// ModuleKerbNetAccess.cs:181
void ModuleKerbNetAccess::OpenKerbNetAction() {
    if (Events()["OpenKerbNet"]->guiActive) OpenKerbNet();
}

// ModuleKerbNetAccess.cs:190
void ModuleKerbNetAccess::OpenKerbNet() {
    if (KerbNetEnvironment::openDialog) KerbNetEnvironment::openDialog(*this);  // KerbNetDialog.Display(this); ResetMapPosition()
}

// ModuleKerbNetAccess.cs:201
std::string ModuleKerbNetAccess::GetModuleDisplayName() { return Localizer::Format("#autoLOC_230490"); }

// ModuleKerbNetAccess.cs:206
std::string ModuleKerbNetAccess::GetInfo() {
    const std::vector<VesselSituations> enhancedSituations = GetEnhancedSituations();
    const std::string deg = "\xC2\xB0";
    std::string sb;
    sb += Localizer::Format("#autoLOC_230499") + " ";
    sb += dotnet::FormatDoubleG(dotnet::Math::Round(fx(MinimumFoV()))) + deg + "\n";
    sb += Localizer::Format("#autoLOC_230500") + " ";
    sb += dotnet::FormatDoubleG(dotnet::Math::Round(fx(MaximumFoV()))) + deg + "\n";
    sb += Localizer::Format("#autoLOC_230501") + " ";
    sb += dotnet::FormatDoubleG(dotnet::Math::Round(fx(AnomalyDetection()) * 100.0)) + "%\n";
    if (!modes.empty()) {
        sb += Localizer::Format("#autoLOC_230505");
        for (size_t i = 0; i < modes.size(); i++) sb += "\n - " + modes[i].displayMode;
        sb += "\n";
    }
    if (!effects.empty()) {  // && GameDatabase.Instance != null && GameDatabase.Instance.ExperienceConfigs != null
        sb += Localizer::Format("#autoLOC_230517");
        for (size_t j = 0; j < effects.size(); j++) {
            const std::string value = ThisThisAndThat(GetTraitsWithEffect(effects[j]), Localizer::Format("#autoLOC_230521"));
            if (!value.empty()) sb += "\n - " + value;
        }
        if (!enhancedSituations.empty()) sb += "\n";
    }
    if (!enhancedSituations.empty()) {
        sb += Localizer::Format("#autoLOC_230533");
        for (size_t k = 0; k < enhancedSituations.size(); k++) sb += "\n - " + Vessel::GetSituationString(enhancedSituations[k]);
    }
    return sb;
}

// ModuleKerbNetAccess.cs:262
Vessel* ModuleKerbNetAccess::GetKerbNetVessel() { return VesselPtr(); }

// ModuleKerbNetAccess.cs:267
Part* ModuleKerbNetAccess::GetKerbNetPart() { return AsPart(part); }

// ModuleKerbNetAccess.cs:272
std::vector<std::string> ModuleKerbNetAccess::GetKerbNetDisplayModes() {
    std::vector<std::string> list;
    for (size_t i = 0; i < modes.size(); i++) list.push_back(modes[i].Mode);
    return list;
}

// ModuleKerbNetAccess.cs:282
float ModuleKerbNetAccess::GetKerbNetMinimumFoV() {
    if (!isEnhanced) return MinimumFoV();
    return EnhancedMinimumFoV();
}

// ModuleKerbNetAccess.cs:291
float ModuleKerbNetAccess::GetKerbNetMaximumFoV() {
    if (!isEnhanced) return MaximumFoV();
    return EnhancedMaximumFoV();
}

// ModuleKerbNetAccess.cs:300
float ModuleKerbNetAccess::GetKerbNetAnomalyChance() { return AnomalyDetection(); }

// ModuleKerbNetAccess.cs:305 (null = no error)
std::string ModuleKerbNetAccess::GetKerbNetErrorState() {
    if (isCheckingEffects && !partHasEffects) return Localizer::Format("#autoLOC_230585");
    if (RequiresAnimation() && !partIsAnimating) return Localizer::Format("#autoLOC_230588");
    return std::string();
}

// ModuleKerbNetAccess.cs:318
bool ModuleKerbNetAccess::SituationIsEnhanced(VesselSituations situation) const {
    return (static_cast<uint32_t>(situation) & EnhancedSituationMask()) != 0;
}

// ModuleKerbNetAccess.cs:323 (Enum.GetValues order: ascending value)
std::vector<VesselSituations> ModuleKerbNetAccess::GetEnhancedSituations() const {
    std::vector<VesselSituations> list;
    const VesselSituations all[] = {VesselSituations::LANDED,      VesselSituations::SPLASHED, VesselSituations::PRELAUNCH, VesselSituations::FLYING,
                                    VesselSituations::SUB_ORBITAL, VesselSituations::ORBITING, VesselSituations::ESCAPING,  VesselSituations::DOCKED};
    for (VesselSituations s : all)
        if (SituationIsEnhanced(s)) list.push_back(s);
    return list;
}

// ModuleKerbNetAccess.cs:339
void ModuleKerbNetAccess::OnVesselSituationChanged(const GameEvents::HostedFromToAction<Vessel*, Vessel_Situations>& vs) {
    if (vs.host == VesselPtr()) CheckVesselSituationFoV(VesselPtr()->situation, false);
}

// ModuleKerbNetAccess.cs:347
bool ModuleKerbNetAccess::HasThisDialogOpen() { return KerbNetEnvironment::hasThisDialogOpen ? KerbNetEnvironment::hasThisDialogOpen(*this) : false; }

// ModuleKerbNetAccess.cs:356
void ModuleKerbNetAccess::CheckVesselSituationFoV(VesselSituations situation, bool firstRun) {
    const bool flag = HasThisDialogOpen();
    const bool flag2 = SituationIsEnhanced(situation);
    bool flag3 = false;
    float min = 0.0f;
    float max = 0.0f;
    if ((!isEnhanced || firstRun) && flag2) {
        isEnhanced = true;
        flag3 = true;
        min = EnhancedMinimumFoV();
        max = EnhancedMaximumFoV();
        if (flag) {
            Part* p = AsPart(part);
            KerbNetEnvironment::screenMessages.push_back(Localizer::Format("#autoLOC_230641", {p->partInfo->title}));
        }
    } else if ((isEnhanced || firstRun) && !flag2) {
        isEnhanced = false;
        flag3 = true;
        min = MinimumFoV();
        max = MaximumFoV();
        if (flag) {
            Part* p = AsPart(part);
            KerbNetEnvironment::screenMessages.push_back(Localizer::Format("#autoLOC_230651", {p->partInfo->title}));
        }
    }
    if (flag3 && flag && KerbNetEnvironment::setFoVBounds) KerbNetEnvironment::setFoVBounds(min, max);
}

// ModuleKerbNetAccess.cs:391
void ModuleKerbNetAccess::InvokeCrewEffectCheck(Vessel* v) {
    if (!(v != VesselPtr())) {
        if (IsInvoking()) CancelInvoke();
        invokePending_ = true;  // Invoke("CheckRequiredEffects", 0.2f)
    }
}

// the 0.2 s Invoke timer fires (MonoBehaviour.Invoke target CheckRequiredEffects)
void ModuleKerbNetAccess::RunInvoke() {
    if (invokePending_) {
        invokePending_ = false;
        CheckRequiredEffects();
    }
}

// ModuleKerbNetAccess.cs:403
void ModuleKerbNetAccess::CheckRequiredEffects() {
    partHasEffects = PartHasRequiredEffects();
    if (KerbNetEnvironment::refreshErrorState) KerbNetEnvironment::refreshErrorState(*this);
}

// ModuleKerbNetAccess.cs:412
bool ModuleKerbNetAccess::PartHasRequiredEffects() {
    if (!effects.empty()) {
        int num = static_cast<int>(effects.size()) - 1;
        while (true) {
            if (num >= 0) {
                const std::string effect = effects[static_cast<size_t>(num)];
                bool flag = false;
                Part* p = AsPart(part);
                if (p == nullptr) throw NullReferenceException();
                int count = static_cast<int>(p->protoModuleCrew.size());
                while (count-- > 0) {
                    if (p->protoModuleCrew[static_cast<size_t>(count)]->HasEffect(effect)) {
                        flag = true;
                        break;
                    }
                }
                if (!flag) break;
                num--;
                continue;
            }
            return true;
        }
        return false;
    }
    return true;
}

// ModuleKerbNetAccess.cs:446
void ModuleKerbNetAccess::OnAnimationGroupStateChanged(ModuleAnimationGroup* group, bool state) {
    if (RequiresAnimation() && !(static_cast<PartModule*>(group)->part != part)) {
        partIsAnimating = state;
        if (KerbNetEnvironment::refreshErrorState) KerbNetEnvironment::refreshErrorState(*this);
    }
}

}  // namespace ksp
