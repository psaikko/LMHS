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
	assignment.h
	This file is part of riss.
	
	16.10.2009
	Copyright 2009 Norbert Manthey
*/

/*
	assumption: Var is some positiv integer
*/

#ifndef _ASSIGNMENT_H
#define _ASSIGNMENT_H

#include <inttypes.h>	// for uint8_t
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "types.h"
#include <vector>
#include "structures/literal_system.h"

#include "defines.h"
#include "macros/malloc_macros.h"

//typedef void* Assignment;

class Assignment {
private:
	void* assignment;
	

	

public:
	Assignment( uint32_t variables ):assignment(0) { create(variables); }
	Assignment( void* memory ): assignment(memory) {}

	// copy constructor
	/// NOTE: this constructor does not copy the data, but only the wrapping object
	Assignment( const Assignment& other) {assignment = other.assignment;}
	
	/// NOTE: this operator does not copy the data, but only the wrapping object
	Assignment& operator=(const Assignment& other){assignment = other.assignment; return *this;}
  
  
	bool operator <=(Assignment& other) const {
	  return assignment == other.assignment;
	}
	
	// to check the state of the assignment
	void init(){ assignment = 0;}
	bool isUninitialized() const { return assignment == 0; }
  
	// initializes the assignment by creating space for varCnt variables
	void create(const uint32_t varCnt);

	void extend(const uint32_t varCnt, const uint32_t newVars);

	Assignment copy(const uint32_t variables) const;

	/**
	* NOTE: Assumes that there is already enough space in the assignment
	*/
	void copy_from(const Assignment& source, const uint32_t variables);

	void destroy();

	void clear(const uint32_t varCnt);

	Polarity get_polarity( const Var variable ) const;

	void set_polarity(const Var variable, const Polarity polarity );
	
	void set_backup(const Var variable, const Polarity polarity );

	Polarity get_backup_polarity(const Var variable ) const;

	void init_polarity_backup(const Var variable, const Polarity polarity );

	void set_polarity_backup(const Var variable, const Polarity polarity );

	void invert_variable(const Var variable);

	void undef_variable(const Var variable);

	void undef_variable_backup(const Var variable);

	bool variable_equal(const Var variable, Polarity value) const;


	// implementations for agility
	inline float& get_agility(){
		static float a = 0;
		return a;
	}

	inline float& get_aging(){
		static float g = 0.9999;
		return g;
	}

	inline float& get_naging(){
		static float g = 1-get_aging();
		return g;
	}

	inline void update_agility(const bool newValue = false){
	//	std::cerr << "c old: " << get_agility() << " call assignment update with " << newValue;
		get_agility() *= get_aging();
		if( newValue ) get_agility() += get_naging();
	//	std::cerr << "new: " << get_agility() << std::endl;
	}

	inline bool is_sat( const Lit l ) const{
		return variable_equal(l.toVar(), l.getPolarity() );
	}

	inline bool is_unsat(const Lit l) const{
		return variable_equal(l.toVar(), l.complement().getPolarity() );
	}

	inline bool is_undef(const Var v) const{
		return variable_equal(v, UNDEF );
	}

	/** check two assignments for common occurences
	 *
	 * common literals are added to the units vector
	 * if variables occure in two different polarities, they are added to equals
	 * @return -1 if a conflict occured, number of units otherwise
	 */
	inline int32_t get_equal_lits(const Assignment& b, Assignment& before, std::vector<Lit>& units, std::vector<Lit>& equals, const uint32_t varCnt) const;


};

/*****

	developer section

****/

inline void Assignment::create(const uint32_t varCnt)
{
	// need twice as much space, as varCnt, because of backup
	assignment = MALLOC( (varCnt + 1) * sizeof( uint8_t ) * 2 );
	memset( assignment, 0, (varCnt + 1) * sizeof( uint8_t ) * 2 );
}

inline void Assignment::clear( const uint32_t varCnt){
	memset( assignment, 0, (varCnt + 1) * sizeof( uint8_t ) * 2 );
}

inline Assignment Assignment::copy(const uint32_t varCnt)  const
{
	// need twice as much space, as varCnt, because of backup
	void* memory = MALLOC( (varCnt + 1) * sizeof( uint8_t ) * 2);
	memcpy( memory, assignment, (varCnt + 1) * sizeof( uint8_t ) * 2 );
	Assignment aC(memory);
	return aC;
}

inline void Assignment::copy_from(const Assignment& source, const uint32_t varCnt)
{
  memcpy( assignment, source.assignment, (varCnt + 1) * sizeof( uint8_t ) * 2 );
}

inline void Assignment::extend(const uint32_t varCnt, const uint32_t newVars){
	if( assignment != 0 ) {
	// get new memory	
	assignment = realloc( assignment, (newVars + 1) * sizeof( uint8_t ) * 2);

	uint8_t* memory = (uint8_t*)assignment;
	// undefine new variables
	if( varCnt == 0 ) memset( assignment, 0, (newVars + 1) * sizeof( uint8_t ) * 2 );
	if( newVars > varCnt ){
		// FIXME: do not overwrite values!
		const uint32_t diff = newVars - varCnt;
		memset( memory+ (2 * (varCnt + 1)), 0, (diff) * sizeof( uint8_t ) * 2 );
	}
	} else {
	  // need twice as much space, as varCnt, because of backup
	  assignment = MALLOC( (newVars + 1) * sizeof( uint8_t ) * 2 );
	  memset( assignment, 0, (newVars + 1) * sizeof( uint8_t ) * 2 ); 
	}
}

inline void Assignment::destroy()
{
	if( assignment != 0 )
	{
		free(assignment);
		assignment = 0;
	}
}

inline Polarity Assignment::get_polarity(const Var variable ) const
{
	// return value from old position
	return (Polarity)( ( (uint8_t*)assignment)[variable * 2] );
}

inline void Assignment::set_polarity(const Var variable, const Polarity polarity )
{
	assert(polarity != UNDEF);
#ifdef ASSI_AGILITY
//	std::cerr << "c set " << variable << " from " << (int)( ( (uint8_t*)assignment)[variable * 2 + 1] ) << " to " << polarity << std::endl;
	update_agility( ( (uint8_t*)assignment)[variable * 2 + 1] != UNDEF && ( (uint8_t*)assignment)[variable * 2 + 1] != polarity);
#endif
	( ( (uint8_t*)assignment)[variable * 2] ) 
		= (uint8_t)polarity;
	/*
	PicoSAT agility (a) and factor g (0<g<1):
	a=a*g
	a=(backup != new) ? a+(1-g) : a;
	*/
}

inline Polarity Assignment::get_backup_polarity(const Var variable ) const
{
	// return value from old position
	return (Polarity)( ( (uint8_t*)assignment)[variable * 2 + 1] );
}

inline void Assignment::init_polarity_backup(const Var variable, const Polarity polarity )
{
		( ( (uint8_t*)assignment)[variable * 2 + 1] ) = (uint8_t)polarity;
}

inline void Assignment::set_backup(const Var variable, const Polarity polarity )
{
#ifdef ASSI_AGILITY
//	std::cerr << "c set " << variable << " from " << (int)( ( (uint8_t*)assignment)[variable * 2 + 1] ) << " to " << polarity << std::endl;
	update_agility( ( (uint8_t*)assignment)[variable * 2 + 1] != UNDEF && ( (uint8_t*)assignment)[variable * 2 + 1] != polarity);
#endif
	( ( (uint8_t*)assignment)[variable * 2 + 1] ) = (uint8_t)polarity;
}

inline void Assignment::set_polarity_backup(const Var variable, const Polarity polarity )
{
	assert(polarity != UNDEF);
#ifdef ASSI_AGILITY
//	std::cerr << "c set " << variable << " from " << (int)( ( (uint8_t*)assignment)[variable * 2 + 1] ) << " to " << polarity << std::endl;
	update_agility( ( (uint8_t*)assignment)[variable * 2 + 1] != UNDEF && ( (uint8_t*)assignment)[variable * 2 + 1] != polarity);
#endif
	( ( (uint8_t*)assignment)[variable * 2 + 1] ) = ( (uint8_t*)assignment)[variable * 2 ];
	( ( (uint8_t*)assignment)[variable * 2] ) = (uint8_t)polarity;
}

inline void Assignment::invert_variable(const Var variable){
	( ( (uint8_t*)assignment)[variable * 2] ) = inv_pol( (Polarity)( ( (uint8_t*)assignment)[variable * 2] ) );
}

inline void Assignment::undef_variable(const Var variable)
{
	( ( (uint8_t*)assignment)[variable * 2] ) = (uint8_t)0;
}

inline void Assignment::undef_variable_backup(const Var variable)
{
	( ( (uint8_t*)assignment)[variable * 2 + 1] ) = ( (uint8_t*)assignment)[variable * 2 ];
	( ( (uint8_t*)assignment)[variable * 2] ) = (uint8_t)0;
}

inline bool Assignment::variable_equal(const Var variable, Polarity value) const
{
	return ( (uint8_t)value == ( (uint8_t*)assignment)[variable * 2] );
}

inline int32_t Assignment::get_equal_lits(const Assignment& b, Assignment& before, std::vector<Lit>& units, std::vector<Lit>& equals, const uint32_t varCnt) const
{
	uint8_t* pa = (uint8_t*)assignment;
	uint8_t* pb = (uint8_t*)b.assignment;
	uint32_t count = 0;

	// TODO: check, whether if in an own for loop would perform faster
	for( uint32_t i = 1 ; i < varCnt + 1; ++ i){
		// only new thins if not already set in before assignment
		if( ! before.is_undef(i) ) continue;
		// found a unit?
		if( pa[2*i] == pb[2*i] &&  (Polarity)pb[2*i] != UNDEF){
			// conflict? -> UNSAT! - how to handle the variable which has been used for probing?
			if( before.variable_equal( i, inv_pol( (Polarity)pb[2*i] ) )  ) return -1;
			// found unit? add it and inc counter		
			units.push_back( Lit( i, (Polarity)pa[2*i] ) );
			count ++;
			continue;
		}
		// look for implications
		if( pa[2*i] != (uint8_t)UNDEF &&  pb[2*i] == (uint8_t)inv_pol( (Polarity)pa[2*i]) ){
			equals.push_back( Lit( i, (Polarity)pa[2*i] ) );
		}
	}
	// return how much units has been found
	return count;
}

/*
// switch
#endif
*/
#endif
