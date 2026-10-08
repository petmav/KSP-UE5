#include "golden_json.hpp"

#include <charconv>
#include <cmath>
#include <filesystem>

#include <cstdio>

#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"

namespace golden {

std::string JsonEscape(const std::string& s) {
    std::string o;
    o.reserve(s.size() + 2);
    o.push_back('"');
    for (unsigned char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            case '\b': o += "\\b"; break;
            case '\f': o += "\\f"; break;
            default:
                if (c < 0x20) {
                    char b[8];
                    std::snprintf(b, sizeof b, "\\u%04x", c);
                    o += b;
                } else {
                    o.push_back(static_cast<char>(c));
                }
        }
    }
    o.push_back('"');
    return o;
}

// Same text as the harness writer (ExactNumber.FormatFloat/FormatDouble): the shortest of .NET's "G6".."G9"
// ("G15".."G17" for doubles) that round-trips, zero as "0.0"/"-0.0", non-finite values as strings.
std::string ShortestFloat(float f) {
    if (std::isnan(f)) return "\"NaN\"";
    if (std::isinf(f)) return f > 0 ? "\"Infinity\"" : "\"-Infinity\"";
    if (f == 0.0f) return std::signbit(f) ? "-0.0" : "0.0";
    for (int p = 6; p <= 9; ++p) {
        const std::string s = ksp::dotnet::FormatSingleG(f, p);
        float back = 0.0f;
        const auto r = std::from_chars(s.data(), s.data() + s.size(), back);
        if (r.ec == std::errc() && back == f) return s;
    }
    return ksp::dotnet::FormatSingleG(f, 9);
}

std::string ShortestDouble(double d) {
    if (std::isnan(d)) return "\"NaN\"";
    if (std::isinf(d)) return d > 0 ? "\"Infinity\"" : "\"-Infinity\"";
    if (d == 0.0) return std::signbit(d) ? "-0.0" : "0.0";
    for (int p = 15; p <= 17; ++p) {
        const std::string s = ksp::dotnet::FormatDoubleG(d, p);
        double back = 0.0;
        const auto r = std::from_chars(s.data(), s.data() + s.size(), back);
        if (r.ec == std::errc() && back == d) return s;
    }
    return ksp::dotnet::FormatDoubleG(d, 17);
}

static const char* TypeName(ColType t) {
    switch (t) {
        case ColType::Str: return "str";
        case ColType::I64: return "i64";
        case ColType::F32: return "f32";
        case ColType::F64: return "f64";
        case ColType::Bool: return "bool";
    }
    return "str";
}

static std::string CellJson(const Cell& c, ColType t) {
    struct V {
        ColType t;
        std::string operator()(std::monostate) const { return "null"; }
        std::string operator()(const std::string& s) const { return JsonEscape(s); }
        std::string operator()(int64_t i) const { return std::to_string(i); }
        std::string operator()(float f) const { return t == ColType::F64 ? ShortestDouble(f) : ShortestFloat(f); }
        std::string operator()(double d) const {
            return t == ColType::F32 ? ShortestFloat(static_cast<float>(d)) : ShortestDouble(d);
        }
        std::string operator()(bool b) const { return b ? "true" : "false"; }
    };
    return std::visit(V{t}, c.v);
}

std::string File::ToJson() const {
    std::string o;
    o += "{\n \"schema\": \"ksp-golden/1\",\n";
    o += " \"dataset\": " + JsonEscape(dataset) + ",\n";
    o += " \"trust\": \"candidate (kspcore)\",\n";
    o += " \"description\": " + JsonEscape(description) + ",\n";
    o += " \"tables\": [";
    for (size_t ti = 0; ti < tables.size(); ++ti) {
        const Table& t = tables[ti];
        o += ti ? ",\n  {" : "\n  {";
        o += "\n   \"name\": " + JsonEscape(t.name) + ",";
        o += "\n   \"description\": " + JsonEscape(t.description) + ",";
        o += "\n   \"key\": [";
        for (size_t k = 0; k < t.key.size(); ++k) o += (k ? "," : "") + JsonEscape(t.key[k]);
        o += "],\n   \"columns\": [";
        for (size_t k = 0; k < t.cols.size(); ++k) {
            o += (k ? "," : "");
            o += "{\"name\":" + JsonEscape(t.cols[k].first) + ",\"type\":\"" + TypeName(t.cols[k].second) + "\"}";
        }
        o += "],\n   \"row_count\": " + std::to_string(t.rows.size()) + ",\n   \"rows\": [";
        for (size_t r = 0; r < t.rows.size(); ++r) {
            o += r ? ",\n    [" : "\n    [";
            const auto& row = t.rows[r];
            for (size_t c = 0; c < row.size(); ++c) {
                if (c) o += ",";
                o += CellJson(row[c], c < t.cols.size() ? t.cols[c].second : ColType::Str);
            }
            o += "]";
        }
        o += "\n   ]\n  }";
    }
    o += "\n ]\n}\n";
    return o;
}

bool WriteTextFile(const std::string& path, const std::string& text) { return ksp::dotnet::WriteAllBytes(path, text); }

bool MakeDirs(const std::string& path) {
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(ksp::dotnet::Utf8ToWide(path)), ec);
    return !ec;
}

}  // namespace golden
