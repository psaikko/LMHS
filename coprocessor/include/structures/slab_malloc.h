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
	slab_malloc.h
	This file is part of riss.
	
	25.01.2010
	Copyright 2010 Norbert Manthey
*/

#ifndef _SLAB_MALLOC_H_
#define _SLAB_MALLOC_H_

#include "structures/slab_memory.h"

// provides malloc/free/realloc functionalit1y if the size of the memory is known when freeing or reallocing memory!
// uses slab memory to handle memory


// defined in slab memory h
// #define SLAB_MALLOC_MAXSIZE 128

extern SlabSizeAllocator slab_alloc;

inline void* slab_malloc(uint32_t size)
{
	return slab_alloc.get( size );
}

inline void slab_free( void* adress, uint32_t size )
{
	slab_alloc.release( adress, size );
}

inline void* slab_realloc( void* adress, uint32_t new_size, uint32_t size )
{
	return slab_alloc.resize( adress, new_size, size );
}


#endif
