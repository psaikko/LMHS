#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

#include <vector>
#include <utility>       // pair
#include <unordered_map>
#include <string>
#include <iosfwd>

#include "NonoptHS.h"

class GlobalConfig {
 public:

  static inline GlobalConfig& get() {
    static GlobalConfig config;
    return config;
  }

  bool initialized;

  //
  bool floatWeights;
  unsigned streamPrecision;

  bool doRerefuteCores;
  bool doMinimizeCores;
  unsigned minimizeSizeLimit;

  bool doDisjointPhase;
  bool doNonOpt;
  bool doEquivSeed;
  bool doResetClauses;
  bool doInvertActivity;

  bool doEnumeration;
  int enumerationType;
  unsigned enumerationLimit;

  long minimizePropLimit;
  long minimizeConfLimit;

  bool doLimitNonopt;

  // use only MIP
  bool solveAsMIP;

  bool isLCNF;
  bool inFileAssumptions;
  bool doFindBvarClauses;

  bool use_coprocessor;
  std::string pre_techniques;
  bool pre_lcnf;
  bool pre_group;
  bool pre_group_only;
  bool pre_only;

  double fracSize;
  unsigned nonoptLimit;
  unsigned satTimeLimit;

  int verbosity;
  int logHS;

  // MIP solver parameters
  int MIP_verbosity;
  int MIP_threads;
  int MIP_intensity;
  int MIP_replace;
  int MIP_capacity;
  int MIP_poplim;
  int MIP_start;
  int MIP_emph;

  // SAT solver parameters
  int SAT_userPolarity;
  bool SAT_lubyRestart;
  bool SAT_rndInitActivity;
  double SAT_varDecay;
  double SAT_clauseDecay;
  double SAT_rndFreq;
  double SAT_rndSeed;
  double SAT_gcFrac;
  double SAT_restartInc;
  double SAT_learntSizeFactor;
  double SAT_learntSizeInc;
  int SAT_verbosity;
  int SAT_phaseSaving;
  int SAT_restartFirst;
  int SAT_ccMinMode;

  int MIP_exportModel;
  std::string MIP_modelFile;

  void printHelp(std::ostream & out);
  void parseArgs(int argc, const char** argv, std::ostream & out);

  NonoptHS::funcType nonoptPrimary, nonoptSecondary;

 private:
  GlobalConfig() { initialized = 0; };
  GlobalConfig(GlobalConfig const&);
  void operator=(GlobalConfig const&);

};

#endif