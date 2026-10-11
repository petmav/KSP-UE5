// WP-33: ModuleInventoryPart.cs (2,002 lines) -- see inventory.hpp for what is ported and what is a seam.
//
// Float rule: every float expression follows Mono's evaluation (double until a store, fx()/f32store()).
#include <algorithm>
#include <cmath>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/editor/editor_logic.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/inventory/construction.hpp"
#include "ksp/inventory/inventory.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/module_part_variants.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace {
bool SceneIs(GameScenes s) { return SaveEnvironment::Current().loadedScene == s; }
bool IsFlight() { return SceneIs(GameScenes::FLIGHT); }
bool IsEditor() { return SceneIs(GameScenes::EDITOR); }

// PartLoader.getPartInfoByName
const AvailablePart* PartInfoByName(const std::string& name) {
    if (PartLoader::Instance() != nullptr) return PartLoader::Instance()->getPartInfoByName(name);
    const SaveEnvironment& env = SaveEnvironment::Current();
    return env.getPartInfoByName ? env.getPartInfoByName(name) : nullptr;
}

bool Contains(const std::vector<std::string>& v, const std::string& s) { return std::find(v.begin(), v.end(), s) != v.end(); }

void Warn(const std::string& msg) {
    log::Warning(msg);
    InventoryEnvironment::log.push_back(msg);
}
void Err(const std::string& msg) {
    log::Error(msg);
    InventoryEnvironment::log.push_back(msg);
}
}  // namespace

Part* ModuleInventoryPart::PartOf() const { return dynamic_cast<Part*>(part); }
Vessel* ModuleInventoryPart::VesselOf() const { return PartOf() ? PartOf()->vessel : nullptr; }

ModuleInventoryPart::~ModuleInventoryPart() { OnDestroy(); }

Vector3f ModuleInventoryPart::TransformPosition() const {
    if (kerbalMode()) return transformPosition;
    return PartOf() != nullptr ? PartOf()->partTransform.position : transformPosition;
}

void ModuleInventoryPart::OnAwake() {}

std::string ModuleInventoryPart::Inventory() const { return IsNull("Inventory") ? std::string() : Get<std::string>("Inventory"); }
void ModuleInventoryPart::SetInventory(const std::string& v) { Set<std::string>("Inventory", v); }

// ModuleInventoryPart.cs:154 (obsolete property)
std::vector<std::string> ModuleInventoryPart::InventoryPartsList() {
    std::vector<std::string> list;
    for (int i = 0; i < InventorySlots(); i++) {
        std::shared_ptr<StoredPart> val;
        if (storedParts.TryGetValue(i, val)) list.push_back(val->partName);
        else list.push_back("");
    }
    return list;
}

// ModuleInventoryPart.cs:175
bool ModuleInventoryPart::InventoryIsEmpty() const { return !storedPartsNull ? storedParts.Count() == 0 : true; }
// ModuleInventoryPart.cs:187
bool ModuleInventoryPart::InventoryIsFull() const { return !storedPartsNull ? storedParts.Count() >= InventorySlots() : false; }
// ModuleInventoryPart.cs:199
int ModuleInventoryPart::InventoryItemCount() const { return !storedPartsNull ? storedParts.Count() : 0; }

// ModuleInventoryPart.cs:211
bool ModuleInventoryPart::AbleToPlaceParts() const {
    Part* p = PartOf();
    if (p != nullptr && p->vessel != nullptr && p->vessel->isEVA() && p->vessel->Landed && p->vessel->isActiveVessel()) return selectedPart == nullptr;
    return false;
}

// ModuleInventoryPart.cs:223
bool ModuleInventoryPart::IsKerbalOnEVA() const {
    Part* p = PartOf();
    if (p != nullptr && p->vessel != nullptr) return p->vessel->isEVA();
    return false;
}

// ModuleInventoryPart.cs:235
bool ModuleInventoryPart::kerbalMode() const {
    if (kerbalReference != nullptr) return kerbalReference->name().value_or("") != "";  // kerbalReference.name != "" (a null name differs from "")
    return false;
}

// ModuleInventoryPart.cs:254
void ModuleInventoryPart::OnEditorPartEvent(ConstructionEventType evt, Part* p) {
    switch (evt) {
        case ConstructionEventType::PartDropped: PartDroppedOnInventory(evt, p); break;
        case ConstructionEventType::PartAttached: PartDroppedOnInventory(evt, p); break;
        case ConstructionEventType::PartDeleted: PartDroppedOnInventory(evt, p); break;
        case ConstructionEventType::PartDetached:
        case ConstructionEventType::PartCopied:
            if (p != nullptr && p->isCargoPart() && UIPartActionControllerInventory::Instance != nullptr)
                UIPartActionControllerInventory::Instance->SetCurrentCargoPart(p);
            break;
        case ConstructionEventType::PartPicked:
        case ConstructionEventType::PartDragging:
        default: break;
    }
}

// ModuleInventoryPart.cs:280
void ModuleInventoryPart::VesselEditorPartHighlighter(Part* p) {
    bool flag;
    Part* self = PartOf();
    if (p == nullptr) {
        highlightLog.push_back("color:default");  // base.part.highlightColor = Highlighter.colorPartHighlightDefault
        flag = false;
    } else {
        if (!Contains(availableCargoParts, p->gameObjectName)) return;
        UIPartActionControllerInventory* c = UIPartActionControllerInventory::Instance;
        if (c != nullptr && c->CurrentCargoPart() != nullptr) {
            const AttachRules r = c->CurrentCargoPart()->fields.attachRules.value_or(AttachRules{});
            if (!r.srfAttach && !r.allowStack) {
                highlightLog.push_back("color:inventoryContainer");
                flag = true;
            } else {
                flag = false;
            }
        } else {
            flag = false;
        }
    }
    highlightLog.push_back(flag ? "highlight:on" : "highlight:off");
    if (InventoryEnvironment::highlightPart) InventoryEnvironment::highlightPart(*self, flag);
    partHighlightActive = flag;
}

// ModuleInventoryPart.cs:308
void ModuleInventoryPart::PartDroppedOnInventory(ConstructionEventType evt, Part* p) {
    if (Contains(availableCargoInventoryParts, p->gameObjectName) || !Contains(availableCargoParts, p->gameObjectName)) return;
    if (partHighlightActive) {
        highlightLog.push_back("color:default");
        highlightLog.push_back("highlight:off");
        if (InventoryEnvironment::highlightPart) InventoryEnvironment::highlightPart(*PartOf(), false);
    }
    auto* moduleCargoPart = dynamic_cast<ModuleCargoPart*>(p->FindModuleOfClass("ModuleCargoPart"));
    const bool mouseOver = InventoryEnvironment::partMouseOver ? InventoryEnvironment::partMouseOver(*this) : false;
    if (moduleCargoPart == nullptr || evt != ConstructionEventType::PartDropped || !mouseOver) return;
    const bool ignore = IgnoreKerbalInventoryLimits();
    if (!ignore && HasPackedVolumeLimit() &&
        (moduleCargoPart->packedVolume() < 0.0f || fx(moduleCargoPart->packedVolume()) + fx(volumeOccupied) > fx(packedVolumeLimit())))
        return;
    if (!IgnoreKerbalInventoryLimits() && HasMassLimit() && fx(massOccupied) + fx(GetPartMass(*p)) > fx(massLimit())) return;
    bool flag = false;
    bool flag2 = false;
    if (moduleCargoPart->stackableQuantity() > 1) {
        const std::string partVariantName = (p->variants == nullptr) ? "" : p->variants->SelectedVariant()->name;
        for (int i = 0; i < storedParts.Count(); i++) {
            const int slotIndex = storedParts.KeysList()[static_cast<size_t>(i)];
            if (CanStackInSlot(*p->partInfo, partVariantName, slotIndex)) {
                const int stackAmountAtSlot = GetStackAmountAtSlot(slotIndex);
                UpdateStackAmountAtSlot(slotIndex, stackAmountAtSlot + 1);
                flag = true;
                break;
            }
        }
    }
    if (!flag && TotalEmptySlots() > 0) {
        const int num = FirstEmptySlot();
        if (StoreCargoPartAtSlot(p, num)) {
            flag2 = true;
            if (pawInventory != nullptr && pawInventoryActive) pawInventory->SpawnLoadedPartAtSlot(num);
        }
    }
    const std::string last = InventoryEnvironment::editorLastEventName ? InventoryEnvironment::editorLastEventName() : "";
    std::string lower = last;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if ((!IsEditor() || lower.find("root") == std::string::npos) && (flag || flag2)) {
        if (InventoryEnvironment::destroyPartObject) InventoryEnvironment::destroyPartObject(*p);  // Object.Destroy(p.gameObject)
    }
}

// ModuleInventoryPart.cs:386
int ModuleInventoryPart::GetStackAmountAtSlot(int slotIndex) {
    std::shared_ptr<StoredPart> v;
    if (!storedParts.TryGetValue(slotIndex, v)) return 0;
    return v->quantity;
}

// ModuleInventoryPart.cs:395
int ModuleInventoryPart::GetStackCapacityAtSlot(int slotIndex) {
    std::shared_ptr<StoredPart> v;
    if (!storedParts.TryGetValue(slotIndex, v)) return 0;
    return v->stackCapacity;
}

// ModuleInventoryPart.cs:404
bool ModuleInventoryPart::IsStackable(int slotIndex) {
    std::shared_ptr<StoredPart> v;
    if (!storedParts.TryGetValue(slotIndex, v)) return false;
    return v->stackCapacity > 1;
}

// ModuleInventoryPart.cs:413
bool ModuleInventoryPart::CanStackInSlot(const AvailablePart& partInfo, const std::string& partVariantName, int slotIndex) {
    std::shared_ptr<StoredPart> val;
    if (storedParts.TryGetValue(slotIndex, val) && val->partName == partInfo.name) {
        if (partVariantName == "") return HasStackingSpace(slotIndex);
        if (partVariantName == val->variantName) return HasStackingSpace(slotIndex);
    }
    return false;
}

// ModuleInventoryPart.cs:429
bool ModuleInventoryPart::UpdateStackAmountAtSlot(int slotIndex, int howMany, const std::string& variantName) {
    std::shared_ptr<StoredPart> val;
    if (!storedParts.TryGetValue(slotIndex, val)) {
        Warn("[ModuleInventoryPart]: Cannot update quantity in slot " + std::to_string(slotIndex) + ". There is no part stored there");
        return false;
    }
    howMany = std::max(1, std::min(howMany, val->stackCapacity));
    if (!variantName.empty() && val->variantName != variantName) {
        Warn("[ModuleInventoryPart]: Cannot update quantity in slot " + std::to_string(slotIndex) + ". There the variant being asked for is not the same");
        return false;
    }
    const bool changed = howMany != val->quantity;
    val->quantity = howMany;
    if (changed) {
        GE::onModuleInventorySlotChanged.Fire(this, slotIndex);
        GE::onModuleInventoryChanged.Fire(this);
    }
    return true;
}

// ModuleInventoryPart.cs:452
std::string ModuleInventoryPart::GetVariantNameForSlot(int slotIndex) {
    std::shared_ptr<StoredPart> v;
    if (!storedParts.TryGetValue(slotIndex, v)) return "";
    return v->variantName;
}

// ModuleInventoryPart.cs:461
bool ModuleInventoryPart::HasStackingSpace(int slotIndex) {
    std::shared_ptr<StoredPart> v;
    if (!storedParts.TryGetValue(slotIndex, v)) return false;
    return v->quantity < v->stackCapacity;
}

// ModuleInventoryPart.cs:470
bool ModuleInventoryPart::IsSlotEmpty(int slotIndex) { return !storedParts.ContainsKey(slotIndex); }

// ModuleInventoryPart.cs:475
void ModuleInventoryPart::DeployInventoryItem(int index) {
    if (AbleToPlaceParts() && InventoryItemCanBeDeployed(index) && !partBeingRetrieved) {
        if (UIPartActionControllerInventory::Instance != nullptr) UIPartActionControllerInventory::Instance->DestroyTooltip();
        placementPendingIndex = index;
        std::shared_ptr<StoredPart> sp;
        storedParts.TryGetValue(index, sp);
        if (InventoryEnvironment::scheduler)
            placementCoroutine_ = InventoryEnvironment::scheduler->StartCoroutine(CreatePartObject(sp), reinterpret_cast<intptr_t>(this) & 0x7fffffff);
    } else if (pawInventory != nullptr) {
        pawInventory->ResetPlacePartIcons(true);
    }
}

// ModuleInventoryPart.cs:489
bool ModuleInventoryPart::InventoryItemCanBeDeployed(int index) {
    std::shared_ptr<StoredPart> val;
    if (storedParts.TryGetValue(index, val) && selectedPart == nullptr && !val->partName.empty()) {
        const AvailablePart* partInfoByName = PartInfoByName(val->partName);
        if (partInfoByName != nullptr && partInfoByName->partPrefab->isGroundDeployable()) return true;
    }
    return false;
}

// ModuleInventoryPart.cs:502 (the colliders, bounds, rigidbody, highlight and PAW refresh of the placement preview are the shell's
// `placementPartCreated` hook; the data steps are ported)
Enumerator ModuleInventoryPart::CreatePartObject(std::shared_ptr<StoredPart> storedPart) {
    partFullyCreated = false;
    if (storedPart == nullptr) co_return;
    Vessel* v = VesselOf();
    if (v != nullptr && v->isEVA() && v->evaController != nullptr) {
        if (InventoryEnvironment::setPartPlacementMode && !InventoryEnvironment::setPartPlacementMode(true, this)) co_return;
        if (InventoryEnvironment::jetpackDeployed && InventoryEnvironment::jetpackDeployed()) {
            if (InventoryEnvironment::toggleJetpack) InventoryEnvironment::toggleJetpack();
        }
    }
    std::unique_ptr<Part> created = storedPart->snapshot->CreatePart();
    Part* part_ = created.get();
    part_->fields.ResumeState = static_cast<int32_t>(PartStates::PLACEMENT);
    part_->set_State(PartStates::PLACEMENT);
    part_->gameObjectName = storedPart->snapshot->partInfo->name;
    part_->fields.persistentId = InventoryEnvironment::checkPartPersistentId ? InventoryEnvironment::checkPartPersistentId(*part_)
                                                                              : part_->fields.persistentId;
    if (!storedPart->variantName.empty() && part_->variants != nullptr) part_->variants->SetVariant(storedPart->variantName);
    selectedPartOwner = std::move(created);
    selectedPart = part_;
    PartModule* groundModule = selectedPart->FindModuleOfClass("ModuleGroundPart");
    if (groundModule != nullptr) {
        placementAllowXRotation = groundModule->Get<bool>("placementAllowXRotation");
        placementAllowYRotation = groundModule->Get<bool>("placementAllowYRotation");
        placementAllowZRotation = groundModule->Get<bool>("placementAllowZRotation");
    } else {
        placementAllowXRotation = true;
        placementAllowYRotation = true;
        placementAllowZRotation = true;
    }
    Part* self = PartOf();
    selectedPart->partTransform.rotation = self ? self->partTransform.rotation : selectedPart->fields.initRotation;
    if (InventoryEnvironment::getPartRendererBound) selectedPartBounds = InventoryEnvironment::getPartRendererBound(*selectedPart);
    selectedPart->hasRigidbody = false;  // Object.Destroy(selectedPart.rb)
    selectedPart->fields.physicalSignificance = static_cast<int32_t>(Part_PhysicalSignificance::NONE);
    selectedPart->partInfo = storedPart->snapshot->partInfo;
    spawnDistance = f32store(fx(Mathf::Max(Mathf::Max(selectedPartBounds.extents().x, selectedPartBounds.extents().y), selectedPartBounds.extents().z)) + fx(0.2f));
    selectedPart->partTransform.position = v->vesselTransform.position + v->vesselTransform.forward() * spawnDistance;
    SetPositionFromTerrain();
    selectedPart->fields.attRotation = Quaternionf::identity();
    selectedPart->fields.attRotation0 = selectedPart->partTransform.rotation;  // transform.localRotation (parentless here)
    selectedPart->fields.attPos0 = selectedPart->partTransform.position;       // transform.localPosition
    co_yield YieldInstruction::Null();  // selectedPart.highlighter.ReinitMaterials(); yield return null
    co_yield YieldInstruction::Null();
    placementPositionOk = true;
    if (InventoryEnvironment::placementPartCreated) InventoryEnvironment::placementPartCreated(*selectedPart);
    partFullyCreated = true;
}

// ModuleInventoryPart.cs:598
void ModuleInventoryPart::SetPositionFromTerrain() {
    const float num = 50.0f;
    Vessel* v = VesselOf();
    placementStartingPosition = v->vesselTransform.position + v->vesselTransform.forward() * spawnDistance;
    Vector3f vector = placementStartingPosition;
    const Vector3d upAxisD = EVAConstructionEnvironment::upAxisAt ? EVAConstructionEnvironment::upAxisAt(Vector3d(placementStartingPosition))
                                                                  : FlightMath::getUpAxis(*v->mainBody(), Vector3d(placementStartingPosition));
    const Vector3d bodyPos = InventoryEnvironment::bodyPosition ? InventoryEnvironment::bodyPosition() : v->mainBody()->position;
    const float maxDistance = static_cast<float>((Vector3d(vector) - bodyPos).magnitude());
    const Vector3f upAxis = upAxisD.ToVector3f();
    // vector += upAxis * 50.0 (Vector3d arithmetic, then the implicit Vector3d -> Vector3 narrowing of the sum)
    vector = (Vector3d(vector) + upAxisD * 50.0).ToVector3f();
    selectedPart->partTransform.position = (upAxisD * 100.0 + Vector3d(placementStartingPosition)).ToVector3f();
    PlacementRayHit hit;
    if (InventoryEnvironment::raycast && InventoryEnvironment::raycast(vector, -upAxis, maxDistance, hit)) {
        placementonTerrain = hit.onTerrainLayer && !hit.isROC;
        if (v != nullptr && v->Splashed) placementonTerrain = false;
        float num2 = f32store(fx(hit.distance) - fx(num));
        float centerPointOffset = 0.0f;
        float boundsPoints = InventoryEnvironment::getBoundsPoints ? InventoryEnvironment::getBoundsPoints(*selectedPart, hit.normal, centerPointOffset) : 0.0f;
        boundsPoints = f32store(fx(boundsPoints) + fx(placementGroundOffset()));
        const float angle = static_cast<float>(std::fabs(Vector3d::Angle(Vector3d(hit.normal), upAxisD)));  // Mathf.Abs((float)Vector3d.Angle(...))
        const float num3 = f32store(fx(Mathf::Cos(f32store(fx(angle) * fx(Mathf::Deg2Rad)))) * fx(boundsPoints));
        if (Mathf::Abs(f32store(fx(boundsPoints) - fx(num3))) > 0.1f) boundsPoints = num3;
        if (Mathf::Abs(num2) > placementGroundOffsetCap()) {
            placementInsideCap = false;
            num2 = Mathf::Clamp(num2, f32store(0.0 - fx(placementGroundOffsetCap()) + fx(centerPointOffset)),
                                f32store(fx(placementGroundOffsetCap()) + fx(centerPointOffset)));
        } else {
            placementInsideCap = true;
        }
        if (num2 > boundsPoints) {
            boundsPoints = f32store(fx(num2) - fx(boundsPoints));
            selectedPart->partTransform.position = (Vector3d(placementStartingPosition) + upAxisD * static_cast<double>(f32store(0.0 - fx(boundsPoints)))).ToVector3f();
        } else {
            boundsPoints = f32store(fx(boundsPoints) - fx(num2));
            selectedPart->partTransform.position = (Vector3d(placementStartingPosition) + upAxisD * static_cast<double>(boundsPoints)).ToVector3f();
        }
    } else {
        selectedPart->partTransform.position = placementStartingPosition;
        placementonTerrain = false;
    }
}

// ModuleInventoryPart.cs:672
void ModuleInventoryPart::DeletePartObject() {
    if (selectedPart != nullptr) {
        selectedPart->OnDelete();
        selectedPartOwner.reset();  // Object.Destroy(selectedPart.gameObject)
        selectedPart = nullptr;
    }
    for (Part* part_ : InventoryEnvironment::persistentLoadedParts ? InventoryEnvironment::persistentLoadedParts() : std::vector<Part*>{}) {
        if (part_ == nullptr) continue;
        // partInfo.category == PartCategories.Cargo: the highlight reset skips the parts still being placed
        const bool cargoCategory = part_->partInfo != nullptr && part_->partInfo->category == 15;
        if (cargoCategory) {
            if (part_->State() != PartStates::PLACEMENT) highlightLog.push_back("reset:" + part_->gameObjectName);
        } else {
            highlightLog.push_back("reset:" + part_->gameObjectName);
        }
    }
}

// ModuleInventoryPart.cs:708
bool ModuleInventoryPart::StoreCargoPartAtSlot(const std::string& partName, bool, int index) {
    if (!CheckPartStorage(partName, index)) {
        Warn("[ModuleInventoryPart]: Unable to store " + partName + " into " + PartOf()->partInfo->title + "-" +
             std::to_string(PartOf()->fields.persistentId) + " as there are no empty slots available.");
        return false;
    }
    if (index == -1) index = FirstEmptySlot();
    return StoreCargoPartAtSlot(partName, index);
}

// ModuleInventoryPart.cs:722
bool ModuleInventoryPart::StoreCargoPartAtSlot(const std::string& partName, int slotIndex) {
    const AvailablePart* partInfoByName = PartInfoByName(partName);
    if (partInfoByName != nullptr) return StoreCargoPartAtSlot(partInfoByName->partPrefab.get(), slotIndex);
    Err("[ModuleInventoryPart]: Unable to store part at index " + std::to_string(slotIndex) + ". Unable find part with name=" + partName);
    return false;
}

// ModuleInventoryPart.cs:733
bool ModuleInventoryPart::StoreCargoPartAtSlot(Part* sPart, int slotIndex) {
    std::shared_ptr<ProtoPartSnapshot> protoPartSnapshot;
    if (sPart != nullptr) {
        for (size_t i = 0; i < sPart->Modules().Count(); i++) {
            if (sPart->Modules()[i] != nullptr) sPart->Modules()[i]->OnStoredInInventory(*this);
        }
    }
    Vessel* v = VesselOf();
    if (v != nullptr) {
        protoPartSnapshot = std::make_shared<ProtoPartSnapshot>(*sPart, v->protoVessel);
    } else if (IsEditor() || SceneIs(GameScenes::MISSIONBUILDER) || SceneIs(GameScenes::MAINMENU) || kerbalMode()) {
        protoPartSnapshot = std::make_shared<ProtoPartSnapshot>(*sPart, nullptr);
    }
    if (protoPartSnapshot == nullptr) {
        Err("[ModuleInventoryPart]: Unable to create a snapshot part at index " + std::to_string(slotIndex) + ". Unable part name=" +
            sPart->gameObjectName);
        return false;
    }
    return StoreCargoPartAtSlot(protoPartSnapshot, slotIndex);
}

// ModuleInventoryPart.cs:762
bool ModuleInventoryPart::StoreCargoPartAtSlot(std::shared_ptr<ProtoPartSnapshot> pPart, int slotIndex) {
    bool flag = false;
    if (slotIndex == -1) slotIndex = FirstEmptySlot();
    if (slotIndex == -1) {
        Err("[ModuleInventoryPart]: Unable to store part - there are no free slots. Unable find part with name=" + pPart->partName.value_or(""));
        return false;
    }
    if (storedParts.ContainsKey(slotIndex)) {
        ClearPartAtSlot(slotIndex, true);
        flag = true;
    }
    if (pPart != nullptr && pPart->partInfo != nullptr) {
        RefillEVAPropellantOnStoring(*pPart);
        auto val = std::make_shared<StoredPart>(pPart->partName.value_or(""), slotIndex);
        val->slotIndex = slotIndex;
        val->snapshot = pPart;
        val->variantName = pPart->moduleVariantName;
        val->quantity = 1;
        val->stackCapacity = pPart->moduleCargoStackableQuantity;
        storedParts.Add(slotIndex, val);
        flag = true;
        ResetInventoryPartsByName();
    }
    if (flag) {
        UpdateModuleUI();
        GE::onModuleInventorySlotChanged.Fire(this, slotIndex);
        GE::onModuleInventoryChanged.Fire(this);
    }
    return true;
}

// ModuleInventoryPart.cs:803
void ModuleInventoryPart::RefillEVAPropellantOnStoring(ProtoPartSnapshot& ppSS) { RefillEVAPropellant(ppSS, *PartOf()); }

// ModuleInventoryPart.cs:808
void ModuleInventoryPart::RefillEVAPropellantOnBoarding(Part& p) {
    for (int i = 0; i < storedParts.Count(); i++) {
        const std::shared_ptr<StoredPart>& storedPart = storedParts.ValuesList()[static_cast<size_t>(i)];
        if (storedPart != nullptr && storedPart->snapshot != nullptr) RefillEVAPropellant(*storedPart->snapshot, p);
    }
}

// ModuleInventoryPart.cs:820
void ModuleInventoryPart::RefillEVAPropellant(ProtoPartSnapshot& ppSS, Part& p) {
    if (p.isKerbalEVA() || p.isKerbalSeat()) return;
    ProtoPartModuleSnapshot* protoPartModuleSnapshot = ppSS.FindModule("ModuleCargoPart");
    if (protoPartModuleSnapshot == nullptr) return;
    bool value = false;
    if (protoPartModuleSnapshot->moduleValues->TryGetValue("reinitResourcesOnStoreInVessel", value) && value) {
        for (size_t i = 0; i < ppSS.resources.size(); i++) {
            ppSS.resources[i]->amount = ppSS.resources[i]->maxAmount;
            ppSS.resources[i]->UpdateConfigNodeAmounts();
        }
    }
}

// ModuleInventoryPart.cs:842
bool ModuleInventoryPart::ClearPartAtSlot(int slotIndex) { return ClearPartAtSlot(slotIndex, false); }

// ModuleInventoryPart.cs:847
bool ModuleInventoryPart::ClearPartAtSlot(int slotIndex, bool bypassEvent) {
    bool flag = false;
    if (storedParts.ContainsKey(slotIndex)) {
        storedParts.Remove(slotIndex);
        flag = true;
        ResetInventoryPartsByName();
    }
    if (flag && !bypassEvent) {
        UpdateModuleUI();
        GE::onModuleInventorySlotChanged.Fire(this, slotIndex);
        GE::onModuleInventoryChanged.Fire(this);
    }
    return true;
}

// ModuleInventoryPart.cs:865
bool ModuleInventoryPart::CheckPartStorage(const std::string&, int) { return !InventoryIsFull(); }

// ModuleInventoryPart.cs:870
void ModuleInventoryPart::RequestUpdateEngineerReport() {
    engineerReportRequests.push_back("request");
    if (InventoryEnvironment::scheduler == nullptr) return;
    // gameObject.activeInHierarchy -> StartCoroutine on the module, else EditorLogic.fetch.StartCoroutine when the editor has a ship
    if (InventoryEnvironment::gameObjectActive) {
        InventoryEnvironment::scheduler->StartCoroutine(UpdateEngineerReport(), reinterpret_cast<intptr_t>(this) & 0x7fffffff);
    } else if (EditorLogic::fetch != nullptr && EditorLogic::fetch->ship != nullptr) {
        InventoryEnvironment::scheduler->StartCoroutine(UpdateEngineerReport(), 0x7ffffffe);
    }
}

// ModuleInventoryPart.cs:882
Enumerator ModuleInventoryPart::UpdateEngineerReport() {
    co_yield YieldInstruction::Null();
    if (EditorLogic::fetch != nullptr && EditorLogic::fetch->ship != nullptr) GE::onEditorShipModified.Fire(EditorLogic::fetch->ship);
}

// ModuleInventoryPart.cs:891
void ModuleInventoryPart::OnModuleInventoryChanged(ModuleInventoryPart* moduleInventory) {
    if (!(moduleInventory != this)) {
        if (kerbalMode()) kerbalReference->SaveInventory(this);
        RequestUpdateEngineerReport();
        UpdateCapacityValues();
    }
}

// ModuleInventoryPart.cs:904
void ModuleInventoryPart::OnPartPurchased(AvailablePart*) {
    availableCargoParts = InventoryEnvironment::GetAvailableCargoPartNames();
    availableCargoInventoryParts = InventoryEnvironment::GetAvailableCargoInventoryPartNames();
}

// ModuleInventoryPart.cs:910
void ModuleInventoryPart::OnStart(StartState) {
    Set<float>("allowedKerbalEvaDistance", InventoryEnvironment::EVA_INVENTORY_RANGE);
    if (!IsFlight() && !IsEditor()) return;
    IgnoreKerbalInventoryLimits();
    InitializeInventory();
    if (IsFlight()) {
        gridPresent = true;
        Fields()["InventorySlots"]->guiActive = true;
    }
    if (IsEditor()) {
        gridPresent = true;
        GE::onEditorPartEvent.Add(this, &ModuleInventoryPart::OnEditorPartEvent);
        GE::OnPartPurchased.Add(this, &ModuleInventoryPart::OnPartPurchased);
        if (!PartOf()->isKerbalEVA()) GE::OnInventoryPartOnMouseChanged.Add(this, &ModuleInventoryPart::VesselEditorPartHighlighter);
        editorEventsAdded_ = true;
    }
    Part* self = PartOf();
    const bool kerbalEva = self != nullptr && self->isKerbalEVA();
    if (kerbalEva && self != nullptr && self->protoModuleCrew.size() > 0) {
        hasInitialized = false;
        if (storedPartsNull) {
            storedParts.Clear();
            storedPartsNull = false;
        } else {
            storedParts.Clear();
        }
        Load(*self->protoModuleCrew[0]->InventoryNode());
    }
    ResetInventoryPartsByName();
    isGridShowing = IsEditor() || InventoryEnvironment::alwaysShowInventory ||
                    (VesselOf() != nullptr && (!VesselOf()->isEVA() || (VesselOf()->isEVA() && InventoryEnvironment::CargoPartsLoaded())));
    ToggleInventoryVisibility(isGridShowing);
    GE::onPartActionUICreate.Add(this, &ModuleInventoryPart::onPartActionUIOpened);
    UpdateCapacityValues();
    UpdateMassVolumeDisplay(true, false);
    if (!IgnoreKerbalInventoryLimits() && HasPackedVolumeLimit() && volumeCapacity > packedVolumeLimit()) volumeCapacityReached = true;
    else volumeCapacityReached = false;
    massCapacity = massOccupied;
    if (!IgnoreKerbalInventoryLimits() && HasMassLimit() && massCapacity > massLimit()) massCapacityReached = true;
    else massCapacityReached = false;
    GE::onModuleInventoryChanged.Add(this, &ModuleInventoryPart::OnModuleInventoryChanged);
    eventsAdded_ = true;
}

// ModuleInventoryPart.cs:1031
void ModuleInventoryPart::OnDestroy() {
    GE::onPartActionUICreate.Remove(this, &ModuleInventoryPart::onPartActionUIOpened);
    GE::onEditorPartEvent.Remove(this, &ModuleInventoryPart::OnEditorPartEvent);
    GE::onModuleInventoryChanged.Remove(this, &ModuleInventoryPart::OnModuleInventoryChanged);
    GE::OnPartPurchased.Remove(this, &ModuleInventoryPart::OnPartPurchased);
    GE::OnInventoryPartOnMouseChanged.Remove(this, &ModuleInventoryPart::VesselEditorPartHighlighter);
    if (placementCoroutine_ && InventoryEnvironment::scheduler) InventoryEnvironment::scheduler->StopCoroutine(placementCoroutine_);
    placementCoroutine_.reset();
}

// ModuleInventoryPart.cs:1040
void ModuleInventoryPart::OnLoad(const ConfigNode& node) {
    InitializeInventory();
    ConfigNode* node2 = nullptr;
    if (node.TryGetNode("STOREDPARTS", node2)) {
        std::vector<ConfigNode*> array = node2->GetNodes("STOREDPART");
        if (array.size() > 0) {
            for (size_t i = 0; i < array.size(); i++) {
                auto storedPart = std::make_shared<StoredPart>(*array[i]);
                if (VesselOf() != nullptr) storedPart->snapshot->pVesselRef = VesselOf()->protoVessel;
                storedParts.Add(storedPart->slotIndex, storedPart);
            }
        } else {
            array = node2->GetNodes("PART");
            ConfigNode* node3 = nullptr;
            const bool flag = node.TryGetNode("STACKAMOUNTS", node3);
            for (size_t j = 0; j < array.size(); j++) {
                ConfigNode* node4 = array[j];
                const int num = FirstEmptySlot();
                if (num >= 0) {
                    auto protoPartSnapshot = std::make_shared<ProtoPartSnapshot>(*node4, nullptr, nullptr);
                    auto storedPart2 = std::make_shared<StoredPart>(protoPartSnapshot->partName.value_or(""), num);
                    storedPart2->snapshot = protoPartSnapshot;
                    if (VesselOf() != nullptr) storedPart2->snapshot->pVesselRef = VesselOf()->protoVessel;
                    if (flag && static_cast<int>(j) < node3->nodes().Count()) {
                        node3->nodes().Shared(static_cast<int>(j))->TryGetValue("amount", storedPart2->quantity);
                    } else {
                        storedPart2->quantity = 1;
                    }
                    storedParts.Add(storedPart2->slotIndex, storedPart2);
                }
            }
        }
    }
    std::string inventoryValue = Inventory();
    if (node.TryGetValue("inventory", inventoryValue)) {
        SetInventory(inventoryValue);
        if (storedParts.Count() < 1) {
            std::vector<std::string> array2 = dotnet::Split(inventoryValue, ",", false);
            for (size_t k = 0; k < array2.size(); k++) {
                const int num2 = FirstEmptySlot();
                if (num2 == -1) break;
                if (!array2[k].empty() && Contains(availableCargoParts, array2[k])) {
                    const AvailablePart* partInfoByName = PartInfoByName(array2[k]);
                    if (partInfoByName != nullptr) StoreCargoPartAtSlot(partInfoByName->partPrefab.get(), num2);
                }
            }
        }
    }
    ConfigNode* node5 = nullptr;
    if (node.TryGetNode("DEFAULTPARTS", node5)) {
        std::vector<std::string> values = node5->GetValues("name");
        defaultInventoryParts.clear();
        defaultInventoryPartsNull = false;
        for (size_t l = 0; l < values.size(); l++) {
            if (!values[l].empty() && static_cast<int>(l) <= InventorySlots() - 1) defaultInventoryParts.push_back(values[l]);
        }
    }
    GE::onModuleInventoryChanged.Fire(this);
}

// ModuleInventoryPart.cs:1130
void ModuleInventoryPart::OnSave(ConfigNode& node) {
    if (!storedPartsNull) {
        auto configNode = ConfigNode::Create();
        configNode->name = "STOREDPARTS";
        for (int i = 0; i < storedParts.Count(); i++) {
            const std::shared_ptr<StoredPart>& sp = storedParts.ValuesList()[static_cast<size_t>(i)];
            if (sp != nullptr) {
                auto node2 = ConfigNode::Create("STOREDPART");
                sp->Save(*node2);
                configNode->AddNode(node2);
            } else {
                Err("[ModuleInventoryPart]: A stored part was null when saving. Skipping this part in " + PartOf()->fields.partName.value_or(""));
            }
        }
        node.AddNode(configNode);
    }
    std::string inv;
    bool flag = false;
    ResetInventoryPartsByName();
    for (size_t j = 0; j < inventoryPartsByName.size(); j++) {
        if (!inventoryPartsByName[j].empty()) {
            if (flag) inv += ",";
            inv += inventoryPartsByName[j];
            flag = true;
        }
    }
    SetInventory(inv);
    if (!inv.empty()) node.AddValue("inventory", inv);
}

// ModuleInventoryPart.cs:1175
void ModuleInventoryPart::SaveDefault(ConfigNode& node) {
    SetInventoryDefaults();
    OnSave(node);
}

// ModuleInventoryPart.cs:1181
void ModuleInventoryPart::Update() {
    if (!IsFlight() && !IsEditor()) return;
    UIPartActionControllerInventory* c = UIPartActionControllerInventory::Instance;
    const bool mouseOver = InventoryEnvironment::partMouseOver ? InventoryEnvironment::partMouseOver(*this) : false;
    if (IsEditor() && mouseOver) {
        if (c == nullptr) throw NullReferenceException();
        if (c->CurrentCargoPart() != nullptr) c->OnInventoryPartHover(PartOf()->fields.persistentId, PartOf());
    }
    EVAConstructionModeController* ec = EVAConstructionModeController::Instance;
    if (IsFlight() && ec != nullptr && (!ec->IsOpen() || ec->panelMode != EVAConstructionModeController::PanelMode::Construction) && mouseOver) {
        if (c == nullptr) throw NullReferenceException();
        if (c->CurrentCargoPart() != nullptr) c->OnInventoryPartHover(PartOf()->fields.persistentId, PartOf());
    }
    if (c == nullptr) throw NullReferenceException();
    if (!(c->CurrentCargoPart() != nullptr)) return;
    if (InventoryEnvironment::MODIFIER_KEY_EDGE && !showPreview) {
        volumeCapacity = volumeOccupied;
        massCapacity = massOccupied;
        showPreview = true;
    }
    if (c->currentInventorySlotHovered != nullptr && showPreview && c->CurrentInventorySlotHoveredModule() == this) {
        volumeCapacity = volumeOccupied;
        massCapacity = massOccupied;
        UIPartActionControllerInventory::amountToStore = InventoryEnvironment::MODIFIER_KEY_DOWN
                                                             ? c->CurrentModuleCargoPart()->stackableQuantity()
                                                             : ((UIPartActionControllerInventory::stackSize == 0) ? 1 : UIPartActionControllerInventory::stackSize);
        const int slotIndex = c->currentInventorySlotHovered->slotIndex;
        std::shared_ptr<StoredPart> val;
        storedParts.TryGetValue(slotIndex, val);
        if (val != nullptr) PreviewLimits(*c->CurrentCargoPart(), UIPartActionControllerInventory::amountToStore, val->snapshot->partInfo, slotIndex);
        else PreviewLimits(*c->CurrentCargoPart(), UIPartActionControllerInventory::amountToStore, nullptr, slotIndex);
    }
}

// ModuleInventoryPart.cs:1223
void ModuleInventoryPart::OnUpdate() {
    if (!IsFlight() && !IsEditor()) return;
    UIPartActionControllerInventory* c = UIPartActionControllerInventory::Instance;
    if (c != nullptr) {
        bool flag = false;
        bool flag2 = false;
        Part* self = PartOf();
        if (c->CurrentCargoPart() != nullptr) {
            Vessel* active = FlightRegistry::ActiveVessel;
            if (active == nullptr) throw NullReferenceException();  // FlightGlobals.ActiveVessel.vesselTransform
            distanceToVessel = (active->vesselTransform.position - self->partTransform.position).magnitude();
            if (containerHighlight) {
                if (distanceToVessel > InventoryEnvironment::EVA_INVENTORY_RANGE) {
                    flag = true;
                    flag2 = false;
                }
            } else if (distanceToVessel <= InventoryEnvironment::EVA_INVENTORY_RANGE) {
                flag = true;
                flag2 = true;
                if (IsFlight()) {
                    EVAConstructionModeController* ec = EVAConstructionModeController::Instance;
                    if (ec != nullptr && ec->IsOpen() && ec->panelMode == EVAConstructionModeController::PanelMode::Construction) {
                        const AttachRules r = c->CurrentCargoPart()->fields.attachRules.value_or(AttachRules{});
                        if (!r.srfAttach && !r.allowStack) flag2 = true;
                        else flag = false;
                    }
                    // `if (Instance == null || Instance != null)` (sic): the body dereferences the instance
                    if (ec == nullptr) throw NullReferenceException();
                    if (ec->IsOpen()) {
                        if (ec->panelMode == EVAConstructionModeController::PanelMode::Cargo) flag2 = true;
                    } else {
                        flag2 = true;
                    }
                }
            }
        } else if (containerHighlight) {
            flag = true;
            flag2 = false;
        }
        if (flag) {
            if (flag2) {
                highlightLog.push_back("color:inventoryContainer");
                highlightLog.push_back("highlight:on");
                if (InventoryEnvironment::highlightPart) InventoryEnvironment::highlightPart(*self, true);
                partHighlightActive = true;
            } else {
                highlightLog.push_back("color:default");
                highlightLog.push_back("highlight:off");
                if (InventoryEnvironment::highlightPart) InventoryEnvironment::highlightPart(*self, false);
                partHighlightActive = false;
            }
            containerHighlight = flag2;
        }
    }
    UpdatePlacement(placementInput);
}

// ModuleInventoryPart.cs:1300-1403 (OnUpdate's placement block)
void ModuleInventoryPart::UpdatePlacement(const PlacementInput& in) {
    Vessel* v = VesselOf();
    if (selectedPart != nullptr) {
        if (!in.escapeDown && (!v->isEVA() || !in.jetpackDeployed)) {
            if (in.jumpDown && in.isActiveVessel) {
                const int pending = placementPendingIndex;
                const int collisions = InventoryEnvironment::currentCollisionCount ? InventoryEnvironment::currentCollisionCount(*selectedPart) : 0;
                if (collisions == 0 && partFullyCreated && placementInsideCap && placementonTerrain) {
                    Vector3f position = selectedPart->partTransform.position;
                    const Quaternionf bodyRot = InventoryEnvironment::bodyRotation ? InventoryEnvironment::bodyRotation() : Quaternionf::identity();
                    Quaternionf rotation = UnityNative::Inverse(bodyRot) * selectedPart->partTransform.rotation;
                    DeletePartObject();
                    std::shared_ptr<StoredPart> sp;
                    storedParts.TryGetValue(pending, sp);
                    DeployGroundPart(*sp, position, rotation);
                    GE::onDeployGroundPart.Fire(sp->partName);
                    ClearPartAtSlot(pending);
                    if (pawInventory != nullptr && pawInventoryActive) pawInventory->DestroyItem(pending);
                    placementPendingIndex = -1;
                    placementCoolDown = true;
                    placementCooldownTimer = InventoryEnvironment::UT();
                    partFullyCreated = false;
                } else {
                    placementSoundPlays++;  // placementNotAllowedsFX.Play() (when not already playing)
                }
                return;
            }
            Part* self = PartOf();
            if (in.rcsToggleDown) selectedPart->partTransform.rotation = self ? self->partTransform.rotation : selectedPart->fields.initRotation;
            const float speed = placementRotateSpeed();
            auto rotate = [&](const Vector3f& axis, float sign) {
                // transform.Rotate(axis * speed * dt) (Space.Self): rotation = rotation * Euler(eulers)
                const Vector3f eulers = (sign > 0 ? axis : -axis) * speed * in.deltaTime;
                selectedPart->partTransform.rotation = selectedPart->partTransform.rotation * UnityNative::Euler(eulers);
            };
            if (placementAllowXRotation) {
                if (in.up) rotate(Vector3f::right(), 1);
                if (in.down) rotate(Vector3f::right(), -1);
            }
            if (placementAllowYRotation) {
                if (in.left) rotate(Vector3f::up(), 1);
                if (in.right) rotate(Vector3f::up(), -1);
            }
            if (placementAllowZRotation) {
                if (in.fwd) rotate(Vector3f::forward(), 1);
                if (in.back) rotate(Vector3f::forward(), -1);
            }
            SetPositionFromTerrain();
            const int collisions = InventoryEnvironment::currentCollisionCount ? InventoryEnvironment::currentCollisionCount(*selectedPart) : 0;
            if (collisions == 0 && placementInsideCap && placementonTerrain) {
                if (!placementPositionOk) {
                    highlightLog.push_back("placement:ok");
                    placementPositionOk = true;
                }
            } else if (placementPositionOk) {
                highlightLog.push_back("placement:blocked");
                placementPositionOk = false;
            }
        } else {
            CancelPartPlacementMode();
        }
    } else if (placementCoolDown && InventoryEnvironment::UT() - placementCooldownTimer > 1.0) {
        placementCoolDown = false;
        if (v != nullptr && v->isEVA() && v->evaController != nullptr && InventoryEnvironment::setPartPlacementMode)
            InventoryEnvironment::setPartPlacementMode(false, nullptr);
        if (constructorModeInventory != nullptr) constructorModeInventory->ResetPlacePartIcons(true);
        else if (pawInventory != nullptr) pawInventory->ResetPlacePartIcons(true);
    }
}

// ModuleInventoryPart.cs:1406
std::string ModuleInventoryPart::GetInfo() {
    std::string text = Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8002218"), dotnet::FormatInt64(InventorySlots())}) + "\n";
    if (HasPackedVolumeLimit() || HasPackedVolumeLimit()) text += "\n\n";
    if (HasPackedVolumeLimit())
        text = text + Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8003415"),
                                                              Localizer::Format("<<1>><<2>>", {dotnet::FormatSingleF(packedVolumeLimit(), 1), "L"})}) + "\n";
    if (HasMassLimit())
        text = text + Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8003416"),
                                                              Localizer::Format("<<1>><<2>>", {dotnet::FormatSingleF(massLimit(), 3), ""})}) + "\n";
    return text;
}

// ModuleInventoryPart.cs:1424
std::string ModuleInventoryPart::GetModuleDisplayName() { return Localizer::Format("#autoLOC_8002219"); }

// ModuleInventoryPart.cs:1429
void ModuleInventoryPart::UpdateCapacityValues() {
    volumeOccupied = 0.0f;
    massOccupied = 0.0f;
    for (int i = 0; i < storedParts.Count(); i++) {
        const std::shared_ptr<StoredPart>& sp = storedParts.ValuesList()[static_cast<size_t>(i)];
        if (sp != nullptr && sp->snapshot != nullptr && sp->snapshot->partInfo != nullptr && sp->snapshot->partInfo->partPrefab) {
            Part& prefab = *sp->snapshot->partInfo->partPrefab;
            auto* moduleCargoPart = dynamic_cast<ModuleCargoPart*>(prefab.FindModuleOfClass("ModuleCargoPart"));
            if (moduleCargoPart != nullptr)
                volumeOccupied = f32store(fx(volumeOccupied) + fx(moduleCargoPart->packedVolume()) * fx(static_cast<float>(sp->quantity)));
            massOccupied = f32store(fx(massOccupied) + fx(GetPartMass(prefab)) * fx(static_cast<float>(sp->quantity)));
        }
    }
    UpdateMassVolumeDisplay(true, false);
}

// ModuleInventoryPart.cs:1449
void ModuleInventoryPart::InitializeMassVolumeDisplay() {
    if (gridPresent && pawInventory != nullptr) pawInventory->InitializeMassVolumeDisplay();
    if (constructorModeInventory != nullptr) constructorModeInventory->InitializeMassVolumeDisplay();
}

// ModuleInventoryPart.cs:1461
void ModuleInventoryPart::UpdateMassVolumeDisplay(bool updateCapacities, bool constructionOnly) {
    if (updateCapacities) {
        volumeCapacity = volumeOccupied;
        massCapacity = massOccupied;
    }
    if (!IgnoreKerbalInventoryLimits() && HasPackedVolumeLimit() && volumeCapacity > packedVolumeLimit()) volumeCapacityReached = true;
    else volumeCapacityReached = false;
    if (!IgnoreKerbalInventoryLimits() && HasMassLimit() && massCapacity > massLimit()) massCapacityReached = true;
    else massCapacityReached = false;
    if (HasPackedVolumeLimit()) {
        if (gridPresent && pawInventory != nullptr) pawInventory->UpdateVolumeLimits(volumeCapacity, packedVolumeLimit(), volumeCapacityReached, constructionOnly);
        if (constructorModeInventory != nullptr)
            constructorModeInventory->UpdateVolumeLimits(volumeCapacity, packedVolumeLimit(), volumeCapacityReached, constructionOnly);
    }
    if (HasMassLimit()) {
        if (gridPresent && pawInventory != nullptr) pawInventory->UpdateMassLimits(massCapacity, massLimit(), massCapacityReached);
        if (constructorModeInventory != nullptr) constructorModeInventory->UpdateMassLimits(massCapacity, massLimit(), massCapacityReached);
    }
}

// ModuleInventoryPart.cs:1508
void ModuleInventoryPart::PreviewLimits(Part& newPart, int amountToStore, const AvailablePart* slotPart, int slotIndex) {
    auto* moduleCargoPart = dynamic_cast<ModuleCargoPart*>(newPart.FindModuleOfClass("ModuleCargoPart"));
    const float pvNew = moduleCargoPart->packedVolume();
    if (slotPart == nullptr) {
        if (pvNew < 0.0f) volumeCapacity = f32store(fx(packedVolumeLimit()) * 2.0);
        else volumeCapacity = f32store(fx(volumeCapacity) + fx(pvNew) * fx(static_cast<float>(amountToStore)));
        massCapacity = f32store(fx(massCapacity) + fx(GetPartMass(newPart)) * fx(static_cast<float>(amountToStore)));
    } else {
        auto* moduleCargoPart2 = dynamic_cast<ModuleCargoPart*>(slotPart->partPrefab->FindModuleOfClass("ModuleCargoPart"));
        auto* variants = dynamic_cast<ModulePartVariants*>(newPart.FindModuleOfClass("ModulePartVariants"));
        const std::string partVariantName = (variants != nullptr) ? variants->SelectedVariant()->name : "";
        const int stackAmountAtSlot = GetStackAmountAtSlot(slotIndex);
        if (!CanStackInSlot(*newPart.partInfo, partVariantName, slotIndex)) {
            if (pvNew < 0.0f)
                volumeCapacity = f32store(fx(packedVolumeLimit()) * 2.0);
            else
                volumeCapacity = f32store(fx(volumeCapacity) + fx(pvNew) * fx(static_cast<float>(amountToStore)) -
                                          fx(moduleCargoPart2->packedVolume()) * fx(static_cast<float>(stackAmountAtSlot)));
            massCapacity = f32store(fx(massCapacity) + fx(GetPartMass(newPart)) * fx(static_cast<float>(amountToStore)) -
                                    fx(GetPartMass(*slotPart->partPrefab)) * fx(static_cast<float>(stackAmountAtSlot)));
        } else {
            const float num = static_cast<float>(std::min(amountToStore + stackAmountAtSlot, moduleCargoPart->stackableQuantity()));
            float num2, num3;
            if (pvNew < 0.0f) {
                num2 = f32store(fx(packedVolumeLimit()) * 2.0);
                num3 = f32store(fx(packedVolumeLimit()) * 2.0);
            } else {
                num2 = f32store(fx(pvNew) * fx(static_cast<float>(stackAmountAtSlot)));
                num3 = f32store(fx(pvNew) * fx(num));
            }
            volumeCapacity = f32store(fx(volumeCapacity) + fx(num3) - fx(num2));
            const float num4 = f32store(fx(GetPartMass(newPart)) * fx(static_cast<float>(stackAmountAtSlot)));
            const float num5 = f32store(fx(GetPartMass(newPart)) * fx(num));
            massCapacity = f32store(fx(massCapacity) + fx(num5) - fx(num4));
        }
    }
    showPreview = false;
    UpdateMassVolumeDisplay(false, pvNew < 0.0f);
}

// ModuleInventoryPart.cs:1566
void ModuleInventoryPart::RestoreLimitsDisplay() { UpdateMassVolumeDisplay(true, false); }

// ModuleInventoryPart.cs:1571
float ModuleInventoryPart::GetPartMass(Part& part_) { return f32store(fx(part_.GetResourceMass()) + fx(part_.fields.mass)); }

// ModuleInventoryPart.cs:1576
bool ModuleInventoryPart::HasCapacity(Part& newPart) {
    bool flag = true;
    auto* moduleCargoPart = dynamic_cast<ModuleCargoPart*>(newPart.FindModuleOfClass("ModuleCargoPart"));
    if (moduleCargoPart->packedVolume() < 0.0f || fx(moduleCargoPart->packedVolume()) + fx(volumeCapacity) > fx(packedVolumeLimit())) flag = false;
    if (flag && fx(GetPartMass(*PartOf())) + fx(massCapacity) > fx(massLimit())) flag = false;
    return flag;
}

// ModuleInventoryPart.cs:1591
void ModuleInventoryPart::KerbalStateChanged() {
    if (gridPresent && pawInventory != nullptr) pawInventory->ResetPlacePartIcons(true);
}

// ModuleInventoryPart.cs:1599
void ModuleInventoryPart::ToggleInventoryVisibility(bool show) {
    Fields()["InventorySlots"]->guiActive = show;
    Fields()["InventorySlots"]->guiActiveEditor = show;
}

// ModuleInventoryPart.cs:1605
void ModuleInventoryPart::onPartActionUIOpened(Part* p) {
    if (part != nullptr && p == PartOf()) UpdateModuleUI();
}

// ModuleInventoryPart.cs:1613
void ModuleInventoryPart::InitializeInventory() {
    availableCargoParts = InventoryEnvironment::GetAvailableCargoPartNames();
    availableCargoInventoryParts = InventoryEnvironment::GetAvailableCargoInventoryPartNames();
    if (!hasInitialized) {
        if (InventorySlots() > maxInventorySlots) SetInventorySlots(maxInventorySlots);
        storedParts.Clear();
        storedPartsNull = false;
        hasInitialized = true;
    }
}

// ModuleInventoryPart.cs:1635
void ModuleInventoryPart::SetInventoryDefaults() {
    if (defaultInventoryPartsNull || defaultInventoryParts.size() < 1) return;
    hasInitialized = false;
    InitializeInventory();
    for (size_t i = 0; i < defaultInventoryParts.size() && static_cast<int>(i) < InventorySlots(); i++) {
        if (Contains(availableCargoParts, defaultInventoryParts[i])) {
            const AvailablePart* partInfoByName = PartInfoByName(defaultInventoryParts[i]);
            if (partInfoByName != nullptr && (!InventoryEnvironment::partModelPurchased || InventoryEnvironment::partModelPurchased(*partInfoByName)))
                StoreCargoPartAtSlot(partInfoByName->partPrefab.get(), FirstEmptySlot());
        }
    }
    ResetInventoryPartsByName();
}

// ModuleInventoryPart.cs:1657
void ModuleInventoryPart::ResetInventoryPartsByName() {
    inventoryPartsByName.clear();
    for (int i = 0; i < InventorySlots(); i++) {
        if (!storedPartsNull && storedParts.ContainsKey(i)) inventoryPartsByName.push_back(storedParts[i]->partName);
        else inventoryPartsByName.push_back("");
    }
}

// ModuleInventoryPart.cs:1683
void ModuleInventoryPart::UpdateModuleUI() {
    if (hasInitialized && gridPresent) gridUpdateSlotItems = true;
}

// ModuleInventoryPart.cs:1691
void ModuleInventoryPart::CancelPartPlacementMode() {
    if (selectedPart != nullptr) {
        placementCoolDown = true;
        placementCooldownTimer = InventoryEnvironment::UT();
        DeletePartObject();
    }
}

// ModuleInventoryPart.cs:1701
int ModuleInventoryPart::FirstEmptySlot() {
    int num = 0;
    while (true) {
        if (num < InventorySlots()) {
            if (!storedParts.ContainsKey(num)) break;
            num++;
            continue;
        }
        return -1;
    }
    return num;
}

// ModuleInventoryPart.cs:1720
int ModuleInventoryPart::FirstFullSlot() {
    int num = 0;
    while (true) {
        if (num < InventorySlots()) {
            if (storedParts.ContainsKey(num)) break;
            num++;
            continue;
        }
        return -1;
    }
    return num;
}

// ModuleInventoryPart.cs:1739
int ModuleInventoryPart::TotalEmptySlots() { return InventorySlots() - storedParts.Count(); }

// ModuleInventoryPart.cs:1744
void ModuleInventoryPart::OnCopy(PartModule& fromModule) {
    auto* from = dynamic_cast<ModuleInventoryPart*>(&fromModule);
    storedParts = DictionaryValueList<int32_t, std::shared_ptr<StoredPart>>();
    storedPartsNull = false;
    for (size_t i = 0; i < from->storedParts.ValuesList().size(); i++) {
        const auto& sp = from->storedParts.ValuesList()[i];
        storedParts.Add(sp->slotIndex, sp->Copy());
    }
}

// ModuleInventoryPart.cs:1754
bool ModuleInventoryPart::ContainsPart(const std::string& partName) {
    if (!storedPartsNull) {
        for (size_t i = 0; i < storedParts.ValuesList().size(); i++)
            if (storedParts.ValuesList()[i]->partName == partName) return true;
    }
    return false;
}

// ModuleInventoryPart.cs:1769
void ModuleInventoryPart::DeployGroundPart(StoredPart& storedPart, const Vector3f& partPosition, const Quaternionf& rotation) {
    ConfigNode::NodePtr protoVesselNode = GetProtoVesselNode(storedPart, partPosition, rotation);
    if (InventoryEnvironment::addVessel) InventoryEnvironment::addVessel(*protoVesselNode);
}

// ModuleInventoryPart.cs:1775
ConfigNode::NodePtr ModuleInventoryPart::GetProtoVesselNode(StoredPart& storedPart, const Vector3f& partPosition, const Quaternionf& rotation) {
    Vessel* v = VesselOf();
    const uint32_t uniqueFlightID = InventoryEnvironment::uniqueFlightID ? InventoryEnvironment::uniqueFlightID() : 0u;
    Orbit orbit = Orbit::CreateRandomOrbitAround(v->mainBody());
    std::string vesselName = storedPart.partName;
    VesselType vesselType = VesselType::Unknown;
    if (storedPart.snapshot != nullptr) {
        ProtoPartModuleSnapshot* m1 = storedPart.snapshot->FindModule("ModuleGroundExpControl");
        ProtoPartModuleSnapshot* m2 = storedPart.snapshot->FindModule("ModuleGroundSciencePart");
        ProtoPartModuleSnapshot* m3 = storedPart.snapshot->FindModule("ModuleGroundExperiment");
        vesselName = storedPart.snapshot->partInfo->title;
        if (m1 != nullptr || m2 != nullptr || m3 != nullptr) vesselType = VesselType::DeployedScienceController;
    }
    ConfigNode::NodePtr configNode = ProtoVesselNodes::CreateVesselNode(vesselName, vesselType, orbit, 0, {CreatePartNode(uniqueFlightID, storedPart)},
                                                                        FlightRegistry::Bodies());
    configNode->SetValue("landedAt", v->mainBody()->bodyName);
    double alt = v->altitude;
    double lat = v->latitude;
    double lon = v->longitude;
    v->mainBody()->GetLatLonAlt(Vector3d(partPosition), lat, lon, alt);
    configNode->SetValue("lat", lat);
    configNode->SetValue("lon", lon);
    configNode->SetValue("sit", std::string("LANDED"));
    configNode->SetValue("alt", alt);
    configNode->SetValue("landed", true);
    configNode->SetValue("splashed", false);
    configNode->SetValue("skipGroundPositioning", false);
    configNode->SetValue("vesselSpawning", true);
    configNode->AddValue("prst", true);
    configNode->AddValue("rot", rotation);
    configNode->SetValue("skipGroundPositioning", false);
    configNode->SetValue("PQSMin", 0, true);
    configNode->SetValue("PQSMax", 0, true);
    return configNode;
}

// ModuleInventoryPart.cs:1816
ConfigNode::NodePtr ModuleInventoryPart::CreatePartNode(uint32_t id, StoredPart& storedPart) {
    auto configNode = ConfigNode::Create("PART");
    storedPart.snapshot->Save(*configNode);
    configNode->SetValue("flag", PartOf()->fields.flagURL.value_or(""), true);
    configNode->SetValue("uid", id, true);
    configNode->SetValue("mid", id, true);
    configNode->SetValue("state", 0);
    std::vector<ConfigNode*> nodes = configNode->GetNodes("MODULE");
    Vessel* v = VesselOf();
    for (size_t i = 0; i < nodes.size(); i++) {
        std::string value;
        ConfigNode* configNode2 = nodes[i];
        if (!configNode2->TryGetValue("name", value)) continue;
        int32_t value2 = 0;
        float num = 0.0f;
        if (configNode2->TryGetValue("powerUnitsProduced", value2) && value2 > 0 && v->isEVA()) {
            auto& crew = v->parts[0]->protoModuleCrew[0];
            if (crew->HasEffect<Experience::Effects::DeployedSciencePowerSkill>()) {
                Experience::Effects::DeployedSciencePowerSkill* effect = crew->GetEffect<Experience::Effects::DeployedSciencePowerSkill>();
                if (effect != nullptr) num = effect->GetValue();
                configNode2->SetValue("powerUnitsProduced", f32store(fx(static_cast<float>(value2)) + fx(num)));
            }
        }
        float num2 = 0.0f;
        if (value == "ModuleGroundExperiment" && v->isEVA()) {
            auto& crew = v->parts[0]->protoModuleCrew[0];
            if (crew->HasEffect<Experience::Effects::DeployedScienceExpSkill>()) {
                Experience::Effects::DeployedScienceExpSkill* effect2 = crew->GetEffect<Experience::Effects::DeployedScienceExpSkill>();
                if (effect2 != nullptr) {
                    num2 = f32store(fx(effect2->GetValue()) * 100.0);
                    configNode2->SetValue("ScienceModifierRate", num2, true);
                }
            }
        }
        if (configNode2->HasValue("beingDeployed")) configNode2->SetValue("beingDeployed", true);
    }
    return configNode;
}

// ModuleInventoryPart.cs:1862
int ModuleInventoryPart::TotalAmountOfPartStored(const std::string& requestedPartName) {
    int num = -1;
    int count = storedParts.Count();
    while (count-- > 0) {
        const std::shared_ptr<StoredPart>& sp = storedParts.ValuesList()[static_cast<size_t>(count)];
        if (sp != nullptr && sp->partName == requestedPartName) {
            if (num == -1) num = 0;
            num += sp->quantity;
        }
    }
    return num;
}

// ModuleInventoryPart.cs:1881
bool ModuleInventoryPart::RemoveNPartsFromInventory(const std::string& requestedPartName, int n, bool playSound) {
    int num = n;
    if (TotalAmountOfPartStored(requestedPartName) < n) return false;
    int count = storedParts.Count();
    std::shared_ptr<StoredPart> storedPart;
    while (true) {
        if (count-- > 0) {
            storedPart = storedParts.ValuesList()[static_cast<size_t>(count)];
            if (storedPart != nullptr && storedPart->partName == requestedPartName) {
                if (playSound) {
                    // effectList = storedPart.snapshot.partPrefab.Effects
                    soundsPlayed++;
                }
                if (storedPart->quantity > num) {
                    storedPart->quantity -= num;
                    if (playSound) effectPlays++;  // effectList?.PlayRandomEffect()
                    GE::onModuleInventorySlotChanged.Fire(this, storedPart->slotIndex);
                    return true;
                }
                if (storedPart->quantity == num) break;
                num -= storedPart->quantity;
                ClearPartAtSlot(storedPart->slotIndex);
                GE::onModuleInventorySlotChanged.Fire(this, storedPart->slotIndex);
            }
            continue;
        }
        Warn("[ModuleInventoryPart]: We removed some of part " + requestedPartName + "- but we ran out of them");
        return false;
    }
    ClearPartAtSlot(storedPart->slotIndex);
    if (playSound) effectPlays++;
    GE::onModuleInventorySlotChanged.Fire(this, storedPart->slotIndex);
    return true;
}

// ModuleInventoryPart.cs:1928
float ModuleInventoryPart::GetModuleMass(float, ModifierStagingSituation) {
    float num = 0.0f;
    if (storedPartsNull) return num;
    if (storedParts.Count() > 0) {
        for (int i = 0; i < storedParts.Count(); i++) {
            const std::shared_ptr<StoredPart>& sp = storedParts.ValuesList()[static_cast<size_t>(i)];
            if (sp == nullptr || sp->snapshot == nullptr) continue;
            const float qty = static_cast<float>(std::max(1, sp->quantity));
            num = f32store(fx(num) + fx(sp->snapshot->mass) * fx(qty));
            float num2 = 0.0f;
            int count = static_cast<int>(sp->snapshot->resources.size());
            while (count-- > 0) {
                ProtoPartResourceSnapshot* r = sp->snapshot->resources[static_cast<size_t>(count)].get();
                if (r != nullptr && r->definition != nullptr)
                    num2 = f32store(fx(num2) + fx(r->definition->density()) * fx(f32store(r->amount)));  // (float)amount is a conv.r4
            }
            num = f32store(fx(num) + fx(num2) * fx(qty));
        }
    }
    return num;
}

// ModuleInventoryPart.cs:1966
float ModuleInventoryPart::GetModuleCost(float, ModifierStagingSituation) {
    float num = 0.0f;
    float dryCost = 0.0f;
    float fuelCost = 0.0f;
    if (InventoryItemCount() > 0) {
        for (int i = 0; i < storedParts.Count(); i++) {
            const std::shared_ptr<StoredPart>& sp = storedParts.ValuesList()[static_cast<size_t>(i)];
            if (sp != nullptr && sp->snapshot != nullptr) {
                ShipConstruction::GetPartCosts(*sp->snapshot, sp->snapshot->partInfo, dryCost, fuelCost);
                num = f32store(fx(num) + (fx(fuelCost) + fx(dryCost)) * fx(static_cast<float>(std::max(1, sp->quantity))));
            }
        }
    }
    return num;
}

// ModuleInventoryPart.cs:1994
bool ModuleInventoryPart::IgnoreKerbalInventoryLimits() {
    if (InventoryEnvironment::IgnoreKerbalInventoryLimits) return PartOf()->isKerbalEVA();
    return false;
}

}  // namespace ksp

