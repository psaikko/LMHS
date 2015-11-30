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
	allocator.h
	This file is part of riss.
	
	27.02.2012
	Copyright 2012 Norbert Manthey
*/

#include "defines.h"

#ifndef REGIONALLOCATOR

  #include "structures/grow_slab_memory.h"
  #define Allocator GrowSlabAllocator
  
#else
  
  #include "structures/region_memory.h"
  #define Allocator RegionAllocator<Clause>
  
#endif


