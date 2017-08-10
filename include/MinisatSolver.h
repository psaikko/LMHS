#pragma once

#include <fpu_control.h>
#include <errno.h>
#include <unordered_set>
#include <algorithm>
#include <utility>
#include <vector>
#include <unordered_map>
#include <cstdio>
#include <cstdlib>
#include <functional>

#include "Util.h"
#include "GlobalConfig.h"
#include "Weights.h"
#include "Timer.h"
#include "minisat/core/Solver.h"

template <typename T>
inline std::ostream& operator<<(std::ostream& o, const Minisat::vec<T> & v) {
  if (v.size() == 0) return o;
  o << v[0];
  for (int i = 1; i < v.size(); ++i)
    o << " " << v[i];
  return o;
}

inline Minisat::Lit int2lit(const int l) {
  assert(l != 0);
  return Minisat::mkLit(abs(l), l < 0);
}

inline int lit2int(const Minisat::Lit l) {
  return Minisat::var(l) * (1 - 2*Minisat::sign(l));
}

inline std::ostream& operator<<(std::ostream& o, const Minisat::Lit l) {
  return (o << lit2int(l));
}

class MinisatSolver {

  Minisat::lbool upol;
  Minisat::Solver* minisat;
  static const int deactivateMask = ~0 << 1;
  Minisat::vec<Minisat::Lit> assumptions;
  Minisat::vec<Minisat::Lit> bvar_assumptions;
  std::unordered_map<int, int> var_assumptionIdx;
  clock_t initTime, start, end;
  GlobalConfig &cfg;

  MinisatSolver(const MinisatSolver&);
  void operator=(MinisatSolver const&);

  Minisat::lbool solveLimited();
  void getConflicts(std::vector<std::vector<int>>& out_cores);

 public:

  unsigned solver_calls;
  unsigned sat_calls;
  unsigned unsat_calls;

  Timer solver_timer;
  Timer sat_timer;
  Timer unsat_timer;

  bool hasModel;

  MinisatSolver();
  ~MinisatSolver() { delete minisat; }

  void addVariable(int var);

  void findCore(std::vector<int>& out_core);
  void findCores(std::vector<std::vector<int>>& out_cores);
  bool findCoreLimited(std::vector<int>& out_core);

  bool _solve();

  int newVar();

  bool addConstraint(std::vector<int>& constr);

  void getModel(std::vector<bool>& model);
  weight_t getModelWeight(std::unordered_map<int, weight_t>& bvar_weights);

  void deleteLearnts() { minisat->deleteLearnts(); }
  void invertActivity() { minisat->invertVarActivity(); }
  void randomizeActivity() { minisat->randomizeVarActivity(); }
  void setRandomSeed(double seed) { minisat->random_seed = seed; }

  void printStats(const char* solver_name);

  void addCardConstraint(int k, std::vector<int>& card);

  int addTempCardPropagator(int k, std::vector<int>& card);

  void removeTempCardPropagator(int b);

  std::vector<int> addTempAtMostOneEncoding(std::vector<int>& core);

  void removeTempAtMostOneEncoding(int b);  

  std::vector<std::vector<int>> & getAcycLearnts();
  void clearAcycLearnts();

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
  void unsetBvars() {
    for (int i = 0; i < bvar_assumptions.size(); i++) bvar_assumptions[i].x |= 1;
  }

  // Deactivate every clause by setting all b-variables to true
  // (set least significant bit to 0)
  void setBvars() {
    for (int i = 0; i < bvar_assumptions.size(); i++)
      bvar_assumptions[i].x &= deactivateMask;
  }

  // activate a clause by its bvar
  void unsetBvar(int bLit) {
    assert(bLit > 0);
    assert(var_assumptionIdx.count(bLit));
    bvar_assumptions[var_assumptionIdx[bLit]].x |= 1;
  }

  // deactivate a clause by its bvar
  void setBvar(int bLit) {
    assert(bLit > 0);
    assert(var_assumptionIdx.count(bLit));
    bvar_assumptions[var_assumptionIdx[bLit]].x &= deactivateMask;
  }

  void clearAssumptions() {
    assumptions.clear();
  }

  void assumeLit(int l) {
    assumptions.push(int2lit(l));
  }

  void assumeBvars() {
    for (auto l : bvar_assumptions)
      assumptions.push(l);
  }

  void addBvarAssumption(int bvar) {
    var_assumptionIdx[bvar] = bvar_assumptions.size();
    bvar_assumptions.push(Minisat::mkLit(bvar));
  }

  void removeBvarAssumption(int bVar) {
    int tailIdx = bvar_assumptions.size() - 1;
    var_assumptionIdx[var(bvar_assumptions[tailIdx])] = var_assumptionIdx[bVar];
    bvar_assumptions[var_assumptionIdx[bVar]] = bvar_assumptions[tailIdx];
    var_assumptionIdx.erase(bVar);
    bvar_assumptions.pop();
  }

  int nVars() { return minisat->nVars(); }

  bool solve() {
    hasModel = _solve();
    return hasModel;
  }

  void setFPU() {
    fpu_control_t oldcw, newcw;
    _FPU_GETCW(oldcw);
    newcw = (oldcw & ~_FPU_EXTENDED) | _FPU_DOUBLE;
    _FPU_SETCW(newcw);
    log(2, "c WARNING: setting FPU to use double precision\n");
  }

};
