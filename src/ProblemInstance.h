#ifndef ProblemInstance_h
#define ProblemInstance_h

#include "ISATSolver.h"
#include "IMIPSolver.h"
#include "GlobalConfig.h"
#include "Util.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <iosfwd>

class ProblemInstance {
 public:
  ProblemInstance();
  ProblemInstance(unsigned nClauses, double top, double* weights,
                  int* raw_clauses);
  ProblemInstance(std::istream & wcnf_in);
  ~ProblemInstance();

  std::unordered_map<int, bool> flippedInternalVarPolarity;
  std::unordered_map<int, double> bvar_weights;

  std::vector<std::vector<int>*> clauses;
  std::vector<std::vector<int>*> hard_clauses;
  std::vector<std::vector<int>*> soft_clauses;
  // map bvariable to soft clause
  std::unordered_map<int, std::vector<std::vector<int>*> > bvar_soft_clauses;
  std::unordered_map<int, int> bvar_clause_ct;
  std::unordered_map<int, bool> isOriginalVariable;

  void forbidCurrentMIPSol();
  void forbidCurrentModel();

  void addHardClause(std::vector<int>& hc, bool original = true);
  int addSoftClause(std::vector<int>& sc, double weight, bool original = true);
  void addSoftClauseWithExistingBvar(std::vector<int>& hc,
                                     bool original = true);

  void getBvarEquivConstraints(std::vector<std::vector<int> >& out_constraints);
  void getLabelOnlyClauses(std::vector<std::vector<int> > & label_clauses);

  double applyMaxRes(std::vector<int>& core,
                   std::unordered_map<int, int>& bvar_replace);

  void getMUS(std::vector<int>& hs, std::vector<int>& out_mus);

  void getSolution(std::vector<int>& out_solution);
  double getSolutionWeight();

  double totalWeight();

  void printStats();

  void attach(ISATSolver * solver);
  void attach(IMIPSolver * solver);

  int max_var; 
  clock_t parseTime;
  std::string filename;

 private:
  ProblemInstance(const ProblemInstance&);
  void operator=(ProblemInstance const&);

  void init();

  void updateBvarMap(int bvar, std::vector<int>* sc);

  GlobalConfig &cfg;

  ISATSolver* sat_solver;
  IMIPSolver* mip_solver;

  std::vector<int> branchVars;
};

#endif