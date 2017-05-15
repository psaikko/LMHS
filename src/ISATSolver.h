#ifndef i_sat_solver_h
#define i_sat_solver_h

#include "Util.h"
#include "GlobalConfig.h"

#include <fpu_control.h>
#include <errno.h>
#include <unordered_set>
#include <algorithm>
#include <utility>
#include <vector>
#include <unordered_map>
#include <cstdio>
#include <cstdlib>

class ISATSolver {

 public:

  unsigned  solver_calls;
  clock_t solver_time,  minimizeTime, rerefuteTime;
  unsigned minimizeCalls, minimizedLits, coresMinimized;
  unsigned rerefuteCalls, rerefutedLits, coresRerefuted;

  ISATSolver() : solver_calls(0),
                 solver_time(0),
                 minimizeTime(0), 
                 rerefuteTime(0), 
                 minimizeCalls(0), 
                 minimizedLits(0), 
                 coresMinimized(0),
                 rerefuteCalls(0), 
                 rerefutedLits(0), 
                 coresRerefuted(0),
                 cfg(GlobalConfig::get()) { }
  virtual ~ISATSolver() {}

  virtual void activateClauses() =0;
  virtual void activateClause(int clause) =0;
  virtual void deactivateClauses() =0;
  virtual void deactivateClause(int clause) =0;

  virtual void addBvarAssumption(int bvar) =0;
  virtual void removeBvarAssumption(int bvar) =0;

  virtual void findCore(std::vector<int>& out_core) =0;
  virtual bool solve() =0; 

  virtual void addConstraint(std::vector<int>& constr) = 0;
  virtual void addVariable(int var) =0;
  
  virtual int newVar() =0;
  virtual int nVars() =0;
  virtual void getModel(std::vector<bool>& model) =0;
  virtual double getModelWeight(std::unordered_map<int, double>& bvar_weights) =0;

  //
  // Not required for basic operation:
  //

  virtual void setVarPolarity(int var, bool polarity) =0;
  virtual void setVarDecision(int var, bool decision) =0;

  virtual void setBudgets(long props, long confs) =0;
  virtual bool findCoreLimited(std::vector<int>& out_core) =0;

  virtual void deleteLearnts() =0;
  virtual void invertActivity() =0;
  virtual void randomizeActivity() =0;

  //
  // Implemented common functionality:
  //

  virtual void printStats() {
    log(1, "c SAT:\n");
    log(1, "c   solver calls:      %u\n", solver_calls);
    log(1, "c   solver time:       %.2fs\n", SECONDS(solver_time));
  }
  double getTime() { return SECONDS(solver_time); }

  void setFPU() {
    fpu_control_t oldcw, newcw;
    _FPU_GETCW(oldcw);
    newcw = (oldcw & ~_FPU_EXTENDED) | _FPU_DOUBLE;
    _FPU_SETCW(newcw);
    log(2, "c WARNING: setting FPU to use double precision\n");
  }

  // Minimize a core using a simple destructive algorithm
  void minimizeCore(std::vector<int>& core) {

    unsigned origSize = core.size();
    ++minimizeCalls;

    // set only soft clauses in core active
    deactivateClauses();
    for (int b : core)
      activateClause(b);

    std::vector<int> subcore;
    std::sort(core.begin(), core.end());
    // for each clause core[i] in the core, check if core \ {core[i]}
    // is unsatisfiable. If so, remove core[i] from the core

    long propBudget = cfg.minimizePropLimit;
    long confBudget = cfg.minimizeConfLimit;
    bool limited = propBudget || confBudget;
    if (limited) setBudgets(propBudget, confBudget);

    for (long i = 0; i < (long)core.size(); i++) {

      int testClause = core[i];
      deactivateClause(testClause);

      if (limited) {
        // stop minimization on exceeding resource budgets
        if (!findCoreLimited(subcore)) break;
      } else {
        findCore(subcore);
      }

      if (subcore.size() > 0) {
        core.swap(subcore);
        std::sort(core.begin(), core.end());
        // core[i] and possibly some of core[i..n] removed
        // core[0..i-1] known to be critical
        --i;
      } else {
        activateClause(testClause);
      }
    }

    int litsRemoved = origSize - core.size();
    if (litsRemoved)
      ++coresMinimized;
    minimizedLits += litsRemoved;
  }

  // Try to reduce a core by re-refuting it
  // Effective for some instances, relatively low cost
  void reRefuteCore(std::vector<int>& core) {

    ++rerefuteCalls;
    unsigned origSize = core.size();
    unsigned prevSize = origSize;

    do {
      // depending on options, reset SAT learnt clauses and invert variable
      // activity between re-refutations
      if (cfg.doResetClauses) deleteLearnts();
      if (cfg.doInvertActivity) invertActivity();

      prevSize = core.size();

      // set SAT assumptions so only core clauses are active
      deactivateClauses();
      for (int b : core) activateClause(b);

      core.clear();
      // Hope for a smaller core
      findCore(core);
    } while (prevSize > core.size());

    if (core.size() != origSize)
      ++coresRerefuted;
    rerefutedLits += origSize - core.size();
  }

protected:
  
  // statistics
  clock_t initTime, start, end;
  GlobalConfig &cfg;

private:
  ISATSolver(const ISATSolver&);
  void operator=(ISATSolver const&);

};

#endif