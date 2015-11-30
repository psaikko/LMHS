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
	clause_container.cc
	This file is part of riss.
	
	15.07.2010
	Copyright 2010 Norbert Manthey
*/

#include "utils/clause_container.h"

#ifdef JAVALIB
/*
#ifndef PARALLEL
	GrowSlabAllocator< Clause > gsa(20);
#else
	GrowThreadMemory< Clause > gsa(20);
#endif

#else

extern "C" {
	#ifndef PARALLEL
		GrowSlabAllocator< Clause > gsa;
	#else
		GrowThreadMemory< Clause > gsa;
	#endif
}
*/
#endif
