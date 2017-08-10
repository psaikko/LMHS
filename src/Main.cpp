#include "GlobalConfig.h"
#include "Solver.h"
#include "Util.h"
#include "VarMapper.h"
#include "Timer.h"

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
VarMapper * varmap;

void stop(int) {
  printf("s UNKNOWN\n");
  if (maxsat_solver) maxsat_solver->printStats();
  fflush(stdout);
  _Exit(1);
}

void stop_incomplete(int) {
  if (maxsat_solver && varmap) {

    stringstream internal_model;
    internal_model << setprecision(GlobalConfig::get().streamPrecision);
    maxsat_solver->instance.printSolution(internal_model);

    varmap->unmap(internal_model, cout);

    fflush(stdout);
    maxsat_solver->printStats();
    fflush(stdout);
  }
  _Exit(1); 
}

int main(int argc, const char* argv[]) {

  Timer main_timer;
  main_timer.start();

  condTerminate(argc == 1, 1, "Error: need filepath.\n");

  GlobalConfig & cfg = GlobalConfig::get();
  
  cfg.parseArgs(argc, argv, cout);

  signal(SIGINT, stop);
  if (cfg.incomplete)
    signal(SIGTERM, stop_incomplete);
  else
    signal(SIGTERM, stop);
  signal(SIGXCPU, stop);

  cout << setprecision(cfg.streamPrecision);

  log(1, "c argv");
  for (int i = 0; i < argc; ++i)
    log(1, " %s", argv[i]);
  log(1, "\n");

#ifdef GITHASH
  log(1, "c solver git hash " GITHASH "\n");
#endif
#ifdef GITDATE
  log(1, "c git commit date " GITDATE "\n");
#endif

  ifstream file(argv[1]);
  
  if (file.fail()) {
    printf("Could not open file %s\n", argv[1]);
    exit(1);
  }

  varmap = new VarMapper();
  stringstream mapped;
  mapped << setprecision(cfg.streamPrecision);
  varmap->map(file, mapped);

  ProblemInstance instance(mapped, cout);

  instance.filename = string(argv[1]);

  maxsat_solver = new Solver(instance, cout);

  file.close();

  maxsat_solver->solve();

  stringstream internal_model;
  internal_model << setprecision(cfg.streamPrecision);
  instance.printSolution(internal_model);

  varmap->unmap(internal_model, cout);
  
  if (cfg.printStats) {
    maxsat_solver->printStats();
  }
  main_timer.stop();
  log(0, "c CPU time: %lu ms\n", main_timer.cpu_ms_total());
  log(0, "c Real time: %lu ms\n", main_timer.real_ms_total());
  return 0;
}
