#ifndef minisat_solver_h
#define minisat_solver_h

#include "ISATSolver.h"
#include "minisat/core/Solver.h"

class MinisatSolver : public ISATSolver {

 private:
  Minisat::lbool upol;
  Minisat::Solver* minisat;
  static const int deactivateMask = ~0 << 1;
  Minisat::vec<Minisat::Lit> assumptions;
  std::unordered_map<int, int> var_assumptionIdx;

  Minisat::lbool solveLimited();
  void getConflict(std::vector<int>& out_core);

 public:
  MinisatSolver();
  ~MinisatSolver() { delete minisat; }

  void addVariable(int var);

  void findCore(std::vector<int>& out_core);
  bool findCoreLimited(std::vector<int>& out_core);

  bool solve();

  int newVar();

  void addConstraint(std::vector<int>& constr);

  void getModel(std::vector<bool>& model);
  double getModelWeight(std::unordered_map<int, double>& bvar_weights);

  void deleteLearnts() { minisat->deleteLearnts(); }
  void invertActivity() { minisat->invertVarActivity(); }
  void setRandomSeed(double seed) { minisat->random_seed = seed; }

  void printStats();

  void setVarPolarity(int var, bool polarity) {
    minisat->setPolarity(var, !polarity ? Minisat::l_True : Minisat::l_False);
  }
  void setVarDecision(int var, bool decision) {
    minisat->setDecisionVar(var, decision);
  }

  void setBudgets(long props, long confs) {
    minisat->budgetOff();
    if (confs) minisat->setConfBudget(confs);
    if (props) minisat->setPropBudget(props);
  }

  // Activate every clause by setting all b-variables to false
  // (set least significant bit to 1)
  void activateClauses() {
    for (int i = 0; i < assumptions.size(); i++) assumptions[i].x |= 1;
  }

  // Deactivate every clause by setting all b-variables to true
  // (set least significant bit to 0)
  void deactivateClauses() {
    for (int i = 0; i < assumptions.size(); i++)
      assumptions[i].x &= deactivateMask;
  }

  // activate a clause by its bvar
  void activateClause(int bLit) {
    assert(bLit > 0);
    assumptions[var_assumptionIdx[bLit]].x |= 1;
  }

  // deactivate a clause by its bvar
  void deactivateClause(int bLit) {
    assert(bLit > 0);
    assumptions[var_assumptionIdx[bLit]].x &= deactivateMask;
  }

  void addBvarAssumption(int bvar) {
    var_assumptionIdx[bvar] = assumptions.size();
    assumptions.push(Minisat::mkLit(bvar));
  }

  void removeBvarAssumption(int bVar) {
    int tailIdx = assumptions.size() - 1;
    var_assumptionIdx[var(assumptions[tailIdx])] = var_assumptionIdx[bVar];
    assumptions[var_assumptionIdx[bVar]] = assumptions[tailIdx];
    assumptions.pop();
  }

  int nVars() { return minisat->nVars(); }
};

#endif