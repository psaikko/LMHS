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
        coprocessor2.cc
        This file is part of riss.

        05.12.2011
        Copyright 2011 Norbert Manthey
*/

#include "preprocessor/coprocessor2.h"

bool Coprocessor2::compress() {

  // enqueue all pure literals
  eliminatePure(true);
  // propagate everything
  propagateTopLevel();
  if (solution == UNSAT) return false;

  Compression compression;
  compression.variables = search.var_cnt;
  compression.mapping = (Var*)malloc(sizeof(Var) * (search.var_cnt + 1));
  memset(compression.mapping, 0, sizeof(Var) * (search.var_cnt + 1));

  uint32_t count[search.var_cnt + 1];
  memset(count, 0, sizeof(uint32_t) * (search.var_cnt + 1));
  // count literals occuring in clauses
  for (uint32_t i = 0; i < formula->size(); ++i) {
    Clause& clause = search.gsa.get((*formula)[i]);
    if (clause.isIgnored()) continue;
    uint32_t j = 0;
    for (; j < clause.size(); ++j) {
      const Lit l = clause.get_literal(j);
      if (search.assignment.is_sat(l)) {
        clause.markToDelete();
        break;
      } else if (search.assignment.is_unsat(l)) {
        clause.remove_index(j);
        j--;
        continue;
      }

      assert(!search.assignment.is_unsat(l));
      assert(l.toVar() <= search.var_cnt);

      count[l.toVar()]++;
    }

    assert(clause.size() != 1 ||
           search.assignment.is_sat(clause.get_literal(0)));
    assert(clause.size() != 1 || clause.isIgnored());
    // found empty clause?
    if (clause.size() == 0) {
      solution = UNSAT;
      return false;
    }
  }
  // which variables can be replaced because they are assigned already or
  // eliminated
  uint32_t tmpCount = 0;
  for (Var v = 1; v <= search.var_cnt; v++) {
    if (!search.assignment.is_undef(v) || search.eliminated.get(v) ||
        eqReplaced.get(v)) {
      count[v] = 0;
      tmpCount++;
    }
  }

  uint32_t diff = 0;
  for (Var v = 1; v <= search.var_cnt; v++) {
    if (count[v] != 0) {
      // setup new variable, store activity right
      compression.mapping[v] = v - diff;
    } else {
      diff++;
      assert(compression.mapping[v] == 0);
    }
  }

  // formula is already compact
  if (diff == 0) return UNKNOWN;

  // store old assignment, clear eliminated information
  assert(search.equivalentTo.size() > search.var_cnt);
  compression.previous = search.assignment.copy(search.var_cnt);
  compression.equivalentTo = search.equivalentTo;

  // replace everything in the clauses
  for (uint32_t i = 0; i < formula->size(); ++i) {
    Clause& clause = search.gsa.get((*formula)[i]);
    if (clause.isIgnored()) continue;

    for (uint32_t j = 0; j < clause.size(); ++j) {
      const Lit l = clause.get_literal(j);
      assert(compression.mapping[l.toVar()] != 0);
      const Polarity p = l.getPolarity();
      const Var v = compression.mapping[l.toVar()];
      clause.set_literal(j, Lit(v, p));
      // polarity of literal has to be kept
      assert(clause.get_literal(j).getPolarity() == p);
    }
  }

  // invert mapping

  // compress bveBlockVariables, so that they can still be used
  for (uint32_t i = 0; i < bveBlockVariables.size(); ++i) {
    // what happens if BVE-blocked variable disappears? -> remove (below)
    bveBlockVariables[i] = compression.mapping[bveBlockVariables[i]];
  }

  for (Var v = 1; v <= search.var_cnt; v++) {
    if (compression.mapping[v] != 0) {
      search.activities[compression.mapping[v]] = search.activities[v];
      search.assignment.set_backup(compression.mapping[v],
                                   search.assignment.get_backup_polarity(v));
      compression.mapping[compression.mapping[v]] = v;
      // only set to 0, if needed afterwards
      if (compression.mapping[v] != v) compression.mapping[v] = 0;
    }
    occurrenceList[Lit(v, POS).toIndex()].clear();
    occurrenceList[Lit(v, NEG).toIndex()].clear();
    search.big.clearAdjacency(Lit(v, POS));
    search.big.clearAdjacency(Lit(v, NEG));
    search.VAR_LEVEL(v) = -1;
    search.VAR_REASON(v) = reasonStruct();
  }

  // compress bveBlockVariables, so that they can still be used
  for (uint32_t i = 0; i < bveBlockVariables.size(); ++i) {
    // what happens if BVE-blocked variable disappears? remove!
    if (compression.mapping[bveBlockVariables[i]] == 0) {
      bveBlockVariables.erase(bveBlockVariables.begin() + i);
      --i;
    }
  }

  // store compression on postprocess stack
  postEle pe;
  pe.kind = 'd';
  pe.d = compression;
  postProcessStack.push_back(pe);

  // only undefine but do not touch the phase saving polarity
  for (Var v = 1; v <= search.var_cnt; ++v) {
    search.assignment.undef_variable(v);
  }
  // search.assignment.clear( search.var_cnt );
  search.eliminated.clear(search.var_cnt + 1);
  for (Var v = 1; v < search.equivalentTo.size(); v++)
    search.equivalentTo[v] = Lit(v, POS);
  // clear the trail
  search.trail.clear();

  // should also build the BIG
  clearStructures();
  addFormula(*formula);
  // reBuildBIG();

  // tell all components, that number of variables has to be set lower
  search.var_cnt -= diff;

  return true;
}

uint32_t Coprocessor2::decompress(Assignment& assignment, uint32_t variables,
                                  Compression& compression) {
  // extend the assignment, so that is large enough
  if (variables < compression.variables) {
    assignment.extend(variables, compression.variables);
  }

  // backwards, because variables will increase - do not overwrite old values!
  for (Var v = compression.variables; v > 0; v--) {
    // use inverse mapping to restore the already given polarities (including
    // UNDEF)
    // if( !assignment.is_undef(v) ) {
    if (compression.mapping[v] != v && compression.mapping[v] != 0) {
      assignment.set_polarity(compression.mapping[v],
                              assignment.get_polarity(v));
      // unassign the old variables (because they do have nothing to do with the
      // new ones)
      assignment.undef_variable(v);
    }
    // }
  }
  // copy already found assignments into the assignment
  for (Var v = 1; v <= compression.variables; v++) {
    if (!compression.previous.is_undef(v)) {
      assignment.set_polarity(v, compression.previous.get_polarity(v));
    }
  }

  if (compression.equivalentTo.size() <= compression.variables) {
    const uint32_t oldSize = compression.equivalentTo.size();
    compression.equivalentTo.resize(compression.variables + 1, NO_LIT);
    // set the replacement for the new variables
    for (Var i = oldSize; i <= compression.variables; ++i) {
      compression.equivalentTo[i] = Lit(i, POS);
    }
  }

  return compression.variables;
}
