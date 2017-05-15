#include "GlobalConfig.h"
#include "ArgsParser.h"
#include "Util.h"

#include <string>
#include <ostream>
#include <climits>

using namespace std;

void GlobalConfig::printHelp(ostream & out) {
  vector<vector<string> > pds;
  // TODO: do something about this mess
  
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
  pds.push_back({"--nonopt", "disjoint", "string", "{disjoint, common, frac, greedy}", 
    "Strategy to use for minimum cost hitting set approximations\n"
    "common: add the variable with the most occurrences in cores so far\n"
    "frac: like common but add some fraction of the most occurring\n"
    "      variables to the hitting set\n"
    "disjoint: add entire core to hitting set. Finds disjoint set of cores between\n"
    "          optimal hitting sets. Equivalent to --nonopt frac --frac-size 1.0\n"
    "greedy: use a greedy MCHS algorithm\n"
    "[strategy]+greedy: use greedy algorithm as fallback for another strategy.\n"});

  pds.push_back({"--frac-size", "0.1", "double", "(0 .. 1]", 
    "When using \"--nonopt frac\", the fraction of the core to add to the nonopt HS"});

  pds.push_back({"--ip", "false", "bool", "", 
    "Solve instance using IP solver with standard MaxSAT to IP transformation"});

  pds.push_back({"--preprocess", "false", "bool", "", 
    "Enable SAT-based preprocessing"});
  pds.push_back({"--pre-only", "false", "bool", "", 
    "Only output preprocessed formula"});
  pds.push_back({"--pre-lcnf", "true", "bool", "", 
    "Treat preprocessed instance as LCNF."
    "Reuses auxiliary variables from preprocessing in solver."});
  pds.push_back({"--pre-group", "true", "bool", "", 
    "Use group detection in preprocessor."});
  pds.push_back({"--pre-group-only", "false", "bool", "", 
    "Only use group detection."});
  pds.push_back({"--pre-techniques", "[vpusb]", "string", "", 
    "Modified CoProcessor2 technique string.\n"
    "Blocks of techniques in brackets are repeated until fixpoint.\n"
    "Nested blocks are supported."});

  pds.push_back({"--random-seed", "9", "int", "[0 .. imax]",
    "Passed directly to srand to seed random number generation for the maxsat solver"});

  pds.push_back({"--mip-threads", "1", "int", "[0 .. imax]",
    "CPLEX Threads"});
  pds.push_back({"--mip-intensity", "2", "int", "[0 .. 4]",
    "CPLEX SolnPoolIntensity"});
  pds.push_back({"--mip-replace", "2", "int", "[0 .. 2]",
    "CPLEX SolnPoolReplace"});
  pds.push_back({"--mip-capacity", "10", "int", "[0 .. imax]",
    "CPLEX SolnPoolCapacity"});
  pds.push_back({"--mip-poplim", "100", "int", "[0 .. imax]",
    "CPLEX PopulateLim"});
  pds.push_back({"--mip-start", "2", "int", "[0 .. 2]",
    "CPLEX AdvInd"});
  pds.push_back({"--mip-emph", "0", "int", "[0 .. 4]",
    "CPLEX MIPEmphasis"});

  pds.push_back({"--sat-polarity", "0", "int", "[-1 .. 1]", 
    "Set the sat solver variable polarity to control branching\n"
    "(-1=false, 0=undefined, 1=true)"});
  pds.push_back({"--sat-var-decay", "0.95", "double", "(0 .. 1)", 
    "MiniSat: Variable activity decay factor"});
  pds.push_back({"--sat-cla-decay", "0.999", "double", "(0 .. 1)", 
    "MiniSat: Clause activity decay factor"});
  pds.push_back({"--sat-rnd-freq", "0", "double", "[0 .. 1]", 
    "MiniSat: The frequency with which the decision heuristic tries to choose a random variable "});
  pds.push_back({"--sat-rnd-seed", "91648253", "double", "(0 .. inf)",
    "MiniSat: The fraction of wasted memory allowed before a garbage collection is triggered"});
  pds.push_back({"--sat-gc-frac", "0.20", "double", "(0 .. inf)",
    "MiniSat: The fraction of wasted memory allowed before a garbage collection is triggered"});
  pds.push_back({"--sat-cc-min-mode", "2", "int", "[0 .. 2]",
    "MiniSat: Controls conflict clause minimization in (0=none, 1=basic, 2=deep)"});
  pds.push_back({"--sat-phase-saving", "2", "int", "[0 .. 2]",
    "MiniSat: Controls the level of phase saving in (0=none, 1=limited, 2=full)"});
  pds.push_back({"--sat-rnd-init-act", "false", "bool", "",
    "MiniSat: Randomize initial activity"});
  pds.push_back({"--sat-luby", "true", "bool", "",
    "MiniSat: Use the Luby restart sequence"});
  pds.push_back({"--sat-restart-first", "100", "int", "[1 .. imax]",
    "MiniSat: The base restart interval"});
  pds.push_back({"--sat-restart-inc", "2", "double", "(1 .. inf)",
    "MiniSat: Restart interval increase factor"});
  pds.push_back({"--sat-learntsize-factor", "1.0/3.0", "double", "(0 .. 100]", 
    "MiniSat: Limit on number of learnt clauses as a fraction of original clauses"});
  pds.push_back({"--sat-learntsize-inc", "1.1", "double", "(0 .. 100]", 
    "MiniSat: Rate at which learntsize-factor increases between restarts"});

  pds.push_back({"--infile-assumps", "false", "bool", "",
    "LCNF: get assumption variables and polarities from \"c assumptions ...\" line in input"});

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
    string nonOptType = args.getStringOption("-nonopt", "common+greedy");
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

  solveAsMIP = args.getBoolOption("-ip", false);

  srand(args.getIntOption("-random-seed", 9));

  // blocking vars / assumptions specified using wcnf comment line
  inFileAssumptions = args.getBoolOption("-infile-assumps", false);

  streamPrecision = args.getIntOption("-stream-precision", 15);

  // zero = no limit
  minimizePropLimit = args.getIntOption("-min-prop-lim", 0);
  minimizeConfLimit = args.getIntOption("-min-conf-lim", 0);

  use_coprocessor = args.getBoolOption("-preprocess", true);
  pre_only = args.getBoolOption("-pre-only", false);
  pre_techniques   = args.getStringOption("-pre-techniques", "[vpusbg]");
  pre_group        = args.getBoolOption("-pre-group", true);
  pre_group_only   = args.getBoolOption("-pre-group-only", false);

  doLimitNonopt = args.getBoolOption("-limit-nonopt", false);
  if (doLimitNonopt)
    nonoptLimit = args.getIntOption("-limit-nonopt", INT_MAX);

  isLCNF = inFileAssumptions || use_coprocessor;
  doFindBvarClauses = args.getBoolOption("-eq-b-clauses", true);

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

  MIP_exportModel = args.getBoolOption("-mip-export-model", false);
  if (MIP_exportModel) {
    MIP_modelFile = args.getStringOption("-mip-export-model", "");
    if (!(MIP_modelFile == "" || string_endsWith(MIP_modelFile, ".lp") 
                            || string_endsWith(MIP_modelFile, ".sav")
                            || string_endsWith(MIP_modelFile, ".mps"))) {
      terminate(1, "Invalid extension on MIP model filename\n");
    }
  }

  initialized = true;
  floatWeights = false;

  printf("c MaxSAT Evaluation 2016 version\n");
}
