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
	literal_system.h
	This file is part of riss.
	
	16.10.2009
	Copyright 2009 Norbert Manthey
*/

#ifndef _LIT_SYSTEM_H
#define _LIT_SYSTEM_H

#include <assert.h>
#include <inttypes.h>
#include "types.h"

#define NO_VAR 0

// example: 
// variable: 2
// positive literal: 4	( 2*2 )
// negative literal: 5	( 2*2 ) + 1


/* inverts a specified polarity
*
* @param polarity polarity to invert
* @return inverted polarity
*
*/
inline Polarity inv_pol(  const Polarity polarity )
{
	return (Polarity)(3^(uint32_t )polarity);
}

/* return the maximal possible index of a literal, given the number of variables
*
* @param variable_count number of variables
* @return highest possible index by the used indexing system
*
*/
inline uint32_t max_index( const uint32_t variable_count ){
	return 2 * ( variable_count + 1 );
}


#endif
