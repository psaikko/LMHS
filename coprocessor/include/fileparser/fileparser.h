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
        fileparser.h
        This file is part of riss.

        16.10.2009
        Copyright 2009 Norbert Manthey
*/

#ifndef _ITER_FILE_PARSER_H
#define _ITER_FILE_PARSER_H

#include <iosfwd>
#include <assert.h>
#include <string>

#include "utils/stringmap.h"
#include "structures/clause.h"
#include "utils/clause_container.h"
#include "structures/clause.h"
#include <vector>
#include <unordered_map>
#include "utils/sort.h"
#include "defines.h"

using namespace std;

/** This class implements an file parser, that can parse cnf files
*
*	This parser does not need the p-line of the cnf file before it scans any
*clauses.
*/
class FileParser {
 protected:
  // parameter
  bool parse;         /// whether to really parse the file
  bool forcePline;    /// if ther eis a pline, set the limits to at least these
                      /// values
  bool print_dimacs;  /// whether to directly output the result again
  bool sortClause;    /// sort the clause before printing it?
  bool doUP;          /// apply UP, if unit clauses are found in the input
  uint32_t doStatistics;  /// measure and display clause sizes after parsing

  uint32_t removedLiterals;  /// literals that are removed due to unit
                             /// propagation
  uint32_t units;            /// found unit clauses
  uint32_t satisfied;        /// clauses that are satisfied
  uint32_t totalLiterals;    /// number of literals that have been added to the
                             /// formula

  vector<Polarity> polarities;  // whenever a unit clause is encountered, store
                                // its polarity here, build reduct with this
                                // vector
 public:
  /** Constructor, setup default configuration without parameter
  */
  // LIB_VIRTUAL
  FileParser();

  /** Constructor, initialized the parameter
  */
  //	LIB_VIRTUAL
  FileParser(const StringMap& commandline);

  /** parses the file, fills the formula and returns a solution, if found
  * @param filename name of the file that contains the cnf
  * @param var_vnt number of variables, that occur in the formula
  * @param clauses the created formula
  * @return a solution for the formula (UNSAT,SAT,UNKNOWN)
  */
  LIB_VIRTUAL solution_t
      parse_file(std::istream& data_in, std::ostream& err_out,
                 searchData& search, std::vector<CL_REF>* clauses,
                 std::unordered_map<Var, long>* whiteVars, long& tW,
                 std::vector<CL_REF>* potential_groups, long& weightRemoved);

 protected:
  /** created a clause out of the given literals
  *
  * @param clause_lits vector that stores the literals of the clause
  */
  CL_REF addLiterals(searchData& search, std::vector<Lit>& clause_lits) const;

  /** created a clause out of the given literals
  *
  * @param clause_lits array that stores the literals of the clause
  * @param size size of the array
  */
  CL_REF addLiterals(searchData& search, Lit* clause_lits, uint32_t size) const;

  /// sort the vector with clauses (literals and clauses)
  void sort(vector<CL_REF>& clauses, searchData& search);
};

#endif
