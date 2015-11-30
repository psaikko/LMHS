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
        coprocessor2rewriting.cc
        This file is part of riss.

        02.01.2012
        Copyright 2012 Norbert Manthey
*/

#include "preprocessor/coprocessor2.h"
#include <cmath>
#include <algorithm>

/** encode the 2-product encoding for the at-most-k constraint!
 *
 * if size is less than naiveLimit, use direct at-most-one for recursion!
 *
 */
void twoProductEncode(searchData& search, Lit* literals, const uint32_t n,
                      vector<CL_REF>& clauses, Var& variables,
                      unsigned naiveLimit = 3) {
  if (n < 2) return;
  if (n <= naiveLimit) {
    // naive
    Lit ab[2];
    for (unsigned i = 0; i < n; ++i) {
      for (unsigned j = i + 1; j < n; ++j) {
        ab[0] = literals[i].complement();
        ab[1] = literals[j].complement();
        clauses.push_back(search.gsa.create(literals, 2, false));
      }
    }
    return;
  }

  unsigned p = sqrt(n - 1) + 1;
  unsigned q = ((n - 1) / p) + 1;

  // create variables Ui and Vi
  vector<Lit> U;
  vector<Lit> V;
  for (unsigned i = 0; i < p; ++i) U.push_back(Lit(++variables, POS));
  for (unsigned i = 0; i < q; ++i) V.push_back(Lit(++variables, POS));

  Lit ab[2];
  for (unsigned k = 0; k < n; ++k) {
    unsigned i = k / q;
    unsigned j = k - (i * q);
    ab[0] = literals[k].complement();
    ab[1] = U[i];
    clauses.push_back(search.gsa.create(literals, 2, false));
    ab[0] = literals[k].complement();
    ab[1] = V[j];
    clauses.push_back(search.gsa.create(literals, 2, false));
  }
  twoProductEncode(search, &(U[0]), U.size(), clauses, variables, naiveLimit);
  twoProductEncode(search, &(V[0]), U.size(), clauses, variables, naiveLimit);
}

bool Coprocessor2::rewriteAMO() {
  // extract direct encoded at most one constraints,
  // replace them bei either the regular encoding,
  // or by the product encoding

  // TODO: implement
  bool didSomething = false;

  // find 1ooN first!
  vector<CL_REF> newClauses;
  vector<CL_REF> clausesToIgnore;
  vector<vector<CL_REF> > binOcc;
  char* seen = new char[max_index(search.var_cnt)];
  memset(seen, 0, max_index(search.var_cnt));

  // initialize the binOccurence-lists
  binOcc = vector<vector<CL_REF> >();
  binOcc.reserve(max_index(search.var_cnt));
  binOcc.clear();
  for (uint32_t i = 0; i < max_index(search.var_cnt) + 1; ++i) {
    vector<CL_REF> tmp;
    tmp = vector<CL_REF>();
    binOcc.push_back(tmp);
  }

  // fill binOcc with the binary clauses
  for (uint32_t i = 0; i < formula->size(); ++i) {
    const CL_REF c = (*formula)[i];
    Clause& cl = search.gsa.get(c);
    if (cl.isIgnored()) continue;
    if (cl.size() == 2) {
      binOcc[cl.get_literal(0).toIndex()].push_back(c);
      binOcc[cl.get_literal(1).toIndex()].push_back(c);
    }
  }

  uint32_t count1ooN = 0;
  uint32_t countAtLeast1ooN = 0;
  uint32_t currentVariables = search.var_cnt;
  uint32_t maxSize = 0;
  // check all larger formula to be 1ooN
  for (uint32_t clauseK = 0; clauseK < formula->size(); ++clauseK) {
    Clause& cl = search.gsa.get((*formula)[clauseK]);
    if (cl.isIgnored()) continue;
    const uint32_t size = cl.size();
    // deleted something here
    if (size < 3 || (int32_t)size <= 8) continue;

    bool is1ooN = false;

    // check for the ordinary 1ooN encoding
    if (!is1ooN) {
      is1ooN = true;
      for (uint32_t i = 0; i < size && is1ooN; ++i) {
        // inverse literal is current literal to check whether it is combined
        // with all the other literals
        const Lit current = cl.get_literal(i).complement();

        // mark all literals that occur with clause
        const uint32_t s = search.big.getAdjacencyList(current).size();
        for (uint32_t j = 0; j < s; ++j) {
          seen[search.big.getAdjacencyList(current)[j].toIndex()] = 1;
        }
        // check clause
        uint32_t count = 0;
        for (uint32_t j = 0; j < size; ++j) {
          if (j == i) continue;
          const Lit jl = cl.get_literal(j);
          if (seen[jl.complement().toIndex()] == 1) count++;
        }
        if (count < size - 1)
          is1ooN = false;  // only if all other literals have been found it
                           // could be an 1ooN clause

        // reset marks
        for (uint32_t j = 0; j < s; ++j) {
          seen[search.big.getAdjacencyList(current)[j].toIndex()] = 0;
        }
      }
      // count, that it is a full 1ooN
      if (is1ooN) count1ooN++;
    }

    // if no 1ooN, there is nothing to do
    if (!is1ooN) {
      // if the candidate clause is the only positive occurence for all
      // literals, blocked binary clauses can be added -> it can be handled as a
      // 1ooN

      uint32_t i = 0;
      for (; i < size; ++i) {
        // inverse literal is current literal to check whether it is combined
        // with all the other literals
        const Lit current = cl.get_literal(i);
        if (list(current).size() > 1) break;
      }
      if (i == size) {
        is1ooN = true;
        countAtLeast1ooN++;
      }
    }

    // clause is not part of 1ooN nor AtLeastOne
    if (!is1ooN) continue;

    maxSize = size > maxSize ? size : maxSize;

    // otherwise, replace found tuple!
    Lit lits[size];
    for (uint32_t i = 0; i < size; ++i) {
      lits[i] = cl.get_literal(i);
    }

    // sort lits according to their occurence! [highest, any, ... , any, second
    // highest ]
    uint32_t p = 0;
    uint32_t o = binOcc[lits[p].toIndex()].size();
    for (uint32_t i = 0; i < size; ++i) {
      if (binOcc[lits[i].toIndex()].size() > o) {
        p = i;
        o = binOcc[lits[p].toIndex()].size();
      }
    }
    Lit tmp = lits[0];
    lits[0] = lits[p];
    lits[p] = tmp;
    p = 1;
    o = binOcc[lits[p].toIndex()].size();
    for (uint32_t i = 1; i < size; ++i) {
      if (binOcc[lits[i].toIndex()].size() > o) {
        p = i;
        o = binOcc[lits[p].toIndex()].size();
      }
    }
    tmp = lits[size - 1];
    lits[size - 1] = lits[p];
    lits[p] = tmp;

    // using marks for complementary literals of the 1ooN
    for (uint32_t i = 0; i < size; ++i) {
      seen[lits[i].complement().toIndex()] = 1;
    }

    for (uint32_t i = 0; i < size; ++i) {
      // inverse literal is current literal to check whether it is combined with
      // all the other literals
      const Lit current = lits[i].complement();

      for (uint32_t j = 0; j < list(current).size(); ++j) {
        Clause& jcl = search.gsa.get(list(current)[j]);
        if (jcl.isIgnored() || jcl.size() != 2) continue;
        if (seen[jcl.get_literal(0).toIndex()] == 1 &&
            seen[jcl.get_literal(1).toIndex()] == 1)
          clausesToIgnore.push_back(list(current)[j]);
      }
    }

    // reset marks
    for (uint32_t i = 0; i < size; ++i) {
      seen[lits[i].complement().toIndex()] = 0;
    }

    Var biggestV = currentVariables;
    // regular encoding?! (because it is done in the paper)
    if (size < 43) {
      // remove 1ooN representing clauses from formula
      clausesToIgnore.push_back((*formula)[clauseK]);
      // introduce new variables and clauses
      Var firstNewV = biggestV + 1;

      {
        Lit l[2];
        // encode: lits[0] = true and [-(new-i+1)] = [-(new-1)]
        l[0] = lits[0].complement();
        l[1] = Lit(firstNewV + 1, NEG);
        newClauses.push_back(search.gsa.create(l, 2, false));
        l[0] = lits[0];
        l[1] = Lit(firstNewV + 1, POS);
        newClauses.push_back(search.gsa.create(l, 2, false));
      }
      for (uint32_t i = 1; i < size - 1; ++i) {
        Lit l[3];
        // encode: lits[i] = [new-i] and [-(new-i+1)]
        l[0] = lits[i].complement();
        l[1] = Lit(firstNewV + i, POS);
        newClauses.push_back(search.gsa.create(l, 2, false));

        l[0] = lits[i].complement();
        l[1] = Lit(firstNewV + i + 1, NEG);
        newClauses.push_back(search.gsa.create(l, 2, false));

        l[0] = lits[i];
        l[1] = Lit(firstNewV + i, NEG);
        l[2] = Lit(firstNewV + i + 1, POS);
        newClauses.push_back(search.gsa.create(l, 3, false));
      }
      {
        Lit l[2];
        // encode: lits[size-1] = [(new-i+1)] and not false = [-(new-1)]
        l[0] = lits[size - 1].complement();
        l[1] = Lit(firstNewV + size - 1, POS);
        newClauses.push_back(search.gsa.create(l, 2, false));
        l[0] = lits[size - 1];
        l[1] = Lit(firstNewV + size - 1, NEG);
        newClauses.push_back(search.gsa.create(l, 2, false));
      }
      // encode order of new variables
      for (uint32_t i = 1; i + 1 < size; ++i) {
        Lit l[2];
        l[0] = Lit(firstNewV + i, POS);
        l[1] = Lit(firstNewV + i + 1, NEG);
        newClauses.push_back(search.gsa.create(l, 2, false));
      }
      {  // unit for smallest value
        Lit l = Lit(firstNewV, POS);
        newClauses.push_back(search.gsa.create(&l, 1, false));
      }

    } else {  // 2 product encoding, which is much better for bigger formulas!

      twoProductEncode(search, lits, size, newClauses, currentVariables, 3);
    }

  }  // continue with looking for the next 1ooN

  // only handle everything, if there have been some 1ooNs
  if (count1ooN > 0 || countAtLeast1ooN > 0) {
    // get space for all needed variables
    extendStructures(currentVariables);

    // ignore all clauses that belong to 1ooN encodings

    for (uint32_t i = 0; i < clausesToIgnore.size(); ++i) {
      search.gsa.get(clausesToIgnore[i]).ignore();
    }
    for (uint32_t i = 0; i < newClauses.size(); ++i) {
      if (!addClause(newClauses[i])) {
        solution = UNSAT;
      }
      formula->push_back(newClauses[i]);
    }
  }

  return count1ooN;

  return didSomething;
}

bool Coprocessor2::rewriteAMK() {
  // TODO: implement
  bool didSomething = false;
  return didSomething;
}
