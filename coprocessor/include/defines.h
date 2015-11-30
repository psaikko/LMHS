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
        defines.h
        This file is part of riss.

        16.10.2009
        Copyright 2009 Norbert Manthey
*/

#ifndef _DEFINES_H
#define _DEFINES_H

#include <string>
#include <sstream>

// returns a string which contains all information about the build flags
std::string getBuildFlagList();

// All compile time parameter can be set up in this file

// choose which kind of solver to build
#ifndef SATSOLVER
#define SATSOLVER
#endif

#ifdef CSPSOLVER
#undef SATSOLVER
#endif

#ifdef REGIONALLOCATOR
// #define REGIONALLOCATOR
#endif

// compile binary, so that it can be used only as preprocessor
#ifdef COPROCESSOR
//  #define COPROCESSOR
#endif

// use parallel stuff? (slower, if run with a single thread!)
#ifndef PARALLEL
#define PARALLEL
#endif

#ifndef NO_SLS
#define NO_SLS
#endif

#ifndef WITH_SLS
#undef NO_SLS
#endif

// disable extra features
#ifndef NOFEATURES
//#define NOFEATURES
#endif

// use the binary heap, instead of a 4-heap
#ifndef BINARY_HEAP
#define BINARY_HEAP
#endif

// use a 4-heap
#ifdef TERNARY_HEAP
#ifdef BINARY_HEAP
#undef BINARY_HEAP
#endif
#endif

// store boolarray in 1bit and assignment in 2bits, instead of using 8bits
#ifndef BITSTRUCTURES
// #define BITSTRUCTURES
#endif

// use dominator analysis
#ifndef DOMINATOR
// #define DOMINATOR
#endif

// using spinlocks instead of semaphores?
#ifndef SPINLOCK
// #define SPINLOCK
#endif

// use the papi library to count hardware events?
#ifndef USE_SIMPLE_PAPI
// #define USE_SIMPLE_PAPI
#endif

// load libraries for solver components
#ifndef LIBRARY_SUPPORT
//#define LIBRARY_SUPPORT
#endif

// enable all at once
#ifdef LIBRARY_SUPPORT
#define LIBRARY_SUPPORT_PARSER
#define LIBRARY_SUPPORT_DECISION
#define LIBRARY_SUPPORT_PROPAGATION
#define LIBRARY_SUPPORT_ANALYSIS
#define LIBRARY_SUPPORT_RESTARTE
#define LIBRARY_SUPPORT_REMOVALE
#define LIBRARY_SUPPORT_PREPROCESSOR
#endif

// #ifdef TRACK_Clause then all clauses that are tracked report progress on them
#ifndef TRACK_Clause
// #define TRACK_Clause
#endif

// defines for competition
#ifndef COMPETITION
//	#define COMPETITION
#endif

// disables almost every output to std::cerr
#ifndef SILENT
//	#define SILENT
#endif

// disables most parts of the help messages
#ifndef TESTBINARY
// #define TESTBINARY
#endif

#ifndef GOOD_STRUCTURES
#define GOOD_STRUCTURES
#endif

#ifdef BAD_STRUCTURES
#undef GOOD_STRUCTURES
#endif

#ifndef BAD_STRUCTURES
#ifndef GOOD_STRUCTURES
#define GOOD_STRUCTURES
#endif
#else
// #define BAD_STRUCTURES
#endif

// use good structures -> define what is needed
#ifdef GOOD_STRUCTURES
#define USE_IMPLICIT_BINARY
//	#define USE_CONFLICT_PREFETCHING
#define BLOCKING_LIT
#define OTFSS
#define USE_PREFETCHING
#define USE_DUAL_PREFETCHING
//	#define PREFETCHINGMETHOD1
#define COMPACT_VAR_DATA
#define USE_RISS_Clause  // USE_CPP_CACHE_Clause // USE_RISS_Clause
#endif

/*****
        define for components
*****/

// enable loading components from libraries
#ifndef USE_LIBRARY
//	#define USE_LIBRARY
#endif

// if this flag is set, the possibilit1y to set components via command line is
// enabled
// the binary will get very large and the compile process will take a while
#ifndef USE_ALL_COMPONENTS
//	#define USE_ALL_COMPONENTS
#endif

// activates some components, use only for development
#ifndef USE_SOME_COMPONENTS
// #define USE_SOME_COMPONENTS
#endif

#ifdef USE_ALL_COMPONENTS
#ifndef USE_SOME_COMPONENTS
#define USE_SOME_COMPONENTS
#endif
#endif

// if this flag is set, the possibilit1y to set parameter for components via
// command line is enabled
// parameter compiler optimisation can not be done at compile time
#ifndef USE_COMMANDLINEPARAMETER
#define USE_COMMANDLINEPARAMETER
#endif

// if competition, do not use any components or parameters
#ifdef COMPETITION
#ifdef USE_COMMANDLINEPARAMETER
#undef USE_COMMANDLINEPARAMETER
#endif
#ifdef USE_SOME_COMPONENTS
#undef USE_SOME_COMPONENTS
#endif
#ifdef USE_ALL_COMPONENTS
#undef USE_ALL_COMPONENTS
#endif
#endif

#ifdef USE_COMMANDLINEPARAMETER
#define CONST_PARAM
#else
#define CONST_PARAM const
#endif

/*****
        defines clause access improvements
*****/

// define number of blocking literals per clause (minisat 2.1)
#ifndef BLOCKING_LIT
#define BLOCKING_LIT
#endif

#ifdef USE_IMPLICIT_BINARY
//#define USE_IMPLICIT_BINARY
#endif

// keep satisfied clauses in current watch list
#ifndef KEEP_SAT_IN_WATCH
//	#define KEEP_SAT_IN_WATCH
#endif

#ifndef USE_CONFLICT_PREFETCHING
//	#define USE_CONFLICT_PREFETCHING
#endif

#ifndef USE_SAME_LEVEL_PREFETCHING
//	#define USE_SAME_LEVEL_PREFETCHING
#endif

// if flag is set components which want to use prefetching are allowed to do so
#ifndef USE_PREFETCHING
//	#define USE_PREFETCHING
#endif

#ifdef USE_DUAL_PREFETCHING
// #define USE_DUAL_PREFETCHING
#endif

// only prefetch curren watch content
#ifndef PREFETCHINGMETHOD1
#define PREFETCHINGMETHOD1
#endif

// prefetch some watches ahead
#ifdef PREFETCHINGMETHOD2
#undef PREFETCHINGMETHOD1
#endif

// prefetch clauses from the current watch during propagation
#ifdef PREFETCHINGMETHOD3
#undef PREFETCHINGMETHOD1
#endif

// store level and reason information compact per variable?
#ifndef COMPACT_VAR_DATA
//	#define COMPACT_VAR_DATA
#endif

#ifdef COMPACT_VAR_DATA
#define VAR_REASON(x) var1[x].reason
#define VAR_LEVEL(x) var1[x].level
#else
#define VAR_REASON(x) reason[x]
#define VAR_LEVEL(x) level[x]
#endif

/*****
        algorithm improvements
*****/

// on the fly self subsumption
#ifndef OTFSS
// #define OTFSS
#endif

// measure agility paper: Adaptive Restart Control for Conﬂict Driven SAT
// Solvers FROM Armin Biere
#ifndef ASSI_AGILITY
// #define ASSI_AGILITY
#endif

/*****
 pack data structures
*****/

// compress clause representation? ( do not use padding bytes )
#ifndef COMPRESS_Clause
//#define COMPRESS_Clause
#endif

// compress structures which are needed for watched propagatin
#ifndef COMPRESS_WATCH_STRUCTS
//#define COMPRESS_WATCH_STRUCTS
#endif

// allign everything to 64 byte?
#ifndef USE_ALLIGNED_MALLOC
// #define USE_ALLIGNED_MALLOC
#endif

#ifdef USE_ALLIGNED_MALLOC
#ifndef MALLOC_ALLIGNMENT
#define MALLOC_ALLIGNMENT 64
#endif
#endif

// use slab memory to allocate literals that do not fit into header of clause
#ifndef USE_SLAB_MALLOC
//#define USE_SLAB_MALLOC
#endif

// what is the mamixmal length which should be handled by the slab malloc system
#ifndef SLAB_MALLOC_MAXSIZE
#define SLAB_MALLOC_MAXSIZE 32
#endif

/*****
    defines for building library-components
*****/
#ifdef COMPILE_LIBRARY
#define LIB_VIRTUAL virtual
#endif

#ifndef LIB_VIRTUAL
#define LIB_VIRTUAL
#endif

/*****
        defines for statistics
*****/

// this flags controls whether data collection will be done or not
// it enables basically data collection whose type is specified with the
// follwing flags
#ifndef COLLECT_STATISTICS
// #define COLLECT_STATISTICS
#endif

// deleted something here

/*****
        defines for data strucutres
*****/

// use all structures of one type, either STL or own c implementation
// predifined groups for faster choice
#ifndef USE_STL_STRUCTURES
#define USE_STL_STRUCTURES
#endif

#ifdef USE_STL_STRUCTURES
#define USE_STL_VECTOR
#define USE_STL_STACK
#define USE_CPP_HEAP
#define USE_STL_RINGBUFFER
#endif

#ifdef USE_C_STRUCTURES
#define USE_C_VECTOR
#define USE_C_STACK
#define USE_C_HEAP
#define USE_C_RINGBUFFER
#endif

// section to choose select only one components
#ifdef USE_C_VECTOR
#undef USE_STL_VECTOR
#endif

#ifdef USE_CPP_VECTOR
#undef USE_STL_VECTOR
#endif

#ifdef USE_C_STACK
#undef USE_STL_STACK
#endif

#ifdef USE_C_HEAP
#undef USE_CPP_HEAP
#endif

#ifdef USE_C_RINGBUFFER
#undef USE_STL_RINGBUFFER
#endif

/*****
        defines for assignment
*****/

// use some assignment compressions?
// not supported any longer!

/*****
        defines for boolarray
*****/
// 32bit packed boolarray or one byte per bool
// #define EXPERIMENTAL_BOOLARRAY

/*****
        defines for clause
*****/

// std.clause: CppCacheSlabClause
// #define USE_CPP_CACHE_Clause

// use c++ clause as standart!
#ifndef USE_CPP_Clause
//#define USE_CPP_Clause
#endif

// if built parameter want to use other clause, undefine cpp_clause
// select only one clause!

#ifdef USE_CPP_CACHE_Clause
#undef USE_CPP_Clause
#endif

#ifndef USE_RISS_Clause
#undef USE_CPP_Clause
#endif

/*******
********
        section to set all defines for output for -v parameter
********
*******/

/* defines to show
VEC_TXT	// kind of vector
STACK_TXT		// kind of stack
HEAP_TXT		// kind of heap
RINGBUFFER_TXT	// kind of ringbuffer
ASSIGNMENT_TXT	// kind of assignment
INTTYP					// size of one assignment element
BOOLARRAY_TXT		// kind of boolarray
CL_TXT			// clause type
*/

#ifndef USE_C_VECTOR
#ifdef USE_STL_VECTOR
#define VEC_TXT "STL_VECTOR"
#else
#define VEC_TXT ""
#endif
#else
#ifdef USE_C_VECTOR
#define VEC_TXT "C_VECTOR"
#else
#ifdef USE_CPP_VECTOR
#define VEC_TXT "CPP_VECTOR"
#else
#define VEC_TXT ""
#endif
#endif
#endif

#ifndef USE_C_STACK
#ifdef USE_STL_STACK
#define STACK_TXT "STL_STACK"
#else
#define STACK_TXT ""
#endif
#else
#define STACK_TXT "C_STACK"
#endif

#ifndef USE_C_HEAP
#ifdef USE_CPP_HEAP
#define HEAP_TXT "CPP_HEAP"
#else
#define HEAP_TXT ""
#endif
#else
#define HEAP_TXT "C_HEAP"
#endif

#ifndef USE_C_RINGBUFFER
#ifdef USE_STL_RINGBUFFER
#define RINGBUFFER_TXT "STL_RINGBUFFER"
#else
#define RINGBUFFER_TXT ""
#endif
#else
#define RINGBUFFER_TXT "C_RINGBUFFER"
#endif

#ifdef EXPERIMENTAL_ASSIGNMENT

#else
#define ASSIGNMENT_TXT "NORMAL"
#endif

#ifdef EXPERIMENTAL_BOOLARRAY

#else
#define BOOLARRAY_TXT "NORMAL"
#endif

#ifdef USE_CPP_Clause
#define CL_TXT "CPP_Clause"
#else
#ifdef USE_CPP_CACHE_Clause
#define CL_TXT "CPP_CACHE_Clause"
#else
#ifdef USE_RISS_Clause
#define CL_TXT "RISS_Clause"
#else
#endif
#endif
#endif
/*
*/

inline std::string getBuildFlagList() {
  std::stringstream s;
  s << VEC_TXT << ",";
  s << STACK_TXT << ",";
  s << HEAP_TXT << ",";
  s << RINGBUFFER_TXT << ",";
  s << "ASSIGNMENT(" << ASSIGNMENT_TXT << "),";
  s << "BOOLARRAY(" << BOOLARRAY_TXT << "),";
  s << CL_TXT << ",";
  s << "Clause PADDING(";
#ifdef USE_COMPONENTS
  s << 0;
#else
  s << 1;
#endif
  s << "),";
  s << "COMPONENTS(";
#ifdef USE_COMPONENTS
  s << 1;
#else
  s << 0;
#endif
  s << ")"
    << ",";
  s << "PARAMETER(";
#ifdef USE_COMMANDLINEPARAMETER
  s << 1;
#else
  s << 0;
#endif
  s << ")"
    << ",";
  s << "STATISTICS(";
#ifdef COLLECT_STATISTICS
  s << 1;
#else
  s << 0;
#endif
  s << ")";
  return s.str();
}

#endif
