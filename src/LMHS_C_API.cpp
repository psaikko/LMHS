#include <vector>
#include <fstream>
#include <istream>
#include <ostream>
#include <sstream>
#include <iomanip>

extern "C" {
#include "LMHS_C_API.h"
}

#include "GlobalConfig.h"
#include "Solver.h"
#include "ProblemInstance.h"
#include "VarMapper.h"
#include "Util.h"
#include "Timer.h"

using namespace std;

Solver * solver;
MaxsatSol currentMaxsatSol;
ProblemInstance * instance;
VarMapper vm;
ofstream nullstream("/dev/null");
bool preprocessed;

void LMHS_reset() { 
  delete solver;
  delete instance;
}

void LMHS_setArgs(int argc, const char* argv[]) {
  GlobalConfig::get().parseArgs(argc, argv, nullstream);
}

int LMHS_initializeWithFile(const char* filepath) {
  ifstream file(filepath);
  instance = new ProblemInstance(file, nullstream);
  file.close();
  solver = new Solver(*instance, nullstream);
  return 1;
}

int LMHS_initializeWithoutData() {
  instance = new ProblemInstance(nullstream);
  solver = new Solver(*instance, nullstream);
  return 1;
}

int LMHS_initializeWithRawData(int n, weight_t top, weight_t* weights, int* clauses) {
  instance = new ProblemInstance(n, top, weights, clauses, nullstream);
  solver = new Solver(*instance, nullstream);
  return 1;
}

int LMHS_getNewVariable() {
  return instance->sat_solver->newVar();
}

void LMHS_addHardClause(int n, int *cl) {
  vector<int> clause(n);
  for (int i = 0; i < n; ++i) clause[i] = cl[i];
  solver->instance.addHardClause(clause);
}

int LMHS_addSoftClause(weight_t weight, int n, int *cl) {
  vector<int> clause(n);
  for (int i = 0; i < n; ++i) clause[i] = cl[i];
  int bvar = solver->instance.addSoftClause(clause, weight);
  return bvar;
}

void LMHS_addSoftClauseWithBv(int n, int *cl) {
  vector<int> clause(n);
  for (int i = 0; i < n; ++i) clause[i] = cl[i];
  solver->instance.addSoftClauseWithBv(clause);
}

void LMHS_addCoreConstraint(int n, int *core) {
  vector<int> coreVec(n);
  for (int i = 0; i < n; ++i)
    coreVec[i] = core[i];
  solver->processCore(coreVec);
}

void _processSolution(weight_t solution_weight, vector<int> & solution_vec) {

  free(currentMaxsatSol.values); // shouldn't be slower than realloc
  int* sol_ptr = (int*)malloc(solution_vec.size() * sizeof(int));
  if (!sol_ptr && solution_vec.size() != 0) {
    // alloc failed. this is bad.
    currentMaxsatSol.size = 0;
    currentMaxsatSol.values = NULL;
    currentMaxsatSol.weight = -1;
    currentMaxsatSol.time = -1;
  } else {
    for (unsigned i = 0; i < solution_vec.size(); ++i) {
      sol_ptr[i] = solution_vec[i];
    }
    
    currentMaxsatSol.size = solution_vec.size();
    currentMaxsatSol.values = sol_ptr;
    currentMaxsatSol.weight = solution_weight;
    currentMaxsatSol.time = instance->solve_timer.cpu_ms_total() / 1000;
  }
}

MaxsatSol * LMHS_getSolution() {

  vector<int> solution_vec;
  weight_t solution_weight = 0;

  if (solver->hardClausesSatisfiable()) {
    solver->solveMaxHS();

    solution_vec = instance->UB_solution;
    solution_weight = instance->UB;
  }
  _processSolution(solution_weight, solution_vec);
  return &currentMaxsatSol;
}

void LMHS_forbidLastHS() {
  solver->instance.forbidCurrentMIPSol();
}

void LMHS_forbidLastModel() {
  solver->instance.forbidCurrentModel();
}

void LMHS_printStats() { solver->printStats(); }

void LMHS_declareBvar(int var, weight_t weight, bool pol) {
  solver->instance.addBvar(var, weight);
  if (!pol)
    solver->instance.flippedInternalVarPolarity[abs(var)] = true;
}