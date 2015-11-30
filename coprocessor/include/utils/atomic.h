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
        atomic.h
        This file is part of riss.

        18.08.2011
        Copyright 2011 Norbert Manthey
*/

/** This file provides some basic atomic functions.
 */

#ifndef __ATOMIC_HH
#define __ATOMIC_HH

#include <inttypes.h>

/** exchange the value at adress with newValue, if old value is assumedOldValue
 * NOTE: is atomic
 * @return true, if old value is assumedOldValue
 * */
inline bool cmpxchg8(volatile uint8_t* adress, const uint8_t assumedOldValue,
                     const uint8_t newValue) {
  return __sync_bool_compare_and_swap(adress, assumedOldValue, newValue);
}

/** exchange the value at adress with newValue, if old value is assumedOldValue
 * NOTE: is atomic
 * @return true, if old value is assumedOldValue
 * */
inline bool cmpxchg16(volatile uint16_t* adress, const uint16_t assumedOldValue,
                      const uint16_t newValue) {
  return __sync_bool_compare_and_swap(adress, assumedOldValue, newValue);
}

/** exchange the value at adress with newValue, if old value is assumedOldValue
 * NOTE: is atomic
 * @return true, if old value is assumedOldValue
 * */
inline bool cmpxchg32(volatile uint32_t* adress, const uint32_t assumedOldValue,
                      const uint32_t newValue) {
  return __sync_bool_compare_and_swap(adress, assumedOldValue, newValue);
}

/** exchange the value at adress with newValue, if old value is assumedOldValue
 * NOTE: is atomic
 * @return the old value of the adress
 * */
inline uint32_t cmpxchg8val(volatile uint8_t* adress,
                            const uint8_t assumedOldValue,
                            const uint8_t newValue) {
  return __sync_val_compare_and_swap(adress, assumedOldValue, newValue);
}

/** exchange the value at adress with newValue, if old value is assumedOldValue
 * NOTE: is atomic
 * @return the old value of the adress
 * */
inline uint32_t cmpxchg16val(volatile uint16_t* adress,
                             const uint16_t assumedOldValue,
                             const uint16_t newValue) {
  return __sync_val_compare_and_swap(adress, assumedOldValue, newValue);
}

/** exchange the value at adress with newValue, if old value is assumedOldValue
 * NOTE: is atomic
 * @return the old value of the adress
 * */
inline uint32_t cmpxchg32val(volatile uint32_t* adress,
                             const uint32_t assumedOldValue,
                             const uint32_t newValue) {
  return __sync_val_compare_and_swap(adress, assumedOldValue, newValue);
}

/** increament a uint32_t value (thread-save)
 * @param adress adress of the value to increment
 * @return value after operation
 */
inline uint32_t inc32ts(uint32_t* adress) {
  volatile uint32_t old;
  do {
    old = *adress;
  } while (!cmpxchg32(adress, old, old + 1));
  return old + 1;
}

/** decrement a uint32_t value (thread-save)
 * @param adress adress of the value to decrement
 * @return value after operation
 */
inline uint32_t dec32ts(uint32_t* adress) {
  volatile uint32_t old;
  do {
    old = *adress;
  } while (!cmpxchg32(adress, old, old - 1));
  return old - 1;
}

#endif
