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
        searchStructs.h
        This file is part of riss.

        02.05.2011
        Copyright 2011 Norbert Manthey
*/

#ifndef _SEARCHSTRUCTS_H
#define _SEARCHSTRUCTS_H

#include "types.h"
#include "defines.h"

using namespace std;

typedef uint32_t CL_REF;

/** class to store reasons
*
*   The reason is either a clause or a single literal (implication, has to
*come from the same level)
*/
class reasonStruct {
  uint32_t data;

 public:
  // constructors for the different needs
  reasonStruct() : data(0) {}

  /** store the reason clause
  */
  explicit reasonStruct(const CL_REF reason) : data(reason << 1) {}

  /** represents a binary clause.
  *
  * The literal l, that is forced by this reason is the binary clause, is forced
  *by the clause [lit,l].
  * Thus, when this struct is created, the inverse of the propagated literal has
  *to be inserted.
  */
  explicit reasonStruct(const Lit lit) : data((lit.data() << 1) + 1) {}
  reasonStruct& operator=(const reasonStruct& other) {
    data = other.data;
    return *this;
  }

  // getter for the data
  bool isLit() const { return ((data & 1) == 1); }
  Lit getLit() const {
    Lit l;
    l.fromData(data >> 1);
    return l;
  }
  CL_REF getCL() const { return data >> 1; }
  bool empty() const { return (data == 0); }
} WATCH_STRUCT_PACK;

/** This structure stores a conflict.
*
*   It either contains a binary clause stored via two literals
*   Or it contains a CL_REF
*/
class conflictStruct {
  uint32_t data[2];

 public:
  // constructors for the different needs
  conflictStruct() {
    data[0] = 0;
    data[1] = 0;
  }
  /// store a conflict clause
  explicit conflictStruct(const CL_REF clause) {
    data[0] = clause;
    data[1] = 0;
  }
  /// store a binary conflict clause, directly in the conflictStruct
  explicit conflictStruct(const Lit propagated, const Lit impliedConflict) {
    data[0] = propagated.data() << 1;
    data[1] = (impliedConflict.data() << 1) + 1;
  }
  conflictStruct& operator=(const conflictStruct& other) {
    data[0] = other.data[0];
    data[1] = other.data[1];
    return *this;
  }

  // getter for the data
  bool isLit() const { return ((data[1] & 1) == 1); }
  Lit getLit(uint32_t ind) const {
    Lit l;
    l.fromData(data[ind] >> 1);
    return l;
  }
  Lit getPropagated() const {
    Lit l;
    l.fromData(data[0] >> 1);
    return l;
  }
  Lit getConflicted() const {
    Lit l;
    l.fromData(data[1] >> 1);
    return l;
  }
  CL_REF getCL() const { return data[0]; }
  bool empty() const { return (data[0] == 0 && data[1] == 0); }
} WATCH_STRUCT_PACK;

#endif
