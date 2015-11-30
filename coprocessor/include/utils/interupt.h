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
        signal.h
        This file is part of riss.

        29.04.2011
        Copyright 2011 Norbert Manthey
*/

#ifndef _INTERUPT_H
#define _INTERUPT_H

#include <signal.h>

/** return whether a signal was send from the outside
*
*		call this method frequently in your search loop!
*		if this method returns true, abort you calculation and
*		return a 0-pointer, to indicate that no solution has been found
*/
bool isInterupted();

/** method to handle a signal
*/
void signalHandler(int s);

/** method to handle the ulimit timeout-signal
*/
void timeoutHandler(int s);

#endif
