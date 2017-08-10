#pragma once

#include <vector>
#include <unordered_map>
#include <iosfwd>

#include "ProblemInstance.h"
#include "Util.h"
#include "GlobalConfig.h"
#include "MinisatSolver.h"
#include "Weights.h"

class Solver {
 public:
  Solver(ProblemInstance& instance, std::ostream & out);

  void findDisjointCores();
  bool getCore(std::vector<int>& hs, std::vector<int>& core);
  bool getCores(std::vector<int>& hs, std::vector<std::vector<int>>& cores);
  void setHSAssumptions(std::vector<int>& hs);
  void findGreedyHittingSet(std::vector<int>& hs);

  void solve();
  void printStats();
  void printBounds(weight_t lb, weight_t ub);
  weight_t findSolution(std::vector<int>& out_solution);

  void solveAsMIP();
  void solveMaxHS();

  void presolve();
  bool hardClausesSatisfiable();
  void reset();
  void processCore(std::vector<int>& core);

  // variables for timing
  Timer disjoint_timer;
  Timer nonopt_timer;

  std::vector<unsigned> coreSizes;
  std::unordered_map<int, unsigned> coreClauseCounts;

  GlobalConfig &cfg;
  ProblemInstance &instance;
  bool newInstance;

  // level weight partitioning
  int current_level;
  std::vector<std::vector<std::pair<weight_t, int>>> levels;

  // statistics
  int nSolutions;
  unsigned nNonoptCores, nEquivConstraints, nDisjointCores;

  std::vector<std::vector<int> > cores;

  std::ostream & out;
};
