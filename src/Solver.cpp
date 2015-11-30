#include "Solver.h"
#include "Util.h"
#include "NonoptHS.h"

#include <ostream>
#include <fstream>
#include <algorithm>
#include <functional>
#include <thread>
#include <cstdio>
#include <cstdlib>

#include <errno.h>
#include <math.h>  // ceil
#include <assert.h>

#if defined(SAT_MINISAT)
#include "MinisatSolver.h"
typedef MinisatSolver SATSolver;
#elif defined(SAT_LINGELING)
#include "LingelingSolver.h"
typedef LingelingSolver SATSolver;
#else
#error "No known SAT solver defined"
#endif

#if defined(MIP_CPLEX)
#include "CPLEXSolver.h"
typedef CPLEXSolver MIPSolver;
#elif defined(MIP_SCIP)
#include "SCIPSolver.h"
typedef SCIPSolver MIPSolver;
#else
#error "No known MIP solver defined"
#endif

using namespace std;

Solver::Solver(ProblemInstance &pi)
    : totalTime(0),
      solveTime(0),
      disjointTime(0),
      cfg(GlobalConfig::get()),
      instance(pi),
      newInstance(true),
      nSolutions(0),
      nLabelCores(0),
      nNonoptCores(0),
      nEquivConstraints(0),
      nDisjointCores(0),
      relaxLB(0),
      LB(0),
      UB(0),
      sat_solver(nullptr),
      mip_solver(nullptr) 
{

  if (!cfg.initialized) {
    ofstream nullstream("/dev/null");
    cfg.parseArgs(0, nullptr, nullstream);
  }

  if (!cfg.solveAsMIP) 
    sat_solver = new SATSolver();
  mip_solver = new MIPSolver();

  if (!cfg.solveAsMIP) {
    instance.attach(sat_solver);
    instance.attach(mip_solver);
  }
}

Solver::~Solver() {
  delete mip_solver;
  delete sat_solver;
}

void Solver::solve(ostream & out) {

  vector<int> solution;
  double weight;

  if (!cfg.solveAsMIP && !hardClausesSatisfiable()) {
    log(0, "s UNSATISFIABLE\n");
    fflush(stdout);
    return;
  }

  if (cfg.solveAsMIP) 
    weight = solveAsMIP(solution);
  else if (cfg.doCGMR)
    weight = solveCoreGuidedMaxRes(solution);
  else if (cfg.doEnumeration) {
    double optWeight = -1;
    do {
      weight = solveMaxHS(solution);
      if (solution.empty()) break;

      double init_UB = 0;
      for (auto & b_w : instance.bvar_weights) init_UB += b_w.second;
      UB = init_UB;

      if (cfg.enumerationType < 0)
        instance.forbidCurrentMIPSol();
      instance.forbidCurrentModel();

      if (optWeight == -1)
        optWeight = weight;
      else if (abs(cfg.enumerationType) == 1 && (weight - optWeight) > EPS)
        break;  
      
      ++nSolutions;
      opt_model.swap(solution);
      opt_weight = weight;
      printSolution(out);
    } while (nSolutions < cfg.enumerationLimit);
  } else {
    weight = solveMaxHS(solution);
  }

  if (!cfg.doEnumeration) {
    opt_model.swap(solution);
    opt_weight = weight;
    ++nSolutions;
  }
}

double Solver::solveCoreGuidedMaxRes(vector<int> &out_solution) {

  vector<int> empty_hs;
  vector<int> new_core;
  unordered_map<int, int> dummy;

  function<double (vector<int> &) > core_minWeight = [&](vector<int> & c) -> double {
    double mw = DBL_MAX;
    for (int i : c) mw = min(mw, instance.bvar_weights[i]);
    return mw;
  };

  function<int (vector<int> &) > core_minWeightCount = [&](vector<int> & c) -> int {
    double mw = DBL_MAX;
    int minCt = 0;
    for (int i : c) mw = min(mw, instance.bvar_weights[i]);
    for (int i : c) 
      if (abs(instance.bvar_weights[i] - mw) < EPS)
        ++minCt;
    return minCt;
  };

  while (true) {

    if (cfg.CGMR_mode == 0) {
      getCore(empty_hs, new_core);
    } else { 
      if (cfg.CGMR_mode == 1) {
        //
        // find max number of heaviest soft clauses to include 
        // using binary search
        //
        vector<pair<double, int>> sorted_bv;
        for (auto & p : instance.bvar_weights) 
          sorted_bv.push_back(make_pair(p.second, p.first));
        sort(sorted_bv.begin(), sorted_bv.end());

        unsigned kmin = 0, kmid;
        unsigned kmax = sorted_bv.size();

        while (kmin < kmax) {
          kmid = (kmin + kmax) / 2;
          vector<int> deactivated_bvars;
          for (unsigned j = 0; j < kmid; ++j)
            deactivated_bvars.push_back(sorted_bv[j].second);

          getCore(deactivated_bvars, new_core);

          if (new_core.empty()) {
            // can add more clauses, deactivate fewer
            updateUB();
            kmax = kmid;
          } else {
            // unsat, deactivate more clauses
            kmin = kmid;
            if (kmin >= kmax - 1) break;
          }
        }
      } else {
        terminate(1, "Unknown CGMR_mode");
      }
    }

    if (new_core.empty()) break;

    cores.push_back(new_core); // for stats
    relaxLB += instance.applyMaxRes(new_core, dummy);
    LB = relaxLB;
    log(1, "c LB %-20f UB %-20f\n", LB, UB);

    if (UB - LB < EPS) {
      log(1, "c solved by LB == UB\n");
      out_solution.swap(UB_solution);
      goto eva_stop;
    }
  }
  instance.getSolution(out_solution);
eva_stop:
  return relaxLB;
}

void Solver::printSolution(ostream & out) {

  if (!opt_model.empty()) {
    out << "v";
    for (int i : opt_model)
      if (instance.isOriginalVariable[abs(i)])
        out << " " << (instance.flippedInternalVarPolarity[abs(i)] ? -i : i);
    out << endl;
    out << "s OPTIMUM FOUND" << endl;
    if (cfg.floatWeights) {
      out << "o " << opt_weight << endl;
    } else {
      unsigned long i_opt_weight = (unsigned long) round(opt_weight);
      out << "o " << i_opt_weight << endl;  
    }
  } else {
    out << "s UNSATISFIABLE" << endl;  
  }
  out.flush();
}

// check that a maxsat solution exists
bool Solver::hardClausesSatisfiable() {

  sat_solver->deactivateClauses();
  if (!sat_solver->solve()) {
    log(1, "c hard clauses unsatisfiable\n");
    return false;
  } else {
    log(1, "c hard clauses satisfiable\n");
    UB = instance.getSolutionWeight();
    return true;
  }
}

void Solver::presolve() {

  log(1, "c presolving\n");

  if (cfg.doDisjointPhase) {
    findDisjointCores(cores);
    // add disjoint core constraints to SAT and MIP instances
    for (auto & core : cores)
      mip_solver->addConstraint(core);
  }

  // seed MIP solver with "equiv-constraints"

  if (cfg.doEquivSeed) {

    vector<vector<int> > equivConstraints;

    instance.getBvarEquivConstraints(equivConstraints);
    nEquivConstraints = equivConstraints.size();
    log(1, "c Seeding MIP solver with %u equiv constraints.\n", nEquivConstraints);
    for (auto & eq : equivConstraints) {
      logCore(2, eq);
      mip_solver->addConstraint(eq);
    }
  }

  newInstance = false;
}

double Solver::solveAsMIP(vector<int>& out_solution) {
  double weight;

  int min_bVar = INT_MAX;
  for (auto b_w : instance.bvar_weights) min_bVar = min(min_bVar, b_w.first);
  for (int i = 1; i < min_bVar; ++i)
    mip_solver->addVariable(i);
  mip_solver->addObjectiveVariables(instance.bvar_weights);
  log(1, "c added MIP variables\n");
  for (auto cl : instance.clauses)
    mip_solver->addConstraint(*cl);
  log(1, "c added MIP constraints\n");
  bool ok = mip_solver->solveForModel(out_solution, weight);
  condTerminate(!ok, 1, "Solver::solveAsMIP : no MIP solution\n");
  return weight;
}

//
// Find a solution to the MAXSAT instance
// solution retured by reference
// return value is solution weight
//
double Solver::solveMaxHS(vector<int>& out_solution) {

  double weight = 0;

  //vector<int> core;
  vector<int> new_core;
  vector<int> hs;

  clock_t solStartTime = clock();

  out_solution.clear();

  // For the first run on an instance, perform initial 
  // steps of finding disjoint cores and seeding MIP
  // solver with equiv constraints

  if (newInstance) presolve();

  if (cores.empty()) {
    getCore(hs, new_core);
    if (!new_core.empty()) {
      cores.push_back(new_core);
      mip_solver->addConstraint(new_core);
    }
  }

  // main MaxHS loop
  for (;;) {

    weight = 0;
    hs.clear();
    if (!cores.empty()) {
      // find minimum cost hitting set
      mip_solver->solveForHS(hs, weight);
      if (cfg.logHS > 0) logHS(0, hs, true);
      if (hs.empty()) { // no MIP solution
        log(1, "c empty hitting set\n");
        out_solution.clear();
        weight = -1;
        break;
      }
    }
    
    getCore(hs, new_core);
    LB = relaxLB + weight;
    log(1, "c LB %-20f UB %-20f\n", LB, UB);
    fflush(stdout);

    // satisfiable with current assumptions -> found an optimal solution
    if (new_core.empty()) {
      instance.getSolution(out_solution);
      UB = instance.getSolutionWeight();
      break;
    }

    // add core to problem instances
    processCore(new_core);

    // reduce MIP solver calls by trying to find cores with non-optimal hitting
    // sets
    if (cfg.doNonOpt) {
      while (true) {
        while (true) {
          cfg.nonoptPrimary(hs, new_core, cores, instance.bvar_weights, coreClauseCounts);
          if (cfg.logHS > 1) logHS(0, hs, false);

          // try to find a core using the non-optimal hitting set
          if (!getCore(hs, new_core)) {
            updateUB();
            if (UB - LB < EPS) {
              log(1, "c solved by LB == UB\n");
              out_solution.swap(UB_solution);
              goto maxhs_stop;
            }
            break;
          }
          ++nNonoptCores;
          processCore(new_core);
        }
        // second nonopt stage exists?
        if (cfg.nonoptSecondary == nullptr) break;
        cfg.nonoptSecondary(hs, new_core, cores, instance.bvar_weights, coreClauseCounts);
        if (cfg.logHS > 1) logHS(0, hs, false);

        if (!getCore(hs, new_core)) {
          updateUB();
          if (UB - LB < EPS) {
            log(1, "c solved by LB == UB\n");
            out_solution.swap(UB_solution);
            goto maxhs_stop;
          }
          break;
        }

        ++nNonoptCores;
        processCore(new_core);
      }
    }
  } // end main MaxHS loop

maxhs_stop:
  solveTime = clock() - solStartTime;
  return relaxLB + weight;
}

void Solver::processCore(vector<int> &core) {

  condTerminate(core.empty(), 1, "Error: attempting to process empty core.\n");
  cores.push_back(core);
  mip_solver->addConstraint(core);
}

//
// Print stats for MAXSAT solver and its SAT and MIP solver components
//
void Solver::printStats() {

  log(1, "c Solutions found: %d\n", nSolutions);
  if (cfg.solveAsMIP) return;

  condLog(cfg.doMinimizeCores, 1, "c Minimization:\n");
  condLog(cfg.doMinimizeCores, 1, "c   calls:      %u\n", sat_solver->minimizeCalls);
  condLog(cfg.doMinimizeCores, 1, "c   minimized:  %u cores\n", sat_solver->coresMinimized);
  condLog(cfg.doRerefuteCores, 1, "c Rerefutation:\n");
  condLog(cfg.doRerefuteCores, 1, "c   calls:      %u\n", sat_solver->rerefuteCalls);
  condLog(cfg.doRerefuteCores, 1, "c   rerefuted:  %u cores\n", sat_solver->coresRerefuted);

  if (mip_solver) mip_solver->printStats();
  if (sat_solver) sat_solver->printStats();
  instance.printStats();
  log(1, "c Cores:\n");
  log(1, "c   total cores:  %lu\n", coreSizes.size());
  condLog(cfg.doDisjointPhase, 1, "c   disjoints:    %d\n", nDisjointCores);
  condLog(cfg.doNonOpt, 1, "c   from nonopt:  %d\n", nNonoptCores);
  condLog(cfg.doEquivSeed, 1, "c   eq-constr:    %d\n", nEquivConstraints);

  unsigned totalSize = 0;
  for (unsigned s : coreSizes) totalSize += s;
  log(1, "c   total size: %d clauses\n", totalSize);
  log(1, "c   avg size:   %.2f clauses\n", totalSize == 0 ? 0 :
         ((double)totalSize) / ((double)coreSizes.size()));

  log(1, "c Time:\n");
  log(1, "c   disjoint phase     %.2fs\n", SECONDS(disjointTime));
  log(1, "c   file parsing       %.2fs\n", SECONDS(instance.parseTime));
}

//
// Set the SAT instance assumptions so that the soft clauses
// indicated by hs are deactivated, and all other soft clauses
// are activated.
//
void Solver::setAssumptions(vector<int>& hs) {
  sat_solver->activateClauses();
  for (int c : hs)
    sat_solver->deactivateClause(c);
}

// Get a core from the SAT solver
// using whatever assumptions have been set
// return false if instance is satisfiable
bool Solver::getCore(vector<int>& hs, vector<int>& core) {

  setAssumptions(hs);
  sat_solver->findCore(core);
  if (core.empty()) return false;

  if (cfg.doRerefuteCores) 
    sat_solver->reRefuteCore(core);
  if (cfg.doMinimizeCores && core.size() < cfg.minimizeSizeLimit)
    sat_solver->minimizeCore(core);
  
  coreSizes.push_back(core.size());
  for (int b : core) coreClauseCounts[b]++;

  if (cfg.doResetClauses) sat_solver->deleteLearnts();
  if (cfg.doInvertActivity) sat_solver->invertActivity();

  return true;
}

// finds initial set of disjoint cores
void Solver::findDisjointCores(vector<vector<int> >& disjointCores) {

  disjointStart = clock();

  vector<int> disjoint_clauses;
  vector<int> core;
  double cost = 0;

  // loop until a new core isn't found
  for (;;) {

    // deactivates all soft clauses in the current set of disjoint cores
    // (getCore sets own assumptions so these need to be reset at each
    // iteration)
    sat_solver->activateClauses();
    for (auto & dc : disjointCores)
      for (int b : dc)
        sat_solver->deactivateClause(b);

    // break when no more disjoint cores
    if (!getCore(disjoint_clauses, core)) break;
    disjointCores.push_back(core);
    nDisjointCores += 1;

    for (int b : core) disjoint_clauses.push_back(b);

    double minCost = DBL_MAX;
    for (int b : core)
      minCost = min(minCost, instance.bvar_weights[b]);
    cost += minCost;
    log(1, "c LB %-20f UB %-20f\n", relaxLB + cost, UB);
  }

  UB = relaxLB + instance.getSolutionWeight();
  log(1, "c LB %-20f UB %-20f\n", relaxLB + cost, UB);


  log(1, "c Found %ld disjoint cores (%d minimized).\n",
           disjointCores.size(), sat_solver->coresMinimized);
  

  disjointTime = clock() - disjointStart;
}

void Solver::updateUB() {
  double w = instance.getSolutionWeight();
  if (relaxLB + w < UB) {
    UB = relaxLB + w;
    instance.getSolution(UB_solution);
  }
}
