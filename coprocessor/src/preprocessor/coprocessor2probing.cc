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
        coprocessor2probing.cc
        This file is part of riss.

        12.12.2011
        Copyright 2011 Norbert Manthey
*/

#include "preprocessor/coprocessor2.h"

bool Coprocessor2::prUpdateWatchLists(const Lit unit) { return true; }

Lit Coprocessor2::prPropagate(const Lit literal, bool topLevel,
                              reasonStruct reason) {
  assert(prUnitQueue.empty() && "there cannot be units before propagating");
  conflictStruct conflict;
  uint32_t oldTrailSize = prTrail.size();
  conflict = prEnqueueLiteral(literal, reason);  // usually, the reason is empty
  if (!conflict.empty()) {
    Lit unitConflict = prAnalyze(conflict);
    if (unitConflict == FAIL_LIT) unitConflict = literal.complement();
    return unitConflict;
  }

  while (!prUnitQueue.empty() && (unlimited || probeLimit > 0)) {
    const Lit unit = prUnitQueue.front();  // this literal is set to true
    prUnitQueue.pop_front();
    // get a reference to the right watch list
    vector<CL_REF>& wl = prWatchLists[unit.toIndex()];  // get list of clauses
                                                        // that will be shrinked
                                                        // by this assignment
    uint32_t j = 0;  // count how many clauses stay in the watch list
    uint32_t i = 0;
    for (; i < wl.size(); ++i) {
      probeLimit =
          (probeLimit == 0) ? 0 : ((topLevel) ? probeLimit - 1
                                              : probeLimit);  // track limit
      techniqueDoCheck(do_probe);  // get some statistics
      Clause& clause = search.gsa.get(wl[i]);
      assert(clause.size() > 2 &&
             "clauses in watch lists have to contain more than 2 literals");
      Lit other = unit.complement() == clause.get_literal(0)
                      ? clause.get_literal(1)
                      : clause.get_literal(0);  // other watched literal
      if (!prCurrent->is_sat(
               other)) {  // only check clause, if it is not already satisfied
        Lit kLit = NO_LIT;
        uint32_t k = 2;
        for (; k < clause.size(); ++k) {
          kLit = clause.get_literal(k);
          if (!prCurrent->is_unsat(kLit)) break;
        }
        if (k != clause.size()) {  // there is another watch
          if (prCurrent->is_sat(kLit)) {
            wl[j++] = wl[i];  // keep clause in list
          } else {
            clause.swap_literals(
                other == clause.get_literal(0) ? 1 : 0,
                k);  // shuffle literal forward to watch position
            prWatchClause(kLit.complement(), wl[i]);  // watch the other literal
            // do something with ternary clauses for double look-ahead
            if (prDouble && topLevel &&
                clause.size() == 3) {  // add literals for double-lookahead
              for (uint32_t k = 0; k < clause.size(); ++k) {
                const Lit& l = clause.get_literal(k);
                if (!prCurrent->is_undef(l.toVar())) continue;
                if (!prArray.get(l.toIndex())) {
                  prArray.set(l.toIndex(), true);
                  prStack.push_back(l);
                }
              }
            }
          }
        } else {  // there is no other watch-able literal
          // all literals except the other watched literal are unsatisfied
          bool isSat = prCurrent->is_sat(other);
          bool lhbrSuccess = false;
          reasonStruct thisReason = reasonStruct(wl[i]);
          if (topLevel && prLHBR) {
            // TODO: do hyper binary resolution here!
            // this literal implies the currently propagated literal
            // prReason[unit.toVar() ].isLit() ? prReason[ unit.toVar()
            // ].getLit() :
            Lit checkLit = unit.complement();
            uint32_t k = 0;
            for (; k < clause.size(); ++k) {
              const Lit& l = clause.get_literal(k);
              // the propagated literal and the watched literals do not have
              // binary reasons (ususally)
              if (l == other || l == checkLit) continue;
              if (!prReason[l.toVar()].isLit()) break;
              // a literal in the clause has to depend either on the propagated
              // literal or on its predecessor
              if (prReason[l.toVar()].getLit() != checkLit) break;
            }
            // found a clause for LHBR
            if (k == clause.size()) {
              lhbrSuccess = true;
              // create new binary clause and add it to the formula, and set the
              // new reason, if not already satisfied
              if (!isSat) thisReason = reasonStruct(checkLit);
              removeClause(wl[i]);
              updateRemoveClause(clause);
              // remove clause from the other watch-list
              vector<CL_REF>& owl = prWatchLists[other.complement().toIndex()];
              bool remFlag = false;
              uint32_t oi = 0;
              for (; oi < owl.size(); ++oi)
                if (owl[oi] == wl[i]) {
                  wl[oi] = owl[owl.size() - 1];
                  owl.pop_back();
                  remFlag = true;
                }
              assert(remFlag &&
                     "the current clause has to be in the watch list of the "
                     "other literal");
              clause.set_literal(0, checkLit);
              clause.set_literal(1, other);
              clause.shrink(2);
              addClause(wl[i]);  // will add watches to BIG
            }
          }

          conflict = prEnqueueLiteral(other);
          if (!conflict.empty()) {
            conflict = conflictStruct(wl[i]);
            break;
          } else {  // otherwise, literal is already satisfied
            if (!lhbrSuccess)
              wl[j++] =
                  wl[i];  // move clause in watch list forward to close gaps
          }
        }
      } else {  // other watch is satisfied, keep clause in list
        wl[j++] = wl[i];
      }
    }  // end for clause in wl

    if (i != wl.size())  // move rest of list, if interrupted
      for (; i < wl.size();) wl[j++] = wl[i++];
    wl.resize(j);  // shrink list to its size

    if (!conflict.empty()) break;
  }
  // if there is a conflict, analyze it. If it results in a unit clause, return
  // the unit clause!
  if (!conflict.empty()) {
    Lit unitConflict = prAnalyze(conflict);
    // if ( unitConflict == FAIL_LIT ) unitConflict = literal.complement();
    return unitConflict;
  }
  // otherwise, continue with the other techniques
  if (prPure && topLevel && (unlimited || probePureLimit > 0)) {
    probePureLimit = probePureLimit > 0 ? probePureLimit - 1 : 0;
    // do pure detection on top level
    const uint32_t oldSize = prTrail.size();
    prDetectPure(prCurrent, prTrail);
    // add the clauses that have been found by Probe-Pure
    for (uint32_t i = oldSize; i < prTrail.size(); ++i) {
      if (prPureClause) {  // be able to disable the feature
        CL_REF ref = 0;
        if (false) {
          Lit lits[2];
          lits[0] = literal.complement();
          lits[1] = prTrail[i];
          prAddedPureClauses.push_back(search.gsa.create(lits, 2, false));
          ref = prAddedPureClauses[prAddedPureClauses.size() - 1];
        } else {
          ref = addNexClause(literal.complement(), prTrail[i]);
        }
        techniqueChangedLiteralNumber(do_probe, 2);
        techniqueChangedClauseNumber(do_probe, 1);
        if (ref == 0) {
          solution = UNSAT;
          break;
        }
      }
      // set reason for literals on trail (to be able to do resolution)
      prReason[prTrail[i].toVar()] = reasonStruct(literal.complement());
    }
  }
  if (prAutarky && topLevel) {
    // do autarky detection
    // TODO: fill in code here!
  }

  if (prDouble && topLevel && (unlimited || probeDoubleLimit > 0)) {
    // TODO: fill in code here that does double-lookahead
    uint32_t min = prDouble > prStack.size() ? prStack.size() : prDouble;
    Lit element = NO_LIT;
    bool extendingPorpagation = false;
    CL_REF extendRef = 0;
    for (uint32_t i = 0; i < min; ++i) {
      probeDoubleLimit = (probeDoubleLimit > 0) ? probeDoubleLimit - 1 : 0;
      if (element ==
          NO_LIT) {  // if element is not predefined, pick one randomly
        uint32_t position = rand() % prStack.size();
        element = prStack[position];
        prArray.set(element.toIndex(), false);
        prStack[position] = prStack[prStack.size() - 1];
        prStack.pop_back();  // remove element from stack
        element = element.complement();
      }
      uint32_t oldTrailSize = prTrail.size();

      // do not work on literal that is already assigned
      if (!prCurrent->is_undef(element.toVar())) continue;

      prUnitQueue.clear();  // if propagation failed, queue is not cleared!
      Lit unit = prPropagate(element, false, extendingPorpagation
                                                 ? reasonStruct(extendRef)
                                                 : reasonStruct());
      // last propagation has been done to extend top level
      if (extendingPorpagation) {
        // if failed, analyze conflict!
        if (unit == FAIL_LIT) {

          prUnitQueue.clear();
          return literal.complement();
        }

      } else {
        // if propagation has been done for double look, remove elements from
        // trail!
        for (uint32_t j = oldTrailSize; j < prTrail.size(); ++j) {
          prCurrent->undef_variable(prTrail[j].toVar());
          prReason[prTrail[j].toVar()] = reasonStruct();
        }
        prTrail.resize(oldTrailSize);
      }

      extendingPorpagation = false;
      // if there has been something to extend the current level, extend it and
      // do not count this as double-look!
      if (unit != NO_LIT) {
        extendingPorpagation = true;
        // TODO: decide whether it would be good to return the unit that has
        // been found on level 2
        if (true && unit != FAIL_LIT) return unit;

        // FIXME TODO: analyze, whether if unit != FAIL_LIT -> found a unit
        // clause!, if so, add this knowledge here!
        if (true)
          element = (unit == FAIL_LIT) ? element.complement() : unit;
        else
          element = element.complement();  // safe fallback
        // if( unit == FAIL_LIT ){ // there has been a longer conflict clause
        extendRef = addNexClause(literal.complement(), element);
        if (extendRef == 0) {
          solution = UNSAT;
          return NO_LIT;
        }
        // }
        i--;
      } else {
        element = NO_LIT;  // reset, so that next candidate can be picked
      }
    }
  }

  return NO_LIT;
}

conflictStruct Coprocessor2::prEnqueueLiteral(const Lit literal,
                                              const reasonStruct clRef) {
  // if literal is unsat, return is as unit conflict
  if (prCurrent->is_unsat(literal))
    return conflictStruct(literal.complement(), literal.complement());
  // if literal is sat, there should not be anything to do, since this method
  // assigns each variable
  if (prCurrent->is_sat(literal)) return conflictStruct();
  // assign variable
  uint32_t processedElements = prTrail.size();  // number of literals on trail
                                                // that have been considered for
                                                // binary propagation already
  prTrail.push_back(literal);
  prReason[literal.toVar()] = clRef;
  prUnitQueue.push_back(literal);
  prCurrent->set_polarity(literal.toVar(), literal.getPolarity());

  conflictStruct conflict;
  // enqueue all binaries that are implied by this literal and that are not
  // already satisfied (and on the trail)
  while (processedElements < prTrail.size()) {
    Lit tLit = prTrail[processedElements++];
    const vector<Lit>& lList = search.big.getAdjacencyList(tLit);
    for (uint32_t i = 0; i < lList.size(); ++i) {
      Lit iLit = lList[i];
      if (prCurrent->is_sat(iLit)) continue;
      if (prCurrent->is_unsat(iLit))
        return conflictStruct(tLit.complement(), iLit);
      prTrail.push_back(iLit);
      prReason[iLit.toVar()] = reasonStruct(tLit);
      prUnitQueue.push_back(iLit);
      prCurrent->set_polarity(iLit.toVar(), iLit.getPolarity());
    }
  }
  return conflict;
}

Lit Coprocessor2::prAnalyze(const conflictStruct conflict, bool levelOne) {
  markArray.nextStep();
  if (conflict.isLit() && conflict.getLit(0) == conflict.getLit(1))
    return conflict.getLit(0);
  uint32_t count = 0;  // count number of literals in the resolvent
  // TODO implement 1st UIP learning here!
  if (conflict.isLit()) {
    if (!search.assignment.is_unsat(conflict.getLit(0))) {
      markArray.setCurrentStep(conflict.getLit(0).toVar());
      count++;
    }
    if (!search.assignment.is_unsat(conflict.getLit(1))) {
      markArray.setCurrentStep(conflict.getLit(1).toVar());
      count++;
    }
  } else {
    const Clause& clause = search.gsa.get(conflict.getCL());
    for (uint32_t i = 0; i < clause.size(); ++i) {
      const Lit& clLit = clause.get_literal(i);
      if (search.assignment.is_unsat(clLit)) continue;
      markArray.setCurrentStep(clLit.toVar());
      count++;
    }
  }
  int32_t currentPosition = prTrail.size();

  if (count <=
      1) {  // in case some external assignment caused the conflict, return it
    while (currentPosition > 0) {
      currentPosition--;
      if (markArray.isCurrentStep(prTrail[currentPosition].toVar())) {
        return prTrail[currentPosition].complement();
      }
    }
  }

  // do resolution only, if there are literals to resolve!
  while (count > 1 && currentPosition > 0) {
    currentPosition--;
    Lit literal = prTrail[currentPosition];
    if (!markArray.isCurrentStep(literal.toVar()))
      continue;  // use only literals that are in the resolvent

    count--;
    markArray.reset(literal.toVar());
    // do not consider literals that are already unsatisfied globally
    if (search.assignment.is_unsat(literal)) continue;
    const reasonStruct reason = prReason[literal.toVar()];
    if (reason.isLit()) {
      if (!markArray.isCurrentStep(
               reason.getLit().toVar())) {  // only add, if not already present
        markArray.setCurrentStep(reason.getLit().toVar());
        count++;
      }
    } else {
      const Clause& clause = search.gsa.get(conflict.getCL());
      for (uint32_t i = 0; i < clause.size(); ++i) {
        const Lit& clLit = clause.get_literal(i);
        if (!markArray.isCurrentStep(
                 clLit.toVar())) {  // only add, if not already present
          markArray.setCurrentStep(clLit.toVar());
          count++;
        }
      }
    }
  }
  if (count == 1) {
    while (currentPosition >
           0) {  // find last literal that is in the resolvent and return it!
      currentPosition--;
      if (markArray.isCurrentStep(prTrail[currentPosition].toVar())) {
        return prTrail[currentPosition].complement();
      }
    }
  }

  if (currentPosition == 0) {
    return FAIL_LIT;
  } else {
    return NO_LIT;
  }
}

void Coprocessor2::prDetectPure(Assignment* assignment,
                                vector<Lit>& pureLiterals) {

  uint8_t count[max_index(search.var_cnt)];
  bool didSomething = false;
  uint32_t oldSize = pureLiterals.size();
  do {
    memset(count, 0, sizeof(uint8_t) * max_index(search.var_cnt));

    // adds the pure literals also to the assignment
    for (uint32_t i = 0; i < formula->size(); ++i) {
      const Clause& clause = search.gsa.get((*formula)[i]);
      if (clause.isIgnored()) continue;
      for (uint32_t j = 0; j < clause.size(); ++j)
        if (assignment->is_sat(clause.get_literal(j))) goto nextPureDetection;
      for (uint32_t j = 0; j < clause.size(); ++j)
        count[clause.get_literal(j).toIndex()] =
            1;  // take care of buffer overflow!
    nextPureDetection:
      ;
    }

    didSomething = false;
    for (Var v = 1; v <= search.var_cnt; ++v) {
      if (assignment->is_undef(v) && !search.eliminated.get(v) &&
          search.equivalentTo[v] == Lit(v, POS)) {
        if (count[Lit(v, POS).toIndex()] == 0) {
          pureLiterals.push_back(Lit(v, NEG));
          assignment->set_polarity(v, NEG);
          didSomething = true;
        } else if (count[Lit(v, NEG).toIndex()] == 0) {
          pureLiterals.push_back(Lit(v, POS));
          assignment->set_polarity(v, POS);
          didSomething = true;
        }
      }
    }
  } while (didSomething);
}

void Coprocessor2::prWatchClause(const Lit l, const CL_REF clause) {
  prWatchLists[l.toIndex()].push_back(clause);
}

void Coprocessor2::prClearStack() {
  for (uint32_t i = 0; i < prStack.size(); ++i)
    prArray.set(prStack[i].toIndex(), false);
  prStack.clear();
}

void Coprocessor2::prPrepareLiteralProbe() {
  prUnitQueue.clear();
  for (uint32_t i = 0; i < prTrail.size(); ++i) {
    prReason[prTrail[i].toVar()] = reasonStruct();
  }
  prTrail.clear();
  // prReason.clear();
  prClearStack();
}

bool Coprocessor2::probing(bool firstCall) {
  bool didSomething = false;
  bool foundEquivalences = false;
  techniqueStart(do_probe);
  prWatchLists.resize(max_index(search.var_cnt));

  prAddedPureClauses.clear();

  // clear all watch lists
  for (Var v = 1; v <= search.var_cnt; ++v) {
    prWatchLists[Lit(v, POS).toIndex()].clear();
    prWatchLists[Lit(v, NEG).toIndex()].clear();
  }
  // watch all clauses with size > 2
  for (uint32_t i = 0; i < formula->size(); ++i) {
    const Clause& clause = search.gsa.get((*formula)[i]);
    if (!clause.isIgnored() && clause.size() > 2) {
      prWatchClause(clause.get_literal(0).complement(), (*formula)[i]);
      prWatchClause(clause.get_literal(1).complement(), (*formula)[i]);
    }
  }
  // determine position that have to be taken for probing randomly
  uint32_t* position = new uint32_t[variableHeap.size()];
  for (uint32_t i = 0; i < variableHeap.size(); ++i) {
    position[i] = variableHeap.size() - (i + 1);
  }

  if (randomized) {
    const uint32_t size = variableHeap.size();
    for (uint32_t i = 0; i + 1 < variableHeap.size(); ++i) {
      uint32_t tmp = position[i];
      uint32_t move = rand() % (size - i - 1);
      position[i] = position[move];
      position[move] = tmp;
    }
  }

  // probe for each variable
  for (uint32_t i = 0; i < variableHeap.size() && !isInterupted() &&
                           (probeLimit > 0 || unlimited);
       ++i) {
    if (solution != UNKNOWN) break;
    // randomize which polarity is represented by the variables with the prefix
    // "pos" and "neg"
    uint8_t r = rand() & 1;
    Var v = variableHeap.item_at(position[i]);
    if (!search.assignment.is_undef(v) || search.eliminated.get(v)) continue;
    assert(!search.eliminated.get(v) &&
           "variable to probe on cannot be eliminated");
    prCurrent = &prPositive;
    prCurrent->copy_from(search.assignment, search.var_cap);
    prPrepareLiteralProbe();
    const Lit pos = Lit(v, r == 0 ? POS : NEG);
    Lit unit = prPropagate(pos);
    if (unit != NO_LIT) {
      if (unit == FAIL_LIT)
        unit = pos.complement();  // take care of the case, that there has been
                                  // a longer conflict

      if (!enqueTopLevelLiteral(unit)) {
        solution = UNSAT;
        goto endProbing;
      }
      // update own structures
      prPrepareLiteralProbe();
      prPropagate(unit, false);  // if fails, top level unit propagation of CP2
                                 // will also fail
      techniqueSuccessEvent(do_probe);
      didSomething = true;
      continue;
    }

    if (solution != UNKNOWN) break;

    prCurrent = &prNegative;
    prCurrent->copy_from(search.assignment, search.var_cap);
    prPrepareLiteralProbe();
    const Lit neg = Lit(v, r == 0 ? NEG : POS);
    unit = prPropagate(neg);
    if (unit != NO_LIT) {
      if (unit == FAIL_LIT)
        unit = neg.complement();  // take care of the case, that there has been
                                  // a longer conflict
      if (!enqueTopLevelLiteral(unit)) {
        solution = UNSAT;
        goto endProbing;
      }
      // update own structures
      prPrepareLiteralProbe();
      prPropagate(unit, false);

      techniqueSuccessEvent(do_probe);
      didSomething = true;
      continue;
    }

    prClearStack();
    prStack.push_back(pos);  // store equivalence class

    for (uint32_t j = 0; j < prTrail.size(); ++j) {
      const Var v = prTrail[j].toVar();
      assert(!prNegative.is_undef(v) &&
             "variables in negative trail have to be assigned");
      // unit?
      if (v == pos.toVar()) continue;  // do nothing to probed variable
      if (prPositive.get_polarity(v) == prNegative.get_polarity(v)) {
        didSomething = true;
        if (!enqueTopLevelLiteral(Lit(v, prNegative.get_polarity(v)))) {
          solution = UNSAT;
          goto endProbing;
        }
        // update own structures
        prPrepareLiteralProbe();
        prPropagate(Lit(v, prNegative.get_polarity(v)), false);

        techniqueSuccessEvent(do_probe);
        continue;
      }
      // pos = l equivalent
      if (!prPositive.is_undef(v)) {
        Lit eLit = Lit(v, prNegative.get_polarity(v)).complement();
        if (!prArray.get(eLit.toIndex())) {
          prArray.set(eLit.toIndex(), true);
          prStack.push_back(eLit);
          techniqueSuccessEvent(do_probe);
        }
      }
    }
    // report found equivalences
    if (prStack.size() > 1) {
      foundEquivalences = true;
      addEquis(prStack);
    }

    if (solution != UNKNOWN) break;
    if (prBinary && probeBinaryLimit > 0) {
      // there is no list
      const vector<Lit>& tmpList =
          search.big.getAdjacencyList(pos.complement());
      if (tmpList.size() == 0) continue;

      probeBinaryLimit = probeBinaryLimit > 0 ? probeBinaryLimit - 1 : 0;
      prCurrent = &prNegative;
      prCurrent->copy_from(search.assignment, search.var_cap);
      // select a binary clause (or repeat step multiple times
      prPrepareLiteralProbe();
      Lit l = tmpList[rand() % tmpList.size()];
      if (!search.assignment.is_undef(l.toVar()))
        continue;  // is already assigned, do not use it!
      // propagate
      unit = prPropagate(l);
      if (unit != NO_LIT) {
        if (unit == FAIL_LIT)
          unit = l.complement();  // take care of the case, that there has been
                                  // a longer conflict
        if (!enqueTopLevelLiteral(unit)) {
          solution = UNSAT;
          goto endProbing;
        }
        // update own structures
        prPrepareLiteralProbe();
        prPropagate(unit, false);

        techniqueSuccessEvent(do_probe);
        didSomething = true;
        goto nextProbeVariable;
      }
      if (solution != UNKNOWN) break;

      prClearStack();
      for (uint32_t j = 0; j < prTrail.size(); ++j) {
        const Var v = prTrail[j].toVar();
        if (v == pos.toVar()) continue;  // do nothing to probed variable
        assert(!prNegative.is_undef(v) &&
               "variables in negative trail have to be assigned");
        // unit?
        if (prPositive.get_polarity(v) == prNegative.get_polarity(v)) {
          didSomething = true;
          if (!enqueTopLevelLiteral(Lit(v, prNegative.get_polarity(v)))) {
            solution = UNSAT;
            goto endProbing;
          }
          // update own structures
          prPrepareLiteralProbe();
          prPropagate(Lit(v, prNegative.get_polarity(v)), false);

          techniqueSuccessEvent(do_probe);
          continue;
        }
        // pos = l equivalent is not possible for clauses
      }
    }

  nextProbeVariable:
    ;
  }  // end for v in heap

  delete[] position;

endProbing:
  ;

  for (uint32_t i = 0; i < prAddedPureClauses.size(); ++i) {
    const CL_REF clRef = prAddedPureClauses[i];
    Clause& clause = search.gsa.get(clRef);
    // do not add all clauses here? e.g. filter blocked clauses?
    if (true) {
      if (!addClause(clRef)) {
        solution = UNSAT;
      }
      formula->push_back(clRef);
    }
  }

  techniqueStop(do_probe);

  if (didSomething || foundEquivalences) propagateTopLevel();
  if (solution == UNKNOWN && foundEquivalences) {
    propagateTopLevel();
    equivalenceElimination();
  }
  return didSomething;
}
