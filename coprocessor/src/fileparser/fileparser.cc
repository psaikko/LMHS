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
        fileparser.cc
        This file is part of riss.

        16.10.2009
        Copyright 2009 Norbert Manthey
*/

#include "fileparser/fileparser.h"
#include <cmath>
#include <climits>
#include <fstream>
#include <iostream>

FileParser::FileParser() {
  doStatistics = 8;
  print_dimacs = true;
  sortClause = false;
  forcePline = false;
  doUP = true;

  removedLiterals = 0;
  units = 0;
  satisfied = 0;
  totalLiterals = 0;
}

FileParser::FileParser(const StringMap& commandline) {
  print_dimacs = false;
  sortClause = false;
  doStatistics = 8;
  forcePline = false;
  doUP = true;

  removedLiterals = 0;
  units = 0;
  satisfied = 0;
  totalLiterals = 0;

  parse = !(commandline.contains((const char*)"--verb-help") ||
            commandline.contains((const char*)"--help"));
#ifndef TESTBINARY
#ifndef COMPETITION
  if (commandline.contains((const char*)"--verb-help") ||
      commandline.contains((const char*)"--help"))
    cerr << endl << "=== file parser information ===" << endl;
#ifdef USE_COMMANDLINEPARAMETER
  commandline.updateParameter("FP_print", print_dimacs,
                              "prints file with according p Line");
  commandline.updateParameter("FP_sort", sortClause,
                              "sort clauses before printing them");
  commandline.updateParameter(
      "FP_stat", doStatistics,
      "lenght of clauses that are counted in detail 0=off", 0, 1024);
  commandline.updateParameter("Fp_pline", forcePline,
                              "set values at least to p Line");
  commandline.updateParameter(
      "Fp_up", doUP,
      "apply UP to following clauses, if units are found in the input");
#endif
#endif
#endif
}

solution_t FileParser::parse_file(std::istream& data_in, std::ostream& err_out,
                                  searchData& search,
                                  std::vector<CL_REF>* clauses,
                                  std::unordered_map<Var, long>* whiteVars,
                                  long& tW, std::vector<CL_REF>* unit_softs,
                                  long& weightRemoved) {
  if (!parse) return UNKNOWN;

  assert(search.capacity() == 0);

  uint32_t sizeCount[doStatistics + 1];
  memset(sizeCount, 0, sizeof(uint32_t) * (doStatistics + 1));

  // use a reference in this procedure
  std::vector<CL_REF>& clause_set = *clauses;
  // used to temporarily hold soft clauses before labeling
  std::unordered_map<CL_REF, double> soft_clauses;

  std::vector<Lit> clause_lits;
  clause_lits.reserve(120);  // literals for current clause

  string line;         // current line
  uint32_t ind = 0;    // current position in current line
  uint32_t lines = 0;  // number of current line
  uint32_t var_cnt = 0;
  bool weighted = false;

  // parse every single line
  while (getline(data_in, line)) {
    lines++;
    ind = 0;

    if (line.size() == 0) continue;
    // skip spaces
    while (line[ind] == ' ') ind++;
    // skip empty lines, comments
    if (line[ind] == 10 || line[ind] == 13 || line[ind] == 0 ||
        line[ind] == 'c' || line[ind] == 'C')
      continue;

    if (line[ind] == 'p') {
      stringstream ss(line);
      ss.get();
      string type;
      uint32_t vc, cc;
      ss >> type >> vc >> cc;

      weighted = (type == "wcnf");

      if (weighted) {
        if (!(ss >> tW)) {
          tW = INT_MAX;
          ss.clear();  // remove error flag
        }
      } else {
        tW = INT_MAX;
      }
      continue;
    }

    // empty clause -> UNSAT
    if (line[ind] == '0') {
      // clean up
      for (uint32_t i = 0; i < clause_set.size(); ++i) {
        Clause& cl = search.gsa.get(clause_set[i]);
        const uint32_t s = cl.getWords();
        cl.Clause::~Clause();
        search.gsa.release(clause_set[i], s);
      }
      return UNSAT;
    }

    // no number
    if (line[ind] != '-' && (line[ind] < '0' || line[ind] > '9')) {
      err_out << "c invalid file ( symbol[" << (int)line[ind] << ", "
              << line[ind] << " ] in line " << lines << " at col " << ind
              << " : " << line << " )" << endl;
      exit(1);
    }

    // needs to be a number except 0 -> scan clause
    clause_lits.clear();
    bool satisfiedClause = false;
    bool isHardC = false;
    long weight = 0;
    uint32_t clause_size = 0;
    if (weighted) {
      while (line[ind] >= '0' && line[ind] <= '9') {
        weight *= 10;
        weight += line[ind++] - '0';
      }
    } else {
      weight = 1;
    }

    // skip whitespaces
    while (line[ind] == ' ') ind++;

    isHardC = (weight == tW);

    while (line.size() > ind) {  // read each single number
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

      if (number == 0) break;
      if (negative) number = -number;

      const Lit lit1 = Lit(number);

      // check, whether current literal is higher than prevously seen ones
      var_cnt = max(var_cnt, (uint32_t)abs(number));
      if (doUP) {
        while (var_cnt >= polarities.size()) polarities.push_back(UNDEF);
      }

      if (!isHardC || !doUP || polarities[abs(number)] == UNDEF) {
        clause_lits.push_back(lit1);
        ++clause_size;
      } else if (polarities[abs(number)] == lit1.getPolarity()) {
        satisfiedClause = true;  // the current clause is satisfied
        satisfied++;
        break;
      } else {
        removedLiterals++;
      }

      // skip whitespaces
      while (line[ind] == ' ') ind++;
    }

    // do not add satisfied clauses
    if (satisfiedClause) continue;

    // empty clause:
    if (clause_size == 0) return UNSAT;

    CL_REF ref;
    if (isHardC) {
      ref = addLiterals(search, clause_lits);
      if (!ref) {
        continue;  // clause rejected as redundant
      }
    } else {
      // never reject soft clauses
      ref = search.gsa.create(&(clause_lits[0]), clause_lits.size(), false);
    }

    if (clause_size == 1 && !isHardC) {
      // MIGHT BE GROUP
      const Lit unit = clause_lits[0];
      long new_w = unit.nr() < 0 ? -weight : weight;
      if (whiteVars->count(abs(unit.nr()))) {
        long old_w = (*whiteVars)[abs(unit.nr())];
        (*whiteVars)[abs(unit.nr())] = new_w + old_w;
        if ((new_w < 0) != (old_w < 0))
          weightRemoved += min(abs(new_w), abs(old_w));
      } else {
        (*unit_softs).push_back(ref);
        (*whiteVars)[abs(unit.nr())] = new_w;
      }
      continue;
    }

    if (isHardC)
      clause_set.push_back(ref);
    else
      soft_clauses[ref] = -weight;

    if (doStatistics && clause_size <= doStatistics) sizeCount[clause_size]++;

    totalLiterals += clause_size;

    if (isHardC && doUP && clause_size == 1) {
      const Lit unit = clause_lits[0];
      polarities[unit.toVar()] = unit.getPolarity();
      units++;
    }
  }

  for (auto cl_w : soft_clauses) {
    clause_lits.clear();
    Clause& cl = search.gsa.get(cl_w.first);
    for (uint32_t j = 0; j < cl.size(); ++j)
      clause_lits.push_back(cl.get_literal(j));
    var_cnt++;
    clause_lits.push_back(Lit(var_cnt));
    (*whiteVars)[var_cnt] = cl_w.second;
    // note: addLiterals cannot reject clause_lits due to fresh label
    clause_set.push_back(addLiterals(search, clause_lits));
  }

  if (sortClause) {
    sort(*clauses, search);

    // remove duplicates!
    for (uint32_t i = clauses->size(); i >= 1; --i) {
      Clause& c1 = search.gsa.get((*clauses)[i]);
      Clause& c2 = search.gsa.get((*clauses)[i - 1]);

      if (c1.size() != c2.size()) continue;

      uint32_t j = 0;
      for (; j < c1.size(); ++j) {
        if (c1.get_literal(j) != c2.get_literal(j)) break;
      }

      if (j == c1.size()) {
        // kick last clause
        (*clauses)[i] = (*clauses)[clauses->size() - 1];
        clauses->pop_back();
      }
    }
  }

  search.init(var_cnt);
  return UNKNOWN;
}

CL_REF FileParser::addLiterals(searchData& search, Lit* clause_lits,
                               uint32_t size) const {
  // do check on clause ( double occurence, tautology )
  selectionsort<Lit>(clause_lits, size);
  uint32_t i = 2;
  for (i = size - 1; i > 0; i--) {
    // tautology?
    if (clause_lits[i] == clause_lits[i - 1].complement()) break;
    // double lits
    if (clause_lits[i] == clause_lits[i - 1]) {
      for (uint32_t j = i + 1; j < size - 1; ++j)
        clause_lits[j] = clause_lits[j + 1];
      --size;
    }
  }
  if (i == 0) {
    return search.gsa.create(clause_lits, size, false);
  }
  return 0;
}

CL_REF FileParser::addLiterals(searchData& search,
                               std::vector<Lit>& clause_lits) const {
  // do check on clause ( double occurence, tautology )
  selectionsort<Lit>(&(clause_lits[0]), clause_lits.size());
  uint32_t i = 2;
  for (i = clause_lits.size() - 1; i > 0; i--) {
    // tautology?
    if (clause_lits[i] == clause_lits[i - 1].complement()) break;
    // double lits
    if (clause_lits[i] == clause_lits[i - 1]) {
      clause_lits.erase(clause_lits.begin() + i);
    }
  }
  if (i == 0) {
    return search.gsa.create(&(clause_lits[0]), clause_lits.size(), false);
  }
  return 0;
}

void FileParser::sort(vector<CL_REF>& clauses, searchData& search) {
  int32_t n = clauses.size();
  int32_t m, s;
  // copy elements from vector
  CL_REF* a = new CL_REF[n];
  for (int32_t i = 0; i < n; i++) {
    a[i] = clauses[i];
  }
  CL_REF* original = a;  // to not delete the wrong field
  CL_REF* b = new CL_REF[n];

  // size of work fields, power of 2
  for (s = 1; s < n; s += s) {
    m = n;
    do {
      m = m - 2 * s;                         // set begin of working field
      int32_t hi = (m + s > 0) ? m + s : 0;  // set middle of working field

      int32_t i = (m > 0) ? m : 0;  // lowest position in field
      int32_t j = hi;

      int32_t stopb = m + 2 * s;  // upper bound of current work area
      int32_t currentb = i;       // current position in field for copy

      // merge two sorted fields into one
      while (i < hi && j < stopb) {
        if ((search.gsa.get(a[i])) < (search.gsa.get(a[j])))
          b[currentb++] = a[i++];
        else
          b[currentb++] = a[j++];
      }
      // copy rest of the elements
      for (; i < hi;) b[currentb++] = a[i++];

      for (; j < stopb;) b[currentb++] = a[j++];

    } while (m > 0);

    // swap fields!
    CL_REF* tmp = a;
    a = b;
    b = tmp;
  }
  // free space
  delete[] b;
  // write data back into vector
  for (int32_t i = 0; i < n; i++) {
    clauses[i] = a[i];
  }
  return;
}
