// System.Version (referencesource System/Version.cs, used by Mono 5.11's mscorlib); see include/ksp/dotnet/version.hpp.
#include "ksp/dotnet/version.hpp"

#include <vector>

#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"

namespace ksp::dotnet {

// Version.cs:47
Version::Version(int major, int minor, int build, int revision) {
    if (major < 0) throw ArgumentOutOfRangeException("major: Version's parameters must be greater than or equal to zero.");
    if (minor < 0) throw ArgumentOutOfRangeException("minor: Version's parameters must be greater than or equal to zero.");
    if (build < 0) throw ArgumentOutOfRangeException("build: Version's parameters must be greater than or equal to zero.");
    if (revision < 0)
        throw ArgumentOutOfRangeException("revision: Version's parameters must be greater than or equal to zero.");
    major_ = major;
    minor_ = minor;
    build_ = build;
    revision_ = revision;
}

// Version.cs:66
Version::Version(int major, int minor, int build) {
    if (major < 0) throw ArgumentOutOfRangeException("major: Version's parameters must be greater than or equal to zero.");
    if (minor < 0) throw ArgumentOutOfRangeException("minor: Version's parameters must be greater than or equal to zero.");
    if (build < 0) throw ArgumentOutOfRangeException("build: Version's parameters must be greater than or equal to zero.");
    major_ = major;
    minor_ = minor;
    build_ = build;
}

// Version.cs:80
Version::Version(int major, int minor) {
    if (major < 0) throw ArgumentOutOfRangeException("major: Version's parameters must be greater than or equal to zero.");
    if (minor < 0) throw ArgumentOutOfRangeException("minor: Version's parameters must be greater than or equal to zero.");
    major_ = major;
    minor_ = minor;
}

namespace {
enum class ParseFailure { None, ArgumentNull, Argument, ArgumentOutOfRange, Format, Overflow };

// Version.cs:325 TryParseVersion + 365 TryParseComponent (Int32.TryParse(component, NumberStyles.Integer, Invariant))
ParseFailure TryParseVersion(std::string_view version, Version& result, std::string& badComponent) {
    const std::vector<std::string> parts = Split(version, ".", false);
    const size_t n = parts.size();
    if (n < 2 || n > 4) return ParseFailure::Argument;
    int comps[4] = {0, 0, -1, -1};
    for (size_t i = 0; i < n; ++i) {
        int32_t v = 0;
        if (!TryParseInt32(parts[i], v)) {
            badComponent = parts[i];
            // VersionResult.GetVersionParseException: int.Parse rethrown to get Format vs Overflow
            int64_t wide = 0;
            return TryParseInt64(parts[i], wide) ? ParseFailure::Overflow : ParseFailure::Format;
        }
        if (v < 0) return ParseFailure::ArgumentOutOfRange;
        comps[i] = v;
    }
    if (n == 2) result = Version(comps[0], comps[1]);
    else if (n == 3) result = Version(comps[0], comps[1], comps[2]);
    else result = Version(comps[0], comps[1], comps[2], comps[3]);
    return ParseFailure::None;
}
}  // namespace

// Version.cs:92 (Parse -> VersionResult.GetVersionParseException)
Version::Version(std::string_view version) {
    Version v;
    std::string bad;
    switch (TryParseVersion(version, v, bad)) {
    case ParseFailure::None: *this = v; return;
    case ParseFailure::Argument: throw ksp::ArgumentException("Version string portion was too short or too long.");
    case ParseFailure::ArgumentOutOfRange:
        throw ArgumentOutOfRangeException("Version's parameters must be greater than or equal to zero.");
    case ParseFailure::Overflow: throw OverflowException("Value was either too large or too small for an Int32.");
    default: throw FormatException("Input string was not in a correct format.");
    }
}

// Version.cs:304
bool Version::TryParse(std::string_view input, Version& result) {
    std::string bad;
    Version v;
    if (TryParseVersion(input, v, bad) != ParseFailure::None) {
        result = Version();
        return false;
    }
    result = v;
    return true;
}

// Version.cs:156
int Version::CompareTo(const Version& value) const {
    if (major_ != value.major_) return major_ > value.major_ ? 1 : -1;
    if (minor_ != value.minor_) return minor_ > value.minor_ ? 1 : -1;
    if (build_ != value.build_) return build_ > value.build_ ? 1 : -1;
    if (revision_ != value.revision_) return revision_ > value.revision_ ? 1 : -1;
    return 0;
}

// Version.cs:236
std::string Version::ToString() const {
    if (build_ == -1) return ToString(2);
    if (revision_ == -1) return ToString(3);
    return ToString(4);
}

// Version.cs:245
std::string Version::ToString(int fieldCount) const {
    switch (fieldCount) {
    case 0: return "";
    case 1: return FormatInt64(major_);
    case 2: return FormatInt64(major_) + "." + FormatInt64(minor_);
    default:
        if (build_ == -1) throw ksp::ArgumentException("fieldCount: Argument must be between 0 and 2.");
        if (fieldCount == 3) return FormatInt64(major_) + "." + FormatInt64(minor_) + "." + FormatInt64(build_);
        if (revision_ == -1) throw ksp::ArgumentException("fieldCount: Argument must be between 0 and 3.");
        if (fieldCount == 4)
            return FormatInt64(major_) + "." + FormatInt64(minor_) + "." + FormatInt64(build_) + "." +
                   FormatInt64(revision_);
        throw ksp::ArgumentException("fieldCount: Argument must be between 0 and 4.");
    }
}

}  // namespace ksp::dotnet
