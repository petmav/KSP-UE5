// Jolt spike: command-line options and the measurement sets.
#pragma once

#include <map>
#include <string>
#include <vector>

#include "flight.hpp"

namespace spike {

class KspContext;

struct Args {
    Args(int argc, char** argv);
    std::vector<std::string> positional;
    std::map<std::string, std::string> opts;
    bool Has(const std::string& k) const { return opts.count(k) != 0; }
    std::string Str(const std::string& k, const std::string& def) const;
    double Num(const std::string& k, double def) const;
    std::vector<std::string> List(const std::string& k, const std::string& def) const;
};

std::string CraftPath(const std::string& nameOrPath);
FlightConfig ConfigFromArgs(const Args& a);
void PrintResultHeader();
void PrintResult(const FlightResult& r, const FlightConfig& cfg);
int RunBench(KspContext& ctx, const std::string& name, const Args& args);

}  // namespace spike
