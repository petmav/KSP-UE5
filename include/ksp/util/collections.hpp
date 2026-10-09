// KSP's small collection helpers: ArrayExtensions.cs, ListExtension.cs, BiDictionaryOneToOne.cs,
// DictionaryValueList.cs, ListDictionary.cs. C# List<T> -> std::vector<T>, T[] -> std::span<T>, Dictionary -> the
// .NET-ordered ksp::dotnet::Dictionary; C# reference-type elements are pointers. Element equality is operator==
// (EqualityComparer<T>.Default: reference equality for KSP classes, value equality for value types and strings).
#pragma once

#include <algorithm>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/unity/random.hpp"

namespace ksp::util {

// ---- ArrayExtensions.cs ----------------------------------------------------------------------------------------------
// ArrayExtensions.cs:3 IndexOf: array[i].Equals(value) (a null element throws NullReferenceException)
template <class T>
int IndexOf(std::span<const T> array, const T& value) {
    for (size_t num = 0; num < array.size(); ++num) {
        if constexpr (std::is_pointer_v<T>) {
            if (array[num] == nullptr) throw NullReferenceException();
        }
        if (array[num] == value) return static_cast<int>(num);
    }
    return -1;
}

// ---- ListExtension.cs ------------------------------------------------------------------------------------------------
// ListExtension.cs:8 Shuffle: Fisher-Yates with UnityEngine.Random.Range(0, num + 1) (the global Unity generator)
template <class T>
void Shuffle(std::vector<T>& list, UnityRandom& random = UnityRandom::Global()) {
    int num = static_cast<int>(list.size());
    while (num > 1) {
        --num;
        const int index = random.Range(0, num + 1);
        T value = list[static_cast<size_t>(index)];
        list[static_cast<size_t>(index)] = list[static_cast<size_t>(num)];
        list[static_cast<size_t>(num)] = value;
    }
}
// ListExtension.cs:20
template <class T>
T AddUnique(std::vector<T>& list, const T& item) {
    if (std::find(list.begin(), list.end(), item) == list.end()) list.push_back(item);
    return item;
}
// ListExtension.cs:29 / 42 (items are checked against the list as it grows, so duplicates inside items collapse)
template <class T, class Range>
void AddUniqueRange(std::vector<T>& list, const Range& items) {
    for (const auto& item : items)
        if (std::find(list.begin(), list.end(), item) == list.end()) list.push_back(item);
}
// ListExtension.cs:55 / 79 MaxAt / MinAt: the loop starts at list[Count] (sic), so every non-empty list throws
// ArgumentOutOfRangeException (std::out_of_range) after sortBy(list[0]) has run; an empty list throws
// ArgumentNullException("collection cannot be empty").
template <class T, class SortBy>
T MaxAt(const std::vector<T>& list, SortBy&& sortBy) {
    if (list.empty()) throw ArgumentNullException("list: collection cannot be empty");
    T val = list[0];
    auto y = sortBy(val);
    for (size_t num = list.size();; --num) {
        const T& val2 = list.at(num);  // num == Count on the first iteration: throws
        auto val3 = sortBy(val2);
        if (val3 > y) {
            val = val2;
            y = val3;
        }
        if (num == 0) break;
    }
    return val;
}
template <class T, class SortBy>
T MinAt(const std::vector<T>& list, SortBy&& sortBy) {
    if (list.empty()) throw ArgumentNullException("list: collection cannot be empty");
    T val = list[0];
    auto y = sortBy(val);
    for (size_t num = list.size();; --num) {
        const T& val2 = list.at(num);
        auto val3 = sortBy(val2);
        if (val3 < y) {
            val = val2;
            y = val3;
        }
        if (num == 0) break;
    }
    return val;
}
// ListExtension.cs:103-157 SetRange (all four List/array combinations): copies range[0..] to list[startIndex..] while
// both have elements (a negative startIndex throws like the List/array indexer)
template <class T>
void SetRange(std::span<T> list, std::span<const T> range, int startIndex) {
    const int count = static_cast<int>(list.size());
    const int num = static_cast<int>(range.size());
    int num2 = 0;
    int num3 = startIndex;
    while (num2 < num && num3 < count) {
        if (num3 < 0) throw std::out_of_range("Index was out of range. Must be non-negative and less than the size of the collection.");
        list[static_cast<size_t>(num3)] = range[static_cast<size_t>(num2)];
        ++num2;
        ++num3;
    }
}
template <class T>
void SetRange(std::vector<T>& list, const std::vector<T>& range, int startIndex) {
    SetRange(std::span<T>(list), std::span<const T>(range), startIndex);
}
// ListExtension.cs:159 / 179 ContainsId(List<Part>, Part / uint): persistentId match (null element or part: NRE)
template <class P>
bool ContainsId(const std::vector<P*>& list, const P* p) {
    for (P* e : list) {
        if (e == nullptr || p == nullptr) throw NullReferenceException();
        if (e->persistentId == p->persistentId) return true;
    }
    return false;
}
template <class P>
bool ContainsId(const std::vector<P*>& list, uint32_t id) {
    for (P* e : list) {
        if (e == nullptr) throw NullReferenceException();
        if (e->persistentId == id) return true;
    }
    return false;
}
// ListExtension.cs:199 ContainsId(HashSet<Part>, Part): null (destroyed) elements are skipped
template <class Set, class P>
bool ContainsIdInSet(const Set& set, const P* p) {
    for (const auto* e : set) {
        if (e == nullptr) continue;
        if (p == nullptr) throw NullReferenceException();
        if (e->persistentId == p->persistentId) return true;
    }
    return false;
}

namespace detail {
// Regex.Replace(s, "\\d+", m => m.Value.PadLeft(width, '0')); \d is ASCII here (KSP strings are ASCII names)
inline std::string PadDigitRuns(std::string_view s, size_t width) {
    std::string out;
    for (size_t i = 0; i < s.size();) {
        if (s[i] >= '0' && s[i] <= '9') {
            size_t j = i;
            while (j < s.size() && s[j] >= '0' && s[j] <= '9') ++j;
            if (j - i < width) out.append(width - (j - i), '0');
            out.append(s.substr(i, j - i));
            i = j;
        } else {
            out.push_back(s[i++]);
        }
    }
    return out;
}
// Regex("\\d+").Matches(s): length of the longest digit run
inline size_t LongestDigitRun(std::string_view s) {
    size_t best = 0, run = 0;
    for (char c : s) {
        run = (c >= '0' && c <= '9') ? run + 1 : 0;
        best = std::max(best, run);
    }
    return best;
}
}  // namespace detail

// ListExtension.cs:216 OrderByAlphaNumeric: digit runs are left-padded with '0' to the longest run in the list, then a
// stable OrderBy with the culture-sensitive string comparison (dotnet::CompareCulture)
template <class T, class Selector>
std::vector<T> OrderByAlphaNumeric(const std::vector<T>& source, Selector&& selector) {
    size_t maxNumberLength = 0;
    for (const auto& e : source) maxNumberLength = std::max(maxNumberLength, detail::LongestDigitRun(selector(e)));
    std::vector<std::pair<std::string, size_t>> keys;
    keys.reserve(source.size());
    for (size_t i = 0; i < source.size(); ++i) keys.emplace_back(detail::PadDigitRuns(selector(source[i]), maxNumberLength), i);
    std::stable_sort(keys.begin(), keys.end(),
                     [](const auto& a, const auto& b) { return dotnet::CompareCulture(a.first, b.first) < 0; });
    std::vector<T> out;
    out.reserve(source.size());
    for (const auto& k : keys) out.push_back(source[k.second]);
    return out;
}
// ListExtension.cs:232 (array version)
template <class T, class Selector>
std::vector<T> OrderByAlphanumeric(std::span<const T> source, Selector&& selector) {
    return OrderByAlphaNumeric(std::vector<T>(source.begin(), source.end()), std::forward<Selector>(selector));
}

}  // namespace ksp::util

namespace ksp {

// ---- BiDictionaryOneToOne.cs -----------------------------------------------------------------------------------------
template <class TFirst, class TSecond>
class BiDictionaryOneToOne {
public:
    dotnet::Dictionary<TFirst, TSecond> firstToSecond;
    dotnet::Dictionary<TSecond, TFirst> secondToFirst;

    int Count() const { return firstToSecond.Count(); }
    // BiDictionaryOneToOne.cs:12
    void Add(const TFirst& first, const TSecond& second) {
        if (firstToSecond.ContainsKey(first) || secondToFirst.ContainsKey(second))
            throw ArgumentException("Duplicate first or second");
        firstToSecond.Add(first, second);
        secondToFirst.Add(second, first);
    }
    TSecond GetByFirst(const TFirst& first) const {
        TSecond value{};
        if (!firstToSecond.TryGetValue(first, value)) throw ArgumentException("first");
        return value;
    }
    TFirst GetBySecond(const TSecond& second) const {
        TFirst value{};
        if (!secondToFirst.TryGetValue(second, value)) throw ArgumentException("second");
        return value;
    }
    void RemoveByFirst(const TFirst& first) {
        TSecond value{};
        if (!firstToSecond.TryGetValue(first, value)) throw ArgumentException("first");
        firstToSecond.Remove(first);
        secondToFirst.Remove(value);
    }
    void RemoveBySecond(const TSecond& second) {
        TFirst value{};
        if (!secondToFirst.TryGetValue(second, value)) throw ArgumentException("second");
        secondToFirst.Remove(second);
        firstToSecond.Remove(value);
    }
    bool TryAdd(const TFirst& first, const TSecond& second) {
        if (!firstToSecond.ContainsKey(first) && !secondToFirst.ContainsKey(second)) {
            firstToSecond.Add(first, second);
            secondToFirst.Add(second, first);
            return true;
        }
        return false;
    }
    bool TryGetByFirst(const TFirst& first, TSecond& second) const { return firstToSecond.TryGetValue(first, second); }
    bool TryGetBySecond(const TSecond& second, TFirst& first) const { return secondToFirst.TryGetValue(second, first); }
    bool TryRemoveByFirst(const TFirst& first) {
        TSecond value{};
        if (!firstToSecond.TryGetValue(first, value)) return false;
        firstToSecond.Remove(first);
        secondToFirst.Remove(value);
        return true;
    }
    bool TryRemoveBySecond(const TSecond& second) {
        TFirst value{};
        if (!secondToFirst.TryGetValue(second, value)) return false;
        secondToFirst.Remove(second);
        firstToSecond.Remove(value);
        return true;
    }
    void Clear() {
        firstToSecond.Clear();
        secondToFirst.Clear();
    }
};

// ---- DictionaryValueList.cs ------------------------------------------------------------------------------------------
// A dictionary plus parallel value/key lists. Quirk kept: removing a key removes the FIRST value in the list that equals
// the key's value (List.Remove), which belongs to another key when two keys map to equal values.
template <class TKey, class TValue>
class DictionaryValueList {
public:
    DictionaryValueList() = default;
    // DictionaryValueList.cs:46 copy constructor (the dictionary copy is compacted in enumeration order)
    DictionaryValueList(const DictionaryValueList& old) : dict(old.dict), list(old.list), listKeys(old.listKeys) {}

    // indexer get (KeyNotFoundException) / set (DictionaryValueList.cs:15)
    const TValue& operator[](const TKey& key) const { return dict.At(key); }
    void Set(const TKey& key, const TValue& value) {
        TValue value2{};
        if (dict.TryGetValue(key, value2)) {
            RemoveFirst(list, value2);
            RemoveFirst(listKeys, key);
            dict.Remove(key);
        }
        dict.Add(key, value);
        list.push_back(value);
        listKeys.push_back(key);
    }
    int Count() const { return static_cast<int>(list.size()); }
    std::vector<TKey> Keys() const { return dict.Keys(); }
    std::vector<TValue> Values() const { return dict.Values(); }
    const std::vector<TValue>& ValuesList() const { return list; }
    const std::vector<TKey>& KeysList() const { return listKeys; }

    void Clear() {
        list.clear();
        listKeys.clear();
        dict.Clear();
    }
    // DictionaryValueList.cs:60: true when the key existed (it is re-added at the end)
    bool Add(const TKey& key, const TValue& val) {
        bool result = false;
        TValue value{};
        if (dict.TryGetValue(key, value)) {
            RemoveFirst(list, value);
            RemoveFirst(listKeys, key);
            dict.Remove(key);
            result = true;
        }
        list.push_back(val);
        listKeys.push_back(key);
        dict.Add(key, val);
        return result;
    }
    bool Remove(const TKey& key) {
        TValue value{};
        if (dict.TryGetValue(key, value)) {
            RemoveFirst(list, value);
            RemoveFirst(listKeys, key);
            dict.Remove(key);
            return true;
        }
        return false;
    }
    bool TryGetValue(const TKey& key, TValue& val) const { return dict.TryGetValue(key, val); }
    const TValue& At(int index) const { return list.at(static_cast<size_t>(index)); }
    const TKey& KeyAt(int index) const { return listKeys.at(static_cast<size_t>(index)); }
    bool Contains(const TKey& key) const { return dict.ContainsKey(key); }
    bool ContainsKey(const TKey& key) const { return dict.ContainsKey(key); }
    int IndexOf(const TValue& val) const {
        auto it = std::find(list.begin(), list.end(), val);
        return it == list.end() ? -1 : static_cast<int>(it - list.begin());
    }
    // DictionaryValueList.cs:122 list.Sort(comparison): .NET's unstable introsort; keys are NOT reordered (sic)
    template <class Comparison>
    void SortList(Comparison comparison) {
        dotnet::ListSort(list, comparison);
    }
    const dotnet::Dictionary<TKey, TValue>& GetDictEnumerator() const { return dict; }
    const std::vector<TValue>& GetListEnumerator() const { return list; }

private:
    template <class U>
    static void RemoveFirst(std::vector<U>& v, const U& x) {
        auto it = std::find(v.begin(), v.end(), x);
        if (it != v.end()) v.erase(it);
    }
    dotnet::Dictionary<TKey, TValue> dict;
    std::vector<TValue> list;
    std::vector<TKey> listKeys;
};

// ---- ListDictionary.cs -----------------------------------------------------------------------------------------------
// Dictionary<TKey, List<TValue>>. The lists are reference objects in C#: here shared_ptr, so the shallow copy
// constructor shares them like KSP does and the indexer returns the live list.
template <class TKey, class TValue>
class ListDictionary {
public:
    using List = std::shared_ptr<std::vector<TValue>>;

    ListDictionary() = default;
    // ListDictionary.cs:30 (shallow: the same list objects; deep: copies of the lists)
    ListDictionary(const ListDictionary& old, bool shallow = true) {
        if (shallow) {
            dict = old.dict;
            return;
        }
        for (const auto& kv : old.dict) dict.Add(kv.first, std::make_shared<std::vector<TValue>>(*kv.second));
    }

    std::vector<TKey> Keys() const { return dict.Keys(); }
    std::vector<List> Values() const { return dict.Values(); }

    // ListDictionary.cs:12 indexer get: the stored list, or a new empty list that is NOT stored
    List operator[](const TKey& key) const {
        List v;
        if (dict.TryGetValue(key, v)) return v;
        return std::make_shared<std::vector<TValue>>();
    }
    void Set(const TKey& key, List value) { dict.Set(key, std::move(value)); }

    void Clear() { dict.Clear(); }
    // ListDictionary.cs:52: true only when val was already in the key's list
    bool Add(const TKey& key, const TValue& val) {
        List value;
        if (dict.TryGetValue(key, value)) {
            if (std::find(value->begin(), value->end(), val) != value->end()) return true;
            value->push_back(val);
            return false;
        }
        value = std::make_shared<std::vector<TValue>>();
        value->push_back(val);
        dict.Add(key, value);
        return false;
    }
    // ListDictionary.cs:68: removes the LAST element equal to val
    bool Remove(const TKey& key, const TValue& val) {
        List value;
        if (dict.TryGetValue(key, value)) {
            size_t count = value->size();
            while (count-- > 0) {
                if constexpr (std::is_pointer_v<TValue>) {
                    if ((*value)[count] == nullptr) throw NullReferenceException();  // value[count].Equals(val)
                }
                if ((*value)[count] == val) {
                    value->erase(value->begin() + static_cast<std::ptrdiff_t>(count));
                    return true;
                }
            }
        }
        return false;
    }
    bool Remove(const TKey& key) { return dict.Remove(key); }
    bool TryGetValue(const TKey& key, List& val) const { return dict.TryGetValue(key, val); }
    const dotnet::Dictionary<TKey, List>& GetEnumerator() const { return dict; }

private:
    dotnet::Dictionary<TKey, List> dict;
};

}  // namespace ksp
