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
        simpleParser.h
        This file is part of riss.

        04.07.2011
        Copyright 2011 Norbert Manthey
*/

#ifndef _SIMPLEPARSER_H
#define _SIMPLEPARSER_H

#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#include "types.h"

using namespace std;

/** parse a clause from string and store it in the literals vector
 *
 */
inline int parseClause(const string& line, vector<Lit>& literals) {
  uint32_t ind = 0;
  // skip whitespaces
  while (ind < line.size() && line[ind] == ' ') ++ind;
  // parse numbers
  while (line.size() > ind)  // read each single number
  {
    // cerr << "c FP check(" << lines << "," << ind << "): " << line[ind] <<
    // endl;
    int32_t number = 0;
    bool negative = false;

    while (ind < line.size() && line[ind] == '-') {
      negative = true;
      ind++;
    }
    // read next number here
    while (ind < line.size() && line[ind] >= '0' && line[ind] <= '9') {
      number *= 10;
      number += line[ind++] - '0';
    }

    if (number == 0)
      break;  // there may be some symbols behind a 0, but they do not matter

    // put right sign on the number
    number = (negative) ? 0 - number : number;

    const Lit lit1 = Lit(number);
    literals.push_back(lit1);

    // skip whitespaces
    while (line[ind] == ' ') ind++;
  }
  return 0;
}

/** return true, if the first string starts with the second one
 */
inline bool startsWith(const string& src, const string& match) {
  return (src.find(match) == 0);
}

/** return true, if the first character in the string is no digit
 */
inline bool isNoLiteral(const string& src) {
  for (uint32_t i = 0; i < src.size(); ++i) {
    if (src[i] == ' ' || src[i] == '-') continue;
    if (src[i] >= '0' && src[i] <= '9') return false;
    return true;
  }
  // nothing in string?
  return true;
}

inline Lit parseLit(const string& src) {
  uint32_t ind = 0;
  // skip whitespaces
  while (ind < src.size() && src[ind] == ' ') ++ind;
  // parse numbers
  int32_t number = 0;
  bool negative = false;

  while (ind < src.size() && src[ind] == '-') {
    negative = !negative;
    ind++;
  }
  // read next number here
  while (src[ind] >= '0' && src[ind] <= '9') {
    number *= 10;
    number += src[ind++] - '0';
  }

  // no number found
  if (number == 0) return NO_LIT;

  // put right sign on the literal
  return Lit((negative) ? 0 - number : number);
}

#endif
