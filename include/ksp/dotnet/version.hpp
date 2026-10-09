// System.Version (Mono mscorlib = referencesource System/Version.cs): the version type the save upgrade pipeline uses.
// Unset Build / Revision are -1; comparison is component-wise (Major, Minor, Build, Revision), so "1.2" < "1.2.0".
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "ksp/core/exceptions.hpp"

namespace ksp::dotnet {

// System.ArgumentOutOfRangeException (derives from ArgumentException in .NET)
class ArgumentOutOfRangeException : public ksp::ArgumentException {
public:
    using ksp::ArgumentException::ArgumentException;
};

class Version {
public:
    // Version.cs:47-90 constructors (ArgumentOutOfRangeException for negative components)
    Version(int major, int minor, int build, int revision);
    Version(int major, int minor, int build);
    Version(int major, int minor);
    // Version.cs:92 Version(string): Parse (ArgumentException for fewer than 2 or more than 4 components,
    // FormatException / OverflowException from int.Parse, ArgumentOutOfRangeException for a negative component)
    explicit Version(std::string_view version);
    Version() = default;  // Version.cs:105: 0.0

    int Major() const { return major_; }
    int Minor() const { return minor_; }
    int Build() const { return build_; }
    int Revision() const { return revision_; }

    // Version.cs:156 CompareTo
    int CompareTo(const Version& value) const;
    bool operator==(const Version& o) const { return CompareTo(o) == 0; }
    bool operator!=(const Version& o) const { return CompareTo(o) != 0; }
    bool operator<(const Version& o) const { return CompareTo(o) < 0; }
    bool operator<=(const Version& o) const { return CompareTo(o) <= 0; }
    bool operator>(const Version& o) const { return CompareTo(o) > 0; }
    bool operator>=(const Version& o) const { return CompareTo(o) >= 0; }

    // Version.cs:236 ToString(): 2, 3 or 4 fields depending on which components are set
    std::string ToString() const;
    // Version.cs:245 ToString(fieldCount): ArgumentException when the requested field is unset or fieldCount > 4
    std::string ToString(int fieldCount) const;

    // Version.cs:304 TryParse
    static bool TryParse(std::string_view input, Version& result);

private:
    int major_ = 0;
    int minor_ = 0;
    int build_ = -1;
    int revision_ = -1;
};

}  // namespace ksp::dotnet
