#include "ksp/modules/cs_expr.hpp"

#include <charconv>
#include <cmath>
#include <limits>

#include "ksp/dotnet/mono_float.hpp"
#include "ksp/localization/localizer.hpp"

namespace ksp::cs {

using Kind = CsValue::Kind;

bool CsValue::operator==(const CsValue& o) const {
    return kind == o.kind && b == o.b && i == o.i && s == o.s && typeName == o.typeName && vec == o.vec &&
           (f == o.f || (f != f && o.f != o.f)) && (d == o.d || (d != d && o.d != o.d));
}

namespace {

struct Tok {
    enum T { End, Num, Str, Ident, Punct } t = End;
    std::string text;
    // Num
    bool isFloat = false, isDouble = false, isUnsigned = false, isLong = false;
};

class Lexer {
public:
    explicit Lexer(std::string_view s) : s_(s) { Advance(); }
    const Tok& Peek() const { return cur_; }
    Tok Next() {
        Tok t = cur_;
        Advance();
        return t;
    }
    bool ok = true;

private:
    void Advance() {
        while (pos_ < s_.size() && (s_[pos_] == ' ' || s_[pos_] == '\t')) pos_++;
        cur_ = Tok{};
        if (pos_ >= s_.size()) return;
        const char c = s_[pos_];
        if (c == '"') {
            cur_.t = Tok::Str;
            pos_++;
            while (pos_ < s_.size() && s_[pos_] != '"') {
                if (s_[pos_] == '\\' && pos_ + 1 < s_.size()) {
                    const char e = s_[pos_ + 1];
                    pos_ += 2;
                    switch (e) {
                        case 'n': cur_.text += '\n'; break;
                        case 't': cur_.text += '\t'; break;
                        case 'r': cur_.text += '\r'; break;
                        case '0': cur_.text += '\0'; break;
                        case 'u': {  // \uXXXX -> UTF-8
                            unsigned cp = 0;
                            for (int k = 0; k < 4 && pos_ < s_.size(); k++, pos_++) {
                                const char h = s_[pos_];
                                cp = cp * 16 + (h <= '9' ? h - '0' : (h | 0x20) - 'a' + 10);
                            }
                            if (cp < 0x80) cur_.text += static_cast<char>(cp);
                            else if (cp < 0x800) {
                                cur_.text += static_cast<char>(0xC0 | (cp >> 6));
                                cur_.text += static_cast<char>(0x80 | (cp & 0x3F));
                            } else {
                                cur_.text += static_cast<char>(0xE0 | (cp >> 12));
                                cur_.text += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                                cur_.text += static_cast<char>(0x80 | (cp & 0x3F));
                            }
                            break;
                        }
                        default: cur_.text += e; break;  // \" \\ \'
                    }
                } else {
                    cur_.text += s_[pos_++];
                }
            }
            if (pos_ >= s_.size()) ok = false;
            pos_++;
            return;
        }
        if ((c >= '0' && c <= '9') || (c == '.' && pos_ + 1 < s_.size() && s_[pos_ + 1] >= '0' && s_[pos_ + 1] <= '9')) {
            cur_.t = Tok::Num;
            size_t b = pos_;
            if (c == '0' && pos_ + 1 < s_.size() && (s_[pos_ + 1] == 'x' || s_[pos_ + 1] == 'X')) {
                pos_ += 2;
                while (pos_ < s_.size() && isxdigit(static_cast<unsigned char>(s_[pos_]))) pos_++;
                cur_.text = std::string(s_.substr(b, pos_ - b));
            } else {
                bool real = false;
                while (pos_ < s_.size()) {
                    const char d = s_[pos_];
                    if (d >= '0' && d <= '9') pos_++;
                    else if (d == '.') { real = true; pos_++; }
                    else if ((d == 'e' || d == 'E')) {
                        real = true;
                        pos_++;
                        if (pos_ < s_.size() && (s_[pos_] == '+' || s_[pos_] == '-')) pos_++;
                    } else break;
                }
                cur_.text = std::string(s_.substr(b, pos_ - b));
                (void)real;
            }
            while (pos_ < s_.size()) {
                const char d = s_[pos_];
                if (d == 'f' || d == 'F') cur_.isFloat = true;
                else if (d == 'd' || d == 'D') cur_.isDouble = true;
                else if (d == 'u' || d == 'U') cur_.isUnsigned = true;
                else if (d == 'l' || d == 'L') cur_.isLong = true;
                else break;
                pos_++;
            }
            if (cur_.text.find_first_of(".eE") != std::string::npos && cur_.text.rfind("0x", 0) != 0 && !cur_.isFloat)
                cur_.isDouble = true;
            return;
        }
        if (isalpha(static_cast<unsigned char>(c)) || c == '_') {
            cur_.t = Tok::Ident;
            size_t b = pos_;
            while (pos_ < s_.size() && (isalnum(static_cast<unsigned char>(s_[pos_])) || s_[pos_] == '_')) pos_++;
            cur_.text = std::string(s_.substr(b, pos_ - b));
            return;
        }
        cur_.t = Tok::Punct;
        cur_.text = std::string(1, c);
        pos_++;
    }
    std::string_view s_;
    size_t pos_ = 0;
    Tok cur_;
};

CsValue Null() { return CsValue{}; }
CsValue MakeInt(int64_t v) {
    CsValue r;
    r.kind = Kind::Int;
    r.i = v;
    return r;
}
CsValue MakeFloat(float v) {
    CsValue r;
    r.kind = Kind::Float;
    r.f = v;
    return r;
}
CsValue MakeDouble(double v) {
    CsValue r;
    r.kind = Kind::Double;
    r.d = v;
    return r;
}
CsValue MakeStr(std::string v) {
    CsValue r;
    r.kind = Kind::String;
    r.s = std::move(v);
    return r;
}
CsValue MakeVec(const char* type, std::vector<float> v) {
    CsValue r;
    r.kind = Kind::Vector;
    r.vecType = type;
    r.vec = std::move(v);
    return r;
}

class Parser {
public:
    explicit Parser(std::string_view s) : lex_(s) {}
    bool ok = true;
    bool Done() const { return lex_.Peek().t == Tok::End; }

    CsValue ParseExpr() { return ParseAdditive(); }
    Lexer& lexer() { return lex_; }

private:
    bool IsPunct(const char* p) const { return lex_.Peek().t == Tok::Punct && lex_.Peek().text == p; }

    CsValue ParseAdditive() {
        CsValue l = ParseTerm();
        while (ok && (IsPunct("+") || IsPunct("-"))) {
            const char op = lex_.Next().text[0];
            CsValue r = ParseTerm();
            l = Binary(op, l, r);
        }
        return l;
    }
    CsValue ParseTerm() {
        CsValue l = ParseUnary();
        while (ok && (IsPunct("*") || IsPunct("/"))) {
            const char op = lex_.Next().text[0];
            CsValue r = ParseUnary();
            l = Binary(op, l, r);
        }
        return l;
    }
    CsValue ParseUnary() {
        if (IsPunct("-")) {
            lex_.Next();
            CsValue v = ParseUnary();
            switch (v.kind) {
                case Kind::Int: v.i = -v.i; break;
                case Kind::Float: v.f = -v.f; break;
                case Kind::Double: v.d = -v.d; break;
                case Kind::Vector:
                    for (float& c : v.vec) c = -c;
                    break;
                default: ok = false;
            }
            return v;
        }
        if (IsPunct("+")) {
            lex_.Next();
            return ParseUnary();
        }
        return ParsePrimary();
    }

    static bool IsNumeric(const CsValue& v) { return v.kind == Kind::Int || v.kind == Kind::Float || v.kind == Kind::Double; }
    static double AsD(const CsValue& v) {
        return v.kind == Kind::Int ? static_cast<double>(v.i) : v.kind == Kind::Float ? static_cast<double>(v.f) : v.d;
    }
    CsValue Binary(char op, const CsValue& a, const CsValue& b) {
        if (!IsNumeric(a) || !IsNumeric(b)) {
            if (op == '+' && a.kind == Kind::String && b.kind == Kind::String) return MakeStr(a.s + b.s);
            ok = false;
            return Null();
        }
        auto apply = [&](double x, double y) {
            switch (op) {
                case '+': return x + y;
                case '-': return x - y;
                case '*': return x * y;
                default: return x / y;
            }
        };
        if (a.kind == Kind::Int && b.kind == Kind::Int) {
            switch (op) {
                case '+': return MakeInt(a.i + b.i);
                case '-': return MakeInt(a.i - b.i);
                case '*': return MakeInt(a.i * b.i);
                default:
                    if (b.i == 0) { ok = false; return Null(); }
                    return MakeInt(a.i / b.i);
            }
        }
        if (a.kind == Kind::Double || b.kind == Kind::Double) return MakeDouble(apply(AsD(a), AsD(b)));
        // float op float / int: evaluated wide, rounded at the (constant-fold) store
        return MakeFloat(f32store(apply(fx(static_cast<float>(AsD(a))), fx(static_cast<float>(AsD(b))))));
    }

    std::vector<CsValue> ParseArgs() {  // after '('
        std::vector<CsValue> args;
        if (IsPunct(")")) { lex_.Next(); return args; }
        while (ok) {
            args.push_back(ParseExpr());
            if (IsPunct(",")) { lex_.Next(); continue; }
            if (IsPunct(")")) { lex_.Next(); break; }
            ok = false;
        }
        return args;
    }

    CsValue ParsePrimary() {
        const Tok t = lex_.Next();
        switch (t.t) {
            case Tok::Num: return Number(t);
            case Tok::Str: return MakeStr(t.text);
            case Tok::Punct:
                if (t.text == "(") {
                    CsValue v = ParseExpr();
                    if (!IsPunct(")")) ok = false; else lex_.Next();
                    return v;
                }
                ok = false;
                return Null();
            case Tok::Ident: break;
            default: ok = false; return Null();
        }
        if (t.text == "true" || t.text == "false") {
            CsValue r;
            r.kind = Kind::Bool;
            r.b = t.text == "true";
            return r;
        }
        if (t.text == "null") return Null();
        if (t.text == "new") {
            std::string type = lex_.Next().text;
            while (IsPunct(".")) { lex_.Next(); type = lex_.Next().text; }
            if (!IsPunct("(")) { ok = false; return Null(); }
            lex_.Next();
            std::vector<CsValue> args = ParseArgs();
            std::vector<float> comps;
            for (const CsValue& a : args) {
                if (!IsNumeric(a)) { ok = false; return Null(); }
                comps.push_back(static_cast<float>(AsD(a)));
            }
            if (type == "Vector2" || type == "Vector3" || type == "Vector4" || type == "Quaternion") {
                const size_t n = type == "Vector2" ? 2 : type == "Vector3" ? 3 : 4;
                if (comps.size() == n - (type == "Vector3" ? 1 : 0) && type == "Vector3") comps.push_back(0.0f);  // Vector3(x, y)
                if (comps.size() != n) { ok = false; return Null(); }
                return MakeVec(type == "Quaternion" ? "Quaternion" : type.c_str(), comps);
            }
            CsValue r;
            r.kind = Kind::New;
            r.typeName = type;
            return r;
        }
        // dotted path (+ optional call)
        std::vector<std::string> path{t.text};
        while (IsPunct(".")) {
            lex_.Next();
            path.push_back(lex_.Next().text);
        }
        if (IsPunct("(")) {
            lex_.Next();
            std::vector<CsValue> args = ParseArgs();
            return Call(path, args);
        }
        return Member(path);
    }

    CsValue Number(const Tok& t) {
        const std::string& x = t.text;
        if (x.rfind("0x", 0) == 0 || x.rfind("0X", 0) == 0) return MakeInt(static_cast<int64_t>(std::stoull(x, nullptr, 16)));
        if (t.isFloat) {
            float v = 0.0f;
            auto r = std::from_chars(x.data(), x.data() + x.size(), v);
            if (r.ec != std::errc()) ok = false;
            return MakeFloat(v);
        }
        if (t.isDouble) {
            double v = 0.0;
            auto r = std::from_chars(x.data(), x.data() + x.size(), v);
            if (r.ec != std::errc()) ok = false;
            return MakeDouble(v);
        }
        return MakeInt(std::stoll(x));
    }

    CsValue Call(const std::vector<std::string>& path, const std::vector<CsValue>& args) {
        if (path.size() == 2 && path[0] == "Localizer" && path[1] == "Format" && !args.empty()) {
            for (const CsValue& a : args)
                if (a.kind != Kind::String) { ok = false; return Null(); }
            if (args.size() == 1) return MakeStr(Localizer::Format(args[0].s));
            std::vector<std::string> rest;
            for (size_t k = 1; k < args.size(); k++) rest.push_back(args[k].s);
            return MakeStr(Localizer::Format(args[0].s, rest));
        }
        ok = false;
        return Null();
    }

    CsValue Member(const std::vector<std::string>& path) {
        if (path.size() == 2) {
            const std::string& t = path[0];
            const std::string& m = path[1];
            if (t == "float") {
                if (m == "NaN") return MakeFloat(std::numeric_limits<float>::quiet_NaN());
                if (m == "MaxValue") return MakeFloat(std::numeric_limits<float>::max());
                if (m == "MinValue") return MakeFloat(-std::numeric_limits<float>::max());
                if (m == "PositiveInfinity") return MakeFloat(std::numeric_limits<float>::infinity());
                if (m == "NegativeInfinity") return MakeFloat(-std::numeric_limits<float>::infinity());
                if (m == "Epsilon") return MakeFloat(std::numeric_limits<float>::denorm_min());
            } else if (t == "double") {
                if (m == "NaN") return MakeDouble(std::numeric_limits<double>::quiet_NaN());
                if (m == "MaxValue") return MakeDouble(std::numeric_limits<double>::max());
                if (m == "MinValue") return MakeDouble(-std::numeric_limits<double>::max());
                if (m == "PositiveInfinity") return MakeDouble(std::numeric_limits<double>::infinity());
                if (m == "NegativeInfinity") return MakeDouble(-std::numeric_limits<double>::infinity());
                if (m == "Epsilon") return MakeDouble(std::numeric_limits<double>::denorm_min());
            } else if (t == "int") {
                if (m == "MaxValue") return MakeInt(std::numeric_limits<int32_t>::max());
                if (m == "MinValue") return MakeInt(std::numeric_limits<int32_t>::min());
            } else if (t == "uint") {
                if (m == "MaxValue") return MakeInt(std::numeric_limits<uint32_t>::max());
                if (m == "MinValue") return MakeInt(0);
            } else if (t == "string" && m == "Empty") {
                return MakeStr("");
            } else if (t == "Vector3") {
                if (m == "zero") return MakeVec("Vector3", {0, 0, 0});
                if (m == "one") return MakeVec("Vector3", {1, 1, 1});
                if (m == "up") return MakeVec("Vector3", {0, 1, 0});
                if (m == "down") return MakeVec("Vector3", {0, -1, 0});
                if (m == "left") return MakeVec("Vector3", {-1, 0, 0});
                if (m == "right") return MakeVec("Vector3", {1, 0, 0});
                if (m == "forward") return MakeVec("Vector3", {0, 0, 1});
                if (m == "back") return MakeVec("Vector3", {0, 0, -1});
            } else if (t == "Vector2") {
                if (m == "zero") return MakeVec("Vector2", {0, 0});
                if (m == "one") return MakeVec("Vector2", {1, 1});
            } else if (t == "Quaternion" && m == "identity") {
                return MakeVec("Quaternion", {0, 0, 0, 1});
            }
        }
        if (path.size() >= 2) {  // Type.Member (enum constant); nested types keep their path
            CsValue r;
            r.kind = Kind::Member;
            r.s = path.back();
            for (size_t k = 0; k + 1 < path.size(); k++) r.typeName += (k ? "." : "") + path[k];
            return r;
        }
        ok = false;
        return Null();
    }

    Lexer lex_;
};

}  // namespace

bool EvaluateExpression(std::string_view text, CsValue& out) {
    Parser p(text);
    out = p.ParseExpr();
    return p.ok && p.lexer().ok && p.Done();
}

bool ParseAttributeArguments(std::string_view text, std::vector<AttributeArg>& out) {
    out.clear();
    // split on top-level commas (outside strings and parentheses)
    std::vector<std::string_view> parts;
    int depth = 0;
    bool inStr = false;
    size_t start = 0;
    for (size_t k = 0; k < text.size(); k++) {
        const char c = text[k];
        if (inStr) {
            if (c == '\\') k++;
            else if (c == '"') inStr = false;
        } else if (c == '"') inStr = true;
        else if (c == '(') depth++;
        else if (c == ')') depth--;
        else if (c == ',' && depth == 0) {
            parts.push_back(text.substr(start, k - start));
            start = k + 1;
        }
    }
    if (start < text.size() || !parts.empty()) parts.push_back(text.substr(start));
    bool all = true;
    for (std::string_view p : parts) {
        AttributeArg a;
        // named: ident '=' expr (a '=' outside strings that is not part of ==)
        size_t eq = std::string_view::npos;
        bool s2 = false;
        for (size_t k = 0; k < p.size(); k++) {
            if (p[k] == '"') s2 = !s2;
            else if (!s2 && p[k] == '=') { eq = k; break; }
            else if (!s2 && p[k] == '(') break;
        }
        std::string_view expr = p;
        if (eq != std::string_view::npos) {
            size_t b = 0, e = eq;
            while (b < e && p[b] == ' ') b++;
            while (e > b && p[e - 1] == ' ') e--;
            a.name = std::string(p.substr(b, e - b));
            expr = p.substr(eq + 1);
        }
        if (!EvaluateExpression(expr, a.value)) all = false;
        out.push_back(std::move(a));
    }
    return all;
}

bool AsBool(const CsValue& v, bool def) { return v.kind == Kind::Bool ? v.b : def; }
float AsFloat(const CsValue& v, float def) {
    switch (v.kind) {
        case Kind::Float: return v.f;
        case Kind::Int: return static_cast<float>(v.i);
        case Kind::Double: return static_cast<float>(v.d);
        default: return def;
    }
}
std::string AsString(const CsValue& v, const std::string& def) { return v.kind == Kind::String ? v.s : def; }

ModuleValue DefaultFieldValue(const ModuleFieldInfo& info) {
    const std::string_view t = info.typeName;
    if (info.enumTable) return EnumValue{0, info.enumTable};
    if (t == "Boolean") return false;
    if (t == "Byte") return uint8_t{0};
    if (t == "UInt16") return uint16_t{0};
    if (t == "Char") return char16_t{0};
    if (t == "UInt64") return uint64_t{0};
    if (t == "Int32") return int32_t{0};
    if (t == "Int64") return int64_t{0};
    if (t == "Single") return 0.0f;
    if (t == "UInt32") return uint32_t{0};
    if (t == "Int16") return int16_t{0};
    if (t == "Double") return 0.0;
    if (t == "Vector2") return Vector2f{};
    if (t == "Vector3") return Vector3f{};
    if (t == "Vector4") return Vector4f{};
    if (t == "Quaternion") return Quaternionf{};
    if (t == "FloatCurve") return FloatCurve{};
    if (t == "DoubleCurve") return DoubleCurve{};
    return std::monostate{};  // string / reference types: null
}

bool ConvertConstant(const CsValue& v, const ModuleFieldInfo& info, ModuleValue& out) {
    const std::string_view t = info.typeName;
    if (info.enumTable) {
        if (v.kind == Kind::Member) {
            for (const auto& [n, val] : info.enumTable->members)
                if (n == v.s) { out = EnumValue{val, info.enumTable}; return true; }
            return false;
        }
        if (v.kind == Kind::Int) { out = EnumValue{v.i, info.enumTable}; return true; }
        return false;
    }
    auto num = [&](double& d) {
        if (v.kind == Kind::Int) { d = static_cast<double>(v.i); return true; }
        if (v.kind == Kind::Float) { d = static_cast<double>(v.f); return true; }
        if (v.kind == Kind::Double) { d = v.d; return true; }
        return false;
    };
    double d = 0;
    if (t == "Boolean") { if (v.kind != Kind::Bool) return false; out = v.b; return true; }
    if (t == "String") {
        if (v.kind == Kind::String) { out = v.s; return true; }
        if (v.kind == Kind::Null) { out = std::monostate{}; return true; }
        return false;
    }
    if (t == "Single") {
        if (v.kind == Kind::Float) { out = v.f; return true; }
        if (v.kind == Kind::Int) { out = f32store(static_cast<double>(v.i)); return true; }
        return false;
    }
    if (t == "Double") { if (!num(d)) return false; out = d; return true; }
    if (t == "Int32") { if (v.kind != Kind::Int) return false; out = static_cast<int32_t>(v.i); return true; }
    if (t == "UInt32") { if (v.kind != Kind::Int) return false; out = static_cast<uint32_t>(v.i); return true; }
    if (t == "Int64") { if (v.kind != Kind::Int) return false; out = v.i; return true; }
    if (t == "UInt64") { if (v.kind != Kind::Int) return false; out = static_cast<uint64_t>(v.i); return true; }
    if (t == "Int16") { if (v.kind != Kind::Int) return false; out = static_cast<int16_t>(v.i); return true; }
    if (t == "UInt16") { if (v.kind != Kind::Int) return false; out = static_cast<uint16_t>(v.i); return true; }
    if (t == "Byte") { if (v.kind != Kind::Int) return false; out = static_cast<uint8_t>(v.i); return true; }
    if (t == "Vector2" || t == "Vector3" || t == "Vector4" || t == "Quaternion") {
        if (v.kind != Kind::Vector || v.vecType != t) return false;
        if (t == "Vector2") out = Vector2f{v.vec[0], v.vec[1]};
        else if (t == "Vector3") out = Vector3f{v.vec[0], v.vec[1], v.vec[2]};
        else if (t == "Vector4") out = Vector4f{v.vec[0], v.vec[1], v.vec[2], v.vec[3]};
        else out = Quaternionf{v.vec[0], v.vec[1], v.vec[2], v.vec[3]};
        return true;
    }
    if (t == "FloatCurve") { if (v.kind == Kind::New && v.typeName == "FloatCurve") { out = FloatCurve{}; return true; } return false; }
    if (t == "DoubleCurve") { if (v.kind == Kind::New && v.typeName == "DoubleCurve") { out = DoubleCurve{}; return true; } return false; }
    return false;
}

ModuleValue FieldInitialValue(const ModuleFieldInfo& info, bool* evaluated) {
    if (evaluated) *evaluated = true;
    const std::string_view init = info.initializer;
    if (init.empty()) return DefaultFieldValue(info);
    CsValue v;
    ModuleValue out;
    if (EvaluateExpression(init, v) && ConvertConstant(v, info, out)) return out;
    if (evaluated) *evaluated = false;
    return DefaultFieldValue(info);
}

}  // namespace ksp::cs
