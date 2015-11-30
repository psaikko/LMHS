#include "ProblemInstance.h"
#include "WCNFParser.h"
#include <istream>
#include <algorithm>
#include <zlib.h>

using namespace std;

void ProblemInstance::attach(ISATSolver* s) {
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

  for (auto cl : clauses) sat_solver->addConstraint(*cl);
}

void ProblemInstance::attach(IMIPSolver* s) {
  mip_solver = s;
  mip_solver->addObjectiveVariables(bvar_weights);
}

void ProblemInstance::init() {
  parseTime = 0;
  max_var = 0;
  sat_solver = nullptr;
  mip_solver = nullptr;
}

ProblemInstance::ProblemInstance()
    : cfg(GlobalConfig::get())
{
  init();
}

ProblemInstance::ProblemInstance(unsigned nClauses, double top, double* weights,
                                 int* raw_clauses)
    : cfg(GlobalConfig::get())
{
  init();

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

ProblemInstance::ProblemInstance(std::istream& wcnf_in)
    : cfg(GlobalConfig::get())
{

  clock_t start = clock();

  init();

  condTerminate(wcnf_in.fail(), 1,
                "Error: Bad input stream (check filepath)\n");

  double top;
  vector<double> weights;
  vector<int> file_assumptions;
  vector<vector<int>> tmp_clauses;
  parseWCNF(wcnf_in, weights, branchVars, top, tmp_clauses, file_assumptions);

  for (auto& cl : tmp_clauses)
    for (int i : cl) max_var = max(abs(i), max_var);

  if (cfg.isLCNF) {
    //
    // LCNF mode
    //
    for (unsigned i = 0; i < file_assumptions.size(); ++i) {
      int a = file_assumptions[i];
      if (a > 0) {
        flippedInternalVarPolarity[a] = true;
        file_assumptions[i] *= -1;
      }
    }
    for (auto& cl : tmp_clauses)
      for (unsigned i = 0; i < cl.size(); ++i)
        if (flippedInternalVarPolarity[abs(cl[i])]) cl[i] *= -1;
    for (unsigned i = 0; i < tmp_clauses.size(); ++i) {
      if (weights[i] < top) {
        if (tmp_clauses[i].size() == 1 &&
            count(file_assumptions.begin(), file_assumptions.end(),
                  tmp_clauses[i][0]) > 0) {
          int bv = -tmp_clauses[i][0];
          bvar_weights[bv] = weights[i];
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
          if (v > 0 && bvar_weights.count(v) == 1) {
            // add as soft clause using existing variables
            addSoftClauseWithExistingBvar(tmp_clauses[i]);
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

  parseTime = (clock() - start);
}

ProblemInstance::~ProblemInstance() {
  for (auto p : clauses) delete p;
  for (auto e : bvar_soft_clauses)
    for (auto p : e.second) delete p;
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

  for (int v : hc) {
    max_var = max(abs(v), max_var);
    if (sat_solver != nullptr && max_var >= sat_solver->nVars())
      sat_solver->addVariable(max_var);
  }

  auto clause = new vector<int>(hc);

  if (original)
    for (int l : hc) isOriginalVariable[abs(l)] = true;

  hard_clauses.push_back(clause);
  clauses.push_back(clause);

  if (sat_solver != nullptr) sat_solver->addConstraint(hc);
}

// add a soft clause to the SAT instance
int ProblemInstance::addSoftClause(vector<int>& sc, double weight,
                                   bool original) {

  for (int v : sc) {
    max_var = max(abs(v), max_var);
    if (sat_solver != nullptr)
      while (max_var >= sat_solver->nVars()) sat_solver->addVariable(max_var);
  }

  auto clause = new vector<int>(sc);

  if (original)
    for (int l : *clause) isOriginalVariable[abs(l)] = true;

  int bVar = ++max_var;
  if (sat_solver != nullptr) sat_solver->addVariable(bVar);
  if (mip_solver != nullptr) mip_solver->addObjectiveVariable(bVar, weight);

  updateBvarMap(bVar, clause);

  bvar_clause_ct[bVar]++;
  bvar_weights[bVar] = weight;

  auto b_clause = new vector<int>(sc);

  (*b_clause).push_back(bVar);
  clauses.push_back(b_clause);
  soft_clauses.push_back(b_clause);

  if (sat_solver != nullptr) {
    sat_solver->addConstraint(*b_clause);
    sat_solver->addBvarAssumption(bVar);
  }

  return bVar;
}

// add a soft clause to the SAT instance with existing bvar(s)
void ProblemInstance::addSoftClauseWithExistingBvar(vector<int>& sc_,
                                                    bool original) 
{

  for (int v : sc_) max_var = max(abs(v), max_var);
  auto sc = new vector<int>(sc_);

  if (original)
    for (int l : *sc) isOriginalVariable[abs(l)] = true;

  for (int v : *sc) {
    if (bvar_weights.count(v) == 1) {  // v is a bvar
      auto cl = new vector<int>(sc->size() - 1);
      copy_if(sc->begin(), sc->end(), cl->begin(),
              [&](int l) { return l != v; });
      bvar_clause_ct[v]++;

      updateBvarMap(v, cl);
    }
  }

  clauses.push_back(sc);
  soft_clauses.push_back(sc);

  if (sat_solver != nullptr) sat_solver->addConstraint(*sc);
}

void ProblemInstance::updateBvarMap(int bvar, std::vector<int>* sc) {
  if (bvar_soft_clauses.count(bvar) == 1) {
    bvar_soft_clauses[bvar].push_back(sc);
  } else {
    bvar_soft_clauses[bvar] = {sc};
  }
}

void ProblemInstance::printStats() {
  if (!sat_solver) return;

  log(1, "c Instance:\n");
  log(1, "c   variables (final): %d\n", sat_solver->nVars());
  log(1, "c   clauses (final):   %lu\n", clauses.size());
  log(1, "c   hard clauses:      %lu\n", hard_clauses.size());
  log(1, "c   soft clauses:      %lu\n", soft_clauses.size());
}

void ProblemInstance::getSolution(std::vector<int>& out_solution) {

  condTerminate(sat_solver == nullptr, 1,
                "Error: no SAT solver attached to ProblemInstance\n");

  out_solution.clear();
  vector<bool> model;
  sat_solver->getModel(model);

  condTerminate(model.size() == 0, 1, "Error: no model given by SAT solver\n");

  for (unsigned i = 0; i < model.size(); ++i) {
    if (isOriginalVariable[i]) {
      out_solution.push_back(model[i] ? i : -i);
    }
  }
}

double ProblemInstance::getSolutionWeight() {

  condTerminate(sat_solver == nullptr, 1,
                "Error: no SAT solver attached to ProblemInstance\n");

  return sat_solver->getModelWeight(bvar_weights);
}

double ProblemInstance::totalWeight() {
  double sum = 0;
  for (auto v_w : bvar_weights) sum += v_w.second;
  return sum;
}

// find equiv constraints in SAT instance with b-variables
void ProblemInstance::getBvarEquivConstraints(
    vector<vector<int>>& out_constraints) {

  unordered_map<int, int> eqs;

  for (auto& p : bvar_soft_clauses) {
    int bvar = p.first;
    eqs[bvar] = bvar;
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
      constr.push_back(eqs[abs(l)] * s);
    }

    if (constr.size() == 2 && constr[0] == -constr[1]) goto bv_eq_next_clause;

    out_constraints.push_back(constr);

  bv_eq_next_clause:
    continue;
  }
}

void ProblemInstance::getBvarHyperedges(vector<vector<int>>& out_hyperedges,
                                        vector<int>& out_vertices) {
  condTerminate(
      cfg.isLCNF, 1,
      "Error: getBvarHyperedges not functional with lcnf mode\n");  // TODO; fix
                                                                    // this

  out_vertices.clear();
  out_hyperedges.clear();
  unordered_map<int, vector<int>> hedges;
  unordered_map<int, int> hverts;

  int vi = 1;
  for (auto p : bvar_soft_clauses) {
    int bvar = p.first;
    vector<int> sc = *(p.second[0]);
    for (int l : sc) {
      int v = abs(l);
      if (hedges.find(v) == hedges.end()) hedges[v] = vector<int>();
      hedges[v].push_back(vi);
    }
    out_vertices.push_back(bvar);
    ++vi;
  }

  for (auto entry : hedges) out_hyperedges.push_back(entry.second);
}

void ProblemInstance::getLabelOnlyClauses(
    std::vector<std::vector<int>>& label_clauses) {

  label_clauses.clear();
  std::function<bool(vector<int>*)> isLabelClause = [this](vector<int>* v) {
    return all_of(v->begin(), v->end(),
                  [this](int i) { return bvar_weights.count(i) == 1; });
  };
  for (auto sc : soft_clauses)
    if (isLabelClause(sc)) label_clauses.push_back(*sc);
}

// Algorithm 3 of "Maximum Satisfiability Using Core-Guided MAXSAT Resolution"
// (Narodytska, Bacchus), using the compressed compensation clauses and adapted
// for weighted maxsat
double ProblemInstance::applyMaxRes(vector<int>& core,
                                    unordered_map<int, int>& bvar_replace) {
  condTerminate(sat_solver == nullptr, 1,
                "Error: no SAT solver attached to ProblemInstance\n");

  unsigned p = core.size();
  bvar_replace.clear();
  // find minimum weight among clauses in the core
  double minw = bvar_weights[core[0]];
  for (unsigned i = 1; i < p; ++i) minw = min(bvar_weights[core[i]], minw);
  // make soft clauses of core hard
  for (int bv : core) {
    double sc_weight = bvar_weights[bv];
    bvar_weights.erase(bv);

    // remove assumption variable
    // effectively makes (sc v bv) hard
    sat_solver->removeBvarAssumption(bv);

    // create new bvars with 'leftover' weight
    if (abs(sc_weight - minw) > EPS) {

      int new_bv = sat_solver->newVar();
      bvar_weights[new_bv] = sc_weight - minw;
      sat_solver->addBvarAssumption(new_bv);
      bvar_replace[bv] = new_bv;

      // efficient clause copy if not LCNF mode
      if (!cfg.isLCNF) {
        vector<int> new_sc = *(bvar_soft_clauses[bv][0]);
        new_sc.push_back(new_bv);
        addSoftClauseWithExistingBvar(new_sc, false);
      }
    }

    for (auto sc : bvar_soft_clauses[bv]) delete sc;
    bvar_soft_clauses.erase(bv);
  }

  // separate clause copy routine for LCNF mode
  if (cfg.isLCNF) {
    vector<vector<int>*> todo;
    for (auto sc : soft_clauses) {
      for (int l : *sc) {
        if (bvar_replace.count(l) == 1) {
          todo.push_back(sc);
          break;
        }
      }
    }
    for (auto sc : todo) {
      vector<int> copy_clause(*sc);
      for (unsigned i = 0; i < copy_clause.size(); ++i) {
        if (bvar_replace.count(copy_clause[i]) == 1)
          copy_clause[i] = bvar_replace[copy_clause[i]];
      }
      addSoftClauseWithExistingBvar(copy_clause, false);
    }
  }

  // create new variables for cardinality constraint
  vector<int> dvars;
  for (unsigned i = 0; i < p - 1; ++i) dvars.push_back(sat_solver->newVar());

  // add core as hard clause
  addHardClause(core, false);

  // temp vector for adding new clauses
  vector<int> cl;

  // add hard clauses for compressed maxres
  if (p > 2)
    for (unsigned i = 0; i < p - 2; ++i) {
      // d_i -> (b_{i+1} v d_{i+1})
      cl = {-dvars[i], dvars[i + 1], core[i + 1]};
      addHardClause(cl, false);

      // (b_{i+1} v d_{i+1}) -> d_i
      // d_i v -b_{i+1}
      cl = {dvars[i], -core[i + 1]};
      addHardClause(cl, false);

      // d_i v -d_{i+1}
      cl = {dvars[i], -dvars[i + 1]};
      addHardClause(cl, false);
    }

  if (p > 1) {
    // handle i = p - 1 case
    cl = {dvars[p - 2], -core[p - 1]};
    addHardClause(cl, false);

    cl = {-dvars[p - 2], core[p - 1]};
    addHardClause(cl, false);
  }

  // add soft clauses for maxres
  for (unsigned i = 0; i < p - 1; ++i) {
    // (-b_i v -d_i, w_i)
    cl = {-core[i], -dvars[i]};
    addSoftClause(cl, minw, false);
  }

  // old cores invalidated, reset mip solver
  mip_solver->reset();
  mip_solver->addObjectiveVariables(bvar_weights);

  return minw;
}