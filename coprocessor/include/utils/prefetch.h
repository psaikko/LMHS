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
        prefetch.h
        This file is part of riss.

        16.10.2009
        Copyright 2009 Norbert Manthey
*/

#ifndef _PREFETCH_H
#define _PREFETCH_H

/// prefetch funktion which trys to copy some memory into L1 cache bevor it is
/// used by the processor
inline void prefetch(const void* p) {
#ifdef USE_PREFETCHING
#ifndef COMPILE_INTEL
  __builtin_prefetch(p);
#else
  __mm_prefetch(p);
#endif
#endif
}

#endif
