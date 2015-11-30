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
	heap.h
	This file is part of riss.
	
	16.10.2009
	Copyright 2009 Norbert Manthey
*/


#include "defines.h"

#ifdef BINARY_HEAP

#ifndef _WEIGHTED_HEAP_H
#define _WEIGHTED_HEAP_H

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <vector>

using namespace std;

template<class Key, class Compare> 
class Heap
{
	private:
		Compare				comp;
	
		std::vector<uint32_t>	sorting;
		std::vector<uint32_t>	indices;
		Key					**values;
		uint32_t			max_value;

		void update(uint32_t item, bool ins, bool rem);
		void swap_items(uint32_t item1, uint32_t item2);

	public:
		Heap(Key **values_ref, uint32_t size);
		Heap() : values(0), max_value(0) {}

		bool is_empty();
		void clear();
		
		/// free resources!
		void free();

		void insert_item(uint32_t item);
		void update_item(uint32_t item);
		void remove_item(uint32_t item);

		uint32_t size() const ;
		uint32_t item_at(uint32_t ind) const ;
		bool contains(uint32_t item);
		
		/** sets a new size for the heap, also sets the new value array
		*/
		void resize(uint32_t newElements,Key** newKeys);
		
		/** reserve space for more elements
		*/
		void reserve(uint32_t reserveElements);

		std::vector<uint32_t>::const_iterator begin();
		std::vector<uint32_t>::const_iterator end();
};




template<class Key, class Compare> 
inline uint32_t Heap<Key, Compare>::size() const
{
	return sorting.size();
}

template<class Key, class Compare> 
inline void Heap<Key, Compare>::resize(uint32_t  newElements,Key** newKeys){
	values = newKeys;
	/*
	vector< uint32_t > elements = sorting;
	vector< uint32_t > oldIndex = indices;
	*/
	
	sorting.reserve( newElements );
	const uint32_t oldSize = indices.size();

//	indices.resize( newElements, UINT_MAX );
	max_value = newElements;
//	cerr << "c resize heap from " << oldSize << " to " << newElements << " elements " << endl;

	if( newElements > oldSize ){
		indices.insert( indices.end(), newElements - oldSize + 1, UINT_MAX );
	}

/*
	for( uint32_t i = 0 ; i < elements.size(); ++ i ) {
	  if( elements[i] != sorting[i] ) assert( false && "elements in heap changed" );
	  if( indices[elements[i]] != oldIndex[ elements[i] ] ) assert( false && "indixes have changed" );
	}
	*/
}

template<class Key, class Compare> 
inline void Heap<Key, Compare>::reserve(uint32_t  reserveElements){
	sorting.reserve( reserveElements );
	indices.reserve( reserveElements );
}

template<class Key, class Compare> 
inline bool Heap<Key, Compare>::is_empty()
{
	return sorting.empty();
}

template<class Key, class Compare> 
inline uint32_t Heap<Key, Compare>::item_at(uint32_t ind) const 
{
	// assert(ind >= 0); // uint32_t
	assert(ind < sorting.size());
// 	assert(indices[ind] < sorting.size() );
	return sorting[ind];
}

template<class Key, class Compare> 
inline bool Heap<Key, Compare>::contains(uint32_t item)
{
	return (indices[item] < sorting.size());
}

#define CPPPARENT(___ind)	(((___ind) - 1) / 2)
#define CPPCHILD_LEFT(___ind)		(2 * (___ind) + 1)
#define CPPCHILD_RIGHT(___ind)		(2 * (___ind) + 2)

#define CPPCOMPARE(___ind1, ___ind2) \
							(comp((*values)[sorting[___ind1]], (*values)[sorting[___ind2]]))

template<class Key, class Compare> 
Heap<Key, Compare>::Heap(Key **values_ref, uint32_t size)
{
	values = values_ref;
	max_value = size;

	sorting.reserve(max_value);
	indices.assign(max_value, UINT_MAX);
//	indices.resize(max_value, UINT_MAX);
}

template<class Key, class Compare> 
void Heap<Key, Compare>::update(uint32_t item, bool ins, bool rem)
{
//	std::cerr << "update heap : item " << item << ", ins " << ins << ", rem " << rem << std::endl;
	uint32_t ind, size, max, old_item;

	// Insert
	if (ins && !rem) {
		//std::cerr << "insert " << item << " with new ind " << sorting.size();
		indices[item] = sorting.size();
		sorting.push_back(item);
	}
	// Remove
	if (rem && !ins) {
		old_item = item;
		ind = indices[item];
		
		item = sorting[sorting.size() - 1];
		swap_items(ind, sorting.size() - 1);
		
		sorting.pop_back();
		indices[old_item] = UINT_MAX;

		// nothing to do, if very last element is removed
		if( item == old_item ) return;
	}

	// Init
	ind = indices[item];
	//if( rem ) std::cerr << "removeind: " << ind << " new item: " << item << std::endl;
	size = sorting.size();
	
	// Nothing left to do
	if (size == 0)
		return;

	assert(CPPCHILD_LEFT(CPPPARENT(ind)) == ind || CPPCHILD_RIGHT(CPPPARENT(ind)) == ind);
	// Move up
	while (ind > 0 && 
		comp((*values)[sorting[ ind ]], (*values)[sorting[ CPPPARENT(ind) ]]) )
	// CPPCOMPARE(ind, CPPPARENT(ind))) 
	
	{
		swap_items(ind, CPPPARENT(ind));
		ind = CPPPARENT(ind);
	}

	assert (ind==0 || !CPPCOMPARE(ind, CPPPARENT(ind)));

	// Move down
	while (1) {
		if (CPPCHILD_LEFT(ind) < size)
			max = CPPCHILD_LEFT(ind);
		else
			break;

		if (CPPCHILD_RIGHT(ind) < size && CPPCOMPARE(CPPCHILD_RIGHT(ind), CPPCHILD_LEFT(ind)))
			max = CPPCHILD_RIGHT(ind);

		if (!CPPCOMPARE(max, ind))
			break;

		swap_items(ind, max);
		ind = max;
	}
	
	assert (ind==0 || !CPPCOMPARE(ind, CPPPARENT(ind)));
	assert (CPPCHILD_LEFT(ind) >= size || !CPPCOMPARE(CPPCHILD_LEFT(ind), ind));
	assert (CPPCHILD_RIGHT(ind) >= size || !CPPCOMPARE(CPPCHILD_RIGHT(ind), ind));
}

template<class Key, class Compare> 
inline void Heap<Key, Compare>::swap_items(uint32_t ind1, uint32_t ind2)
{
	uint32_t tmp;

	// Swap values
	tmp = sorting[ind1];
	sorting[ind1] = sorting[ind2];
	sorting[ind2] = tmp;

	// Swap indices
	tmp = indices[sorting[ind1]];
	indices[sorting[ind1]] = indices[sorting[ind2]];
	indices[sorting[ind2]] = tmp;
}

template<class Key, class Compare> 
void Heap<Key, Compare>::clear()
{
	sorting.clear();
	indices.assign(max_value, UINT_MAX);
}

template<class Key, class Compare> 
void Heap<Key, Compare>::free()
{
	sorting.clear();
	sorting.swap(sorting);
	indices.clear();
	indices.swap(indices);
}

template<class Key, class Compare> 
void Heap<Key, Compare>::insert_item(uint32_t item)
{
	assert( !contains(item) && "an inserted item cannot be already in the heap" );
	update(item, true, false);
}

template<class Key, class Compare> 
void Heap<Key, Compare>::update_item(uint32_t item)
{
	assert( contains(item) && "heap needs to contain an item to update it" );
	update(item, true, true);
}

template<class Key, class Compare> 
void Heap<Key, Compare>::remove_item(uint32_t item)
{
	update(item, false, true);
}

template<class Key, class Compare> 
std::vector<uint32_t>::const_iterator Heap<Key, Compare>::begin()
{
	return sorting.begin();
}

template<class Key, class Compare> 
std::vector<uint32_t>::const_iterator Heap<Key, Compare>::end()
{
	return sorting.end();
}

#endif

#else // BINARY_HEAP
/*
	heap.h
	This file is part of riss.
	
	16.10.2009
	Copyright 2009 Norbert Manthey
*/

#ifndef _WEIGHTED_HEAP_H
#define _WEIGHTED_HEAP_H

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <vector>

template<class Key, class Compare> 
class Heap
{
	private:
		Compare				comp;
	
		std::vector<unsigned>	sorting;
		std::vector<unsigned>	indices;
		Key					**values;
		unsigned			max_value;

		void update(unsigned item, bool ins, bool rem);
		void swap_items(unsigned item1, unsigned item2);

	public:
		Heap(Key **values_ref, unsigned size);
		Heap() : values(0), max_value(0) {}

		bool is_empty();
		void clear();

		void free();
		
		void insert_item(unsigned item);
		void update_item(unsigned item);
		void remove_item(unsigned item);

		unsigned size() const ;
		unsigned item_at(unsigned ind) const ;
		bool contains(unsigned item);
		
		/** sets a new size for the heap, also sets the new value array
		*/
		void resize(unsigned int newElements,Key** newKeys);
		
		/** reserve space for more elements
		*/
		void reserve(unsigned int reserveElements);

		std::vector<unsigned>::const_iterator begin();
		std::vector<unsigned>::const_iterator end();
};









template<class Key, class Compare> 
inline unsigned Heap<Key, Compare>::size() const
{
	return sorting.size();
}

template<class Key, class Compare> 
inline void Heap<Key, Compare>::resize(unsigned int newElements,Key** newKeys){
	values = newKeys;
	sorting.reserve( newElements );
	const uint32_t oldSize = indices.size();

	indices.reserve( newElements );
	max_value = newElements;

	if( newElements > oldSize ){
		indices.insert( indices.end(), newElements - oldSize + 1, UINT_MAX );
	}
}

template<class Key, class Compare> 
inline void Heap<Key, Compare>::reserve(unsigned int reserveElements){
	sorting.reserve( reserveElements );
	indices.reserve( reserveElements );
}

template<class Key, class Compare> 
inline bool Heap<Key, Compare>::is_empty()
{
	return sorting.empty();
}

template<class Key, class Compare> 
inline unsigned Heap<Key, Compare>::item_at(unsigned ind) const 
{
	// assert(ind >= 0); // unsigned
	assert(ind < sorting.size());

	return sorting[ind];
}

template<class Key, class Compare> 
inline bool Heap<Key, Compare>::contains(unsigned item)
{
	return (indices[item] < sorting.size());
}

#define CPPPARENT(___ind)	(((___ind) - 1) / 4)
#define CPPCHILD_ONE(___ind)		(4 * (___ind) + 1)
#define CPPCHILD_TWO(___ind)		(4 * (___ind) + 2)
#define CPPCHILD_THREE(___ind)		(4 * (___ind) + 3)
#define CPPCHILD_FOUR(___ind)		(4 * (___ind) + 4)

#define CPPCOMPARE(___ind1, ___ind2) \
							(comp((*values)[sorting[___ind1]], (*values)[sorting[___ind2]]))

template<class Key, class Compare> 
Heap<Key, Compare>::Heap(Key **values_ref, unsigned size)
{
	values = values_ref;
	max_value = size;

	sorting.reserve(max_value);
	indices.assign(max_value, UINT_MAX);
//	indices.resize(max_value, UINT_MAX);
}

template<class Key, class Compare> 
void Heap<Key, Compare>::update(unsigned item, bool ins, bool rem)
{
//	std::cerr << "update heap : item " << item << ", ins " << ins << ", rem " << rem << std::endl;
	unsigned ind, size, max, old_item;

	// Insert
	if (ins && !rem) {
		//std::cerr << "insert " << item << " with new ind " << sorting.size();
		indices[item] = sorting.size();
		sorting.push_back(item);
	}
	// Remove
	if (rem && !ins) {
		old_item = item;
		ind = indices[item];
		
		item = sorting[sorting.size() - 1];
		swap_items(ind, sorting.size() - 1);
		
		sorting.pop_back();
		indices[old_item] = UINT_MAX;

		// nothing to do, if very last element is removed
		if( item == old_item ) return;
	}

	// Init
	ind = indices[item];
	//if( rem ) std::cerr << "removeind: " << ind << " new item: " << item << std::endl;
	size = sorting.size();
	
	// Nothing left to do
	if (size == 0)
		return;

	assert(CPPCHILD_ONE(CPPPARENT(ind)) == ind 
		|| CPPCHILD_TWO(CPPPARENT(ind)) == ind 
		|| CPPCHILD_THREE(CPPPARENT(ind)) == ind 
		|| CPPCHILD_FOUR(CPPPARENT(ind)) == ind);

	// Move up
	while (ind > 0 && 
		comp((*values)[sorting[ ind ]], (*values)[sorting[ CPPPARENT(ind) ]]) )
	// CPPCOMPARE(ind, CPPPARENT(ind))) 
	
	{
		swap_items(ind, CPPPARENT(ind));
		ind = CPPPARENT(ind);
	}

	assert (ind==0 || !CPPCOMPARE(ind, CPPPARENT(ind)));

	// Move down
	while (1) {
		if (CPPCHILD_ONE(ind) < size)
			max = CPPCHILD_ONE(ind);
		else
			break;

		if (CPPCHILD_TWO(ind) < size 
			&& CPPCOMPARE(CPPCHILD_TWO(ind), CPPCHILD_ONE(ind)))
			max = CPPCHILD_TWO(ind);

		if (CPPCHILD_THREE(ind) < size 
			&& CPPCOMPARE(CPPCHILD_THREE(ind), CPPCHILD_ONE(ind))
			&& CPPCOMPARE(CPPCHILD_THREE(ind), CPPCHILD_TWO(ind)))
			max = CPPCHILD_THREE(ind);

		if (CPPCHILD_FOUR(ind) < size 
			&& CPPCOMPARE(CPPCHILD_FOUR(ind), CPPCHILD_ONE(ind))
			&& CPPCOMPARE(CPPCHILD_FOUR(ind), CPPCHILD_TWO(ind))
			&& CPPCOMPARE(CPPCHILD_FOUR(ind), CPPCHILD_THREE(ind)))
			max = CPPCHILD_FOUR(ind);

		if (!CPPCOMPARE(max, ind))
			break;

		swap_items(ind, max);
		ind = max;
	}
	
	assert (ind==0 || !CPPCOMPARE(ind, CPPPARENT(ind)));
	assert (CPPCHILD_ONE(ind) >= size || !CPPCOMPARE(CPPCHILD_ONE(ind), ind));
	assert (CPPCHILD_TWO(ind) >= size || !CPPCOMPARE(CPPCHILD_TWO(ind), ind));
	assert (CPPCHILD_THREE(ind) >= size || !CPPCOMPARE(CPPCHILD_THREE(ind), ind));
	assert (CPPCHILD_FOUR(ind) >= size || !CPPCOMPARE(CPPCHILD_FOUR(ind), ind));
}

template<class Key, class Compare> 
inline void Heap<Key, Compare>::swap_items(unsigned ind1, unsigned ind2)
{
	unsigned tmp;

	// Swap values
	tmp = sorting[ind1];
	sorting[ind1] = sorting[ind2];
	sorting[ind2] = tmp;

	// Swap indices
	tmp = indices[sorting[ind1]];
	indices[sorting[ind1]] = indices[sorting[ind2]];
	indices[sorting[ind2]] = tmp;
}

template<class Key, class Compare> 
void Heap<Key, Compare>::clear()
{
	sorting.clear();
	indices.assign(max_value, UINT_MAX);
}

template<class Key, class Compare> 
void Heap<Key, Compare>::free()
{
	sorting.clear();
	sorting.swap(sorting);
	indices.clear();
	indices.swap(indices);
}

template<class Key, class Compare> 
void Heap<Key, Compare>::insert_item(unsigned item)
{
	update(item, true, false);
}

template<class Key, class Compare> 
void Heap<Key, Compare>::update_item(unsigned item)
{
	update(item, true, true);
}

template<class Key, class Compare> 
void Heap<Key, Compare>::remove_item(unsigned item)
{
	update(item, false, true);
}

template<class Key, class Compare> 
std::vector<unsigned>::const_iterator Heap<Key, Compare>::begin()
{
	return sorting.begin();
}

template<class Key, class Compare> 
std::vector<unsigned>::const_iterator Heap<Key, Compare>::end()
{
	return sorting.end();
}

#endif



#endif // BINARY_HEAP

// #endif

