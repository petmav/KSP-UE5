// WP-17: DeltaVPartInfo.cs, DeltaVGlobals.cs, DeltaVAppValues.cs (data half), DeltaVPropellantInfo.cs and the DeltaV*Extensions.cs files.
#include <algorithm>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/modules/module_decouplers.hpp"
#include "ksp/modules/module_resource_intake.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/delta_v.hpp"

namespace ksp {

// ---- DeltaVAppValues.cs -------------------------------------------------------------------------------------------------------

// DeltaVAppValues.cs:81 atmPressure => (float)(body.GetPressure(altitude) * 0.009869232667160128)
float DeltaVAppValues::atmPressure() const { return f32store(body->GetPressure(altitude) * 0.009869232667160128); }

// DeltaVAppValues.cs:83 atmDensity => body.GetDensity(body.GetPressure(altitude), body.GetTemperature(altitude))
double DeltaVAppValues::atmDensity() const { return body->GetDensity(body->GetPressure(altitude), body->GetTemperature(altitude)); }

// ---- DeltaVGlobals.cs ---------------------------------------------------------------------------------------------------------

// DeltaVGlobals.cs:60 PropellantsToIgnore getter: "IntakeAir".GetHashCode(), "ElectricCharge".GetHashCode()
const std::vector<int>& DeltaVGlobals::PropellantsToIgnore() {
    if (!propellantsInit_) {
        propellantsToIgnore_.clear();
        propellantsToIgnore_.push_back(MonoStringHash("IntakeAir"));
        propellantsToIgnore_.push_back(MonoStringHash("ElectricCharge"));
        propellantsInit_ = true;
    }
    return propellantsToIgnore_;
}

// DeltaVGlobals.cs:73 PropellantsToIgnore setter
void DeltaVGlobals::SetPropellantsToIgnore(std::vector<int> v) {
    propellantsToIgnore_ = std::move(v);
    propellantsInit_ = true;
    const int a = MonoStringHash("IntakeAir");
    const int b = MonoStringHash("ElectricCharge");
    if (std::find(propellantsToIgnore_.begin(), propellantsToIgnore_.end(), a) == propellantsToIgnore_.end()) propellantsToIgnore_.push_back(a);
    if (std::find(propellantsToIgnore_.begin(), propellantsToIgnore_.end(), b) == propellantsToIgnore_.end()) propellantsToIgnore_.push_back(b);
}

void DeltaVGlobals::ResetPropellantsToIgnore() {
    propellantsToIgnore_.clear();
    propellantsInit_ = false;
}

bool DeltaVGlobals::PropellantIgnored(int id) {
    const std::vector<int>& v = PropellantsToIgnore();
    return std::find(v.begin(), v.end(), id) != v.end();
}

bool DeltaVGlobals::LoadedSceneIsEditor() { return SaveEnvironment::Current().loadedScene == GameScenes::EDITOR; }
bool DeltaVGlobals::LoadedSceneIsFlight() { return SaveEnvironment::Current().loadedScene == GameScenes::FLIGHT; }

// DeltaVGlobals.cs:129 EnableStockSimluations
bool DeltaVGlobals::EnableStockSimluations() {
    if (!DeltaVGameSettings::DELTAV_CALCULATIONS_ENABLED) return false;
    _doStockSimulations = true;
    return true;
}

// DeltaVGlobals.cs:140 DisableStockSimluations
bool DeltaVGlobals::DisableStockSimluations() {
    _doStockSimulations = false;
    return true;
}

// ---- DeltaVPropellantInfo.cs --------------------------------------------------------------------------------------------------

// DeltaVPropellantInfo.cs:30
DeltaVPropellantInfo::DeltaVPropellantInfo(Propellant* p) : propellant(p) {}

// DeltaVPropellantInfo.cs:43
DeltaVPropellantInfo DeltaVPropellantInfo::Copy() const {
    DeltaVPropellantInfo c(propellant);
    c.amountAvailable = amountAvailable;
    c.amountBurnt = amountBurnt;
    c.maxBurnTime = maxBurnTime;
    c.amountPerSecondMaxThrottle = amountPerSecondMaxThrottle;
    c.amountPerSecondSetThrottle = amountPerSecondSetThrottle;
    c.amountPerSecondCurrentThrottle = amountPerSecondCurrentThrottle;
    c.pendingDemand = pendingDemand;
    c.timeLeftSetThrottle = timeLeftSetThrottle;
    c.timeLeftCurrentThrottle = timeLeftCurrentThrottle;
    return c;
}

// DeltaVPropellantInfo.cs:57 MassBurnt: propellant.getPartResourceDefinition().density * (float)amountBurnt
float DeltaVPropellantInfo::MassBurnt() const {
    if (DeltaVGlobals::PropellantIgnored(propellant->id)) return 0.0f;
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    if (lib == nullptr) throw NullReferenceException();  // getPartResourceDefinition() is null without a library
    const PartResourceDefinition* def = lib->GetDefinition(propellant->id);
    if (def == nullptr) throw NullReferenceException();
    return f32store(fx(def->density()) * fx(f32store(amountBurnt)));
}

// ---- extension files ----------------------------------------------------------------------------------------------------------

namespace deltav_ext {

// DeltaVEngineExtensions.cs:5
bool Contains(const std::vector<DeltaVEngineInfoPtr>& list, const Part* part) {
    for (const auto& e : list)
        if (e->engine->part == part) return true;
    return false;
}

// DeltaVEngineExtensions.cs:20
DeltaVEngineInfo* Get(const std::vector<DeltaVEngineInfoPtr>& list, const Part* part) {
    for (const auto& e : list)
        if (e->engine->part == part) return e.get();
    return nullptr;
}

// DeltaVPartExtensions.cs:5
bool ContainsPart(const std::vector<DeltaVPartInfoPtr>& list, const Part* part) {
    for (const auto& p : list)
        if (p->part->fields.persistentId == part->fields.persistentId) return true;
    return false;
}

// DeltaVPartExtensions.cs:20
DeltaVPartInfo* Get(const std::vector<DeltaVPartInfoPtr>& list, const Part* part) {
    for (const auto& p : list)
        if (p->part->fields.persistentId == part->fields.persistentId) return p.get();
    return nullptr;
}

// DeltaVPartExtensions.cs:37
std::vector<Part*> PartsInStage(const std::vector<DeltaVPartInfoPtr>& list, int stage) {
    std::vector<Part*> r;
    r.reserve(list.size());
    for (const auto& p : list)
        if (p->decoupleStage != stage || (p->decoupleStage == stage && !p->decoupleBeforeBurn)) r.push_back(p->part);
    return r;
}

// DeltaVEngineResourcePartExtensions.cs:6
bool ContainsPartInfo(const std::vector<DeltaVEngineResourcePartInfo>& list, const DeltaVPartInfo& partInfo) {
    if (partInfo.part != nullptr) {
        for (const auto& e : list)
            if (e.resourcePart->part != nullptr && e.resourcePart->part->fields.persistentId == partInfo.part->fields.persistentId) return true;
    }
    return false;
}

// DeltaVEngineResourcePartExtensions.cs:20
DeltaVEngineResourcePartInfo* Get(std::vector<DeltaVEngineResourcePartInfo>& list, const DeltaVPartInfo& partInfo) {
    if (partInfo.part != nullptr) {
        for (auto& e : list)
            if (e.resourcePart->part != nullptr && e.resourcePart->part->fields.persistentId == partInfo.part->fields.persistentId) return &e;
    }
    return nullptr;
}

// DeltaVEngineResourcePartExtensions.cs:46
int GetHighestPartSeparationIndex(const std::vector<DeltaVEngineResourcePartInfo>& list, int stage) {
    int num = -1;
    for (const auto& e : list)
        if (e.resourcePart->decoupleStage < stage) num = dotnet::Math::Max(num, e.resourcePart->decoupleStage);
    return num;
}

// DeltaVEngineResourcePartExtensions.cs:58
std::vector<DeltaVEngineResourcePartInfo> PartsMatchingSeparationIndex(const std::vector<DeltaVEngineResourcePartInfo>& input, int index) {
    std::vector<DeltaVEngineResourcePartInfo> r;
    for (const auto& e : input)
        if (e.resourcePart->decoupleStage == index) r.push_back(e);
    return r;
}

}  // namespace deltav_ext

// ---- DeltaVPartInfo.cs --------------------------------------------------------------------------------------------------------

namespace {

// Part.FindModuleImplementing<T>() for the module classes the delta-v code looks at
template <class T>
T* FindModule(Part& part) {
    for (size_t i = 0; i < part.Modules().Count(); i++)
        if (auto* m = dynamic_cast<T*>(part.Modules().GetModule(i))) return m;
    return nullptr;
}

PartModule* FindClass(Part& part, const char* className) { return part.FindModuleOfClass(className); }

}  // namespace

// DeltaVPartInfo.cs:76
DeltaVPartInfo::DeltaVPartInfo(Part* p, VesselDeltaV* v) : vesselDeltaV(v), part(p) {
    // Part.isDecoupler(out ModuleDecouple) / isAnchoredDecoupler(out ModuleAnchoredDecoupler)  (Part.cs:1285, 1279)
    moduleDecoupler = FindModule<ModuleDecouple>(*part);
    const bool isDec = moduleDecoupler != nullptr;
    if (!isDec) moduleAnchoredDecoupler = FindModule<ModuleAnchoredDecoupler>(*part);
    isDecoupler = isDec || moduleAnchoredDecoupler != nullptr;
    moduleStageSeparator = FindModule<IStageSeparator>(*part);
    if (moduleStageSeparator != nullptr) isStageSeparator = true;
    isFairing = part->HasModuleImplementing([](PartModule& m) { return m.IsA("ModuleProceduralFairing"); });  // Part.cs:1331
    moduleDockingNode = FindClass(*part, "ModuleDockingNode");                                                 // Part.cs:1291
    isDockingPort = moduleDockingNode != nullptr;
    for (size_t i = 0; i < part->Modules().Count(); i++)                                                       // Part.cs:1341
        if (auto* e = dynamic_cast<ModuleEngines*>(part->Modules().GetModule(i))) engines.push_back(e);
    isEngine = !engines.empty();
    moduleResourceIntake = FindModule<ModuleResourceIntake>(*part);                                            // Part.cs:1297
    if (moduleResourceIntake != nullptr && moduleResourceIntake->Get<std::string>("resourceName") != "IntakeAir") moduleResourceIntake = nullptr;
    isIntake = moduleResourceIntake != nullptr;
    moduleDeployableSolarPanel = FindClass(*part, "ModuleDeployableSolarPanel");                               // Part.cs:1307
    isSolarPanel = moduleDeployableSolarPanel != nullptr;
    moduleGenerator = FindClass(*part, "ModuleGenerator");                                                     // Part.cs:1325
    isGenerator = moduleGenerator != nullptr;
    decoupleBeforeBurn = false;
    CalculateMassValues();
    CalculateStagingValues();
}

// DeltaVPartInfo.cs:99
int DeltaVPartInfo::CheckDecoupler(int chkStage) {
    ModuleDecouple* moduleDecouple = FindModule<ModuleDecouple>(*part);
    if (moduleDecouple != nullptr) {
        AttachNode* attachNode = moduleDecouple->ExplosiveNode() == nullptr ? part->FindAttachNode(moduleDecouple->Get<std::string>("explosiveNodeID"))
                                                                            : moduleDecouple->ExplosiveNode();
        if (attachNode != nullptr && attachNode->attachedPart != nullptr) jettisonPart = attachNode->attachedPart;
        if (moduleDecouple->Get<bool>("partDecoupled") && part->parent != nullptr && attachNode != nullptr &&
            (attachNode->attachedPart == nullptr ||
             (attachNode->attachedPart != nullptr && attachNode->attachedPart != part->parent && !part->parent->isFairing()))) {
            return part->parent->fields.inverseStage;
        }
        if (part->parent != nullptr && !moduleDecouple->Get<bool>("stagingEnabled")) {
            activationStage = part->parent->fields.separationIndex;
            return part->parent->fields.separationIndex;
        }
    } else {
        ModuleAnchoredDecoupler* moduleAnchored = FindModule<ModuleAnchoredDecoupler>(*part);
        if (moduleAnchored != nullptr) {
            AttachNode* attachNode = moduleAnchored->ExplosiveNode() == nullptr ? part->FindAttachNode(moduleAnchored->Get<std::string>("explosiveNodeID"))
                                                                                : moduleAnchored->ExplosiveNode();
            if (attachNode != nullptr && attachNode->attachedPart != nullptr) jettisonPart = attachNode->attachedPart;
            if (moduleAnchored->Get<bool>("partDecoupled") && part->parent != nullptr && attachNode != nullptr && attachNode->attachedPart != nullptr &&
                attachNode->attachedPart != part->parent && !part->parent->isFairing()) {
                return part->parent->fields.inverseStage;
            }
            if (part->parent != nullptr && !moduleAnchored->Get<bool>("stagingEnabled")) {
                activationStage = part->parent->fields.separationIndex;
                return part->parent->fields.separationIndex;
            }
        }
    }
    return chkStage;
}

// DeltaVPartInfo.cs:159
int DeltaVPartInfo::GetSeparationStage(int chkStage) const {
    const std::vector<int>& idx = vesselDeltaV->SeparationStageIndexes();
    if (idx.empty()) return -1;
    int num = -1;
    for (size_t i = 0; i < idx.size(); i++)
        if (idx[i] > num && idx[i] <= chkStage) num = dotnet::Math::Max(num, idx[i]);
    return num;
}

// DeltaVPartInfo.cs:176
void DeltaVPartInfo::StageStartFuelMass(int stage) {
    PartStageFuelMass* p = nullptr;
    auto it = stageFuelMass.find(stage);
    if (it != stageFuelMass.end()) {
        p = it->second.get();
    } else {
        auto n = std::make_unique<PartStageFuelMass>(*part);
        p = n.get();
        stageFuelMass.emplace(stage, std::move(n));
    }
    p->startMass = GetCurrentFuelMass();
}

// DeltaVPartInfo.cs:191
void DeltaVPartInfo::StageEndFuelMass(int stage) {
    auto it = stageFuelMass.find(stage);
    if (it != stageFuelMass.end()) {
        it->second->endMass = GetCurrentFuelMass();
    } else {
        PartStageFuelMass tmp(*part);  // a throw-away record, as in C#
        tmp.endMass = GetCurrentFuelMass();
    }
}

// DeltaVPartInfo.cs:198
void DeltaVPartInfo::CalculateMassValues() {
    dryMass = 0.0f;
    fuelMass = 0.0f;
    jettisonMass = 0.0f;
    float num = part->fields.mass;
    for (size_t i = 0; i < part->Modules().Count(); i++) {
        IPartMassModifier* mm = part->Modules().GetModule(i)->AsMassModifier();
        if (mm != nullptr && mm->GetModuleMassChangeWhen() == ModifierChangeWhen::STAGED) {
            const float moduleMass = mm->GetModuleMass(num, ModifierStagingSituation::UNSTAGED);
            jettisonMass = f32store(fx(jettisonMass) + fx(moduleMass));
            isJettison = true;
        }
    }
    float num2 = 0.0f;
    PartResourceList& simulationResources = part->SimulationResources();
    int count = simulationResources.Count();
    while (count-- > 0) {
        PartResource& partResource = simulationResources[static_cast<size_t>(count)];
        const PartResourceDefinition* info = partResource.info;
        num2 = f32store(fx(num2) + fx(info->density()) * fx(f32store(partResource.amount)));
    }
    dryMass = f32store(fx(dryMass) + fx(num));
    fuelMass = f32store(fx(fuelMass) + fx(num2));
}

// DeltaVPartInfo.cs:231
void DeltaVPartInfo::CalculateStagingValues() {
    activationStage = part->fields.inverseStage;
    if (isEngine) {
        DeltaVEngineInfo* e = deltav_ext::Get(vesselDeltaV->WorkingEngineInfo(), part);
        if (e != nullptr) activationStage = dotnet::Math::Max(activationStage, e->startBurnStage);
    }
    int num = part->fields.separationIndex;
    if (num > activationStage) num = activationStage;
    if (isDecoupler) {
        num = CheckDecoupler(num);
        const std::vector<int>& idx = vesselDeltaV->SeparationStageIndexes();
        if (std::find(idx.begin(), idx.end(), num) == idx.end()) vesselDeltaV->ResetSeparationIndexes();
    }
    decoupleStage = GetSeparationStage(num);
    if (activationStage < decoupleStage) decoupleStage = activationStage;
    if (isEngine && activationStage == 0 && decoupleStage == activationStage) {
        Part* decouplerParent = nullptr;
        bool flag = false;
        if (FindDecouplerParent(part, decouplerParent)) {
            DeltaVPartInfo* dp = deltav_ext::Get(vesselDeltaV->PartInfo(), decouplerParent);
            if (dp != nullptr && dp->activationStage == 0) flag = true;
        }
        if (!flag) decoupleStage = -1;
    }
    if (isEngine) {
        if (activationStage <= decoupleStage) decoupleBeforeBurn = true;
        else decoupleBeforeBurn = false;
    }
}

// DeltaVPartInfo.cs:290
bool DeltaVPartInfo::FindDecouplerParent(Part* inPart, Part*& decouplerParent) const {
    decouplerParent = nullptr;
    if (inPart->parent != nullptr) {
        if (FindModule<ModuleDecouple>(*inPart->parent) != nullptr) {
            decouplerParent = inPart->parent;
            return true;
        }
        return FindDecouplerParent(inPart->parent, decouplerParent);
    }
    return false;
}

// DeltaVPartInfo.cs:305
void DeltaVPartInfo::ResetStartSimulationResources(int stage) {
    auto it = stageFuelMass.find(stage);
    if (it != stageFuelMass.end()) part->ResetSimulationResources(&it->second->resources);
    else part->ResetSimulationResources();
}

// DeltaVPartInfo.cs:317
bool DeltaVPartInfo::JettisonInStage(int stage) const { return isJettison && decoupleStage < stage && activationStage >= stage; }

// DeltaVPartInfo.cs:326
float DeltaVPartInfo::GetCurrentFuelMass() const {
    float num = 0.0f;
    PartResourceList& simulationResources = part->SimulationResources();
    int count = simulationResources.Count();
    while (count-- > 0) {
        PartResource& partResource = simulationResources[static_cast<size_t>(count)];
        const PartResourceDefinition* info = partResource.info;
        num = f32store(fx(num) + fx(info->density()) * fx(f32store(partResource.amount)));
    }
    return num;
}

// DeltaVPartInfo.cs:340
float DeltaVPartInfo::GetStageStartMass(int stage) const {
    auto it = stageFuelMass.find(stage);
    if (it != stageFuelMass.end()) return it->second->startMass;
    return fuelMass;
}

// DeltaVPartInfo.cs:349
float DeltaVPartInfo::GetStageEndMass(int stage) const {
    auto it = stageFuelMass.find(stage);
    if (it != stageFuelMass.end()) return it->second->endMass;
    return fuelMass;
}

}  // namespace ksp
