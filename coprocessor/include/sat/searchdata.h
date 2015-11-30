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
        searchdata.h
        This file is part of riss.

        20.02.2010
        Copyright 2010 Norbert Manthey
*/

#ifndef _SEARCHDATA_H
#define _SEARCHDATA_H

#include "types.h"
#include "defines.h"

#include <vector>
#include "structures/assignment.h"
#include "structures/boolarray.h"
#include "structures/literal_system.h"
#include "sat/searchStructs.h"

#include "utils/binaryImplicationGraph.h"

class Clause;
#include "structures/allocator.h"

/** structure to combine the data for a variable, namely level and reason
*/
struct var1Data {
  int32_t level;
  reasonStruct reason;
} WATCH_STRUCT_PACK;

/** This class represents a certain search state.
*/
class searchData {
 private:
 public:
  vector<Lit> trail;  /// stack of literals, which were assigned to some
                      /// polarity. STACK_TOP(Lit,  sd.trail );
#ifndef COMPACT_VAR_DATA
  reasonStruct* reason;  /// array, stores the sd.reason for an implication for
                         /// a literal under current assignment.
                         /// sd.reason[literal]
  int32_t* level;  /// array, stores sd.level on which the literal is assigned.
                   /// sd.level[literal]
#else
  var1Data* var1;  /// store data for variable next to each other
#endif
#ifdef DOMINATOR
  Lit* dominator;  /// literal that dominates another variable assignment (per
                   /// variable)
#endif
  Assignment assignment;  /// current assignment corresponding to the trail
  std::vector<Lit> equivalentTo;  /// array that stores per literal to which
                                  /// smaller one it is equivalent
  int32_t current_level;          /// stores the current decision sd.level
  uint32_t var_cap;  /// capacity, how many variables can be handled by this
                     /// searchData (space is already there)
  uint32_t var_cnt;  /// number of variables that are really used at the moment
  uint32_t original_var_cnt;  /// number of variables, that occur in the
                              /// original formula
  double* activities;         /// activity per variable
  BoolArray eliminated;       /// indicate that a certain variable has been
                              /// eliminated durnig search

  vector<CL_REF>* formula;
  BIG big;  /// binary implication graph of the formula

  Allocator& gsa;  /// central storage for clauses

  double globalLearntAvg;   // store the avg of the length of learnt clauses
  double currentLearntAvg;  // store avg of learnt clauses between the last two
                            // restarts
  double globalLearntMin;   // store the min of the length of learnt clauses
  double currentLearntMin;  // store min of learnt clauses between the last two
                            // restarts
  double currentLearnts;    // number of learnt clauses so far

  solution_t solution;

  /***** METHODS ***/

  searchData(Allocator& clauseStorage);

  /** initialize the search data object using another already initialized object
  *
  * NOTE: this method does not copy the activity array!! it simply copies the
  *pointer
  * NOTE: this method does not re-assign the gsa-reference
  * NOTE: this method does not copy the related formula
  */
  void init(searchData& other);

  /** initialize the search data object for the given number of variables (after
   * parsing the formula)
  */
  void init(uint32_t var_cnt);

  /** constructor that initialized the search state
  *
  * @param var_cnt number of variables
  * @param assignment assignment that should be used
  */
  searchData(Allocator& clauseStorage, uint32_t var_cnt);

  /** copy constructor, really copies elements
   * @ATTENTION except activities
   * NOTE: also copies clauseStorage-reference from other object
   * NOTE: this method does not copy the related formula
  */
  searchData(searchData& other);

  /** free the used resources
  */
  ~searchData();

  /** setup data structure for more variables
  */
  void extend(const uint32_t newVars);

  /** reset counters
  */
  void resetLocalCounter();

  /** space is already allocated for N variables
  */
  uint32_t capacity() const;

 private:
  /** copy object
   * @ATTENTION this object does not copy any of the pointers
   */
  searchData& operator=(const searchData&) { return *this; }

  /** get space for the new number of variables
  *
  *   allocates memory only, if the new number is higher than the previous one
  */
  void allocateSpace(uint32_t newCapacity);
};

// careful here with forward declaration
#include "structures/clause.h"

inline searchData::searchData(Allocator& clauseStorage)
    :
#ifndef COMPACT_VAR_DATA
      reason(0),
      level(0),
#else
      var1(0),
#endif
#ifdef DOMINATOR
      dominator(0),
#endif
      assignment((void*)0),
      var_cap(0),
      var_cnt(0),
      original_var_cnt(0),
      activities(0),
      gsa(clauseStorage) {

  current_level = 0;
  globalLearntAvg = 0;
  currentLearntAvg = 0;
  globalLearntMin = var_cnt;
  currentLearntMin = var_cnt;
}

/** initialize the search data object using another already initialized object
*
* NOTE: this method does not copy the activity array!! it simply copies the
*pointer
* NOTE: this method does not re-assign the gsa-reference
*/
inline void searchData::init(searchData& other) {
  var_cnt = other.var_cnt;
  var_cap = other.var_cap;
  original_var_cnt = other.original_var_cnt;
  trail = other.trail;
#ifndef COMPACT_VAR_DATA
  reason = (CL_REF*)MALLOC(sizeof(reasonStruct) * (var_cap + 1));
  memcpy(reason, other.reason, var_cap * sizeof(reasonStruct));
  level = (int*)MALLOC(sizeof(int) * (var_cap + 1));
  memcpy(level, other.level, sizeof(int) * (var_cap + 1));
#else
  var1 = (var1Data*)MALLOC(sizeof(var1Data) * (var_cap + 1));
  memcpy(var1, other.var1, sizeof(var1Data) * (var_cap + 1));
#endif
  current_level = 0;

  assignment = other.assignment.copy(var_cap);
  equivalentTo = other.equivalentTo;
  // Allocate activity list and init to 0
  activities = other.activities;

#ifdef DOMINATOR
  dominator = other.dominator;
#endif

  globalLearntAvg = 0;
  currentLearntAvg = 0;
  globalLearntMin = var_cnt;
  currentLearntMin = var_cnt;
  currentLearnts = 0;

  big.init(var_cnt);
}

inline void searchData::init(const uint32_t variables) {
  assert(var_cnt == 0);
  assert(var_cap == 0);
  var_cnt = variables;
  var_cap = variables;
  original_var_cnt = variables;

#ifndef COMPACT_VAR_DATA
  reason = (CL_REF*)MALLOC(sizeof(reasonStruct) * (var_cap + 1));
  memset(this->reason, 0, var_cap * sizeof(reasonStruct));
  level = (int*)MALLOC(sizeof(int) * (var_cap + 1));
  for (uint32_t i = 0; i < (var_cap + 1); ++i) this->level[i] = -1;
#else
  var1 = (var1Data*)MALLOC(sizeof(var1Data) * (var_cap + 1));
  for (uint32_t i = 0; i < (var_cap + 1); ++i) {
    var1[i].level = -1;
    var1[i].reason = reasonStruct();
  }
#endif

  current_level = 0;

  assignment.create(var_cap);
  equivalentTo.resize(var_cap + 1);
  for (Var v = 1; v <= var_cap; ++v) {
    equivalentTo[v] = Lit(v, POS);
  }
  eliminated.init(var_cap + 1);

  // Allocate activity list and init to 0
  activities = (double*)MALLOC(sizeof(double) * (var_cap + 1));
  memset(activities, 0, sizeof(double) * (var_cap + 1));

#ifdef DOMINATOR
  dominator = (Lit*)MALLOC(sizeof(Lit) * (var_cap + 1));
  memset(dominator, 0, sizeof(Lit) * (var_cap + 1));
#endif

  globalLearntAvg = 0;
  currentLearntAvg = 0;
  globalLearntMin = var_cnt;
  currentLearntMin = var_cnt;
  currentLearnts = 0;

  big.init(var_cnt);
}

/** constructor that initialized the search state
*
* @param var_cnt number of variables
* @param assignment assignment that should be used
*/
inline searchData::searchData(Allocator& clauseStorage, uint32_t var_cnt)
    : assignment(var_cnt), eliminated(var_cnt + 1), gsa(clauseStorage) {
  this->var_cnt = var_cnt;
  var_cap = var_cnt;
  original_var_cnt = var_cnt;

#ifndef COMPACT_VAR_DATA
  reason = (CL_REF*)MALLOC(sizeof(reasonStruct) * (var_cap + 1));
  memset(this->reason, 0, var_cap * sizeof(reasonStruct));
  level = (int*)MALLOC(sizeof(int) * (var_cap + 1));
  for (uint32_t i = 0; i < (var_cap + 1); ++i) this->level[i] = -1;
#else
  var1 = (var1Data*)MALLOC(sizeof(var1Data) * (var_cap + 1));
  for (uint32_t i = 0; i < (var_cap + 1); ++i) {
    var1[i].level = -1;
    var1[i].reason = reasonStruct();
  }
#endif
  current_level = 0;

  equivalentTo.resize(var_cnt + 1);
  for (Var v = 1; v <= var_cap; ++v) {
    equivalentTo[v] = Lit(v, POS);
  }

  // Allocate activity list and init to 0
  activities = (double*)MALLOC(sizeof(double) * (var_cap + 1));
  memset(activities, 0, sizeof(double) * (var_cap + 1));

#ifdef DOMINATOR
  dominator = (Lit*)MALLOC(sizeof(Lit) * (var_cap + 1));
  memset(dominator, 0, sizeof(Lit) * (var_cap + 1));
#endif

  globalLearntAvg = 0;
  currentLearntAvg = 0;
  globalLearntMin = var_cnt;
  currentLearntMin = var_cnt;
  currentLearnts = 0;

  big.init(var_cnt);
}

/** copy constructor, really copies elements, @ATTENTION except activities
 * NOTE: also copies clauseStorage-reference from other object
*/
inline searchData::searchData(searchData& other)
    : assignment(other.var_cap), gsa(other.gsa) {
  var_cnt = other.var_cnt;
  var_cap = other.var_cap;
  original_var_cnt = other.original_var_cnt;
  trail = other.trail;
#ifndef COMPACT_VAR_DATA
  reason = (CL_REF*)MALLOC(sizeof(reasonStruct) * (var_cap + 1));
  memcpy(reason, other.reason, var_cap * sizeof(reasonStruct));
  level = (int*)MALLOC(sizeof(int) * (var_cap + 1));
  memcpy(level, other.level, sizeof(int) * (var_cap + 1));
#else
  var1 = (var1Data*)MALLOC(sizeof(var1Data) * (var_cap + 1));
  memcpy(var1, other.var1, sizeof(var1Data) * (var_cap + 1));
#endif
  current_level = 0;

  // Allocate activity list and init to 0
  activities = other.activities;
#ifdef DOMINATOR
  dominator = other.dominator;
#endif
  equivalentTo = other.equivalentTo;

  globalLearntAvg = 0;
  currentLearntAvg = 0;
  globalLearntMin = var_cnt;
  currentLearntMin = var_cnt;
  currentLearnts = 0;

  big.init(var_cnt);
}

/** free the used resources
*/
inline searchData::~searchData() {
#ifndef COMPACT_VAR_DATA
  if (reason != 0) free(reason);
  if (level != 0) free(level);
#else
  if (var1 != 0) free(var1);
#endif
  if (activities != 0) free(activities);
#ifdef DOMINATOR
  if (dominator != 0) free(dominator);
#endif
  eliminated.destroy();

  assignment.destroy();
}

/** setup data structure for more variables
*/
inline void searchData::extend(const uint32_t newVars) {
  // get space only, if necessary
  if (newVars > var_cap) {
    uint32_t diff = newVars - var_cap;  // always positive!
    if (diff < 2560) diff = 2560;
    allocateSpace(var_cap + diff);
  }
  var_cnt = newVars;
  big.extend(var_cnt);
}

/** reset counters
*/
inline void searchData::resetLocalCounter() {
  currentLearntAvg = 0;
  currentLearntMin = var_cnt;
  currentLearnts = 0;
}

/** space is already allocated for N variables
        */
inline uint32_t searchData::capacity() const { return var_cap; }

/** get space for the new number of variables
*
*   allocates memory only, if the new number is higher than the previous one
*/
inline void searchData::allocateSpace(uint32_t newCapacity) {
  // no need to do something with the trail because its growing by itself

  // only do something, if not already enough space is there
  if (newCapacity > var_cap) {

    const uint32_t diff = newCapacity - var_cap;

#ifndef COMPACT_VAR_DATA
    reason = (reasonStruct*)realloc(reason,
                                    (newCapacity + 1) * sizeof(reasonStruct));
    level = (int32_t*)realloc(level, (newCapacity + 1) * sizeof(int32_t));
#else
    var1 = (var1Data*)realloc(var1, (newCapacity + 1) * sizeof(var1Data));
#endif
    assignment.extend(var_cap, newCapacity);
    // uint32_t oldSize = equivalentTo.size();
    // Lit last = equivalentTo[ oldSize -1 ];
    equivalentTo.resize(newCapacity + 1);
    // assert( last == equivalentTo[ oldSize -1 ]  && "resizing a vector should
    // not alter any of the elements" );
    for (Var v = var_cap + 1; v <= newCapacity; ++v) {
      equivalentTo[v] = Lit(v, POS);
    }
    // assert( last == equivalentTo[ oldSize -1 ]  && "resizing a equivalentTo
    // should not alter any of the elements" );
    activities =
        (double*)realloc(activities, (newCapacity + 1) * sizeof(double));

#ifndef COMPACT_VAR_DATA
    memset(reason + var_cap + 1, 0, diff * sizeof(reasonStruct));
    for (uint32_t i = 1; i <= diff; ++i) {
      level[var_cap + i] = -1;
    }
#else
    for (uint32_t i = 1; i <= diff; ++i) {
      var1[var_cap + i].level = -1;
      var1[var_cap + i].reason = reasonStruct();
    }
#endif
    // initialize new activities with 0!
    memset(activities + var_cap + 1, 0, diff * sizeof(double));

    eliminated.extend(var_cap + 1, newCapacity + 1);
  } else {
    // cerr << "c [SD] silent extend to " << newCapacity << "(old=" << var_cap
    // << ")" << endl;
  }

  // finally update capacity
  var_cap = newCapacity;
}

#endif
