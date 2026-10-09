// save_corpus: WP-04 round-trip gate over the original game's save and craft files.
//
//   save_corpus --saves <KSP-dev/saves> --ships <KSP-dev/Ships> [--out <dir>] [--filter <substr>] [--verbose]
//
// *.sfs: ConfigNode.Load -> new Game(root) -> Game.Save into a new root -> ConfigNode.Save bytes (R1); then the same from
// R1 (R2). *.craft: ConfigNode.Load -> ShipConstruct.LoadShip -> SaveShip (R1, R2).
// The environment of each file is the one that wrote it (SetupEnvironmentFromSave): envInfo, LoaderInfo, installed DLC,
// the GAME-level values / nodes owned by other systems (MESSAGESYSTEM, mods) fed back through onGameStateSave as in KSP.
//
// Gate (exit code 0 = pass):
//  * R2 == R1 byte for byte for every file (a re-saved file loads and saves to itself);
//  * R1 == the file byte for byte, OR every structural difference between the file's tree and R1's tree is explained
//    by a documented rule of what KSP itself rewrites (ExplainValue / CompareNodes; categories are printed with
//    counts). Any unexplained difference fails the gate.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/config/kspparseable.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/dotnet/version.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"

namespace fs = std::filesystem;
using namespace ksp;

namespace {

std::string ReadBytes(const fs::path& p) {
    auto s = dotnet::ReadAllBytes(p.string());
    return s ? *s : std::string();
}
std::vector<std::string> Lines(const std::string& text) {
    return dotnet::SplitLinesLikeReadLine(dotnet::DecodeStreamText(text));
}
ConfigNode::NodePtr LoadText(const std::string& bytes) { return ConfigNode::LoadFromLines(Lines(bytes)); }
std::string U8(const fs::path& p) {
    const std::u8string s = p.generic_u8string();
    return std::string(reinterpret_cast<const char*>(s.data()), s.size());
}

// GAME-level nodes Game(ConfigNode) consumes; every other GAME child belongs to a system that writes it back through
// GameEvents.onGameStateSave (MessageSystem's MESSAGESYSTEM, mods' nodes).
const std::set<std::string> kGameNodes = {"SCENARIO",    "CometNames",  "LoaderInfo",     "PARAMETERS",
                                          "FLIGHTSTATE", "RemovedROCs", "MISSIONTOSTART", "ROSTER"};
// GAME-level values Game reads or writes; other GAME values were added by onGameStateSave listeners (mods)
const std::set<std::string> kGameValues = {"version", "Title", "Description", "linkURL", "linkCaption", "Mode",
                                           "Status", "Seed", "ROCSeed", "scene", "editor", "flag", "launchID",
                                           "defaultVABLaunchSite", "defaultSPHLaunchSite", "modded", "envInfo",
                                           "versionFull", "versionCreated"};

struct AdditionalSystems {
    std::vector<std::pair<std::string, std::string>> values;
    std::vector<ConfigNode::NodePtr> nodes;
    void OnSave(ConfigNode* n) {
        for (const auto& [k, v] : values) n->AddValue(k, v);
        for (const auto& c : nodes) n->AddNode(c->CreateCopy());
    }
};

bool AnyValue(const ConfigNode& n, std::string_view name) {
    if (n.HasValue(name)) return true;
    for (const auto& c : n.nodes())
        if (AnyValue(*c, name)) return true;
    return false;
}

void ResetEnvironment() {
    SaveEnvironment::Reset();
    SaveEnvironment& env = SaveEnvironment::Current();
    env.newGuidHashCode = [] {
        static uint32_t state = 0x5EED1234u;  // deterministic stand-in for Guid.NewGuid().GetHashCode()
        state = state * 1664525u + 1013904223u;
        return static_cast<int32_t>(state);
    };
    env.tickCount = [] { return 0; };
}

// The machine that wrote the file: GameDatabase environment string and loader info, installed DLC (ROCSeed only
// exists with Breaking Ground, PreFailState only with Making History), a ROCManager when RemovedROCs exists, the
// systems that write extra GAME values / nodes.
void SetupEnvironmentFromSave(const ConfigNode& root, const std::string& rawBytes, ROCManagerRemovedROCs& roc,
                              AdditionalSystems& extra) {
    ResetEnvironment();
    SaveEnvironment& env = SaveEnvironment::Current();
    const ConfigNode* game = root.GetNode("GAME");
    extra.values.clear();
    extra.nodes.clear();
    if (game == nullptr) return;
    // GameDatabase.EnvironmentInfo starts and ends with " - " (newlines replaced); ConfigNode.Load trims values, so
    // the running game's string is taken from the raw line
    if (game->HasValue("envInfo")) {
        const std::string key = "\n\tenvInfo = ";
        const size_t at = rawBytes.find(key);
        if (at != std::string::npos) {
            const size_t start = at + key.size();
            const size_t end = rawBytes.find_first_of("\r\n", start);
            env.gameDatabaseEnvironmentInfo = rawBytes.substr(start, end - start);
        }
    }
    if (const ConfigNode* li = game->GetNode("LoaderInfo"))
        for (const auto& v : li->values())
            if (v.value == "True") env.gameDatabaseLoaderInfo.push_back(v.name);
    if (game->HasValue("ROCSeed") || game->GetNode("SCENARIO", "name", std::string("DeployedScience")) != nullptr)
        env.SetStockExpansionInstalled("Serenity", true);
    if (AnyValue(*game, "PreFailState")) env.SetStockExpansionInstalled("MakingHistory", true);
    if (game->HasNode("RemovedROCs")) env.rocManager = &roc;
    for (const auto& v : game->values())
        if (!kGameValues.count(v.name)) extra.values.emplace_back(v.name, v.value);
    for (int i = 0; i < game->nodes().Count(); ++i)
        if (!kGameNodes.count(game->nodes()[i]->name)) extra.nodes.push_back(game->nodes()[i]->CreateCopy());
}

std::string SaveGameText(Game& g) {
    ConfigNode root;
    g.Save(root);
    return root.SaveToString();
}

// ======================================== structural comparison and rules ==========================================

// Field lists KSP's Save methods write (always or conditionally) per node type, from the decompiled Save code.
const std::map<std::string, std::set<std::string>>& SaveFields() {
    static const std::map<std::string, std::set<std::string>> m = {
        {"GAME", kGameValues},
        {"FLIGHTSTATE", {"version", "UT", "activeVessel", "mapViewFiltering", "commNetUIModeTracking", "commNetUIModeFlight"}},
        {"VESSEL",
         {"pid", "persistentId", "name", "type", "sit", "landed", "skipGroundPositioning", "skipGroundPositioningForDroppedPart",
          "vesselSpawning", "launchedFrom", "landedAt", "displaylandedAt", "splashed", "met", "lct", "lastUT",
          "distanceTraveled", "cln", "clnRsn", "root", "lat", "lon", "alt", "hgt", "nrm", "rot", "CoM", "stg", "prst", "ref",
          "ctrl", "PQSMin", "PQSMax", "GroupOverride", "OverrideDefault", "OverrideActionControl", "OverrideAxisControl",
          "OverrideGroupNames", "altDispState"}},
        {"PART",
         {"name", "cid", "uid", "mid", "persistentId", "launchID", "parent", "position", "rotation", "mirror", "symMethod",
          "istg", "resPri", "dstg", "sqor", "sepI", "sidx", "attm", "sameVesselCollision", "cData", "sym", "srfN", "attN",
          "mass", "shielded", "temp", "tempExt", "tempExtUnexp", "staticPressureAtm", "expt", "state", "PreFailState",
          "attached", "autostrutMode", "rigidAttachment", "flag", "rTrf", "modCost", "modMass", "moduleVariantName",
          "moduleCargoStackableQuantity", "partRendererBoundsIgnore", "crew"}},
        {"ORBIT", {"SMA", "ECC", "INC", "LPE", "LAN", "MNA", "EPH", "REF"}},
        {"KERBAL",
         {"name", "gender", "type", "trait", "brave", "dumb", "badS", "veteran", "tour", "state", "inactive",
          "inactiveTimeEnd", "gExperienced", "outDueToG", "ToD", "idx", "extraXP", "hasHelmetOn", "hasNeckRingOn",
          "hasVisorDown", "lightR", "lightG", "lightB", "completedFirstEVA", "suit", "hero", "suitTexturePath",
          "normalTexturePath", "spritePath", "comboId"}},
        {"TARGET", {"type", "vesselID", "partID", "moduleIdx", "tgtId"}},
        {"WAYPOINT", {"type", "navigationID"}},
        {"VESSELNAMING", {"name", "type", "priority"}},
        {"PARAMETERS", {"preset"}},
        // ShipConstruct.SaveShip (ShipConstruct.cs:153)
        {"CRAFT",
         {"ship", "version", "description", "type", "size", "steamPublishedFileId", "persistentId", "rot", "missionFlag",
          "vesselType", "OverrideDefault", "OverrideActionControl", "OverrideAxisControl", "OverrideGroupNames"}},
        {"CRAFT_PART",
         {"part", "partName", "persistentId", "pos", "attPos", "attPos0", "rot", "attRot", "attRot0", "mir", "symMethod",
          "autostrutMode", "rigidAttachment", "istg", "resPri", "dstg", "sidx", "sqor", "sepI", "attm",
          "sameVesselCollision", "modCost", "modMass", "modSize", "cData", "link", "sym", "srfN", "attN"}},
    };
    return m;
}

// Typed nodes whose loader reads a fixed set of names (anything else is ignored and not written back)
bool IsTypedValueNode(const std::string& type) {
    return type == "GAME" || type == "FLIGHTSTATE" || type == "ORBIT" || type == "KERBAL" || type == "TARGET" ||
           type == "WAYPOINT" || type == "VESSELNAMING" || type == "PARAMETERS" || type.rfind("PARAMETERS/", 0) == 0 ||
           type == "CRAFT" || type == "CRAFT_PART";
}

// Known child nodes of typed nodes (other children are ignored by the loader)
const std::map<std::string, std::set<std::string>>& KnownChildren() {
    static const std::map<std::string, std::set<std::string>> m = {
        {"VESSEL", {"PART", "WAYPOINT", "VESSELMODULES", "DISCOVERY", "CTRLSTATE", "FLIGHTPLAN", "TARGET", "ACTIONGROUPS", "ORBIT"}},
        {"PART", {"MODULE", "ACTIONS", "PARTDATA", "RESOURCE", "VESSELNAMING", "EVENTS", "EFFECTS"}},
        {"KERBAL", {"EVACHUTE", "INVENTORY", "CAREER_LOG", "FLIGHT_LOG"}},
        {"ROSTER", {"KERBAL"}},
        {"FLIGHTSTATE", {"VESSEL"}},
        {"CRAFT", {"PART"}},
        {"CRAFT_PART", {"MODULE", "ACTIONS", "PARTDATA", "RESOURCE", "VESSELNAMING", "EVENTS", "EFFECTS"}},
    };
    return m;
}

const std::set<std::string> kStockParameterNodes = {
    "FLIGHT", "EDITOR", "TRACKINGSTATION", "SPACECENTER", "DIFFICULTY", "CAREER", "CommNetParams", "AdvancedParams",
    "MissionParamsGeneral", "MissionParamsExtras", "MissionParamsFacilities"};

std::set<std::string> ParameterMembers(const std::string& node) {
    static std::map<std::string, std::set<std::string>> cache;
    auto it = cache.find(node);
    if (it != cache.end()) return it->second;
    GameParameters gp;
    std::set<std::string> names;
    const GameParameters::ParameterNode* p = nullptr;
    if (node == "FLIGHT") p = &gp.Flight;
    else if (node == "EDITOR") p = &gp.Editor;
    else if (node == "TRACKINGSTATION") p = &gp.TrackingStation;
    else if (node == "SPACECENTER") p = &gp.SpaceCenter;
    else if (node == "DIFFICULTY") p = &gp.Difficulty;
    else if (node == "CAREER") p = &gp.Career;
    else if (kStockParameterNodes.count(node)) p = gp.CustomParams(node);
    if (p)
        for (const auto& m : p->Members())
            if (m.kind == ParameterMemberInfo::Kind::Field || m.kind == ParameterMemberInfo::Kind::Property)
                names.insert(m.name);
    return cache[node] = names;
}

struct FileInfo {
    bool old = false;     // written by a version before 1.12.5 (fields added since, older layouts)
    bool modded = false;  // GAME.modded = True
    std::map<std::string, int> persistentIdCounts;
};

bool NumericEqual(const std::string& a, const std::string& b) {
    const std::vector<std::string> pa = dotnet::Split(a, ",", false), pb = dotnet::Split(b, ",", false);
    if (pa.size() != pb.size()) return false;
    for (size_t i = 0; i < pa.size(); ++i) {
        double x = 0, y = 0;
        if (!dotnet::TryParseDouble(pa[i], x) || !dotnet::TryParseDouble(pb[i], y)) return false;
        if (!(x == y || (x != x && y != y))) return false;
    }
    return true;
}

// the same float32 values (an older KSP wrote floats with ToString(), today's ConfigNode uses "G9"); ',' and '|' lists
bool FloatEqual(const std::string& a, const std::string& b) {
    auto split = [](const std::string& s) {
        std::vector<std::string> out;
        for (const auto& x : dotnet::Split(s, ",", false))
            for (const auto& y : dotnet::Split(x, "|", false)) out.push_back(y);
        return out;
    };
    const std::vector<std::string> pa = split(a), pb = split(b);
    if (pa.size() != pb.size()) return false;
    for (size_t i = 0; i < pa.size(); ++i) {
        float x = 0, y = 0;
        if (!dotnet::TryParseSingle(pa[i], x) || !dotnet::TryParseSingle(pb[i], y)) return false;
        if (!(x == y || (x != x && y != y))) return false;
    }
    return true;
}

bool IsInteger(const std::string& s) {
    int64_t v = 0;
    return dotnet::TryParseInt64(s, v);
}

// node "type" used by the rules: the node name, PARAMETERS children as PARAMETERS/<name>
std::string TypeOf(const std::vector<std::string>& path) {
    if (path.empty()) return "";
    if (path.front() == "CRAFT") return path.size() == 1 ? "CRAFT" : "CRAFT_" + path.back();
    if (path.size() >= 2 && path[path.size() - 2] == "PARAMETERS") return "PARAMETERS/" + path.back();
    return path.back();
}
std::string PathKey(const std::vector<std::string>& path) {
    std::string s;
    for (const auto& p : path) s += (s.empty() ? "" : "/") + p;
    return s;
}

// attN / srfN of an older layout completed by SaveShip: the id and partner are kept, every vector the file has keeps its
// float value, the rest come from the part prefab in KSP (proxies here)
bool AttachCompleted(const std::string& orig, const std::string& r1) {
    const size_t co = orig.find(','), cr = r1.find(',');
    if (co == std::string::npos || cr == std::string::npos || orig.substr(0, co) != r1.substr(0, cr)) return false;
    auto fields = [](const std::string& s) {
        std::vector<std::string> out;
        for (const auto& a : dotnet::Split(s, ",", false))
            for (const auto& b : dotnet::Split(a, "_", false)) out.push_back(b);
        return out;
    };
    const std::vector<std::string> fo = fields(orig.substr(co + 1)), fr = fields(r1.substr(cr + 1));
    if (fo.size() > fr.size() || fo.empty() || fo[0] != fr[0]) return false;
    for (size_t i = 1; i < fo.size(); ++i)
        if (fo[i] != fr[i] && !FloatEqual(fo[i], fr[i])) return false;
    return true;
}

// Explain one value-level difference; "" = unexplained
std::string ExplainValue(const std::string& kind, const std::vector<std::string>& path, const std::string& name,
                         const std::string& orig, const std::string& r1, const FileInfo& fi) {
    const std::string type = TypeOf(path);
    const auto& fields = SaveFields();
    const auto fit = fields.find(type);
    bool kspWrites = fit != fields.end() && fit->second.count(name);
    if (type.rfind("PARAMETERS/", 0) == 0) kspWrites = ParameterMembers(type.substr(11)).count(name) > 0;
    if (kind == "value-added") {
        if (kspWrites && fi.old) return "added: KSP's Save writes " + type + "." + name + " (missing in an older file; default/derived value)";
        if (type == "RESOURCE" && (name == "flowState" || name == "amount" || name == "maxAmount"))
            return "added: ProtoPartResourceSnapshot.Save SetValue(" + name + ", createIfNotFound)";
        if (type == "CAREER_LOG" || type == "FLIGHT_LOG") return "added: FlightLog.Save writes flight";
        return "";
    }
    if (kind == "value-dropped" && type == "CRAFT_PART" && name == "pSym")
        return "dropped: CRAFT PART pSym (old symmetry by craftID) is written as sym";
    if (kind == "value-dropped") {
        if (IsTypedValueNode(type) && !kspWrites) return "dropped: " + type + " loader ignores " + name + " (not a KSP field)";
        if (type == "VESSEL" && (name == "cln" || name == "clnRsn")) return "dropped: VESSEL cln/clnRsn only written when autoClean";
        if (type == "PART" && name == "connected") return "dropped: PART.connected is read and ignored";
        if (type == "PART" && name == "PreFailState") return "dropped: PART.PreFailState only with Making History";
        if (type == "PART" && name == "cData" && orig.empty()) return "dropped: PART.cData only written when non-empty";
        if (type == "GAME" && name == "ROCSeed") return "dropped: GAME.ROCSeed only with Breaking Ground";
        if (type == "CAREER_LOG" || type == "FLIGHT_LOG") return "dropped: FlightLog entry that is not 'type[,target]'";
        return "";
    }
    // value-changed
    if (type == "CRAFT" && name == "version") return "changed: craft version stamped with the running game's (SaveShip)";
    if (type == "CRAFT_PART" && (name == "attN" || name == "srfN") && fi.old && AttachCompleted(orig, r1))
        return "changed: older craft attach node completed by SaveShip (prefab vectors: proxies without the part database)";
    if (type == "CRAFT_PART" && name == "srfN") {
        // ShipConstruct.LoadShip -> KSPUtil.GetAttachNodeInfo parses mesh and vectors only for the id "srfAttach"; for
        // any other id the shared attachMeshName variable keeps the previous part's value, which SaveShip writes back
        const std::vector<std::string> a = dotnet::Split(orig, ",", false), b = dotnet::Split(r1, ",", false);
        if (a.size() == 6 && b.size() == 6 && a[0] != "srfAttach" && a[0] == b[0] && a[1] == b[1] && a[2] != b[2] &&
            FloatEqual(a[3] + "," + a[4] + "," + a[5], b[3] + "," + b[4] + "," + b[5]))
            return "changed: srfN mesh of a non-'srfAttach' node = GetAttachNodeInfo's stale attachMeshName (KSP quirk)";
    }
    if (type == "CRAFT_PART" && (name == "modCost" || name == "modMass" || name == "modSize") && fi.old)
        return "changed: module cost/mass/size (module host; older file layout)";
    if (fi.old && NumericEqual(orig, r1)) return "changed: number re-formatted (same value; older file)";
    if (fi.old && FloatEqual(orig, r1))
        return "changed: float re-formatted (same float32 value; older file, KSP writes \"G9\" today)";
    bool bo = false, br = false;
    if (dotnet::TryParseBool(orig, bo) && dotnet::TryParseBool(r1, br) && bo == br) return "changed: bool re-formatted";
    if (fi.old && IsInteger(orig) && !IsInteger(r1) && !r1.empty() &&
        (name == "Mode" || name == "state" || name == "type" || name == "sit" || name == "editor" ||
         name == "altDispState" || name == "symMethod" || name == "autostrutMode"))
        return "changed: enum written by name (" + type + "." + name + ")";
    if (type == "GAME" && (name == "envInfo" || name == "modded")) return "changed: GAME." + name + " is the running game's";
    if (type == "LoaderInfo") return "changed: LoaderInfo flags re-evaluated against GameDatabase.loaderInfo";
    if (type == "GAME" && name == "Description" && dotnet::Replace(orig, "^", "") == r1)
        return "changed: Description '^' -> newline -> stripped by AddValue";
    if (name == "persistentId" && fi.persistentIdCounts.count(orig) && fi.persistentIdCounts.at(orig) > 1)
        return "changed: duplicate persistentId re-generated (FlightGlobals.Check*persistentId)";
    if (name == "persistentId" && (type == "VESSEL" || type == "PART") && (orig == "0" || !IsInteger(orig)))
        return "changed: invalid / zero persistentId re-generated";
    if (type == "PART" && !kspWrites && KSPParseable(orig).Save() == r1)
        return "changed: unknown PART value written by KSPParseable.Save (\"value, TYPE\")";
    if (type == "VESSEL" && !kspWrites && KSPParseable(orig).value == r1)
        return "changed: unknown VESSEL value written as KSPParseable.value";
    if (type == "KERBAL" && name == "trait" && orig.empty()) return "changed: empty trait assigned by SetExperienceTrait";
    if (type == "KERBAL" && name == "suit") return "changed: suit reset to Default (DLC suit not installed)";
    if (type == "FLIGHTSTATE" && name == "activeVessel") return "changed: activeVessel re-indexed by CleanUpUnsanitaryEVAKerbals";
    if (type == "GAME" && (name == "defaultVABLaunchSite" || name == "defaultSPHLaunchSite"))
        return "changed: launch sites reset outside SANDBOX/SCIENCE_SANDBOX";
    if (type == "GAME" && name == "Seed" && orig == "-1") return "changed: Seed -1 populated (PopulateGameSeed)";
    if (type == "VESSEL" && name == "OverrideGroupNames") return "changed: OverrideGroupNames padded to 4 (ArrayMinSize)";
    if (type == "VESSEL" && (name == "OverrideDefault" || name == "OverrideActionControl" || name == "OverrideAxisControl"))
        return "changed: Override arrays padded to 4 (ArrayMinSize)";
    if (type == "PART" && (name == "srfN" || name == "attN")) {
        try {
            if (AttachNodeSnapshot(orig).Save() == r1) return "changed: AttachNodeSnapshot re-written (\"id, idx[,mesh]\")";
        } catch (...) {
        }
    }
    return "";
}

struct Comparison {
    std::map<std::string, int> explained;
    std::vector<std::string> unexplained;
    bool onlyDropped = true;                         // every difference is a "dropped:" category
    std::set<const ConfigNode::Value*> droppedValues;  // elements of the file KSP's loaders drop
    std::set<const ConfigNode*> droppedNodes;
};

void Note(Comparison& c, const std::string& why, const std::string& what) {
    if (why.rfind("dropped:", 0) != 0) c.onlyDropped = false;
    if (why.empty()) {
        if (c.unexplained.size() < 40) c.unexplained.push_back(what);
        else if (c.unexplained.size() == 40) c.unexplained.push_back("...");
        c.explained["UNEXPLAINED"]++;
    } else {
        c.explained[why]++;
    }
}

void CompareNodes(const ConfigNode& o, const ConfigNode& r, std::vector<std::string>& path, const FileInfo& fi,
                  Comparison& c) {
    const std::string type = TypeOf(path);
    const std::string key = PathKey(path);
    // ---- values (aligned by name and occurrence) ----
    std::map<std::string, std::vector<std::string>> ov, rv;
    std::map<std::string, std::vector<const ConfigNode::Value*>> ovPtr;
    std::vector<std::string> oOrder, rOrder;
    for (const auto& v : o.values()) {
        if (!ov.count(v.name)) oOrder.push_back(v.name);
        ov[v.name].push_back(v.value);
        ovPtr[v.name].push_back(&v);
    }
    for (const auto& v : r.values()) {
        if (!rv.count(v.name)) rOrder.push_back(v.name);
        rv[v.name].push_back(v.value);
    }
    std::set<std::string> names;
    for (const auto& kv : ov) names.insert(kv.first);
    for (const auto& kv : rv) names.insert(kv.first);
    const std::vector<std::string> none;
    for (const std::string& n : names) {
        const auto& a = ov.count(n) ? ov.at(n) : none;
        const auto& b = rv.count(n) ? rv.at(n) : none;
        const size_t common = std::min(a.size(), b.size());
        for (size_t i = 0; i < common; ++i)
            if (a[i] != b[i])
                Note(c, ExplainValue("value-changed", path, n, a[i], b[i], fi),
                     "value-changed " + key + "." + n + ": '" + a[i] + "' -> '" + b[i] + "'");
        for (size_t i = common; i < a.size(); ++i) {
            Note(c, ExplainValue("value-dropped", path, n, a[i], "", fi), "value-dropped " + key + "." + n + " = '" + a[i] + "'");
            c.droppedValues.insert(ovPtr.at(n)[i]);
        }
        for (size_t i = common; i < b.size(); ++i)
            Note(c, ExplainValue("value-added", path, n, "", b[i], fi), "value-added " + key + "." + n + " = '" + b[i] + "'");
    }
    {
        std::vector<std::string> oc, rc;
        for (const auto& n : oOrder)
            if (rv.count(n)) oc.push_back(n);
        for (const auto& n : rOrder)
            if (ov.count(n)) rc.push_back(n);
        if (oc != rc)
            Note(c, fi.old ? "reordered: values written in KSP's fixed Save order (older file layout)" : "",
                 "value order differs in " + key);
    }
    // ---- child nodes (aligned by name and occurrence) ----
    std::map<std::string, std::vector<const ConfigNode*>> on, rn;
    std::vector<std::string> onOrder, rnOrder;
    auto childKey = [](const ConfigNode& ch) {
        if (ch.name == "SCENARIO") return ch.name + "[" + ch.GetValue("name").value_or("") + "]";
        return ch.name;
    };
    for (const auto& ch : o.nodes()) {
        on[childKey(*ch)].push_back(ch.get());
        onOrder.push_back(childKey(*ch));
    }
    for (const auto& ch : r.nodes()) {
        rn[childKey(*ch)].push_back(ch.get());
        rnOrder.push_back(childKey(*ch));
    }
    std::set<std::string> nnames;
    for (const auto& kv : on) nnames.insert(kv.first);
    for (const auto& kv : rn) nnames.insert(kv.first);
    const auto& known = KnownChildren();
    const std::vector<const ConfigNode*> noNodes;
    for (const std::string& n : nnames) {
        const auto& a = on.count(n) ? on.at(n) : noNodes;
        const auto& b = rn.count(n) ? rn.at(n) : noNodes;
        const size_t common = std::min(a.size(), b.size());
        const std::string childName = n.rfind("SCENARIO[", 0) == 0 ? "SCENARIO" : n;
        for (size_t i = 0; i < common; ++i) {
            path.push_back(childName);
            CompareNodes(*a[i], *b[i], path, fi, c);
            path.pop_back();
        }
        for (size_t i = common; i < a.size(); ++i) {
            std::string why;
            const auto kit = known.find(type);
            if (type == "PARAMETERS" && n == "RESEARCHANDDEVELOPMENT")
                why = "dropped: PARAMETERS/RESEARCHANDDEVELOPMENT is ignored by GameParameters.Load";
            else if (type == "PARAMETERS" && !kStockParameterNodes.count(n))
                why = "dropped: PARAMETERS/<mod type> (GameParameters.LoadCustomNode: type not loaded)";
            else if (kit != known.end() && !kit->second.count(n))
                why = "dropped: " + type + " loader ignores child node " + n;
            else if (type == "VESSEL" && n == "TARGET" && !a[i]->HasData())
                why = "dropped: empty TARGET (written only with data)";
            else if (type == "VESSEL" && n == "WAYPOINT")
                why = "dropped: WAYPOINT without navigation id";
            else if (type == "ROSTER" && (n == "CREW" || n == "APPLICANTS"))
                why = "dropped: ROSTER " + n + " converted to KERBAL";
            Note(c, why, "node-dropped " + key + "/" + n);
            c.droppedNodes.insert(a[i]);
        }
        for (size_t i = common; i < b.size(); ++i) {
            std::string why;
            if ((type == "GAME" && (n == "CometNames" || n == "LoaderInfo" || n == "RemovedROCs")) ||
                (type == "PARAMETERS" && kStockParameterNodes.count(n)) ||
                (type == "VESSEL" && (n == "DISCOVERY" || n == "FLIGHTPLAN" || n == "CTRLSTATE" || n == "VESSELMODULES")) ||
                (type == "KERBAL" && (n == "CAREER_LOG" || n == "FLIGHT_LOG")))
                why = "added: KSP's Save always writes " + type + "/" + n;
            else if (type == "CRAFT_PART" && (n == "EVENTS" || n == "ACTIONS" || n == "PARTDATA"))
                why = "added: SaveShip always writes PART/" + n;
            else if (type == "ROSTER" && n == "KERBAL" && (on.count("CREW") || on.count("APPLICANTS")))
                why = "added: ROSTER KERBAL from CREW/APPLICANTS";
            Note(c, why, "node-added " + key + "/" + n);
        }
    }
    {
        std::vector<std::string> oc, rc;
        for (const auto& n : onOrder)
            if (rn.count(n)) oc.push_back(n);
        for (const auto& n : rnOrder)
            if (on.count(n)) rc.push_back(n);
        auto trim = [](std::vector<std::string>& v, const std::map<std::string, std::vector<const ConfigNode*>>& other) {
            std::map<std::string, size_t> seen;
            std::vector<std::string> out;
            for (const auto& n : v)
                if (seen[n]++ < other.at(n).size()) out.push_back(n);
            v = out;
        };
        trim(oc, rn);
        trim(rc, on);
        if (oc != rc)
            Note(c, fi.old ? "reordered: child nodes written in KSP's fixed Save order (older file layout)" : "",
                 "node order differs in " + key);
    }
}

// the file without the elements KSP's loaders drop (mod-owned values / nodes), as KSP 1.12.5 would write it
void PruneInto(const ConfigNode& src, ConfigNode& dst, const Comparison& c) {
    for (const auto& v : src.values())
        if (!c.droppedValues.count(&v)) dst.AddValue(v.name, v.value);
    for (const auto& ch : src.nodes())
        if (!c.droppedNodes.count(ch.get())) PruneInto(*ch, *dst.AddNode(ch->name), c);
}

void CountPersistentIds(const ConfigNode& n, std::map<std::string, int>& counts) {
    for (const auto& v : n.values())
        if (v.name == "persistentId") counts[v.value]++;
    for (const auto& ch : n.nodes()) CountPersistentIds(*ch, counts);
}

struct FileResult {
    std::string rel;
    std::string kind;
    bool r1Identical = false;
    bool r2Identical = false;
    bool old = false;  // not written by KSP 1.12.5
    bool prunedIdentical = false;  // R1 == the file without the dropped (mod-owned) elements, byte for byte
    std::string error;
    Comparison cmp;
};

void WriteFile(const fs::path& p, const std::string& bytes) {
    fs::create_directories(p.parent_path());
    dotnet::WriteAllBytes(p.string(), bytes);
}

void WriteOutputs(const std::string& outDir, const std::string& rel, const FileResult& r, const std::string& r1,
                  const std::string& r2) {
    if (outDir.empty() || (r.r1Identical && r.r2Identical)) return;
    WriteFile(fs::path(dotnet::Utf8ToWide(outDir)) / fs::path(dotnet::Utf8ToWide(rel + ".r1")), r1);
    if (!r.r2Identical) WriteFile(fs::path(dotnet::Utf8ToWide(outDir)) / fs::path(dotnet::Utf8ToWide(rel + ".r2")), r2);
}

FileResult RunSfs(const fs::path& p, const std::string& rel, const std::string& outDir) {
    FileResult r;
    r.rel = rel;
    r.kind = "sfs";
    try {
        const std::string original = ReadBytes(p);
        ConfigNode::NodePtr root = LoadText(original);
        ConfigNode::NodePtr pristine = LoadText(original);  // Game(ConfigNode) modifies the tree it reads
        ROCManagerRemovedROCs roc;
        AdditionalSystems extra;
        SetupEnvironmentFromSave(*root, original, roc, extra);
        events::GameEvents::onGameStateSave.Add(&extra, &AdditionalSystems::OnSave);
        std::string r1, r2;
        try {
            SaveEnvironment::Current().persistentIds.ClearpersistentIdDictionaries();
            Game g(root.get());
            r1 = SaveGameText(g);
            ConfigNode::NodePtr root2 = LoadText(r1);
            SaveEnvironment::Current().persistentIds.ClearpersistentIdDictionaries();
            Game g2(root2.get());
            r2 = SaveGameText(g2);
        } catch (...) {
            events::GameEvents::onGameStateSave.Remove(&extra, &AdditionalSystems::OnSave);
            throw;
        }
        events::GameEvents::onGameStateSave.Remove(&extra, &AdditionalSystems::OnSave);
        r.r1Identical = r1 == original;
        r.r2Identical = r2 == r1;
        if (!r.r1Identical) {
            FileInfo fi;
            const ConfigNode* game = pristine->GetNode("GAME");
            const std::string* ver = game ? game->GetValuePtr("version") : nullptr;
            dotnet::Version v;
            fi.old = ver == nullptr || !dotnet::Version::TryParse(*ver, v) || v < dotnet::Version(1, 12, 5);
            fi.modded = game && game->GetValue("modded") == "True";
            CountPersistentIds(*pristine, fi.persistentIdCounts);
            r.old = fi.old;
            ConfigNode::NodePtr r1Tree = LoadText(r1);
            std::vector<std::string> path;
            CompareNodes(*pristine, *r1Tree, path, fi, r.cmp);
            if (r.cmp.onlyDropped && !r.cmp.explained.count("UNEXPLAINED")) {
                ConfigNode pruned;
                PruneInto(*pristine, pruned, r.cmp);
                // ConfigNode.Load trimmed envInfo's leading / trailing blanks (" - ... - "); the file has them
                if (ConfigNode* g = pruned.GetNode("GAME"); g != nullptr && g->HasValue("envInfo"))
                    g->SetValue("envInfo", SaveEnvironment::Current().gameDatabaseEnvironmentInfo);
                r.prunedIdentical = pruned.SaveToString() == r1;
                if (!r.prunedIdentical && !outDir.empty())
                    WriteFile(fs::path(dotnet::Utf8ToWide(outDir)) / fs::path(dotnet::Utf8ToWide(rel + ".pruned")),
                              pruned.SaveToString());
            }
        }
        WriteOutputs(outDir, rel, r, r1, r2);
    } catch (const std::exception& e) {
        r.error = e.what();
    }
    return r;
}

FileResult RunCraft(const fs::path& p, const std::string& rel, const std::string& outDir) {
    FileResult r;
    r.rel = rel;
    r.kind = "craft";
    try {
        ResetEnvironment();
        const std::string original = ReadBytes(p);
        ConfigNode::NodePtr root = LoadText(original);
        ConfigNode::NodePtr pristine = LoadText(original);
        std::string r1, r2;
        {
            SaveEnvironment::Current().persistentIds.ClearpersistentIdDictionaries();
            ShipConstruct ship;
            std::string err;
            if (!ship.LoadShip(*root, 0, true, err)) throw std::runtime_error("LoadShip failed: " + err);
            r1 = ship.SaveShip()->SaveToString();
        }
        {
            ConfigNode::NodePtr root2 = LoadText(r1);
            SaveEnvironment::Current().persistentIds.ClearpersistentIdDictionaries();
            ShipConstruct ship;
            std::string err;
            if (!ship.LoadShip(*root2, 0, true, err)) throw std::runtime_error("LoadShip (R2) failed: " + err);
            r2 = ship.SaveShip()->SaveToString();
        }
        r.r1Identical = r1 == original;
        r.r2Identical = r2 == r1;
        if (!r.r1Identical) {
            FileInfo fi;
            const std::string* ver = pristine->GetValuePtr("version");
            dotnet::Version v;
            fi.old = ver == nullptr || !dotnet::Version::TryParse(*ver, v) || v < dotnet::Version(1, 12, 5) ||
                     !pristine->HasValue("OverrideGroupNames");
            CountPersistentIds(*pristine, fi.persistentIdCounts);
            r.old = fi.old;
            ConfigNode::NodePtr r1Tree = LoadText(r1);
            std::vector<std::string> path = {"CRAFT"};
            CompareNodes(*pristine, *r1Tree, path, fi, r.cmp);
            if (r.cmp.onlyDropped && !r.cmp.explained.count("UNEXPLAINED")) {
                ConfigNode pruned;
                PruneInto(*pristine, pruned, r.cmp);
                // ConfigNode.Load trimmed envInfo's leading / trailing blanks (" - ... - "); the file has them
                if (ConfigNode* g = pruned.GetNode("GAME"); g != nullptr && g->HasValue("envInfo"))
                    g->SetValue("envInfo", SaveEnvironment::Current().gameDatabaseEnvironmentInfo);
                r.prunedIdentical = pruned.SaveToString() == r1;
                if (!r.prunedIdentical && !outDir.empty())
                    WriteFile(fs::path(dotnet::Utf8ToWide(outDir)) / fs::path(dotnet::Utf8ToWide(rel + ".pruned")),
                              pruned.SaveToString());
            }
        }
        WriteOutputs(outDir, rel, r, r1, r2);
    } catch (const std::exception& e) {
        r.error = e.what();
    }
    return r;
}

}  // namespace

int main(int argc, char** argv) {
    std::string savesDir, shipsDir, outDir, filter;
    bool verbose = false;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--saves" && i + 1 < argc) savesDir = argv[++i];
        else if (a == "--ships" && i + 1 < argc) shipsDir = argv[++i];
        else if (a == "--out" && i + 1 < argc) outDir = argv[++i];
        else if (a == "--filter" && i + 1 < argc) filter = argv[++i];
        else if (a == "--verbose") verbose = true;
    }
    log::SetSink([](log::Level, std::string_view) {});
    // MainMenu.cs:186 (MainMenu.Start): GetDefaultParameters runs before any save is loaded; its first GameParameters
    // sorts the static custom-parameter type list, which fixes the order every later instance saves in
    GameParameters::GetDefaultParameters(Game_Modes::CAREER, GameParameters::Preset::Normal);

    struct Input {
        fs::path path;
        std::string rel;
        bool craft;
    };
    std::vector<Input> inputs;
    auto collect = [&](const std::string& dir, const std::string& prefix) {
        if (dir.empty()) return;
        const fs::path base(dotnet::Utf8ToWide(dir));
        for (const auto& e : fs::recursive_directory_iterator(base)) {
            if (!e.is_regular_file()) continue;
            const auto ext = e.path().extension();
            if (ext != ".sfs" && ext != ".craft") continue;
            const std::string rel = prefix + U8(fs::relative(e.path(), base));
            if (!filter.empty() && rel.find(filter) == std::string::npos) continue;
            inputs.push_back({e.path(), rel, ext == ".craft"});
        }
    };
    collect(savesDir, "saves/");
    collect(shipsDir, "Ships/");
    std::sort(inputs.begin(), inputs.end(), [](const Input& a, const Input& b) { return a.rel < b.rel; });

    std::vector<FileResult> results;
    for (const Input& in : inputs) {
        const auto t0 = std::chrono::steady_clock::now();
        results.push_back(in.craft ? RunCraft(in.path, in.rel, outDir) : RunSfs(in.path, in.rel, outDir));
        if (verbose)
            std::cout << "  " << in.rel << " "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count()
                      << " ms" << std::endl;
    }

    std::map<std::string, std::pair<int, int>> catTotals;  // category -> (count, files)
    int failures = 0;
    std::map<std::string, std::map<std::string, int>> summary;  // kind -> counters
    for (const auto& r : results) {
        auto& s = summary[r.kind];
        s["files"]++;
        if (!r.error.empty()) {
            ++failures;
            s["errors"]++;
            std::cout << "FAIL " << r.rel << ": exception: " << r.error << "\n";
            continue;
        }
        if (r.r1Identical) s["r1_identical"]++;
        if (r.r2Identical) s["r2_idempotent"]++;
        else {
            ++failures;
            std::cout << "FAIL " << r.rel << ": second round trip differs from the first\n";
        }
        if (!r.r1Identical) {
            if (r.cmp.explained.count("UNEXPLAINED")) {
                ++failures;
                s["r1_unexplained"]++;
                std::cout << "FAIL " << r.rel << ": " << r.cmp.explained.at("UNEXPLAINED") << " unexplained differences\n";
                for (const auto& u : r.cmp.unexplained) std::cout << "      " << u.substr(0, 300) << "\n";
            } else {
                s["r1_explained"]++;
            }
            // a file KSP 1.12.5 wrote must come back byte for byte once the mod-owned elements KSP's loaders drop are
            // removed; the only other admitted difference is the srfN stale-mesh quirk of non-'srfAttach' nodes
            if (!r.old && !r.prunedIdentical) {
                bool ok = true;
                for (const auto& [cat, n] : r.cmp.explained)
                    if (cat.rfind("dropped:", 0) != 0 && cat.find("stale attachMeshName") == std::string::npos) ok = false;
                if (!ok) {
                    ++failures;
                    std::cout << "FAIL " << r.rel << ": KSP 1.12.5 file differs beyond the dropped mod-owned elements\n";
                }
            }
            for (const auto& [cat, n] : r.cmp.explained) {
                const std::string k = r.kind + (r.old ? " older: " : " 1.12.5: ") + cat;
                catTotals[k].first += n;
                catTotals[k].second += 1;
            }
            s[r.old ? "r1_diff_older" : "r1_diff_1125"]++;
            if (r.prunedIdentical) s["pruned_identical"]++;
            if (verbose) {
                std::cout << "  " << r.rel << ":";
                for (const auto& [cat, n] : r.cmp.explained) std::cout << " [" << cat << " x" << n << "]";
                std::cout << "\n";
            }
        }
    }
    std::cout << "\nDocumented R1 differences (category: values, files):\n";
    for (const auto& [cat, t] : catTotals) std::cout << "  " << cat << ": " << t.first << ", " << t.second << "\n";
    for (const auto& [kind, s] : summary) {
        auto get = [&](const char* k) {
            auto it = s.find(k);
            return it == s.end() ? 0 : it->second;
        };
        std::cout << kind << ": " << get("files") << " files, R1 byte-identical " << get("r1_identical")
                  << ", R1 differences all documented " << get("r1_explained") << " (KSP 1.12.5 files "
                  << get("r1_diff_1125") << ", older files " << get("r1_diff_older") << "), unexplained "
                  << get("r1_unexplained") << ", byte-identical once the dropped mod-owned elements are removed "
                  << get("pruned_identical") << ", R2 idempotent " << get("r2_idempotent") << ", exceptions "
                  << get("errors") << "\n";
    }
    std::cout << (failures == 0 ? "PASS" : "FAIL") << " (" << failures << " failures)\n";
    return failures == 0 ? 0 : 1;
}
