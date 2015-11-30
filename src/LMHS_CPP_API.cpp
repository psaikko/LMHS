#include "LMHS_CPP_API.h"
#include "GlobalConfig.h"
#include "Solver.h"
#include "ProblemInstance.h"
#include <vector>
#include <istream>
#include <fstream>

using namespace std;

namespace LMHS {

Solver * solver;
ProblemInstance * instance;

void reset() { 
  delete solver;
  delete instance;
}

void setArgs(int argc, const char* argv[]) {
  ofstream nullstream("/dev/null"); // dont't display helptext through api
  GlobalConfig::get().parseArgs(argc, argv, nullstream);
}

bool initialize(istream & wcnf_in) {
  instance = new ProblemInstance(wcnf_in);
  solver = new Solver(*instance);
  return true;
}

bool initialize() {
  instance = new ProblemInstance();
  solver = new Solver(*instance);
  return true;
}

bool initialize(double top, vector<double>& weights, vector<vector<int>>& clauses) {
  instance = new ProblemInstance();

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
  solver = new Solver(*instance);
  return true;
}

int getNewVariable() {
  return solver->sat_solver->newVar();
}

void addHardClause(vector<int>& lits) {
  solver->instance.addHardClause(lits);
}

int addSoftClause(double weight, vector<int>& lits) {
  int bvar = solver->instance.addSoftClause(lits, weight);
  return bvar;
}

void addCoreConstraint(vector<int>& core) {
  solver->processCore(core);
}

bool getOptimalSolution(double & out_weight, vector<int> & out_solution) {
  out_weight = -1;
  out_solution.clear();
  if (solver->hardClausesSatisfiable()) {
    out_weight = solver->solveMaxHS(out_solution);
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

}