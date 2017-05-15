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
        types.h
        This file is part of riss.

        16.10.2009
        Copyright 2009 Norbert Manthey
*/

#ifndef _TYPES_H
#define _TYPES_H

// for int types
#include <inttypes.h>

#include "defines.h"
// 64bit number for very large inputs?!

/// variable type
typedef uint32_t Var;

/// weight type
typedef long weight_t;

/// polarity POS, NEG and UNDEF of a literal
typedef enum {
  POS = 1,
  NEG = 2,
  UNDEF = 0
} Polarity;

/// typedef enum for SAT, UNSAT, UNKNOWN of the solution
typedef enum {
  SAT = 10,
  UNSAT = 20,
  UNKNOWN = 0
} solution_t;

/// literal type

class Lit {
  uint32_t literal;

 public:
  Lit() : literal(0) {}
  // Lit(uint32_t d): data(d) {}

  explicit Lit(const int32_t number) {
    literal = (number < 0) ? (-2 * number + 1) : 2 * number;
  }

  Lit(const Var number, Polarity pol) {
    literal = (pol == NEG) ? (2 * number + 1) : 2 * number;
  }

  /*
  uint32_t& operator=(const uint32_t& other){
    literal = other;
    return literal;
  }
  */

  bool operator<(const Lit& other) const { return literal < other.literal; }

  bool operator>(const Lit& other) const { return literal > other.literal; }

  bool operator==(const Lit& other) const { return literal == other.literal; }

  bool operator!=(const Lit& other) const { return literal != other.literal; }

  /*
int32_t& operator=(const Lit& other){
literal = other.literal;
return (int32_t&)literal;
}
*/

  Lit complement() const {
    Lit t;
    t.literal = literal ^ 1;
    return t;
  }

  Polarity getPolarity() const { return (Polarity)(1 + (literal & 0x1)); }

  bool isPositive() const { return (literal & 0x1) == 0; }

  Var toVar() const { return literal / 2; }

  uint32_t toIndex() const { return literal; }

  int32_t nr() const {
    return isPositive() ? (literal / 2) : (((int32_t)literal) / 2 * -1);
  }

  uint32_t data() const { return literal; }

  /// NOTE: be careful with this method, trz to avoid usage!
  void fromData(uint32_t data) { literal = data; }
};

const Lit NO_LIT = Lit(0);
const Lit FAIL_LIT = Lit(1);

// typedef uint32_t Lit;

/*****
        implementation of compress commands
*****/

#ifndef COMPRESS_Clause
#define CL_PACK __attribute__((packed))
#else
#define CL_PACK
#endif

#ifdef COMPRESS_WATCH_STRUCTS
#define WATCH_STRUCT_PACK __attribute__((packed))
#else
#define WATCH_STRUCT_PACK
#endif

#endif
