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
        simplePapi.h
        This file is part of riss.

        23.02.2012
        Copyright 2012 Norbert Manthey
*/

#ifndef SIMPLE_PAPI_H
#define SIMPLE_PAPI_H

#ifndef USE_SIMPLE_PAPI

class SimplePapi {

 public:
  SimplePapi() {}
  bool isEnabled() { return false; }
  bool init() { return true; }
  bool start() { return true; }
  bool stop() { return true; }
  double getIPC() { return 0.0; }
  double getL2missrate() { return 0.0; }
  double getL3missrate() { return 0.0; }
};

#else

#include "../../../projects/PAPI/papiC/src/papi.h"

class SimplePapi {

  /* available events
  PAPI_TOT_INS 0x80000032  Yes   No   Instructions completed
  PAPI_TOT_CYC 0x8000003b  Yes   No   Total cycles

  PAPI_L2_TCM  0x80000007  Yes   No   Level 2 cache misses
  PAPI_L3_TCM  0x80000008  Yes   No   Level 3 cache misses

  PAPI_L2_TCA  0x80000059  Yes   Yes  Level 2 total cache accesses
  PAPI_L3_TCA  0x8000005a  Yes   No   Level 3 total cache accesses

  PAPI_TLB_DM  0x80000014  Yes   Yes  Data translation lookaside buffer misses
  PAPI_TLB_IM  0x80000015  Yes   No   Instruction TLB misses

  PAPI_BR_CN   0x8000002b  Yes   No   Conditional branch instructions
  PAPI_BR_MSP  0x8000002e  Yes   No   Conditional branch instructions
  mispredicted

  */

  const unsigned usedCounters;
  long_long values[10];
  int num_hwcntrs;

 public:
  SimplePapi();

  bool isEnabled() { return true; }

  bool init();

  bool start();

  bool stop();

  double getIPC();

  double getL2missrate();

  double getL3missrate();
};

#endif

#endif
