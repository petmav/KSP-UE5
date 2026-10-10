// PartCrewManifest.cs and VesselCrewManifest.cs (+ KerbalRoster.DefaultCrewForVessel); see include/ksp/kerbals/crew_manifest.hpp.
#include "ksp/kerbals/crew_manifest.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/vessel/vessel_types.hpp"

namespace ksp {

using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace {
// HighLogic.CurrentGame.CrewRoster (NullReferenceException when there is no game)
KerbalRoster& CurrentRoster() {
    Game* g = SaveEnvironment::Current().currentGame;
    if (g == nullptr || !g->CrewRoster) throw NullReferenceException("HighLogic.CurrentGame.CrewRoster is null");
    return *g->CrewRoster;
}
bool LoadedSceneIsEditor() { return SaveEnvironment::Current().loadedScene == GameScenes::EDITOR; }
int CrewCapacityOf(const AvailablePart* info) {
    if (info == nullptr || !info->partPrefab) throw NullReferenceException("PartCrewManifest.partInfo.partPrefab is null");
    return info->partPrefab->fields.CrewCapacity;
}
std::string PartTitle(const AvailablePart* info) {
    if (info == nullptr) throw NullReferenceException("PartCrewManifest.partInfo is null");
    return info->title;
}
// Mono's N2 on a float: culture en-US group separators
std::string FormatN2(double v) {
    std::string f = dotnet::FormatDoubleF(v, 2);
    const size_t dot = f.find('.');
    std::string intPart = f.substr(0, dot == std::string::npos ? f.size() : dot);
    std::string rest = dot == std::string::npos ? "" : f.substr(dot);
    const bool neg = !intPart.empty() && intPart[0] == '-';
    if (neg) intPart.erase(0, 1);
    for (int i = static_cast<int>(intPart.size()) - 3; i > 0; i -= 3) intPart.insert(static_cast<size_t>(i), ",");
    return (neg ? "-" : "") + intPart + rest;
}
}  // namespace

// ============================================== PartCrewManifest ====================================================

const AvailablePart* PartCrewManifest::GetPartInfoByName(const std::string& name) {
    const SaveEnvironment& env = SaveEnvironment::Current();
    if (env.getPartInfoByName) return env.getPartInfoByName(name);
    if (PartLoader* pl = PartLoader::Instance()) return pl->getPartInfoByName(name);
    return nullptr;
}

// PartCrewManifest.cs:25
std::shared_ptr<PartCrewManifest> PartCrewManifest::FromConfigNode(const ConfigNode& node, VesselCrewManifest* v) {
    auto partCrewManifest = std::make_shared<PartCrewManifest>(v);
    const std::string* part = node.GetValuePtr("part");
    if (part == nullptr) throw NullReferenceException("PartCrewManifest.FromConfigNode: no part value");
    const std::vector<std::string> array = dotnet::Split(*part, "_", false);
    partCrewManifest->partInfo_ = GetPartInfoByName(array[0]);
    if (partCrewManifest->partInfo_ == nullptr) return nullptr;
    const int crewCapacity = CrewCapacityOf(partCrewManifest->partInfo_);
    partCrewManifest->partCrew.assign(static_cast<size_t>(crewCapacity), std::string());
    if (array.size() < 2) throw std::out_of_range("Index was outside the bounds of the array.");
    uint32_t id = 0;
    if (!dotnet::TryParseUInt32(array[1], id)) throw dotnet::FormatException("Input string was not in a correct format.");
    partCrewManifest->partID_ = id;
    for (int i = 0; i < crewCapacity; ++i) partCrewManifest->partCrew[static_cast<size_t>(i)] = "";
    return partCrewManifest;
}

// PartCrewManifest.cs:44
std::shared_ptr<PartCrewManifest> PartCrewManifest::CloneOf(const PartCrewManifest& original, VesselCrewManifest* vcm,
                                                            bool blank) {
    auto partCrewManifest = std::make_shared<PartCrewManifest>(vcm);
    partCrewManifest->partInfo_ = original.partInfo_;
    const size_t num = original.partCrew.size();
    partCrewManifest->partCrew.assign(num, std::string());
    partCrewManifest->partID_ = original.partID_;
    for (size_t i = 0; i < num; ++i) {
        if (!blank) {
            partCrewManifest->partCrew[i] = original.partCrew[i];
            vcm->SetCrewMember(partCrewManifest->partCrew[i], partCrewManifest);
        } else {
            partCrewManifest->partCrew[i] = "";
        }
    }
    return partCrewManifest;
}

// PartCrewManifest.cs:64 (sic: seatIndex <= CrewCapacity admits one index too many: partCrew[capacity] then throws)
void PartCrewManifest::AddCrewToSeat(const PCM& crew, int seatIndex) {
    const int cap = CrewCapacityOf(partInfo_);
    if (seatIndex >= 0 && seatIndex <= cap) {
        if (seatIndex >= static_cast<int>(partCrew.size())) throw std::out_of_range("Index was outside the bounds of the array.");
        if (partCrew[static_cast<size_t>(seatIndex)] != std::string()) {
            log::Error("[PartCrewManifest Error]: seat " + dotnet::FormatInt64(seatIndex) + " on " + partInfo_->name +
                       " was occupied already.");
        }
        partCrew[static_cast<size_t>(seatIndex)] = crew->name().value_or("");
        if (vcm == nullptr) throw NullReferenceException("PartCrewManifest.vcm is null");
        vcm->SetCrewMember(crew->name().value_or(""), shared_from_this());
        Game* g = SaveEnvironment::Current().currentGame;
        bool persistFirst = false;
        if (g != nullptr) {
            if (!g->Parameters) throw NullReferenceException("HighLogic.CurrentGame.Parameters is null");
            persistFirst = !g->Parameters->Difficulty.persistKerbalInventories;
        }
        // `crew.InventoryNode == null`: the getter creates the default inventory when it is null (so it is never null here)
        if (persistFirst || crew->InventoryNode() == nullptr) crew->SetDefaultInventory();
        if (LoadedSceneIsEditor()) GE::onEditorShipCrewModified.Fire(vcm);
        return;
    }
    throw std::out_of_range("[PartCrewManifest Error]: seat index out of range: i = " + dotnet::FormatInt64(seatIndex) +
                            " while " + partInfo_->name + " has " + dotnet::FormatInt64(cap) + " seats");
}

// PartCrewManifest.cs:87
void PartCrewManifest::RemoveCrewFromSeat(int seatIndex) {
    const int cap = CrewCapacityOf(partInfo_);
    if (seatIndex >= 0 && seatIndex <= cap) {
        if (seatIndex >= static_cast<int>(partCrew.size())) throw std::out_of_range("Index was outside the bounds of the array.");
        const std::string cName = partCrew[static_cast<size_t>(seatIndex)];
        partCrew[static_cast<size_t>(seatIndex)] = std::string();
        if (vcm == nullptr) throw NullReferenceException("PartCrewManifest.vcm is null");
        vcm->RemoveCrewMember(cName);
        if (LoadedSceneIsEditor()) GE::onEditorShipCrewModified.Fire(vcm);
        return;
    }
    throw std::out_of_range("[PartCrewManifest Error]: seat index out of range: i = " + dotnet::FormatInt64(seatIndex) +
                            " while part has " + dotnet::FormatInt64(cap) + " seats");
}

// PartCrewManifest.cs:103
void PartCrewManifest::RemoveCrew(const std::string& name) {
    const int crewSeat = GetCrewSeat(name);
    if (crewSeat != -1) RemoveCrewFromSeat(crewSeat);
}

// PartCrewManifest.cs:119
std::vector<PartCrewManifest::PCM> PartCrewManifest::GetPartCrew() {
    std::vector<PCM> crewArray(partCrew.size());
    GetPartCrew(crewArray);
    return crewArray;
}

// PartCrewManifest.cs:119
void PartCrewManifest::GetPartCrew(std::vector<PCM>& crewArray) {
    if (crewArray.size() != partCrew.size()) crewArray.assign(partCrew.size(), nullptr);
    for (size_t i = 0; i < partCrew.size(); ++i) {
        if (partCrew[i] != std::string())
            crewArray[i] = CurrentRoster()[partCrew[i]];
        else
            crewArray[i] = nullptr;
    }
}

// PartCrewManifest.cs:139
bool PartCrewManifest::Contains(const PCM& crew) { return GetCrewSeat(crew->name().value_or("")) >= 0; }

// PartCrewManifest.cs:149
int PartCrewManifest::GetCrewSeat(const ProtoCrewMember& crew) { return GetCrewSeat(crew.name().value_or("")); }

// PartCrewManifest.cs:149
int PartCrewManifest::GetCrewSeat(const std::string& name) {
    const int num = static_cast<int>(partCrew.size());
    for (int i = 0; i < num; ++i)
        if (partCrew[static_cast<size_t>(i)] == name) return i;
    return -1;
}

// PartCrewManifest.cs:184
bool PartCrewManifest::AllSeatsEmpty() { return AllSeatsEmpty(CurrentRoster()); }

// PartCrewManifest.cs:184
bool PartCrewManifest::AllSeatsEmpty(KerbalRoster& roster) {
    for (size_t i = 0; i < partCrew.size(); ++i)
        if (!partCrew[i].empty() && roster[partCrew[i]] != nullptr) return false;
    return true;
}

// PartCrewManifest.cs:209
bool PartCrewManifest::AnySeatTaken() { return AnySeatTaken(CurrentRoster()); }

// PartCrewManifest.cs:209
bool PartCrewManifest::AnySeatTaken(KerbalRoster& roster) {
    for (size_t i = 0; i < partCrew.size(); ++i)
        if (!partCrew[i].empty() && roster[partCrew[i]] != nullptr) return true;
    return false;
}

// PartCrewManifest.cs:234
int PartCrewManifest::CountCrewType(ProtoCrewMember_KerbalType type) { return CountCrewType(type, CurrentRoster()); }

// PartCrewManifest.cs:234
int PartCrewManifest::CountCrewType(ProtoCrewMember_KerbalType type, KerbalRoster& roster) {
    int num = 0;
    for (size_t i = 0; i < partCrew.size(); ++i) {
        if (partCrew[i] == std::string()) continue;
        PCM pcm = roster[partCrew[i]];
        if (pcm != nullptr && pcm->type() == type) num++;
    }
    return num;
}

// PartCrewManifest.cs:257
int PartCrewManifest::CountCrewNotType(ProtoCrewMember_KerbalType type) { return CountCrewNotType(type, CurrentRoster()); }

// PartCrewManifest.cs:257
int PartCrewManifest::CountCrewNotType(ProtoCrewMember_KerbalType type, KerbalRoster& roster) {
    int num = 0;
    for (size_t i = 0; i < partCrew.size(); ++i) {
        if (partCrew[i] == std::string()) continue;
        PCM pcm = roster[partCrew[i]];
        if (pcm != nullptr && pcm->type() != type) num++;
    }
    return num;
}

// ============================================== VesselCrewManifest ==================================================

// VesselCrewManifest.cs:31
std::shared_ptr<VesselCrewManifest> VesselCrewManifest::FromConfigNode(const ConfigNode& craftNode) {
    auto vesselCrewManifest = std::make_shared<VesselCrewManifest>();
    const int count = craftNode.nodes().Count();
    for (int i = 0; i < count; ++i) {
        const ConfigNode& configNode = *craftNode.nodes()[i];
        if (configNode.name != "PART") continue;
        PartManifest partCrewManifest = PartCrewManifest::FromConfigNode(configNode, vesselCrewManifest.get());
        if (partCrewManifest != nullptr) vesselCrewManifest->SetPartManifest(partCrewManifest->PartID(), partCrewManifest);
    }
    return vesselCrewManifest;
}

// VesselCrewManifest.cs:50
std::shared_ptr<VesselCrewManifest> VesselCrewManifest::CloneOf(VesselCrewManifest& original, bool blank) {
    auto vesselCrewManifest = std::make_shared<VesselCrewManifest>();
    const int count = static_cast<int>(original.partManifests_.size());
    for (int i = 0; i < count; ++i) {
        PartManifest partCrewManifest = original.partManifests_[static_cast<size_t>(i)];
        vesselCrewManifest->SetPartManifest(partCrewManifest->PartID(),
                                            PartCrewManifest::CloneOf(*partCrewManifest, vesselCrewManifest.get(), blank));
    }
    return vesselCrewManifest;
}

// VesselCrewManifest.cs:62 (sic: a null filter removes nothing from m1 and merges nothing from m2)
void VesselCrewManifest::MergeInto(VesselCrewManifest& m1, VesselCrewManifest& m2,
                                   const std::function<bool(PartCrewManifest&)>& inclusionFilter) {
    int count = static_cast<int>(m1.partManifests_.size());
    while (count-- > 0) {
        PartManifest partCrewManifest = m1.partManifests_[static_cast<size_t>(count)];
        if (inclusionFilter && !inclusionFilter(*partCrewManifest)) m1.RemovePartManifest(partCrewManifest);
    }
    const int count2 = static_cast<int>(m2.partManifests_.size());
    for (int i = 0; i < count2; ++i) {
        PartManifest partCrewManifest = m2.partManifests_[static_cast<size_t>(i)];
        if (inclusionFilter && inclusionFilter(*partCrewManifest))
            m1.SetPartManifestNoOverwrite(partCrewManifest->PartID(), partCrewManifest);
    }
}

// VesselCrewManifest.cs:86
std::vector<VesselCrewManifest::PCM> VesselCrewManifest::GetAllCrew(bool includeNulls) {
    return GetAllCrew(includeNulls, CurrentRoster());
}

// VesselCrewManifest.cs:91
std::vector<VesselCrewManifest::PCM> VesselCrewManifest::GetAllCrew(bool includeNulls, KerbalRoster& roster) {
    std::vector<PCM> list;
    const int count = static_cast<int>(partManifests_.size());
    for (int i = 0; i < count; ++i) {
        PartCrewManifest& partCrewManifest = *partManifests_[static_cast<size_t>(i)];
        const int num = static_cast<int>(partCrewManifest.partCrew.size());
        for (int j = 0; j < num; ++j) {
            if (!partCrewManifest.partCrew[static_cast<size_t>(j)].empty() || includeNulls) {
                PCM protoCrewMember = roster[partCrewManifest.partCrew[static_cast<size_t>(j)]];
                if (protoCrewMember != nullptr || includeNulls) list.push_back(protoCrewMember);
            }
        }
    }
    return list;
}

// VesselCrewManifest.cs:112
float VesselCrewManifest::GetCrewInventoryCosts(KerbalRoster* roster) {
    float num = 0.0f;
    Game* g = SaveEnvironment::Current().currentGame;
    if (roster != nullptr || (g != nullptr && g->CrewRoster)) {
        std::vector<PCM> allCrew = GetAllCrew(false, roster != nullptr ? *roster : *g->CrewRoster);
        for (size_t i = 0; i < allCrew.size(); ++i) num = f32store(fx(num) + fx(allCrew[i]->InventoryCosts()));
    }
    return num;
}

// VesselCrewManifest.cs:131
float VesselCrewManifest::GetCrewInventoryMass(KerbalRoster* roster) {
    float num = 0.0f;
    Game* g = SaveEnvironment::Current().currentGame;
    if (roster != nullptr || (g != nullptr && g->CrewRoster)) {
        std::vector<PCM> allCrew = GetAllCrew(false, roster != nullptr ? *roster : *g->CrewRoster);
        for (size_t i = 0; i < allCrew.size(); ++i) num = f32store(fx(num) + fx(allCrew[i]->InventoryMass()));
    }
    return num;
}

// VesselCrewManifest.cs:150
float VesselCrewManifest::GetCrewResourceMass(KerbalRoster* roster) {
    float num = 0.0f;
    Game* g = SaveEnvironment::Current().currentGame;
    if (roster != nullptr || (g != nullptr && g->CrewRoster)) {
        std::vector<PCM> allCrew = GetAllCrew(false, roster != nullptr ? *roster : *g->CrewRoster);
        for (size_t i = 0; i < allCrew.size(); ++i) num = f32store(fx(num) + fx(allCrew[i]->ResourceMass()));
    }
    return num;
}

// VesselCrewManifest.cs:164
bool VesselCrewManifest::Contains(const PCM& crew) const { return crewLookup_.count(crew->name().value_or("")) != 0; }

// VesselCrewManifest.cs:169
std::vector<VesselCrewManifest::PartManifest> VesselCrewManifest::GetCrewableParts() {
    std::vector<PartManifest> list(partManifests_);
    int count = static_cast<int>(list.size());
    while (count-- > 0)
        if (CrewCapacityOf(list[static_cast<size_t>(count)]->PartInfo()) == 0) list.erase(list.begin() + count);
    return list;
}

// VesselCrewManifest.cs:182
void VesselCrewManifest::SetPartManifestNoOverwrite(uint32_t id, const PartManifest& newPCMtoUse) {
    PartManifest value;
    auto it = partLookup_.find(id);
    if (it != partLookup_.end()) {
        value = it->second;
        if (value == newPCMtoUse) {
            newPCMtoUse->vcm = this;
            return;
        }
        int num = static_cast<int>(value->partCrew.size());
        while (num-- > 0) {
            if (!value->partCrew[static_cast<size_t>(num)].empty()) {
                crewLookup_.erase(value->partCrew[static_cast<size_t>(num)]);
                crewCountOptimizedForFloat_ = static_cast<float>(crewLookup_.size());
            }
        }
    }
    newPCMtoUse->vcm = this;
    partLookup_[id] = newPCMtoUse;
    if (value != nullptr) {
        auto pos = std::find(partManifests_.begin(), partManifests_.end(), value);
        if (pos != partManifests_.end()) partManifests_.erase(pos);
    }
    partManifests_.push_back(newPCMtoUse);
    int num2 = static_cast<int>(newPCMtoUse->partCrew.size());
    while (num2-- > 0) {
        std::string& seat = newPCMtoUse->partCrew[static_cast<size_t>(num2)];
        if (!seat.empty()) {
            if (crewLookup_.count(seat) != 0) {
                seat = std::string();
                crewCountOptimizedForFloat_ = static_cast<float>(crewLookup_.size());
            } else {
                crewLookup_[seat] = newPCMtoUse;
                crewCountOptimizedForFloat_ = static_cast<float>(crewLookup_.size());
            }
        }
    }
}

// VesselCrewManifest.cs:229
void VesselCrewManifest::SetPartManifest(uint32_t id, const PartManifest& newPCMtoUse) {
    PartManifest value;
    auto it = partLookup_.find(id);
    if (it != partLookup_.end()) {
        value = it->second;
        if (value == newPCMtoUse) {
            newPCMtoUse->vcm = this;
            return;
        }
        int num = static_cast<int>(value->partCrew.size());
        while (num-- > 0) {
            if (!value->partCrew[static_cast<size_t>(num)].empty()) {
                crewLookup_.erase(value->partCrew[static_cast<size_t>(num)]);
                crewCountOptimizedForFloat_ = static_cast<float>(crewLookup_.size());
            }
        }
    }
    newPCMtoUse->vcm = this;
    partLookup_[id] = newPCMtoUse;
    if (value != nullptr) {
        auto pos = std::find(partManifests_.begin(), partManifests_.end(), value);
        if (pos != partManifests_.end()) partManifests_.erase(pos);
    }
    partManifests_.push_back(newPCMtoUse);
    int num2 = static_cast<int>(newPCMtoUse->partCrew.size());
    while (num2-- > 0) {
        const std::string seat = newPCMtoUse->partCrew[static_cast<size_t>(num2)];
        if (!seat.empty()) {
            auto found = crewLookup_.find(seat);
            if (found != crewLookup_.end() && found->second != newPCMtoUse) {
                PartManifest holder = found->second;  // `value` of TryGetValue
                holder->RemoveCrew(seat);
            }
            crewLookup_[seat] = newPCMtoUse;
            crewCountOptimizedForFloat_ = static_cast<float>(crewLookup_.size());
        }
    }
}

// VesselCrewManifest.cs:271
void VesselCrewManifest::UpdatePartManifest(uint32_t id, const PartManifest& referencePCM) {
    auto it = partLookup_.find(id);
    if (it == partLookup_.end()) return;
    PartManifest value = it->second;
    if (value == referencePCM) {
        value->vcm = this;
        return;
    }
    int num = static_cast<int>(referencePCM->partCrew.size());
    while (num-- > 0) {
        const std::string seat = referencePCM->partCrew[static_cast<size_t>(num)];
        if (!seat.empty()) {
            auto found = crewLookup_.find(seat);
            if (found != crewLookup_.end() && found->second != value) {
                PartManifest holder = found->second;
                holder->RemoveCrew(seat);
            }
            if (static_cast<size_t>(num) >= value->partCrew.size()) throw std::out_of_range("Index was outside the bounds of the array.");
            value->partCrew[static_cast<size_t>(num)] = referencePCM->partCrew[static_cast<size_t>(num)];
            crewLookup_[value->partCrew[static_cast<size_t>(num)]] = value;
            crewCountOptimizedForFloat_ = static_cast<float>(crewLookup_.size());
        }
    }
}

// VesselCrewManifest.cs:300
void VesselCrewManifest::RemovePartManifest(const PartManifest& pcm) {
    int num = static_cast<int>(pcm->partCrew.size());
    while (num-- > 0) {
        if (!pcm->partCrew[static_cast<size_t>(num)].empty()) {
            crewLookup_.erase(pcm->partCrew[static_cast<size_t>(num)]);
            crewCountOptimizedForFloat_ = static_cast<float>(crewLookup_.size());
        }
    }
    partLookup_.erase(pcm->PartID());
    auto pos = std::find(partManifests_.begin(), partManifests_.end(), pcm);
    if (pos != partManifests_.end()) partManifests_.erase(pos);
}

// VesselCrewManifest.cs:315
void VesselCrewManifest::SetCrewMember(const std::string& cName, const PartManifest& pcm) {
    PartManifest value;
    auto it = crewLookup_.find(cName);
    if (it != crewLookup_.end()) {
        value = it->second;
        if (value != pcm) value->RemoveCrew(cName);
    }
    if (pcm != value) {
        crewLookup_[cName] = pcm;
        crewCountOptimizedForFloat_ = static_cast<float>(crewLookup_.size());
    }
}

// VesselCrewManifest.cs:329
void VesselCrewManifest::RemoveCrewMember(const std::string& cName) {
    auto it = crewLookup_.find(cName);
    if (it != crewLookup_.end()) {
        PartManifest value = it->second;
        crewLookup_.erase(it);
        value->RemoveCrew(cName);
        crewCountOptimizedForFloat_ = static_cast<float>(crewLookup_.size());
    }
}

// VesselCrewManifest.cs:340
VesselCrewManifest::PartManifest VesselCrewManifest::GetPartCrewManifest(uint32_t id) {
    auto it = partLookup_.find(id);
    return it != partLookup_.end() ? it->second : nullptr;
}

// VesselCrewManifest.cs:349
VesselCrewManifest::PartManifest VesselCrewManifest::GetPartForCrew(const ProtoCrewMember& crew) {
    auto it = crewLookup_.find(crew.name().value_or(""));
    return it != crewLookup_.end() ? it->second : nullptr;
}

// VesselCrewManifest.cs:358 (Dictionary.Values order: here the partManifests order)
void VesselCrewManifest::DebugManifest() {
    for (const PartManifest& value : partManifests_) {
        const std::vector<PCM> crew = value->GetPartCrew();
        for (size_t i = 0; i < crew.size(); ++i) {
            if (crew[i] != nullptr)
                log::Info("Crewmember " + crew[i]->name().value_or("") + " is on seat #" + dotnet::FormatInt64(static_cast<int64_t>(i)) +
                          " on " + PartTitle(value->PartInfo()));
            else
                log::Info("Seat #" + dotnet::FormatInt64(static_cast<int64_t>(i)) + " on " + PartTitle(value->PartInfo()) + " is empty");
        }
    }
}

// VesselCrewManifest.cs:376
void VesselCrewManifest::Filter(const std::function<bool(PartCrewManifest&)>& inclusionFilter) {
    int count = static_cast<int>(partManifests_.size());
    while (count-- > 0) {
        PartManifest partCrewManifest = partManifests_[static_cast<size_t>(count)];
        if (!inclusionFilter(*partCrewManifest)) RemovePartManifest(partCrewManifest);
    }
}

// VesselCrewManifest.cs:394
bool VesselCrewManifest::HasAnyCrew() { return HasAnyCrew(CurrentRoster()); }

// VesselCrewManifest.cs:394
bool VesselCrewManifest::HasAnyCrew(KerbalRoster& roster) {
    int count = static_cast<int>(partManifests_.size());
    while (count-- > 0) {
        PartManifest partCrewManifest = partManifests_[static_cast<size_t>(count)];
        if (CrewCapacityOf(partCrewManifest->PartInfo()) <= 0) continue;
        int num = static_cast<int>(partCrewManifest->partCrew.size());
        while (num-- > 0) {
            if (!partCrewManifest->partCrew[static_cast<size_t>(num)].empty()) {
                if (roster[partCrewManifest->partCrew[static_cast<size_t>(num)]] != nullptr) return true;
                partCrewManifest->RemoveCrewFromSeat(num);
            }
        }
    }
    return false;
}

// VesselCrewManifest.cs:425
bool VesselCrewManifest::AnyCrewInState(ProtoCrewMember_RosterStatus state, bool notInState) {
    return AnyCrewInState(state, CurrentRoster(), notInState);
}

// VesselCrewManifest.cs:425
bool VesselCrewManifest::AnyCrewInState(ProtoCrewMember_RosterStatus state, KerbalRoster& roster, bool notInState) {
    int count = static_cast<int>(partManifests_.size());
    while (count-- > 0) {
        PartManifest partCrewManifest = partManifests_[static_cast<size_t>(count)];
        int num = static_cast<int>(partCrewManifest->partCrew.size());
        while (num-- > 0) {
            if (partCrewManifest->partCrew[static_cast<size_t>(num)].empty()) continue;
            PCM protoCrewMember = roster[partCrewManifest->partCrew[static_cast<size_t>(num)]];
            if (protoCrewMember == nullptr) {
                partCrewManifest->RemoveCrewFromSeat(num);
            } else if (notInState) {
                if (protoCrewMember->rosterStatus() != state) return true;
            } else if (protoCrewMember->rosterStatus() == state) {
                return true;
            }
        }
    }
    return false;
}

// VesselCrewManifest.cs:464
bool VesselCrewManifest::AnyCrewWithTrait(const std::string& trait, bool noCrewWithTrait) {
    return AnyCrewWithTrait(trait, CurrentRoster(), noCrewWithTrait);
}

// VesselCrewManifest.cs:464
bool VesselCrewManifest::AnyCrewWithTrait(const std::string& trait, KerbalRoster& roster, bool noCrewWithTrait) {
    int count = static_cast<int>(partManifests_.size());
    while (count-- > 0) {
        PartManifest partCrewManifest = partManifests_[static_cast<size_t>(count)];
        int num = static_cast<int>(partCrewManifest->partCrew.size());
        while (num-- > 0) {
            if (!partCrewManifest->partCrew[static_cast<size_t>(num)].empty()) {
                PCM protoCrewMember = roster[partCrewManifest->partCrew[static_cast<size_t>(num)]];
                if (protoCrewMember == nullptr)
                    partCrewManifest->RemoveCrewFromSeat(num);
                else if (!noCrewWithTrait && protoCrewMember->trait == trait)
                    return true;
            }
        }
    }
    return !noCrewWithTrait;
}

// VesselCrewManifest.cs:495
void VesselCrewManifest::AssignCrewToVessel(ShipConstruct& ship) { AssignCrewToVessel(ship, CurrentRoster()); }

// VesselCrewManifest.cs:495
void VesselCrewManifest::AssignCrewToVessel(ShipConstruct& ship, KerbalRoster& roster) {
    const int count = static_cast<int>(partManifests_.size());
    for (int i = 0; i < count; ++i) {
        PartManifest pcm = partManifests_[static_cast<size_t>(i)];
        const int num = static_cast<int>(pcm->partCrew.size());
        if (num == 0) continue;
        Part* part = nullptr;
        for (Part* prt : ship.liveParts)
            if (prt->fields.craftID == pcm->PartID()) {
                part = prt;
                break;
            }
        if (part != nullptr) {
            for (int j = 0; j < num; ++j) {
                PCM protoCrewMember = roster[pcm->partCrew[static_cast<size_t>(j)]];
                if (protoCrewMember != nullptr) {
                    part->AddCrewmemberAt(protoCrewMember, j);
                    log::Info("Crewmember " + protoCrewMember->name().value_or("") + " assigned to " +
                              PartTitle(part->partInfo) + ", seat # " + dotnet::FormatInt64(j) + " (crew seat index: " +
                              dotnet::FormatInt64(protoCrewMember->seatIdx) + ")");
                }
            }
        } else {
            log::Warning("[CrewAssignment]: No " + pcm->PartInfo()->name + " with id " + dotnet::FormatUInt64(pcm->PartID()) +
                         " was found in " + ship.shipName);
        }
    }
}

// VesselCrewManifest.cs:527
std::shared_ptr<VesselCrewManifest> VesselCrewManifest::UpdateCrewForVessel(
    const ConfigNode& vesselNode, const std::function<bool(PartCrewManifest&)>& persistFilter) {
    std::shared_ptr<VesselCrewManifest> vesselCrewManifest = FromConfigNode(vesselNode);
    const int count = static_cast<int>(partManifests_.size());
    for (int i = 0; i < count; ++i) {
        PartManifest partCrewManifest = partManifests_[static_cast<size_t>(i)];
        if (CrewCapacityOf(partCrewManifest->PartInfo()) > 0 && persistFilter && persistFilter(*partCrewManifest))
            vesselCrewManifest->UpdatePartManifest(partCrewManifest->PartID(), partCrewManifest);
    }
    return vesselCrewManifest;
}

// VesselCrewManifest.cs:542
void VesselCrewManifest::AddCrewMembers(std::vector<Expansions::Missions::Crew>& vesselCrew, KerbalRoster& roster) {
    int count = static_cast<int>(vesselCrew.size());
    while (count-- > 0) {
        Expansions::Missions::Crew& crew = vesselCrew[static_cast<size_t>(count)];
        PartManifest partCrewManifest = GetPartCrewManifest(crew.partPersistentID);
        if (partCrewManifest != nullptr) {
            int count2 = static_cast<int>(crew.crewNames.size());
            while (count2-- > 0) {
                PCM protoCrewMember = roster[crew.crewNames[static_cast<size_t>(count2)]];
                if (protoCrewMember != nullptr) {
                    if (protoCrewMember->seatIdx >= 0 && protoCrewMember->seatIdx <= CrewCapacityOf(partCrewManifest->PartInfo())) {
                        partCrewManifest->AddCrewToSeat(protoCrewMember, protoCrewMember->seatIdx);
                        continue;
                    }
                    protoCrewMember->set_rosterStatus(ProtoCrewMember::RosterStatus::Available);
                    crew.crewNames.erase(crew.crewNames.begin() + count2);
                    log::Info("[CrewManifest]: Unable to assign " + protoCrewMember->name().value_or("") + " to part " +
                              partCrewManifest->PartInfo()->name + ". Set kerbal roster status back to Available.");
                }
            }
            continue;
        }
        int count3 = static_cast<int>(crew.crewNames.size());
        while (count3-- > 0) {
            PCM protoCrewMember2 = roster[crew.crewNames[static_cast<size_t>(count3)]];
            if (protoCrewMember2 != nullptr) {
                protoCrewMember2->set_rosterStatus(ProtoCrewMember::RosterStatus::Available);
                crew.crewNames.erase(crew.crewNames.begin() + count3);
            }
        }
        vesselCrew.erase(vesselCrew.begin() + count);
    }
}

// ================================================= KerbalRoster =====================================================

// KerbalRoster.cs:491
std::shared_ptr<VesselCrewManifest> KerbalRoster::DefaultCrewForVessel(const ConfigNode& vesselNode, VesselCrewManifest* previous,
                                                                       bool autohire, bool usePreviousVCMToFill) {
    using Env = CrewAssignmentEnvironment;
    std::shared_ptr<VesselCrewManifest> vesselCrewManifest = VesselCrewManifest::FromConfigNode(vesselNode);
    std::vector<PCM> list = Kerbals(ProtoCrewMember_KerbalType::Crew, {ProtoCrewMember_RosterStatus::Available});
    bool flag = previous == nullptr || previous->AnyCrewWithTrait(pilotTrait, true);
    const int count = static_cast<int>(vesselCrewManifest->PartManifests().size());
    auto post = [](const std::string& text, float d) {
        if (Env::postScreenMessage) Env::postScreenMessage(text, d);
    };
    for (int i = 0; i < count; ++i) {
        PartCrewManifest& partCrewManifest = *vesselCrewManifest->PartManifests()[static_cast<size_t>(i)];
        const AvailablePart* info = partCrewManifest.PartInfo();
        bool flag2 = false;
        if (info->partPrefab->fields.isControlSource == static_cast<int>(ControlLevel::NONE) && info->partPrefab->fields.CrewCapacity > 0 &&
            info->partPrefab->FindModuleOfClass("KerbalSeat") != nullptr)
            flag2 = true;
        if (!((info->partPrefab->fields.isControlSource > static_cast<int>(ControlLevel::NONE) && info->partPrefab->fields.CrewCapacity > 0) || flag2))
            continue;
        const int capacity = info->partPrefab->fields.CrewCapacity;
        Game* game = SaveEnvironment::Current().currentGame;
        if (!flag2 && static_cast<int>(list.size()) < capacity) {
            if (game == nullptr || !game->Parameters) throw NullReferenceException("HighLogic.CurrentGame.Parameters is null");
            if (game->Parameters->Difficulty.AutoHireCrews && autohire) {
                if (GetActiveCrewCount() < (Env::activeCrewLimit ? Env::activeCrewLimit() : 0)) {
                    log::Info("[Crew Assignment]: Part " + info->title + " holds " + dotnet::FormatInt64(capacity) +
                              " crew, but only " + dotnet::FormatInt64(static_cast<int64_t>(list.size())) + " are available.");
                    const int num = capacity - static_cast<int>(list.size());
                    Update(SaveUniversalTime());
                    for (int j = 0; j < num; ++j) {
                        const float cost = Env::recruitHireCost ? Env::recruitHireCost(GetActiveCrewCount()) : 0.0f;
                        if (!Env::canAfford || Env::canAfford(cost)) {
                            PCM protoCrewMember = GetNextApplicant();
                            if (protoCrewMember == nullptr) protoCrewMember = GetNewKerbal(ProtoCrewMember_KerbalType::Applicant);
                            HireApplicant(protoCrewMember);
                            list.push_back(protoCrewMember);
                            const std::string pname = protoCrewMember->name().value_or("");
                            if (Env::fundingPresent && Env::fundingPresent()) {
                                const float c2 = Env::recruitHireCost ? Env::recruitHireCost(GetActiveCrewCount() - 1) : 0.0f;
                                post(Localizer::Format("#autoLOC_21131", {"#a8ff04", pname, FormatN2(c2)}), 5.0f);
                            } else {
                                post(Localizer::Format("#autoLOC_7003260", {"#a8ff04", pname}), 5.0f);
                            }
                        } else {
                            post(Localizer::Format("#autoLOC_21144"), 5.0f);
                        }
                    }
                } else {
                    post(Localizer::Format("#autoLOC_21150"), 5.0f);
                }
            } else {
                log::Info("[Crew Assignment]: Part " + info->title + " holds " + dotnet::FormatInt64(capacity) + " crew, but only " +
                          dotnet::FormatInt64(static_cast<int64_t>(list.size())) +
                          " are available. Auto-hiring is disabled or not allowed at this point.");
            }
        }
        for (int k = 0; k < capacity; ++k) {
            if (!usePreviousVCMToFill && list.empty()) break;
            PCM protoCrewMember2;
            if (previous != nullptr) {
                if (usePreviousVCMToFill) {
                    VesselCrewManifest::PartManifest partCrewManifest2 = previous->GetPartCrewManifest(partCrewManifest.PartID());
                    if (partCrewManifest2 != nullptr) {
                        std::vector<PCM> partCrew = partCrewManifest2->GetPartCrew();
                        if (static_cast<int>(partCrew.size()) > k) protoCrewMember2 = partCrew[static_cast<size_t>(k)];
                    }
                }
                if (!flag2 && protoCrewMember2 == nullptr) {
                    auto findIf = [&](const std::function<bool(const PCM&)>& pred) -> PCM {
                        for (const PCM& c : list)
                            if (pred(c)) return c;
                        return nullptr;
                    };
                    if (flag) {
                        protoCrewMember2 = findIf([&](const PCM& c) { return c->trait == pilotTrait && !c->inactive() && !previous->Contains(c); });
                        if (protoCrewMember2 == nullptr) protoCrewMember2 = findIf([&](const PCM& c) { return !previous->Contains(c); });
                        flag = false;
                    } else {
                        protoCrewMember2 = findIf([&](const PCM& c) { return !previous->Contains(c) && !c->inactive(); });
                    }
                }
            } else if (!flag2) {
                if (flag) {
                    for (const PCM& c : list)
                        if (c->trait == pilotTrait && !c->inactive()) {
                            protoCrewMember2 = c;
                            break;
                        }
                    if (protoCrewMember2 == nullptr) {
                        if (list.empty()) throw std::out_of_range("Index was out of range. Must be non-negative and less than the size of the collection.");
                        protoCrewMember2 = list[0];
                    }
                    flag = false;
                } else {
                    for (const PCM& c : list)
                        if (!c->inactive()) {
                            protoCrewMember2 = c;
                            break;
                        }
                }
            }
            if (protoCrewMember2 != nullptr) {
                partCrewManifest.AddCrewToSeat(protoCrewMember2, k);
                list.erase(std::find(list.begin(), list.end(), protoCrewMember2));
            } else if (!flag2) {
                post(Localizer::Format("#autoLOC_21195", {dotnet::FormatInt64(k), dotnet::FormatInt64(capacity), info->title}), 5.0f);
                break;
            }
        }
        break;
    }
    return vesselCrewManifest;
}

}  // namespace ksp
