// PartResourceList.cs (WP-15): see ksp/resources/part_resource_list.hpp.
#include "ksp/resources/part_resource_list.hpp"

#include "ksp/core/log.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"

namespace ksp {

PartResource* PartResourceList::Insert(std::unique_ptr<PartResource> r, int id) {
    PartResource* raw = r.get();
    storage_.push_back(std::move(r));
    dict_.Add(id, raw);
    return raw;
}

// PartResourceList.cs:59
void PartResourceList::Initializer(Part* p) {
    valid_ = true;
    dict_ = DictionaryValueList<int, PartResource*>();
    part = p;
}

// PartResourceList.cs:65
void PartResourceList::RefListInitializer(Part* p, const PartResourceList* refList, bool simSet) {
    if (refList != nullptr && refList->IsValid()) {
        int count = refList->Count();
        for (int i = 0; i < count; i++) {
            auto partResource = std::make_unique<PartResource>(refList->At(i), simSet);
            partResource->part = p;
            const int id = partResource->info->id();
            Insert(std::move(partResource), id);
        }
    }
}

// PartResourceList.cs:79
void PartResourceList::RefreshSimulationListAmounts(const PartResourceList* refList) {
    if (refList == nullptr || !refList->IsValid()) return;
    int count = refList->Count();
    for (int i = 0; i < count; i++) {
        PartResource& ref = refList->At(i);
        PartResource* partResource = Get(ref.resourceName);
        if (partResource == nullptr) {
            auto fresh = std::make_unique<PartResource>(ref, simulationSet);
            fresh->part = ref.part;
            const int id = fresh->info->id();
            Insert(std::move(fresh), id);
        } else {
            partResource->amount = ref.amount;
            partResource->flowMode = ref.flowMode;     // _flowMode / _flowState: the fields, no events
            partResource->flowState = ref.flowState;
            partResource->simulationResource = true;
        }
    }
}

// PartResourceList.cs:120
PartResource* PartResourceList::Get(int id) const {
    PartResource* val = nullptr;
    if (dict_.TryGetValue(id, val)) return val;
    return nullptr;
}

// PartResourceList.cs:129
bool PartResourceList::GetAll(std::vector<PartResource*>& sources, int id) const {
    PartResource* partResource = Get(id);
    if (partResource != nullptr) {
        sources.push_back(partResource);
        return true;
    }
    return false;
}

// PartResourceList.cs:140
bool PartResourceList::GetAllFlowing(std::vector<PartResource*>& sources, int id) const {
    PartResource* partResource = Get(id);
    if (partResource != nullptr && partResource->flowState) {
        sources.push_back(partResource);
        return true;
    }
    return false;
}

// PartResourceList.cs:151 (both branches of `pulling` read the same two fields)
void PartResourceList::GetFlowingTotals(int id, double& amount, double& maxAmount, bool pulling) const {
    PartResource* flowing = GetFlowing(id, pulling);
    if (flowing != nullptr) {
        amount = flowing->amount;
        maxAmount = flowing->maxAmount;
    } else {
        double num = 0.0;
        maxAmount = 0.0;
        amount = num;
    }
}

// PartResourceList.cs:175
PartResource* PartResourceList::GetFlowing(int id, bool pulling) const {
    PartResource* partResource = Get(id);
    if (partResource != nullptr && partResource->flowState) {
        if (pulling) {
            if ((static_cast<int>(partResource->flowMode) & static_cast<int>(PartResourceFlowMode::Out)) > 0) return partResource;
        } else if ((static_cast<int>(partResource->flowMode) & static_cast<int>(PartResourceFlowMode::In)) > 0) {
            return partResource;
        }
    }
    return nullptr;
}

// PartResourceList.cs:195
PartResource* PartResourceList::Add(const ConfigNode& node) {
    if (!node.HasValue("name")) {
        log::Warning("ConfigNode has no value 'name'");
        return nullptr;
    }
    const std::string value = *node.GetValue("name");
    if (Contains(value)) {
        log::Warning(std::string("Part already contains ") + (simulationSet ? " simulation resource '" : " resource '") + value + "'");
        return nullptr;
    }
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    const PartResourceDefinition* definition = lib ? lib->GetDefinition(value) : nullptr;
    if (definition == nullptr) {
        log::Warning(std::string("Could not create ") + (simulationSet ? "simulation PartResource" : "PartResource") + " of type '" + value);
        return nullptr;
    }
    auto partResource = std::make_unique<PartResource>(part, simulationSet);
    partResource->SetInfo(definition);
    partResource->Load(node);
    PartResource* raw = Insert(std::move(partResource), definition->id());
    if (!simulationSet) events::GameEvents::onPartResourceListChange.Fire(part);
    return raw;
}

// PartResourceList.cs:225
PartResource* PartResourceList::Add(const PartResource& res) {
    auto partResource = std::make_unique<PartResource>(part);
    partResource->Copy(res);
    const int id = partResource->info->id();
    PartResource* raw = Insert(std::move(partResource), id);
    if (!simulationSet) events::GameEvents::onPartResourceListChange.Fire(part);
    return raw;
}

// PartResourceList.cs:237
PartResource* PartResourceList::Add(std::string_view name, double amount, double maxAmount, bool flowState, bool isTweakable, bool hideFlow,
                                    bool isVisible, PartResourceFlowMode flow) {
    if (Contains(name)) {
        log::Warning(std::string("Part already contains") + (simulationSet ? " simulation resource '" : " resource '") + std::string(name) + "'");
        return nullptr;
    }
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    const PartResourceDefinition* definition = lib ? lib->GetDefinition(name) : nullptr;
    if (definition == nullptr) {
        log::Warning(std::string("Could not create") + (simulationSet ? " simulation PartResource " : " PartResource ") + "of type '" + std::string(name));
        return nullptr;
    }
    auto partResource = std::make_unique<PartResource>(part);
    partResource->resourceName = std::string(name);
    partResource->SetInfo(definition);
    partResource->amount = amount;
    partResource->maxAmount = maxAmount;
    partResource->SetFlowState(flowState);
    partResource->isTweakable = isTweakable;
    partResource->hideFlow = hideFlow;
    partResource->isVisible = isVisible;
    partResource->SetFlowMode(flow);
    PartResource* raw = Insert(std::move(partResource), definition->id());
    if (!simulationSet) events::GameEvents::onPartResourceListChange.Fire(part);
    return raw;
}

// PartResourceList.cs:268
bool PartResourceList::Remove(const PartResource& res) {
    if (dict_.Remove(res.info->id())) {
        if (!simulationSet) events::GameEvents::onPartResourceListChange.Fire(part);
        return true;
    }
    return false;
}

// PartResourceList.cs:286
bool PartResourceList::Remove(std::string_view resName) {
    if (dict_.Remove(MonoStringHash(resName))) {
        if (!simulationSet) events::GameEvents::onPartResourceListChange.Fire(part);
        return true;
    }
    return false;
}

// PartResourceList.cs:299
bool PartResourceList::Remove(int resID) {
    if (dict_.Remove(resID)) {
        if (!simulationSet) events::GameEvents::onPartResourceListChange.Fire(part);
        return true;
    }
    return false;
}

// PartResourceList.cs:312
bool PartResourceList::HasFlowable() const {
    int count = Count();
    do {
        if (count-- <= 0) return false;
    } while (dict_.At(count)->info->resourceFlowMode() <= static_cast<int32_t>(ResourceFlowMode::NO_FLOW));
    return true;
}

// PartResourceList.cs:326
bool PartResourceList::HasFlowableUnhidden() const {
    int count = Count();
    PartResource* partResource;
    do {
        if (count-- > 0) {
            partResource = dict_.At(count);
            continue;
        }
        return false;
    } while (partResource->info->resourceFlowMode() <= static_cast<int32_t>(ResourceFlowMode::NO_FLOW) || !partResource->isVisible);
    return true;
}

// Object.Instantiate of the Part: the serialized list is cloned (see the header)
void PartResourceList::CloneFrom(const PartResourceList& source, Part* newPart) {
    valid_ = source.valid_;
    part = newPart;
    simulationSet = source.simulationSet;
    dict_ = DictionaryValueList<int, PartResource*>();
    for (const PartResource& r : source) {
        auto c = std::make_unique<PartResource>(r);
        c->part = newPart;
        const int id = c->info ? c->info->id() : MonoStringHash(c->resourceName);
        Insert(std::move(c), id);
    }
}

}  // namespace ksp
