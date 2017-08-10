#include <iostream>
#include <fstream>
#include <algorithm>
#include <functional>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <math.h>  // ceil
#include <assert.h>

#include "Solver.h"
#include "Util.h"
#include "NonoptHS.h"

#include "Timer.h"
#include "MinisatSolver.h"
#include "CPLEXSolver.h"

using namespace std;

Solver::Solver(ProblemInstance &pi, ostream& out)
    : cfg(GlobalConfig::get()),
      instance(pi),
      newInstance(true),
      nSolutions(0),
      nNonoptCores(0),
      nEquivConstraints(0),
      nDisjointCores(0),
      out(out)
{

  if (!cfg.initialized) {
    ofstream nullstream("/dev/null");
    cfg.parseArgs(0, nullptr, nullstream);
  }

  instance.attach(new CPLEXSolver());
  if (!cfg.solveAsMIP) {
    instance.attach(new MinisatSolver());
    if (cfg.separate_muser) {
      instance.attachMuser(new MinisatSolver());
    }
  }
}

void Solver::solve() {
  log(3, "c Solver::solve\n");
  
  instance.solve_timer.start();

  weight_t weight = -1;

  //instance.printStats();

  if (!cfg.solveAsMIP && !hardClausesSatisfiable()) {
    //log(0, "s UNSATISFIABLE\n");
    //fflush(stdout);
    return;
  }

  if (cfg.solveAsMIP) {
    solveAsMIP();
  }
  else if (cfg.doEnumeration) {
    weight_t optWeight = 0;
    bool optFound = false;

    weight_t init_UB = 0;
    for (auto & b_w : instance.bvar_weights) 
      init_UB += b_w.second;
    
    do {
      solveMaxHS();
      weight = instance.UB;
      if (instance.UB_solution.empty()) break;

      if (cfg.enumerationType < 0)
        instance.forbidCurrentMIPSol();
      instance.forbidCurrentModel();

      if (!optFound) {
        optWeight = instance.UB;
        optFound = true;
      } else if (abs(cfg.enumerationType) == 1 && (weight - optWeight) > EPS) {
        break;  
      }
      
      // reset upper bound
      instance.UB = init_UB;

      ++nSolutions;
      instance.printSolution(cout);

    } while (nSolutions < cfg.enumerationLimit);
  } else {
    solveMaxHS();
    assert(instance.LB == instance.UB);
    ++nSolutions;
  }
}

// check that a maxsat solution exists
bool Solver::hardClausesSatisfiable() {

  instance.sat_solver->setBvars();
  instance.sat_solver->clearAssumptions();
  instance.sat_solver->assumeBvars();
  if (!instance.sat_solver->solve()) {
    log(1, "c hard clauses unsatisfiable\n");
    return false;
  } else {

    log(1, "c hard clauses satisfiable\n");
    instance.updateUB(instance.getSolutionWeight(instance.sat_solver));
    return true;
  }
}

void Solver::presolve() {
  log(3, "c Solver::presolve\n");

  if (cfg.doDisjointPhase) 
    findDisjointCores();

  // seed MIP solver with "equiv-constraints"

  if (cfg.doEquivSeed) {

    vector<vector<int> > equivConstraints;

    instance.getBvarEquivConstraints(equivConstraints);
    nEquivConstraints = equivConstraints.size();
    log(1, "c Seeding MIP solver with %u equiv constraints.\n", nEquivConstraints);
    for (auto & eq : equivConstraints) {
      logCore(2, eq);
      instance.mip_solver->addConstraint(eq);
    }
  }

  newInstance = false;
}

void Solver::solveAsMIP() {
  log(3, "c Solver::solveAsMIP\n");
  weight_t weight;

  for (int i = 1; i < instance.max_var; ++i)
    if (!instance.bvar_weights.count(i))
      instance.mip_solver->addVariable(i);

  instance.mip_solver->addObjectiveVariables(instance.bvar_weights);

  log(1, "c added MIP variables\n");
  for (auto cl : instance.clauses)
    instance.mip_solver->addConstraint(*cl);

  log(1, "c added MIP constraints\n");
  bool ok = instance.mip_solver->solveForModel(instance.UB_solution, weight);
  condTerminate(!ok, 1, "c Solver::solveAsMIP : no MIP solution\n");
  //instance.updateUB(weight);
  instance.UB = weight;
  instance.LB = weight;
}

//
// Find a solution to the MAXSAT instance
// solution retured by reference
// return value is solution weight
//
void Solver::solveMaxHS() {
  log(3, "c Solver::solveMaxHS\n");

  //vector<int> core;
  vector<vector<int>> new_cores;
  vector<int> hs;

  // For the first run on an instance, perform initial 
  // steps of finding disjoint cores and seeding MIP
  // solver with equiv constraints

  if (newInstance) presolve();

    // main MaxHS loop
    for (unsigned iteration = 0;;++iteration) {

      hs.clear();

      // find minimum cost hitting set
      weight_t opt_lb;
      CPLEXSolver::Status status = instance.mip_solver->solveForHS(hs, opt_lb, &instance);

      while (!instance.fixQueue.empty()) {
        int fixed = instance.fixQueue.back();
        instance.fixQueue.pop_back();

        coreClauseCounts.erase(fixed);

        for (auto & core : cores) {
          core.erase(std::remove(core.begin(), core.end(), fixed), core.end());
          if (core.size() == 0)
            terminate(1, "Empty core in core set pruning");
        }
      }

      while (!instance.relaxQueue.empty()) {
        int relaxed = instance.relaxQueue.back();
        instance.relaxQueue.pop_back();

        for (auto & core : cores) {
          if (std::find(core.begin(), core.end(), relaxed) != core.end()) {
            for (int l : core) {
              coreClauseCounts[l]--;
            }
          } 
        }

        cores.erase(std::remove_if(cores.begin(), cores.end(), [&](vector<int> & x){
            return std::find(x.begin(), x.end(), relaxed) != x.end();
          }), cores.end());
      }

      if (status == CPLEXSolver::Status::Failed) { // no MIP solution
        instance.UB_solution.clear();
        log(1, "c empty hitting set\n");
        break;
      } else if (cfg.printHittingSets & PRINT_OPT_HS) {
        out << "c opt hs " << hs << endl;
      }

      log(1, "c CPLEX opt %" WGT_FMT "\n", opt_lb);

      if (status == CPLEXSolver::Status::Optimal) {
        instance.updateLB(opt_lb);
        if (instance.UB == instance.LB) {
          log(1, "c solved by LB == UB\n");
          goto maxhs_stop;
        }
      }
      
      getCores(hs, new_cores);
      
      // satisfiable with current assumptions -> found an optimal solution
      if (new_cores.empty()) {
        if (status == CPLEXSolver::Status::Optimal) {
          if (current_level == 0)
            instance.updateLB(instance.UB);
          break;
        } else {
          if (instance.UB == instance.LB) {
            log(1, "c solved by LB == UB\n");
            goto maxhs_stop;
          }
        }
      }

      // reduce MIP solver calls by trying to find cores with non-optimal hitting
      // sets
      int nonOpts = 0;
      if (cfg.lpNonOpt) {
        nonopt_timer.start();
        while (true) {
          weight_t lb;
          instance.mip_solver->LPsolveHS(hs, lb);
          if (cfg.printHittingSets & PRINT_NONOPT_HS) {
            out << "c nonopt (lp) hs " << hs << endl;
          }
          instance.updateLB(lb);

          if (!getCores(hs, new_cores)) {
            if (instance.UB == instance.LB) {
              log(1, "c solved by LB == UB\n");
              nonopt_timer.stop();
              goto maxhs_stop;
            }
            nonopt_timer.stop();
            break;
          }
          nNonoptCores += 1;
        }
      } else if (cfg.nonoptPrimary) {
        nonopt_timer.start();
        while (true) {
          while (true) {
            cfg.nonoptPrimary(hs, new_cores, cores, instance.bvar_weights, coreClauseCounts);
            if (cfg.printHittingSets & PRINT_NONOPT_HS) {
              out << "c nonopt (1) hs " << hs << endl;
            }

            // try to find a core using the non-optimal hitting set
            if (!getCores(hs, new_cores)) {
              if (instance.UB == instance.LB) {
                log(1, "c solved by LB == UB\n");
                nonopt_timer.stop();
                goto maxhs_stop;
              }
              break;
            }
            nNonoptCores += 1;
            nonOpts += 1;
            if (nonOpts >= cfg.nonoptLimit) goto nonopt_stop;
          }
          // second nonopt stage exists?
          if (not cfg.nonoptSecondary) break;
          cfg.nonoptSecondary(hs, new_cores, cores, instance.bvar_weights, coreClauseCounts);
          if (cfg.printHittingSets & PRINT_NONOPT_HS) {
            out << "c nonopt (2) hs " << hs << endl;
          }

          if (!getCores(hs, new_cores)) {
            if (instance.UB == instance.LB) {
              log(1, "c solved by LB == UB\n");
              nonopt_timer.stop();
              goto maxhs_stop;
            }
            break;
          }

          nNonoptCores += 1;
          nonOpts += 1;
          if (nonOpts >= cfg.nonoptLimit) goto nonopt_stop;
        }
      }
      nonopt_stop: nonopt_timer.stop();

      log(1, "c iteration %d: %d cores\n", iteration, nonOpts+1);
      continue;
    } // end main MaxHS loop

  maxhs_stop:
    
  if (cfg.MIP_modelFile != "") {
    instance.mip_solver->exportModel(cfg.MIP_modelFile);
  }

  instance.solve_timer.stop();
}

void Solver::processCore(vector<int> &core) {
  static int processed = 0;
  ++ processed;
  condTerminate(core.empty(), 1, "Error: attempting to process empty core.\n");
  cores.push_back(core);

  if (cfg.printCores) {
    out << "c core " << core << endl;
  }

  instance.mip_solver->addConstraint(core);
  coreSizes.push_back(core.size());
  for (int b : core) {
    coreClauseCounts[b]++;
    assert(b > 0);
  } 
}

//
// Print stats for MAXSAT solver and its SAT and MIP solver components
//
void Solver::printStats() {

  log(0, "c Solutions found: %d\n", nSolutions);
  if (cfg.solveAsMIP) return;
  log(0, "c Nonopt time:        %lu ms\n", nonopt_timer.cpu_ms_total());
  if (instance.mip_solver) instance.mip_solver->printStats();
  if (instance.sat_solver) instance.sat_solver->printStats("Minisat");
  if (instance.muser) instance.muser->printStats("Muser");
  instance.printStats();
  log(0, "c Cores:\n");
  log(0, "c   total cores:  %lu\n", coreSizes.size());
  condLog(cfg.doDisjointPhase, 0, "c   disjoints:    %d\n", nDisjointCores);
  condLog(cfg.nonoptPrimary != nullptr,        0, "c   from nonopt:  %d\n", nNonoptCores);
  condLog(cfg.doEquivSeed,     0, "c   eq-constr:    %d\n", nEquivConstraints);

  unsigned totalSize = 0;
  for (unsigned s : coreSizes) totalSize += s;
  log(0, "c   total size: %d clauses\n", totalSize);
  log(0, "c   avg size:   %.2f clauses\n", totalSize == 0 ? 0 :
         ((double)totalSize) / ((double)coreSizes.size()));

  log(0, "c Time:\n");
  log(0, "c   disjoint phase     %lu ms\n", disjoint_timer.cpu_ms_total());
  log(0, "c   file parsing       %lu ms\n", instance.parse_timer.cpu_ms_total());

  cout.flush();
}


//
// Set the SAT instance assumptions so that the soft clauses
// indicated by hs are deactivated, and all other soft clauses
// are activated.
//
void Solver::setHSAssumptions(vector<int>& hs) {
  log(3, "c Solver::setHSAssumptions\n");
  
  instance.sat_solver->unsetBvars();
  for (int c : hs)
    instance.sat_solver->setBvar(c);
  instance.sat_solver->clearAssumptions();
  instance.sat_solver->assumeBvars();
}

// Get a core from the SAT solver
// using whatever assumptions have been set
// return false if instance is satisfiable
bool Solver::getCore(vector<int>& hs, vector<int>& core) {
  log(3, "c Solver::getCore\n");
  // todo: rewrite as probleminstance::getcore

  setHSAssumptions(hs);

  instance.sat_solver->findCore(core);

  if (core.empty()) {
    weight_t soln_weight = instance.getSolutionWeight(instance.sat_solver);
    log(2, "c getcore found soln w=%lu\n", soln_weight);
    return false;
  }
  log(3, "c Solver::getCore found core (size %lu)\n", core.size());

  if (cfg.doRerefuteCores) {
    instance.reduceCore(core, MinimizeAlgorithm::rerefute);
  }

  if (cfg.doMinimizeCores) {
    instance.reduceCore(core, cfg.minAlg);
  }

  processCore(core);

  if (cfg.doResetClauses) instance.sat_solver->deleteLearnts();
  if (cfg.doInvertActivity) instance.sat_solver->invertActivity();

  return true;
}

bool Solver::getCores(vector<int>& hs, vector<vector<int>>& cores) {
  log(3, "c Solver::getCores\n");

  // todo: rewrite as probleminstance::getcore
 
  setHSAssumptions(hs);
  instance.sat_solver->findCores(cores);

  if (cores.empty()) {
    instance.updateUB(instance.getSolutionWeight(instance.sat_solver));
    return false;
  }

  for (auto core : cores) {
    if (cfg.doRerefuteCores) {
      instance.reduceCore(core, MinimizeAlgorithm::rerefute);
    }

    if (cfg.doMinimizeCores) {
      instance.reduceCore(core, cfg.minAlg);
    }

    processCore(core);

    if (cfg.doResetClauses) instance.sat_solver->deleteLearnts();
    if (cfg.doInvertActivity) instance.sat_solver->invertActivity();
  }

  return true;
}

// finds initial set of disjoint cores
void Solver::findDisjointCores() {
  log(3, "c Solver::findDisjointCores\n");
  disjoint_timer.start();

  vector<int> disjoint_clauses;
  vector<int> core;
  weight_t cost = 0;

  // loop until a new core isn't found
  for (;;) {

    // break when no more disjoint cores
    if (!getCore(disjoint_clauses, core)) break;
    nDisjointCores += 1;

    for (int b : core) disjoint_clauses.push_back(b);

    weight_t minCost = WEIGHT_MAX;
    for (int b : core)
      minCost = min(minCost, instance.bvar_weights[b]);
    cost += minCost;
    instance.updateLB(cost);
  }

  cost = instance.getSolutionWeight(instance.sat_solver);
  instance.updateUB(cost);

  log(1, "c Found %ld disjoint cores\n", cores.size());
    //cores.size(), instance.sat_solver->coresMinimized);
  
  disjoint_timer.stop();
}