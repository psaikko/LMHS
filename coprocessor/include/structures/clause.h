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
	clause.h
	This file is part of riss.	19.08.2010
	Copyright 2010 Norbert Manthey
*/


#ifndef _RISS_Clause_H
#define _RISS_Clause_H

#include "defines.h"

#ifndef REGIONALLOCATOR

#include "structures/splitClause.h"

#else 

#include <assert.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <inttypes.h>

#include "types.h"
#include "structures/literal_system.h"
#include "macros/malloc_macros.h"
#include "utils/sort.h"

#include "utils/atomic.h"

using namespace std;

// for having an index
#define CL_REF uint32_t
#define NO_CLREF 0
// for some output
#define CL_OBJSIZE sizeof(Clause)


/** This class implements a clause and the main operation that are are done on the clauses.
*
*	This class can also represent XOR constraints
*	xor: true = l1 + l2 + ... + ln
*/
class Clause
{

private:
	uint32_t _sizedata;	/// field that stores the clauseSize of the clause and some flags
	union {	/// for preprocessing its the hash, for normal usage its the data.activity
		float activity;
		uint32_t hash;
	} data;

	/** set storeage of the clause
	*
	* @param s number of literals that the clause should be able to store
	*/
	void set_size(uint32_t s );
	
public:
	/// internally stored literals, be careful when using
	Lit	lits[0] ;	

  
	void reset(const Clause& other);
	
	/** create a clause with the specified literals
	 * * NOTE: use carefully
	*
	* @param lits literals for the clause
	*/
	void reset(const std::vector<Lit>& lits);
	/** create a clause with the specified literals
	 * NOTE: use carefully
	*
	* @param lits literals for the clause
	* @param clauseSize number of literals
	*/
	void reset(const Lit *lits, uint32_t count);
	/** initializes the clause attributes
	 * NOTE: use carefully!
	*/
	void init();
	/** set storeage of the clause
	* NOTE: use carefully
	* 
	* @param s number of literals that the clause should be able to store
	*/
	void	setStorage(uint32_t s);
	
	/** Create an empty clause
	*
	* @param learnt indicate whether the clause is a learnt clause
	*/
	Clause( bool learnt=true);	/** Create a clause from the specified literals
	*
	* @param lits literals for the clause
	* @param learnt indicate whether the clause is a learnt clause
	*/
	Clause(std::vector<Lit>& lits, bool learnt=true);	/** Create a clause from the specified literals
	*
	* @param lits literals for the clause
	* @param clauseSize number of literals
	* @param learnt indicate whether the clause is a learnt clause
	*/
	Clause(const Lit *lits, uint32_t count, bool learnt=true);
	
#if 0
	/** Copy operator - copies the other clause
	*
	* @param other clause to copy
	*/
	Clause& operator=(const Clause& other);
#endif

	/** Free used resources
	*/
	~Clause();

	/** for comparisons
	*
	*   Assumes, that the two clauses have been sorted using sort() before the comparison is done
	*/
	bool operator ==(Clause& other) const ;
	
	/** for comparisons
	*
	*   Assumes, that the two clauses have been sorted using sort() before the comparison is done
	*/
	bool operator >=(Clause& other) const ;
	
	/** for comparisons
	*
	*   Assumes, that the two clauses have been sorted using sort() before the comparison is done
	*/
	bool operator <=(Clause& other) const;
	
	/** for comparisons
	*
	*   Assumes, that the two clauses have been sorted using sort() before the comparison is done
	*/
	bool operator < (const Clause& other) const;

	/** compare two clauses
	*
	* @param other clause to compare to
	* @return true if the two clauses contain the same literals
	*
	*/
	bool equals(const Clause& other) const;
	
	/** get the number of literals
	*
	* @return number of literals
	*/
	uint32_t size() const ;
	
	/** get the number of words that are used by this clause
	 * 
	 * @return number of words that are used by this clause (one word = 4bytes)
	*/
	uint32_t getWords() const ;
	
	/** get the literal with a certain index
	*
	* @param pos index of the literal
	* @return literal with the specified index
	*/
	Lit get_literal(uint32_t pos) const;
	
	/** set the literal at a certain index
	*
	* @param pos index of the literal
	* @param lit1 literal to store on that position
	*/
	void set_literal(uint32_t pos, Lit lit1);
	
	/** copies the other clause into the current one
	*
	*/
	void copyFrom(const Clause& clause);
	
	/** add a certain literal to the clause
	*
	* @param lit1 literal to add to the clause
	*/
	// void add(Lit lit1);
	
	/** remove a certain literal from the clause
	* NOTE: keeps the order in the clause
	* @param lit1 literal to remove
	*/
	void remove(Lit lit1);

	/** remove a certain literal from the clause
	* NOTE: does not keep the order in the clause
	* @param lit1 literal to remove
	*/
	void removeLin(Lit lit1);
	
	/** remove a certain literal from the clause
	* NOTE: keeps the order in the clause
	* @param ind index of the literal that has to be removed
	*/
	void remove_index(uint32_t ind);
	
	/** remove a certain literal from the clause
	* NOTE: does not keep the order in the clause
	* @param ind index of the literal that has to be removed
	*/
	void remove_indexLin(uint32_t ind);
	
	/** reduce literals to the first newSize ones
	 */
	void shrink( uint32_t newSize );
	
	/** swap two literals of the clause
	*
	* @param pos1 index of the first literal
	* @param pos2 index of the second literal
	*/
	void swap_literals(uint32_t pos1, uint32_t pos2);
	
	/** check whether a certain literal is part of the clause
	*
	* @param literal literal to find
	* @return true, if literal is part of the clause
	*/
	bool contains_literal( const Lit literal) const;

	/** return the activity of the clause
	*
	* @return activity of the clause
	*/
	float get_activity() const;
	
	/** set the activity of the clause
	*
	* @param act new activity for the clause
	*/
	void set_activity( const float act);
	
	/** increase the activity of the clause
	*
	* @param act increase the activity by this value
	*/
	void inc_activity( const float act);
	
	/** divide current activity by specified value
	*/
	void div_activity( const float act);

	/** set 1/LBD as new activity
	*/
	//void setLBDactivity( const searchData& sd );
	
	// RESOLUTION
	/** resolve two clauses on the first matching variable
	*
	* @param other clause to be resolved with
	* @return resolvent or 0, if no mathcing variable exists
	*/
	Clause* resolve(const Clause& other) const;	/** resolve two clauses on a certain variable
	*
	* @param other clause to be resolved with
	* @param variable variable to use during resolution
	* @return resolvent or 0, if no mathcing variable exists
	*/
	Clause* resolve(const Clause& other, Var variable) const;

	/** check whether two clauses can be resolved
	*
	* @param other clause to be resolved with
	* @param variable variable to use during resolution
	* @return true, if resolvent is no tautology
	*/
	bool check_resolve( const Clause& other, Var variable) const;	/** check whether two clauses can be subsumed
	*
	* @param other clause to be subsumed
	* @param lit1 literal to resolve with (can be 0, thus subsumption can be applied)
	* @return true, if any simplification is possible
	*/
	bool subsumes_or_simplifies_clause(const Clause& other, Lit *lit1) const;

	/** resolve two clauses on a certain variable (linear)
	*
	* Assumption: both clauses are ordered
	* @param other clause to be resolved with
	* @param variable variable to use during resolution
	* @return resolvent or 0, if no mathcing variable exists
	*/
	Clause* resolveLin(const Clause& other, Var variable) const;
	
	/** check the resolution of two clauses on a certain variable (linear)
	*
	* Assumption: both clauses are ordered, each clause contains a variable only once
	* @param other clause to be resolved with
	* @param variable variable to use during resolution
	* @return true if possible and no tautology
	*/
	bool check_resolveLin( const Clause& other, Var variable) const;
	
	/** sorts the literals according to their number - quadratic!
	*/
	void sort();
	
	/** check whether the clause is sorted (according to sort function)
	 */
	bool isSorted() const ;

	// getter and setter for the remaining flags
	bool getFlag1() const ;
	void setFlag1(bool flag);

	bool getFlag2() const ;
	void setFlag2(bool flag);

	bool isXor() const ;
	void setXor(bool flag);

	/** this clause has to be removed by owner, not by removal heuristic */
	bool isKeepMe() const;
	/** set the state of this clause, whether it should not be removed by the removal heuristic */
	void setKeepMe(bool flag);

	bool isLearnt() const ;
	void setLearnt( bool l );

	void ignore(){
		setFlag1(true);
	}
	
	void unignore(){
		setFlag1(false);
	}

	void markToDelete(){
		// assert( false );
		setFlag2( true );
	}
	
	/** activate the delete-bit asynchronously
	 * */
	void markToDeleteSynced(){
	
	  // try to mark the delete bit, until it is finaly done
	  uint32_t old, newsizedata;
	  do {
	    old = _sizedata;
	    newsizedata = (old & ~0x40000000) | ((uint32_t)1 << 30); 
	  } while (! cmpxchg32(&_sizedata, old, newsizedata) );
	}

	bool isIgnored() const {
		return getFlag1() || getFlag2();
	}
	
	bool isToDelete() const {
		return getFlag2();
	}


	
	/** print all the data of the clause
	*/
	void printInfo() const ;
}  //CL_PACK
;


#include "sat/searchdata.h"

	// print all the data of the clause!
inline void Clause::printInfo() const {
	/*
	std::cerr << "cl[" << std::hex << this << std::dec << "]: clauseSize()(" << size() << "), act(" << data.activity
			<< ") flags: |" << getFlag1() << "|" << getFlag2() << "|" << getFlag3() << "|" << "|"
			
			<< ", literals:";
	for( uint32_t i = 0 ; i < size(); ++i ){
		std::cerr << " " << lits[i].nr();
	}
	std::cerr << std::endl;
	*/
}

inline Clause::Clause(bool learnt)
{
	init();
	setStorage(0);
	setLearnt(learnt);
}

inline Clause::Clause(std::vector<Lit>& lits, bool learnt)
{
	init();
	reset(lits);
	setLearnt(learnt);
}

inline Clause::Clause(const Lit *lits, uint32_t count, bool learnt)
{
	init();
	reset(lits, count);
	setLearnt(learnt);
}

#if 0
inline Clause& Clause::operator=(const Clause& other)
{
	// everything to 0
	init();
	// copy flags
	_sizedata = other._sizedata;
	// reset own clauseSize
	set_size(0);
	// go on as usual
	setStorage( other.size() );
	assert( size() == other.size());
	if( size()>_CL_ELEMENTS){
		memcpy (own_lits, other.own_lits, _CL_ELEMENTS*sizeof(Lit));
		memcpy (ext_lits, other.ext_lits, ( size()- _CL_ELEMENTS)*sizeof(Lit));
	} else {
		memcpy (own_lits, other.own_lits, _CL_ELEMENTS*sizeof(Lit));
	}
	data.activity = other.data.activity;
	TRACK_CHECK("copied to other clause");
	return *this;
}
#endif

inline bool Clause::operator < (const Clause& other) const {
	const uint32_t clauseSize = size();
	if( other.isIgnored() && !isIgnored() ) return true;
	if( !other.isIgnored() && isIgnored() ) return false;
	if( clauseSize > other.size() ) return false;
	if( clauseSize < other.size() ) return true;
	for( uint32_t i = 0 ; i < clauseSize; i++ ){
		if( get_literal(i) > other.get_literal(i) ) return false;
		if( get_literal(i) < other.get_literal(i) ) return true;
	}
	return false;
}

inline bool Clause::operator >=(Clause& other) const {
	return ! (*this < other);
}
	
inline bool Clause::operator <=(Clause& other) const {
	const uint32_t clauseSize = size();
	if( other.isIgnored() && !isIgnored() ) return true;
	if( !other.isIgnored() && isIgnored() ) return false;
	if( clauseSize > other.size() ) return false;
	if( clauseSize < other.size() ) return true;
	for( uint32_t i = 0 ; i < clauseSize; i++ ){
		if( get_literal(i) > other.get_literal(i) ) return false;
		if( get_literal(i) < other.get_literal(i) ) return true;
	}
	return true;
}

inline bool Clause::operator ==(Clause& other) const {
  const uint32_t clauseSize = size();
  if( other.isIgnored() != isIgnored() ) return false;
  if( clauseSize != other.size() ) return false;
  for( uint32_t i = 0 ; i < clauseSize; i++ ){
	  if( get_literal(i) != other.get_literal(i) ) return false;
  }
  return true;
}

inline Clause::~Clause()
{
	set_size( 0 );
	_sizedata = 0;
	set_activity( 0 );
}

inline void Clause::reset(const Clause& other)
{
  setStorage( other.size() );
  for (uint32_t i=0; i<other.size(); i++)
		set_literal(i, other.get_literal(i) );
}
  

inline void Clause::reset(const std::vector<Lit>& lits)
{
	setStorage( lits.size() );

	for (uint32_t i=0; i<lits.size(); i++)
		set_literal(i, lits[i]);
}

inline void Clause::reset(const Lit *lits, uint32_t count)
{
	setStorage(count);
	for (uint32_t i=0; i<count; i++)
		set_literal(i, lits[i]);
}

inline bool Clause::equals(const Clause& other) const {
	const uint32_t myS = size();
	const uint32_t oS = other.size();
	for( uint32_t i = 0 ; i < myS ; ++i ){
		uint32_t j = 0;
		for( ; j < oS; ++j ){
			if( get_literal(i) == other.get_literal(j) ) break;
		}
		if( j != oS ) return false;
	}
	return true;
}

inline uint32_t Clause::size() const
{
	return _sizedata & 0x7FFFFFF;	// mask bits!
}

inline uint32_t Clause::getWords() const 
{
  assert ( (sizeof(Clause) & 3) == 0 && "size of clause needs to be a multiple of 4" );
  return sizeof( Clause ) / 4 + size();
}

inline void Clause::set_size(uint32_t s ){
	_sizedata = (_sizedata & ~0x7FFFFFF) | (s & 0x7FFFFFF);	// mask bits! keep flags
}

inline Lit Clause::get_literal(uint32_t pos) const
{
  return lits[pos];
}

inline void Clause::set_literal(uint32_t pos, Lit lit1)
{
  lits[pos] = lit1;
}

inline void Clause::swap_literals(uint32_t pos1, uint32_t pos2)
{
	assert (pos1 < size());
	assert (pos2 < size());
	// could possibly be implemented faster
	Lit tmp = get_literal(pos1);
	set_literal(pos1, get_literal(pos2));
	set_literal(pos2, tmp);
}

inline void Clause::remove(Lit lit1)
{
	uint32_t i, j;

	const uint32_t s = size();
	for (i = 0, j = 0; i < s; i ++) {
		if (get_literal(i) != lit1)
			set_literal(j++, get_literal(i));
	}
	setStorage(j);

}

inline void Clause::removeLin(Lit lit1)
{
	const uint32_t s = size();
	for (uint32_t i = 0; i < s; i ++) {
		if (get_literal(i) == lit1)
		{
		  remove_indexLin(i);
		  break;
		}
	}

}

inline void Clause::remove_index(uint32_t ind)
{
	uint32_t i, j;
	const uint32_t s = size();
	for (i = ind+1, j = ind; i < s; i ++) {
		set_literal(j++, get_literal(i));
	}
	setStorage(j);
}

inline void Clause::remove_indexLin(uint32_t ind)
{
	const uint32_t s = size();
	swap_literals( ind, s - 1 );
	setStorage( s - 1 );
}

inline void Clause::shrink( uint32_t newSize ) {
	setStorage( newSize );
}

inline void Clause::copyFrom(const Clause& clause)
{
  memcpy( this, &clause, sizeof(Clause) + clause.size() * 4 );
}

inline void Clause::setStorage(uint32_t new_lits)
{
	assert( sizeof(Lit)==4 );
	if (size() != new_lits) {
	  set_size( new_lits );
	}
}

inline void Clause::init()
{
	_sizedata = 0;
	data.activity = 0;
	core = false;
}

inline float Clause::get_activity() const
{
	return data.activity;
}

inline void Clause::set_activity( const float activity)
{
	data.activity = activity;
}

inline void Clause::inc_activity( const float act)
{
	data.activity += act;
}

inline void Clause::div_activity( const float act){
	data.activity /= act;
}

#if 0
inline void Clause::setLBDactivity( const searchData& sd ){
	int levels[ size() ];
	for( uint32_t j = 0 ; j < size(); j++ ) levels[j] = sd.VAR_LEVEL(  get_literal(j ) .toVar() );
	// sort them and count borders			
	int count = 1;
	mergesort<int>( levels, size() );
	// count undefined variables
	int minusOneCount = 0;
	for( uint32_t j = 1 ; j < size(); j++ ){
		if( levels[j] == -1 ) minusOneCount++;
		else if( levels[j-1] != levels[j] ) count++;
	}
	// undefined literals are counted as extra half levels
	count += minusOneCount >> 1;
	count = (count == 0 ) ? 1 : count;
	// set new activity
	data.activity = 1.0f / (float) count;
}
#endif

inline Clause* Clause::resolve(const Clause& other) const
{
	const uint32_t s = size();
	for( int32_t i = s - 1; i >=0; i--)
	{
		const uint32_t os = other.size();
		for( int32_t j = os - 1; j >=0; j--)
		{
			if( get_literal( i ) ==  other.get_literal( j ) .complement() )
			return resolve( other,  get_literal( i ) .toVar() );
		}
	}
 	return 0;
}

inline Clause* Clause::resolveLin(const Clause& other, Var variable) const
{
	const uint32_t bytes = (size() + other.size()) * sizeof(Lit);

	Lit lits[ bytes ];

	const Clause& a = (*this);
	const Clause& b = other;

	assert( a.isSorted() );
	assert( b.isSorted() );
	
	uint32_t i=0,j=0, sz=0;
	const uint32_t sa = a.size();
	const uint32_t sb = b.size();
	
	do {
		const Lit& la = a.get_literal(i);
		if( la.toVar() == variable ) {i++; continue; }
		const Lit& lb = b.get_literal(j);
		if( lb.toVar() == variable ) {j++; continue; }
		if( la == lb.complement() ) return 0; // tautology
		Lit lc = la;
		if (lb < la) { lc = lb; j++; } else { i++; if( la == lb ) j++; } // update index of smallest literal
		lits[sz++] = lc;
	} while( i < sa && j < sb );
	// copy the remaining literals from the one clause, that has not finished yet
	if( i < sa ){
	  do {
	    const Lit& la = a.get_literal(i++);
	    if( la.toVar() != variable ) lits[sz++] = la;
	    else break;
	  } while( i < sa );
	  while( i < sa ) lits[sz++] = a.get_literal(i++);
	} else if ( j < sb ) {
	  do {
	    const Lit& lb = b.get_literal(j++);
	    if( lb.toVar() != variable )lits[sz++] = lb;
	    else break;
	  } while( j < sb );
	  while( j < sb ) lits[sz++] = b.get_literal(j++);
	}

	// Generate resolvent
	return new Clause(lits, sz, a.isLearnt() || b.isLearnt());
}

inline bool Clause::check_resolveLin( const Clause& other, Var variable) const
{
	const Clause& a = (*this);
	const Clause& b = other;
	
	assert( a.isSorted() );
	assert( b.isSorted() );

	uint32_t i=0,j=0;
	const uint32_t sa = a.size();
	const uint32_t sb = b.size();
	do {
		const Lit& la = a.get_literal(i);
		if( la.toVar() == variable ) {i++; continue; }
		const Lit& lb = b.get_literal(j);
		if( lb.toVar() == variable ) {j++; continue; }
		if( la == lb.complement() ) return false;	// tautology
		(lb < la) ? j++ : i++; // update index of smallest literal
	} while( i < sa && j < sb );
	/*
	if( i < sa && j < sb ){
		do {
	//		if( a.get_literal(i).toVar() == variable ) {i++; continue; }	// can not happen any more
			if( b.get_literal(j).toVar() == variable ) {j++; continue; }
			if( a.get_literal(i) == b.get_literal(j).complement() ) return false;	// tautology
			Lit c = (a.get_literal(i) < b.get_literal(j)) ? a.get_literal(i++) : ( (a.get_literal(i) > b.get_literal(j++)) ? b.get_literal(j) : a.get_literal(i++) );
		} while( i < sa && j < sb );
	}
	*/
	return true;
}


inline Clause* Clause::resolve(const Clause& other, Var variable) const
{
	Var var11, var12;
	Lit lit11, lit12;
	uint32_t j, sz;

	uint32_t bytes = (size() + other.size())  * sizeof(Lit);

	Lit new_lits[ bytes ];
	sz = 0;

	const Clause& cls1 = (size() < other.size()) ? (*this) : other;
	const Clause& cls2 = (size() < other.size()) ? other : (*this);

	for (uint32_t i = 0; i < cls1.  size(); i++)
	{
		lit11 = cls1.get_literal(i);
		var11 = lit11.toVar();

		if (variable == var11)
			continue;

		for (j = 0; j < cls2.  size(); j++)
		{
			lit12 = cls2.get_literal(j);
			var12 = lit12.toVar();

			if (var12 == var11) {
				if (lit11 == lit12.complement()) {
					return 0;
				}
				else
					break;
			}
		}

		if (j == cls2.size())
			new_lits[sz++] = lit11;
	}

	for (uint32_t j = 0; j < cls2.  size(); j++)
	{
		lit12 = cls2.get_literal(j);
		var12 = lit12.toVar();

		if (var12 != variable)
			new_lits[sz++] = lit12;
	}

	// Generate resolvent
	return new Clause(new_lits, sz, cls1.isLearnt() || cls2.isLearnt() );
}

inline bool Clause::check_resolve( const Clause& other, Var variable) const
{
	Var var11, var12;
	Lit lit11, lit12;

	const Clause& cls1 = (size() < other.size()) ? (*this) : other;
	const Clause& cls2 = (size() < other.size()) ? other : (*this);

	for (uint32_t i = 0; i < cls1.size(); i++)
	{
		lit11 = cls1.get_literal(i);
		var11 = lit11.toVar();

    	if (variable == var11)
    		continue;

		for (uint32_t j = 0; j < cls2.size(); j++)
		{
			lit12 = cls2.get_literal(j);
			var12 = lit12.toVar();

			if (var12 == var11) {
				if (lit11 == lit12.complement())
					return false;
				else
					break;
			}
		}
	}

	// Generate resolvant
	return true;
}

inline bool Clause::contains_literal( const Lit literal) const
{
	const uint32_t sz = size();
	for (uint32_t i = 0; i < sz; i++)
	{
		if( get_literal(i) == literal ) return true;
	}
	return false;
}


inline bool Clause::subsumes_or_simplifies_clause(const Clause& other, Lit *lit1) const
{
	// The subsuming clause has to be smaller
	const uint32_t other_clauseSize = other.size();
	const uint32_t clauseSize = size();
	if ( clauseSize > other_clauseSize) return false;

	*lit1 = NO_LIT;

	// all lits need to be oin other clause
	for (uint32_t i=0; i<clauseSize; i++) {
		uint32_t j=0;
		for (; j<other_clauseSize; j++) {
			const Lit otherJ = other.get_literal(j);
			// equal found
			if (get_literal(i) == otherJ) break;
			// find inv lit -> resolution
			if ((*lit1 == NO_LIT) && (get_literal(i) == otherJ.complement()) ) {
				*lit1 = otherJ;
				break;
			}
			// found more than one inv lit -> resolution results in taut
			if ((*lit1 != NO_LIT )&& (get_literal(i) == otherJ.complement()))
			  return false;
		}
		// literal is not in clause
		if (j==other_clauseSize) return false;
	}

	// all lits in cl -> lit1=NO_LIT, or resolution lit1 = resolution L
	return true;
}

/** sorts the literals according to their number - quadratic!
*/
inline void Clause::sort(){
    // insertion sort
    const uint32_t s = size();
    for (uint32_t j = 1; j < s; ++j)
    {
        const Lit key = get_literal(j);
        int32_t i = j - 1;
        while ( i >= 0 && get_literal(i) > key )
        {
            set_literal(i+1, get_literal(i) );
            i--;
        }
        set_literal(i+1, key);
    }
}

inline bool Clause::isSorted() const {
  const uint32_t clauseSize = size()-1;
  for( uint32_t i = 0 ; i < clauseSize; ++i ) if( get_literal(i) > get_literal(i+1) ) return false;
  return true;
}

inline bool Clause::getFlag1() const { 
	return _sizedata & 0x80000000;
}

inline void Clause::setFlag1(bool flag){ 
	 _sizedata = (_sizedata & ~0x80000000) | ((uint32_t)flag << 31); 
}

inline bool Clause::getFlag2() const { 
	 return _sizedata & 0x40000000; 
}

inline void Clause::setFlag2(bool flag){ 
	// assert(false);
	 _sizedata = (_sizedata & ~0x40000000) | ((uint32_t)flag << 30); 
}

inline bool Clause::isXor() const { 
	 return _sizedata & 0x20000000; 
}

inline void Clause::setXor(bool flag){ 
	 _sizedata = (_sizedata & ~0x20000000) | ((uint32_t)flag << 29); 
}

inline bool Clause::isKeepMe() const { 
	return _sizedata & 0x10000000; 
}

inline void Clause::setKeepMe(bool flag){ 
	 _sizedata = (_sizedata & ~0x10000000) | ((uint32_t)flag << 28); 
}

inline bool Clause::isLearnt() const { 
	 return _sizedata & 0x8000000; 
}

inline bool Clause::isCoreClause() const { 
	 return false;
}

inline void Clause::setCoreClause(){ 
	 return;
}

inline void Clause::setLearnt( bool l ){ 
	 _sizedata = (_sizedata & ~0x8000000) | ((uint32_t)l << 27); 
}

#endif // REGIONALLOCATOR

#endif // !_CACHE_Clause
