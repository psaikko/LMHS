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
        randomizer.h
        This file is part of riss.

        12.12.2011
        Copyright 2011 Norbert Manthey
*/

#ifndef _RANDOMIZER_H
#define _RANDOMIZER_H

/** this class re-implements the rand-method so that it can be used more
 * independently
 */
#include <stdint.h>
class Randomizer {
  uint64_t hold;

 public:
  Randomizer() {
    hold = 0;
  };

  /** sets the current random value
   */
  void set(uint64_t newValue) { hold = newValue; }

  /** return the next random value
   */
  uint64_t rand() {
    return (((hold = hold * 214013L + 2531011L) >> 16) & 0x7fff);
  }

  /** return the next random value modulo some other value
   */
  uint64_t rand(uint64_t upperBound) {
    uint64_t ret = rand();
    return upperBound > 0 ? ret % upperBound : 0;
  }
};

#endif