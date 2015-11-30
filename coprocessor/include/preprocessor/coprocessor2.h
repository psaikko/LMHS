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
        coprocessor2.h
        This file is part of riss.

        05.12.2011
        Copyright 2011 Norbert Manthey
*/

#ifndef _Coprocessor2_H
#define _Coprocessor2_H

#include "defines.h"
#include "types.h"

#include "structures/literal_system.h"
#include "structures/assignment.h"
#include "structures/heap.h"
#include "structures/boolarray.h"

#include <vector>
#include <deque>

#include "structures/clause.h"
#include "utils/clause_container.h"
#include "structures/region_memory.h"
#include "structures/heap.h"

#include "utils/stringmap.h"
#include "utils/microtime.h"
#include "utils/sort.h"
#include "utils/varfileparser.h"
#include "utils/prefetch.h"
#include "utils/simpleParser.h"

#include "structures/markArray.h"
#include "utils/interupt.h"
#include "sat/searchdata.h"

#include <iosfwd>
#include <pthread.h>
#include <unordered_map>

#include <unistd.h>

using namespace std;

/** Preprocessor that is also able to simplify the formula during search.
 *
 *  Includes techniques like VE,BCE,EE,Failed Literal
 *Propagation,HTE,Vivification,Subsumption
 *
 */
class Coprocessor2 {

 protected:
  // ==== STRUCTURES ====

  // for memorizing eliminated clauses
  struct elimination_t {
    Var var1;
    uint32_t number;
    CL_REF* clauses;
  };

  // for memorizing replaced literals
  struct equivalence_t {
    uint32_t number;
    Lit* literals;
  };

  // for memorizing blocked clauses
  struct blocking_t {
    Lit lit1;
    CL_REF clause;
  };

  // for memorizing blocked clauses
  struct conversion_t {
    uint32_t size;
    Lit* lits;
  };

  struct Compression {
    // mapping from old variables to new ones
    Var* mapping;
    // old assignment TODO: check whether it would be more useful to find
    // another representation
    Assignment previous;  // remember which variable has been assigned already
    uint32_t variables;   // number of variables before compression
    vector<Lit> equivalentTo;  // store the equivalence of literals

    Compression() : mapping(0), previous((void*)0x0), variables(0) {};
    /// free the used resources again
    void destroy() {
      if (mapping != 0) delete[] mapping;
      previous.destroy();
    };
  };

  // for memorizing blocked clauses

  // element to put on the postProcessStack to undo the changes to the formula
  struct postEle {
    char kind;  // 'b' = BCE, 'e' EE, 'c' 1ooN conversion, 'd' compression, //
                // deleted something here
    blocking_t b;
    elimination_t e;
    conversion_t c;
    Compression d;
    equivalence_t ee;
  };

  /** store a pair of literals
   */
  class literalPair {
   public:
    Lit lits[2];

    literalPair() {}

    literalPair& operator=(const literalPair& other) {
      lits[0] = other.lits[0];
      lits[1] = other.lits[1];
      return *this;
    }

    bool operator<(const literalPair& other) const {
      return lits[0] < other.lits[0] ||
             (lits[0] == other.lits[0] && lits[1] < other.lits[1]);
    }

    /*
    bool operator<(const litPair &other) const
    { return lits[0] < other.lits[0] || (lits[0] == other.lits[0] && lits[1] <
    other.lits[1] ); }
    */

    literalPair(const literalPair& other) {
      lits[0] = other.lits[0];
      lits[1] = other.lits[1];
    }

    literalPair(const Lit& l1, const Lit& l2) {
      lits[0] = l1 < l2 ? l1 : l2;
      lits[1] = l1 < l2 ? l2 : l1;
    }
  };

 public:
  // LIB_VIRTUAL
  Coprocessor2(std::vector<CL_REF>* clause_set, searchData& sd,
               const StringMap& commandline, long tW);

  LIB_VIRTUAL ~Coprocessor2();

  /** preprocesses the formula by running several techniques in a loop as
   *specified by the parameter
   * NOTE: assumes that the clauses are sorted initially, if the sorted
   *parameter is set to true
   *
   */
  LIB_VIRTUAL solution_t
      preprocess(std::ostream& data_out, std::istream& data_in,
                 std::ostream& map_out, std::ostream& err_out,
                 std::vector<CL_REF>* formula,
                 std::vector<CL_REF>* potentialGroups,
                 std::unordered_map<Var, long>* whiteVars, long init_lb);

  LIB_VIRTUAL bool doSimplify(searchData& sd);

  // LIB_VIRTUAL solution_t simplifysearch (searchData& sd, std::vector<CL_REF>&
  // clauses, std::vector<CL_REF>& learnt);

  /** postprocess a given assignment
   * @param assignments vector of assignments that should be extended
   * @param vars number of variables in formula. NOTE: should only be used, if
   * the preprocessor is used without the solver
   * @param external assignment comes from an external solver, that gives
   * assignments for all variables
   */
  LIB_VIRTUAL void postprocess(std::ostream& err_out,
                               std::vector<Assignment>* assignments,
                               uint32_t vars = 0, bool external = false);

  /** signal preprocessor, that and equality has been found, so that it is able
   * to apply these EQs during simplification
   * @return false, if adding these equivalences results in an inconsistent
   * equivalent vector
  */
  LIB_VIRTUAL bool addEquis(const std::vector<Lit>& eqs,
                            bool explicitely = false);

  /** load the preprocessor state from a map-file to undo preprocessing and to
   *post-process models
   *
   * @return number of variables of original formula | -1 in case of error
   */
  LIB_VIRTUAL int32_t
      loadPostprocessInfo(std::istream& map_in, std::ostream& err_out);

  /** increase the number of variables, that has to be handled
  *
  *@param newVariables number of variables that have to be handled
  *@param suggestedCapacity suggested capacity
  */
  LIB_VIRTUAL void extend(uint32_t newVariables,
                          uint32_t suggestedCapacity = 0);

  std::unordered_map<Var, long> labelWeight;  /// map that stores the label
                                              /// weights
  long weightRemoved;

 protected:
  /** write postprocess information to file specified in mapFile attribute
   */
  void outputPostprocessInfo(std::ostream& map_out, std::ostream& err_out);

  /** extend all structures, such that they can handle the given number of
  *variables
  *
  * @param newVar number of variables that should be handled
  * @param initialize indicates that method is called from constructor
  */
  void extendStructures(uint32_t newVar, bool initialize = false);

  void set_parameter(const StringMap& commandline);

  /** schedule the preprocessing techniques according to the string that has
   * been passed to coprocessor
   * @return the solution of the formula
   */
  solution_t preprocessScheduled();

  // ==== Maintaining Formula ====
  vector<CL_REF> tmpClauses;  /// vector of clauses that can be used by any
                              /// technique (call clear before use!!)
  vector<Lit> tmpLiterals;    /// vector of clauses that can be used by any
                              /// technique (call clear before use!!)
  uint32_t* varOcc;  /// array that stores the number of occurences per variable
  uint32_t* litOcc;  /// array that stores the number of occurences per literal
  uint32_t missedCores;  /// For counting how many  cores were not added in VE
  uint32_t addedCores;   /// For counting how many  cores were not added in VE
  bool mod_cores;        /// For logging if bve modifications trigger
  BoolArray doNotTouch;  /// array that stores whether a variable is disallowed
                         /// to be altered semantically
  vector<vector<CL_REF> > occurrenceList;  /// store per literal a reference to
                                           /// the clause where it appears

  long topW;  /// top weight for setting hard clauses
  Heap<uint32_t, std::less<uint32_t> > literalHeap;   /// heap that stores the
                                                      /// literals according to
                                                      /// their frequency
  Heap<uint32_t, std::less<uint32_t> > variableHeap;  /// heap that stores the
                                                      /// variables according to
                                                      /// their frequency
  MarkArray markArray;  /// markarray that can be used for any technique
  std::vector<postEle> postProcessStack;  /// stack that contains all the
                                          /// information to postprocess
                                          /// assignments
  Allocator postClauses;
  uint64_t timeoutTime;     /// microtime when preprocessing has to end
                            /// (0=unlimited)
  uint64_t maintainTime;    /// time that is required to load formula and do all
                            /// the method calls around the search
  uint64_t* techniqueTime;  /// time that is consumed per technique
  int32_t* techniqueClaues;   /// number of clauses that is removed by technique
  uint32_t* techniqueEvents;  /// number of successful events of a technique
                              /// (e.g. units for UP, pure for pure, ... )
  uint32_t* techniqueCalls;   /// time that is consumed per technique
  uint64_t* techniqueCheck;   /// number of calls to a certain check in the
                              /// technique (do determine runtime)
  int32_t* techniqueLits;     /// number of calls to a certain check in the
                              /// technique (do determine runtime)
  /// return a reference to the vector with the clauses in which l occurs
  vector<CL_REF>& list(const Lit& l);
  /** add all clauses from a set of clauses to the maintain lists
   * @return false if unsatisfiable
   */
  bool addFormula(vector<CL_REF>& clauses);
  /**	 add the clause to the structures, enque if unit clause, ignore if
   * already sat
   * @param callChange if true, will also call "updateChangeClause" method
   * @return false, if adding results immediately in unsat
   */
  bool addClause(const CL_REF clause, bool callChange = true);
  /** create a new binary clause and add it to all the structures
   * @return if 0 is returned, adding the clause failed
   */
  CL_REF addNexClause(const Lit& l1, const Lit& l2);
  /// remove clause from all the lists
  void removeClause(const CL_REF clause);
  /** remove binary clause from lists
   *  Note: also updates the literal counts for the two literals
   */
  void removeClause(const Lit a, const Lit b, bool markDelete = false);
  /// remove all duplicate entries from a clause list
  void removeDuplicateClauses(const Lit literal);
  /// removes the clause at index l from a list
  void removeListIndex(const Lit& l, const uint32_t& index);
  /// remove a certain clause from the given list, return true, if found
  bool removeFromList(const Lit& l, const CL_REF ref);
  /// remove all ignored clauses from the formula
  void reduceFormula();
  /// remove everything (occCount, heaps, ... )
  void clearStructures();
  /// update litOcc and varOcc by removing the given clause, and all heaps
  void updateRemoveClause(const Clause& clause);
  /// if a clause has been altered, update structures that need that info
  void updateChangeClause(const CL_REF clause, bool addToSubsume = true);
  /// update litOcc and varOcc by removing the given clause, and all heaps
  void updateInsertLiteral(const Lit& l);
  void updateRemoveLiteral(const Lit& l);
  void updateDeleteLiteral(const Lit& l);

  /// free most of the resources that are used by the coprocessor
  void freeResources();

  /// if the garbage fraction is too high, do garbage collection
  void garbageCollect();

  /// sort clauses in occurrence list of literal l according to their length,
  /// remove ignored
  void sortListLength(const Lit& l);
  /** sorts the clauses in the list according to their literals (assumes that
   * all clauses do have the same size)
   *  Note: assumes that each clause is already sorted
   */
  void sortClauseList(vector<CL_REF>& clauseList);

  /// prints the formula and outputs the result as exit code
  void printFormula(std::ostream& data_out, std::ostream& err_out);
  void printMaxSATSolution(std::ostream& data_out);

  /// print an empty map file into the file that is given by the mapFile
  /// parameter
  void printEmptyMapfile(std::ostream& map_out);

  /// count the number of literals that are still in the formula
  uint32_t countLiterals();

  /// return true, if the timeout has been reached
  bool timedOut() const;
  // ==== enlarging the formula ===
  /** return the next variable that can be used for a definition
   * NOTE: all structures will be updated before the method returns a variable
   * @return next free variable that can be used
   */
  Var nextVariable();

  // ==== technique control ====
  enum availableTechniques {
    do_up = 1,       // [u] unit propagation
    do_pure = 2,     // [p] pure literal elimination
    do_subSimp = 3,  // [s] subsumption and self-subsuming resolution
    do_bve = 4,      // [v] bounded variable elimination
    // deleted something here
    do_ee = 6,      // [e] equivalence elimination
    do_hte = 7,     // [h] hidden tautology elimination
    do_bce = 8,     // [b] blocked clause elimination
    do_probe = 9,   // [r] blocked clause elimination
    do_hyper = 10,  // [y] hyper binary resolution
    do_vivi = 11,   // [v] clause vivification
    // deleted something here
    do_unhide = 13,  // [b] advanced unhiding algorithm
    // deleted something here
    // if technique is added, also add line to showStatistics method
    numberOfTechniques =
        15,  // number of techniques, add other techniques before!
  };

  /** method that returns whether the next wanted technique can be executed
   * @param round round of loop in preprocessor
   */
  bool runNextTechnique(availableTechniques nextTechnique, uint32_t round);
  /** do another technique iteration?
   */
  bool moreIterationsRequired(uint32_t rounds) const;
  /** do not run another technique
   * @param rounds number of rounds that have been executed until now
   */
  bool stopPreprocessing(bool change, uint32_t rounds) const;

  /** method to report that a technique starts now
   */
  void techniqueStart(availableTechniques nextTechnique);
  /** method to report that a technique stops now
   */
  void techniqueStop(availableTechniques nextTechnique);
  /** report that a technique has removed or added a certain number of clauses
   */
  void techniqueChangedClauseNumber(availableTechniques nextTechnique,
                                    int32_t clauses);
  /** report that a technique has removed or added a certain number of literals
   */
  void techniqueChangedLiteralNumber(availableTechniques nextTechnique,
                                     int32_t literals);
  /** report that technique could be applied sccesfully (one unit has been
   * propagated, one variable has been eliminated, ... )
   */
  void techniqueSuccessEvent(availableTechniques nextTechnique);
  /** report that technique could be applied sccesfully (one unit has been
   * propagated, one variable has been eliminated, ... )
   */
  void techniqueDoCheck(availableTechniques nextTechnique);
  /** shows statistics about current state and techniques
   */
  void showStatistics();
  // ==== unit Propagation ====
  deque<Lit> propagationQueue;
  /** enqueue literal l to the top level interpretation
   * @return false, if enqueing fails
   */
  bool enqueTopLevelLiteral(const Lit& l, bool evenIfSAT = false);
  /** propagate the top level interpretation through the formula
   * @NOTE: can change the solution for the formula to UNSAT!
   * @param evenIfSAT set to true, if there appear new clauses with that
   * literal, that have to be propagated as well
   * @return true, if the formula has been changed
   */
  bool propagateTopLevel();

  // ==== pure literal detection ====
  /** delete all pure literals until fixpoint
   * @param expensive true, if really all variables should be checked
   * @return true, if the formula has been changed
   */
  bool eliminatePure(bool expensive = false);

  // ==== subsumption, self subsuming resolution ====
  deque<CL_REF> subsumptionQueue;  /// queue that stores all the clauses that
                                   /// should be tested for subsuming something

  /** run subsumption for the formula
   * @return true, if the formula has been changed
   */
  bool selfSubsumption(bool firstCall = false);

  // ==== variable eleimination ====
  Heap<uint32_t, std::less<uint32_t> > bveHeap;  /// heap that stores the
                                                 /// variables according to
                                                 /// their frequency (dedicated
                                                 /// for BVE)
  BoolArray bveTouchedVars;  /// array, memorizing candidate variables for bve

  /** applies VE by resolving variable out
   * @param variable variable to resolve on
   * @param force ignore limits( 0=dont ignore, 1=ignore, 2=ignore+dont store
   * old clauses, 3=ignore+no subsumption
   * @return report whether something happened
   */
  bool resolveVar(const Var variable, int force = 0);

  /** add all clauses that are active wrt. BVE again to subsumtion queue,
   * variables into heap, reset bveTouchedVars
   */
  void addActiveBveClauses();

  /** check the occurrence lists of variable v for gates, return the number of
   * clauses per polarity that take part
   * NOTE: sorts the occurrencelists so that the first pos clauses in the
   * positive list and the first neg clauses in the negative list represent the
   * gate
   * @param variable variable to check for gate
   * @param pos number of (first) clauses in the positive list that participate
   * in the gate
   * @param neg number of (first) clauses in the negative list that participate
   * in the gate
   * @return true, if a gate has been found
   */
  bool bveFindGate(const Var variable, uint32_t& pos, uint32_t& neg);

  /** try to find a gate x <-> ITE(s,t,f) for the given variable
   * NOTE: if gate is found, the two corresponding clauses per polarity will be
   * in the first two positions of the according occurence lists
   *  @return true, if a gate has been found
   */
  bool bveFindITEGate(const Var x);

  /** run bounded variable elimination
   * @NOTE selfSubsumption should be called before this method!
   * @NOTE VE will not consider learned clauses, but also removes them is a
   * variable is eliminated
   * @param firstCall true, if the method is called the first time (for
   * initialization)
   * @return true, if the formula has been changed
   */
  bool variableElimination(bool firstCall = false);

  // deleted something here

  // ==== equivalence removal ====
  /** equivalences in formula. index is variable, literal is another equivalent
  * literal for this variable.
  *   EQs are stored as tree, s.t. processing from bottom to top assignment
  * gives the correct assignment again
  */
  BoolArray eqReplaced;  /// indicate whether a literal has been already
                         /// replaced by EE
  BoolArray eqTouched;   /// bool array that stores whether a variable has been
                         /// touched for EQ (new binary clause) since last EQ
  vector<Lit> eqDoAnalyze;  /// vector that stores all the literals that have to
                            /// be analyzed for EE
  BoolArray eqLitInStack;   /// mark whether an element is in the stack
  vector<Lit> eqStack;      /// stack for the tarjan algorithm
  vector<int32_t> eqNodeLowLinks;  /// stores the lowest link for a node
  vector<int32_t> eqNodeIndex;     /// index per node
  BoolArray eqInSCC;  /// indicate whether a literal has already been found in
                      /// another SCC (than it cannot be in the current one)
  vector<Lit> eqCurrentComponent;  /// literals in the currently searched SCC
  uint32_t eqIndex;                /// help variable for finding SCCs

  /** rebuilds the binary implication graph in the searchData object
   */
  void reBuildBIG();

  /** searches for equivalences in the binary implication graph
   * @return true, if something has been altered
   */
  bool searchEquivalences(bool firstCall = false);

  /** return whether two literals are equivalent
   *  NOTE: this method should be used because the equivalence representation is
   * not direct
   * @return true, if the literals are equivalent
   */
  bool isEquivalent(const Lit& a, const Lit& b);
  bool isEquivalent(const Var& a, const Var& b);

  /** ececute the tarjan algorithm
   *  store result in eqCurrentComponent vector
   * @param literal
   * @param add add the currently handled literals to the compoenent?
   * @return true, if a non-trivial component has been found
   */
  bool eqTarjan(Lit literal, Lit list);

  /** checks the equivalence information and enqueues all literals, whose
   * equivalent literals are also enqueued
   * NOTE: if UNSAT has been found, solution is set to UNSAT
   * @return true, if some literal replacements have been done
   */
  bool enqueueAllEEunits();

  /** searches for equivalences and applies them in the formula
   * @return true, if something has been altered
   */
  bool equivalenceElimination(bool firstCall = false);

  // ==== hidden tautology elimination ====

  MarkArray hlaPositive;  /// markArray for positive literal of v (last call to
                          /// fillHlaArrays
  MarkArray hlaNegative;  /// markArray for negative literal of v (last call to
                          /// fillHlaArrays
  Lit* hlaQueue;  /// literals that need to be precessed for filling hla arrays
  Heap<uint32_t, std::less<uint32_t> > hteHeap;  /// heap that stores the
                                                 /// variables according to
                                                 /// their frequency (dedicated
                                                 /// for HTE)
  Heap<uint32_t, std::less<uint32_t> > bceHeap;  /// heap that stores the
                                                 /// variables according to
                                                 /// their frequency (dedicated
                                                 /// for BCE)

  /** fills the arrays for the positive and negative literal of v by hidden
   * literal addition
   * @NOTE: the implementation of Marijn Heule does not use the BIG, but an
   * inverse BIG
   * @return literal, that is unit because the other polarity is a failed
   * literal
   */
  Lit fillHlaArrays(Var v);

  /** mark all the literals with which the given clause could be extended with
   * the current step in the markArray
   * @NOTE: does not call the nextStep() method of the markArray
   * @return true, if the clause is a tautology
   */
  bool hlaMarkClause(const CL_REF clause);

  /** run hidden tautology elimination for variable v
   * @NOTE: the implementation of Marijn Heule does not use the BIG, but an
   * inverse BIG
   * @NOTE: assumes the fillHlaArrays has been called for variable v before (and
   * in the mean time not for some other variable)
   * @param v variable to run hte
   */
  bool hiddenTautologyElimination(Var v);

  /** run hidden tautology elimination
   * @note if hbce is set to true, calls also bce for hbce @see hbce
   * @param fistCall, this method is called the first time
   */
  bool hiddenTautologyElimination(bool firstCall = false);

  // ==== blocked clause elimination ====

  /** run blocked clause elimination for variable v
   * @param hla if true,use the hlaArrays and assume they are correctly set
   * already
   */
  bool blockedClauseElimination(Var v, bool hla);

  /** run blocked clause elimination
   * @note calls the above method with hla==false
   * @param fistCall, this method is called the first time
   */
  bool blockedClauseElimination(bool firstCall = false);

  // ==== probing ====

  vector<vector<CL_REF> > prWatchLists;  /// watchLists
  vector<Lit> prTrail;                   /// trail for probing
  vector<reasonStruct> prReason;  /// store reason per assigned literal (as
                                  /// array!)
  Assignment prPositive;   /// assignment that stores the interpretation for
                           /// positive assumption (literal 1)
  Assignment prNegative;   /// assignment that stores the interpretation for
                           /// negative assumption (literal 2)
  Assignment prTemporary;  /// assignment that is used for double look-ahead
  Assignment* prCurrent;   /// assignment that is currently used by all methods
  BoolArray prArray;  /// mark literals for the current propagation as candidate
                      /// for double-lookahead
  vector<Lit> prStack;                /// stores candidates for double-lookahead
  deque<Lit> prUnitQueue;             /// propagation queue for probing
  vector<CL_REF> prAddedPureClauses;  /// stores the pure clauses that have been
                                      /// found by prDetectPure and should be
                                      /// added

  /** adds literal as decision to prTrail and propagates all implication
   * fills prTrail and prReason
   * @note uses search.big for binary clauses
   * @param topLevel current call is after the first assumption
   * @param reason gives the reason for the literal (if used during double
   * lookahead)
   * @return a conflict unit clause, crated by 1st UIP conflict analysis
   */
  Lit prPropagate(const Lit literal, bool topLevel = true,
                  reasonStruct reason = reasonStruct());

  /** tries to extend the current trail by literal literal
   * @note will also add all binary implications immediately
   * @param clRef represents the reason for the literal
   * @return an empty conflict if everything works nicely
   */
  conflictStruct prEnqueueLiteral(const Lit literal,
                                  const reasonStruct clRef = reasonStruct());

  /** analyzes the current trail, returns a learned unit clause
   * @note performs propagation until there is a unit clause
   * @param conflict conflict that has been detected during propagation
   * @param levelOne indicate that the current conflict analysis is done on
   * level 1
   */
  Lit prAnalyze(const conflictStruct conflict, bool levelOne = true);

  /** adds literals to pureLiterals that are pure in the reduct of the formula
   * wrt. given assignment
   * @note adds the pure literals also to the assignment
   */
  void prDetectPure(Assignment* assignment, vector<Lit>& pureLiterals);

  /** put clause into the watch list of literal l
   */
  void prWatchClause(const Lit l, const CL_REF clause);

  /** remove elements from prStack and also clear them in prArray
   */
  void prClearStack();

  /** resets all structures that are used during probing
   */
  void prPrepareLiteralProbe();

  /** updates the watchlists for the literal unit
   * @return false, if process results in unsat
   */
  bool prUpdateWatchLists(const Lit unit);

  /** run probing to detect failed literals, units and equivalences
   * @note does not handle unit clauses, uses BIG for propagating binary clauses
   * @param fistCall, this method is called the first time
   */
  bool probing(bool firstCall = false);

  // ==== Distillation, Asymmetric Branching, Vivification ====
  /** run clause vivification in a randomized fashion
   * NOTE: assumes that the BIG is valid
   * NOTE: assumes that there are no satisfied clauses in the formula
   * traverses the BIG to linearize the algorithm
   */
  bool randomizedVivification(bool firstCall = false);

  // ==== unhiding ====

  /// structure that store all necessary stamp information of the paper for each
  /// literal
  struct literalData {
    uint32_t fin;  // finished
    uint32_t dsc;  // discovered
    uint32_t obs;  // observed last
    Lit parent;    // parent literal (directly implied by)
    Lit root;      // root literal of the subtree that also implied this literal
    Lit lastSeen;  //
    uint32_t index;  // index of the literal that has already been processed in
                     // the adjacence list of the literal
    literalData()
        : fin(0), dsc(0), obs(0), parent(NO_LIT), root(NO_LIT), index(0) {};
  };

  /// stamp information (access via literalData[ literal.toIndex() ] ), is
  /// maintained by extendStructures-method
  literalData* stampInfo;

  /// queue of literals that have to be stamped in the current function call
  deque<Lit> stampQueue;
  /// equivalent literals during stamping
  vector<Lit> stampEE;
  vector<Lit> stampClassEE;
  vector<char> unhideEEflag;

  /** sorts the given array with increasing discovery stamp
   * NOTE: uses insertion sort
   */
  void sortStampTime(Lit* literalArray, const uint32_t size);

  /** execute the advanced stamping algorithm
   * NOTE: there is a parameter that controls whether the advanced stamping is
   *used
   *
   *  @param literal root literal for the subtree to stamp
   *  @param stamp current stamp index
   *  @param detectedEE mark whether equivalent literals have been found
   */
  uint32_t stampLiteral(const Lit literal, uint32_t stamp, bool& detectedEE);

  /// linear version of the advanced stamping
  uint32_t linStamp(const Lit literal, uint32_t stamp, bool& detectedEE);

  /// recursive version
  uint32_t recStamp(const Lit literal, uint32_t stamp, bool& detectedEE);

  /** simplify the formula based on the literal stamps
   *
   */
  bool unhideSimplify();

  /** run the advanced stamping algorithm
   *
   * @return true, if some changes could be applied
   */
  bool unhiding(bool firstCall = false);

  // ==== formula compression ====

  /** compress the formula by removing all variables that are assigned, not
   * present or eliminated
   * the gaps in the variables are closed afterwards
   * @NOTE will also remove eliminated information
   * @NOTE will keep track of the activity per variable, thus, can be used
   * during simplification
   * @NOTE will rebuild the binary implication graph
   */
  bool compress();

  /// decompress a clause
  // void decompress (Clause& clause, const Compression& compression);

  /** decompress the entire assignment
   * @param variables number of current variables
   * @return number of variables after decompression
   */
  uint32_t decompress(Assignment& assignment, uint32_t variables,
                      Compression& compression);

  /// print compression table into file
  // void printTable(ofstream& file);

  /// print compression table into file
  // void loadTable(ifstream& file);

  // ==== formula rewriting ====

  vector<Var> bveBlockVariables;  /// variables that are not allowed to be used
                                  /// by BVE

  /** try to extract the at-most-one encoding and replace it by another
   * (smaller) encoding by using auxillary variables
   * @return true, if something has been replaced
   */
  bool rewriteAMO();

  /** try to extract the at-most-k encoding and replace it by another (smaller)
   * encoding by using auxillary variables
   * @return true, if something has been replaced
   */
  bool rewriteAMK();

  // deleted something here

  // ==== debug methods ====
  /** check all clauses whether they are ignored if they contain eliminated
  *literals
  *
  *	checks also, whether they still occur in any occurence lists
  */
  void scanClauses();

  // ==== PARAMETER ====
  CONST_PARAM uint32_t debug;     /// preprocessor debug level
  CONST_PARAM bool print_dimacs;  /// print formula after preprocessing
  CONST_PARAM bool group_encode;  /// Find group labels and use them...
  CONST_PARAM bool only_group;    /// If true ONLY group encode and print the
                                  /// formula
  CONST_PARAM bool sort;          /// print formula after preprocessing
  CONST_PARAM bool pline;         /// print pline of preprocessed formula
  CONST_PARAM bool enabled;       /// disable preprocessing, if requested
  CONST_PARAM bool fastSAT;  /// if all clauses are gone, give back a simple
                             /// model?
  std::string whiteFile;  /// name of the file that contains variables that have
                          /// to be kept
  std::string blackFile;  /// name of the file that contains variables that have
                          /// to be eliminated
  std::string mapFile;    /// name of the file that can be used to store the
                          /// variable mapping
  bool do_compress;       /// compress the formula after applying all techniques
  CONST_PARAM uint32_t rewriteMode;  /// when to try to rewrite
                                     /// (0=never,1=before preprocessing,2=after
                                     /// preprocessing,3=before and after)
  CONST_PARAM bool do_rewrite1;  /// try to extract and replace AMO constraints
                                 /// and replace them by the product encoding
  CONST_PARAM bool do_rewritek;  /// try to extract a AM-K encoding (naive,sinz)
                                 /// and replace it with the binary encoding
  // enable techniques
  CONST_PARAM bool unlimited;  /// run al techniques without limits
  CONST_PARAM uint32_t
      variableLimit;  /// number of variables that are still handled
  CONST_PARAM uint32_t
      clauseLimit;                /// number of clauses that are still handled
  CONST_PARAM uint32_t fastLoop;  /// interrupt a technique, if another
                                  /// technique can be applied again (e.g. a
                                  /// unit has been found), levelbased
  bool randomized;  /// execute preprocessing techniques in a randomized fashion
                    /// (at least during simplification this is useful!)
  std::string techniques;     /// order which techniques have to be applied
  CONST_PARAM bool up;        /// run up on the formula
  CONST_PARAM bool pure;      /// run pure detection on the formula
  CONST_PARAM bool pureSimp;  /// run pure literal detection during
                              /// simplification
  CONST_PARAM bool subSimp;   /// run subsumption and elimination on the formula
  uint32_t susiLimit;         /// run subsumption until limit is reached
  bool bve;                   /// run subsumption and elimination on the formula
  CONST_PARAM bool bveGate;   /// do gate extraction during bve?
  CONST_PARAM bool bveGateXor;  /// do gate extraction for xor gates during bve?
  CONST_PARAM bool bveGateIte;  /// do gate extraction for ITE gates during bve?
  CONST_PARAM bool bveOTFSS;    /// do OTFSS during BVE
  CONST_PARAM uint32_t bveShrink;  /// apply bve to variables only, if (0=number
                                   /// of clauses,1=number of literals,2=both
                                   /// values decrease)
  uint32_t bveLimit;       /// maximum number of checks until BVE is stopped
  bool ee;                 /// do equivalence elimination?
  bool eeBehindBve;        /// do equivalence elimination after BVE?
  uint32_t eeLimit;        /// limit for equivalence reasoning
  bool hte;                /// do hidden tautology elimination?
  uint32_t hteLimit;       /// maximum number of checks until HTE is stopped
  CONST_PARAM bool hbce;   /// enable hidden literal addition before doing bce,
                           /// only together with hte
  bool bce;                /// do blocked clause elimination?
  uint32_t bceLimit;       /// maximum number of checks until BCE is stopped
  uint32_t hbceLimit;      /// maximum number of checks until HBCE is stopped
  CONST_PARAM bool probe;  /// do probing?
  CONST_PARAM uint32_t prDouble;  /// do double-lookahead for literals in
                                  /// reduced ternary clauses (number^=percent
                                  /// of found literals?
  bool prPure;  /// do pure literal detection after propagating
  CONST_PARAM bool prPureClause;  /// add the found binary clauses of prPure
  CONST_PARAM bool prAutarky;  /// do autarky detection after propagating (and
                               /// prPure)
  CONST_PARAM bool prBinary;   /// do probing for binary clauses?
  CONST_PARAM bool prLHBR;  /// do lazy hyper binary resolution during probing?
  uint32_t probeLimit;  /// maximum number of checks until probing is stopped
  uint32_t probePureLimit;    /// maximum number of checks until pure-probing is
                              /// stopped
  uint32_t probeDoubleLimit;  /// maximum number of checks until double-probing
                              /// is stopped
  uint32_t probeBinaryLimit;  /// maximum number of checks until double-probing
                              /// is stopped
  CONST_PARAM bool vivi;      /// do clause vivification?
  CONST_PARAM uint32_t
      viviLenPercent;       /// percent of max clause length that is processed
  uint32_t viviLimit;       /// number of propagations for vivification
  CONST_PARAM bool hyper;   /// do hyper binary resolution?
  CONST_PARAM bool unhide;  /// do unhiding algorithm
  CONST_PARAM bool uhdTransitive;  /// do transitive edge reduction in advanced
                                   /// stamping during unhiding
  CONST_PARAM uint32_t
      unhideIter;  /// number of iterations for the unhiding algorithm
  CONST_PARAM bool advancedStamping;  /// use the advanced stamping algorithm
  uint32_t unhideLimit;  /// required, because of non-deterministic behaviour.
                         /// next call very likely will find more
                         /// simplifications
  CONST_PARAM uint32_t doUHLE;    /// do unhiding hidden literal elimination
                                  /// (0=no,1=positive literals,2=negative
                                  /// literals,3=both)
  CONST_PARAM bool doUHTE;        /// do unhiding hidden tautology elimination
  CONST_PARAM bool uhdNoShuffle;  /// disable shuffeling for unhiding (should be
                                  /// used only for debugging!)
  bool uhdEE;                     /// do EE during unhiding?
  // deleted stuff here

  // ==== COPROCESSOR DATA ===
  std::vector<CL_REF>* formula;  /// stores the current formula that should be
                                 /// processed
  searchData& search;            /// search state
  solution_t solution;           /// state of the formula
  int32_t numberOfSolutions;     /// number of solutions (if >1, forbid
                                 /// variableadding techniques after
                                 /// preprocessing!)
  uint32_t lastTrailSize;  /// number of literals on the trail when coprocessor
                           /// had the searchdata object the last time (used for
                           /// triggering simplification)
  uint32_t simpRequests;   /// number of rejected simplification requests since
                           /// the last simplification
  uint32_t simpRequestLimit;     /// number of rejected simplification requests
                                 /// until the next one is served
  uint32_t simpRequestLimitInc;  /// difference for request limit to next call
  uint32_t simpRequestLimitIncInc;  /// inc for the inc
  bool simplifying;  /// indicate, whether current call is simplification
                     /// instead of preprocessing
  uint32_t simplifications;  /// number of simplifications
  uint32_t verbose;          /// verbosity level for preprocessor
  uint32_t blockedLearnt;    /// number blocked learned clauses
  CL_REF trackClause;        /// id of clause that should be tracked
};

#endif
