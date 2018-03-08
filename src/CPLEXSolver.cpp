#include "CPLEXSolver.h"
#include "GlobalConfig.h"
#include "ProblemInstance.h"
#include <algorithm>
#include <unordered_set>

ILOSTLBEGIN

ILOMIPINFOCALLBACK1(LBCutoffCallback,
                    IloNum, lb)
{
  if ( hasIncumbent() && getIncumbentObjValue() == lb) {
    /*std::cout << "c IP LB cutoff " << getIncumbentObjValue()
                   << std::endl;*/
    abort();
  }
}

ILOMIPINFOCALLBACK1(UBCutoffCallback,
                    IloNum, ub)
{
  if ( hasIncumbent() && getIncumbentObjValue() < ub) {
    /*std::cout << "c IP UB cutoff " << getIncumbentObjValue()
                   << std::endl;*/
    abort();
  }
}

CPLEXSolver::CPLEXSolver() : solver_calls(0), lp_calls(0), has_lp_model(false),
    solutionExists(false), objFuncAttached(false), nObjVars(0), nVars(0),
    reducedCostForcedVars(0), reducedCostRelaxedVars(0),
    lookaheadForcedVars(0), lookaheadImplications(0)
{
  GlobalConfig & cfg = GlobalConfig::get();
  largestCore = 0;

  env = new IloEnv();
  model = IloModel(*env);
  objVars = IloNumVarArray(*env);
  vars    = IloNumVarArray(*env);
  cons = IloRangeArray(*env);
  cplex = IloCplex(model);

  if (cfg.use_LP) {
    has_lp_model = true;
    lp_model = IloModel(*env);
    lp_model.add(model);
    lp_cplex = IloCplex(lp_model);
    lp_model.add(IloConversion(*env, objVars, ILOFLOAT));
    lp_cplex.setParam(IloCplex::Threads, cfg.MIP_threads);
    lp_cplex.setParam(IloCplex::SimDisplay, cfg.MIP_verbosity);
    lp_cplex.setParam(IloCplex::MIPDisplay, cfg.MIP_verbosity);
  }

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

IloNumVar CPLEXSolver::newObjVar(int bVar, weight_t w) {
  log(2, "c MIP obj variable %d %" WGT_FMT "\n", bVar, w);
  char name[11];
  snprintf(name, 11, "%10d", bVar);

  IloNumVar x(*env, 0, 1, ILOBOOL, name);

  var_to_IloVar[bVar] = x;
  var_to_weight[bVar] = w;

  objVars.add(x);
  if (GlobalConfig::get().use_LP) {
    // http://www-01.ibm.com/support/docview.wss?uid=swg21400005
    lp_model.add(IloConversion(*env, x, ILOFLOAT));
  }
  nObjVars++;
  return x;
}

// Create a CPLEX variable for the bvar and append
// it to the objective function with the given weight
void CPLEXSolver::addObjectiveVariable(int bVar, weight_t weight) {
  if (objFuncAttached) {
    model.remove(objective);
    objFuncAttached = false;
  }

#ifdef FLOAT_WEIGHTS
  objective.setExpr(objective.getExpr() + weight * newObjVar(bVar, weight));
#else
  objective.setExpr(objective.getExpr() + ((int64_t)weight) * newObjVar(bVar, weight));
#endif

}

// adds variables to the cplex instance and sets the objective function to
// minimize weight
void CPLEXSolver::addObjectiveVariables(std::unordered_map<int, weight_t> & bvar_weights) {
  if (objFuncAttached) {
    model.remove(objective);
    objFuncAttached = false;
  }

  IloNumExpr objExpr = objective.getExpr();

  weight_t total_w = 0;

  for (auto b_w : bvar_weights) {
    objExpr += long(b_w.second) * newObjVar(b_w.first, b_w.second);
    total_w += b_w.second;
  }

  objective.setExpr(objExpr);

  if (total_w >= 10000000000) {
    // turn on extra precision --
    printf("c increase CPLEX abs. tolerances for very high weights\n");
    //cplex.setParam(IloCplex::EpInt, 0.01);
    cplex.setParam(IloCplex::EpAGap, 1e-4);
    cplex.setParam(IloCplex::EpOpt, 1e-4);
    cplex.setParam(IloCplex::EpRHS, 1e-4);
    cplex.setParam(IloCplex::EpLin, 1e-4);
    cplex.setParam(IloCplex::EpMrk, 0.02);
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

  long solutionSize = 0;

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
void CPLEXSolver::addConstraint(std::vector<int>& core, double bound, Comparator comp) {
  condTerminate(core.empty(), 1,
    "CPLEXSolver::addConstraint - empty constraint\n");

  if (core.size() > largestCore) largestCore = core.size();

  log(2, "c adding MIP constraint (size %lu)\n", core.size());
  logCore(3, core);

  unsigned negs = 0;
  IloExpr expr(*env);
  for (int c : core) {
    if (!var_to_IloVar.count(abs(c))) printf("%d\n", c);
    assert(var_to_IloVar.count(abs(c)));
    IloNumVar x = var_to_IloVar[abs(c)];
    if (c > 0) {
      expr += x;
    } else {
      expr += -x;
      ++negs;
    }
  }
  switch (comp) {
    case GTE:
    {
      IloRange con = (expr >= (bound - negs));
      cons.add(con);
      model.add(con);
      break;
    }
    case LTE:
    {
      IloRange con = (expr <= (bound - negs));
      cons.add(con);
      model.add(con);
      break;
    }
  }
}

bool CPLEXSolver::solveForModel(std::vector<int>& sat_model, weight_t& weight) {
  if (!objFuncAttached) {
    model.add(objective);
    objFuncAttached = true;
  }

  model.add(vars);
  model.add(objVars);
  // reuse solve method but ignore hitting set solution

  sat_model.clear();

  log(2, "c CPLEX: solving MIP problem\n");

  ++solver_calls;
  solver_timer.start();

  if (!cplex.solve()) return false;

  solver_timer.stop();

  if (cplex.getStatus() != IloAlgorithm::Status::Optimal) {
    cerr << "CPLEX status not optimal" << endl;
  }
  log(2, "c CPLEX: Solution status = %d\n", cplex.getStatus());

#if defined(FLOAT_WEIGHTS)
  weight = cplex.getObjValue();
#else
  weight = lround(cplex.getObjValue());
#endif

  // get variable values from cplex
  IloNumArray vals(*env);
  cplex.getValues(vals, vars);

  for (unsigned i = 0; i < nVars; ++i) {
    bool pos = IloAbs(vals[i] - 1.0) < EPS;
    // parse bvars from cplex variable names
    int var;
    sscanf(vars[i].getName(), "%10d", &var);
    sat_model.push_back(pos ? var : -var);
  }

  log(3, "c CPLEX model\n");
  logCore(3, sat_model);

  return true;
}

bool CPLEXSolver::LPsolveHS(std::vector<int>& hittingSet, weight_t& weight) {

  hittingSet.clear();

  if (!objFuncAttached) {
    model.add(objective);
    objFuncAttached = true;
  }

  lp_timer.start();
  ++lp_calls;
  if (!lp_cplex.solve()) {
    lp_timer.stop();
    return false;
  }
  lp_timer.stop();

  #if defined(FLOAT_WEIGHTS)
  weight = lp_cplex.getObjValue();
  #else
  weight = lround(lp_cplex.getObjValue());
  #endif

  IloNumArray vals(*env);
  lp_cplex.getValues(vals, objVars);

  // gather set of variables with true value
  for (unsigned i = 0; i < nObjVars; ++i) {
    if (vals[i] > (1.0 / (double)largestCore)) {
      // parse bvars from cplex variable names
      int bVar;
      sscanf(objVars[i].getName(), "%10d", &bVar);
      hittingSet.push_back(bVar);
    }
  }

  return true;

}

bool CPLEXSolver::getLPConditionalLB(int var, weight_t & out_lb, IloNumArray& out_rc) {

  out_lb = 0;

  if (!objFuncAttached) {
    model.add(objective);
    objFuncAttached = true;
  }

  IloRange cons;

  cons = (var_to_IloVar[var] >= 1);
  lp_model.add(cons);

  lp_timer.start();
  lp_calls++;
  if (!lp_cplex.solve()) {
    lp_timer.stop();
    lp_model.remove(cons);
    if (lp_cplex.getStatus() == IloAlgorithm::Status::Infeasible) {
      return false;
    } else {
      terminate(1, "CPLEX LP unexpected error");
    }
  }
  lp_timer.stop();

  lp_cplex.getReducedCosts(out_rc, objVars);

#if defined(FLOAT_WEIGHTS)
  weight_t weight = lp_cplex.getObjValue();
#else
  weight_t weight = lround(lp_cplex.getObjValue());
#endif

  lp_model.remove(cons);

  out_lb = weight;
  return true;
}

// returns positive valued objective function variables in 'solution'
// and value of objective function in 'weight'
CPLEXSolver::Status CPLEXSolver::solveForHS(std::vector<int>& hittingSet, weight_t& opt, ProblemInstance *instance) {

  static std::unordered_set<int> hitBVars;

  if (!objFuncAttached) {
    model.add(objective);
    objFuncAttached = true;
  }

  static weight_t LB = 0;
  static weight_t UB = 0;

  bool bounds_changed = LB != instance->LB || UB != instance->UB;

  // Bounding is enabled and bounds have changed
  if (GlobalConfig::get().CPLEX_reducedCosts && bounds_changed) {

    LB = instance->LB;
    UB = instance->UB;

    lp_timer.start();
    ++lp_calls;
    lp_cplex.solve();
    lp_timer.stop();

    double LB_lp = lp_cplex.getObjValue();

    if (ceil(LB_lp - EPS) > LB) {
      log(1, "c LP improved LB\n");
      instance->updateLB(ceil(LB_lp - EPS));
    }

    if (ceil(LB_lp - EPS) == UB) {
      opt = UB;
      return CPLEXSolver::Status::Optimal;
    }

    IloNumArray reducedCosts(*env);
    lp_cplex.getReducedCosts(reducedCosts, objVars);

    IloNumArray lp_vals(*env);
    lp_cplex.getValues(lp_vals, objVars);

    for (unsigned i = 0; i < nObjVars; ++i) {
      // variable is forced already?
      if (IloAbs(objVars[i].getUB() - objVars[i].getLB()) < EPS)
        continue;

      int bVar;
      sscanf(objVars[i].getName(), "%10d", &bVar);

      weight_t w = var_to_weight[bVar];
      double rc = ceil(reducedCosts[i]);
      weight_t bvar_LB = 0;

      // conditional lb cannot exceed ub
      if (UB > (weight_t(ceil(LB_lp - EPS)) + w)) continue;

      assert(instance->UB_bool_solution.size() > unsigned(bVar));

      if (IloAbs(lp_vals[i]) < EPS) { // try to harden

        // reduced cost is zero or negative?
        if (rc <= EPS) continue;

        bvar_LB = weight_t(ceil(LB_lp + rc));

        // conditinal lb too low to force
        if (UB > bvar_LB) continue;

        bool true_in_best_model = instance->UB_bool_solution[bVar];
        // cannot force to 0 since best found model might be optimal
        if (true_in_best_model && bvar_LB == UB) continue;

        forceVar(instance, bVar, false);
        std::cout << "c forced bVar " << bVar << " lp_lb: " << LB_lp \
          << " ub: " << UB << " bv_lb: " << bvar_LB << std::endl;
        ++reducedCostForcedVars;
      } else {  // try to relax

        // reduced cost is zero or negative?
        if (rc >= EPS) continue;

        bvar_LB = weight_t(ceil(LB_lp - rc - EPS));

        // conditinal lb too low to relax
        if (UB > bvar_LB) continue;

        bool false_in_best_model = !instance->UB_bool_solution[bVar];
        // cannot force to 1 since best found model might be optimal
        if (false_in_best_model && bvar_LB == UB) continue;

        forceVar(instance, bVar, true);
        std::cout << "c relaxed bVar " << bVar << " lp_lb: " << LB_lp \
          << " ub: " << UB << " bv_lb: " << bvar_LB << std::endl;
        ++reducedCostRelaxedVars;
      }
    }
  }


  // previous result was optimal for fewer constraints,
  // so we can cut off search if we get there again
  static IloNum lastOpt = 0;

  if (GlobalConfig::get().CPLEX_lb_cutoff) {
    cplex.use(LBCutoffCallback(*env, lastOpt));
  }

  if (GlobalConfig::get().CPLEX_ub_cutoff) {
    cplex.use(UBCutoffCallback(*env, instance->UB));
  }

  log(2, "c CPLEX: solving MIP problem\n");

  ++solver_calls;
  solver_timer.start();
  hittingSet.clear();

  if (!cplex.solve()) {
    return CPLEXSolver::Status::Failed;
  }

  solver_timer.stop();

  CPLEXSolver::Status status;
  switch (cplex.getStatus()) {
    case IloAlgorithm::Status::Optimal:
      status = Status::Optimal;
      lastOpt = cplex.getObjValue();
      log(2, "c CPLEX optimal solution\n");
      break;
    case IloAlgorithm::Status::Feasible:
      status = Status::Feasible;
      log(2, "c CPLEX feasible solution\n");
      break;
    default:
      status = Status::Failed;
      std::cerr << "error: bad cplex status" << std::endl;
      break;
  }

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
      sscanf(objVars[i].getName(), "%10d", &bVar);
      hittingSet.push_back(bVar);
      hitBVars.insert(bVar);
    }
  }

  std::sort(hittingSet.begin(), hittingSet.end());
  log(2, "c CPLEX: hitting set:\n");
  logCore(2, hittingSet);

#if defined(FLOAT_WEIGHTS)
  opt = cplex.getObjValue();
#else
  opt = lround(cplex.getObjValue());
#endif

  solutionExists = true;

  return status;
}

void CPLEXSolver::exportModel(std::string file) {
  cplex.exportModel(file.c_str());
}

void CPLEXSolver::forceVar(ProblemInstance * instance, int var, bool val) {
  if (var_to_IloVar.count(var)) {
    int bound = val ? 1 : 0;

    var_to_IloVar[var].setBounds(bound, bound);
    instance->forceBvar(var, val);
  }
}
