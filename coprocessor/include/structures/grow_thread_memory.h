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
	grow_thread_memory.h
	This file is part of riss.
	
	13.09.2010
	Copyright 2010 Norbert Manthey
*/

#ifndef _GROW_THREAD_MEMORY_H_
#define _GROW_THREAD_MEMORY_H_

#include <cstdlib>
#include <cstring>
#include <inttypes.h>

#include <pthread.h>

#include "defines.h"

#include "macros/malloc_macros.h"

/** Allocator that stores slabs and enlarges itself (via copying)
 */
template <typename T>
class GrowThreadMemory{
	union element{
		char data[ sizeof(T) ];	// only used as storage, types with constructor are not allowed
		uint32_t next;
	};
	
/*
 * pthread_mutex_init(&addElements, 0);
 * pthread_mutex_lock(&addElements);
 * pthread_mutex_unlock(&addElements);
 * pthread_mutex_destroy(&addElements);
 */

	// TODO: think about good values!
	const static uint32_t BELE = 131072; // assumption: sizeof(T)=32, 4M block (131072
	const static uint32_t ldBELE = 17; // (17
	const static uint32_t localBins = 1024; // basic storage to never run out...

	uint32_t eles;	// number of max elements with current storage
	uint32_t next_free; // number of next free cell
	uint32_t given_eles;	// number of eles that are used

	uint32_t usedStorages;	// number of already created storages
	element* storages[localBins];	// pointer to memory blocks /first 31 and pointer to external memory
	std::vector<element*> ext;

	pthread_mutex_t addElements;

	/// enlarge underlying space
	void enlarge(){
		// no need for locking, because called in critical sections only!
		// pthread_mutex_lock(&addElements);
		if( given_eles >= eles ){
			// check enlarge condition again (other thread could have just enlarged the container)
			if( usedStorages < localBins ){
				storages[usedStorages] = (element*) malloc( sizeof(element) * BELE );
				memset( storages[usedStorages] ,0, sizeof(element) * BELE );
				++usedStorages;
				eles += BELE;
			} else {
				ext.push_back( (element*) MALLOC( sizeof(element) * BELE ) );
				memset( ext[ext.size() - 1] ,0, sizeof(element) * BELE );
				eles += BELE;
			}
		}
		// no need for locking, because called in critical sections only!
		// pthread_mutex_unlock(&addElements);
	}
	
public:

	GrowThreadMemory() :
	eles(BELE), next_free(1), given_eles(1),usedStorages(1)	// never lext next_free get to value 0! very first element is reserved!
	{
		pthread_mutex_init(&addElements, 0);
		storages[0] = (element*) malloc( sizeof(element) * BELE );
		memset( storages[0] ,0, sizeof(element) * BELE );
	}
	
	GrowThreadMemory(uint32_t elements) :
	eles( ((elements - 1)/BELE +1)*BELE), next_free(1), given_eles(1),usedStorages(0)	// never let next_free get to value 0! very first element is reserved!
	{
		pthread_mutex_init(&addElements, 0);
		usedStorages = eles / BELE;	// integer result, div without rest, because of initialization
		for( uint32_t i = 0 ; i < usedStorages; ++i ){
			storages[i] = (element*) malloc( sizeof(element) * BELE );
			memset( storages[i] ,0, sizeof(element) * BELE );
		}
	}
	
	~GrowThreadMemory(){
		for( uint32_t i = 0 ; i < usedStorages; ++i ){
			free( storages[i] );
		}
		for( uint32_t i = 0 ; i < ext.size(); ++i ){
			free( ext[i] );
		}
		pthread_mutex_destroy(&addElements);
	}
	
	void printSizes(){
		//std::cerr << "sizeof(T): " << sizeof(T) << "sizeof(element): " << sizeof(element) << std::endl;
	}
	
	void printInfo(){
		//std::cerr << "cap: " << eles << " used: " << given_eles << " next_free: " << next_free << " acc.nextField: " << get_ele(next_free).next << std::endl;
	}
	
	void printFreeChain(){
		/*std::cerr << "free chain:";
		uint32_t tmp = next_free;
		std::cerr << " " << next_free;
		for( uint32_t i = 0 ; i < eles - given_eles - 1; ++i ){
			tmp = (get_ele(tmp).next==0) ? tmp + 1 : get_ele(tmp).next;
			std::cerr << " " << tmp;
		}
		std::cerr << std::endl;*/
	}
	
	/// store next element in storage, update next_free ptr, enlarge if neccessary
	uint32_t create( const T& t ){
		pthread_mutex_lock(&addElements);
		// backup
		uint32_t tmp = next_free;

		// extract next field
		next_free = ( get_ele(next_free).next == (uint32_t)0 ) ? next_free + 1 : get_ele(next_free).next;
		T* ele = (T*)( addr(tmp) );

		*ele = t;
		if( ++given_eles >= eles ) enlarge();
		pthread_mutex_unlock(&addElements);
		return tmp;
	}
	
	/// store next element in storage, update next_free ptr, enlarge if neccessary
	uint32_t createCleared( const T& t ){
		pthread_mutex_lock(&addElements);
		// backup
		uint32_t tmp = next_free;

		// extract next field
		next_free = ( (T*)( get_ele(next_free) ).next == (uint32_t)0 ) ? next_free + 1 : (T*)( get_ele(next_free) ).next;
		T* ele = (T*)( addr(tmp) );
		memset( ele, 0, sizeof(element) );

		*ele = t;
		if( ++given_eles >= eles ) enlarge();
		pthread_mutex_unlock(&addElements);
		return tmp;
	}
	
	T& get(uint32_t ref){
		T* tmp = (T*)addr(ref);
		return (*tmp);
	}
	
	void* addr(uint32_t ref){
		const uint32_t b = ref >> ldBELE;
		const uint32_t o = ref & (BELE -1);
		if( b < localBins ) return &(storages[b][o]);
		return &(ext[b - localBins][o]);
	}
	
	element& get_ele(uint32_t ref){
		const uint32_t b = ref >> ldBELE;
		const uint32_t o = ref & (BELE -1);
//		std::cerr << "get_ele " << ref << " b=" << b << " o=" << o << " used=" << usedStorages << std::endl;
		if( b < localBins ) return storages[b][o];
		return ext[b - localBins][o];
	}
	
	void release( uint32_t i ){
		pthread_mutex_lock(&addElements);
		get_ele(i).next = next_free;
		next_free = i;
		given_eles--;
		pthread_mutex_unlock(&addElements);
	}

};

#endif
