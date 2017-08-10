#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <iosfwd>

#include "MinisatSolver.h"
#include "CPLEXSolver.h"
#include "GlobalConfig.h"
#include "Util.h"
#include "Weights.h"
#include "preprocessorinterface.hpp"
#include "Timer.h"
#include "Defines.h"

class ProblemInstance {
 public:
  ProblemInstance(std::ostream& out);
  ProblemInstance(unsigned nClauses, weight_t top, weight_t* weights,
                  int* raw_clauses, std::ostream& out);
  ProblemInstance(std::istream & wcnf_in, std::ostream& out);
  ~ProblemInstance();

  std::vector<int> reconstruct(std::vector<int> & model);

  Timer parse_timer;
  Timer solve_timer;
  Timer preprocess_timer;
  Timer reduce_timer;

  unsigned fixed_variables;

  MinisatSolver* sat_solver;
  MinisatSolver* muser;
  CPLEXSolver* mip_solver;

  std::vector<int> fixQueue;
  std::vector<int> relaxQueue;

  std::unordered_map<int, bool> flippedInternalVarPolarity;
  std::unordered_map<int, weight_t> bvar_weights;

  std::vector<std::vector<int>*> clauses;
  std::vector<std::vector<int>*> hard_clauses;
  std::vector<std::vector<int>*> soft_clauses;
  // map bvariable to soft clause
  std::unordered_map<unsigned, std::vector<std::vector<int>*> > bvar_soft_clauses;
  std::unordered_map<unsigned, int> bvar_clause_ct;
  std::unordered_map<unsigned, bool> isOriginalVariable;

  void forbidCurrentMIPSol();
  void forbidCurrentModel();

  void addHardClause(std::vector<int>& hc, bool original = true);
  int addSoftClause(std::vector<int>& sc, weight_t weight, bool original = true);
  void addSoftClauseWithBv(std::vector<int>& hc,
                                     bool original = true);

  void forceBvar(int bv, bool pol);

  void getBvarEquivConstraints(std::vector<std::vector<int> >& out_constraints);
  void getLabelOnlyClauses(std::vector<std::vector<int> > & label_clauses);

  void getSolution(std::vector<int>& out_solution);
  void getSolution(std::vector<int>& out_solution, MinisatSolver * solver);
  weight_t tightenModel(std::vector<bool>& model);
  weight_t getSolutionWeight(MinisatSolver * solver);
  bool bVarSatisfied(std::vector<bool> & model, int b);

  weight_t totalWeight();

  void printStats();

  void attach(MinisatSolver * solver);
  void attachMuser(MinisatSolver * solver);
  void attach(CPLEXSolver * solver);

  void toLCNF(std::ostream& out);

  int max_var; 
  std::string filename;
  bool isUNSAT;

  void updateBvarMap(int bvar, std::vector<int>* sc);
  void addBvar(int var, weight_t weight);

  void reduceCore(std::vector<int>& core, MinimizeAlgorithm alg);

  void updateUB(weight_t);
  void updateUB(weight_t, MinisatSolver * solver);
  void updateLB(weight_t);

  void printSolution(std::ostream & model_out);

  weight_t LB;
  weight_t UB;
  std::vector<int> UB_solution;
  std::vector<bool> UB_bool_solution;

 private:
  ProblemInstance(const ProblemInstance&);
  void operator=(ProblemInstance const&);

  GlobalConfig &cfg;

  void reRefuteCore(MinisatSolver * solver, std::vector<int>& core);
  void destructiveMinimize(MinisatSolver * solver, std::vector<int>& core);
  void constructiveMinimize(MinisatSolver * solver, std::vector<int>& core);
  void binarySearchMinimize(MinisatSolver * solver, std::vector<int>& core);
  void cardinalityMinimize(MinisatSolver * solver, std::vector<int>& core);

  std::vector<int> branchVars;

  maxPreprocessor::PreprocessorInterface * preprocessor;
  std::ostream & out;
};