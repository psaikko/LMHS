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
	region_memory.h
	This file is part of riss.
	
	21.02.2012
	Copyright 2012 Norbert Manthey
*/

#ifndef REGION_MEMORY_H
#define REGION_MEMORY_H


#include <errno.h>
#include <cstdlib>

#include <stdint.h>
#include <inttypes.h>
#include <limits>
#include <assert.h>
#include <cstring>
#include <inttypes.h>

#include "defines.h"

#include "macros/malloc_macros.h"
#include "structures/clause.h"

template<class T>
class RegionAllocator
{
    T*     memory;
    uint32_t  size_;
    uint32_t  capacity_;
    uint32_t  wasted_;

    void reserve(uint32_t capacity);

 public:

    RegionAllocator(uint32_t start_cap = 1024*1024) : memory(0), size_(1), capacity_(0), wasted_(1) { reserve(start_cap); }

    ~RegionAllocator()
    {
        if (memory != 0) ::free(memory);
    }

    uint32_t size      () const      { return size_; }
    uint32_t wasted    () const      { return wasted_; }
    uint32_t capacity  () const      { return capacity_; }

    uint32_t alloc     (int s); 
    void     free      (int s)    { wasted_ += s; }

    
	
	/// store next element in storage, update next_free ptr, enlarge if neccessary
	uint32_t create( const Clause& clause ){
		const uint32_t ref = alloc( sizeof(Clause) / 4 + clause.size() );
		Clause* ele = (Clause*)addr(ref);
		ele->copyFrom(clause);
		return ref;
	}
	/// create clause in storage
	uint32_t create( bool learnt=true ){
		const uint32_t ref = alloc( sizeof(Clause) / 4 );
		Clause* ele = (Clause*)addr(ref); 
		ele->init();ele->setLearnt(learnt);
		ele->setStorage(0);
		return ref;
	}
	/// create clause in storage
	uint32_t create(const std::vector<Lit>& lits, bool learnt=true){
		const uint32_t ref = alloc( sizeof(Clause) / 4 + lits.size() );
		Clause* ele = (Clause*)addr(ref);
		ele->init();ele->setLearnt(learnt);
		ele->reset(lits);
		return ref;
	}
	/// create clause in storage
	uint32_t create(const Lit *lits, uint32_t count, bool learnt=true){
		const uint32_t ref = alloc( sizeof(Clause) / 4 + count );
		Clause* ele = (Clause*)addr(ref);
		ele->init();ele->setLearnt(learnt);
		ele->reset(lits,count);
		return ref;
	}
	/// create a copy of a given clause
	uint32_t copy(const uint32_t ref){
		const Clause& tmpClause = get(ref);
		const uint32_t ref2 = alloc( sizeof(Clause) / 4 + tmpClause.size() );
		Clause* ele = (Clause*)addr(ref2); 
		ele->init();ele->setLearnt(tmpClause.isLearnt());
		ele->reset(tmpClause);
		return ref2;
	}

    T& get(uint32_t ref){
	    return memory[ref];
    }

    void* addr(uint32_t ref){
	    return &(memory[ref]);
    }

    void release( uint32_t i, uint32_t s ){ wasted_ += s; }
        
    void  moveTo(RegionAllocator& to) {
        if (to.memory != 0) {
            ::free(to.memory);
        }
        to.memory = memory;
        to.size_ = size_;
        to.capacity_ = capacity_;
        to.wasted_ = wasted_;
        size_ = capacity_ = wasted_ = 0;
	memory = 0;
    }

    float getGarbageFraction()
    {
      return (double)wasted() / (double)size();
    }

};

template<class T>
void RegionAllocator<T>::reserve(uint32_t min_cap)
{
    if (capacity_ >= min_cap) return;
    uint32_t prev_cap = capacity_;
    while (capacity_ < min_cap){
        uint32_t delta = ((capacity_ >> 1) + (capacity_ >> 3) + 2) & ~1;
        capacity_ += delta;
        if (capacity_ <= prev_cap)
	{
            assert( false && "controlled memory error" );
	      exit(-1);
	}
    }
    assert(capacity_ > 0);
    memory = (T*)realloc(memory, sizeof(T)*capacity_);
}

template<class T>
uint32_t RegionAllocator<T>::alloc(int s)
{ 
    assert(s > 0);
    reserve(size_ + s);
    uint32_t p = size_;
    size_ += s;
    if (size_ < p)
    {
      assert( false && "controlled memory error" );
      exit(-1);
    }
    return p;
}


#endif
