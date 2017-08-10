#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include "Util.h"
#include "Weights.h"
#include "Timer.h"
#include "GlobalConfig.h"

#include <ilcplex/ilocplex.h>

class ProblemInstance;

class CPLEXSolver {

 public:

  enum Status { Failed, Feasible, Optimal };
  enum Comparator { LTE, GTE };

  CPLEXSolver();
  ~CPLEXSolver();
  void addVariable(int var);
  void addObjectiveVariable(int bVar, weight_t weight);
  void addObjectiveVariables(std::unordered_map<int, weight_t> & bvar_weights);
  void reset();
  void forbidCurrentSolution();
  void addConstraint(std::vector<int>& core, double bound=1.0, Comparator comp=GTE);

  bool LPsolveHS(std::vector<int>& hittingSet, weight_t& weight);
  bool solveForModel(std::vector<int>& model, weight_t& weight);
  Status solveForHS(std::vector<int>& hittingSet, weight_t& weight, ProblemInstance* instance);

  void exportModel(std::string file);
  void forceVar(ProblemInstance * instance, int var, bool val);
  bool getLPConditionalLB(int var, weight_t & out_lb, IloNumArray& out_rc);

  Timer solver_timer;
  Timer lp_timer;
  unsigned solver_calls;
  unsigned lp_calls;

  void printStats() {
    log(1, "c CPLEX:\n");
    log(1, "c   solver calls: %d\n", solver_calls);
    if (has_lp_model)
      log(1, "c   LP calls: %d\n", lp_calls);
    log(1, "c   IP solver time:  %lu ms\n", solver_timer.cpu_ms_total());
    if (has_lp_model)
      log(1, "c   LP solver time:  %lu ms\n", lp_timer.cpu_ms_total());
    if (GlobalConfig::get().CPLEX_reducedCosts) {
      log(1, "c reducedcost forced %u\n", reducedCostForcedVars);
      log(1, "c reducedcost relaxed %u\n", reducedCostRelaxedVars);
    }
  }

 private:
  CPLEXSolver(const CPLEXSolver&);
  void operator=(CPLEXSolver const&);

  IloNumVar newObjVar(int v, weight_t w);

  bool solutionExists;
  bool objFuncAttached;

  IloEnv* env;
  IloModel model;
  IloModel lp_model;
  IloNumVarArray objVars;
  IloNumVarArray vars;

  bool has_lp_model;
  int verbosity;
  unsigned largestCore;
  unsigned nObjVars;
  unsigned nVars;

  unsigned reducedCostForcedVars;
  unsigned reducedCostRelaxedVars;
  unsigned lookaheadForcedVars;
  unsigned lookaheadImplications;

  IloObjective objective;
  IloRangeArray cons;
  IloCplex cplex;
  IloCplex lp_cplex;
  std::unordered_map<int, weight_t> var_to_weight;
  std::unordered_map<int, IloNumVar> var_to_IloVar;
};