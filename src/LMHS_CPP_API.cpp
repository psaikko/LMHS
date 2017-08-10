#include <vector>
#include <istream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

#include "LMHS_CPP_API.h"
#include "GlobalConfig.h"
#include "Solver.h"
#include "ProblemInstance.h"
#include "VarMapper.h"
#include "Util.h"
#include "Timer.h"

using namespace std;

namespace LMHS {

Solver * solver;
ProblemInstance * instance;
ofstream nullstream("/dev/null");
VarMapper vm;
bool preprocessed;

void reset() { 
  delete solver;
  delete instance;
}

void setArgs(int argc, const char* argv[]) {
  GlobalConfig::get().parseArgs(argc, argv, nullstream);
}

bool initialize(istream & wcnf_in) {
  instance = new ProblemInstance(wcnf_in, nullstream);
  solver = new Solver(*instance, nullstream);
  return true;
}

bool initialize() {
  instance = new ProblemInstance(nullstream);
  solver = new Solver(*instance, nullstream);
  return true;
}

bool initialize(weight_t top, vector<weight_t>& weights, vector<vector<int>>& clauses) {
  instance = new ProblemInstance(nullstream);

  int max_orig_var = 0;
  for (auto cl : clauses)
    for (auto l : cl)
      max_orig_var = max(max_orig_var, abs(l));
  instance->max_var = max_orig_var;

  for (unsigned i = 0; i < clauses.size(); ++i) {
    if (weights[i] == top) {
      instance->addHardClause(clauses[i]);
    } else {
      instance->addSoftClause(clauses[i], weights[i]);
    }
  }
  solver = new Solver(*instance, nullstream);
  return true;
}

int getNewVariable() {
  return instance->sat_solver->newVar();
}

void addHardClause(vector<int>& lits) {
  solver->instance.addHardClause(lits);
}

int addSoftClause(weight_t weight, vector<int>& lits) {
  int bvar = solver->instance.addSoftClause(lits, weight);
  return bvar;
}

void addSoftClauseWithBv(vector<int>& lits) {
  solver->instance.addSoftClauseWithBv(lits);
}

void addCoreConstraint(vector<int>& core) {
  solver->processCore(core);
}

bool getSolution(weight_t & out_weight, vector<int> & out_solution) {
  out_weight = -1;
  out_solution.clear();

  if (solver->hardClausesSatisfiable()) {
    solver->solveMaxHS();

    out_solution = instance->UB_solution;
    out_weight = instance->UB;  
  }
  return out_solution.size() != 0;
}

void forbidLastHS() {
  solver->instance.forbidCurrentMIPSol();
}

void forbidLastModel() {
  solver->instance.forbidCurrentModel();
}

void printStats() { solver->printStats(); }

void declareBvar(int var, weight_t weight, bool pol) {
  solver->instance.addBvar(var, weight);
  if (!pol)
    solver->instance.flippedInternalVarPolarity[abs(var)] = true;
}

}