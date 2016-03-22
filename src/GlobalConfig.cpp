#include "GlobalConfig.h"
#include "ArgsParser.h"
#include "Util.h"

#include <string>
#include <ostream>
#include <climits>

using namespace std;

// TODO: do something about this mess

void GlobalConfig::printHelp(ostream & out) {
  vector<vector<string> > pds;
  
  // parameter defaultValue possibleValues description
  pds.push_back({"--verb", "1", "int", "[0 .. 3]", 
    "Set the maxsat solver verbosity level"});
  pds.push_back({"--sat-verb", "0", "int", "[0 .. 3]", 
    "Set the sat solver verbosity level"});
  pds.push_back({"--mip-verb", "0", "int", "[0 .. 3]", 
    "Set the mip solver verbosity level"});
  pds.push_back({"--disjoint-pre", "true", "bool", "", 
    "Find disjoint cores before main algorithm loop"});
  pds.push_back({"--minimize", "true", "bool", "", 
    "Minimize found cores"});
  pds.push_back({"--rerefute", "true", "bool", "", 
    "Re-refute found cores"});
  pds.push_back({"--equiv-seed", "true", "bool", "", 
    "Use blocking variable equivalence seeding for MIP solver"});
  pds.push_back({"--reset", "true", "bool", "", 
    "Clear learnt clauses between refutations, re-refutations"});
  pds.push_back({"--invert", "true", "bool", "", 
    "Invert variable activity between refutations, re-refutations"});

  pds.push_back({"--enumerate", "false", "bool", "", 
    "Incrementally enumerate multiple MaxSAT solutions"});
  pds.push_back({"--enumerate", "1", "int", "{1, 2, -1, -2}", 
    "1: only enumerate optimal solutions\n"
    "2: also report best sub-optimal solutions (use --n-enum to limit enumeration)\n"
    "-1, -2: same as above, but enumerate only solutions with unique sets of\n"
    "        satisfied soft clauses."});
  pds.push_back({"--n-enum", "imax", "int", "[1 .. imax]", 
    "Maximum number of solutions to enumerate"});

  pds.push_back({"--nonopt", "true", "bool", "", 
    "Use approximations min cost hitting set to find multiple cores per iteration"});
  pds.push_back({"--nonopt", "disjoint+greedy", "string", "{disjoint, common, frac, greedy}", 
    "Strategy to use for minimum cost hitting set approximations\n"
    "common: add the variable with the most occurrences in cores so far\n"
    "frac: like common but add some fraction of the most occurring\n"
    "      variables to the hitting set\n"
    "disjoint: add entire core to hitting set. Finds disjoint set of cores between\n"
    "          optimal hitting sets. Equivalent to --nonopt frac --frac-size 1.0\n"
    "greedy: use a greedy MCHS algorithm\n"
    "[strategy]+greedy: use greedy algorithm as fallback for another strategy."});

  pds.push_back({"--frac-size", "0.1", "double", "(0 .. 1]", 
    "When using \"--nonopt frac\", the fraction of the core to add to the nonopt HS"});

  pds.push_back({"--cgmr", "false", "bool", "", 
    "Relax cores in SAT formula instead of finding hitting sets.\n"
    "When used with --no-rerefute and --no-minimize, essentially identical to\n"
    "eva solver of AAAI 2014 paper \"Maximum Satisfiability using core-guided MAXSAT Resolution\""});
  pds.push_back({"--cgmr", "0", "int", "[0 .. 1]",
    "Additional heuristics for core-guided MAXSAT resolution\n"
    "0: (default) relax first core found\n"
    "1: use binary search to find max min weight core"});

  pds.push_back({"--ip", "false", "bool", "", 
    "Solve instance using IP solver with standard MaxSAT to IP transformation"});

  pds.push_back({"--relax-disjoint", "false", "bool", "", 
    "Find and relax an initial set of disjoint cores as a\n"
    "presolving step before MaxHS"});  

  pds.push_back({"--preprocess", "true", "bool", "", 
    "Enable SAT-based preprocessing"});
  pds.push_back({"--pre-lcnf", "true", "bool", "", 
    "Treat preprocessed instance as LCNF."
    "Reuses auxiliary variables from preprocessing in solver."});
  pds.push_back({"--pre-group", "true", "bool", "", 
    "Use group detection in preprocessor."});
  pds.push_back({"--pre-group-only", "false", "bool", "", 
    "Only use group detection."});
  pds.push_back({"--pre-techniques", "[vpusb]+", "string", "valid coprocessor 2 technique parameter", 
    "Technique selection string passed to preprocessor."});

  pds.push_back({"--infile-assumps", "false", "bool", "",
    "LCNF: get assumption variables and polarities from \"c assumptions ...\" line in input"});

  pds.push_back({"--float-weights", "true", "bool", "",
    "Disable if output precision issues arise with very\n"
    "large integer weights."});
  pds.push_back({"--stream-precision", "15", "int", "",
    "C++ output stream decimal precision (std::setprecision)"});

  for (auto pd : pds) {
    if (pd[2] == "bool") {
      out << pd[0] << ", " << "--no" << pd[0].substr(1);
      if (pd[1] == "true")
        out << " (default: on)" << endl;
      else
        out << " (default: off)" << endl;
    } else {
      out << pd[0] << " (default: " << pd[1] << ")" << endl;
      out << "\t" << pd[2] << ": " << pd[3] << endl;
    }
    string description = pd[4];
    size_t i = 0;
    while ((i = description.find("\n", i)) != string::npos) {
      description.replace(i, 1, "\n\t");
      i += 2;
    }
    out << "\t" << description << endl;
    out << endl;
  }

  out << "For integer/double/string -valued options use \"--option value\"" << endl;
  out << "For boolean-valued options use \"--option\" or \"--no-option\"" << endl;
  out << "Command line options are _not_ checked for correctness\n" << endl;

  exit(0);
}

// handle user-settable options from command line arguments
void GlobalConfig::parseArgs(int argc, const char** argv, ostream & out) {

  ArgsParser args(argv, argv + argc);

  if (args.hasFlag("--help")) printHelp(out);
  verbosity = args.getIntOption("-verb", 1);
  logHS = args.getIntOption("-log-hs", 0);
  SAT_verbosity = args.getIntOption("-sat-verb", 0);
  MIP_verbosity = args.getIntOption("-mip-verb", 0);
  doDisjointPhase = args.getBoolOption("-disjoint-pre", true);
  doMinimizeCores = args.getBoolOption("-minimize", true);
  doRerefuteCores = args.getBoolOption("-rerefute", true);
  minimizeSizeLimit = args.getIntOption("-min-size-lim", INT_MAX);
  
  doEquivSeed = args.getBoolOption("-equiv-seed", true);
  doResetClauses = args.getBoolOption("-reset", true);
  doInvertActivity = args.getBoolOption("-invert", true);

  doEnumeration = args.getBoolOption("-enumerate", false);
  enumerationType = args.getIntOption("-enumerate", 1);
  enumerationLimit = args.getIntOption("-enum-n", INT_MAX);

  doNonOpt = args.getBoolOption("-nonopt", true);
  fracSize = args.getDoubleOption("-frac-size", 0.1);
  // type of non-optimal hitting set method to use
  if (doNonOpt) {
    string nonOptType = args.getStringOption("-nonopt", "disjoint+greedy");
    if (string_beginsWith(nonOptType, "common")) {
      nonoptPrimary = NonoptHS::common;
    } else if (string_beginsWith(nonOptType, "greedy")) {
      nonoptPrimary = NonoptHS::greedy;
    } else if (string_beginsWith(nonOptType, "frac")) {
      nonoptPrimary = NonoptHS::frac(fracSize);
    } else if (string_beginsWith(nonOptType, "disjoint")) {
      nonoptPrimary = NonoptHS::disjoint;
    } else {
      terminate(1, 
          "Expected one of \"common\" \"frac\" \"greedy\" "
          "\"disjoint\" --nonopt, got \"%s\"\n",
          nonOptType.c_str());
    }
    if (string_endsWith(nonOptType, "+greedy")) {
      nonoptSecondary = NonoptHS::greedy;
    }
  }

  doCGMR = args.getBoolOption("-cgmr", false);
  if (doCGMR)
    CGMR_mode = args.getIntOption("-relax-only", 0);  

  solveAsMIP = args.getBoolOption("-ip", false);

  srand(args.getIntOption("-random-seed", 9));

  // blocking vars / assumptions specified using wcnf comment line
  inFileAssumptions = args.getBoolOption("-infile-assumps", false);

  // CPLEX options
  MIP_threads = args.getIntOption("-mip-threads", 1);
  MIP_intensity = args.getIntOption("-mip-intensity", 2);
  MIP_replace = args.getIntOption("-mip-replace", 2);
  MIP_capacity = args.getIntOption("-mip-capacity", 10);
  MIP_poplim = args.getIntOption("-mip-poplim", 100);
  MIP_start = args.getIntOption("-mip-start", 1);
  MIP_emph = args.getIntOption("-mip-emph", 0);

  // minisat options
  SAT_userPolarity = args.getIntOption("-sat-polarity", 0);
  SAT_varDecay = args.getDoubleOption("-sat-var-decay", 0.95);
  SAT_clauseDecay = args.getDoubleOption("-sat-cla-decay", 0.999);
  SAT_rndFreq = args.getDoubleOption("-sat-rnd-freq", 0);
  SAT_rndSeed = args.getDoubleOption("-sat-rnd-seed", 91648253);
  SAT_gcFrac = args.getDoubleOption("-sat-gc-frac", 0.20);
  SAT_ccMinMode = args.getIntOption("-sat-cc-min-mode", 2);
  SAT_phaseSaving = args.getIntOption("-sat-phase-saving", 2);
  SAT_rndInitActivity = args.getBoolOption("-sat-rnd-init-act", false);
  SAT_lubyRestart = args.getBoolOption("-sat-luby", true);
  SAT_restartInc = args.getDoubleOption("-sat-restart-inc", 2);
  SAT_learntSizeFactor = args.getDoubleOption("-sat-learntsize-factor", 1.0/3.0);
  SAT_learntSizeInc = args.getDoubleOption("-sat-learntsize-inc", 1.1);
  SAT_restartFirst = args.getIntOption("-sat-restart-first", 100);

  use_coprocessor = args.getBoolOption("-preprocess", true);
  pre_lcnf         = args.getBoolOption("-pre-lcnf", true);
  pre_techniques   = args.getStringOption("-pre-techniques", "[vpusb]+");
  pre_group        = args.getBoolOption("-pre-group", true);
  pre_group_only   = args.getBoolOption("-pre-group-only", false);

  isLCNF = inFileAssumptions || (use_coprocessor && pre_lcnf);

  streamPrecision = args.getIntOption("-stream-precision", 15);
  floatWeights = args.getBoolOption("-float-weights", false);

  // zero = no limit
  minimizePropLimit = args.getIntOption("-min-prop-lim", 0);
  minimizeConfLimit = args.getIntOption("-min-conf-lim", 0);

  initialized = true;


#ifdef E2015I
  // enable preprocessing
  use_coprocessor = true;
  isLCNF  = true;
  floatWeights = false;
  // force nonopt to "disjoint"
  nonoptPrimary = NonoptHS::disjoint;
  nonoptSecondary = nullptr;
#endif

#ifdef E2015C
  // enable preprocessing
  use_coprocessor = true;
  isLCNF  = true;
  floatWeights = false;
  // force nonopt to "common+greedy"
  nonoptPrimary = NonoptHS::common;
  nonoptSecondary = NonoptHS::greedy;
#endif
}
