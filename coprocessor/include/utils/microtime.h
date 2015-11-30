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
        microtime.h
        This file is part of riss.

        16.10.2009
        Copyright 2009 Norbert Manthey
*/

#ifndef _MICROTIME_H
#define _MICROTIME_H

#include <sys/time.h>
#include <time.h>

#include <inttypes.h>

/** return time in micro seconds (10 ^ -6 )
 * */
inline uint64_t get_microseconds() {
  timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec * 1000000 + t.tv_usec;
}

/** continue to measure on a certain timer (for first measurement, init with 0!)
 * */
inline void continueClock(uint64_t* timer) {
  *timer = get_microseconds() - *timer;
}

/** stop to measure on a certain timer
 * */
inline void stopClock(uint64_t* timer) { *timer = get_microseconds() - *timer; }

#endif
