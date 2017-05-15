#include "SCIPSolver.h"
#include "GlobalConfig.h"

SCIPSolver::SCIPSolver() : lastSol(nullptr) {
  SCIPcreate(& scip);
  SCIPincludeDefaultPlugins(scip);
  SCIPcreateProb(scip, "min_cost_hs", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  SCIPsetObjsense(scip, SCIP_OBJSENSE_MINIMIZE);
  SCIPsetMessagehdlrQuiet(scip, verbosity < 1);

  if (verbosity > 1) SCIPprintVersion(scip, NULL);
}

SCIPSolver::~SCIPSolver() {
  // free variables
  for (auto v : scip_variables) SCIPreleaseVar(scip, &v);
  // free constraints
  for (auto c : scip_constraints) SCIPreleaseCons(scip, &c);
  // free solver
  SCIPfree(& scip);
}

void SCIPSolver::addObjectiveVariables(std::unordered_map<int, double> & bvar_weights) {
  for (auto it : bvar_weights) {
      addObjectiveVariable(it.first, it.second);
  }
}

void SCIPSolver::addObjectiveVariable(int bVar, double weight) {
  SCIP_VAR*       var;
  
  // create binary variable and add it to the SCIP problem if it does not exist yet
  if (bvar_scip_var.count(bVar) == 0) {
    char varname[11];
    snprintf(varname, 11, "%10d", bVar);

    SCIPcreateVar(
        /* SCIP instance */ scip,                   /* address: */      & var, 
        /* name: */         varname,                /* lower bound: */  0.0, 
        /* upper bound: */  1.0,                    /* objective: */    weight, 
        /* type: */         SCIP_VARTYPE_BINARY,    /* initial: */      TRUE, 
        /* removable: */    FALSE,                  /* user data: */    NULL, NULL, NULL, NULL, NULL);
    SCIPaddVar(scip, var);

    log(2, "new SCIP variable \"%s\" with objective %f\n", var->name, var->obj);

    bvar_scip_var[bVar] = var;
    scip_variables.push_back(var);
  } else { 
    var = bvar_scip_var[bVar];
    log(2, "old SCIP variable \"%s\" with objective %f\n", var->name, var->obj);
    condTerminate(weight != var->obj, 1, 
      "Error: bvar %d added to MIP twice with conflicting weights (%.2f, %.2f)\n", var, weight, var->obj);
  }
}

void SCIPSolver::reset() {
  if (SCIP_OKAY != _reset())
    terminate(1, "SCIP Error in reset?\n");
}

SCIP_RETCODE SCIPSolver::_reset() 
{
  // free variables
  for (auto v : scip_variables) SCIP_CALL(SCIPreleaseVar(scip, &v));
  // free constraints
  for (auto c : scip_constraints) SCIP_CALL(SCIPreleaseCons(scip, &c));
  
  scip_variables.clear();
  scip_constraints.clear();
  bvar_scip_var.clear();

  solutionExists = false;
  lastSol = nullptr;

  return SCIP_OKAY;
}

void SCIPSolver::forbidCurrentSolution() {
  condTerminate(!solutionExists, 1,
    "SCIPSolver::forbidCurrentSolution - no current solution exists\n");
  if (SCIP_OKAY != _forbidCurrentSolution())
    terminate(1, "SCIP Error in forbidCurrentSolution\n");
}

SCIP_RETCODE SCIPSolver::_forbidCurrentSolution() {
  
  SCIP_CONS*              sub_cons;
  SCIP_CONS*              super_cons;
  char                    sub_cons_name[19];
  char                    super_cons_name[19];
  
  std::vector<int> sub_vars;
  std::vector<int> super_vars;

  for (unsigned i = 0; i < scip_variables.size(); i++) {
    if (SCIPgetSolVal(scip, lastSol, scip_variables[i]) > 0.5 ) {
      super_vars.push_back(i);
    } else {
      sub_vars.push_back(i);
    }
  }

  SCIP_CALL(SCIPfreeTransform(scip));

  snprintf(sub_cons_name, 19, "incr_cons%10ld", scip_constraints.size());
  snprintf(super_cons_name, 19, "incr_cons%10ld", scip_constraints.size()+1);

  SCIP_CALL(SCIPcreateConsLinear(
      /* SCIP instance: */ scip,         /* address: */          & sub_cons, 
      /* name: */          sub_cons_name, /* nvars: */            0, 
      /* var array: */     NULL,         /* coefficients: */     NULL, 
      /* lower bound: */   1.0 ,         /* upper bound: */      SCIPinfinity(scip), 
      TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));

  SCIP_CALL(SCIPcreateConsLinear(
      /* SCIP instance: */ scip,                  /* address: */          & super_cons, 
      /* name: */          super_cons_name,       /* nvars: */            0, 
      /* var array: */     NULL,                  /* coefficients: */     NULL, 
      /* lower bound: */   1.0 - sub_vars.size(), /* upper bound: */      SCIPinfinity(scip), 
      TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));

  for (auto i : super_vars)
    SCIP_CALL(SCIPaddCoefLinear(scip, super_cons, scip_variables[i], -1.0));
  for (auto i : sub_vars)
    SCIP_CALL(SCIPaddCoefLinear(scip, sub_cons, scip_variables[i], 1.0));

  SCIP_CALL(SCIPaddCons(scip, sub_cons));
  scip_constraints.push_back(sub_cons);
  SCIP_CALL(SCIPaddCons(scip, super_cons));
  scip_constraints.push_back(super_cons);

  return SCIP_OKAY;
}

void SCIPSolver::addConstraint(std::vector<int>& core) {
  condTerminate(!core.size(), 1,
    "SCIPSolver::addConstraint - empty core\n");
  if (SCIP_OKAY != _addConstraint(core))
    terminate(1, "SCIP Error in addConstraint\n");
}

SCIP_RETCODE SCIPSolver::_addConstraint(std::vector<int> & core) 
{
  SCIP_CALL(SCIPfreeTransform(scip));
  std::vector<SCIP_VAR*> core_vars;
  std::vector<SCIP_Real> coeffs;
  SCIP_CONS*             cons;
  char                   cons_name[15];
  int                    negs = 0;

  for (int b : core) {
    core_vars.push_back(bvar_scip_var[abs(b)]);
    coeffs.push_back(b < 0 ? -1.0 : 1.0);  
    if (b < 0) ++negs;
  }
  
  snprintf(cons_name, 15, "core%10ld", scip_constraints.size());

  SCIP_CALL(SCIPcreateConsLinear(
    /* SCIP instance: */ scip,       /* address: */          & cons, 
    /* name: */          cons_name,  /* nvars: */            0, 
    /* var array: */     NULL,       /* coefficients: */     NULL, 
    /* lower bound: */   1.0 - negs, /* upper bound: */      SCIPinfinity(scip), 
    TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));

  log(2, "new SCIP constraint \"%s\" with %ld variables:\n", 
    cons->name, core_vars.size());

  for (unsigned i = 0; i < core_vars.size(); i++)
    SCIP_CALL(SCIPaddCoefLinear(scip, cons, core_vars[i], coeffs[i]));    
  
  SCIP_CALL(SCIPaddCons(scip, cons));
  scip_constraints.push_back(cons);

  return SCIP_OKAY;
}

bool SCIPSolver::solveForHS(std::vector<int>& out_solution, double& out_weight) {
  if (SCIP_OKAY != _findSolution(out_solution, out_weight))
    return false;
  solutionExists = true;
  return true;
}

void SCIPSolver::exportModel(std::string file) {
  SCIPwriteOrigProblem(scip, file.c_str(), NULL, true);
}

SCIP_RETCODE SCIPSolver::_findSolution (
        std::vector<int>&   out_solution,
        double&             out_weight) 
{

  solver_calls++;

  // get solution
  clock_t start = clock();
  SCIP_CALL( SCIPsolve(scip) );
  lastSol = SCIPgetBestSol(scip);
  clock_t end = clock();
  solver_time += (end - start);
  
  condTerminate(lastSol == NULL, 1, 
    "SCIPSolver::findSolution - no MCHS solution\n");

  int clause;
  out_weight = 0;

  for (auto v : scip_variables) {
    sscanf(v->name, "%d", &clause);
    if (SCIPgetSolVal(scip, lastSol, v) > 0.5 ) {
      out_weight += (double)v->obj;
      out_solution.push_back(clause);
    }
  }

  log(2, "c SCIP: Minimum cost hitting set:\n");
  logCore(2, out_solution);
  log(2, "c weight %.2f\n", out_weight);

  return SCIP_OKAY;
}

void SCIPSolver::addVariable(int) {
  printf("SCIPSolver::addVariable not implemented\n");
  exit(1);
}

bool SCIPSolver::solveForModel(std::vector<int>&, double&) {
  printf("SCIPSolver::solveForModel not implemented\n");
  exit(1);
}

void SCIPSolver::setUpperBound(double ub) {
  if (ub < scip->primal->upperbound) {
    SCIPprimalSetUpperbound(scip->primal, scip->mem->probmem, scip->set, scip->stat, scip->eventqueue, scip->transprob, scip->tree, scip->lp, ub);
  }
}