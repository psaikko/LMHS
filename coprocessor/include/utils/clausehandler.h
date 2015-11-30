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
        clausehandler.h
        This file is part of riss.

        20.02.2010
        Copyright 2010 Norbert Manthey
*/

#ifndef _ClauseHANDLER_H
#define _ClauseHANDLER_H

#include <iostream>

#include <stdlib.h>
#include <assert.h>
#include <fstream>
#include <sstream>
#include <string>
// for int types
#include <inttypes.h>

#include <vector>
#include "structures/clause.h"
#include "structures/cpp_vector.h"
#include "utils/clause_container.h"
#include "structures/literal_system.h"

#include "utils/sort.h"

/** writes clauses to file or reads clauses from file
*/
class ClauseHandler {
  std::fstream file;
  std::string name;

 public:
  ClauseHandler(const char* filename);
  ~ClauseHandler();

  void write(const Clause& clause);
  void read(searchData& search, CppVector<CL_REF>& clauses);
};

/*****
        developer section
****/

inline ClauseHandler::ClauseHandler(const char* filename) : name(filename) {}

inline ClauseHandler::~ClauseHandler() { file.close(); }

inline void ClauseHandler::write(const Clause& clause) {
  if (!file.is_open()) {
    file.open(name.c_str(), std::ios_base::out | std::ios_base::trunc);
  }
  const Clause& cl = clause;
  for (uint32_t i = 0; i < cl.size(); ++i) {
    file << cl.get_literal(i).nr() << " ";
  }
  file << "0" << std::endl;
}

inline void ClauseHandler::read(searchData& search,
                                CppVector<CL_REF>& clauses) {
  // close the file (might be opend from writing with wrong flags)
  if (file.is_open()) file.close();
  // open file and start reading from the first byte
  file.open(name.c_str(), std::ios_base::in);
  // if there is no file, there is nothing to read
  if (!file.is_open()) return;

  file.seekg(0);

  // read all the clauses
  std::string line;
  uint32_t lines = 0;
  uint32_t ind = 0;
  std::vector<Lit> clause_lits;
  clause_lits = std::vector<Lit>();
  clause_lits.reserve(120);  // literals for current clause
  uint32_t var_cnt = 0;
  while (getline(file, line)) {
    lines++;
    ind = 0;
    if (line.size() == 0) continue;
    // skip spaces
    while (line[ind] == ' ') ind++;
    // skip empty lines
    if (line[ind] == 10 || line[ind] == 13 || line[ind] == 0) continue;
    if (line[ind] == 'p')  // skip
      continue;
    if (line[ind] == 'c' || line[ind] == 'C')  // skip comments
      continue;
    // empty clause -> UNSAT -> still add clause!
    if (line[ind] == '0') {
      clauses.push_back(search.gsa.create());
    }
    // no number
    if (line[ind] != '-' && (line[ind] < '0' || line[ind] > '9')) {
      std::cerr << "invalid learnt file ( symbol[" << (int)line[ind] << ", "
                << line[ind] << " ] in line " << lines << " at col " << ind
                << " : " << line << " )" << std::endl;
      continue;
    }
    clause_lits.clear();
    while (line.size() > ind)  // read each single number
    {
      int32_t number = 0;
      bool negative = false;
      if (line[ind] == '-') {
        negative = true;
        ind++;
      }
      // read next number here
      while (line[ind] >= '0' && line[ind] <= '9') {
        number *= 10;
        number += line[ind++] - '0';
      }
      if (number == 0)
        break;  // there may be some symbols behind a 0, but they do not matter
      // put right sign on the number
      number = (negative) ? 0 - number : number;

      const Lit lit1(number);
      clause_lits.push_back(lit1);
      // check, whether current literal is higher than prevously seen ones
      if (var_cnt < (uint32_t)lit1.toVar()) var_cnt = lit1.toVar();

      // skip whitespaces
      while (line[ind] == ' ') ind++;
    }
    // do check on clause ( double occurence, tautology )
    selectionsort<Lit>(&(clause_lits[0]), clause_lits.size());
    uint32_t i = 2;
    for (i = clause_lits.size() - 1; i > 0; i--) {
      // tautology?
      if (clause_lits[i] == clause_lits[i - 1].complement()) break;
      // doulbe lits
      if (clause_lits[i] == clause_lits[i - 1]) {
        clause_lits.erase(clause_lits.begin() + i);
      }
    }

    if (i == 0)  // no tautology, so add the clause
    {
      clauses.push_back(
          search.gsa.create(&(clause_lits[0]), clause_lits.size()));
    }
  }
  file.clear();
  file.close();
}

#endif
