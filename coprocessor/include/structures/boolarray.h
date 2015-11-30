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
	boolarray.h
	This file is part of riss.
	
	16.10.2009
	Copyright 2009 Norbert Manthey
*/


#ifndef _BOOLARRAY_H
#define _BOOLARRAY_H

#include "defines.h"
#include "macros/malloc_macros.h"


#include <inttypes.h>	// for uint32_t
#include <string.h>
#include <stdlib.h>
#include <assert.h>

class BoolArray{
  private:
    uint8_t* boolarray;
    
  public:
  // initially set to false
  BoolArray(const uint32_t elements);
  
  BoolArray();

  // initialize the array
  void init(const uint32_t elements);
  
  /** return a copy of this array
  */
  BoolArray copy(const uint32_t elements) const ;

  /** copy the data from the other bool array into the current one
  */
  void copyFrom(const BoolArray& ba, const uint32_t elements);

  void destroy();

  void clear(const uint32_t elements);

  bool get( const uint32_t ind ) const ;

  void set( const uint32_t ind, const bool value );

  void extend(const uint32_t elements, const uint32_t newElements);

};


/*****

	developer section

****/
#ifndef BITSTRUCTURES

inline BoolArray::BoolArray(const uint32_t elements)
  : boolarray(0)
{
  // get memory and clear it
  boolarray = (uint8_t*)MALLOC( elements * sizeof(uint8_t) );
  memset( boolarray, 0, elements * sizeof(uint8_t) );
}

inline BoolArray::BoolArray() : boolarray(0) {}

inline void BoolArray::init(const uint32_t elements){
    assert( boolarray == 0 );
    // get memory and clear it
    boolarray = (uint8_t*)MALLOC( elements * sizeof(uint8_t) );
    memset( boolarray, 0, elements * sizeof(uint8_t) );   
}

inline BoolArray BoolArray::copy(const uint32_t elements) const
{
  BoolArray ba;
  ba.boolarray = (uint8_t*)MALLOC( elements * sizeof(uint8_t));
  memcpy( ba.boolarray, boolarray, elements * sizeof(uint8_t) );
  return ba;
}

inline void BoolArray::copyFrom(const BoolArray& ba, const uint32_t elements)
{
  memcpy( boolarray, ba.boolarray, elements * sizeof(uint8_t) );
}

inline void BoolArray::extend(const uint32_t elements, const uint32_t newElements){
	if ( boolarray != 0 )
	{
	  // get new memory
	  boolarray = (uint8_t*)realloc( boolarray, (newElements ) * sizeof(uint8_t) );
	  
	  // set new variables to 0
	  if( newElements > elements ){
		  const uint32_t diff = newElements - elements;
		  memset( ((uint8_t*)boolarray)+elements, 0, diff * sizeof(uint8_t) );
	  }
	} else {
	  boolarray = (uint8_t*)malloc( newElements * sizeof(uint8_t) );
	  memset( boolarray, 0, newElements * sizeof(uint8_t) );
	}
}

inline void BoolArray::destroy()
{
	if( boolarray != 0 )
	{
		free(boolarray);
		boolarray = 0;
	}
}

inline void BoolArray::clear(const uint32_t elements){
	assert( boolarray != 0 );
	memset( boolarray, 0, elements * sizeof(uint8_t) );
}

inline bool BoolArray::get( const uint32_t ind ) const
{
    assert( boolarray != 0 );
    return ((uint8_t*)boolarray)[ ind ]==1;
}

inline void BoolArray::set( const uint32_t ind, const bool value )
{
	assert( boolarray != 0 );
	((uint8_t*)boolarray)[ ind ] = value == true ? 1 : 0;
}

#else // BITSTRUCTURES

inline BoolArray::BoolArray(const uint32_t elements)
  : boolarray(0)
{
  // get memory and clear it
  const uint32_t eles = (elements + 7) / 8;
  boolarray = (uint8_t*)MALLOC( eles * sizeof(uint8_t) );
  memset( boolarray, 0, eles * sizeof(uint8_t) );
}

inline BoolArray::BoolArray() : boolarray(0) {}

inline void BoolArray::init(const uint32_t elements){
  assert( boolarray == 0 );
  // get memory and clear it
  const uint32_t eles = (elements + 7) / 8;
  boolarray = (uint8_t*)MALLOC( eles * sizeof(uint8_t) );
  memset( boolarray, 0, eles * sizeof(uint8_t) );
}

inline BoolArray BoolArray::copy(const uint32_t elements) const
{
  BoolArray ba;
  const uint32_t eles = (elements + 7) / 8;
  ba.boolarray = (uint8_t*)MALLOC( eles * sizeof(uint8_t));
  memcpy( ba.boolarray, boolarray, eles * sizeof(uint8_t) );
  return ba;
}

inline void BoolArray::copyFrom(const BoolArray& ba, const uint32_t elements)
{
  const uint32_t eles = (elements + 7) / 8;
  memcpy( boolarray, ba.boolarray, eles * sizeof(uint8_t) );
}

inline void BoolArray::extend(const uint32_t elements, const uint32_t newElements){
	const uint32_t neweles = (newElements + 7) / 8;
	if ( boolarray != 0 )
	{
	  // get new memory
	  const uint32_t eles = (elements + 7) / 8;
 
	  //std::cerr << "c get " << neweles << " new bytes" << std::endl;
	  boolarray = (uint8_t*)realloc( boolarray, (neweles ) * sizeof(uint8_t) );
	  
	  // set new variables to 0
	  if( newElements > elements ){
		  const uint32_t diff = neweles - eles;
		  memset( ((uint8_t*)boolarray)+eles, 0, diff * sizeof(uint8_t) );
		  
		  // take care of the intermediate byte! at most 7 more bits to set!
		  const uint32_t min = newElements < ((elements + 7) & ~7) - 1 ? newElements : ((elements + 7) & ~7) - 1;
		  for( uint32_t i = elements; i < min; ++i ) {
		   // std::cerr << "c set " << i << " / " << min << std::endl;
		    set(i,false);
		  }
	  }
	} else {
	  boolarray = (uint8_t*)malloc( neweles * sizeof(uint8_t) );
	  memset( boolarray, 0, neweles * sizeof(uint8_t) );
	}
}

inline void BoolArray::destroy()
{
	if( boolarray != 0 )
	{
		free(boolarray);
		boolarray = 0;
	}
}

inline void BoolArray::clear(const uint32_t elements){
	assert( boolarray != 0 );
	const uint32_t eles = (elements + 7) / 8;
	memset( boolarray, 0, eles * sizeof(uint8_t) );
}

inline bool BoolArray::get( const uint32_t ind ) const
{
    assert( boolarray != 0 );
    const uint32_t index = ind >> 3;
    const uint32_t offset = ind & 7;
    return ((((uint8_t*)boolarray)[ index ] & (1 << offset)) != 0);
    //return ((uint8_t*)boolarray)[ ind ]==1;
}

inline void BoolArray::set( const uint32_t ind, const bool value )
{
  assert( boolarray != 0 );

  const uint32_t index = ind >> 3;
  const uint32_t offset = ind & 7;
  uint8_t& byte = ((uint8_t*)boolarray)[ index ];
  byte = (value == true) ? (byte | 1 << offset) : (byte & ~(1 << offset));
  
  // = value == true ? 1 : 0;
}

#endif // BITSTRUCTURES

#endif
