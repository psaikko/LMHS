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
        coprocessor2equivalence.cc
        This file is part of riss.

        08.12.2011
        Copyright 2011 Norbert Manthey
*/

#include "preprocessor/coprocessor2.h"

bool Coprocessor2::isEquivalent(const Lit& a, const Lit& b) {
  Lit l = a;
  // find root element of the tree (traversing linked)
  while (l.toVar() != search.equivalentTo[l.toVar()].toVar()) {
    // do not consider eliminated variables!
    if (search.eliminated.get(search.equivalentTo[l.toVar()].toVar())) {
      break;
    }
    l = l.getPolarity() == POS ? search.equivalentTo[l.toVar()]
                               : search.equivalentTo[l.toVar()].complement();
  }

  Lit ae = l;

  l = b;
  // find root element of the tree (traversing linked)
  while (l.toVar() != search.equivalentTo[l.toVar()].toVar()) {
    // do not consider eliminated variables!
    if (search.eliminated.get(search.equivalentTo[l.toVar()].toVar())) {
      break;
    }
    l = l.getPolarity() == POS ? search.equivalentTo[l.toVar()]
                               : search.equivalentTo[l.toVar()].complement();
  }

  return ae == l;
}

bool Coprocessor2::isEquivalent(const Var& a, const Var& b) {
  Lit l = Lit(a, POS);
  // find root element of the tree (traversing linked)
  while (l.toVar() != search.equivalentTo[l.toVar()].toVar()) {
    // do not consider eliminated variables!
    if (search.eliminated.get(search.equivalentTo[l.toVar()].toVar())) {
      break;
    }
    l = l.getPolarity() == POS ? search.equivalentTo[l.toVar()]
                               : search.equivalentTo[l.toVar()].complement();
  }

  Lit ae = l;

  l = Lit(b, POS);
  ;
  // find root element of the tree (traversing linked)
  while (l.toVar() != search.equivalentTo[l.toVar()].toVar()) {
    // do not consider eliminated variables!
    if (search.eliminated.get(search.equivalentTo[l.toVar()].toVar())) {
      break;
    }
    l = l.getPolarity() == POS ? search.equivalentTo[l.toVar()]
                               : search.equivalentTo[l.toVar()].complement();
  }

  return ae.toVar() == l.toVar();
}

bool Coprocessor2::enqueueAllEEunits() {
  bool didSomething = false;
  // all occurence lists of literals that are replaced can be cleared
  tmpLiterals.clear();

  bool units = false;
  for (Var v = 1; v <= search.var_cnt && (unlimited || eeLimit > 0); ++v) {
    if (true)  // also check already assigned variables (because of probing!!)
    {
      // do not work on variables that are already eliminated
      if (search.eliminated.get(v)) {
        assert(search.assignment.is_undef(v) &&
               "an eliminated variable needs to be undefined");
        continue;
      }
      // do not replace variables again, that have been already replaced
      if (eqReplaced.get(v) && search.assignment.is_undef(v)) {
        continue;
      }
      // if there is an equivalent smaller variable, and the replacement has not
      // already been done
      if (search.equivalentTo[v] != Lit(v, POS)) {
        didSomething = !eqReplaced.get(v);

        eeLimit = (eeLimit > 0) ? eeLimit - 1 : 0;

        // remember, that this variable is not in the formula any longer
        // search.eliminated.set( v, true ); // store for global access
        // update elimination better by storing the replacement variable per
        // variable
        Lit r = Lit(v, POS);
        assert(!doNotTouch.get(r.toVar()) &&
               "ee should not be used for do-not-touch variables");
        do {
          Lit oldR = r;
          Var innerVariable = oldR.toVar();
          r = r.isPositive() ? search.equivalentTo[r.toVar()]
                             : search.equivalentTo[r.toVar()].complement();
          // write parent of current literal to old literal
          Lit newR = r.isPositive()
                         ? search.equivalentTo[r.toVar()]
                         : search.equivalentTo[r.toVar()].complement();
          assert(!doNotTouch.get(newR.toVar()) &&
                 "ee should not be used for do-not-touch variables");
          // set right literal for equivalence class
          search.equivalentTo[oldR.toVar()] =
              oldR.isPositive() ? newR : newR.complement();

          if (!search.assignment.is_undef(r.toVar()) &&
              search.assignment.is_undef(innerVariable)) {
            if (!enqueTopLevelLiteral(
                     Lit(r.toVar(), search.assignment.get_polarity(r.toVar())),
                     true)) {
              solution = UNSAT;
              return didSomething;
            }
            units = true;
          }
          if (search.assignment.is_undef(r.toVar()) &&
              !search.assignment.is_undef(innerVariable)) {
            // enqueue the right polarity!
            if (!enqueTopLevelLiteral(
                     search.assignment.is_sat(oldR) ? r : r.complement())) {
              solution = UNSAT;
              return didSomething;
            }
            units = true;
          }
        } while (r.toVar() != search.equivalentTo[r.toVar()]
                                  .toVar());  // repeat for all literals that
                                              // are equivalent until fix point
      }
    }
  }

  if (!didSomething) {
    return didSomething;
  }
  if (units) {
    if (!propagateTopLevel()) {
      solution = UNSAT;
      return didSomething;
    }
  }
  return didSomething;
}

bool Coprocessor2::equivalenceElimination(bool firstCall) {
  bool didSomething = false;

  // if ee is disabled by a parameter, do not execute it!
  if (!ee) {
    return didSomething;
  }

  techniqueStart(do_ee);
  if (firstCall && !simplifying)
    reBuildBIG();  // TODO: decide whether to rebuild big each time!
  didSomething = searchEquivalences(firstCall) || didSomething;

  didSomething = enqueueAllEEunits() || didSomething;

  tmpLiterals.clear();
  for (Var v = 1; v <= search.var_cnt; ++v) {
    if (search.eliminated.get(v)) {
      assert(search.assignment.is_undef(v) &&
             "an eliminated variable needs to be undefined");
      continue;
    }
    // if there is an equivalent smaller variable, and the replacement has not
    // already been done
    if (search.equivalentTo[v] != Lit(v, POS)) {
      eqReplaced.set(v, true);
      // add activities
      search.activities[search.equivalentTo[v].toVar()] += search.activities[v];
      search.activities[v] = 0;
      // update all structures
      /*
      list( Lit( v, POS) ).clear();
      list( Lit( v, NEG) ).clear();
      */
      search.big.removeLiteral(Lit(v, NEG));
      search.big.removeLiteral(Lit(v, POS));
      // memorize that this literal will be removed
      tmpLiterals.push_back(Lit(v, POS));
    }
  }

  if (true) {  // execute only, if above happened something
    // only replace literals in the formula, no need to move clauses
    // for ( uint32_t formulaIndex = 0; formulaIndex <  (*formula) .size();
    // ++formulaIndex ) {

    for (uint32_t literalIndex = 0; literalIndex < tmpLiterals.size();
         ++literalIndex) {
      for (uint32_t pol = 0; pol < 2; ++pol) {

        vector<CL_REF>& cList =
            list(pol == 0 ? tmpLiterals[literalIndex]
                          : tmpLiterals[literalIndex].complement());
        while (cList.size() > 0) {
          eeLimit = (eeLimit > 0) ? eeLimit - 1 : 0;
          const CL_REF clause_ref = cList[cList.size() - 1];
          Clause& clause = search.gsa.get(clause_ref);
          // process only relevant clauses
          if (clause.isIgnored()) {
            cList.pop_back();
            continue;
          }
          bool changed = false;

          // there needs to be a replaced-literal in the clause. otherwise, this
          // list would not be chosen
          // check whether something needs to be replaced
          uint32_t ci = 0;
          for (; ci < clause.size(); ++ci) {
            const Lit lit1 = clause.get_literal(ci);
            if (search.equivalentTo[lit1.toVar()] != Lit(lit1.toVar(), POS))
              break;
          }
          // remove the clause, if something needs to be changed
          // if ( ci != clause.size() ) removeClause (clause_ref);
          if (ci == clause.size()) {
            assert(false &&
                   "there cannot be a clause in a list of a literal that "
                   "should be replaced without having that literal");
            continue;
          }

          // try to replace lit in clause
          removeClause(clause_ref);

          if (clause.size() == 2) {
            search.big.removeClauseEdges(clause.get_literal(0),
                                         clause.get_literal(1));
          }

          const uint32_t oldSize = clause.size();
          updateRemoveClause(clause);
          for (uint32_t i = 0; i < clause.size(); ++i) {
            const Lit lit1 = clause.get_literal(i);
            if (search.equivalentTo[lit1.toVar()] != Lit(lit1.toVar(), POS)) {
              changed = true;
              Lit lit12 = (lit1.getPolarity() == POS)
                              ? search.equivalentTo[lit1.toVar()]
                              : search.equivalentTo[lit1.toVar()].complement();
              assert(!search.eliminated.get(lit12.toVar()) &&
                     " variable used for replacement cannot be eliminated");
              clause.set_literal(i, lit12);
            }
          }

          // if something in the clause has been changed
          if (changed) {
            changed = false;

            bool isTautology = false;
            // remove double occurrences and stop if tautology
            clause.sort();
            uint32_t j = 1;
            for (uint32_t i = 1; i < clause.size(); ++i) {
              const Lit lj = clause.get_literal(j - 1);
              const Lit li = clause.get_literal(i);
              // set new variable
              if (li.toVar() != lj.toVar())
                clause.set_literal(j++, li);
              else {
                if (li == lj.complement()) {
                  isTautology = true;
                  break;
                }
              }
            }

            techniqueSuccessEvent(do_ee);
            if (isTautology) {  // tautology, no need to add again
              clause.markToDelete();
              techniqueChangedLiteralNumber(do_ee, 0 - (int32_t)clause.size());
              techniqueChangedClauseNumber(do_ee, -1);
              removeClause(clause_ref);
            } else {  // no tautology, add again, before, remove from kept lists
              clause.shrink(j);
              if (!addClause(clause_ref)) {
                solution = UNSAT;
                break;
              }
              techniqueChangedLiteralNumber(
                  do_ee, (int32_t)clause.size() - (int32_t)oldSize);
            }
          }
        }  // end while clist
      }    // end while polarity
    }      // end replaced literal
  }

  // put at the stack, that equivalences have been applied
  postEle pe;
  pe.kind = 'e';
  pe.ee.number = tmpLiterals.size();
  pe.ee.literals = (Lit*)malloc(sizeof(Lit) * pe.ee.number);
  for (uint32_t i = 0; i < tmpLiterals.size(); ++i)
    pe.ee.literals[i] = tmpLiterals[i];
  postProcessStack.push_back(pe);

  // not necessary!?
  /*
  for( uint32_t i = 0 ; i < tmpLiterals.size(); ++ i ) {
    updateDeleteLiteral( tmpLiterals[i] );
    updateDeleteLiteral( tmpLiterals[i].complement() );
  }
  */

  techniqueStop(do_ee);
  return didSomething;
}

void Coprocessor2::reBuildBIG() {
  // clear the graph
  for (Var v = 1; v <= search.var_cnt; ++v) {
    search.big.clearAdjacency(Lit(v, POS));
    search.big.clearAdjacency(Lit(v, NEG));
  }
  // add all binary clauses
  for (uint32_t i = 0; i < formula->size(); ++i) {
    Clause& clause = search.gsa.get((*formula)[i]);
    if (clause.isIgnored()) continue;
    if (clause.size() == 2) {
      clause.setLearnt(false);
      const Lit l0 = clause.get_literal(0);
      const Lit l1 = clause.get_literal(1);

      assert(!search.eliminated.get(l0.toVar()) &&
             !search.eliminated.get(l1.toVar()) &&
             "variables in clauses cannot be eliminated");
      search.big.addClauseEdges(l0, l1);
    }
  }
}

#define MININ(x, y) (x) < (y) ? (x) : (y)

bool Coprocessor2::eqTarjan(Lit v, Lit list) {
  eqNodeIndex[v.data()] = eqIndex;
  eqNodeLowLinks[v.data()] = eqIndex;
  eqIndex++;
  eqStack.push_back(v);
  eqLitInStack.set(v.toIndex(), true);

  const vector<Lit> impliedLiterals = search.big.getAdjacencyList(list);
  bool didSomething = false;
  for (uint32_t i = 0; i < impliedLiterals.size(); ++i) {
    const Lit n = impliedLiterals[i];
    if (eqNodeIndex[n.toIndex()] == -1) {
      eeLimit = (eeLimit > 0) ? eeLimit - 1 : 0;
      didSomething = eqTarjan(n, n) || didSomething;
      eqNodeLowLinks[v.toIndex()] =
          MININ(eqNodeLowLinks[v.toIndex()], eqNodeLowLinks[n.toIndex()]);
    } else if (eqLitInStack.get(n.toIndex())) {
      eqNodeLowLinks[v.toIndex()] =
          MININ(eqNodeLowLinks[v.toIndex()], eqNodeIndex[n.toIndex()]);
    }
  }

  if (eqNodeLowLinks[v.toIndex()] == eqNodeIndex[v.toIndex()]) {
    Lit n;
    eqCurrentComponent.clear();
    do {
      n = eqStack[eqStack.size() - 1];  // *(eqStack.rbegin());
      eqStack.pop_back();
      eqLitInStack.set(n.toIndex(), false);
      eqInSCC.set(n.toVar(), true);
      eqCurrentComponent.push_back(n);
    } while (n != v);
    if (eqCurrentComponent.size() > 1) {
      addEquis(eqCurrentComponent);
      didSomething = true;
    }
  }
  return didSomething;
}

bool Coprocessor2::searchEquivalences(bool firstCall) {
  bool didSomething = false;
  techniqueStart(do_ee);
  // reset all data structures
  eqIndex = 0;
  eqNodeIndex.assign(eqNodeIndex.size(), -1);
  eqNodeLowLinks.assign(eqNodeLowLinks.size(), -1);
  eqLitInStack.clear(search.capacity());
  eqInSCC.clear(search.capacity());
  eqStack.clear();

  // find SCCs

  if (firstCall) {
    for (Var v = 1; v <= search.var_cnt; ++v) {
      if (!search.eliminated.get(v)) {
        eqDoAnalyze.push_back(Lit(v, POS));
      }
    }
  }

  while (!eqDoAnalyze.empty() && (unlimited || eeLimit > 0)) {
    techniqueDoCheck(do_ee);  // get some statistics
    Lit randL = eqDoAnalyze[eqDoAnalyze.size() - 1];
    if (randomized) {  // shuffle an element back!
      uint32_t index = rand() % eqDoAnalyze.size();
      eqDoAnalyze[eqDoAnalyze.size() - 1] = eqDoAnalyze[index];
      eqDoAnalyze[index] = randL;
      randL = eqDoAnalyze[eqDoAnalyze.size() - 1];
    }
    eqDoAnalyze.pop_back();

    const Lit l = randL;
    // work only with the literal, if it is the first call, or if something
    // happened since the last look
    if (!firstCall && !eqTouched.get(l.toIndex())) continue;
    // skip literal, if it is already in a SCC or it does not imply anything
    if (eqInSCC.get(l.toVar())) continue;
    if (search.big.getAdjacencyList(l).empty()) continue;
    if (!search.assignment.is_undef(l.toVar())) continue;
    // compute SCC
    eqCurrentComponent.clear();
    // if there is any SCC, add it to SCC, if it contains more than one literal
    if (eqTarjan(l, l)) {
      didSomething = true;
    }
  }
  techniqueStop(do_ee);
  return didSomething;
}
