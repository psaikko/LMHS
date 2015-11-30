#include "IMIPSolver.h"
#include <ilcplex/ilocplex.h>

class CPLEXSolver : public IMIPSolver {

 public:
  CPLEXSolver();
  ~CPLEXSolver();
  void addVariable(int var);
  void addObjectiveVariable(int bVar, double weight);
  void addObjectiveVariables(std::unordered_map<int, double> & bvar_weights);
  void reset();
  void forbidCurrentSolution();
  void addConstraint(std::vector<int>& core);

  bool solveForModel(std::vector<int>& model, double& weight);
  bool solveForHS(std::vector<int>& hittingSet, double& weight);

 private:

  IloNumVar newObjVar(int v, double w);

  bool solutionExists;
  bool objFuncAttached;

  IloEnv* env;
  IloModel model;
  IloNumVarArray objVars;
  IloNumVarArray vars;
  unsigned nObjVars;
  unsigned nVars;
  IloObjective objective;
  IloRangeArray cons;
  IloCplex cplex;
  std::unordered_map<int, IloNumVar> var_to_IloVar;
};