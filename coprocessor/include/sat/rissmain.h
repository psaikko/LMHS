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
        rissmain.h
        This file is part of riss.

        09.08.2010
        Copyright 2009 Norbert Manthey
*/
#include "defines.h"
#ifdef SATSOLVER

#ifndef RISSMAIN_H
#define RISSMAIN_H

#include "utils/stringmap.h"
#include "utils/microtime.h"
#include "utils/interupt.h"

#include "structures/assignment.h"
#include "structures/literal_system.h"

#include "structures/clause.h"
#include "utils/clause_container.h"

#include "utils/simpleParser.h"
#include "utils/simplePapi.h"

#include <vector>
#include <unordered_map>
#include <iosfwd>

#include "types.h"

// for preprcoess and completemodel methods
#include "preprocessor/coprocessor2.h"
#include "fileparser/fileparser.h"

/** method to start the sat solver with the given commandline parameter
*
* @param commandline commandline parameter
* @return solution for the formula (SAT,UNSAT,UNKNOWN)
*/
int32_t rissmain(std::ostream& err_out, StringMap& commandline);

/** executes only the preprocessor to complete a model
 *
 * @param commandline runtime parameters
 */
int32_t completeModel(std::ostream& data_out, std::istream& data_in,
                      std::istream& map_in, std::ostream& err_out,
                      StringMap& commandline);

/** preprocess the formula without solving anything
 *
 * @param commandline runtime parameters
 */
int32_t preprocess(std::ostream& data_out, std::istream& data_in,
                   std::ostream& map_out, std::ostream& err_out,
                   StringMap& commandline);

/** output model to stdout or file
 * @param commandline runtime parameters to determine whether output to file or
 * not
 * @param solutions solutions of the solver, 0 -> UNSAT, no solutions -> UNSAT
 */
void printSolution(std::ostream& data_out, std::ostream& err_out,
                   StringMap& commandline, uint32_t var_cnt,
                   std::vector<Assignment>& solutions);

#endif

#endif  // #ifdef SATSOLVER
