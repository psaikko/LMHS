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
	markArray.h
	This file is part of riss.
	
	6.12.2011
	Copyright 2011 Norbert Manthey
*/

#ifndef _MARKARRAY_H
#define _MARKARRAY_H


#include <inttypes.h>	// for uint8_t
#include <string.h>
#include <stdlib.h>
//#include <malloc.h>
#include <assert.h>

#include "defines.h"
#include "types.h"

class MarkArray {
private:
	uint32_t* array;
	uint32_t step;
	uint32_t mySize;

public:
	MarkArray ():
	 array(0),
	 step(0),
	 mySize(0)
	 {}
	 
	~MarkArray ()
	{
	  destroy();
	}
	 
	void destroy() {
	  if( array != 0 ) free( array );
	  mySize = 0; step = 0; array = 0;
	}
	 
	void create(const uint32_t newSize){
	  assert( array == 0 );
	  array = (uint32_t * ) malloc( sizeof( uint32_t) * newSize );
	  memset( array, 0 , sizeof( uint32_t) * newSize );
	  mySize = newSize;
	}

	void resize(const uint32_t newSize) {
	  if( newSize > mySize ) {
	    array = (uint32_t * ) realloc( array, sizeof( uint32_t) * newSize );
	    memset( &(array[mySize]), 0 , sizeof( uint32_t) * (newSize - mySize) );
	    mySize = newSize;
	  }
	}
	
	/** reset the mark of a single item
	 */
	void reset( const uint32_t index ) {
	  array[index] = 0;
	}
	
	/** reset the marks of the whole array
	 */
	void reset() {
	  memset( array, 0 , sizeof( uint32_t) * mySize );
	  step = 0;
	}
	
	/** give number of next step. if value becomes critical, array will be reset
	 */
	uint32_t nextStep() {
	  if( step >= 1 << 30 ) reset();
	  return ++step;
	}
	
	/** returns whether an item has been marked at the current step
	 */
	bool isCurrentStep( const uint32_t index ) const {
	  return array[index] == step;
	}
	
	/** set an item to the current step value
	 */
	void setCurrentStep( const uint32_t index ) {
	  array[index] = step;
	}
	
	uint32_t size() const {
	  return mySize;
	}
	
};

#endif
