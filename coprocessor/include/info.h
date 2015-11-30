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
        info.h
        This file is part of riss.

        10.12.2009
        Copyright 2009 Norbert Manthey
*/

#ifndef INFO_HH
#define INFO_HH

#include <string>
#include <inttypes.h>  // for uint64_t

#include <cstdlib>
#include <cstring>

/** This method prints information about the build process
    The source file is built while building the whole solver
*/

extern const char* builttime;
extern const char* kernel;
extern const char* revision;
extern const char* buildflags;
extern const char* hostname;

inline const char* get_builttime() { return builttime; }

inline const char* get_kernel() { return kernel; }

inline uint32_t get_revision() {
  uint32_t i = 0;
  for (; i < strlen(revision); ++i) {
    if (revision[i] == ' ') break;
  }
  return atoi(&(revision[i]));
}

inline const char* get_buildflags() { return buildflags; }

inline const char* get_hostname() { return hostname; }

inline void print_build_info() {
  /*
  std::cout << "c ========= build information ==========" << std::endl
  << "c build on: " << get_hostname() << std::endl
  << "c build time(yymmddhhmmss): " << get_builttime() << std::endl
  << "c build kernel: " << get_kernel() << std::endl
  << "c revision: " << get_revision() << std::endl
  << "c build flags: " << get_buildflags() << std::endl
  << "c ======================================" << std::endl;
  */
}

#endif
