// WP-53 slice of ModuleCargoPart.cs (see include/ksp/modules/module_cargo_part.hpp).
#include "ksp/modules/module_cargo_part.hpp"

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/science/science.hpp"
#include "ksp/serenity/serenity_env.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/save/save_environment.hpp"

namespace ksp {

using KSP::Localization::Localizer;
using Expansions::Serenity::SerenityEnvironment;

Part* ModuleCargoPart::PartPtr() const { return dynamic_cast<Part*>(part); }
Vessel* ModuleCargoPart::VesselPtr() const {
    Part* p = PartPtr();
    return p ? p->vessel : nullptr;
}

ModuleCargoPart::~ModuleCargoPart() {
    RemoveEventHandlers();
    // a destroyed MonoBehaviour's coroutines stop (Unity); StartCoroutine used Scheduler(), the process fallback when no scheduler is set
    // (WP-20g: since Part.Unpack runs OnPartUnpack, a single landed cargo part starts MakePartSettle there, and the fallback scheduler
    // otherwise kept a coroutine of a freed module)
    ScienceEnvironment::Scheduler().StopAllCoroutines(CoroutineOwner());
}

std::shared_ptr<Coroutine> ModuleCargoPart::StartCoroutine(Enumerator e) {
    return ScienceEnvironment::Scheduler().StartCoroutine(std::move(e), CoroutineOwner());
}

void ModuleCargoPart::StopCoroutineRef(std::shared_ptr<Coroutine>& c) {
    if (c) ScienceEnvironment::Scheduler().StopCoroutine(c);
    c.reset();
}

// ModuleCargoPart.cs:55
float ModuleCargoPart::MassForWeightTesting() const {
    if (preSettlingPrefabMass > 0.0f) return preSettlingPrefabMass;
    return part->fields.mass;
}

// ModuleCargoPart.cs:67
void ModuleCargoPart::OnDestroy() {
    StopCoroutineRef(settleCoroutine_);
    RemoveEventHandlers();
}

// ModuleCargoPart.cs:84
void ModuleCargoPart::OnPartUnpack() {
    if (beingAttached()) {
        Set<bool>("beingAttached", false);
    } else {
        Vessel* v = VesselPtr();
        if (v == nullptr) throw NullReferenceException();
        if (beingSettled() ||
            (v->parts.size() == 1 && (v->situation == Vessel::Situations::LANDED || v->situation == Vessel::Situations::PRELAUNCH))) {
            settleCoroutine_ = StartCoroutine(MakePartSettle(false));
        }
    }
}

// ModuleCargoPart.cs:96
void ModuleCargoPart::OnLoad(const ConfigNode& node) {
    PartModule::OnLoad(node);
    if (stackableQuantity() > 1 && part != nullptr && !part->Resources.empty()) {
        log::Info("[ModuleCargoPart]: Setting stackableQuantity to 1 as " + part->gameObjectName + " contains resources");
        Set<int32_t>("stackableQuantity", 1);
    }
}

// ModuleCargoPart.cs:164
std::string ModuleCargoPart::GetInfo() {
    std::string text = (!(packedVolume() < 0.0f)) ? Localizer::Format("#autoLOC_8002220") : Localizer::Format("#autoLOC_6002641");
    text += "\n\n";
    Part* p = PartPtr();
    if (p == nullptr || p->partInfo == nullptr || !p->partInfo->partPrefab) throw NullReferenceException();
    const float prefabMass = p->partInfo->partPrefab->fields.mass;
    const float total = f32store(fx(prefabMass) + fx(p->GetModuleMass(prefabMass)));
    text += Localizer::Format("#autoLOC_8002186") + " " + dotnet::FormatSingleF(total, 3) + " t\\n";  // sic: literal backslash-n
    if (packedVolume() > 0.0f)
        text += Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8003414"),
                                                         Localizer::Format("<<1>><<2>>", {dotnet::FormatSingleF(packedVolume(), 1), "L"})}) + "\n";
    if (stackableQuantity() > 1)
        text += Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8003418"), std::to_string(stackableQuantity())}) + "\n";
    return text;
}

// ModuleCargoPart.cs:336
std::string ModuleCargoPart::GetTooltip() {
    if (Get<std::string>("inventoryTooltip").empty()) return GetInfo();
    return Localizer::Format(Get<std::string>("inventoryTooltip"));
}

// ModuleCargoPart.cs:345
bool ModuleCargoPart::IsDeployedSciencePart() {
    if (!SaveEnvironment::Current().IsExpansionInstalled("Serenity")) return false;
    Part* p = PartPtr();
    if (p == nullptr) throw NullReferenceException();
    return p->FindModuleOfClass("ModuleGroundSciencePart") != nullptr;
}

// ModuleCargoPart.cs:354
void ModuleCargoPart::EnableModule() { isEnabled = true; }

// ModuleCargoPart.cs:396
std::string ModuleCargoPart::GetModuleDisplayName() { return Localizer::Format("#autoLOC_8002221"); }

// ModuleCargoPart.cs:202
Enumerator ModuleCargoPart::MakePartSettle(bool waitForMotionStop) {
    bool adjustMass = false;
    preSettlingPrefabMass = part->fields.prefabMass;
    Vessel* v = VesselPtr();
    if (v != nullptr && v->mainBody() != nullptr) {
        if (v->mainBody()->GeeASL < 0.800000011920929) {
            Set<bool>("beingSettled", true);
            adjustMass = true;
            part->fields.prefabMass = 20.0f;
        } else if (v->mainBody()->GeeASL < 1.100000023841858) {
            Set<bool>("beingSettled", true);
            adjustMass = true;
            part->fields.prefabMass = 4.0f;
        }
    }
    if (kinematicDelay() > 0.0f) co_yield YieldInstruction::WaitForSeconds(kinematicDelay());
    if (waitForMotionStop) {
        const double timeWaitingForSrfVelocity = ScienceEnvironment::UniversalTime();
        const float settleDelay = f32store(fx(kinematicDelay()) * 10.0);
        while (true) {
            if (ScienceEnvironment::UniversalTime() - timeWaitingForSrfVelocity > static_cast<double>(settleDelay)) break;
            Vessel* vv = VesselPtr();
            if (vv == nullptr) throw NullReferenceException();
            const double mag = SerenityEnvironment::srfVelocityMagnitude ? SerenityEnvironment::srfVelocityMagnitude(*vv) : vv->srf_velocity.magnitude();
            if (mag < static_cast<double>(placementMaxRivotVelocity())) break;
            co_yield YieldInstruction::Null();
        }
    }
    if (adjustMass) {
        part->fields.prefabMass = preSettlingPrefabMass;
        preSettlingPrefabMass = 0.0f;
        Set<bool>("beingSettled", false);
    }
}

}  // namespace ksp
