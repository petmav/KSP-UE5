// WP-13: FlagNameManager (FlagNameManager.cs). FlagMesh.cs and FlagOrientation.cs belong to WP-48 (rendering); only the data
// members FlagNameManager reads are declared here (flagTransform / flagRend are Unity objects).
#pragma once

#include <map>
#include <string>
#include <vector>

namespace ksp {

// FlagOrientation.cs
enum class FlagOrientation : int { LANDSCAPE = 0, PORTRAIT = 1 };

// FlagMesh.cs (data members)
struct FlagMesh {
    std::string name;
    int index = 0;
    int indexOffset = 0;
    FlagOrientation flagOrientation = FlagOrientation::LANDSCAPE;
    std::string displayName;
    std::string meshName;
    float mass = 0.0f;
    float cost = 0.0f;
    std::string variantNames;
};

class FlagNameManager {
public:
    enum class NameType { SizeName, DisplayName };
    static constexpr int STRUCTURAL_PART = 1000;     // FlagNameManager.cs:13
    static constexpr int NON_STRUCTURAL_PART = 0;    // :15

    FlagNameManager() = default;
    explicit FlagNameManager(const std::vector<FlagMesh>& flagMeshes) { Initialize(flagMeshes); }  // :17 (null list: default ctor)
    // :25
    void Initialize(const std::vector<FlagMesh>& flagMeshes);
    // :55 SizeNames: an empty list when the orientation / structure type is unknown (a new list in C#)
    std::vector<std::string> SizeNames(FlagOrientation flagOrientation, int structureType) const;
    // :64
    bool HasSizeNames(FlagOrientation flagOrientation, int structureType) const;
    // :73
    std::vector<std::string> DisplayNames(FlagOrientation flagOrientation, int structureType) const;

private:
    using NameMap = std::map<NameType, std::vector<std::string>>;
    std::map<FlagOrientation, std::map<int, NameMap>> flagNames_;
    bool created_ = false;  // flagNames != null
};

}  // namespace ksp
