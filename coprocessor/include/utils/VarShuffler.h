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
        VarShuffler.h
        This file is part of riss.

        08.03.2012
        Copyright 2012 Norbert Manthey
*/

/** This file provides some basic atomic functions.
 */

#ifndef _VAR_SHUFFLER_HH
#define _VAR_SHUFFLER_HH

#include "defines.h"
#include "sat/searchdata.h"
#include "types.h"
#include <stdlib.h>

#include "utils/randomizer.h"

class VarShuffler {
  uint32_t variables;
  vector<Lit> replacedBy;
  uint32_t seed;

  Randomizer randomizer;

 public:
  VarShuffler() : variables(0), seed(0) {}

  void setSeed(uint32_t s) {
    seed = s;
    randomizer.set(seed);
  }

  void shuffle(uint32_t vars, vector<CL_REF>& clauses, searchData& sd) {
    // shuffle only once!
    if (variables != 0) return;
    variables = vars;

    replacedBy.resize(vars, NO_LIT);
    for (Var v = 1; v <= vars; ++v) replacedBy[v] = Lit(v, POS);

    for (Var v = 1; v <= vars; ++v) {
      uint32_t r = randomizer.rand() % vars + 1;
      Lit lr = replacedBy[v];
      replacedBy[v] = replacedBy[r];
      replacedBy[r] = lr;
    }

    for (Var v = 1; v <= vars; ++v) {
      const uint32_t r = randomizer.rand() & 1;
      if (r == 1) replacedBy[v] = replacedBy[v].complement();
    }

    for (uint32_t i = 0; i < clauses.size(); ++i) {
      Clause& c = sd.gsa.get(clauses[i]);
      for (uint32_t j = 0; j < c.size(); ++j) {
        const Lit l = c.get_literal(j);
        c.set_literal(j, l.isPositive() ? replacedBy[l.toVar()]
                                        : replacedBy[l.toVar()].complement());
      }
    }
  }

  void unshuffle(Assignment& assignment) {
    // create copy of the assignment
    Assignment copy = assignment.copy(variables);

    for (Var v = 1; v <= variables; ++v) {
      /*
      cerr << "var v: " << copy.get_polarity(v) << " r=" << replacedBy[v].nr()
      << " raplacePol= "
        << ( replacedBy[v].isPositive() ?
      copy.get_polarity(replacedBy[v].toVar()) : inv_pol(
      copy.get_polarity(replacedBy[v].toVar()) ) ) << endl;
      */
      assignment.set_polarity(
          v, replacedBy[v].isPositive()
                 ? copy.get_polarity(replacedBy[v].toVar())
                 : inv_pol(copy.get_polarity(replacedBy[v].toVar())));
    }
    // destroy the copy again
    copy.destroy();
  }
};

#endif