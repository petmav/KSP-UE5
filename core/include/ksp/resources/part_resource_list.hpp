// PartResourceList.cs (WP-15): the list of PartResource objects of a Part (Part.Resources / Part.SimulationResources).
//
// C#: a [Serializable] class over `DictionaryValueList<int, PartResource> dict` keyed by the definition id
// (name.GetHashCode()); `dict == null` is "not valid" (IsValid). Indexer by position = dict.At(index) (insertion order),
// by name = lookup by the Mono string hash.
//
// C++: the PartResource objects are heap objects with stable addresses (PartSet keeps PartResource pointers in its
// priority lists, and the C# objects stay alive as long as anything references them). Removed or replaced resources are
// retired, not freed, until the list dies (the garbage collector would keep them reachable from a PartSet that still
// lists them; PartSets are rebuilt on onPartResourceListChange, see Vessel.UpdateResourceSetsEventCheckPart).
#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "ksp/parts/part_data.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/util/collections.hpp"

namespace ksp {

class Part;

class PartResourceList {
public:
    // `new PartResourceList()` is never done by KSP code except through the constructors below; the default C++ object is
    // the C# null/invalid list (dict == null).
    PartResourceList() = default;
    explicit PartResourceList(Part* part) { Initializer(part); }                                   // :36
    PartResourceList(Part* part, bool simulationSet) {                                              // :41
        Initializer(part);
        this->simulationSet = simulationSet;
    }
    PartResourceList(Part* part, const PartResourceList* refList) {                                 // :47
        Initializer(part);
        RefListInitializer(part, refList, false);
    }
    PartResourceList(Part* part, const PartResourceList* refList, bool simulationSet) {             // :53
        Initializer(part);
        RefListInitializer(part, refList, simulationSet);
    }
    PartResourceList(const PartResourceList&) = delete;
    PartResourceList& operator=(const PartResourceList&) = delete;
    PartResourceList(PartResourceList&&) noexcept = default;
    PartResourceList& operator=(PartResourceList&&) noexcept = default;

    Part* part = nullptr;          // :13 private Part part (serialized)
    bool simulationSet = false;    // :16

    bool IsValid() const { return valid_; }                                     // :18
    PartResource& At(int index) const { return *dict_.At(index); }              // :20 this[int]
    PartResource& operator[](size_t index) const { return *dict_.At(static_cast<int>(index)); }
    PartResource* Get(std::string_view name) const { return Get(MonoStringHash(name)); }  // :115 / :120 Get(string)
    PartResource* Get(int id) const;                                            // :120
    int Count() const { return dict_.Count(); }                                 // :34
    size_t size() const { return static_cast<size_t>(dict_.Count()); }
    bool empty() const { return dict_.Count() == 0; }

    void RefreshSimulationListAmounts(const PartResourceList* refList);         // :79
    bool Contains(std::string_view name) const { return Contains(MonoStringHash(name)); }  // :105
    bool Contains(int id) const { return dict_.Contains(id); }                  // :110
    bool GetAll(std::vector<PartResource*>& sources, int id) const;             // :129
    bool GetAllFlowing(std::vector<PartResource*>& sources, int id) const;      // :140
    void GetFlowingTotals(int id, double& amount, double& maxAmount, bool pulling) const;  // :151
    PartResource* GetFlowing(int id, bool pulling) const;                       // :175
    PartResource* Add(const ConfigNode& node);                                  // :195
    PartResource* Add(const PartResource& res);                                 // :225
    PartResource* Add(std::string_view name, double amount, double maxAmount, bool flowState, bool isTweakable, bool hideFlow,
                      bool isVisible, PartResourceFlowMode flow);               // :237
    bool Remove(const PartResource& res);                                       // :268
    void Clear() { dict_.Clear(); }                                             // :281
    bool Remove(std::string_view resName);                                      // :286
    bool Remove(int resID);                                                     // :299
    bool HasFlowable() const;                                                   // :312
    bool HasFlowableUnhidden() const;                                           // :326

    // dict.Add(id, resource) of an already-built resource (the list takes ownership; used by the Add overloads and tests)
    PartResource* Insert(std::unique_ptr<PartResource> r, int id);

    // enumeration (C#: IEnumerable<PartResource> over the value list)
    class iterator {
    public:
        using Vec = std::vector<PartResource*>;
        iterator(const Vec* v, size_t i) : v_(v), i_(i) {}
        PartResource& operator*() const { return *(*v_)[i_]; }
        PartResource* operator->() const { return (*v_)[i_]; }
        iterator& operator++() {
            ++i_;
            return *this;
        }
        bool operator==(const iterator& o) const { return i_ == o.i_; }
        bool operator!=(const iterator& o) const { return i_ != o.i_; }

    private:
        const Vec* v_;
        size_t i_;
    };
    iterator begin() const { return iterator(&dict_.GetListEnumerator(), 0); }
    iterator end() const { return iterator(&dict_.GetListEnumerator(), dict_.GetListEnumerator().size()); }

    // Object.Instantiate of a Part clones its serialized PartResourceList: new PartResource objects with the same values,
    // `part` re-pointed at the clone (the reference to the original part inside the hierarchy is remapped).
    void CloneFrom(const PartResourceList& source, Part* newPart);

private:
    void Initializer(Part* p);                                                  // :59
    void RefListInitializer(Part* p, const PartResourceList* refList, bool simulationSet);  // :65
    bool valid_ = false;
    DictionaryValueList<int, PartResource*> dict_;
    std::vector<std::unique_ptr<PartResource>> storage_;  // owns every PartResource ever created by this list (see the file comment)
};

}  // namespace ksp
