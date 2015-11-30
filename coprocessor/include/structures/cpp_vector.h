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
	cpp_vector.h
	This file is part of riss.
	
	21.02.2012
	Copyright 2012 Norbert Manthey
*/

#ifndef _CPP_VECTOR_H
#define _CPP_VECTOR_H

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <vector>

#include <assert.h>

#include "macros/malloc_macros.h"

#if 1

template <typename T>
class CppVector {
	T* data;	// extern data, if intern data is not sufficient
	uint32_t _size;	// overall siz of the vector (number of current elements)
	uint32_t _cap;	// capacity of the vector, elements that can be stored in the currently reserved storage

public:
	CppVector();
	
	CppVector( const uint32_t elements );
	
	CppVector( const CppVector& other );
	
	~CppVector();
	
	T& operator [] (const uint32_t index);
	
	const T& operator [] (const uint32_t index) const ;

	CppVector& operator=(const CppVector& other);
	
	void assign( const CppVector& src);

	void destroy();

	void resize( const uint32_t elements, const T& new_element );

	/** reserves at least elements bytes
	*	does not decrease the storage, if the specified value is lower than the current capacity
	*/
	void reserve( const uint32_t elements );

	void reserve_another( const uint32_t more_elements );

	void push_back( const T& element );

	void push_back_another(const T* elements, const uint32_t number );
	
	void push_back_another(const CppVector<T>& elements, const uint32_t number );

	void pop_back();

	void erase(const uint32_t ind );

	void erase_no_order(const uint32_t ind );

	uint32_t size() const;

	uint32_t capacity() const;

	void clear();

	void minimize();
};

/*
	This section implements the functions listed above.
	It is done here, because compiler can not handle template functions in seperate source files.
*/

template <typename T>
inline 
CppVector<T>::CppVector()
 : data(0), _size(0), _cap(0)
{}

	
template <typename T>
inline 
CppVector<T>::CppVector( uint32_t elements )
 : data(0), _size(0), _cap(0)
{
	reserve( elements );
}
	
template <typename T>
inline 
CppVector<T>::CppVector( const CppVector& other )
: data(0), _size(0), _cap(0)
{
	reserve( other.size() + 1);
	memcpy( data, other.data,  _size * sizeof(T) );
}

template <typename T>
inline CppVector<T>::~CppVector(){
	if( data != 0 ) free(data);
}

template <typename T>
inline CppVector<T>& CppVector<T>::operator=(const CppVector& other){
	reserve( other.size() + 1);	// at least this size, might be more, if already more
	memcpy( data, other.data, other.size() * sizeof(T) );
	_size = other.size();
	return *this;
}
	
template <typename T>
inline T& CppVector<T>::operator [] (const uint32_t index){
	return data[index];
}

template <typename T>
inline const T& CppVector<T>::operator [] (const uint32_t index) const {
  return data[index];
}
	
template <typename T>
inline void CppVector<T>::assign( const CppVector& other){
	reserve( other.size() );	// at least this size, might be more, if already more
	memcpy( data, other.data,  _size * sizeof(T) );
}

template <typename T>
inline void CppVector<T>::destroy(){
	if( data != 0 ) free( data );
	_size = 0; _cap = 0;
}

template <typename T>
inline void CppVector<T>::resize(const uint32_t elements, const T& new_element ){
	// 
	if( size() >= elements ){
		_size = elements;
		// delete data? no, cap knows how many elements can be placed there
	}
	
	// get storage
	reserve( elements );
	
	// fill remaining new pieces with the new_element
	for( uint32_t i = _size; i < elements; ++i ) (*this)[i] = new_element;

}

template <typename T>
inline void CppVector<T>::reserve( const uint32_t elements ){
	assert( elements < 300000 );
	// do not remove storage
	if( elements < _cap  ) return;

	// if no memory has been allocated yet, get some, otherwise, resize the storage
	data = (data == 0) ? (T*) MALLOC( elements * sizeof(T) ) : (T*) realloc( data, elements * sizeof(T) );
	_cap = elements;
}

template <typename T>
inline void CppVector<T>::reserve_another( const uint32_t more_elements ){
	reserve( size() + more_elements );
}

template <typename T>
inline void CppVector<T>::push_back( const T& element ){
	if( size() + 1 >= capacity() ){
		// get twice of the current storage
		reserve( (1+size()) << 1 );
	}
	// assign the element and increase the size
	(*this)[size()] = element;
	_size ++;
}

template <typename T>
inline void CppVector<T>::push_back_another(const T* elements, const uint32_t number ){
	if( capacity() < size() + number ){
		// get exact number of more space
		reserve_another( number );
	}
	memcpy( &data[ size() ], elements, number * sizeof(T) );
}

template <typename T>
inline void CppVector<T>::push_back_another(const CppVector<T>& elements, const uint32_t number ){
	push_back_another( elements.data, number );
}

template <typename T>
inline void CppVector<T>::pop_back(){
	assert ( size() > 0 );
	_size = _size - 1;
}

template <typename T>
inline void CppVector<T>::erase(const uint32_t ind ){
	assert ( size() == 0 );
	// remove on remaining data
	const uint32_t max = size() - 1;
	for(uint32_t i = ind; i < max; i++ ){
		data[i] = data[ i + 1 ];
	}
	// update size information
	_size = (_size > 0 ) ? _size - 1 : 0;
}

template <typename T>
inline void CppVector<T>::erase_no_order(const uint32_t ind ){
	assert ( size() == 0 );
	
	(*this)[ind] = (*this)[size()-1];
	// update size information
	pop_back();
}

template <typename T>
inline uint32_t CppVector<T>::size() const{
	return _size;
}

template <typename T>
inline uint32_t CppVector<T>::capacity() const{
	return _cap;
}

template <typename T>
inline void CppVector<T>::clear(){
	_size = 0;
}

template <typename T>
inline void CppVector<T>::minimize(){
	const uint32_t elements = size();
	// do not remove storage
	if( elements == _cap ) return;

	// if no memory has been allocated yet, get some, otherwise, resize the storage
	data = (data == 0) ? (T*) MALLOC( elements * sizeof(T) ) : (T*) realloc( data, elements * sizeof(T) );
	_cap = elements;
}

#endif

#endif
