#include "IMIPSolver.h"
#include <scip/scip.h>
#include <scip/scipdefplugins.h>

class SCIPSolver : public IMIPSolver {

 public:
  SCIPSolver();
  ~SCIPSolver();

  void addVariable(int var);
  void addObjectiveVariable(int bVar, double weight);
  void addObjectiveVariables(std::unordered_map<int, double> & bvar_weights);
  void reset();
  void forbidCurrentSolution();
  void addConstraint(std::vector<int>& core);

  bool solveForModel(std::vector<int>& model, double& weight);
  bool solveForHS(std::vector<int>& hittingSet, double& weight);

 private:
  SCIP* scip;
  std::vector<SCIP_VAR*> scip_variables;
  std::unordered_map<int, SCIP_VAR*> bvar_scip_var;
  std::vector<SCIP_CONS*> scip_constraints;
  SCIP_SOL* lastSol;

  SCIP_RETCODE _reset();
  SCIP_RETCODE _addConstraint(std::vector<int>& core);
  SCIP_RETCODE _findSolution(std::vector<int>& solution, double& weight);
  SCIP_RETCODE _forbidCurrentSolution();
};