#include "confignode_tree.hpp"

using namespace ksp;
using ojson = nlohmann::ordered_json;

// ConfigNodeGen.cs TreeJson: {"name","id","comment","values":[[name,value,comment]...],"nodes":[...]}
ojson TreeJson(const ConfigNode& n) {
    ojson o;
    o["name"] = n.name;
    o["id"] = n.id;
    o["comment"] = n.comment ? ojson(*n.comment) : ojson(nullptr);
    ojson vals = ojson::array();
    for (const auto& v : n.values()) vals.push_back(ojson::array({v.name, v.value, v.comment ? ojson(*v.comment) : ojson(nullptr)}));
    o["values"] = std::move(vals);
    ojson nodes = ojson::array();
    for (const auto& c : n.nodes()) nodes.push_back(TreeJson(*c));
    o["nodes"] = std::move(nodes);
    return o;
}

