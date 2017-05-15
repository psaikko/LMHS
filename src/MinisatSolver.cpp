#include "MinisatSolver.h"
#include <algorithm> // count, sort
#include <cstdio>
using namespace std;

MinisatSolver::MinisatSolver() { 
  minisat = new Minisat::Solver(); 

  setFPU();
  minisat->verbosity = cfg.SAT_verbosity;
  minisat->var_decay = cfg.SAT_varDecay;
  minisat->clause_decay = cfg.SAT_clauseDecay;
  minisat->random_var_freq = cfg.SAT_rndFreq;
  minisat->random_seed = cfg.SAT_rndSeed;
  minisat->ccmin_mode = cfg.SAT_ccMinMode;
  minisat->phase_saving = cfg.SAT_phaseSaving;
  minisat->rnd_init_act = cfg.SAT_rndInitActivity;
  minisat->garbage_frac = cfg.SAT_gcFrac;
  minisat->luby_restart = cfg.SAT_lubyRestart;
  minisat->restart_first = cfg.SAT_restartFirst;
  minisat->restart_inc = cfg.SAT_restartInc;
  minisat->learntsize_factor = cfg.SAT_learntSizeFactor;
  minisat->learntsize_inc = cfg.SAT_learntSizeInc;


  if (cfg.SAT_userPolarity == 1)
    upol = Minisat::l_False;
  else if (cfg.SAT_userPolarity == -1)
    upol = Minisat::l_True;
  else
    upol = Minisat::l_Undef;

}

void MinisatSolver::addVariable(int var) 
{
  while (abs(var) >= minisat->nVars()) 
    newVar();
}
 
// solve instance and get unsatisfiable subformula (core)
void MinisatSolver::findCore(vector<int>& out_core) 
{
  out_core.clear();
  solve();
  getConflict(out_core);
}

bool MinisatSolver::findCoreLimited(std::vector<int>& out_core)
{
  out_core.clear();
  Minisat::lbool res = solveLimited();
  
  if (res == Minisat::l_Undef) // Hit resource cap
    return false;
  else if (res == Minisat::l_False) // UNSAT -> fetch conflict
    getConflict(out_core);
  // else: SAT -> no core
  return true;
}

// solve instance with current assumptions, return SAT?
bool MinisatSolver::solve() 
{
  start = clock();
  bool ret;

  ret = minisat->solve(assumptions);
  
  condTerminate(!minisat->okay(), 1, "Error: MiniSat in conflicting state\n");

  end = clock();
  solver_time += (end - start);
  solver_calls++;

  return ret;
}

Minisat::lbool MinisatSolver::solveLimited() {
  Minisat::lbool ret;

  start = clock();

  ret = minisat->solveLimited(assumptions);

  end = clock();
  solver_time += (end - start);
  solver_calls++;

  return ret;
}

void MinisatSolver::getConflict(vector<int>& out_core) 
{
  const Minisat::vec<Minisat::Lit>& conflict = minisat->conflict.toVec();
  int conflict_size = conflict.size();

  log(2, conflict_size ? "c Core found\n" : "c No core found\n");

  out_core.clear();
  out_core.resize(conflict_size);
  for (int i = 0; i < conflict_size; i++) {
    int v = Minisat::var(conflict[i]);
    int s = Minisat::sign(conflict[i]);
    out_core[i] = v * (1 - 2 * s);
  }
}

int MinisatSolver::newVar() {
  int v = 0;
  do {
    v = minisat->newVar(upol);
  } while (v == 0);
  return v;
}

void MinisatSolver::addConstraint(vector<int>& constr) 
{

  Minisat::vec<Minisat::Lit> minisat_clause(constr.size());
  for (unsigned i = 0; i < constr.size(); ++i) {
    int v = abs(constr[i]);
    bool s = constr[i] < 0;
    Minisat::Lit l = s ? ~Minisat::mkLit(v) : Minisat::mkLit(v);
    minisat_clause[i] = l;
  }
  bool ok = minisat->addClause_(minisat_clause);
  condTerminate(!ok, 1, "Error: MiniSat addClause failed\n");
}

void MinisatSolver::getModel(vector<bool>& model) 
{
  model.clear();
  int model_size = minisat->model.size();
  model.resize(model_size);

  for (int i = 0; i < model_size; ++i)
    model[i] = (minisat->model[i] == Minisat::l_True);
}

double MinisatSolver::getModelWeight(std::unordered_map<int, double>& bvar_weights) {

  condTerminate(minisat->model.size() == 0, 1, "MinisatSolver::getModelWeight: no model exists\n");

  double opt = 0;
  for (auto kv : bvar_weights)
    if (minisat->model[kv.first] == Minisat::l_True) opt += kv.second;
  return opt;
}

void MinisatSolver::printStats() {
  log(1, "c SAT:\n");
  log(1, "c   solver calls: %u\n", solver_calls);
  log(1, "c   solver time:  %.2fs\n", SECONDS(solver_time));
}
