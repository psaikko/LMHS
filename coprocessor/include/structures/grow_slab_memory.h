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
	grow_slab_memory.h
	This file is part of riss.
	
	15.07.2010
	Copyright 2010 Norbert Manthey
*/

// idea introduced in: SunOS by Jeff Bonwick

#ifndef _GROW_SLAB_MEMORY_H_
#define _GROW_SLAB_MEMORY_H_

#include <cstdlib>
#include <cstring>
#include <inttypes.h>
#include "assert.h"
#include <vector>

#include "defines.h"

#include "macros/malloc_macros.h"
#include "structures/clause.h"
#include "structures/grow_thread_memory.h"

using namespace std;

/** Allocator that stores slabs and enlarges itself (via copying)
 */
class GrowSlabAllocator{
	union element{
		char data[ sizeof(Clause) ];	// only used as storage, types with constructor are not allowed
		uint32_t next;
	};
	
	element* storage;	// pointer to memory
	uint32_t eles;	// number of max elements with current storage
	uint32_t next_free; // number of next free cell
	uint32_t given_eles;	// number of eles that are used

	/// enlarge underlying space
	void enlarge(){
		uint32_t oldSize = eles;
		eles = (eles * 5) / 4;
		storage = (element*)realloc(storage, sizeof(element) * eles);
		memset( &(storage[oldSize]),0, sizeof(element) * (eles - oldSize) );
	}
	
	// vector< uint32_t > freed;
	
public:

	GrowSlabAllocator() :
	storage(0), eles(100000), next_free(1), given_eles(1)	// never let next_free get to value 0! very first element is reserved!
	{
		// cerr << "c before create: " << std::hex << storage << std::dec << endl;
		storage = (element*)MALLOC( sizeof(element) * eles);
		// cerr << "c create storage at " << std::hex << storage << std::dec << endl;
		memset( storage ,0, sizeof(element) * eles );
		// freed.resize(eles, 0 );
	}
	
	GrowSlabAllocator(uint32_t elements) :
	storage(0), eles(elements), next_free(1), given_eles(1)	// never lext next_free get to value 0! very first element is reserved!
	{
		// cerr << "c before reserve create: " << std::hex << storage << std::dec << endl;
		storage = (element*)MALLOC( sizeof(element) * eles);
		// cerr << "c create reserve storage at " << std::hex << storage << std::dec << endl;
		memset( storage ,0, sizeof(element) * eles );
		/*
		freed.resize(eles, 0 );
		assert( next_free < eles );
		*/
	}
	
	~GrowSlabAllocator(){
		// cerr << "c free storage at " << std::hex << storage << std::dec << endl;
		if( storage != 0 ) { ::free( (void*)storage ); storage=0; }
		// assert(false);
	}
	
	void printSizes(){
		//std::cerr << "sizeof(Clause): " << sizeof(Clause) << "sizeof(element): " << sizeof(element) << std::endl;
	}
	
	void printInfo(){
		//std::cerr << "cap: " << eles << " used: " << given_eles << " next_free: " << next_free << " acc.nextField: " << storage[next_free].next << std::endl;
	}
	
	void printFreeChain(){
		/*
		std::cerr << "free chain:";
		uint32_t tmp = next_free;
		std::cerr << " " << next_free;
		for( uint32_t i = 0 ; i < eles - given_eles - 1; ++i ){
			tmp = (storage[tmp].next==0) ? tmp + 1 : storage[tmp].next;
			std::cerr << " " << tmp;
		}
		std::cerr << std::endl;
		*/
	}
	
	/// store next element in storage, update next_free ptr, enlarge if neccessary

	/* possible constructors for clause:
	Clause( bool learnt=true);	
	Clause(std::vector<Lit>& lits, bool learnt=true);
	Clause(const Lit *lits, uint32_t count, bool learnt=true)
	*/
	
	/// store next element in storage, update next_free ptr, enlarge if neccessary
	uint32_t create( const Clause& clause ){
		uint32_t tmp = next_free;
		next_free = (storage[next_free].next == (uint32_t)0 ) ? next_free + 1 : storage[next_free].next;
		Clause* ele = (Clause*)(&storage[tmp]); 
		ele->init();
		ele->setLearnt( clause.isLearnt() );
		ele->reset( clause );
		if( ++given_eles == eles ) enlarge();
		return tmp;
	}
	
	/// store next element in storage, update next_free ptr, enlarge if neccessary
	uint32_t create( bool learnt=true ){
		uint32_t tmp = next_free;
		next_free = (storage[next_free].next == (uint32_t)0 ) ? next_free + 1 : storage[next_free].next;
		Clause* ele = (Clause*)(&storage[tmp]); 
		ele->init();ele->setLearnt(learnt);
		ele->setStorage(0);
		if( ++given_eles == eles ) enlarge();
		return tmp;
	}
	
	/// store next element in storage, update next_free ptr, enlarge if neccessary
	uint32_t create(const std::vector<Lit>& lits, bool learnt=true){
		uint32_t tmp = next_free;
		next_free = (storage[next_free].next == (uint32_t)0 ) ? next_free + 1 : storage[next_free].next;
		Clause* ele = (Clause*)(&storage[tmp]); 
		ele->init();ele->setLearnt(learnt);
		ele->reset(lits);
		if( ++given_eles == eles ) enlarge();
		return tmp;
	}
	/// store next element in storage, update next_free ptr, enlarge if neccessary
	uint32_t create(const Lit *lits, uint32_t count, bool learnt=true){
		uint32_t tmp = next_free;
		next_free = (storage[next_free].next == (uint32_t)0 ) ? next_free + 1 : storage[next_free].next;
		Clause* ele = (Clause*)(&storage[tmp]); 
		ele->init();ele->setLearnt(learnt);
		ele->reset(lits,count);
		if( ++given_eles == eles ) enlarge();
		return tmp;
	}
	/// store next element in storage, update next_free ptr, enlarge if neccessary
	uint32_t copy(const uint32_t ref){
		const Clause& tmpClause = (Clause*)&(storage[ref].data);
		uint32_t tmp = next_free;
		next_free = (storage[next_free].next == (uint32_t)0 ) ? next_free + 1 : storage[next_free].next;
		Clause* ele = (Clause*)(&storage[tmp]); 
		ele->init();ele->setLearnt(tmpClause.isLearnt());
		ele->reset(tmpClause);
		if( ++given_eles == eles ) enlarge();
		return tmp;
	}
	
	/// store next element in storage, update next_free ptr, enlarge if neccessary
	uint32_t createCleared( const Clause& t ){
		uint32_t tmp = next_free;
		next_free = (storage[next_free].next == (uint32_t)0 ) ? next_free + 1 : storage[next_free].next;
		Clause* ele = (Clause*)(&storage[tmp]); // TODO FIXME reset the old element?
		memset( ele, 0, sizeof(element) );
		*ele = t;
		if( ++given_eles == eles ) enlarge();
		return tmp;
	}
	
	Clause& get(uint32_t ref){
		Clause* tmp = (Clause*)&(storage[ref].data);
		return (*tmp);
	}
	
	void* addr(uint32_t ref){
		// assert( next_free < eles );
		return &(storage[ref].data);
	}
	
	void release( uint32_t i, uint32_t ){
		storage[i].next = next_free;
		next_free = i;
		given_eles--;
	}

	/// dummies
	float getGarbageFraction() const { return 0; }
	
	void moveTo( GrowSlabAllocator& gsa ) {
	  gsa.storage=storage;
	  gsa.eles = eles;	// number of max elements with current storage
	  gsa.next_free = next_free; // number of next free cell
	  gsa.given_eles = given_eles;
	  if( gsa.storage != 0 ) ::free ( gsa.storage );
	  gsa.storage = storage;
	  eles = next_free = given_eles = 0;
	  storage = 0; 
	}
	
	uint32_t capacity() const { return 0; }
	uint32_t alloc( uint32_t ) const { return 0; }
	uint32_t size      () const      { return 0; }
	uint32_t wasted    () const      { return 0; }
	void     free      (int) const {}
};

#endif
