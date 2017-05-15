#ifndef lingeling_solver_h
#define lingeling_solver_h

#include "ISATSolver.h"

extern "C" 
{
#include "lglib.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
}

class LingelingSolver : public ISATSolver {

 private:

  LGL * lgl;

  std::vector<int> assumptions;
  std::unordered_map<int, int> var_assumptionIdx;

  void getConflict(std::vector<int>& conflict);

 public:
  LingelingSolver();
  ~LingelingSolver();

  int newVar();
  void addVariable(int var);
  void addConstraint(std::vector<int>& constr);

  void findCore(std::vector<int>& out_core);
  
  bool solve();
  void getModel(std::vector<bool>& model);
  double getModelWeight(std::unordered_map<int, double>& bvar_weights);

  // TODO: implement optional methods
  bool findCoreLimited(std::vector<int>& out_core);
  void deleteLearnts();
  void invertActivity();
  void setVarPolarity(int var, bool polarity);
  void setVarDecision(int /*var*/, bool /*decision*/) {}
  void setBudgets(long /*props*/, long /*confs*/) {}
  void randomizeActivity();

  // Activate every clause by setting all b-variables to false
  // (set least significant bit to 1)
  void activateClauses();

  // Deactivate every clause by setting all b-variables to true
  // (set least significant bit to 0)
  void deactivateClauses();

  // activate a clause by its bvar
  void activateClause(int bvar);

  // deactivate a clause by its bvar
  void deactivateClause(int bvar);

  void addBvarAssumption(int bvar);

  void removeBvarAssumption(int bVar);

  int nVars();
};

#endif