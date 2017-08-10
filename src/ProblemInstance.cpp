#include <istream>
#include <iostream>
#include <algorithm>
#include <zlib.h>
#include <cassert>

#include "ProblemInstance.h"
#include "WCNFParser.h"

using namespace std;


vector<int> ProblemInstance::reconstruct(std::vector<int> & model) {
  assert(preprocessor != nullptr);
  return preprocessor->reconstruct(model);
}

ProblemInstance::ProblemInstance(ostream& out)
    : cfg(GlobalConfig::get()),
      LB(0),
      UB(numeric_limits<weight_t>::max()),
      sat_solver(nullptr),
      mip_solver(nullptr),
      muser(nullptr),
      max_var(0),
      fixed_variables(0),
      out(out)
{
}

ProblemInstance::ProblemInstance(unsigned nClauses, weight_t top, weight_t* weights,
                                 int* raw_clauses, ostream& out)
    : cfg(GlobalConfig::get()),
      LB(0),
      UB(numeric_limits<weight_t>::max()),
      sat_solver(nullptr),
      mip_solver(nullptr),
      muser(nullptr),
      max_var(0),
      fixed_variables(0),
      out(out)
{
  vector<vector<int>> tmp_clauses;

  unsigned i = 0, j = 0;
  while (i < nClauses) {
    vector<int> clause;
    while (raw_clauses[j] != 0) {
      max_var = max(max_var, abs(raw_clauses[j]));
      clause.push_back(raw_clauses[j++]);
    }
    tmp_clauses.push_back(clause);
    ++i;
    ++j;
  }

  for (auto cl : tmp_clauses)
    for (int i : cl) max_var = max(i, max_var);

  for (i = 0; i < tmp_clauses.size(); ++i) {
    if (weights[i] < top) {
      addSoftClause(tmp_clauses[i], weights[i]);
    } else {
      addHardClause(tmp_clauses[i]);
    }
  }
}

ProblemInstance::ProblemInstance(istream& wcnf_in, ostream& out)
    : cfg(GlobalConfig::get()),
      LB(0),
      UB(numeric_limits<weight_t>::max()),
      sat_solver(nullptr),
      mip_solver(nullptr),
      muser(nullptr),
      max_var(0),
      fixed_variables(0),
      out(out)
{

  parse_timer.start();

  condTerminate(wcnf_in.fail(), 1,
                "Error: Bad input stream (check filepath)\n");

  weight_t top;
  vector<weight_t> weights;
  vector<int> file_assumptions;
  vector<vector<int>> tmp_clauses;

  parseWCNF(wcnf_in, weights, branchVars, top, tmp_clauses, 
            file_assumptions, max_var);

  if (cfg.preprocess) {    
    preprocess_timer.start();

    vector<vector<int>> preprocessed_clauses;
    vector<weight_t> preprocessed_weights;
    file_assumptions.clear();

    int loglevel = 0;
    double time_limit = 1e9;

    preprocessor = new maxPreprocessor::PreprocessorInterface(tmp_clauses, weights, top);

    preprocessor->preprocess(cfg.pre_techniques, loglevel, time_limit);

    preprocessor->getInstance(preprocessed_clauses, preprocessed_weights, file_assumptions);

    weights = preprocessed_weights;
    tmp_clauses = preprocessed_clauses;

    preprocess_timer.stop();

    log(1, "c preprocessing time %lu ms\n", preprocess_timer.cpu_ms_total());

    if (cfg.pre_only) {
      printf("p wcnf %d %lu %lu\n", 0, tmp_clauses.size(), top);
      for (unsigned i = 0; i < tmp_clauses.size(); ++i) {
        printf("%ld", weights[i]);
        for (int l : tmp_clauses[i])
          printf(" %d", l);
        printf(" 0\n");
      }
      exit(1);
    }
  }

  //for (int i = 1; i <= max_var; ++i)
  //  isOriginalVariable[i] = true;

  if (cfg.is_LCNF) {
    //
    // LCNF mode
    //
    for (unsigned i = 0; i < file_assumptions.size(); ++i) {
      int a = file_assumptions[i];
      if (a > 0) {
        flippedInternalVarPolarity[abs(a)] = true;
      }
    }

    for (unsigned i = 0; i < tmp_clauses.size(); ++i) {
      if (weights[i] < top) {
        if (tmp_clauses[i].size() == 1 &&
            count(file_assumptions.begin(), file_assumptions.end(),
                  tmp_clauses[i][0])) {
          int bv = abs(tmp_clauses[i][0]);
          addBvar(bv, weights[i]);
          isOriginalVariable[bv] = true;
        } else {
          addSoftClause(tmp_clauses[i], weights[i]);
        }
      }
    }
    //
    // LCNF: add hard clauses
    //
    for (unsigned i = 0; i < tmp_clauses.size(); ++i) {
      if (weights[i] >= top) {
        // check if a bvar exists in the hard clause
        for (int v : tmp_clauses[i]) {
          if (bvar_weights.count(abs(v)) == 1) {
            // add as soft clause using existing variables
            addSoftClauseWithBv(tmp_clauses[i]);
            goto next_clause;
          }
        }
        // else add normally as hard clause
        addHardClause(tmp_clauses[i]);
      next_clause:
        continue;
      }
    }
  } else {
    //
    // Normal WCNF instance
    //
    for (unsigned i = 0; i < tmp_clauses.size(); ++i) {
      if (weights[i] < top) {
        addSoftClause(tmp_clauses[i], weights[i]);
      } else {
        addHardClause(tmp_clauses[i]);
      }
    }
  }

  parse_timer.stop();
}

ProblemInstance::~ProblemInstance() {
  for (auto p : clauses) delete p;
  for (auto e : bvar_soft_clauses)
    for (auto p : e.second) delete p;

  delete sat_solver;
  delete mip_solver;
  if (muser) delete muser;
}

void ProblemInstance::attach(MinisatSolver* s) {
  sat_solver = s;

  // add all variables to solver
  for (int i = 0; i <= max_var; ++i) sat_solver->addVariable(i);

  // create assumption data for bvars
  for (auto kv : bvar_weights) sat_solver->addBvarAssumption(kv.first);

  // if user-specified branching for sat solver, set decision vars
  if (branchVars.size() > 0) {
    for (int i = 0; i < sat_solver->nVars(); ++i)
      sat_solver->setVarDecision(i, false);
    for (int v : branchVars) sat_solver->setVarDecision(v, true);
  }

  for (auto cl : clauses) {
    if (!sat_solver->addConstraint(*cl)) {
      isUNSAT = true;
      return;
    }
  }
}

void ProblemInstance::attachMuser(MinisatSolver* s) {
  muser = s;

  // add all variables to solver
  for (int i = 0; i <= max_var; ++i) muser->addVariable(i);

  // create assumption data for bvars
  for (auto kv : bvar_weights) muser->addBvarAssumption(kv.first);

  // if user-specified branching for sat solver, set decision vars
  if (branchVars.size() > 0) {
    for (int i = 0; i < muser->nVars(); ++i)
      muser->setVarDecision(i, false);
    for (int v : branchVars) muser->setVarDecision(v, true);
  }

  for (auto cl : clauses) {
    if (!muser->addConstraint(*cl)) {
      isUNSAT = true;
      return;
    }
  }
}

void ProblemInstance::attach(CPLEXSolver* s) {
  mip_solver = s;
  mip_solver->addObjectiveVariables(bvar_weights);
}

void ProblemInstance::toLCNF(ostream& out) {
  // declare labels
  /*out << "c assumptions";
  for (auto v_w : bvar_weights) {
    int var = v_w.first;
    out << " " << (flippedInternalVarPolarity[var] ? -var : var);
  }
  out << endl;*/

  // header
  long top = numeric_limits<long>::max() / 2;
  out << "p wcnf " << max_var << " " << (clauses.size() + bvar_weights.size()) << " " << top << endl; 

  // hard clauses
  for (auto clause : clauses) {
    out << top;
    for (auto lit : *clause) {
      out << " " << (flippedInternalVarPolarity[abs(lit)] ? -lit : lit);
    }
    out << " 0" << endl;
  }

  // label-clauses
  for (auto v_w : bvar_weights) {
    auto weight = v_w.second;
    auto var = v_w.first;
    out << weight << " " << (flippedInternalVarPolarity[var] ? var : -var) << " 0" << endl;
  }
}


void ProblemInstance::forbidCurrentMIPSol() {
  condTerminate(mip_solver == nullptr, 1,
                "Error: no MIP solver attached to ProblemInstance\n");

  mip_solver->forbidCurrentSolution();
}

void ProblemInstance::forbidCurrentModel() {
  condTerminate(sat_solver == nullptr, 1,
                "Error: no SAT solver attached to ProblemInstance\n");
  vector<int> solution;
  getSolution(solution);

  condTerminate(solution.size() == 0, 1,
                "Error: no model given by SAT solver\n");

  // negate assignment
  for (unsigned i = 0; i < solution.size(); ++i) solution[i] = -solution[i];

  addHardClause(solution);
}

// add a hard clause to the SAT instance
void ProblemInstance::addHardClause(vector<int>& hc, bool original) {

  for (int l : hc) {
    assert(bvar_weights.count(abs(l)) == 0);

    max_var = max(abs(l), max_var);
    if (sat_solver != nullptr && max_var >= sat_solver->nVars())
      sat_solver->addVariable(max_var);
  }

  auto clause = new vector<int>(hc);

  if (original)
    for (int l : hc) isOriginalVariable[abs(l)] = true;

  hard_clauses.push_back(clause);
  clauses.push_back(clause);

  if (sat_solver != nullptr) {
    if (!sat_solver->addConstraint(hc)) {
      isUNSAT = true;
    }
  }

  if (muser != nullptr) {
    if (!muser->addConstraint(hc)) {
      isUNSAT = true;
    }
  }
}

// add a soft clause to the SAT instance
int ProblemInstance::addSoftClause(vector<int>& sc, weight_t weight,
                                   bool original) 
{

  if (weight < EPS) {
    printf("c warning: ignoring 0-weight clause\n");
    return -1;
  }

  for (int l : sc) {
    assert(bvar_weights.count(abs(l)) == 0);

    max_var = max(abs(l), max_var);
    if (sat_solver != nullptr)
      while (max_var >= sat_solver->nVars()) sat_solver->addVariable(max_var);
  }

  auto clause = new vector<int>(sc);

  if (original)
    for (int l : *clause) isOriginalVariable[abs(l)] = true;

  int bVar = ++max_var;
  addBvar(bVar, weight);
  updateBvarMap(bVar, clause);

  auto b_clause = new vector<int>(sc);

  (*b_clause).push_back(bVar);
  clauses.push_back(b_clause);
  soft_clauses.push_back(b_clause);

  if (sat_solver != nullptr) sat_solver->addConstraint(*b_clause);
  if (muser != nullptr) muser->addConstraint(*b_clause);

  return bVar;
}

void ProblemInstance::addBvar(int bVar, weight_t weight) {
  assert(bVar > 0);
  assert(bvar_weights.count(bVar) == 0);

  if (sat_solver != nullptr) {
    sat_solver->addVariable(bVar);
    sat_solver->addBvarAssumption(bVar);
  }
  if (muser != nullptr) {
    muser->addVariable(bVar);
    muser->addBvarAssumption(bVar);
  }
  if (mip_solver != nullptr) {
    mip_solver->addObjectiveVariable(bVar, weight);  
  }
  bvar_weights[bVar] = weight;
}

// add a soft clause to the SAT instance with existing bvar(s)
void ProblemInstance::addSoftClauseWithBv(vector<int>& sc_, bool original) 
{

  for (unsigned i = 0; i < sc_.size(); i++) {
    int v = abs(sc_[i]);
    max_var = max(v, max_var);
    if (flippedInternalVarPolarity[v])
      sc_[i] *= -1;
  }

  auto sc = new vector<int>(sc_);

  if (original)
    for (int l : *sc) isOriginalVariable[abs(l)] = true;

  for (int v : *sc) {
    if (bvar_weights.count(abs(v)) == 1) {  // v is a bvar
      auto cl = new vector<int>(sc->size() - 1);
      copy_if(sc->begin(), sc->end(), cl->begin(),
              [&](int l) { return abs(l) != abs(v); });
      updateBvarMap(abs(v), cl);
    }
  }

  clauses.push_back(sc);
  soft_clauses.push_back(sc);

  if (sat_solver != nullptr) sat_solver->addConstraint(*sc);
  if (muser != nullptr) muser->addConstraint(*sc);
}

void ProblemInstance::updateBvarMap(int bVar, vector<int>* sc) {
  assert(bVar > 0);
  assert(bvar_weights.count(bVar));

  bvar_clause_ct[bVar]++;
  if (bvar_soft_clauses.count(bVar)) {
    bvar_soft_clauses[bVar].push_back(sc);
  } else {
    bvar_soft_clauses[bVar] = {sc};
  }
}

void ProblemInstance::forceBvar(int bv, bool pol) {
  // fix the variable in sat solver
  fixed_variables++;

  vector<int> * unit_cl = new vector<int>({ pol ? bv : -bv });

  hard_clauses.push_back(unit_cl);
  clauses.push_back(unit_cl);

  if (sat_solver != nullptr) {
    if (pol == false)
      sat_solver->removeBvarAssumption(bv);
    if (!sat_solver->addConstraint(*unit_cl)) {
      isUNSAT = true;
    }
  }

  if (muser != nullptr) {
    if (pol == false)
      muser->removeBvarAssumption(bv);
    if (!muser->addConstraint(*unit_cl)) {
      isUNSAT = true;
    }
  }

  // remove from structures
  if (pol == false) {
    bvar_weights.erase(bv);
    bvar_soft_clauses.erase(bv);
    bvar_clause_ct.erase(bv);
  }

  // TODO: remove from found cores
  if (pol == false)
    fixQueue.push_back(bv);
//  else
//    relaxQueue.push_back(bv);
}

void ProblemInstance::printStats() {
  if (!sat_solver) return;

  log(1, "c Core reduce time: %lu ms\n", reduce_timer.cpu_ms_total());

  log(1, "c Variables:        %d\n", sat_solver->nVars());
  log(1, "c Labels:           %lu\n", bvar_weights.size());
  log(1, "c Fixed:            %u\n", fixed_variables);
  log(1, "c Clauses:          %lu\n", clauses.size());
  log(1, "c Hard clauses:     %lu\n", hard_clauses.size());
  log(1, "c Soft clauses:     %lu\n", soft_clauses.size());
}

void ProblemInstance::getSolution(vector<int>& out_solution) {
  getSolution(out_solution, sat_solver);
}

void ProblemInstance::getSolution(vector<int>& out_solution, MinisatSolver * solver) {

  condTerminate(solver == nullptr, 1,
                "Error: no SAT solver attached to ProblemInstance\n");

  out_solution.clear();
  vector<bool> model;
  solver->getModel(model);

  assert(model.size() > 0 || max_var == 0); // Error: no model given by SAT solver

  for (auto b_w : bvar_weights) {
    int b = b_w.first;
    // if bVar is selected, we check that it is necessary
    if (model[abs(b)]) {
      // for each clause of the bvar
      for (auto cl : bvar_soft_clauses[b]) {
        // check if it is satisfied
        for (int lit : *cl) {
          if ((lit > 0) == model[abs(lit)]) 
            goto cl_sat;
        }
        // if unsat, we stop and count the cost of the bvar
        goto next_bv;
        
        // if sat, we check the remaining clauses
        cl_sat: continue;
      }
      // all clauses satisfied, so we change the model 
      // to disable the bvar and recompute 
      model[abs(b)] = false;
    }  
    next_bv: continue;
  }

  for (unsigned i = 0; i < model.size(); ++i) {
    if (isOriginalVariable[i]) {
      out_solution.push_back(model[i] ? i : -i);
    }
  }
}

bool ProblemInstance::bVarSatisfied(vector<bool> & model, int b) {
  // for each clause of the bvar
  for (auto cl : bvar_soft_clauses[b]) {
    // check if it is satisfied
    for (int lit : *cl) {
      if ((lit > 0) == model[abs(lit)]) 
        goto cl_sat;
    }

    // a clause is unsatisfied
    return false;
    
    // if sat, we check the remaining clauses
    cl_sat: continue;
  }
  return true;
}

weight_t ProblemInstance::tightenModel(vector<bool>& model) {
  weight_t model_cost = 0;
  
  for (auto b_w : bvar_weights) {
    int b = b_w.first;
    weight_t w = b_w.second;
    // if bVar is selected, we check if it is necessary
    if (model[b]) {
      if (bVarSatisfied(model, b)) {
        // clauses of the bvar are satisfied, it is redundant
        model[b] = false;
      } else {
        // some clause of b is unsat, increase weight
        model_cost += w;
      }
    } else {
      // bVar is not active: clause is satisfied: no cost accrued
    }
  }

  return model_cost;
}

weight_t ProblemInstance::getSolutionWeight(MinisatSolver * solver) {

  condTerminate(solver == nullptr, 1,
                "Error: no SAT solver attached to ProblemInstance\n");

  vector<bool> model;
  solver->getModel(model);

  weight_t w = tightenModel(model);

  return w;  
  
}

weight_t ProblemInstance::totalWeight() {
  weight_t sum = 0;
  for (auto v_w : bvar_weights) sum += v_w.second;
  return sum;
}

// find equiv constraints in SAT instance with b-variables
void ProblemInstance::getBvarEquivConstraints(
    vector<vector<int>>& out_constraints) 
{
  unordered_map<int, int> eqs;

  for (auto& p : bvar_soft_clauses) {
    int bvar = p.first;
    eqs[bvar] = bvar;
    // no equivalence if same bvar has multiple clauses
    if (p.second.size() == 1) {
      vector<int>& sc = *(p.second[0]);
      if (sc.size() == 1 && bvar_weights.count(abs(sc[0])) == 0) {
        // For finding an optimal solution, the b-variable of a unit clause
        // is equivalent to the negation of that clause's literal
        int l = sc[0];
        int v = abs(l); 
        
        int s = l < 0 ? 1 : -1;
        eqs[v] = bvar * s;
      }
    }
  }

  for (auto cl : clauses) {
    vector<int> constr;

    for (int l : *cl) {
      if (eqs.count(abs(l)) == 0) goto bv_eq_next_clause;
      int s = l < 0 ? -1 : 1;
      if (!count(constr.begin(), constr.end(), (eqs[abs(l)] * s)))
        constr.push_back(eqs[abs(l)] * s);
    }

    if (constr.size() == 2 && constr[0] == -constr[1]) goto bv_eq_next_clause;
    out_constraints.push_back(constr);

  bv_eq_next_clause:
    continue;
  }
}

void ProblemInstance::getLabelOnlyClauses(vector<vector<int>>& label_cls) 
{

  label_cls.clear();

  function<bool(vector<int>*)> isLabelClause = [this](vector<int>* v) {
    return all_of(v->begin(), v->end(),
                  [this](int i) { return bvar_weights.count(abs(i)) == 1; });
  };

  for (auto sc : soft_clauses)
    if (isLabelClause(sc)) label_cls.push_back(*sc);
}

void ProblemInstance::reduceCore(vector<int>& core, MinimizeAlgorithm alg) {
  log(3, "c ProblemInstance::reduceCore (size %lu)\n", core.size());
  logCore(3, core);
  MinisatSolver * min_solver = cfg.separate_muser ? muser : sat_solver;

  if (core.size() == 1) return;

  reduce_timer.start();

  switch (alg) {
    case MinimizeAlgorithm::binary:
      binarySearchMinimize(min_solver, core);
      break;
    case MinimizeAlgorithm::rerefute:
      reRefuteCore(min_solver, core);
      break;
    case MinimizeAlgorithm::constructive:
      constructiveMinimize(min_solver, core);
      break;
    case MinimizeAlgorithm::destructive:
      destructiveMinimize(min_solver, core);
      break;
    case MinimizeAlgorithm::cardinality:
      cardinalityMinimize(min_solver, core);
      break;
  }

  reduce_timer.stop();
}


// 
void ProblemInstance::constructiveMinimize(MinisatSolver * solver, vector<int>& core) {
  log(3, "c ProblemInstance::constructiveMinimize (size %lu)\n", core.size());
  vector<int> mus;
  vector<int> lits(core);
  random_shuffle(lits.begin(), lits.end());
  vector<int> subcore;

  bool is_mus = false;

  while( !is_mus ) {

    solver->setBvars();
    for (int b : mus)
      solver->unsetBvar(b);

    for (unsigned i = 0; i <= lits.size(); ++i) {
      solver->clearAssumptions();
      solver->assumeBvars();
      solver->findCore(subcore);
      bool sat = !subcore.size();

      if (sat) {
        weight_t w = getSolutionWeight(solver);
        if (w < UB) {
          printf("c UB improved in minimize %lu -> %lu\n", UB, w);
          updateUB(w, solver);
        }
        // need to add more lits 
        solver->unsetBvar(lits[i]);
        continue;
      } else { // unsat
        if (i == 0) { // mus vector is unsat
          is_mus = true;
          break;
        } else { // lits[i - 1] is critical
          mus.push_back(lits[i - 1]);
          lits.clear();

          for (int l : subcore)
            if (find(mus.begin(), mus.end(), l) == mus.end())
              lits.push_back(l);
          
          if (lits.size() == 0) is_mus = true;
          break;
        }
      }
    }
  }

  core.swap(mus);

}

void ProblemInstance::binarySearchMinimize(MinisatSolver * solver, vector<int>& core) {
  log(3, "c ProblemInstance::binarySearchMinimize (size %lu)\n", core.size());
  vector<int> mus;
  vector<int> lits(core);
  random_shuffle(lits.begin(), lits.end());
  vector<int> subcore;

  //int s = core.size();

  while( true ) {
    solver->setBvars();
    for (int b : mus)
      solver->unsetBvar(b);

    int start = 0;
    int end = lits.size() - 1;
    int mid = 0;

    // use binary search to find critical literal
    while ( start != end ) {
      mid = (start + end) / 2;
      solver->setBvars();
      for (int l : mus) 
        solver->unsetBvar(l);
      for (int i = 0; i <= mid; ++i)
        solver->unsetBvar(lits[i]);

      solver->clearAssumptions();
      solver->assumeBvars();
      solver->findCore(subcore);
      bool sat = !subcore.size();

      if (sat) {
        weight_t w = getSolutionWeight(solver);
        if (w < UB) {
          printf("c UB improved in minimize %lu -> %lu\n", UB, w);
          updateUB(w, solver);
        }
        start = mid + 1;
      } else {
        end = mid;
      }
    }
    // lits[start] is critical
    mus.push_back(lits[start]);
    lits[start] = lits[lits.size() - 1];
    lits.pop_back();

    // loop until mus unsat
    solver->setBvars();
    for (auto l : mus) solver->unsetBvar(l);

    solver->clearAssumptions();
    solver->assumeBvars();  
    solver->findCore(subcore);
    if (subcore.size()) break;  
  }

  core.swap(mus);
}

// Minimize a core using a simple destructive algorithm
// for each s in core test if core \ {s} is a core, and updating if it is
void ProblemInstance::destructiveMinimize(MinisatSolver * solver, vector<int>& core) {
  log(3, "c ProblemInstance::destructiveMinimize (size %lu)\n", core.size());
  //unsigned origSize = core.size();
  //++minimizeCalls;

  // set only soft clauses in core active
  solver->setBvars();
  for (int b : core)
    solver->unsetBvar(b);

  vector<int> subcore;

  // sort core in order of descending weight
  sort(core.begin(), core.end(), [&](int b1, int b2){
    if (bvar_weights[b1] == bvar_weights[b2])
      return b1 > b2;
    return bvar_weights[b1] > bvar_weights[b2];
  });

  // for each clause core[i] in the core, check if core \ {core[i]}
  // is unsatisfiable. If so, remove core[i] from the core

  long propBudget = cfg.minimizePropLimit;
  long confBudget = cfg.minimizeConfLimit;
  bool limited = propBudget || confBudget;

  if (limited) solver->setBudgets(propBudget, confBudget);

  int calls = 0;

  for (unsigned i = 0; i < core.size(); i++) {

    if (++calls > cfg.minimizeLimit) break;


    int testClause = core[i];
    solver->setBvar(testClause);

    /*
    printf("c trying");
    for (unsigned j = 0; j < core.size(); ++j)
      if (i != j)
        printf(" %d", core[j]);
    printf("\n");
    */

    if (limited) {
      solver->clearAssumptions();
      solver->assumeBvars();
      // stop minimization on exceeding resource budgets
      if (!solver->findCoreLimited(subcore)) break;
    } else {
      solver->clearAssumptions();
      solver->assumeBvars();
      solver->findCore(subcore);
    }

    if (subcore.size() > 0) {

      core.swap(subcore);

      // sort subcore in order of descending weight
      sort(core.begin(), core.end(), [&](int b1, int b2){
        if (bvar_weights[b1] == bvar_weights[b2])
          return b1 > b2;
        return bvar_weights[b1] > bvar_weights[b2];
      });

      // core[i] and possibly some of core[i..n] removed
      // core[0..i-1] known to be critical
      --i;
    } else {
      weight_t w = getSolutionWeight(solver);
      if (w < UB) {
        printf("c UB improved in minimize %lu -> %lu\n", UB, w);
        updateUB(w, solver);
      }
      solver->unsetBvar(testClause);
    }
  }

  assert(core.size());

  //int litsRemoved = origSize - core.size();
  //if (litsRemoved) ++coresMinimized;
  //minimizedLits += litsRemoved;
}

void ProblemInstance::cardinalityMinimize(MinisatSolver * solver, vector<int>& core) {
  log(3, "c ProblemInstance::cardinalityMinimize (size %lu)\n", core.size());

  vector<int> subcore;
  vector<int> mus;
  vector<int> lits(core);

  // add |relaxed lits| <= 1 constraint

  vector<int> enc_vars = solver->addTempAtMostOneEncoding(lits);
  int card_id = enc_vars[0];
  for (int v : enc_vars) isOriginalVariable[v] = false;

  while (true) {
    // remove all assumptions
    solver->clearAssumptions();
    // activate cardinality constraint
    solver->assumeLit(-card_id);
    // block all soft clauses not in lits
    for (auto b_w : bvar_weights) {
      int b = b_w.first;
      if (find(lits.begin(), lits.end(), b) == lits.end() and 
          find(mus.begin(), mus.end(), b) == mus.end()) {
        solver->assumeLit(b);
      }
    }

    // unblock all clauses in (incomplete) mus 
    for (auto l : mus)
      solver->assumeLit(-l);

    subcore.clear();
    solver->findCore(subcore);

    if (subcore.size()) { // UNSAT

      // If cardinality constraint is reason for unsatisfiability
      // multiple muses exist

      if (find(subcore.begin(), subcore.end(), card_id) != subcore.end()
          && lits.size() > 1) {

        // now mus \union (lits - x)
        // is a core for all x \in lits

        lits.pop_back(); // TODO: could remove any l in lits to get a different mus

        solver->removeTempAtMostOneEncoding(card_id);

        enc_vars = solver->addTempAtMostOneEncoding(lits);

        card_id = enc_vars[0];
        for (int v : enc_vars) isOriginalVariable[v] = false;

        continue;
      }

      // Otherwise mus is unsat, and we are finished
      core = mus;
      break;
    } else { // SAT
      
      // Get model from SAT solver
      vector<bool> model;
      solver->getModel(model);

      // Find which l \in lits was relaxed to satisfy cardinality constraint
      for (unsigned i = 0; i < lits.size(); ++i) {
        if (model[lits[i]]) {
          // this l is a transition clause
          // add it to mus and remove it from lits
          mus.push_back(lits[i]);
          lits[i] = lits[lits.size() - 1];
          lits.pop_back();  
          break;
        }
      } 

      // update ub 
      weight_t w = getSolutionWeight(solver);
      if (w < UB) {
        printf("c UB improved in minimize %lu -> %lu\n", UB, w);
        updateUB(w, solver);
      }
    }
  }

  solver->removeTempAtMostOneEncoding(card_id);
}

// Try to reduce a core by re-refuting it
// Effective for some instances, relatively low cost
void ProblemInstance::reRefuteCore(MinisatSolver * solver, vector<int>& core) {
  log(3, "c ProblemInstance::reRefuteCore (size %lu)\n", core.size());
  //++rerefuteCalls;
  unsigned origSize = core.size();
  unsigned prevSize = origSize;

  do {
    // depending on options, reset SAT learnt clauses and invert variable
    // activity between re-refutations
    if (cfg.doResetClauses) solver->deleteLearnts();
    if (cfg.doInvertActivity) solver->invertActivity();

    prevSize = core.size();

    // set SAT assumptions so only core clauses are active
    solver->setBvars();
    for (int b : core) solver->unsetBvar(b);

    core.clear();
    // Hope for a smaller core
    solver->clearAssumptions();
    solver->assumeBvars();
    solver->findCore(core);
  
  } while (prevSize > core.size());

  //int litsRemoved = origSize - core.size();
  //if (litsRemoved) ++coresRerefuted;
  //rerefutedLits += litsRemoved;
}

void ProblemInstance::updateLB(weight_t w) {
  assert (w <= UB);
  if (w > LB) {
    LB = w;
    if (cfg.printBounds) {
      out << "c LB " << LB << "\t(" << solve_timer.cpu_ms_total() << " ms)" << endl;
    }
  }
}

void ProblemInstance::updateUB(weight_t w) {
  updateUB(w, sat_solver);
}

void ProblemInstance::updateUB(weight_t w, MinisatSolver * solver) {
  assert (w >= LB);
  if (w < UB) {
    UB = w;

    solver->getModel(UB_bool_solution);
    tightenModel(UB_bool_solution);

    getSolution(UB_solution, solver); // remember best solution

    if (cfg.printBounds) {
      out << "c UB " << UB << "\t(" << solve_timer.cpu_ms_total() << " ms)" << endl;
    }

    if (cfg.printSolutions) {
      printSolution(out);
    }
  }
}

void ProblemInstance::printSolution(ostream & model_out) {

  if (cfg.solveAsMIP || (sat_solver && sat_solver->hasModel) || UB_solution.size()) {
    model_out << "v ";
    if (cfg.preprocess) {
      vector<int> UB_copy(UB_solution);
      for (unsigned i = 0; i < UB_copy.size(); ++i)
        if (flippedInternalVarPolarity[abs(UB_copy[i])])
          UB_copy[i] *= -1;
      model_out << reconstruct(UB_copy);
    } else {
      for (int i : UB_solution)
        if (isOriginalVariable[abs(i)])
          model_out << " " << (flippedInternalVarPolarity[abs(i)] ? -i : i);
    }
    model_out << endl;
    model_out << "o " << UB << endl;
    if (UB == LB)
      model_out << "s OPTIMUM FOUND" << endl;  
    else
      model_out << "s UNKNOWN" << endl;  
  } else {
    model_out << "s UNSATISFIABLE" << endl;  
  }
  model_out.flush();
}
