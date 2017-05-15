#ifndef i_mip_solver_h
#define i_mip_solver_h

#include "GlobalConfig.h"
#include "Util.h"

#include <vector>
#include <unordered_map>
#include <string>

class IMIPSolver {

 public:
  IMIPSolver() : solver_time(0), solver_calls(0), verbosity(GlobalConfig::get().MIP_verbosity), solutionExists(false) { }
  virtual ~IMIPSolver() { }

  virtual void addVariable(int v) = 0;
  virtual void addObjectiveVariable(int bVar, double weight) = 0;
  virtual void addObjectiveVariables(std::unordered_map<int, double> & bvar_weights) = 0;
  virtual void reset() = 0;
  virtual void addConstraint(std::vector<int>& core) =0;
  virtual bool solveForHS(std::vector<int>& hittingSet, double& weight) =0;

  // required for: --enumerate
  virtual void forbidCurrentSolution() = 0;
  // required for: --ip
  virtual bool solveForModel(std::vector<int>& model, double& weight) =0;
  // required for: --mip-export-model
  virtual void exportModel(std::string file) =0;
  // required for: --mip-set-up
  virtual void setUpperBound(double ub) =0;
  // required for: --mip-force-heavy
  virtual void forceVar(int var, bool val) =0;

  double getTime() { return SECONDS(solver_time); }

  void printStats() {
    log(1, "c MIP:\n");
    log(1, "c   solver calls: %d\n", solver_calls);
    log(1, "c   solver time:  %.2fs\n", SECONDS(solver_time));
  }

 protected:
  clock_t solver_time;
  int solver_calls;
  int verbosity;
  bool solutionExists;

 private:
  IMIPSolver(const IMIPSolver&);
  void operator=(IMIPSolver const&);
};

#endif
