/*************************************************************************************************
Copyright (c) 2012, Norbert Manthey

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************************************/
/*
      main.c
      This file is part of riss.

      16.10.2009
      Copyright 2009 Norbert Manthey
*/

void print_defines();

#include <fstream>
#include <iostream>
#include <iomanip>
#include "stdlib.h"

//#include <sys/resource.h>

#include "defines.h"
#include "structures/clause.h"
#include "sat/searchdata.h"
#include "utils/commandlineparser.h"

#include "info.h"
#include "utils/monitor.h"
#include "utils/interupt.h"

#include "sat/rissmain.h"

using namespace std;

int32_t main(int32_t argc, char const* argv[]) {
  // rlimit stack_limit = {RLIM_INFINITY, RLIM_INFINITY};
  // setrlimit(RLIMIT_STACK, &stack_limit);

  // set signal handlers
  signal(SIGTERM, signalHandler);
  signal(SIGINT, signalHandler);
  signal(SIGXCPU, signalHandler);

  // set output precision
  cout << setprecision(4);
  cerr << setprecision(4);

  StringMap commandline;
  CommandLineParser cmp;
  commandline = cmp.parse(argc, argv);

  std::ofstream null_out;

  bool silence_stderr = commandline.contains("CP_nostderr");
  if (silence_stderr) null_out.open("/dev/null");

  std::ostream& err_out = silence_stderr ? null_out : std::cout;

  err_out << "c *****************************************************" << endl;
  err_out << "c *  Coprocessor 2.0, Copyright 2012 Norbert Manthey  *" << endl;
  err_out << "c *   This program may be redistributed or modified   *" << endl;
  err_out << "c * under the terms of the GNU General Public License *" << endl;
  err_out << "c * This tool can only be used for research purposes  *" << endl;
  err_out << "c *****************************************************" << endl;
  err_out << "c process id: " << getpid() << endl;

  // no parameter given?
  if (argc < 2 && !commandline.contains((const char*)"-h") &&
      !commandline.contains((const char*)"--help") &&
      !commandline.contains((const char*)"file")) {
    err_out << "use the -h parameter for more help." << endl << endl;
#ifndef COMPETITION
    if (commandline.contains((const char*)"-i")) print_build_info();
    if (commandline.contains((const char*)"-v")) print_defines();
#endif
    exit(0);
  }

// show detailed info about binary
#ifndef COMPETITION
  if (commandline.contains((const char*)"-i")) print_build_info();
  if (commandline.contains((const char*)"-v")) print_defines();
#endif

  int32_t ret = 0;
#ifdef CSPSOLVER
  ret = crissspmain(commandline);
#else
  ret = rissmain(err_out, commandline);
#endif
  MON_WAIT_WINDOW();
  return ret;
}

/// show information about the solver
void print_defines() {
  cerr << "c --------------------" << endl;
  cerr << "c Coprocessor" << endl;

  cerr << "c datastructure built parameter:" << endl;
  cerr << "c vector type:         " << VEC_TXT << endl;
  cerr << "c stack type:          " << STACK_TXT << endl;
  cerr << "c heap type:           " << HEAP_TXT << endl;
  cerr << "c ringbuffer type:     " << RINGBUFFER_TXT << endl;
  cerr << "c assignment type:     " << ASSIGNMENT_TXT << endl;
  cerr << "c boolarray type:      " << BOOLARRAY_TXT << endl;
  cerr << "c clause type:         " << CL_TXT << endl;
#ifdef COMPRESS_Clause
  cerr << "c clause padding:      no" << endl;
#else
  cerr << "c clause padding:      yes" << endl;
#endif
#ifdef USE_C_Clause  // does only work with objects. void* can not be
                     // dereferenced
  cerr << "c clause size:         "
       << "flatten" << endl;
#else
  cerr << "c clause size:         " << CL_OBJSIZE << endl;
#endif
#ifdef _CL_ELEMENTS
  cerr << "c clause elements:     " << _CL_ELEMENTS << endl;
#endif
/*
cerr << "c lit size:            " << sizeof(Lit) << endl;
        cerr << "c lit* size:           " << sizeof(Lit*) << endl;
*/
#ifdef COMPRESS_WATCH_STRUCTS
  cerr << "c watch struct padding:no" << endl;
#else
  cerr << "c watch struct padding:yes" << endl;
#endif
#ifdef USE_SLAB_MALLOC
  cerr << "c slab malloc lits:    yes" << endl;
#else
  cerr << "c slab malloc lits:    no" << endl;
#endif
  cerr << "c max slab malloc:     " << SLAB_MALLOC_MAXSIZE << endl;
#ifndef USE_ALLIGNED_MALLOC
  cerr << "c malloc allignment:   none" << endl;
#else
  cerr << "c malloc allignment:   64" << endl;
#endif
#ifdef BLOCKING_LIT
  cerr << "c blocking literals:   1" << endl;
#else
  cerr << "c blocking literals:   0" << endl;
#endif
#ifdef USE_IMPLICIT_BINARY
  cerr << "c implicit binaries:   1" << endl;
#else
  cerr << "c implicit binaries:   0" << endl;
#endif
#ifndef KEEP_SAT_IN_WATCH
  cerr << "c keep sat in watch:   no" << endl;
#else
  cerr << "c keep sat in watch:   yes" << endl;
#endif
#ifdef USE_PREFETCHING
  cerr << "c prefetch in dualprop:yes" << endl;
#else
  cerr << "c prefetch in dualprop:no" << endl;
#endif
#ifdef USE_DUAL_PREFETCHING
  cerr << "c prefetch dual watchs:yes" << endl;
#else
  cerr << "c prefetch dual watchs:no" << endl;
#endif
#ifdef PREFETCHINGMETHOD1
  cerr << "c prefetch:            current" << endl;
#else  // ^= #ifdef PREFETCHINGMETHOD2
  cerr << "c prefetch:            some ahead" << endl;
#endif
#ifndef USE_CONFLICT_PREFETCHING
  cerr << "c prefetch analysis:   no" << endl;
#else
  cerr << "c prefetch analysis:   yes" << endl;
#endif
#ifndef USE_SAME_LEVEL_PREFETCHING
  cerr << "c prefetch same level: no" << endl;
#else
  cerr << "c prefetch same level: yes" << endl;
#endif
#ifdef COMPACT_VAR_DATA
  cerr << "c compact var data:    yes" << endl;
#else
  cerr << "c compact var data:    no" << endl;
#endif
  cerr << "c reason struct size:  " << sizeof(reasonStruct) << endl;
#ifdef BAD_STRUCTURES
  cerr << "c data structures:     bad" << endl;
#endif
#ifdef GOOD_STRUCTURES
  cerr << "c data structures:     good" << endl;
#endif
#ifdef BITSTRUCTURES
  cerr << "c bit structures:      yes" << endl;
#else
  cerr << "c bit structures:      no" << endl;
#endif
#ifdef OTFSS
  cerr << "c oftss:               yes" << endl;
#else
  cerr << "c oftss:               no" << endl;
#endif
#ifndef ASSI_AGILITY
  cerr << "c assignment agility:  no" << endl;
#else
  cerr << "c assignment agility:  yes" << endl;
#endif
#ifdef USE_ALL_COMPONENTS
  cerr << "c component system:    all" << endl;
#else
#ifdef USE_SOME_COMPONENTS
  cerr << "c component system:    some" << endl;
#endif
#endif
#ifndef USE_ALL_COMPONENTS
#ifndef USE_SOME_COMPONENTS
  cerr << "c component system:    none" << endl;
#endif
#endif
#ifdef USE_COMMANDLINEPARAMETER
  cerr << "c parameter system:    enabled" << endl;
#else
  cerr << "c parameter system:    disabled" << endl;
#endif
#ifdef NOFEATURES
  cerr << "c extra features:      enabled" << endl;
#else
  cerr << "c extra features:      disabled" << endl;
#endif
#ifdef COLLECT_STATISTICS
  cerr << "c statistic system:    enabled" << endl;
#else
  cerr << "c statistic system:    disabled" << endl;
#endif
#ifndef TRACK_Clause
  cerr << "c track clauses:       no" << endl;
#else
  cerr << "c track clauses:       yes" << endl;
#endif
#ifdef SPINLOCK
  cerr << "c use spinlocks:       yes" << endl;
#else
  cerr << "c use spinlocks:       no" << endl;
#endif
  cerr << "c --------------------" << endl;
}
