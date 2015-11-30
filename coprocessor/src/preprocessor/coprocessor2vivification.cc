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
        coprocessor2vivification.cc
        This file is part of riss.

        13.12.2011
        Copyright 2011 Norbert Manthey
*/

#include "preprocessor/coprocessor2.h"

// 	/// class that represents two timestamps of when a literal has been
// added to a clause
// 	class numberPair {
// 	public:
// 	  uint32_t in,out;
// 	  numberPair(): in(0),out(0) {}
// 	  /** the other pair has been created after this pair
// 	   */
// 	  bool consumes( const numberPair& np ) { return np.in > in && np.out <
// out; }
// 	};

bool Coprocessor2::randomizedVivification(bool firstCall) {
  bool didSomething = false;
  techniqueStart(do_vivi);
  vector<Lit> trail;
  vector<Lit> enqueue;  // store all the literals, that need to be enqueued

  uint32_t maxSize = 0;
  for (uint32_t i = 0; i < formula->size() && (unlimited || viviLimit > 0);
       ++i) {
    const CL_REF ref = (*formula)[i];
    Clause& clause = search.gsa.get(ref);
    if (clause.isIgnored()) continue;
    maxSize = clause.size() > maxSize ? clause.size() : maxSize;
  }

  maxSize = (maxSize * viviLenPercent) / 100;
  maxSize = maxSize > 2 ? maxSize : 3;  // do not process binary clauses,
                                        // because they are used for propagation

  for (uint32_t i = 0; i < formula->size() && (unlimited || viviLimit > 0);
       ++i) {
    const CL_REF ref = (*formula)[i];
    Clause& clause = search.gsa.get(ref);
    if (clause.isIgnored()) continue;
    if (clause.size() < maxSize) continue;

    assert(clause.size() > 2 &&
           "clauses for vivification have to be larger than 2");

    trail.clear();
    uint32_t seenIndex =
        0;  // literal in trail that has been propagated already
    for (uint32_t i = 0; i < clause.size(); ++i) {
      // stop, if the clause became "too" small for vivification
      if (clause.size() < 3) break;
      if (randomized) {
        uint32_t pos = rand() % (clause.size() - i);
        clause.swap_literals(i, pos);
      } else {
        // use the most frequent literal next!
        uint32_t pos = i;
        for (uint32_t j = i + 1; j < clause.size(); ++j)
          if (litOcc[clause.get_literal(i).toIndex()] <
              litOcc[clause.get_literal(j).toIndex()])
            pos = j;
        clause.swap_literals(i, pos);
      }

      Lit literal = clause.get_literal(i);
      // TODO: decide whether this check should be done for all literals!
      // if the selected literal is not unassigned, the clause can be shortened
      if (!search.assignment.is_undef(literal.toVar())) {
        if (search.assignment.is_sat(literal)) {
          for (uint32_t j = i + 1; j < clause.size(); ++j) {
            // remove the literal, remove the clause from the list
            const Lit jLit = clause.get_literal(j);
            updateRemoveLiteral(jLit);
            for (uint32_t k = 0; k < list(jLit).size(); ++k) {
              if (ref == list(jLit)[k]) {
                removeListIndex(jLit, k);
                break;
              }
            }
          }
          if (clause.size() == 2)
            search.big.removeClauseEdges(clause.get_literal(0),
                                         clause.get_literal(1));
          // remove all upper literals from the clause
          techniqueChangedLiteralNumber(
              do_vivi, (int32_t)(i + 1) - (int32_t)clause.size());
          clause.shrink(i + 1);
          updateChangeClause(ref);
          if (clause.size() == 1) enqueue.push_back(clause.get_literal(0));
          techniqueSuccessEvent(do_vivi);
          break;  // there is no need to continue on this clause
        } else {
          // the current assignment implies one literal negated -> remove this
          // literal!
          updateRemoveLiteral(literal);
          for (uint32_t k = 0; k < list(literal).size(); ++k) {
            if (ref == list(literal)[k]) {
              removeListIndex(literal, k);
              break;
            }
          }
          techniqueChangedLiteralNumber(do_vivi, -1);
          clause.remove_indexLin(i);
          updateChangeClause(ref);
          if (clause.size() == 1) enqueue.push_back(clause.get_literal(0));
          --i;
          techniqueSuccessEvent(do_vivi);
          continue;  // on this clause could be worked further
        }
      }

      // decision is the next negated literal of the clause
      literal = literal.complement();
      trail.push_back(literal);
      search.assignment.set_polarity(literal.toVar(), literal.getPolarity());
      bool conflict = false;
      while (seenIndex < trail.size()) {
        viviLimit = (viviLimit > 0) ? viviLimit - 1 : 0;
        techniqueDoCheck(do_vivi);  // get some statistics
        const Lit propagateLiteral = trail[seenIndex++];
        const vector<Lit>& aList =
            search.big.getAdjacencyList(propagateLiteral);
        for (uint32_t j = 0; j < aList.size(); ++j) {
          if (search.assignment.is_sat(aList[j]))
            continue;  // no need to add to trail
          if (search.assignment.is_unsat(aList[j])) {
            conflict = true;
            break;
          }  // there is a conflict!
          // otherwise enqueue literal
          trail.push_back(aList[j]);
          search.assignment.set_polarity(aList[j].toVar(),
                                         aList[j].getPolarity());
        }
        if (conflict) break;
      }

      // the first negatively assumed literals of the clause result in a
      // conflict -> disallow this case by shrinking the clause
      if (conflict) {
        for (uint32_t j = i + 1; j < clause.size(); ++j) {
          // remove the literal, remove the clause from the list
          const Lit jLit = clause.get_literal(j);
          updateRemoveLiteral(jLit);
          for (uint32_t k = 0; k < list(jLit).size(); ++k) {
            if (ref == list(jLit)[k]) {
              removeListIndex(jLit, k);
              break;
            }
          }
        }
        if (clause.size() == 2)
          search.big.removeClauseEdges(clause.get_literal(0),
                                       clause.get_literal(1));
        // remove all upper literals from the clause
        techniqueChangedLiteralNumber(
            do_vivi, (int32_t)(i + 1) - (int32_t)clause.size());
        clause.shrink(i + 1);
        updateChangeClause(ref);
        techniqueSuccessEvent(do_vivi);
        if (clause.size() == 1) enqueue.push_back(clause.get_literal(0));
        break;
      }
    }

    for (uint32_t i = 0; i < trail.size(); ++i) {
      search.assignment.undef_variable(trail[i].toVar());
    }
  }

  for (uint32_t i = 0; i < enqueue.size(); ++i) {
    if (!enqueTopLevelLiteral(enqueue[i])) {
      solution = UNSAT;
      return didSomething;
    }
  }

  techniqueStop(do_vivi);
  return didSomething;
}