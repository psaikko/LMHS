#pragma once

#include <vector>
#include <utility>       // pair
#include <unordered_map>
#include <string>
#include <iosfwd>

#include "Defines.h"

class GlobalConfig {
 public:

  static inline GlobalConfig& get() {
    static GlobalConfig config;
    return config;
  }

  #define PRINT_NO_HS     0
  #define PRINT_OPT_HS    1
  #define PRINT_NONOPT_HS 2
  #define PRINT_ALL_HS    3

  bool initialized;
  bool use_LP;
  bool is_LCNF;

  MinimizeAlgorithm minAlg;

  #include "OptionDeclarations.cpp"

  void printHelp(std::ostream & out);
  void parseArgs(int argc, const char** argv, std::ostream & out);

  NonOptHSFunc nonoptPrimary, nonoptSecondary;

 private:
  GlobalConfig() { initialized = 0; };
  GlobalConfig(GlobalConfig const&);
  void operator=(GlobalConfig const&);

};
