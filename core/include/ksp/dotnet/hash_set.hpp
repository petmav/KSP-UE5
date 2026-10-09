// System.Collections.Generic.HashSet<T> with .NET's enumeration order (WP-15; the resource flow graphs enumerate
// HashSet<Part> and the order shows in which part an equal-priority request drains first).
//
// HashSet<T> (Mono's System.Core / the .NET reference source `HashSet<T>`) keeps its elements in a slots array in insertion
// order; Remove puts the slot on a LIFO free list that the next Add refills. Enumeration walks the slots array in index
// order, skipping free slots. That is exactly the entry-array behaviour of Dictionary<TKey,TValue> (ksp/dotnet/collections.hpp),
// so the set is a Dictionary<T, char>. Hashing never shows. Element equality is `Hash`/`Eq` (default: operator== / std::hash,
// i.e. reference identity for the Part* the flow code stores).
//
// Behaviours the flow code relies on:
//   Add returns false when the element exists; UnionWith adds the other set's elements in the other's enumeration order;
//   the copy constructor (`new HashSet<T>(other)`) copies in enumeration order (holes are closed: the reference source
//   copies the slots verbatim only when the capacities allow it, which never differs for sets without removals);
//   Overlaps / SetEquals / DeepCompare-style queries are order independent.
#pragma once

#include <functional>
#include <vector>

#include "ksp/dotnet/collections.hpp"

namespace ksp::dotnet {

template <class T, class Hash = std::hash<T>, class Eq = std::equal_to<T>>
class HashSet {
public:
    HashSet() = default;
    // new HashSet<T>(IEnumerable<T>)
    template <class Range>
    explicit HashSet(const Range& items) {
        for (const auto& e : items) Add(e);
    }
    HashSet(const HashSet&) = default;
    HashSet& operator=(const HashSet&) = default;
    HashSet(HashSet&&) noexcept = default;
    HashSet& operator=(HashSet&&) noexcept = default;

    int Count() const { return d_.Count(); }
    size_t size() const { return static_cast<size_t>(d_.Count()); }
    bool empty() const { return d_.Count() == 0; }
    bool Contains(const T& item) const { return d_.ContainsKey(item); }
    // HashSet.Add: true when the element was added
    bool Add(const T& item) {
        if (d_.ContainsKey(item)) return false;
        d_.Add(item, 0);
        return true;
    }
    bool Remove(const T& item) { return d_.Remove(item); }
    void Clear() { d_.Clear(); }
    // HashSet.UnionWith(IEnumerable<T>)
    template <class Range>
    void UnionWith(const Range& other) {
        for (const auto& e : other) Add(e);
    }
    // HashSet.Overlaps(IEnumerable<T>)
    template <class Range>
    bool Overlaps(const Range& other) const {
        if (Count() == 0) return false;
        for (const auto& e : other)
            if (Contains(e)) return true;
        return false;
    }

    class const_iterator {
    public:
        explicit const_iterator(typename Dictionary<T, char, Hash, Eq>::const_iterator it) : it_(it) {}
        const T& operator*() const { return it_->first; }
        const T* operator->() const { return &it_->first; }
        const_iterator& operator++() {
            ++it_;
            return *this;
        }
        bool operator==(const const_iterator& o) const { return it_ == o.it_; }
        bool operator!=(const const_iterator& o) const { return it_ != o.it_; }

    private:
        typename Dictionary<T, char, Hash, Eq>::const_iterator it_;
    };
    const_iterator begin() const { return const_iterator(d_.begin()); }
    const_iterator end() const { return const_iterator(d_.end()); }
    // std::set-like lookup so KSPUtil::DeepCompare (a.size(), b.find()) works on it
    const_iterator find(const T& item) const { return d_.ContainsKey(item) ? const_iterator(d_.begin()) : end(); }

    std::vector<T> ToList() const {
        std::vector<T> r;
        for (const T& e : *this) r.push_back(e);
        return r;
    }

private:
    Dictionary<T, char, Hash, Eq> d_;
};

}  // namespace ksp::dotnet
