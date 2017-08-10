#include <string>
#include <ostream>
#include <climits>

#include "GlobalConfig.h"
#include "NonoptHS.h"
#include "ArgsParser.h"
#include "Util.h"

#include "OptionParsing.h"

using namespace std;

void GlobalConfig::printHelp(ostream & out) {
  
  #include "OptionHelp.cpp"

  out << "For integer/double/string -valued options use \"--option value\"" << endl;
  out << "For boolean-valued options use \"--option\" or \"--no-option\"" << endl;

  exit(0);
}

// handle user-settable options from command line arguments
void GlobalConfig::parseArgs(int argc, const char** argv, ostream & out) {

  ArgsParser args(argv, argv + argc);

  unordered_map<string, string> flag_type;

  #include "OptionParsing.cpp"

  if (string_beginsWith(nonOptType, "common")) {
    nonoptPrimary = NonoptHS::common;
  } else if (string_beginsWith(nonOptType, "greedy")) {
    nonoptPrimary = NonoptHS::greedy;
  } else if (string_beginsWith(nonOptType, "frac")) {
    nonoptPrimary = NonoptHS::frac(fracSize);
  } else if (string_beginsWith(nonOptType, "disjoint")) {
    nonoptPrimary = NonoptHS::disjoint;
  } else { // "none"
    nonoptPrimary = nullptr;
  }

  if (string_endsWith(nonOptType, "+greedy")) {
    nonoptSecondary = NonoptHS::greedy;
  } else {
    nonoptSecondary = nullptr;
  }

  if (minimizeAlgorithmStr == "destructive") {
    minAlg = MinimizeAlgorithm::destructive;
  } else if (minimizeAlgorithmStr == "constructive") {
    minAlg = MinimizeAlgorithm::constructive;
  } else if (minimizeAlgorithmStr == "binarysearch") {
    minAlg = MinimizeAlgorithm::binary;
  } else if (minimizeAlgorithmStr == "cardinality") {
    minAlg = MinimizeAlgorithm::cardinality;
  } else if (minimizeAlgorithmStr == "rerefute") {
    minAlg = MinimizeAlgorithm::rerefute;
  }    

  // validate flags
  for (int i = 2; i < argc; ++i) {
    for (auto p : flag_type) {
      string flag = p.first;
      string type = p.second;
      
      if (argv[i] == ("--" + flag)) {
        if (type == "bool") {
          goto next;
        } else {
          // skip parameter
          goto skip;
        }
      }

      if (type == "bool") {
        if (argv[i] == ("--no-" + flag)) {
          goto next;
        }
      }
    }

    cout << "Unexpected parameter " << argv[i] << endl;
    exit(1);

    skip: i += 1;

    next: continue;
  }

  initialized = true;

  srand(randomSeed);

  is_LCNF = inFileAssumptions || preprocess;
  use_LP = lpNonOpt || CPLEX_reducedCosts;

  if (help != 0) {
    printHelp(out);
  }

  //if (args.hasFlag("--help")) printHelp(out);
}
