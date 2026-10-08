// .NET collection semantics that change observable KSP behaviour (Mono 5.11 BCL = .NET reference source):
//  * ListSort: List<T>.Sort / Array.Sort (Mono mscorlib System/Collections/Generic/ArraySortHelper.cs): an UNSTABLE
//    introspective sort (insertion sort for partitions <= 16, median-of-three quicksort, heapsort below a depth limit
//    of 2 * FloorLog2(array length)); equal elements end up in the order this exact algorithm produces.
//  * Dictionary: Dictionary<TKey, TValue> (Mono mscorlib System/Collections/Generic/Dictionary.cs) enumerates its
//    entries array in index order; Remove pushes the slot on a LIFO free list that the next Add reuses, so enumeration
//    order is insertion order with removed slots refilled most-recently-freed first. Hashing never shows; lookup here
//    is a std::unordered_map index.
#pragma once

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ksp/core/exceptions.hpp"

namespace ksp::dotnet {

class InvalidOperationException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

namespace detail {

// IntrospectiveSortUtilities.FloorLog2
inline int FloorLog2(int n) {
    int num = 0;
    while (n >= 1) {
        ++num;
        n /= 2;
    }
    return num;
}

template <class T, class Cmp>
class IntroSorter {
public:
    IntroSorter(std::vector<T>& keys, Cmp& comparer) : keys_(keys), comparer_(comparer) {}

    // ArraySortHelper.cs:101
    void IntrospectiveSort(int left, int length, int arrayLength) {
        if (length >= 2) IntroSort(left, length + left - 1, 2 * FloorLog2(arrayLength));
    }

private:
    // keys[i] with IndexOutOfRangeException (an inconsistent comparer walks off the array) -> std::out_of_range
    T& K(int i) {
        if (i < 0 || static_cast<size_t>(i) >= keys_.size()) throw std::out_of_range("Index was outside the bounds of the array.");
        return keys_[static_cast<size_t>(i)];
    }
    int C(const T& a, const T& b) { return comparer_(a, b); }

    // ArraySortHelper.cs:81
    void SwapIfGreater(int a, int b) {
        if (a != b && C(K(a), K(b)) > 0) std::swap(K(a), K(b));
    }
    void Swap(int i, int j) {
        if (i != j) std::swap(K(i), K(j));
    }
    // ArraySortHelper.cs:109
    void IntroSort(int lo, int hi, int depthLimit) {
        while (hi > lo) {
            const int num = hi - lo + 1;
            if (num <= 16) {
                switch (num) {
                    case 1:
                        break;
                    case 2:
                        SwapIfGreater(lo, hi);
                        break;
                    case 3:
                        SwapIfGreater(lo, hi - 1);
                        SwapIfGreater(lo, hi);
                        SwapIfGreater(hi - 1, hi);
                        break;
                    default:
                        InsertionSort(lo, hi);
                        break;
                }
                break;
            }
            if (depthLimit == 0) {
                Heapsort(lo, hi);
                break;
            }
            --depthLimit;
            const int num2 = PickPivotAndPartition(lo, hi);
            IntroSort(num2 + 1, hi, depthLimit);
            hi = num2 - 1;
        }
    }
    // ArraySortHelper.cs:146
    int PickPivotAndPartition(int lo, int hi) {
        const int num = lo + (hi - lo) / 2;
        SwapIfGreater(lo, num);
        SwapIfGreater(lo, hi);
        SwapIfGreater(num, hi);
        T val = K(num);
        Swap(num, hi - 1);
        int num2 = lo;
        int num3 = hi - 1;
        while (num2 < num3) {
            while (C(K(++num2), val) < 0) {
            }
            while (C(val, K(--num3)) < 0) {
            }
            if (num2 >= num3) break;
            Swap(num2, num3);
        }
        Swap(num2, hi - 1);
        return num2;
    }
    // ArraySortHelper.cs:174
    void Heapsort(int lo, int hi) {
        const int num = hi - lo + 1;
        for (int num2 = num / 2; num2 >= 1; --num2) DownHeap(num2, num, lo);
        for (int num3 = num; num3 > 1; --num3) {
            Swap(lo, lo + num3 - 1);
            DownHeap(1, num3 - 1, lo);
        }
    }
    // ArraySortHelper.cs:192
    void DownHeap(int i, int n, int lo) {
        T val = K(lo + i - 1);
        while (i <= n / 2) {
            int num = 2 * i;
            if (num < n && C(K(lo + num - 1), K(lo + num)) < 0) ++num;
            if (C(val, K(lo + num - 1)) >= 0) break;
            K(lo + i - 1) = K(lo + num - 1);
            i = num;
        }
        K(lo + i - 1) = val;
    }
    // ArraySortHelper.cs:208
    void InsertionSort(int lo, int hi) {
        for (int i = lo; i < hi; ++i) {
            int num = i;
            T val = K(i + 1);
            while (num >= lo && C(val, K(num)) < 0) {
                K(num + 1) = K(num);
                --num;
            }
            K(num + 1) = val;
        }
    }

    std::vector<T>& keys_;
    Cmp& comparer_;
};

}  // namespace detail

// List<T>.Sort(Comparison<T>) (List.cs:1033 -> ArraySortHelper<T>.Sort): comparison(a, b) < 0 / 0 / > 0.
// arrayLength: length of the List's backing array (its Capacity), which sets the depth limit; defaults to Count
// (identical results unless an input drives the quicksort past 2*FloorLog2 levels).
// An inconsistent comparer: ArgumentException (C#: IndexOutOfRange inside the sort); a throwing comparer:
// InvalidOperationException("Failed to compare two elements in the array.").
template <class T, class Comparison>
void ListSort(std::vector<T>& list, Comparison comparison, int arrayLength = -1) {
    if (list.empty()) return;
    try {
        detail::IntroSorter<T, Comparison> s(list, comparison);
        s.IntrospectiveSort(0, static_cast<int>(list.size()), arrayLength < 0 ? static_cast<int>(list.size()) : arrayLength);
    } catch (const std::out_of_range&) {
        throw ArgumentException(
            "Unable to sort because the IComparer.Compare() method returns inconsistent results. Either a value does not "
            "compare equal to itself, or one value repeatedly compared to another value yields different results.");
    } catch (const std::exception&) {
        throw InvalidOperationException("Failed to compare two elements in the array.");
    }
}

// System.Collections.Generic.Dictionary<TKey, TValue> with .NET's enumeration order (see the file comment).
template <class K, class V, class Hash = std::hash<K>, class Eq = std::equal_to<K>>
class Dictionary {
public:
    using value_type = std::pair<K, V>;

    Dictionary() = default;
    // new Dictionary<K,V>(IDictionary): copies the source in its enumeration order (no holes)
    Dictionary(const Dictionary& o) : Dictionary() {
        for (const auto& kv : o) Add(kv.first, kv.second);
    }
    Dictionary& operator=(const Dictionary& o) {
        if (this != &o) {
            Dictionary tmp(o);
            *this = std::move(tmp);
        }
        return *this;
    }
    Dictionary(Dictionary&&) noexcept = default;
    Dictionary& operator=(Dictionary&&) noexcept = default;

    int Count() const { return static_cast<int>(index_.size()); }

    // Add: ArgumentException for a duplicate key
    void Add(const K& key, V value) {
        if (index_.count(key)) throw ArgumentException("An item with the same key has already been added.");
        Insert(key, std::move(value));
    }
    // indexer set: overwrite in place, or add
    void Set(const K& key, V value) {
        auto it = index_.find(key);
        if (it != index_.end()) entries_[static_cast<size_t>(it->second)]->second = std::move(value);
        else Insert(key, std::move(value));
    }
    // indexer get: KeyNotFoundException
    V& At(const K& key) {
        auto it = index_.find(key);
        if (it == index_.end()) throw KeyNotFoundException("The given key was not present in the dictionary.");
        return entries_[static_cast<size_t>(it->second)]->second;
    }
    const V& At(const K& key) const { return const_cast<Dictionary*>(this)->At(key); }
    bool TryGetValue(const K& key, V& value) const {
        auto it = index_.find(key);
        if (it == index_.end()) {
            value = V{};
            return false;
        }
        value = entries_[static_cast<size_t>(it->second)]->second;
        return true;
    }
    V* Find(const K& key) {
        auto it = index_.find(key);
        return it == index_.end() ? nullptr : &entries_[static_cast<size_t>(it->second)]->second;
    }
    bool ContainsKey(const K& key) const { return index_.count(key) != 0; }
    bool Remove(const K& key) {
        auto it = index_.find(key);
        if (it == index_.end()) return false;
        const int i = it->second;
        index_.erase(it);
        entries_[static_cast<size_t>(i)].reset();
        freeList_.push_back(i);
        return true;
    }
    void Clear() {
        entries_.clear();
        index_.clear();
        freeList_.clear();
    }

    // enumeration in .NET order
    class const_iterator {
    public:
        const_iterator(const Dictionary* d, size_t i) : d_(d), i_(i) { Skip(); }
        const value_type& operator*() const { return *d_->entries_[i_]; }
        const value_type* operator->() const { return &*d_->entries_[i_]; }
        const_iterator& operator++() {
            ++i_;
            Skip();
            return *this;
        }
        bool operator==(const const_iterator& o) const { return i_ == o.i_; }
        bool operator!=(const const_iterator& o) const { return i_ != o.i_; }

    private:
        void Skip() {
            while (i_ < d_->entries_.size() && !d_->entries_[i_]) ++i_;
        }
        const Dictionary* d_;
        size_t i_;
    };
    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end() const { return const_iterator(this, entries_.size()); }

    std::vector<K> Keys() const {
        std::vector<K> r;
        for (const auto& kv : *this) r.push_back(kv.first);
        return r;
    }
    std::vector<V> Values() const {
        std::vector<V> r;
        for (const auto& kv : *this) r.push_back(kv.second);
        return r;
    }

private:
    void Insert(const K& key, V value) {
        int i;
        if (!freeList_.empty()) {
            i = freeList_.back();
            freeList_.pop_back();
            entries_[static_cast<size_t>(i)].emplace(key, std::move(value));
        } else {
            i = static_cast<int>(entries_.size());
            entries_.emplace_back(std::in_place, key, std::move(value));
        }
        index_.emplace(key, i);
    }

    std::vector<std::optional<value_type>> entries_;
    std::unordered_map<K, int, Hash, Eq> index_;
    std::vector<int> freeList_;  // LIFO: Dictionary.freeList chain
};

}  // namespace ksp::dotnet
