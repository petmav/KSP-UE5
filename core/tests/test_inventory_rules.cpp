// WP-33 unit tests: StoredPart, ModuleInventoryPart persistence / events / stack rules / EVA propellant refill / OnCopy, ModuleCargoPart,
// KerbalInventoryScenario and the kerbal glue. Every expectation is derived from the decompiled C# (the derivation is in the comment above
// each check); the numeric gates against the Python reference are in test_inventory.cpp.
#include "inventory_fixture.hpp"

using namespace ksp;
using namespace ksp::inv33;

namespace {
std::string SaveText(const ConfigNode& n) { return n.ToString(); }
// the first differing line of two texts ("" when equal)
std::string FirstDiff(const std::string& a, const std::string& b) {
    size_t i = 0;
    while (i < a.size() && i < b.size() && a[i] == b[i]) i++;
    if (i == a.size() && i == b.size()) return "";
    auto lineAt = [&](const std::string& s) {
        size_t from = s.rfind('\n', i == 0 ? 0 : i - 1);
        from = from == std::string::npos ? 0 : from + 1;
        size_t to = s.find('\n', from);
        return s.substr(from, to == std::string::npos ? std::string::npos : to - from);
    };
    return lineAt(a) + " | " + lineAt(b);
}
}  // namespace

// StoredPart.cs:19-99
TEST_CASE("WP-33 StoredPart: IsEmpty / IsFull / CanStack, Save / Load round trip and Copy") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    SceneScope scene(GameScenes::EDITOR);
    StoredPart none("evaRepairKit", 3);
    CHECK(none.slotIndex == 3);
    CHECK(none.quantity == 0);
    CHECK(none.stackCapacity == 1);
    CHECK(none.variantName == "");
    CHECK_FALSE(none.IsEmpty());  // snapshot == null -> false
    CHECK_FALSE(none.CanStack());
    CHECK(none.IsFull());  // sic: quantity (0) < stackCapacity (1)
    StoredPart sp("evaRepairKit", 5);
    sp.snapshot = std::make_shared<ProtoPartSnapshot>(Prefab(g, "evaRepairKit"), nullptr);
    sp.quantity = 2;
    sp.stackCapacity = 4;
    CHECK_FALSE(sp.IsEmpty());
    CHECK(sp.CanStack());
    CHECK(sp.IsFull());
    sp.quantity = 4;
    CHECK_FALSE(sp.IsFull());
    sp.quantity = 0;
    CHECK(sp.IsEmpty());
    sp.quantity = 3;
    sp.stackCapacity = 1;
    CHECK_FALSE(sp.CanStack());
    sp.stackCapacity = 4;
    ConfigNode node("STOREDPART");
    sp.Save(node);
    CHECK(node.HasNode("PART"));
    for (const char* k : {"slotIndex", "partName", "quantity", "stackCapacity", "variantName"}) CHECK(node.HasValue(k));
    std::string v;
    node.TryGetValue("slotIndex", v);
    CHECK(v == "5");
    StoredPart back(node);
    CHECK(back.slotIndex == 5);
    CHECK(back.partName == "evaRepairKit");
    CHECK(back.quantity == 3);
    CHECK(back.stackCapacity == 4);
    REQUIRE(back.snapshot != nullptr);
    CHECK(back.snapshot->partName.value_or("") == "evaRepairKit");
    std::shared_ptr<StoredPart> copy = back.Copy();
    CHECK(copy.get() != &back);
    CHECK(copy->snapshot.get() != back.snapshot.get());
    CHECK(copy->slotIndex == 5);
    CHECK(copy->quantity == 3);
    CHECK(copy->stackCapacity == 4);
    // the copy's snapshot is built from the saved node while the original id is registered, so ProtoPartSnapshot takes a new persistentId
    CHECK(copy->snapshot->persistentId != back.snapshot->persistentId);
    copy->snapshot->persistentId = back.snapshot->persistentId;
    ConfigNode n1("A"), n2("A");
    back.Save(n1);
    copy->Save(n2);
    CHECK(FirstDiff(SaveText(n1), SaveText(n2)) == "");
}

// ModuleInventoryPart.cs:1040 / 1130: STOREDPARTS round trip, the legacy PART + STACKAMOUNTS format, the `inventory` string, DEFAULTPARTS
TEST_CASE("WP-33 ModuleInventoryPart: OnSave / OnLoad formats (STOREDPART, legacy PART + STACKAMOUNTS, inventory string, DEFAULTPARTS)") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    SceneScope scene(GameScenes::EDITOR);
    Host a(g, "cargoContainer");
    REQUIRE(a.inv->StoreCargoPartAtSlot(&Prefab(g, "evaRepairKit"), 4));
    REQUIRE(a.inv->UpdateStackAmountAtSlot(4, 3));
    REQUIRE(a.inv->StoreCargoPartAtSlot(&Prefab(g, "evaJetpack"), 1));
    ConfigNode save("MODULE");
    a.inv->OnSave(save);
    ConfigNode* stored = save.GetNode("STOREDPARTS");
    REQUIRE(stored != nullptr);
    CHECK(stored->GetNodes("STOREDPART").size() == 2);
    std::string inventory;
    CHECK(save.TryGetValue("inventory", inventory));
    CHECK(inventory == "evaJetpack,evaRepairKit");  // ResetInventoryPartsByName order: slot 1 then slot 4
    Host b(g, "cargoContainer");
    Counts c;
    b.inv->OnLoad(save);
    REQUIRE(b.inv->storedParts.Count() == 2);
    CHECK(b.inv->storedParts.ValuesList()[0]->slotIndex == 4);  // the dictionary keeps the saved node order
    CHECK(b.inv->storedParts.ValuesList()[0]->quantity == 3);
    CHECK(b.inv->storedParts.ValuesList()[1]->partName == "evaJetpack");
    CHECK(c.changed == 1);  // OnLoad fires onModuleInventoryChanged once at its end
    // legacy: PART nodes directly under STOREDPARTS, quantities from STACKAMOUNTS in node order, slots by FirstEmptySlot
    ConfigNode legacy("MODULE");
    ConfigNode* lp = legacy.AddNode("STOREDPARTS");
    ProtoPartSnapshot s1(Prefab(g, "evaRepairKit"), nullptr), s2(Prefab(g, "evaJetpack"), nullptr), s3(Prefab(g, "evaChute"), nullptr);
    s1.Save(*lp->AddNode("PART"));
    s2.Save(*lp->AddNode("PART"));
    s3.Save(*lp->AddNode("PART"));
    ConfigNode* sa = legacy.AddNode("STACKAMOUNTS");
    sa->AddNode("STACK")->AddValue("amount", 4);
    sa->AddNode("STACK")->AddValue("amount", 1);  // only two entries: the third part gets quantity 1
    Host d(g, "cargoContainer");
    d.inv->OnLoad(legacy);
    REQUIRE(d.inv->storedParts.Count() == 3);
    CHECK(d.inv->storedParts[0]->partName == "evaRepairKit");
    CHECK(d.inv->storedParts[0]->quantity == 4);
    CHECK(d.inv->storedParts[1]->quantity == 1);
    CHECK(d.inv->storedParts[2]->partName == "evaChute");
    CHECK(d.inv->storedParts[2]->quantity == 1);
    // the very old `inventory = a,b,,c` string (only when no stored parts were read): the names that are available cargo parts
    ConfigNode oldfmt("MODULE");
    oldfmt.AddValue("inventory", "evaJetpack,,notAPart,evaChute");
    Host e(g, "cargoContainer");
    e.inv->OnLoad(oldfmt);
    REQUIRE(e.inv->storedParts.Count() == 2);
    CHECK(e.inv->storedParts[0]->partName == "evaJetpack");
    CHECK(e.inv->storedParts[1]->partName == "evaChute");  // an empty or unknown entry takes no slot
    // DEFAULTPARTS: names beyond InventorySlots-1 are dropped; SetInventoryDefaults stores the available ones
    ConfigNode defs("MODULE");
    ConfigNode* dp = defs.AddNode("DEFAULTPARTS");
    dp->AddValue("name", "evaChute");
    dp->AddValue("name", "evaJetpack");
    dp->AddValue("name", "evaRepairKit");
    Host f(g, "kerbalEVA");  // two slots
    f.inv->OnLoad(defs);
    REQUIRE(f.inv->defaultInventoryParts.size() == 2);
    f.inv->SetInventoryDefaults();
    REQUIRE(f.inv->storedParts.Count() == 2);
    CHECK(f.inv->storedParts[0]->partName == "evaChute");
    CHECK(f.inv->storedParts[1]->partName == "evaJetpack");
    ConfigNode out("INVENTORY");
    f.inv->SaveDefault(out);
    CHECK(out.GetNode("STOREDPARTS")->GetNodes("STOREDPART").size() == 2);
}

// ModuleInventoryPart.cs:386-470 / 429 / 842 / 1881: stack rules, events, clamping
TEST_CASE("WP-33 ModuleInventoryPart: slot, stack and variant rules fire the inventory events as the C# does") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    SceneScope scene(GameScenes::EDITOR);
    Host h(g, "cargoContainer");
    ModuleInventoryPart* inv = h.inv;
    Counts c;
    CHECK(inv->InventoryIsEmpty());
    CHECK_FALSE(inv->InventoryIsFull());
    CHECK(inv->FirstEmptySlot() == 0);
    CHECK(inv->FirstFullSlot() == -1);
    CHECK(inv->TotalEmptySlots() == 9);
    CHECK(inv->GetStackAmountAtSlot(2) == 0);
    CHECK(inv->GetStackCapacityAtSlot(2) == 0);
    CHECK_FALSE(inv->IsStackable(2));
    CHECK(inv->GetVariantNameForSlot(2) == "");
    CHECK_FALSE(inv->HasStackingSpace(2));
    CHECK(inv->IsSlotEmpty(2));
    REQUIRE(inv->StoreCargoPartAtSlot(&Prefab(g, "evaRepairKit"), 2));
    CHECK(c.changed == 1);
    CHECK(c.slots == std::vector<int>{2});
    CHECK(inv->IsStackable(2));
    CHECK(inv->GetStackCapacityAtSlot(2) == 4);
    CHECK(inv->GetStackAmountAtSlot(2) == 1);
    CHECK(inv->HasStackingSpace(2));
    CHECK(inv->FirstEmptySlot() == 0);
    CHECK(inv->FirstFullSlot() == 2);
    const AvailablePart& kit = g.Info("evaRepairKit");
    CHECK(inv->CanStackInSlot(kit, "", 2));
    CHECK_FALSE(inv->CanStackInSlot(g.Info("evaJetpack"), "", 2));
    CHECK_FALSE(inv->CanStackInSlot(kit, "someVariant", 2));  // the stored part's variantName is ""
    c.slots.clear();
    c.changed = 0;
    CHECK(inv->UpdateStackAmountAtSlot(2, 1));  // unchanged: no events
    CHECK(c.changed == 0);
    CHECK(inv->UpdateStackAmountAtSlot(2, 99));
    CHECK(inv->GetStackAmountAtSlot(2) == 4);
    CHECK_FALSE(inv->HasStackingSpace(2));
    CHECK_FALSE(inv->CanStackInSlot(kit, "", 2));
    CHECK(inv->UpdateStackAmountAtSlot(2, -5));
    CHECK(inv->GetStackAmountAtSlot(2) == 1);
    CHECK(c.changed == 2);
    CHECK(c.slots == std::vector<int>{2, 2});
    InventoryEnvironment::log.clear();
    CHECK_FALSE(inv->UpdateStackAmountAtSlot(7, 2));
    CHECK_FALSE(inv->UpdateStackAmountAtSlot(2, 2, "other"));
    CHECK(InventoryEnvironment::log.size() == 2);
    CHECK(inv->GetStackAmountAtSlot(2) == 1);
    // replacing an occupied slot: Clear (bypass) then Add; the dictionary moves the slot to the end
    REQUIRE(inv->StoreCargoPartAtSlot(&Prefab(g, "evaJetpack"), 5));
    REQUIRE(inv->StoreCargoPartAtSlot(&Prefab(g, "evaChute"), 2));
    CHECK(inv->storedParts.KeysList() == std::vector<int>{5, 2});
    c.slots.clear();
    REQUIRE(inv->StoreCargoPartAtSlot(std::make_shared<ProtoPartSnapshot>(Prefab(g, "evaJetpack"), nullptr), -1));  // -1 = FirstEmptySlot
    CHECK(c.slots == std::vector<int>{0});
    CHECK(inv->ContainsPart("evaJetpack"));
    CHECK_FALSE(inv->ContainsPart("evaRepairKit"));
    CHECK(inv->TotalAmountOfPartStored("evaJetpack") == 2);
    CHECK(inv->TotalAmountOfPartStored("evaRepairKit") == -1);
    CHECK_FALSE(inv->RemoveNPartsFromInventory("evaJetpack", 3));  // not enough
    CHECK(inv->RemoveNPartsFromInventory("evaJetpack", 2));
    CHECK_FALSE(inv->ContainsPart("evaJetpack"));
    c.changed = 0;
    CHECK(inv->ClearPartAtSlot(8));  // empty slot: true and silent
    CHECK(c.changed == 0);
    CHECK(inv->ClearPartAtSlot(2));
    CHECK(c.changed == 1);
    Host small(g, "smallCargoContainer");
    REQUIRE(small.inv->StoreCargoPartAtSlot(&Prefab(g, "evaJetpack"), -1));
    REQUIRE(small.inv->StoreCargoPartAtSlot(&Prefab(g, "evaJetpack"), -1));
    REQUIRE(small.inv->StoreCargoPartAtSlot(&Prefab(g, "evaJetpack"), -1));
    CHECK(small.inv->InventoryIsFull());
    CHECK_FALSE(small.inv->CheckPartStorage("evaJetpack"));
    CHECK_FALSE(small.inv->StoreCargoPartAtSlot(std::string("evaJetpack"), true, -1));  // the obsolete overload refuses a full container
    CHECK(small.inv->FirstEmptySlot() == -1);
    CHECK_FALSE(small.inv->StoreCargoPartAtSlot(std::make_shared<ProtoPartSnapshot>(Prefab(g, "evaJetpack"), nullptr), -1));
    CHECK_FALSE(inv->StoreCargoPartAtSlot(std::string("noSuchPart"), 3));
}

// ModuleInventoryPart.cs:803-840 / 1744-1752
TEST_CASE("WP-33 ModuleInventoryPart: EVA propellant refill on storing in a vessel container and OnCopy") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    SceneScope scene(GameScenes::EDITOR);
    auto jet = g.New("evaJetpack");
    REQUIRE(jet->Resources.Count() == 1);
    jet->Resources.At(0).amount = 2.0;
    Host box(g, "cargoContainer");
    REQUIRE(box.inv->StoreCargoPartAtSlot(jet.get(), 0));
    ProtoPartSnapshot& snap = *box.inv->storedParts[0]->snapshot;
    REQUIRE(snap.resources.size() == 1);
    CHECK(snap.resources[0]->amount == snap.resources[0]->maxAmount);  // reinitResourcesOnStoreInVessel: refilled
    CHECK(snap.resources[0]->amount == 5.0);
    // an EVA kerbal's own inventory leaves the propellant alone
    Host kerbal(g, "kerbalEVA");
    jet->Resources.At(0).amount = 1.0;
    REQUIRE(kerbal.inv->StoreCargoPartAtSlot(jet.get(), 0));
    CHECK(kerbal.inv->storedParts[0]->snapshot->resources[0]->amount == 1.0);
    auto pod = g.New("mk1pod.v2");
    kerbal.inv->RefillEVAPropellantOnBoarding(*pod);  // boarding a non-EVA part refills every stored snapshot
    CHECK(kerbal.inv->storedParts[0]->snapshot->resources[0]->amount == 5.0);
    Host copyTo(g, "cargoContainer");
    REQUIRE(box.inv->StoreCargoPartAtSlot(&Prefab(g, "evaRepairKit"), 6));
    box.inv->UpdateStackAmountAtSlot(6, 3);
    copyTo.inv->OnCopy(*box.inv);
    REQUIRE(copyTo.inv->storedParts.Count() == 2);
    CHECK(copyTo.inv->storedParts.KeysList() == box.inv->storedParts.KeysList());
    CHECK(copyTo.inv->storedParts[6]->quantity == 3);
    CHECK(copyTo.inv->storedParts[6].get() != box.inv->storedParts[6].get());
    CHECK(copyTo.inv->storedParts[6]->snapshot.get() != box.inv->storedParts[6]->snapshot.get());
}

// ModuleCargoPart.cs:44 / 96 / 164 / 286 / 330 / 358-384
TEST_CASE("WP-33 ModuleCargoPart: settle mass, info text, tooltip, linked compound parts, construction highlight decision") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    PhysicsWorld phys;
    SceneScope scene(GameScenes::FLIGHT);
    LoadLocalizer();
    auto kit = g.New("evaRepairKit");
    auto* cargo = Mod<ModuleCargoPart>(*kit, "ModuleCargoPart");
    CHECK(cargo->packedVolume() == 5.0f);
    CHECK(cargo->stackableQuantity() == 4);
    // MassForWeightTesting: preSettlingPrefabMass when set, else the part mass
    CHECK(Bits(cargo->MassForWeightTesting()) == Bits(kit->fields.mass));
    cargo->preSettlingPrefabMass = 20.0f;
    CHECK(cargo->MassForWeightTesting() == 20.0f);
    cargo->preSettlingPrefabMass = 0.0f;
    // GetInfo (:164): volume line when packedVolume > 0, stack line when stackableQuantity > 1; the literal backslash-n after the mass
    const std::string info = cargo->GetInfo();
    CHECK(info.find(Localizer::Format("#autoLOC_8002220")) == 0);  // packedVolume >= 0: the cargo header
    CHECK(info.find(" t\\n") != std::string::npos);
    CHECK(info.find("5.0") != std::string::npos);  // "0.0" format of the packed volume
    CHECK(cargo->GetTooltip() == info);            // inventoryTooltip is empty
    cargo->Set<std::string>("inventoryTooltip", "#autoLOC_8002219");
    CHECK(cargo->GetTooltip() == Localizer::Format("#autoLOC_8002219"));
    CHECK(cargo->GetModuleDisplayName() == Localizer::Format("#autoLOC_8002221"));
    // packedVolume < 0: the construction-only header and no volume line
    auto strut = g.New("fuelLine");
    auto* sc = Mod<ModuleCargoPart>(*strut, "ModuleCargoPart");
    sc->Set<float>("packedVolume", -1.0f);
    CHECK(sc->GetInfo().find(Localizer::Format("#autoLOC_6002641")) == 0);
    // linked compound parts: AddUnique / Remove by target persistentId
    CHECK_FALSE(cargo->HasLinkedCompoundParts());
    CompoundPart link;
    link.target = kit.get();
    cargo->OnCompoundPartLinked(&link);
    cargo->OnCompoundPartLinked(&link);
    CHECK(cargo->linkedCompoundParts.size() == 1);
    CHECK(cargo->HasLinkedCompoundParts());
    CompoundPart other;
    other.target = strut.get();
    kit->fields.persistentId = 11;
    strut->fields.persistentId = 22;
    cargo->OnCompoundPartLinked(&other);
    CHECK(cargo->linkedCompoundParts.size() == 1);
    cargo->OnCompoundPartDetached(&link);
    CHECK_FALSE(cargo->HasLinkedCompoundParts());
    // OnPartUnpack clears beingAttached first
    cargo->Set<bool>("beingAttached", true);
    cargo->OnPartUnpack();
    CHECK_FALSE(cargo->beingAttached());
    // the construction highlight (:286): in range, light enough, detachable, childless, no links -> highlight on
    auto vessel = BareVessel(VesselType::EVA, Vector3f(0.0f, 0.0f, 0.0f));
    FlightRegistry::ActiveVessel = vessel.get();
    InventoryEnvironment::activeVesselPosition = [&] { return vessel->vesselTransform.position; };
    cargo->inConstructionMode = true;
    kit->partTransform.position = Vector3f(2.0f, 0.0f, 0.0f);
    cargo->OnUpdateHighlight();
    CHECK(cargo->constructionModeHighlight);
    CHECK(cargo->distanceToEngineer == 2.0f);
    CHECK(cargo->constructionHighlightApplied.value_or(false));
    kit->partTransform.position = Vector3f(9.0f, 0.0f, 0.0f);  // beyond EVA_CONSTRUCTION_RANGE (7)
    cargo->OnUpdateHighlight();
    CHECK_FALSE(cargo->constructionModeHighlight);
    cargo->HideUI();  // UIHidden: highlight forced off
    CHECK(cargo->UIHidden);
    cargo->ShowUI();
    CHECK_FALSE(cargo->UIHidden);
    // IsDeployedSciencePart: needs Serenity and a ModuleGroundSciencePart
    CHECK_FALSE(cargo->IsDeployedSciencePart());
    ModuleCargoPart::isExpansionInstalled = [](const std::string&) { return false; };
    CHECK_FALSE(cargo->IsDeployedSciencePart());
}

// KerbalInventoryScenario.cs:12-133
TEST_CASE("WP-33 KerbalInventoryScenario: instance registry, rename, remove, OnSave into the roster") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    SceneScope scene(GameScenes::EDITOR);
    Host a(g, "kerbalEVA");
    Host b(g, "kerbalEVA");
    KerbalInventoryScenario scen;
    scen.Awake();
    CHECK(KerbalInventoryScenario::Instance == &scen);
    scen.AddKerbalInventoryInstance("Jebediah Kerman", a.inv);
    scen.AddKerbalInventoryInstance("Bill Kerman", b.inv);
    CHECK(scen.ContainsCrew("Jebediah Kerman"));
    CHECK(KerbalInventoryScenario::kerbalInventoryGameObjectChildren == std::vector<std::string>{"Jebediah Kerman", "Bill Kerman"});
    InventoryEnvironment::log.clear();
    scen.AddKerbalInventoryInstance("Bill Kerman", a.inv);  // duplicate: replaced, no second child
    CHECK(scen.Inventories()[1].second == a.inv);
    CHECK(KerbalInventoryScenario::kerbalInventoryGameObjectChildren.size() == 2);
    // a rename moves the entry to the end under the new name
    ProtoCrewMember jeb(ProtoCrewMember::KerbalType::Crew, "Jebediah Kerman");
    GameEvents::onKerbalNameChanged.Fire(&jeb, std::string("Jebediah Kerman"), std::string("Jeb Kerman"));
    CHECK_FALSE(scen.ContainsCrew("Jebediah Kerman"));
    CHECK(scen.ContainsCrew("Jeb Kerman"));
    CHECK(scen.Inventories().back().first == "Jeb Kerman");
    // OnSave: roster[name].SaveInventory(module)
    auto crew = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Jeb Kerman");
    KerbalInventoryScenario::findCrew = [&](const std::string& n) -> std::shared_ptr<ProtoCrewMember> { return n == "Jeb Kerman" ? crew : nullptr; };
    REQUIRE(b.inv->StoreCargoPartAtSlot(&Prefab(g, "evaChute"), 0));
    // KerbalInventoryScenario.Update calls ModuleInventoryPart.Update, which dereferences UIPartActionControllerInventory.Instance in the editor
    CHECK_THROWS_AS(scen.Update(), NullReferenceException);
    ConfigNode unused("SCENARIO");
    UIPartActionControllerInventory ctrl;
    scen.Update();
    scen.OnSave(unused);
    REQUIRE(crew->inventoryNode != nullptr);
    CHECK(crew->inventoryNode->name == "INVENTORY");
    // Remove: the entry and the child object go
    scen.RemoveKerbalInventoryInstance("Jeb Kerman");
    CHECK_FALSE(scen.ContainsCrew("Jeb Kerman"));
    scen.RemoveKerbalInventoryInstance("nobody");  // silent
    // sic: a rename only re-keys the dictionary (cs:88), the GameObject keeps its old name, so Remove (cs:118) finds no child to destroy
    CHECK(KerbalInventoryScenario::kerbalInventoryGameObjectChildren == std::vector<std::string>{"Jebediah Kerman", "Bill Kerman"});
    scen.OnDestroy();
    CHECK(KerbalInventoryScenario::Instance == nullptr);
    CHECK(KerbalInventoryScenario::kerbalInventoryGameObjectChildren.empty());
}

// ProtoCrewMember.cs:348 / 761 / 775 / 789 / 810 / 831 through the installed glue
TEST_CASE("WP-33 kerbal inventory glue: default inventory, InventoryCosts / InventoryMass / ResourceMass, SaveInventory") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    SceneScope scene(GameScenes::EDITOR);
    KerbalInventory::Install();
    auto jeb = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Jebediah Kerman");
    // SetDefaultInventory: the kerbalEVA prefab's SaveDefault -> the two DEFAULTPARTS (evaChute, evaJetpack)
    ConfigNode::NodePtr inv = jeb->InventoryNode();
    REQUIRE(inv != nullptr);
    ConfigNode* stored = inv->GetNode("STOREDPARTS");
    REQUIRE(stored != nullptr);
    CHECK(stored->GetNodes("STOREDPART").size() == 2);
    ModuleInventoryPart* m = KerbalInventory::ModuleFor(*jeb);
    REQUIRE(m != nullptr);
    CHECK(m->kerbalMode());
    CHECK(m->storedParts.Count() == 2);
    CHECK(KerbalInventory::ModuleFor(*jeb) == m);  // cached per kerbal
    CHECK(Bits(jeb->InventoryMass()) == Bits(m->GetModuleMass(0.0f, ModifierStagingSituation::CURRENT)));
    CHECK(Bits(jeb->InventoryCosts()) == Bits(m->GetModuleCost(0.0f, ModifierStagingSituation::CURRENT)));
    CHECK(jeb->InventoryMass() > 0.0f);
    CHECK(jeb->InventoryCosts() > 0.0f);
    // SaveInventory (:831): the node is rebuilt from the module; tourists keep an empty one
    jeb->SaveInventory(m);
    CHECK(jeb->inventoryNode->GetNode("STOREDPARTS")->GetNodes("STOREDPART").size() == 2);
    ProtoCrewMember tourist(ProtoCrewMember::KerbalType::Tourist, "Tourist Kerman");
    tourist.SaveInventory(m);
    CHECK_FALSE(tourist.inventoryNode->HasNode("STOREDPARTS"));
    CHECK(tourist.InventoryMass() == 0.0f);
    CHECK(tourist.InventoryCosts() == 0.0f);
    CHECK(tourist.ResourceMass() == 0.0f);
    // OnModuleInventoryChanged of a kerbal-mode module writes the kerbal's node
    jeb->inventoryNode.reset();
    m->ClearPartAtSlot(0);
    REQUIRE(jeb->inventoryNode != nullptr);
    CHECK(jeb->inventoryNode->GetNode("STOREDPARTS")->GetNodes("STOREDPART").size() == 1);
    KerbalInventory::Uninstall();
}

// PartModule.cs:960 OnStoredInInventory, dispatched by ModuleInventoryPart.cs:733 StoreCargoPartAtSlot(Part, slot) to every module of the stored part
TEST_CASE("WP-33 StoreCargoPartAtSlot calls OnStoredInInventory on the part's modules") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    SceneScope scene(GameScenes::EDITOR);
    // a stock cargo part with ModuleAnimateGeneric (ModuleAnimateGeneric.cs:358: animSwitch = true, animTime = 0)
    Part* animated = nullptr;
    for (const auto& ap : g.data.parts.loadedParts) {
        if (!ap->partPrefab || ap->partPrefab->FindModuleOfClass("ModuleCargoPart") == nullptr) continue;
        PartModule* m = ap->partPrefab->FindModuleOfClass("ModuleAnimateGeneric");
        if (m == nullptr) continue;
        animated = ap->partPrefab.get();
        break;
    }
    REQUIRE(animated != nullptr);
    PartModule* anim = animated->FindModuleOfClass("ModuleAnimateGeneric");
    anim->Set<bool>("animSwitch", false);
    anim->Set<float>("animTime", 0.5f);
    Host h(g, "cargoContainer");
    REQUIRE(h.inv->StoreCargoPartAtSlot(animated, 0));
    CHECK(anim->Get<bool>("animSwitch"));
    CHECK(anim->Get<float>("animTime") == 0.0f);
}
