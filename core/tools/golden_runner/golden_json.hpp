// Writer for the harness's ksp-golden/1 table schema (see harness/README.md and harness/compare/compare.py).
// Numbers are written as the shortest round-trip text (float32 columns: shortest float32 text), NaN/Infinity as the
// strings "NaN"/"Infinity"/"-Infinity", unused cells as null.
#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace golden {

enum class ColType { Str, I64, F32, F64, Bool };

struct Cell {
    std::variant<std::monostate, std::string, int64_t, float, double, bool> v;
    Cell() = default;
    Cell(std::nullptr_t) {}
    Cell(std::string s) : v(std::move(s)) {}
    Cell(const char* s) : v(std::string(s)) {}
    Cell(int64_t i) : v(i) {}
    Cell(int i) : v(static_cast<int64_t>(i)) {}
    Cell(size_t i) : v(static_cast<int64_t>(i)) {}
    Cell(float f) : v(f) {}
    Cell(double d) : v(d) {}
    Cell(bool b) : v(b) {}
};

struct Table {
    std::string name;
    std::string description;
    std::vector<std::string> key;
    std::vector<std::pair<std::string, ColType>> cols;
    std::vector<std::vector<Cell>> rows;

    Table(std::string n, std::string d) : name(std::move(n)), description(std::move(d)) {}
    Table& Col(std::string n, ColType t) {
        cols.emplace_back(std::move(n), t);
        return *this;
    }
    void Add(std::vector<Cell> row) { rows.push_back(std::move(row)); }
};

struct File {
    std::string dataset;
    std::string description;
    std::vector<Table> tables;
    std::string ToJson() const;
};

std::string JsonEscape(const std::string& s);
std::string ShortestFloat(float f);
std::string ShortestDouble(double d);

bool WriteTextFile(const std::string& path, const std::string& text);
bool MakeDirs(const std::string& path);

}  // namespace golden
