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
        commandlineparser.h
        This file is part of riss.

        16.10.2009
        Copyright 2009 Norbert Manthey
*/

#ifndef _COMMANDLINEPARSER_H
#define _COMMANDLINEPARSER_H

#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include "utils/stringmap.h"
#include "utils/tokenizer.h"

using namespace std;

/** Parses the commandline and stores the options in a map
*/
class CommandLineParser {
  StringMap map;

  void print_sat_help();
  void print_csp_help();

 public:
  StringMap parse(int32_t argc, char const* argv[], bool binary = true);

  bool parseFile(string filename, StringMap& thisMap);
  bool parseString(const string& commands, StringMap& thisMap);
};

/*****

        developer section

****/

#endif
