#include "GlobalConfig.h"
#include "Solver.h"
#include "CoprocessorInterface.h"
#include "Util.h"
#include "VarMapper.h"
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

using namespace std;

Solver * maxsat_solver;

void stop(int) {
  printf("s UNKNOWN\n");
  fflush(stdout);
  if (maxsat_solver) maxsat_solver->printStats();
  _Exit(1);
}

int main(int argc, const char* argv[]) {

  clock_t begin = clock();

  condTerminate(argc == 1, 1, "Error: need filepath.\n");

  signal(SIGINT, stop);
  signal(SIGTERM, stop);
  signal(SIGXCPU, stop);

  GlobalConfig & cfg = GlobalConfig::get();
  cfg.parseArgs(argc, argv, cout);

  cout << setprecision(cfg.streamPrecision);

  log(1, "c argv");
  for (int i = 0; i < argc; ++i)
    log(1, " %s", argv[i]);
  log(1, "\n");

#ifdef VERSION
  log(1, "c LMHS version " VERSION "\n");
#endif
#ifdef GITDATE
  log(1, "c git commit date " GITDATE "\n");
#endif

  CoprocessorInterface ci;
  ifstream file(argv[1]);
  stringstream pre;

  VarMapper vm;

  if (cfg.use_coprocessor) {
    stringstream mapped;
    mapped << setprecision(cfg.streamPrecision);
    pre << setprecision(cfg.streamPrecision);

    vm.map(file, mapped);

    if (!vm.partial)  {
      // Skip preprocessing if no hard clauses exist
      pre.str(mapped.str());
    } else {
      int ci_status = ci.CP_preprocess(mapped, pre);
      if (ci_status == CI_UNSAT) {
        log(0, "s UNSATISFIABLE\n");
        log(0, "c CPU time: %.2f seconds\n", SECONDS(clock() - begin));
        return 0;
      } else if (ci_status == CI_SAT) {
        // bypass issue with coprocessor hack (TODO: fixme)
        cfg.use_coprocessor = false;
        file.close();
        file.open(argv[1]);
      } else if (ci_status == CI_ERR) {
        log(0, "s UNKNOWN\n");
        return 0;
      } // else ci_status == CI_UNKNOWN
    }
  }

  istream & wcnf_in = cfg.use_coprocessor ? (istream &)pre : (istream &)file;

  ProblemInstance instance(wcnf_in);

  maxsat_solver = new Solver(instance);

  file.close();

  maxsat_solver->solve(cout);
  if (cfg.use_coprocessor) {

    stringstream pre_model, model;
    pre_model << setprecision(cfg.streamPrecision);
    model << setprecision(cfg.streamPrecision);

    maxsat_solver->printSolution(pre_model);
    if (vm.partial) {
      ci.CP_completeModel(pre_model, model);
      vm.unmap(model, cout);
    } else {
      vm.unmap(pre_model, cout);
    }
  } else {
    maxsat_solver->printSolution(cout);
  }
  
  maxsat_solver->printStats();

  log(1, "c CPU time: %.2f seconds\n", SECONDS(clock() - begin));
  return 0;
}
