// PartLoader, data side (Assembly-CSharp/PartLoader.cs): compiles every PART config of the GameDatabase into an
// AvailablePart record (available_part.hpp), in KSP's order and with KSP's quirks:
//
// - LoadedPartsList starts with PartLoader's 9 prefab parts built in code from Unity assets (PartLoader.cs:169;
//   kerbalEVA, kerbalEVAfemale, flag, kerbalEVAVintage, kerbalEVAfemaleVintage, kerbalEVAFuture, kerbalEVAfemaleFuture,
//   kerbalEVASlimSuit, kerbalEVASlimSuitFemale). A PART config with one of these names is applied ONTO that prefab
//   (ApplyPartValue, EFFECTS/DRAG_CUBE/RESOURCE/MODULE with __OVERLOAD, sounds; PartLoader.cs:511) instead of being
//   compiled. kspcore has no Unity prefab: the record starts from Part's C# defaults (documented gap: the serialized
//   prefab values, e.g. the EVA mass, are not known), and a prefab part without a PART config is marked
//   prefabCompiled = false / loadError = NullReferenceException (the 4 DLC suits when the DLC is not installed).
// - ParsePart (PartLoader.cs:642): name '_' -> '.', LoadObjectFromConfig(AvailablePart, removeAfterUse) strips the
//   persistent keys from partConfig, entryCost, SanitizePartCosts, ApplyPartValue for every value (double fields are
//   parsed with float.Parse: sic), node_* attach nodes (6..11 values, scaled by rescaleFactor x scale in value order),
//   fx_* / sound_* / texture / material values, skinMaxTemp <= 0 -> maxTemp, DRAG_CUBE / EFFECTS / NODE / RESOURCE /
//   MODULE nodes, ModuleTripLogger for vesselType > Unknown, INTERNAL, minimum mass/cost, auto tags.
// - Drag cubes: the part cfg's DRAG_CUBE node wins; otherwise the top-level PART{url=<partUrl>} node of
//   <game root>/PartDatabase.cfg (nested PART nodes are ignored; the version line must match; "timestamps", when
//   present, must match the cfg/model file times). Parts with neither would have cubes rendered (not possible here:
//   DragCubes stay empty and `cubesNeedRendering` is set).
// - CompilePartInfo (PartLoader.cs:1169): crewed parts lose PerCommandSeatReduction (any ModuleCommand) or
//   PerSeatReduction per seat from `mass`; the variants module's variant list becomes AvailablePart.Variants.
// Module instantiation (PartModule.OnLoad behaviour, GetInfo texts, module mass/cost modifiers) is WP-06.
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/parts/available_part.hpp"

namespace ksp {

class GameDatabase;

class PartLoader {
public:
    struct Options {
        // <game root>/PartDatabase.cfg; empty: no database (every part without in-cfg cubes needs rendering)
        std::string partDatabasePath;
        // Versioning.VersionString of the build the database was written by
        std::string versionString = "1.12.5.3190 (WindowsPlayer x64)";
        // The prefab parts' names in PartLoader.parts order (PartLoader.cs:481 switch indices 0..8)
        std::vector<std::string> codeBuiltParts = DefaultCodeBuiltParts();
        // PhysicsGlobals values used by the loader (taken from PhysicsGlobals::Instance() when it exists)
        float perSeatReduction = 0.0f;
        float perCommandSeatReduction = 0.0f;
        float partMassMin = 0.0001f;
        float partRBMassMin = 0.001f;
        bool usePhysicsGlobalsInstance = true;
    };
    static std::vector<std::string> DefaultCodeBuiltParts();

    PartLoader();
    ~PartLoader();
    PartLoader(const PartLoader&) = delete;
    PartLoader& operator=(const PartLoader&) = delete;
    static PartLoader* Instance();

    // PartLoader.cs:161 CompileAll (parts; PROP / INTERNAL spaces and variant themes are recorded as configs only)
    void CompileAll(const GameDatabase& db, const Options& options);

    std::vector<std::unique_ptr<AvailablePart>> loadedParts;  // LoadedPartsList
    std::vector<std::string> log;                             // "PartLoader: Compiling Part '<url>'" and errors
    std::vector<std::string> cubesNeedRendering;              // part names whose cubes KSP would render
    ConfigNode::NodePtr partDatabase;                         // PartDatabase.cfg as loaded (+ in-memory updates)

    // PartLoader.cs:384 / 398
    const AvailablePart* getPartInfoByName(std::string_view name) const;
    bool DoesPartExist(std::string_view name) const { return getPartInfoByName(name) != nullptr; }

    // PartLoader.cs:985 ApplyPartValue: true when the value was consumed (a field was set, or a warning logged).
    static bool ApplyPartValue(PartPrefab& part, const ConfigNode::Value& nodeValue);
    // PartLoader.cs:1581 InferBulkheadProfiles (dead code in KSP: never called)
    static std::string InferBulkheadProfiles(const ConfigNode& n);
    // BasePartCategorizer.cs:311 GeneratePartAutoTags (default English tag tables)
    static std::string GeneratePartAutoTags(const AvailablePart& p);

    // PartLoader.cs:294/329: the PartDatabase.cfg node of a part (created in memory when missing; reset when the
    // timestamps do not match), and its DRAG_CUBE child
    ConfigNode* GetDatabaseConfig(const AvailablePart& ap);
    ConfigNode* GetDatabaseConfig(const AvailablePart& ap, std::string_view nodeName);
    void SetDatabaseConfig(const AvailablePart& ap, const ConfigNode::NodePtr& newNode);

private:
    std::unique_ptr<AvailablePart> ParsePart(const GameDatabase& db, io::UrlConfig& urlConfig, ConfigNode& node);
    void ApplyPrefabOverlay(const GameDatabase& db, AvailablePart& ap, io::UrlConfig& urlConfig);
    void CompilePartInfo(AvailablePart& ap);
    bool CompileModel(const GameDatabase& db, io::UrlConfig& cfg, ConfigNode& partCfg, AvailablePart& ap);
    void InitializePartDatabase();
    void AddModule(PartPrefab& part, ConfigNode& node, bool forceAwake);
    PartModuleData* AddModule(PartPrefab& part, std::string_view moduleName);
    void ResolveAssets(const GameDatabase& db, AvailablePart& ap);

    Options options_;
};

}  // namespace ksp
