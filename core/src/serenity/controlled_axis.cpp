// WP-52: ControlledAxis.cs, ControlledAction.cs and RoboticControllerManager.cs (see include/ksp/serenity/robotic_controller.hpp). Every function cites
// <File>.cs:<line> of decompiled/KSP-Decompiled/Assembly-CSharp/Expansions/Serenity/.
#include <algorithm>
#include <cfloat>
#include <cmath>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/serenity/robotic_controller.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp::Expansions::Serenity {

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
float F(double v) { return f32store(v); }

// AnimationCurve.MoveKey(index, key) (native, UNVERIFIED, see the header): the key at `index` is replaced, the order kept
void MoveKey(AnimationCurve& curve, int index, const Keyframe& key) {
    std::vector<Keyframe> keys = curve.keys();
    if (index < 0 || index >= static_cast<int>(keys.size())) return;
    keys[static_cast<size_t>(index)] = key;
    curve.SetKeys(keys);
}

// the module an axis field belongs to (BaseAxisField.module)
PartModule* ModuleOf(BaseAxisField* f) { return f == nullptr ? nullptr : dynamic_cast<PartModule*>(f->host()); }
}  // namespace

// ---------------------------------------------------------------------------------------------------------------- ControlledAxis

const FloatCurve& ControlledAxis::RefSine() {  // :87
    static const FloatCurve c(std::vector<Keyframe>{Keyframe(0.0f, 1.0f, 0.0f, 0.0f), Keyframe(0.5f, 0.0f, 0.0f, 0.0f), Keyframe(1.0f, 1.0f, 0.0f, 0.0f)});
    return c;
}
const FloatCurve& ControlledAxis::RefSquare() {  // :93
    static const FloatCurve c(std::vector<Keyframe>{Keyframe(0.0f, 0.0f, 0.0f, 0.0f), Keyframe(0.499f, 0.0f, 0.0f, 1000.0f), Keyframe(0.5f, 1.0f, 1000.0f, 0.0f),
                                                    Keyframe(0.999f, 1.0f, 0.0f, -1000.0f), Keyframe(1.0f, 0.0f, -1000.0f, 0.0f)});
    return c;
}
const FloatCurve& ControlledAxis::RefTriangle() {  // :101
    static const FloatCurve c(std::vector<Keyframe>{Keyframe(0.0f, 0.0f, -2.0f, 2.0f), Keyframe(0.5f, 1.0f, 2.0f, -2.0f), Keyframe(1.0f, 0.0f, -2.0f, 2.0f)});
    return c;
}
const FloatCurve& ControlledAxis::RefSaw() {  // :107
    static const FloatCurve c(std::vector<Keyframe>{Keyframe(0.0f, 0.0f, 0.0f, 1.0f), Keyframe(0.999f, 1.0f, 1.0f, -1000.0f), Keyframe(1.0f, 0.0f, -1000.0f, 1.0f)});
    return c;
}
const FloatCurve& ControlledAxis::RefRevSaw() {  // :113
    static const FloatCurve c(std::vector<Keyframe>{Keyframe(0.0f, 1.0f, 0.0f, -1.0f), Keyframe(0.999f, 0.0f, -1.0f, 1000.0f), Keyframe(1.0f, 1.0f, 1000.0f, -1.0f)});
    return c;
}

// ControlledAxis.cs:55
ControlledAxis::ControlledAxis(const ControlledAxis& sourceAxis)
    : ControlledAxis(sourceAxis.PartPtr(), sourceAxis.Module(), sourceAxis.AxisField, sourceAxis.Controller) {
    timeValue = FloatCurve(sourceAxis.timeValue.Curve().keys());
}

// ControlledAxis.cs:61
ControlledAxis::ControlledAxis(Part* part, PartModule* module, BaseAxisField* axisField, ModuleRoboticController* controller)
    : ControlledBase(part, module, controller) {
    if (axisField == nullptr) throw Nre();
    axisName = axisField->name();
    AxisField = axisField;
    timeValue = FloatCurve(std::vector<Keyframe>{Keyframe(0.0f, axisField->minValue), Keyframe(1.0f, axisField->minValue)});
    RebuildSymmetryList();
}

// ControlledAxis.cs:74
ControlledAxis::ControlledAxis() {
    AxisField = nullptr;
    timeValue = FloatCurve(std::vector<Keyframe>{Keyframe(0.0f, 0.0f), Keyframe(1.0f, 0.0f)});
    SymmetryFields.clear();
}

// ControlledAxis.cs:121
void ControlledAxis::ReverseCurve() {
    std::vector<Keyframe> array(static_cast<size_t>(timeValue.Curve().length()));
    for (int i = 0; i < timeValue.Curve().length(); i++) {
        const Keyframe tempKeyframe = timeValue.Curve()[timeValue.Curve().length() - 1 - i];
        array[static_cast<size_t>(i)].time = F(fx(timeValue.maxTime()) - fx(tempKeyframe.time));
        array[static_cast<size_t>(i)].value = tempKeyframe.value;
        array[static_cast<size_t>(i)].inTangent = F(0.0 - fx(tempKeyframe.outTangent));
        array[static_cast<size_t>(i)].outTangent = F(0.0 - fx(tempKeyframe.inTangent));
    }
    timeValue = FloatCurve(array);
}

// ControlledAxis.cs:135
void ControlledAxis::InvertCurve() {
    for (int i = 0; i < timeValue.Curve().length(); i++) {
        Keyframe tempKeyframe = timeValue.Curve()[i];
        tempKeyframe.value = F(fx(axisMax()) - (fx(tempKeyframe.value) - fx(axisMin())));
        tempKeyframe.inTangent = F(0.0 - fx(tempKeyframe.inTangent));
        tempKeyframe.outTangent = F(0.0 - fx(tempKeyframe.outTangent));
        MoveKey(timeValue.Curve(), i, tempKeyframe);
    }
}

// ControlledAxis.cs:147
void ControlledAxis::RescaleCurveTime(float adjustmentRatio, float minSpace) {
    const float maxTime = timeValue.maxTime();
    for (int i = 0; i < timeValue.Curve().length(); i++) {
        Keyframe tempKeyframe = timeValue.Curve()[i];
        tempKeyframe.time = F(fx(tempKeyframe.time) * fx(adjustmentRatio));
        if (adjustmentRatio > 1.0f)
            tempKeyframe.time = Mathf::Min(tempKeyframe.time, F(fx(maxTime) - fx(minSpace) * fx(static_cast<float>(timeValue.Curve().length() - i - 1))));
        MoveKey(timeValue.Curve(), i, tempKeyframe);
    }
    if (adjustmentRatio < 1.0f) {
        timeValue.Add(maxTime, timeValue.Curve()[timeValue.Curve().length() - 1].value);
        Keyframe tempKeyframe = timeValue.Curve()[timeValue.Curve().length() - 2];
        tempKeyframe.outTangent = 0.0f;
        MoveKey(timeValue.Curve(), timeValue.Curve().length() - 2, tempKeyframe);
        tempKeyframe = timeValue.Curve()[timeValue.Curve().length() - 1];
        tempKeyframe.inTangent = 0.0f;
        MoveKey(timeValue.Curve(), timeValue.Curve().length() - 1, tempKeyframe);
    }
}

// ControlledAxis.cs:172
void ControlledAxis::AlignCurveEnds() {
    Keyframe key = timeValue.Curve()[timeValue.Curve().length() - 1];
    key.value = timeValue.Curve()[0].value;
    key.inTangent = timeValue.Curve()[0].outTangent;
    MoveKey(timeValue.Curve(), timeValue.Curve().length() - 1, key);
}

// ControlledAxis.cs:180
void ControlledAxis::ClampAllPointValues() {
    for (int i = 0; i < timeValue.Curve().length(); i++) {
        Keyframe tempKeyframe = timeValue.Curve()[i];
        tempKeyframe.value = Mathf::Clamp(tempKeyframe.value, axisMin(), axisMax());
        MoveKey(timeValue.Curve(), i, tempKeyframe);
    }
}

// ControlledAxis.cs:190 (sic: `index <= length`, so index == length reads past the end: IndexOutOfRange in C#)
void ControlledAxis::ClampPointValue(int index) {
    if (index >= 0 && index <= timeValue.Curve().length()) {
        if (index >= timeValue.Curve().length()) throw std::out_of_range("Index was outside the bounds of the array.");
        Keyframe tempKeyframe = timeValue.Curve()[index];
        tempKeyframe.value = Mathf::Clamp(tempKeyframe.value, axisMin(), axisMax());
        MoveKey(timeValue.Curve(), index, tempKeyframe);
    }
}

// ControlledAxis.cs:200
void ControlledAxis::PresetFlat() {
    const float maxTime = timeValue.maxTime();
    float value = F((fx(axisMax()) - fx(axisMin())) / 2.0);
    auto* limits = dynamic_cast<IAxisFieldLimits*>(ModuleOf(AxisField));
    if (limits != nullptr && limits->HasAxisFieldLimit(axisName)) {
        AxisFieldLimit* l = limits->GetAxisFieldLimit(axisName);
        value = F((fx(l->softLimits.y) - fx(l->softLimits.x)) / 2.0);
    }
    timeValue = FloatCurve(std::vector<Keyframe>{Keyframe(0.0f, value), Keyframe(maxTime, value)});
    lastUsedRefName = "stockFlat";
}

// ControlledAxis.cs:217
void ControlledAxis::PresetSine(float cycles, float phase) {
    FloatCurve generated;
    if (GenerateCurveFromReference("stockSine", cycles, phase, &RefSine(), generated)) timeValue = generated;
}
// ControlledAxis.cs:225
void ControlledAxis::PresetSquare(float cycles, float phase) {
    FloatCurve generated;
    if (GenerateCurveFromReference("stockSquare", cycles, phase, &RefSquare(), generated)) timeValue = generated;
}
// ControlledAxis.cs:233
void ControlledAxis::PresetTriangle(float cycles, float phase) {
    FloatCurve generated;
    if (GenerateCurveFromReference("stockTriangle", cycles, phase, &RefTriangle(), generated)) timeValue = generated;
}
// ControlledAxis.cs:241
void ControlledAxis::PresetSaw(float cycles, float phase) {
    FloatCurve generated;
    if (GenerateCurveFromReference("stockSaw", cycles, phase, &RefSaw(), generated)) timeValue = generated;
}
// ControlledAxis.cs:249
void ControlledAxis::PresetRevSaw(float cycles, float phase) {
    FloatCurve generated;
    if (GenerateCurveFromReference("stockRevSaw", cycles, phase, &RefRevSaw(), generated)) timeValue = generated;
}

// ControlledAxis.cs:257
void ControlledAxis::UpdatePreset(float cycles, float phase) {
    FloatCurve generated;
    if (!(lastUsedRefName == "stockFlat") && GenerateCurveFromReference(lastUsedRefName, cycles, phase, nullptr, generated)) timeValue = generated;
}

// ControlledAxis.cs:265
void ControlledAxis::ClearPresetRefs() {
    lastUsedRefName = "";
    lastUsedRefCurve = nullptr;
}

// ControlledAxis.cs:271
bool ControlledAxis::GenerateCurveFromReference(const std::string& refCurveID, float cycles, float phase, const FloatCurve* referenceCurve,
                                                FloatCurve& generatedCurve) {
    if (lastUsedRefCurve == nullptr && referenceCurve == nullptr) {
        log::Error("[ControlledAxis] Unable to Generate curve - no reference supplied and no existing reference to use");
        generatedCurve = FloatCurve();
        return false;
    }
    float num = axisMax();
    float num2 = axisMin();
    if (dynamic_cast<ModuleRoboticController*>(ModuleOf(AxisField)) != nullptr) {
        num = 1.0f;
        num2 = 0.0f;
    }
    if (auto* limits = dynamic_cast<IAxisFieldLimits*>(ModuleOf(AxisField)); limits != nullptr && limits->HasAxisFieldLimit(axisName)) {
        AxisFieldLimit* l = limits->GetAxisFieldLimit(axisName);
        num2 = l->softLimits.x;
        num = l->softLimits.y;
    }
    const int num3 =
        std::max(Mathf::CeilToInt(F((fx(cycles) + fx(static_cast<float>(refCurveCyclesPadding))) / fx(static_cast<float>(refCurveCyclesStepIncrement)))), 2) *
        refCurveCyclesStepIncrement;
    if (lastUsedRefName != refCurveID || !lastUsedRefKeysSet || lastUsedCurve == nullptr || lastUsedRefMax != num || lastUsedRefMin != num2 ||
        lastUsedCurve->length() < num3) {
        lastUsedRefMin = num2;
        lastUsedRefMax = num;
        lastUsedRefName = refCurveID;
        if (referenceCurve != nullptr)
            lastUsedRefCurve = std::make_shared<FloatCurve>(*referenceCurve);
        else
            referenceCurve = lastUsedRefCurve.get();
        lastUsedRefKeys.clear();
        lastUsedRefKeysSet = true;
        const std::vector<Keyframe>& rk = referenceCurve->Curve().keys();
        const double span = fx(num) - fx(num2);
        lastUsedRefKeys.push_back(Keyframe(rk[0].time, F(fx(num2) + fx(rk[0].value) * (fx(num) - fx(num2))), F(fx(rk[0].inTangent) * (fx(num) - fx(num2))),
                                           F(fx(rk[0].outTangent) * (fx(num) - fx(num2)))));
        (void)span;
        for (int i = 0; i < num3; i++) {
            for (int j = 1; j < referenceCurve->Curve().length(); j++) {
                const Keyframe& keyframe = rk[static_cast<size_t>(j)];
                lastUsedRefKeys.push_back(Keyframe(F(fx(keyframe.time) + fx(static_cast<float>(i))), F(fx(num2) + fx(keyframe.value) * (fx(num) - fx(num2))),
                                                   F(fx(keyframe.inTangent) * (fx(num) - fx(num2))), F(fx(keyframe.outTangent) * (fx(num) - fx(num2)))));
            }
        }
        lastUsedCurve = std::make_shared<AnimationCurve>(lastUsedRefKeys);
    }
    const float num4 = F(fx(phase) / fx(100.0f));
    std::vector<Keyframe> list;
    for (size_t k = 0; k < lastUsedRefKeys.size(); k++) {
        Keyframe item = lastUsedRefKeys[k];
        item.time = F(fx(item.time) - fx(num4));
        if (item.time <= cycles) {
            if (list.empty()) {
                if (item.time < 0.0f) continue;
                if (item.time != num4) {
                    const float e0 = lastUsedCurve->Evaluate(num4);
                    const float e1 = lastUsedCurve->Evaluate(F(fx(num4) + fx(0.001f)));
                    list.push_back(Keyframe(0.0f, e0, 0.0f, F((fx(e1) - fx(e0)) / fx(0.001f) * fx(cycles))));
                }
            }
            item.time = F(fx(item.time) / fx(cycles));
            item.inTangent = F(fx(item.inTangent) * fx(cycles));
            item.outTangent = F(fx(item.outTangent) * fx(cycles));
            list.push_back(item);
            continue;
        }
        if (k == 0) throw std::out_of_range("Index was out of range. Must be non-negative and less than the size of the collection.");
        if (fx(lastUsedRefKeys[k - 1].time) - fx(num4) < fx(cycles)) {
            const float e1 = lastUsedCurve->Evaluate(F(fx(cycles) + fx(num4)));
            const float e0 = lastUsedCurve->Evaluate(F(fx(cycles) + fx(num4) - fx(0.001f)));
            list.push_back(Keyframe(1.0f, e1, F((fx(e1) - fx(e0)) / fx(0.001f) * fx(cycles)), 0.0f));
        }
        break;
    }
    generatedCurve = FloatCurve(list);
    return true;
}

// ControlledAxis.cs:355
bool ControlledAxis::OnChangeSymmetryMaster(Part* newPart, uint32_t& oldPartId) {
    oldPartId = 0u;
    BaseAxisField* baseAxisField = nullptr;
    for (size_t i = 0; i < newPart->Modules().Count(); i++) {
        baseAxisField = dynamic_cast<BaseAxisField*>(newPart->Modules()[i]->Fields()[axisName]);
        if (baseAxisField != nullptr) break;
    }
    if (baseAxisField == nullptr) {
        log::Error("[ControlledAxis]: Cannot change Symmetry Master to " + std::to_string(newPart->fields.persistentId) + " as its not listed in the SymmetryParts for " + axisName);
        return false;
    }
    oldPartId = partId;
    AxisField = baseAxisField;
    return true;
}

// ControlledAxis.cs:377
void ControlledAxis::UpdateFieldValue(float time, float valueMultiple, bool updateSymmetryPartners) {
    if (AxisField == nullptr) return;
    const float num = F(fx(timeValue.Evaluate(time)) * fx(valueMultiple));
    if (RoboticControllerManager::Instance != nullptr) {
        if (Controller == nullptr) throw Nre();
        RoboticControllerManager::Instance->QueueFieldUpdate(AxisField, num, Controller->Priority());
        if (updateSymmetryPartners)
            for (size_t i = 0; i < SymmetryFields.size(); i++) RoboticControllerManager::Instance->QueueFieldUpdate(SymmetryFields[i], num, Controller->Priority());
        return;
    }
    AxisField->SetValue(ModuleValue(num));
    if (updateSymmetryPartners)
        for (size_t j = 0; j < SymmetryFields.size(); j++) SymmetryFields[j]->SetValue(ModuleValue(num));
}

// ControlledAxis.cs:406
void ControlledAxis::UpdateSoftLimits(const Vector2f& newLimits, bool updateSymmetryPartners) {
    auto* limits = dynamic_cast<IAxisFieldLimits*>(ModuleOf(AxisField));
    if (AxisField == nullptr || limits == nullptr) return;
    limits->SetSoftLimits(axisName, newLimits);
    if (!updateSymmetryPartners) return;
    for (size_t i = 0; i < SymmetryFields.size(); i++)
        if (auto* l2 = dynamic_cast<IAxisFieldLimits*>(ModuleOf(SymmetryFields[i]))) l2->SetSoftLimits(axisName, newLimits);
}

// ControlledAxis.cs:426
bool ControlledAxis::OnAssignReferenceVars() {
    if (module_ != nullptr) {
        if (auto* axisField = dynamic_cast<BaseAxisField*>(module_->Fields()[axisName])) AxisField = axisField;
    } else {
        Part* p = part_;
        for (size_t i = 0; i < p->Modules().Count(); i++) {
            PartModule* m = p->Modules()[i];
            for (size_t j = 0; j < m->Fields().Count(); j++) {
                if (auto* baseAxisField = dynamic_cast<BaseAxisField*>(m->Fields()[j]); baseAxisField != nullptr && baseAxisField->name() == axisName) {
                    AxisField = baseAxisField;
                    module_ = m;
                    moduleId = module_->GetPersistentId();
                    persistentActionId = SerenityMiscEnvironment::persistentActionId ? SerenityMiscEnvironment::persistentActionId(*module_) : 0u;
                    break;
                }
            }
        }
    }
    if (AxisField == nullptr) {
        log::Warning("[ModuleRoboticController]: Unable to find AxisField in vessel part: " + part_->gameObjectName + " - " + axisName);
        return false;
    }
    return true;
}

// ControlledAxis.cs:460
void ControlledAxis::ClearSymmetryLists() { SymmetryFields.clear(); }

// ControlledAxis.cs:472
void ControlledAxis::AddSymmetryPart(Part* part) {
    PartModule* own = ModuleOf(AxisField);
    if (own == nullptr) throw Nre();
    if (part->Modules().Contains(own->ClassName())) {
        if (auto* item = dynamic_cast<BaseAxisField*>(part->Modules()[own->ClassName()]->Fields()[axisName])) SymmetryFields.push_back(item);
    }
}

// ControlledAxis.cs:480
void ControlledAxis::OnLoad(const ConfigNode& node) {
    node.TryGetValue("axisName", axisName);
    ConfigNode* node2 = nullptr;
    if (node.TryGetNode("timeValueCurve", node2)) timeValue.Load(*node2);
    if (timeValue.Curve().length() == 0) {
        timeValue.Curve().AddKey(0.0f, 0.0f);
        timeValue.Curve().AddKey(1.0f, 0.0f);
    }
}

// ControlledAxis.cs:495
void ControlledAxis::OnSave(ConfigNode& node) {
    node.AddValue("axisName", axisName);
    timeValue.Save(*node.AddNode("timeValueCurve"));
}

// ---------------------------------------------------------------------------------------------------------------- ControlledAction

// ControlledAction.cs:25
ControlledAction::ControlledAction(const ControlledAction& sourceAction) : ControlledAction(sourceAction.PartPtr(), sourceAction.Module(), sourceAction.Action, sourceAction.Controller) {
    times = sourceAction.times;
}

// ControlledAction.cs:31
ControlledAction::ControlledAction(Part* part, PartModule* module, BaseAction* action, ModuleRoboticController* controller)
    : ControlledBase(part, module, controller) {
    if (action == nullptr) throw Nre();
    actionName = action->name;
    Action = action;
    times.clear();
    RebuildSymmetryList();
}

// ControlledAction.cs:40
ControlledAction::ControlledAction() {
    Action = nullptr;
    times.clear();
    SymmetryActions.clear();
}

// ControlledAction.cs:47
void ControlledAction::ReverseTimes() {
    std::vector<float> list;
    size_t count = times.size();
    while (count-- > 0) list.push_back(F(1.0 - fx(times[count])));
    times = list;
}

// ControlledAction.cs:58
void ControlledAction::RescaleTimes(float adjustmentRatio, float minSpace) {
    std::vector<float> list;
    for (size_t i = 0; i < times.size(); i++) {
        tempTime = times[i];
        tempTime = F(fx(tempTime) * fx(adjustmentRatio));
        if (adjustmentRatio > 1.0f) tempTime = Mathf::Min(tempTime, F(1.0 - fx(minSpace) * fx(static_cast<float>(static_cast<int>(times.size()) - static_cast<int>(i) - 1))));
        list.push_back(tempTime);
    }
    times = list;
}

namespace {
// Part.Actions[name] (the part's own KSPActions)
BaseAction* PartAction(Part& p, const std::string& name) { return ControllerEnvironment::partAction ? ControllerEnvironment::partAction(p, name) : nullptr; }
}  // namespace

// ControlledAction.cs:74
bool ControlledAction::OnChangeSymmetryMaster(Part* newPart, uint32_t& oldPartId) {
    oldPartId = 0u;
    BaseAction* baseAction = nullptr;
    // for (i < newPart.Actions.Count) baseAction = newPart.Actions[actionName]: the part's own action list (a seam)
    if (ControllerEnvironment::partAction) baseAction = PartAction(*newPart, actionName);
    if (baseAction == nullptr) {
        log::Error("[ControlledAction]: Cannot change Symmetry Master to " + std::to_string(newPart->fields.persistentId) + ". No " + actionName + " field on this part");
        return false;
    }
    oldPartId = partId;
    Action = baseAction;
    return true;
}

// ControlledAction.cs:96
bool ControlledAction::OnAssignReferenceVars() {
    if (module_ != nullptr) {
        BaseAction* baseAction = module_->Actions()[actionName];
        if (baseAction != nullptr) Action = baseAction;
    } else if (PartAction(*part_, actionName) != nullptr) {
        Action = PartAction(*part_, actionName);
    } else {
        for (size_t i = 0; i < part_->Modules().Count(); i++) {
            BaseAction* baseAction2 = part_->Modules()[i]->Actions()[actionName];
            if (baseAction2 != nullptr) {
                Action = baseAction2;
                module_ = part_->Modules()[i];
                moduleId = module_->GetPersistentId();
                persistentActionId = SerenityMiscEnvironment::persistentActionId ? SerenityMiscEnvironment::persistentActionId(*module_) : 0u;
                break;
            }
        }
    }
    if (Action == nullptr) {
        log::Warning("[ModuleRoboticController]: Unable to find Action in vessel part: " + part_->gameObjectName + " - " + actionName);
        return false;
    }
    return true;
}

// ControlledAction.cs:133
void ControlledAction::ClearSymmetryLists() { SymmetryActions.clear(); }

// ControlledAction.cs:145
void ControlledAction::AddSymmetryPart(Part* part) {
    if (BaseAction* a = PartAction(*part, actionName)) {
        SymmetryActions.push_back(a);
        return;
    }
    for (size_t num = 0; num < part->Modules().Count(); num++) {
        BaseAction* baseAction = part->Modules()[num]->Actions()[actionName];
        if (baseAction != nullptr) {
            SymmetryActions.push_back(baseAction);
            return;
        }
    }
}

// ControlledAction.cs:171
void ControlledAction::OnLoad(const ConfigNode& node) {
    node.TryGetValue("actionName", actionName);
    ConfigNode* node2 = nullptr;
    if (!node.TryGetNode("TIMES", node2)) return;
    const std::vector<std::string> valuesList = node2->GetValuesList("time");
    for (size_t i = 0; i < valuesList.size(); i++) times.push_back(dotnet::ParseSingle(valuesList[i]));
    size_t count = times.size();
    while (count-- > 0) {
        if (times[count] > 1.0f || times[count] < 0.0f) times.erase(times.begin() + static_cast<std::ptrdiff_t>(count));
    }
}

// ControlledAction.cs:194
void ControlledAction::OnSave(ConfigNode& node) {
    node.AddValue("actionName", actionName);
    ConfigNode* configNode = node.AddNode("TIMES");
    for (size_t i = 0; i < times.size(); i++) configNode->AddValue("time", times[i]);
}

// ---------------------------------------------------------------------------------------------------------------- RoboticControllerManager

// RoboticControllerManager.cs:38
void RoboticControllerManager::Awake() {
    if (!SaveEnvironment::Current().IsExpansionInstalled("Serenity")) {
        destroyed = true;
        return;
    }
    if (Instance != nullptr) {
        log::Error("[RoboticControllerManager]: Only one instance of this class may exist per scene. Destroying potential usurper.");
        destroyed = true;
        return;
    }
    Instance = this;
    queue_.clear();
    GameEvents::onGameSceneLoadRequested.Add(this, &RoboticControllerManager::OnSceneLoad);
}

// RoboticControllerManager.cs:57 (a destroyed UnityEngine.Object compares equal to null: Instance reads as null afterwards)
void RoboticControllerManager::OnDestroy() {
    GameEvents::onGameSceneLoadRequested.Remove(this, &RoboticControllerManager::OnSceneLoad);
    if (Instance == this) Instance = nullptr;
}

// RoboticControllerManager.cs:106
void RoboticControllerManager::QueueFieldUpdate(BaseAxisField* axisField, float newValue, int priority) {
    auto it = std::find_if(queue_.begin(), queue_.end(), [&](const auto& e) { return e.first == axisField; });
    if (it == queue_.end()) {
        queue_.emplace_back(axisField, AxisUpdate());
        it = queue_.end() - 1;
    }
    AxisUpdate& val = it->second;
    if (priority >= val.priority) {
        if (priority > val.priority) {
            val.priority = priority;
            val.newValue = newValue;
            val.values = 1;
        } else {
            val.newValue = F(fx(val.newValue) + fx(newValue));
            val.values++;
        }
    }
}

// RoboticControllerManager.cs:134
void RoboticControllerManager::UpdateFieldValues() {
    for (size_t i = 0; i < queue_.size(); i++) {
        BaseAxisField* baseAxisField = queue_[i].first;
        AxisUpdate& u = queue_[i].second;
        if (u.values > 0) {
            updateValueAverage = F(fx(u.newValue) / fx(static_cast<float>(u.values)));
            baseAxisField->SetValue(ModuleValue(updateValueAverage));
        }
        u = AxisUpdate();  // Reset
    }
}

}  // namespace ksp::Expansions::Serenity
