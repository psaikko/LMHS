extern "C" {
#include "LMHS_C_API.h"
}

#include "GlobalConfig.h"
#include "Solver.h"
#include "ProblemInstance.h"
#include <vector>
#include <fstream>

using namespace std;

Solver * solver;
MaxsatSol currentMaxsatSol;
ProblemInstance * instance;

void LMHS_reset() { 
  delete solver;
  delete instance;
}

void LMHS_setArgs(int argc, const char* argv[]) {
  ofstream nullstream("/dev/null"); // dont't display helptext through api
  GlobalConfig::get().parseArgs(argc, argv, nullstream);
}

int LMHS_initializeWithFile(const char* filepath) {
  ifstream file(filepath);
  instance = new ProblemInstance(file);
  file.close();
  solver = new Solver(*instance);
  return 1;
}

int LMHS_initializeWithoutData() {
  instance = new ProblemInstance();
  solver = new Solver(*instance);
  return 1;
}

int LMHS_initializeWithRawData(int n, double top, double* weights, int* clauses) {
  instance = new ProblemInstance(n, top, weights, clauses);
  solver = new Solver(*instance);
  return 1;
}

int LMHS_getNewVariable() {
  return solver->sat_solver->newVar();
}

void LMHS_addHardClause(int n, int *cl) {
  vector<int> clause(n);
  for (int i = 0; i < n; ++i) clause[i] = cl[i];
  solver->instance.addHardClause(clause);
}

int LMHS_addSoftClause(double weight, int n, int *cl) {
  vector<int> clause(n);
  for (int i = 0; i < n; ++i) clause[i] = cl[i];
  int bvar = solver->instance.addSoftClause(clause, weight);
  return bvar;
}

void LMHS_addCoreConstraint(int n, int *core) {
  vector<int> coreVec(n);
  for (int i = 0; i < n; ++i)
    coreVec[i] = core[i];
  solver->processCore(coreVec);
}

void _processSolution(double solution_weight, vector<int> & solution_vec) {

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
    currentMaxsatSol.time = SECONDS(solver->solveTime);
  }
}

MaxsatSol * LMHS_getOptimalSolution() {
  vector<int> solution_vec;
  double solution_weight = 0;
  if (solver->hardClausesSatisfiable())
    solution_weight = solver->solveMaxHS(solution_vec);
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
