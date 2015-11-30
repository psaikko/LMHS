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
	signal.cc
	This file is part of riss.
	
	29.04.2011
	Copyright 2011 Norbert Manthey
*/

#include "utils/interupt.h"
#include <iostream>
#include <cstdlib>

// variable to store whether there has been an interupt
static bool interupted = false;

using namespace std;

bool isInterupted()
{
	return interupted;
}

void signalHandler(int s)
{
	if( interupted ){
		cerr << "c received signal another time, abort immediatly" << endl;
		exit(-1);
	} else {
		cerr << "c received signal, interupt calculation!" << endl;
		interupted = true;
	}
}

