// WP-33: StoredPart.cs, ModuleCargoPart.cs, KerbalInventoryScenario.cs, the ProtoVessel node builders the inventory code calls, and the
// InventoryEnvironment seams. ModuleInventoryPart is in module_inventory_part.cpp.
#include "ksp/inventory/inventory.hpp"

#include <algorithm>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/config/kspparseable.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/inventory/construction.hpp"
#include "ksp/kerbals/crew_manifest.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/parts/compound_part.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace {
bool SceneIs(GameScenes s) { return SaveEnvironment::Current().loadedScene == s; }
bool IsFlight() { return SceneIs(GameScenes::FLIGHT); }
}  // namespace

// ================================================================ InventoryEnvironment

void InventoryEnvironment::Reset() {
    EVA_INVENTORY_RANGE = 5.0f;
    EVA_CONSTRUCTION_RANGE = 7.0f;
    EVA_CONSTRUCTION_COMBINE_ENABLED = true;
    EVA_CONSTRUCTION_COMBINE_NONENGINEERS = true;
    EVA_CONSTRUCTION_COMBINE_RANGE = 7.0f;
    VAB_USE_ANGLE_SNAP = false;
    VAB_FINE_OFFSET_THRESHOLD = 20.0f;
    MODIFIER_KEY_DOWN = false;
    IgnoreEVAConstructionMassLimit = false;
    IgnoreKerbalInventoryLimits = false;
    partModelPurchased = nullptr;
    missionMode = false;
    scheduler = nullptr;
    postScreenMessage = nullptr;
    screenMessages.clear();
    highlightPart = nullptr;
    destroyPartObject = nullptr;
    raycast = nullptr;
    getBoundsPoints = nullptr;
    getPartRendererBound = nullptr;
    placementPartCreated = nullptr;
    partMouseOver = nullptr;
    addVessel = nullptr;
    newGuidN = nullptr;
    randomRotation = nullptr;
    uniquePersistentId = nullptr;
    checkPartPersistentId = nullptr;
    universalTime = nullptr;
    log.clear();
    weldSparkClipCount = 0;
    bodyPosition = nullptr;
    bodyRotation = nullptr;
    currentCollisionCount = nullptr;
    setPartPlacementMode = nullptr;
    jetpackDeployed = nullptr;
    toggleJetpack = nullptr;
    editorLastEventName = nullptr;
    uniqueFlightID = nullptr;
    persistentLoadedParts = nullptr;
    constructionOriginalRotation = nullptr;
    alwaysShowInventory = false;
    gameObjectActive = true;
    MODIFIER_KEY_EDGE = false;
    randomRange = nullptr;
    playWeldSparks = nullptr;
    stopWeldSparks = nullptr;
    weldStartDelay = nullptr;
    releasePartRigidbody = nullptr;
    activeVesselPosition = nullptr;
    ModuleCargoPart::isExpansionInstalled = nullptr;
    KerbalInventoryScenario::findCrew = nullptr;
}

double InventoryEnvironment::UT() { return universalTime ? universalTime() : Planetarium::GetUniversalTime(); }

void InventoryEnvironment::Post(const std::string& text, float duration) {
    screenMessages.push_back(text);
    if (postScreenMessage) postScreenMessage(text, duration);
}

namespace {
// PartLoader.cs:2114 GetUnlockedCargoPartNames: the loader's cargoParts / cargoInventoryParts lists are filled in PartLoader.cs:961-968
// in load order with `part.FindModuleImplementing<T>() != null`
std::vector<std::string> UnlockedNames(const char* moduleClass) {
    std::vector<std::string> list;
    PartLoader* loader = PartLoader::Instance();
    if (loader == nullptr) return list;
    const bool flag = InventoryEnvironment::missionMode;  // HighLogic.CurrentGame != null && Mode == MISSION
    for (const auto& ap : loader->loadedParts) {
        if (!ap->partPrefab || ap->partPrefab->FindModuleOfClass(moduleClass) == nullptr) continue;
        if (flag || !InventoryEnvironment::partModelPurchased || InventoryEnvironment::partModelPurchased(*ap)) list.push_back(ap->name);
    }
    return list;
}
}  // namespace

// PartLoader.cs:2152
std::vector<std::string> InventoryEnvironment::GetAvailableCargoPartNames() { return UnlockedNames("ModuleCargoPart"); }
// PartLoader.cs:2157
std::vector<std::string> InventoryEnvironment::GetAvailableCargoInventoryPartNames() { return UnlockedNames("ModuleInventoryPart"); }
// PartLoader.cs:67 (cargoParts.Count > 0)
bool InventoryEnvironment::CargoPartsLoaded() {
    PartLoader* loader = PartLoader::Instance();
    if (loader == nullptr) return false;
    for (const auto& ap : loader->loadedParts)
        if (ap->partPrefab && ap->partPrefab->FindModuleOfClass("ModuleCargoPart") != nullptr) return true;
    return false;
}

// ================================================================ StoredPart

// StoredPart.cs:18
bool StoredPart::IsEmpty() const { return snapshot != nullptr ? quantity < 1 : false; }
// StoredPart.cs:30
bool StoredPart::IsFull() const { return quantity < stackCapacity; }
// StoredPart.cs:32
bool StoredPart::CanStack() const { return snapshot != nullptr ? stackCapacity > 1 : false; }

// StoredPart.cs:44
StoredPart::StoredPart(std::string partName_, int slotIndex_) : slotIndex(slotIndex_), partName(std::move(partName_)) {}

// StoredPart.cs:50
StoredPart::StoredPart(const ConfigNode& node) { Load(node); }

// StoredPart.cs:55
void StoredPart::Load(const ConfigNode& node) {
    node.TryGetValue("slotIndex", slotIndex);
    node.TryGetValue("partName", partName);
    node.TryGetValue("quantity", quantity);
    node.TryGetValue("stackCapacity", stackCapacity);
    node.TryGetValue("variantName", variantName);
    if (node.HasNode("PART")) snapshot = std::make_shared<ProtoPartSnapshot>(*node.GetNode("PART"), nullptr, nullptr);
}

// StoredPart.cs:68
void StoredPart::Save(ConfigNode& node) const {
    node.AddValue("slotIndex", slotIndex);
    node.AddValue("partName", partName);
    node.AddValue("quantity", quantity);
    node.AddValue("stackCapacity", stackCapacity);
    node.AddValue("variantName", variantName);
    if (snapshot != nullptr) snapshot->Save(*node.AddNode("PART"));
}

// StoredPart.cs:81
std::shared_ptr<StoredPart> StoredPart::Copy() const {
    auto storedPart = std::make_shared<StoredPart>(partName, slotIndex);
    storedPart->variantName = variantName;
    storedPart->stackCapacity = stackCapacity;
    storedPart->quantity = quantity;
    if (snapshot != nullptr) {
        ConfigNode node;
        snapshot->Save(node);
        storedPart->snapshot = std::make_shared<ProtoPartSnapshot>(node, snapshot->pVesselRef, nullptr);
    }
    return storedPart;
}

// ================================================================ ProtoVessel node builders

// ProtoVessel.cs:853 (additionalNodes: none of the inventory callers pass any; ACTIONGROUPS is added when absent)
ConfigNode::NodePtr ProtoVesselNodes::CreateVesselNode(const std::string& vesselName, VesselType vesselType, const Orbit& orbit,
                                                       int rootPartIndex, const std::vector<ConfigNode::NodePtr>& partNodes,
                                                       const std::vector<CelestialBody*>& bodies) {
    const Vector3d positionAtUT = const_cast<Orbit&>(orbit).getPositionAtUT(InventoryEnvironment::UT());
    double lat = 0.0, lon = 0.0, alt = 0.0;
    orbit.referenceBody->GetLatLonAlt(positionAtUT, lat, lon, alt);
    const uint32_t uniquepersistentId = InventoryEnvironment::uniquePersistentId ? InventoryEnvironment::uniquePersistentId() : 0u;
    auto configNode = ConfigNode::Create();
    configNode->AddValue("pid", InventoryEnvironment::newGuidN ? InventoryEnvironment::newGuidN() : std::string(32, '0'));
    configNode->AddValue("name", vesselName);
    configNode->AddValue("type", dotnet::EnumToString(vesselType));
    configNode->AddValue("persistentId", uniquepersistentId);
    configNode->AddValue("sit", std::string("ORBITING"));  // Vessel.Situations.ORBITING.ToString()
    configNode->AddValue("landed", false);
    configNode->AddValue("skipGroundPositioning", false);
    configNode->AddValue("vesselSpawning", true);
    configNode->AddValue("landedAt", "");
    configNode->AddValue("splashed", false);
    configNode->AddValue("alt", alt);
    configNode->AddValue("lat", lat);
    configNode->AddValue("lon", lon);
    configNode->AddValue("hgt", -1.0f);
    configNode->AddValue("nrm", KSPUtil::WriteVector(Vector3f::up()));
    configNode->AddValue("rot", KSPUtil::WriteQuaternion(InventoryEnvironment::randomRotation ? InventoryEnvironment::randomRotation()
                                                                                              : Quaternionf::identity()));
    configNode->AddValue("met", InventoryEnvironment::UT());
    configNode->AddValue("lct", InventoryEnvironment::UT());
    configNode->AddValue("root", rootPartIndex);
    configNode->AddValue("PQSMin", 0);
    configNode->AddValue("PQSMax", 0);
    configNode->AddValue("altDispState", std::string("DEFAULT"));
    configNode->AddNode(ProtoVessel::CreateOrbitNode(orbit, bodies));
    for (const auto& p : partNodes) configNode->AddNode(p);
    if (!configNode->HasNode("ACTIONGROUPS")) configNode->AddNode("ACTIONGROUPS");
    return configNode;
}

// ProtoVessel.cs:907 (internal CreatePartNode(Part part, ...)); part == nullptr is the prefab path of the public overload
ConfigNode::NodePtr ProtoVesselNodes::CreatePartNode(Part* part, const std::string& partName, uint32_t id,
                                                      const std::vector<std::shared_ptr<ProtoCrewMember>>& crew) {
    auto configNode = ConfigNode::Create("PART");
    Part* part2 = part;
    const AvailablePart* partInfoByName = SaveEnvironment::Current().getPartInfoByName
                                              ? SaveEnvironment::Current().getPartInfoByName(KSPUtil::SanitizeInstanceName(partName))
                                              : nullptr;
    if (part2 == nullptr) {
        if (partInfoByName == nullptr) throw std::runtime_error("[VesselSpawner Error]: No part in loader found with name: " + partName + ".");
        part2 = partInfoByName->partPrefab.get();
    }
    const uint32_t uniquepersistentId = InventoryEnvironment::uniquePersistentId ? InventoryEnvironment::uniquePersistentId() : 0u;
    configNode->AddValue("name", partInfoByName->name);
    configNode->AddValue("uid", id);
    configNode->AddValue("mid", id);
    configNode->AddValue("persistentId", uniquepersistentId);
    configNode->AddValue("parent", 0);
    configNode->AddValue("position", KSPUtil::WriteVector(Vector3f::zero()));
    configNode->AddValue("rotation", KSPUtil::WriteQuaternion(Quaternionf::identity()));
    configNode->AddValue("mirror", KSPUtil::WriteVector(Vector3f::one()));
    configNode->AddValue("srfN", "None, -1");
    configNode->AddValue("connected", true);
    configNode->AddValue("attached", true);
    configNode->AddValue("state", 0);
    configNode->AddValue("temp", part2->fields.temperature);
    configNode->AddValue("mass", part2->fields.mass);
    configNode->AddValue("expt", part2->fields.explosionPotential);
    for (const AttachNode& attachNode : part2->attachNodes) configNode->AddValue("attN", attachNode.id + ", -1");
    for (const auto& c : crew)
        if (c) configNode->AddValue("crew", c->name().value_or(""));
    if (part2->Resources.IsValid()) {
        const int count2 = part2->Resources.Count();
        for (int k = 0; k < count2; k++) ProtoPartResourceSnapshot(part2->Resources[static_cast<size_t>(k)]).Save(*configNode->AddNode("RESOURCE"));
    }
    return configNode;
}

// ================================================================ ModuleCargoPart

Part* ModuleCargoPart::PartOf() const { return dynamic_cast<Part*>(part); }

ModuleCargoPart::~ModuleCargoPart() { OnDestroy(); }

void ModuleCargoPart::OnAwake() {}

// ModuleCargoPart.cs:55
float ModuleCargoPart::MassForWeightTesting() const {
    if (preSettlingPrefabMass > 0.0f) return preSettlingPrefabMass;
    return part->fields.mass;
}

// ModuleCargoPart.cs:67
void ModuleCargoPart::OnDestroy() {
    if (settleCoroutine && InventoryEnvironment::scheduler) InventoryEnvironment::scheduler->StopCoroutine(settleCoroutine);  // StopCoroutine("MakePartSettle")
    settleCoroutine.reset();
    GE::OnEVAConstructionMode.Remove(this, &ModuleCargoPart::OnEVAConstructionMode);
    GE::onPartWillDie.Remove(this, &ModuleCargoPart::OnPartWillDie);
    GE::OnEVAConstructionWeldStart.Remove(this, &ModuleCargoPart::OnWeldStart);
    GE::OnEVAConstructionWeldFinish.Remove(this, &ModuleCargoPart::OnWeldFinish);
    GE::onHideUI.Remove(this, &ModuleCargoPart::HideUI);
    GE::onShowUI.Remove(this, &ModuleCargoPart::ShowUI);
    GE::OnFlightCompoundPartLinked.Remove(this, &ModuleCargoPart::OnCompoundPartLinked);
    GE::OnFlightCompoundPartDetached.Remove(this, &ModuleCargoPart::OnCompoundPartDetached);
}

// ModuleCargoPart.cs:84
void ModuleCargoPart::OnPartUnpack() {
    if (beingAttached()) {
        Set<bool>("beingAttached", false);
    } else {
        Vessel* v = PartOf() ? PartOf()->vessel : nullptr;
        const bool landed = v != nullptr && v->parts.size() == 1 && (v->situation == Vessel::Situations::LANDED || v->situation == Vessel::Situations::PRELAUNCH);
        if (beingSettled() || landed) {
            if (InventoryEnvironment::scheduler)
                settleCoroutine = InventoryEnvironment::scheduler->StartCoroutine(MakePartSettle(false), reinterpret_cast<intptr_t>(this) & 0x7fffffff);
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

// ModuleCargoPart.cs:106 (the FXGroup / AudioSource setup of the welding sparks is the engine's: FX hooks)
void ModuleCargoPart::OnStart(StartState) {
    if (!IsFlight()) return;
    GE::onPartWillDie.Add(this, &ModuleCargoPart::OnPartWillDie);
    linkedCompoundParts.clear();
    GE::OnFlightCompoundPartLinked.Add(this, &ModuleCargoPart::OnCompoundPartLinked);
    GE::OnFlightCompoundPartDetached.Add(this, &ModuleCargoPart::OnCompoundPartDetached);
    GE::OnEVAConstructionWeldStart.Add(this, &ModuleCargoPart::OnWeldStart);
    GE::OnEVAConstructionWeldFinish.Add(this, &ModuleCargoPart::OnWeldFinish);
    GE::onHideUI.Add(this, &ModuleCargoPart::HideUI);
    GE::onShowUI.Add(this, &ModuleCargoPart::ShowUI);
    inConstructionMode = EVAConstructionModeController::Instance != nullptr &&
                         (EVAConstructionModeController::Instance->IsOpen() ||
                          EVAConstructionModeController::Instance->panelMode != EVAConstructionModeController::PanelMode::Construction);
}

// ModuleCargoPart.cs:158
void ModuleCargoPart::OnInitialize() {
    PartModule::OnInitialize();
    // InitializeModules may run more than once per module (assembly, then Vessel.Initialize): one subscription is kept, so the single
    // Remove of OnDestroy leaves no handler on a destroyed module (the C# adds a second delegate here and leaks it)
    GE::OnEVAConstructionMode.Remove(this, &ModuleCargoPart::OnEVAConstructionMode);
    GE::OnEVAConstructionMode.Add(this, &ModuleCargoPart::OnEVAConstructionMode);
}

// ModuleCargoPart.cs:164
std::string ModuleCargoPart::GetInfo() {
    std::string text;
    text = (!(packedVolume() < 0.0f)) ? Localizer::Format("#autoLOC_8002220") : Localizer::Format("#autoLOC_6002641");
    text += "\n\n";
    Part* p = PartOf();
    Part& prefab = *p->partInfo->partPrefab;
    const float modMass = PartGetModuleMass(p->Modules(), prefab.fields.mass);  // base.part.GetModuleMass(prefab.mass)
    text = text + Localizer::Format("#autoLOC_8002186") + " " +
           dotnet::FormatSingleF(f32store(fx(prefab.fields.mass) + fx(modMass)), 3) + " t\\n";
    if (packedVolume() > 0.0f)
        text = text + Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8003414"),
                                                                                 Localizer::Format("<<1>><<2>>", {dotnet::FormatSingleF(packedVolume(), 1), "L"})}) + "\n";
    if (stackableQuantity() > 1)
        text = text + Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8003418"),
                                                                                 dotnet::FormatInt64(stackableQuantity())}) + "\n";
    return text;
}

// ModuleCargoPart.cs:396
std::string ModuleCargoPart::GetModuleDisplayName() { return Localizer::Format("#autoLOC_8002221"); }

// ModuleCargoPart.cs:181 (the Rigidbody constraint reset is the physics engine's: hook)
void ModuleCargoPart::OnPartWillDie(Part* p) {
    if (part != nullptr && p->fields.persistentId == part->fields.persistentId && PartOf()->hasRigidbody) {
        if (InventoryEnvironment::releasePartRigidbody) InventoryEnvironment::releasePartRigidbody(*PartOf());
    }
}

// ModuleCargoPart.cs:190
void ModuleCargoPart::HideUI() {
    UIHidden = true;
    constructionModeHighlight = false;
    SetConstructionHighlight(constructionModeHighlight);
}

// ModuleCargoPart.cs:197
void ModuleCargoPart::ShowUI() { UIHidden = false; }

// ModuleCargoPart.cs:202 (WaitForSeconds / yield null replay on the WP-00 scheduler)
Enumerator ModuleCargoPart::MakePartSettle(bool waitForMotionStop) {
    bool adjustMass = false;
    Part* p = PartOf();
    preSettlingPrefabMass = p->fields.prefabMass;
    Vessel* v = p->vessel;
    if (v != nullptr && v->mainBody() != nullptr) {
        if (v->mainBody()->GeeASL < 0.800000011920929) {
            Set<bool>("beingSettled", true);
            adjustMass = true;
            p->fields.prefabMass = 20.0f;
        } else if (v->mainBody()->GeeASL < 1.100000023841858) {
            Set<bool>("beingSettled", true);
            adjustMass = true;
            p->fields.prefabMass = 4.0f;
        }
    }
    if (kinematicDelay() > 0.0f) co_yield YieldInstruction::WaitForSeconds(kinematicDelay());
    if (waitForMotionStop) {
        const double timeWaitingForSrfVelocity = InventoryEnvironment::UT();
        const float settleDelay = f32store(fx(kinematicDelay()) * 10.0);
        while (!(InventoryEnvironment::UT() - timeWaitingForSrfVelocity > static_cast<double>(settleDelay)) &&
               !(v->srf_velocity.magnitude() < static_cast<double>(placementMaxRivotVelocity())))
            co_yield YieldInstruction::Null();
    }
    if (adjustMass) {
        p->fields.prefabMass = preSettlingPrefabMass;
        preSettlingPrefabMass = 0.0f;
        Set<bool>("beingSettled", false);
    }
}

// ModuleCargoPart.cs:258
void ModuleCargoPart::OnWeldStart(KerbalEVA* kerbalEVA) {
    EVAConstructionModeController* c = EVAConstructionModeController::Instance;
    if (c->evaEditor->LastAttachedPart()->fields.persistentId == part->fields.persistentId && InventoryEnvironment::weldSparkClipCount > 0 &&
        !weldSparksPlaying) {
        const int index = InventoryEnvironment::randomRange ? InventoryEnvironment::randomRange(0, InventoryEnvironment::weldSparkClipCount) : 0;
        weldSparksPlaying = true;
        if (InventoryEnvironment::playWeldSparks)
            InventoryEnvironment::playWeldSparks(*this, index, InventoryEnvironment::weldStartDelay ? InventoryEnvironment::weldStartDelay(kerbalEVA) : 0.0f);
    }
}

// ModuleCargoPart.cs:268
void ModuleCargoPart::OnWeldFinish(KerbalEVA*) {
    EVAConstructionModeController* c = EVAConstructionModeController::Instance;
    if (c->evaEditor->LastAttachedPart()->fields.persistentId == part->fields.persistentId && InventoryEnvironment::weldSparkClipCount > 0 &&
        weldSparksPlaying) {
        weldSparksPlaying = false;
        if (InventoryEnvironment::stopWeldSparks) InventoryEnvironment::stopWeldSparks(*this);
    }
}

// ModuleCargoPart.cs:242
void ModuleCargoPart::OnEVAConstructionMode(bool opened) {
    if (EVAConstructionModeController::Instance != nullptr &&
        EVAConstructionModeController::Instance->panelMode == EVAConstructionModeController::PanelMode::Construction)
        inConstructionMode = opened;
    else
        inConstructionMode = false;
    if (!inConstructionMode) OnUpdateHighlight();
}

// ModuleCargoPart.cs:276
void ModuleCargoPart::SetConstructionHighlight(bool state) {
    if (part == nullptr) return;
    constructionHighlightApplied = state;
    if (setConstructionHighlight) setConstructionHighlight(*PartOf(), state);
}

// ModuleCargoPart.cs:295
void ModuleCargoPart::OnUpdate() {
    if (inConstructionMode) OnUpdateHighlight();
}

// ModuleCargoPart.cs:303
void ModuleCargoPart::OnUpdateHighlight() {
    bool flag = false;
    bool flag2 = false;
    if (!UIHidden && part != nullptr) {
        Part* p = PartOf();
        Vector3f active;
        if (InventoryEnvironment::activeVesselPosition) {
            active = InventoryEnvironment::activeVesselPosition();
        } else {
            if (FlightRegistry::ActiveVessel == nullptr) throw NullReferenceException();  // FlightGlobals.ActiveVessel.vesselTransform
            active = FlightRegistry::ActiveVessel->vesselTransform.position;
        }
        distanceToEngineer = (active - p->partTransform.position).magnitude();
        const bool underLimit = EVAConstructionUtil::IsUnderConstructionWeightLimit(*p);
        if (constructionModeHighlight) {
            if (!inConstructionMode || distanceToEngineer > InventoryEnvironment::EVA_CONSTRUCTION_RANGE || !underLimit ||
                HasLinkedCompoundParts() || !p->children.empty() || !p->PartCanBeDetached()) {
                flag = true;
                flag2 = false;
            }
        } else if (inConstructionMode && distanceToEngineer <= InventoryEnvironment::EVA_CONSTRUCTION_RANGE && underLimit &&
                   !HasLinkedCompoundParts() && p->children.size() <= 0 && p->PartCanBeDetached()) {
            flag = true;
            flag2 = true;
        }
    } else {
        flag = true;
        flag2 = false;
    }
    if (flag) {
        constructionModeHighlight = flag2;
        SetConstructionHighlight(constructionModeHighlight);
    }
}

// ModuleCargoPart.cs:336
std::string ModuleCargoPart::GetTooltip() {
    if (IsNull("inventoryTooltip") || Get<std::string>("inventoryTooltip").empty()) return GetInfo();
    return Localizer::Format(Get<std::string>("inventoryTooltip"));
}

// ModuleCargoPart.cs:345
bool ModuleCargoPart::IsDeployedSciencePart() {
    const bool serenity = isExpansionInstalled ? isExpansionInstalled("Serenity") : true;
    if (!serenity) return false;
    return PartOf()->FindModuleOfClass("ModuleGroundSciencePart") != nullptr;
}

// ModuleCargoPart.cs:354
void ModuleCargoPart::EnableModule() { isEnabled = true; }

// ModuleCargoPart.cs:363 (CompoundPart.target is the part the compound part is linked to)
void ModuleCargoPart::OnCompoundPartLinked(CompoundPart* linkedPart) {
    if (linkedPart == nullptr) return;
    Part* target = linkedPart->target;
    if (target != nullptr && target->fields.persistentId == part->fields.persistentId) {
        if (std::find(linkedCompoundParts.begin(), linkedCompoundParts.end(), linkedPart) == linkedCompoundParts.end())
            linkedCompoundParts.push_back(linkedPart);  // List.AddUnique
    }
}

// ModuleCargoPart.cs:375
void ModuleCargoPart::OnCompoundPartDetached(CompoundPart* linkedPart) {
    if (linkedPart == nullptr) return;
    Part* target = linkedPart->target;
    if (target != nullptr && target->fields.persistentId == part->fields.persistentId) {
        auto it = std::find(linkedCompoundParts.begin(), linkedCompoundParts.end(), linkedPart);
        if (it != linkedCompoundParts.end()) linkedCompoundParts.erase(it);
    }
}

// ModuleCargoPart.cs:387
bool ModuleCargoPart::HasLinkedCompoundParts() const { return !linkedCompoundParts.empty(); }

// ================================================================ KerbalInventoryScenario

KerbalInventoryScenario::KerbalInventoryScenario() : ScenarioModule("KerbalInventoryScenario") {}

KerbalInventoryScenario::~KerbalInventoryScenario() {
    if (Instance == this) OnDestroy();
}

int KerbalInventoryScenario::Find(const std::string& name) const {
    for (size_t i = 0; i < kerbalInventories_.size(); i++)
        if (kerbalInventories_[i].first == name) return static_cast<int>(i);
    return -1;
}

// KerbalInventoryScenario.cs:33
void KerbalInventoryScenario::OnAwake() {
    if (Instance != nullptr && Instance != this) {
        log::Error("[KerbalInventoryScenario]: Instance already exists!");
        // Object.Destroy(Instance): the old scenario component is destroyed at the end of the frame; its OnDestroy runs then
        Instance->OnDestroy();
    }
    Instance = this;
    kerbalInventories_.clear();
    GE::onKerbalNameChanged.Add(this, &KerbalInventoryScenario::OnKerbalNameChanged);
    eventsAdded_ = true;
}

// KerbalInventoryScenario.cs:45
void KerbalInventoryScenario::Update() {
    for (auto& kv : kerbalInventories_) kv.second->Update();
}

// KerbalInventoryScenario.cs:53
void KerbalInventoryScenario::OnSave(ConfigNode&) {
    for (auto& kv : kerbalInventories_) {
        if (kv.second == nullptr) continue;
        std::shared_ptr<ProtoCrewMember> crew;
        if (findCrew) {
            crew = findCrew(kv.first);
        } else {
            Game* g = SaveEnvironment::Current().currentGame;
            if (g != nullptr && g->CrewRoster) crew = (*g->CrewRoster)[kv.first];
        }
        if (crew) crew->SaveInventory(kv.second);
    }
}

// KerbalInventoryScenario.cs:64
void KerbalInventoryScenario::OnDestroy() {
    for (auto& kv : kerbalInventories_) {
        if (kv.second != nullptr) {
            // gameObject.DestroyGameObject(): the module's object goes with it
            auto it = std::find(kerbalInventoryGameObjectChildren.begin(), kerbalInventoryGameObjectChildren.end(), kv.first);
            if (it != kerbalInventoryGameObjectChildren.end()) kerbalInventoryGameObjectChildren.erase(it);
        }
    }
    kerbalInventories_.clear();
    for (int num = static_cast<int>(kerbalInventoryGameObjectChildren.size()) - 1; num >= 0; num--)
        kerbalInventoryGameObjectChildren.erase(kerbalInventoryGameObjectChildren.begin() + num);
    if (eventsAdded_) GE::onKerbalNameChanged.Remove(this, &KerbalInventoryScenario::OnKerbalNameChanged);
    eventsAdded_ = false;
    if (Instance == this) Instance = nullptr;
}

// KerbalInventoryScenario.cs:85
void KerbalInventoryScenario::OnKerbalNameChanged(ProtoCrewMember*, std::string prevName, std::string newName) {
    const int i = Find(prevName);
    if (i >= 0) {
        ModuleInventoryPart* value = kerbalInventories_[static_cast<size_t>(i)].second;
        kerbalInventories_.erase(kerbalInventories_.begin() + i);
        if (Find(newName) >= 0) throw std::runtime_error("An item with the same key has already been added.");  // Dictionary.Add
        kerbalInventories_.emplace_back(newName, value);
    }
}

// KerbalInventoryScenario.cs:95
void KerbalInventoryScenario::AddKerbalInventoryInstance(const std::string& crewName, ModuleInventoryPart* inventory) {
    const int i = Find(crewName);
    if (i >= 0) {
        log::Warning("[KerbalInventoryScenario]: Duplicate Inventory Instance, replaced old instance for " + crewName);
        kerbalInventories_[static_cast<size_t>(i)].second = inventory;
    } else {
        kerbalInventories_.emplace_back(crewName, inventory);
        kerbalInventoryGameObjectChildren.push_back(crewName);  // SetParent(KerbalInventoryGameObject) + name = crewName
    }
}

// KerbalInventoryScenario.cs:110
bool KerbalInventoryScenario::ContainsCrew(const std::string& crewName) const { return Find(crewName) >= 0; }

// KerbalInventoryScenario.cs:115
void KerbalInventoryScenario::RemoveKerbalInventoryInstance(const std::string& crewName) {
    const int i = Find(crewName);
    if (i < 0) return;
    kerbalInventories_.erase(kerbalInventories_.begin() + i);
    int childCount = static_cast<int>(kerbalInventoryGameObjectChildren.size());
    while (childCount-- > 0) {
        if (kerbalInventoryGameObjectChildren[static_cast<size_t>(childCount)] == crewName) {
            kerbalInventoryGameObjectChildren.erase(kerbalInventoryGameObjectChildren.begin() + childCount);  // Object.Destroy
            return;
        }
    }
}

void KerbalInventoryScenario::Register() {
    ScenarioModuleFactory::Register("KerbalInventoryScenario", [](const std::string&) { return std::unique_ptr<ScenarioModule>(new KerbalInventoryScenario()); });
}

void RegisterInventoryScenarios() { KerbalInventoryScenario::Register(); }

void KerbalInventoryScenario::ClearStatics() {
    Instance = nullptr;
    kerbalInventoryGameObjectChildren.clear();
    findCrew = nullptr;
}

}  // namespace ksp
