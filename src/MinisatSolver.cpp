#include <algorithm> // count, sort
#include <cstdio>
#include <iostream>

#include "MinisatSolver.h"

using namespace std;

MinisatSolver::MinisatSolver()
    : solver_calls(0),
      sat_calls(0),
      unsat_calls(0),
      hasModel(false),
      cfg(GlobalConfig::get()),
      minisat(new Minisat::Solver()) {

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

  minisat->n_shuffles = cfg.n_shuffles;
  minisat->do_shuffle = cfg.do_shuffle;
  minisat->shuffle_time = 0;
  minisat->shuffle_disjoint = cfg.shuffle_disjoint;

  if (cfg.SAT_userPolarity == 1)
    upol = Minisat::l_False;
  else if (cfg.SAT_userPolarity == -1)
    upol = Minisat::l_True;
  else
    upol = Minisat::l_Undef;
}

void MinisatSolver::addVariable(int var) 
{
  log(2, "Add SAT variable %d\n", var);
  while (abs(var) >= minisat->nVars())  {
    newVar();
  }
}
 
// solve instance and get unsatisfiable subformula (core)
void MinisatSolver::findCore(vector<int>& out_core) 
{
  log(3, "c MinisatSolver::findCore\n");
  out_core.clear();
  solve();

  vector<vector<int>> cores;
  getConflicts(cores);

  if (cores.size()) {
    out_core = cores[0];
    log(3, "c found core (size %lu)\n", out_core.size());
    logCore(3, out_core);
  } 
}

void MinisatSolver::findCores(vector<vector<int>>& out_cores) 
{
  out_cores.clear();
  solve();
  getConflicts(out_cores);
}

bool MinisatSolver::findCoreLimited(std::vector<int>& )
{
  printf("findCoreLimited unimplemented\n");
  exit(1);
  return false;

  /*
  out_core.clear();
  Minisat::lbool res = solveLimited();
  
  if (res == Minisat::l_Undef) // Hit resource cap
    return false;
  else if (res == Minisat::l_False) // UNSAT -> fetch conflict
    getConflict(out_core);
  // else: SAT -> no core
  return true;
  */
}

// solve instance with current assumptions, return SAT?
bool MinisatSolver::_solve() 
{
  log(3, "c MinisatSolver::_solve\n");
  Timer timer;
  timer.start();

  //cout << "c assumptions " << assumptions << endl;

  bool ret = minisat->solve(assumptions);
  log(3, "c Minisat retcode %d\n", ret);

  condLog(!minisat->okay(), 1, "c MiniSat in conflicting state\n");

  timer.stop();

  solver_calls++;
  solver_timer.add(timer);
  if (ret) {
    log(3, "c SAT\n");
    ++sat_calls;
    sat_timer.add(timer);
  } else {
    log(3, "c UNSAT\n");
    ++unsat_calls;
    unsat_timer.add(timer);
  }

  return ret;
}

Minisat::lbool MinisatSolver::solveLimited() 
{ 
  Timer timer; 
  timer.start();

  Minisat::lbool ret = minisat->solveLimited(assumptions);

  timer.stop();

  solver_calls++;
  solver_timer.add(timer);
  if (ret == Minisat::l_True) {
    ++sat_calls;
    sat_timer.add(timer);
  } else {
    ++unsat_calls;
    unsat_timer.add(timer);
  }

  return ret;
}

void MinisatSolver::getConflicts(vector<vector<int>>& out_cores) 
{
  log(3, "c minisat %d cores\n", minisat->out_conflicts.size());  
  out_cores.clear();

  for (int i = 0; i < minisat->out_conflicts.size(); ++i) {

    const Minisat::vec<Minisat::Lit>& conflict = minisat->out_conflicts[i]->toVec();
    int conflict_size = conflict.size();

    vector<int> out_core;
    out_core.resize(conflict_size);

    for (int i = 0; i < conflict_size; i++) {
      out_core[i] = lit2int(conflict[i]);
    }

    out_cores.push_back(out_core);
  }

  // sort by size
  sort(out_cores.begin(), out_cores.end(),
         [] (const vector<int> &a, const vector<int> &b) { return a.size() < b.size(); } );

  // clear conflicts
  for (int i = 0; i < minisat->out_conflicts.size(); ++i) {
    minisat->out_conflicts[i]->clear();
    delete minisat->out_conflicts[i];
  }

  minisat->out_conflicts.clear();

  while(out_cores.size() && out_cores.size() > unsigned(cfg.shuffle_coreLimit)) 
      out_cores.pop_back();

  /*
  if (out_cores.size()) { 
    cout << "c core minisat conflicts " << out_cores.size() << endl;
    for (auto core : out_cores) {
      cout << core << endl;
    }
  }
  */
  
}

int MinisatSolver::newVar() {
  int v = 0;
  do {
    v = minisat->newVar(upol);
  } while (v == 0);
  return v;
}

bool MinisatSolver::addConstraint(vector<int>& constr) 
{
  Minisat::vec<Minisat::Lit> minisat_clause(constr.size());
  for (unsigned i = 0; i < constr.size(); ++i) {
    int v = abs(constr[i]);
    bool s = constr[i] < 0;
    Minisat::Lit l = s ? ~Minisat::mkLit(v) : Minisat::mkLit(v);
    minisat_clause[i] = l;
  }
  bool ok = minisat->addClause_(minisat_clause);
  return ok;
  //condTerminate(!ok, 1, "Error: MiniSat addClause failed\n");
}

void MinisatSolver::getModel(vector<bool>& model) 
{
  model.clear();
  int model_size = minisat->model.size();
  model.resize(model_size);

  for (int i = 0; i < model_size; ++i)
    model[i] = (minisat->model[i] == Minisat::l_True);
}

weight_t MinisatSolver::getModelWeight(std::unordered_map<int, weight_t>& bvar_weights) {

  condTerminate(minisat->model.size() == 0, 1, "c MinisatSolver::getModelWeight: no model exists\n");

  weight_t opt = 0;
  for (auto kv : bvar_weights)
    if (minisat->model[kv.first] == Minisat::l_True) opt += kv.second;
  return opt;
}

void MinisatSolver::printStats(const char* solver_name) {

  log(1, "c %s solver calls: %u\n", solver_name, solver_calls);
  log(1, "c %s sat calls:    %u\n", solver_name, sat_calls);
  log(1, "c %s unsat calls:  %u\n", solver_name, unsat_calls);
  log(1, "c %s solver time:  %lu ms\n", solver_name, solver_timer.cpu_ms_total());
  log(1, "c %s sat time:     %lu ms\n", solver_name, sat_timer.cpu_ms_total());
  log(1, "c %s unsat time:   %lu ms\n", solver_name, unsat_timer.cpu_ms_total());
  if (minisat->do_shuffle) {
    log(1, "c %s shuffle time: %.2fs\n", solver_name, float(minisat->shuffle_time) / 1000000.0);
  }
}


vector<int> MinisatSolver::addTempAtMostOneEncoding(vector<int>& core) {
  // encoding as in MaxHS

  int b = newVar();
  vector<int> encv;

  //cout << "encoding AT MOST ONE " << core << endl;
  encv.push_back(b); // encv[0] is blocking var for <= 1 encoding
  for (unsigned i = 0; i < core.size() - 1; ++i)
    encv.push_back(newVar());
  
  vector<int> clause;

  for(unsigned i = 0; i < core.size() - 1 ; i++) {
    clause = {-core[i], encv[i+1], b};
    addConstraint(clause);
    //cout << "amo(1) " << clause << endl;
  }

  for(unsigned i = 1; i < core.size() - 1 ; i++) {
    clause = {-encv[i], encv[i+1], b};
    addConstraint(clause);
    //cout << "amo(2) " << clause << endl;
  }
  
  for(unsigned i = 1; i < core.size(); i++) {
    clause = {-encv[i], -core[i], b};
    addConstraint(clause);
    //cout << "amo(3) " << clause << endl;
  }

  return encv; // return all new vars so they can be set as not original vars
}

void MinisatSolver::removeTempAtMostOneEncoding(int b) {
  minisat->releaseVar(Minisat::mkLit(b, false));
}