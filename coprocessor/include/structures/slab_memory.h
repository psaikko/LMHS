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
	slab_memory.h
	This file is part of riss.
	
	12.12.2009
	Copyright 2009 Norbert Manthey
*/

// idea introduced in: SunOS by Jeff Bonwick

#ifndef _SLAB_MEMORY_H_
#define _SLAB_MEMORY_H_

#include <cstdlib>
#include <cstring>
#include <inttypes.h>

#include "defines.h"
#include "macros/malloc_macros.h"

#define SLAB_PAGE_SIZE (4096 - 24) // be aware of malloc overhead for next page!

template <typename T>
class SlabAllocator{
	union element{
		char data[ sizeof(T) ];	// only used as storage, types with constructor are not allowed
		element* next;
	};
	
	element* _head;	// begin of list of pages
	element* _lastPage; // stores begin of first page
	
	element* _nextCell;	// memory cell which is returned next
	element* _lastCell;	// last free memory cell
	
	void* initPage();
	
public:
	SlabAllocator() : _head(0) {
		_lastPage = (element*)valloc( SLAB_PAGE_SIZE );
		memset( _lastPage, 0, SLAB_PAGE_SIZE );
	
		const uint32_t pageElements = ( SLAB_PAGE_SIZE/sizeof(element) );
		_nextCell = &( _lastPage[1] );
		_lastCell = &( _lastPage[pageElements - 1 ] );
		
		_head = _lastPage;
	}
	
	~SlabAllocator(){
		while( _head != _lastPage )
		{
			element* tmp = _head->next;
			free( _head );
			_head = tmp;
		}
		// dont forget very last page!
		free( _head );
	}
	
	void* get();
	void release( void* adress );

};

template <typename T>
inline void* SlabAllocator<T>::initPage()
{
	_lastPage->next = (element*)valloc( SLAB_PAGE_SIZE );
	_lastPage = _lastPage->next;
	memset( _lastPage, 0, SLAB_PAGE_SIZE );
	
	const uint32_t pageElements = ( SLAB_PAGE_SIZE/sizeof(element) );
	_nextCell = &( _lastPage[1] );
	_lastCell = &( _lastPage[pageElements - 1 ] );
	return _lastPage;
}


template <typename T>
inline void* SlabAllocator<T>::get()
{
	element* t = _nextCell;
	if( _nextCell != _lastCell ){
		_nextCell = (_nextCell->next==0) ? _nextCell+1 : _nextCell->next;
	} else {
		initPage();
	}
	return t;
}


template <typename T>
inline void SlabAllocator<T>::release( void* adress )
{
	_lastCell->next = (element*)adress;
	_lastCell = (element*)adress;
}


/*
	if size is given and no typ should be given ( to reimplement C malloc and free )
*/

// defined in define.h
// /* #define SLAB_MALLOC_MAXSIZE 128 */


class SlabSizeAllocator{

	template <int T>
	union element{
		char data[ T ];	// only used as storage, types with constructor are not allowed
		element* next;
	};
	
	void** _head[SLAB_MALLOC_MAXSIZE];	// begin of list of pages
	void** _lastPage[SLAB_MALLOC_MAXSIZE]; // stores begin of first page
	
	void** _nextCell[SLAB_MALLOC_MAXSIZE];	// memory cell which is returned next
	void** _lastCell[SLAB_MALLOC_MAXSIZE];	// last free memory cell
	
	void* initPage(uint32_t size);
	
public:
	SlabSizeAllocator(){

		for( uint32_t i = sizeof( void* ) ; i < SLAB_MALLOC_MAXSIZE; ++i ){
			// std::cerr << "init page for" << i << " bytes" <<std::endl;
			_head[i] = 0;

	
		_lastPage[i] = (void**)valloc( SLAB_PAGE_SIZE );
		memset( _lastPage[i], 0, SLAB_PAGE_SIZE );
	
		const uint32_t pageElements = ( (SLAB_PAGE_SIZE-8)/i); // think of the one pointer that needs to store the next page!
		_nextCell[i] = &( _lastPage[i][1] );
		_lastCell[i] = reinterpret_cast<void **>( reinterpret_cast<unsigned long>( &(_lastPage[i][1]) ) + (pageElements - 1) * i );
		// std::cerr << "set lastCell[" << i << "] to "  << std::hex << _lastCell[i] << std::dec <<  ", page(" << pageElements << " eles) base: " << std::hex << _lastPage[i] << std::dec << std::endl;		
		_head[i] = _lastPage[i];
		}
	}
	
	~SlabSizeAllocator(){
		for( uint32_t i = sizeof( void* ) ; i < SLAB_MALLOC_MAXSIZE; ++i ){
			while( _head[i] != _lastPage[i] )
			{
				void** tmp = (void**)(*(_head[i]));
				free( (void*)_head[i] );
				_head[i] = tmp;
			}
			// dont forget very last page!
			free( _head[i] );
		}
	}
	
	void* get(uint32_t size);
	void release( void* adress, uint32_t size );
	void* resize( void* adress, uint32_t new_size, uint32_t size );

};

inline void* SlabSizeAllocator::initPage(uint32_t size)
{
	// std::cerr << "init page for" << size << " bytes" <<std::endl;
	*(_lastPage[size]) = (void**)valloc( SLAB_PAGE_SIZE );
	_lastPage[size] = (void**) *(_lastPage[size]);
	memset( _lastPage[size], 0, SLAB_PAGE_SIZE );
	
	const uint32_t pageElements = ( (SLAB_PAGE_SIZE-8)/size); // think of the one pointer that needs to store the next page!
	_nextCell[size] = &( _lastPage[size][1] );
	_lastCell[size] = reinterpret_cast<void **>( reinterpret_cast<unsigned long>( &(_lastPage[size][1]) ) + (pageElements - 1) * size );
		// std::cerr << "set lastCell[" << size << "] to "  << std::hex << _lastCell[size] << std::dec <<  ", page(" << pageElements << " eles) base: " << std::hex << _lastPage[size] << std::dec << std::endl;		
	/*_lastCell[size] = &( _lastPage[size][pageElements - 1 ] );*/
	return _lastPage[size];
}


inline void* SlabSizeAllocator::get(uint32_t size)
{
	// check size and set real one!
	if( size >= SLAB_MALLOC_MAXSIZE ) return MALLOC(size);
	if( size < sizeof( void* ) ) size = sizeof( void* );

	void** t = _nextCell[size];
	if( _nextCell[size] != _lastCell[size] ){
		_nextCell[size] = ( (*(_nextCell[size]))==0 ) 
			? /* TODO: fix this expression according to size!*/ /*_nextCell[size] + sizeof( void* )*/

					reinterpret_cast<void **>( reinterpret_cast<unsigned long>(_nextCell[size]) + size)
			
				: (void**)(*(_nextCell[size]));
	} else {
		initPage(size);
	}
	// std::cerr << "get" << size << " bytes at " << std::hex << t << " set new_ele to " << _nextCell[size] << std::dec << std::endl;
	// std::cerr << " with content: " << *((unsigned int*)(_nextCell[size])) << std::endl;
	return t;
}


inline void SlabSizeAllocator::release( void* adress, uint32_t size )
{
	// std::cerr << "release" << size << " bytes at " << std::hex << adress << std::dec << std::endl;
	// check size and set real one!
	if( size >= SLAB_MALLOC_MAXSIZE ) return free( adress );
	if( size < sizeof( void* ) ) size = sizeof( void* );

	// set content of last Cell to jump to next cell
	*(_lastCell[size]) 
						= adress;
	// jump to new last cell
	_lastCell[size] 
						= (void**)adress;
}

inline void* SlabSizeAllocator::resize( void* adress, uint32_t new_size, uint32_t size )
{
	// std::cerr << "resize " << std::hex << adress << " from " << std::dec << size << " to " << new_size << std::endl;
	// get new memory
	void* mem = get(new_size);
	uint32_t smaller = ( new_size < size ) ? new_size : size;
	// copy memory!
	memcpy( mem, adress, smaller );
	// free old memory
	release( adress, size );
	// return memory
	return mem;
}
#endif
