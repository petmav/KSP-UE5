// WP-05: the loaded-part records PartLoader produces (AvailablePart.cs + the data of the Part prefab).
//
// Public shape for consumers:
//   AvailablePart            -- one entry of PartLoader.LoadedPartsList (AvailablePart.cs fields), plus
//     .partPrefab            -- PartPrefab: the Part's public fields (PartFields, generated from Part.cs, same names
//                               and defaults), attach nodes, drag cubes, resources, MODULE records (PartModuleData:
//                               class + typed cfg fields; WP-06 instantiates modules from them), variants, FX/sound
//                               declarations, and the model/texture references the prefab was compiled from.
//     .assets                -- the files the game resolved for this part (cfg, models, texture replacements, internal):
//                               the UE import pipeline reads these GameData-relative paths.
// Nothing here owns Unity objects; mesh and texture decoding is the UE import pipeline's job.
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/io/url_dir.hpp"
#include "ksp/parts/part_data.hpp"
#include "ksp/parts/part_module_data.hpp"

namespace ksp {

// PartVariant.cs (data; Materials need the model: see PartVariant::textures)
struct PartVariant {
    std::string name;
    std::string displayName;
    NullableString primaryColor;
    NullableString secondaryColor;
    NullableString sizeGroup;
    float mass = 0.0f;
    float cost = 0.0f;
    std::vector<std::pair<std::string, bool>> infoGameObjects;   // GAMEOBJECTS
    std::vector<std::pair<std::string, std::string>> extraInfo;  // EXTRA_INFO
    std::vector<AttachNode> attachNodes;
    std::optional<AttachNode> srfAttachNode;
    std::vector<ConfigNode::NodePtr> textures;                   // TEXTURE nodes (PartMaterialInfo), raw
    std::vector<std::string> disabledAnimations;
    std::vector<std::string> disabledEvents;
    NullableString themeName;
    NullableAttachRules attachRules;
    // PartVariant.cs:263 Load
    void Load(const ConfigNode& node);
};

// A model instantiated into the prefab by CompileModel (PartLoader.cs:1765).
struct PartModelRef {
    std::string url;               // GameDatabase model url
    io::UrlFile* file = nullptr;   // the .mu / .dae file (nullptr: not found)
    bool fromModelNode = false;    // MODEL{} node (else: first model of the part's directory, "mesh =" legacy)
    std::string parent;            // MODEL parent =
    NullableString name;           // MODEL name = (the instance's GameObject name)
    std::optional<Vector3f> scale;     // MODEL scale = (else Vector3.one)
    std::optional<Vector3f> position;  // MODEL position =
    std::optional<Vector3f> rotation;  // MODEL rotation = (Euler degrees)
    bool iconHidden = false;
    // MODEL texture = <original name> : <new texture url>, accepted ones only (both textures exist)
    struct TextureReplacement {
        std::string original;
        std::string url;
        io::UrlFile* file = nullptr;
    };
    std::vector<TextureReplacement> textureReplacements;
};

// A file the game resolved for the part (the C9 `assets` table without the renderer texture slots).
struct PartAssetRef {
    std::string kind;   // cfg | model | texture_replacement | internal
    std::string slot;   // partUrl | MODEL[i] | "mesh (first model in <dir>)" | "MODEL[i] <original>" | INTERNAL.name
    std::string url;
    std::string file;   // GameData-relative path ("GameData/..."), empty when unresolved
    std::string source; // GameData | code | internal | unresolved
};

// The data of the Part prefab (Part.cs fields + what PartLoader attaches to it).
struct PartPrefab {
    std::string className = "Part";       // Part subclass of `module =` ("Part", "CompoundPart")
    std::string gameObjectName;           // CreatePart: partModel.name = partName
    PartFields fields;                    // every public Part field (C# defaults, then ApplyPartValue)
    CompoundPartFields compoundFields;    // CompoundPart's own fields (className == "CompoundPart")
    std::vector<AttachNode> attachNodes;
    std::optional<AttachNode> srfAttachNode;
    DragCubeList DragCubes;
    std::vector<PartResource> Resources;
    std::vector<PartModuleData> Modules;
    std::vector<ConfigNode::NodePtr> effects;  // EFFECTS nodes (Part.LoadEffects input)
    // fx_* / sound_* values in cfg order: name and raw value (FX prefabs and sounds are Unity/Audio assets)
    std::vector<std::pair<std::string, std::string>> fxDefinitions;
    std::vector<std::pair<std::string, std::string>> soundDefinitions;
    std::vector<PartModelRef> models;     // CompileModel inputs, in instantiation order
    // texture / normalmap / alphaCutoff / specPower / rimFalloff part values (applied to every MeshRenderer)
    std::vector<std::pair<std::string, std::string>> materialOverrides;
    // ModulePartVariants data (Part.baseVariant / Part.variants.variantList)
    std::optional<PartVariant> baseVariant;
    std::vector<PartVariant> variantList;
    bool hasVariantsModule = false;       // part.variants set in CompilePartInfo
};

// AvailablePart.cs
struct AvailablePart {
    // [Persistent] fields (LoadObjectFromConfig) and the rest, AvailablePart.cs:35-135, Init() defaults
    std::string name = "unknownPart";
    std::string title;
    std::string manufacturer;
    std::string author;
    std::string description;
    NullableString typeDescription;
    NullableString moduleInfo;
    NullableString resourceInfo;
    int32_t category = 0;                 // PartCategories (kEnum_PartCategories); default 0 = Propulsion
    std::string TechRequired;
    bool TechHidden = false;
    int32_t entryCost = 0;                // _entryCost
    std::string identicalParts;
    int32_t amountAvailable = 999;
    float cost = 0.0f;
    NullableString bulkheadProfiles;
    std::string tags = "*";
    bool showVesselNaming = false;
    std::string partUrl;
    float iconScale = 0.0f;
    bool mapActionsToSymmetryParts = true;
    float minimumMass = 0.0001f;
    float minimumRBMass = 0.001f;
    float minimumCost = 0.0f;
    int32_t preferredStage = 0;           // PreferredInitialStage
    ConfigNode::NodePtr internalConfig = ConfigNode::Create();
    ConfigNode::NodePtr partConfig;       // the PART node, as left by ParsePart (persistent keys removed)
    std::string configFileFullName;
    io::UrlConfig* partUrlConfig = nullptr;
    std::vector<std::string> fileTimes;
    float partSize = 0.0f;                // renderer-bounds magnitude: needs the model (not computed, see STATUS)
    bool costsFunds = true;
    std::unique_ptr<PartPrefab> partPrefab;

    // kspcore additions
    bool codeBuilt = false;               // one of PartLoader's prefab parts (EVA kerbals, flag): Unity prefab base
    bool prefabCompiled = true;           // false: code-built part without its PART cfg (prefab never Awake'd)
    std::string loadError;                // reading the record in game threw (C9 error rows), e.g. NullReferenceException
    std::vector<PartAssetRef> assets;

    // AvailablePart.Variants: partPrefab.variants.variantList (null when the prefab has no variants module)
    const std::vector<PartVariant>* Variants() const;
    float MinimumMass() const { return minimumMass; }
    void SetMinimumMass(float v);         // Mathf.Max(0.0001f, v)
    void SetMinimumRBMass(float v);       // Mathf.Max(0.001f, v)
    void AddFileTime(const io::UrlFile* file);  // AvailablePart.cs:247
};

}  // namespace ksp
