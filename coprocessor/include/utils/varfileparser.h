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
        varfileparser.h
        This file is part of riss.

        26.10.2010
        Copyright 2010 Norbert Manthey
*/

#ifndef _VARFILEPARSER_H
#define _VARFILEPARSER_H

#include <iostream>

#include <fstream>
#include <sstream>
#include <string>
// for int types
#include <inttypes.h>
#include <cstdlib>
#include <cstring>

#include "types.h"
#include <vector>

using namespace std;

/** parse files, that contain variables or variable ranges
 *
 *  Format of file: per line a variable, of a variable range per "a..b"
 */
class VarFileParser {
  std::fstream file;

 public:
  /** open the specified file
  */
  VarFileParser(const string& filename);

  /** extract the variables from the file and store them in the vector vars
  */
  void extract(std::vector<Var>& vars);
};

/*****
        developer section
****/

inline void VarFileParser::extract(std::vector<Var>& vars) {
  file.seekg(0);
  std::string line;
  while (getline(file, line)) {
    if (line.size() == 0) continue;
    // ignore comment lines
    if (line.at(0) == 'c') continue;
    if (line.find('.') != string::npos) {
      // range of numbers
      uint32_t dotPos = line.find('.');
      std::string first = line.substr(0, line.find('.'));
      Var firstVar = atoi(first.c_str());

      line = line.substr(dotPos + 2);
      Var secondVar = atoi(line.c_str());
      for (Var v = firstVar; v <= secondVar; v++) vars.push_back(v);
    } else {
      // single number
      vars.push_back(atoi(line.c_str()));
    }
  }
}

inline VarFileParser::VarFileParser(const string& filename) {
  file.open(filename.c_str(), std::ios_base::in);
  if (!file.is_open()) {
    cerr << "c variable parser was not able to open file " << filename << endl;
  }
}

#endif
