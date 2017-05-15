#include "CPLEXSolver.h"
#include "GlobalConfig.h"

CPLEXSolver::CPLEXSolver() : solutionExists(false), objFuncAttached(false), nObjVars(0), nVars(0) {
  GlobalConfig & cfg = GlobalConfig::get();

  env = new IloEnv();
  model = IloModel(*env);
  objVars = IloNumVarArray(*env);
  vars    = IloNumVarArray(*env);
  cons = IloRangeArray(*env);
  cplex = IloCplex(model);
  
  verbosity = cfg.MIP_verbosity;
  cplex.setParam(IloCplex::MIPDisplay, cfg.MIP_verbosity);
  cplex.setParam(IloCplex::EpGap, 0.0);
  cplex.setParam(IloCplex::EpAGap, 0.0);

  cplex.setParam(IloCplex::Threads, cfg.MIP_threads);
  cplex.setParam(IloCplex::SolnPoolIntensity, cfg.MIP_intensity);
  cplex.setParam(IloCplex::SolnPoolReplace, cfg.MIP_replace);
  cplex.setParam(IloCplex::SolnPoolCapacity, cfg.MIP_capacity);
  cplex.setParam(IloCplex::PopulateLim, cfg.MIP_poplim);
  cplex.setParam(IloCplex::AdvInd, cfg.MIP_start);
  cplex.setParam(IloCplex::MIPEmphasis, cfg.MIP_emph);

  objective = IloMinimize(*env, IloExpr(*env));
}

CPLEXSolver::~CPLEXSolver() {
  (*env).end();
  delete env;
}

void CPLEXSolver::addVariable(int var) {
  log(2, "c MIP variable %d\n", var);
  char name[11];
  snprintf(name, 11, "%10d", var);
  IloNumVar x(*env, 0, 1, ILOBOOL, name);
  var_to_IloVar[var] = x;
  vars.add(x);
  nVars++;
}

IloNumVar CPLEXSolver::newObjVar(int bVar, double w) {
  log(2, "c MIP obj variable %d %.2f\n", bVar, w);
  char name[11];
  snprintf(name, 11, "%10d", bVar);

  IloNumVar x(*env, 0, 1, ILOBOOL, name);
  var_to_IloVar[bVar] = x;
  objVars.add(x);
  nObjVars++;
  return x;
}

// Create a CPLEX variable for the bvar and append
// it to the objective function with the given weight
void CPLEXSolver::addObjectiveVariable(int bVar, double weight) {
  if (objFuncAttached) {
    model.remove(objective);
    objFuncAttached = false;
  }

  objective.setExpr(objective.getExpr() + weight * newObjVar(bVar, weight));
}

// adds variables to the cplex instance and sets the objective function to
// minimize weight
void CPLEXSolver::addObjectiveVariables(std::unordered_map<int, double> & bvar_weights) {
  if (objFuncAttached) {
    model.remove(objective);
    objFuncAttached = false;
  }

  IloNumExpr objExpr = objective.getExpr();

  double total_w = 0;

  for (auto b_w : bvar_weights) {
    if (b_w.second > EPS)
      objExpr += b_w.second * newObjVar(b_w.first, b_w.second);
    total_w += b_w.second;
  }

  objective.setExpr(objExpr);

  if (total_w >= 10000000000) {
    // turn on extra precision --
    printf("c increase CPLEX abs. tolerances for very high weights\n");
    cplex.setParam(IloCplex::EpInt, 1e-2);
    cplex.setParam(IloCplex::EpAGap, 1e-2);
    cplex.setParam(IloCplex::EpOpt, 1e-2);
    cplex.setParam(IloCplex::EpRHS, 1e-2);
    cplex.setParam(IloCplex::EpLin, 1e-2);
    cplex.setParam(IloCplex::EpMrk, 0.5);
    cplex.setParam(IloCplex::NumericalEmphasis, true);
  }
}

void CPLEXSolver::reset() {
  model.remove(objective);
  objective.end();
  objective = IloMinimize(*env, IloExpr(*env));
  
  model.remove(objVars);
  objVars.endElements();
  objVars.end();
  objVars = IloNumVarArray(*env);
  nObjVars = 0;

  cons.endElements();
  cons.end();
  cons = IloRangeArray(*env);

  var_to_IloVar.clear();
  objFuncAttached = false;
  solutionExists = false;
}

// disallow a found solution hs by adding a constraints to the MIP instance
void CPLEXSolver::forbidCurrentSolution() {
  condTerminate(!solutionExists, 1, 
    "CPLEXSolver::forbidCurrentSolution - no current hitting set exists\n");

  IloExpr sub_expr(*env);  // make sure next solution isn't a subset of this one
  IloExpr super_expr(*env);  // and not a superset

  // get values from cplex
  IloNumArray vals(*env);
  cplex.getValues(vals, objVars);

  double solutionSize = 0;

  // build constraints of clauses in current solution and those not in current
  // solution
  for (unsigned i = 0; i < nObjVars; ++i) {
    if (IloAbs(vals[i] - 1.0) < EPS) {
      super_expr += objVars[i];
      ++solutionSize;
    } else {
      sub_expr += objVars[i];
    }
  }

  // include at least one clause not in this solution
  IloRange sub_con = (sub_expr >= 1.0);
  // leave out at least one clause in this solution
  IloRange super_con = (super_expr <= (solutionSize - 1));

  cons.add(sub_con);
  cons.add(super_con);
  model.add(sub_con);
  model.add(super_con);
}

// adds a constraint to the MIP instance, works with clauses containing negative
// literals
void CPLEXSolver::addConstraint(std::vector<int>& core) {
  condTerminate(core.empty(), 1, 
    "CPLEXSolver::addConstraint - empty constraint\n");

  log(2, "c adding MIP constraint:\n");
  logCore(2, core);

  unsigned negs = 0;
  IloExpr expr(*env);
  for (int c : core) {
    IloNumVar x = var_to_IloVar[abs(c)];
    if (c > 0) {
      expr += x;
    } else {
      expr += -x;
      ++negs;
    }
  }
  IloRange con = (expr >= (1.0 - negs));
  cons.add(con);
  model.add(con);
}

bool CPLEXSolver::solveForModel(std::vector<int>& model, double& weight) {
  // reuse solve method but ignore hitting set solution
  std::vector<int> dummy;
  model.clear();
  if (!solveForHS(dummy, weight)) return false;

  // get variable values from cplex
  IloNumArray vals(*env);
  cplex.getValues(vals, vars);

  for (unsigned i = 0; i < nVars; ++i) {
    bool pos = IloAbs(vals[i] - 1.0) < EPS;
    // parse bvars from cplex variable names
    int var;
    sscanf(vars[i].getName(), "%100d", &var);
    model.push_back(pos ? var : -var);
  }
  return true;
}

// returns positive valued objective function variables in 'solution'
// and value of objective function in 'weight'
bool CPLEXSolver::solveForHS(std::vector<int>& hittingSet, double& weight) {
  if (!objFuncAttached) {
    model.add(objective);
    objFuncAttached = true;
  }

  log(2, "c CPLEX: solving MIP problem\n");

  ++solver_calls;
  clock_t start = clock();
  hittingSet.clear();

  if (!cplex.solve()) return false;

  clock_t end = clock();
  solver_time += end - start;

  log(2, "c CPLEX: Solution status = %d\n", cplex.getStatus());
  log(2, "c CPLEX: Solution value = %f\n", cplex.getObjValue());

  // get objective variable values from cplex
  IloNumArray vals(*env);
  cplex.getValues(vals, objVars);

  // gather set of variables with true value
  for (unsigned i = 0; i < nObjVars; ++i) {
    if (IloAbs(vals[i] - 1.0) < EPS) {
      // parse bvars from cplex variable names
      int bVar;
      sscanf(objVars[i].getName(), "%100d", &bVar);
      hittingSet.push_back(bVar);
    }
  }

  log(2, "c CPLEX: hitting set:\n");
  logCore(2, hittingSet);

  weight = cplex.getObjValue();
  solutionExists = true;
  return true;
}

void CPLEXSolver::exportModel(std::string file) {
  cplex.exportModel(file.c_str());
}

void CPLEXSolver::setUpperBound(double ub) {
  cplex.setParam(IloCplex::CutUp, ub);
}

void CPLEXSolver::forceVar(int var, bool val) {
  if (var_to_IloVar.count(var)) {
    if (val == true) {
      var_to_IloVar[var].setBounds(1,1);
    } else {
      std::cout << var_to_IloVar[var].getLB() << " " << var_to_IloVar[var].getUB() << std::endl;
      var_to_IloVar[var].setBounds(0,0);
    }
  }
}
