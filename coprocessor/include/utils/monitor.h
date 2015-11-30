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
        monitor.h
        This file is part of riss.

        22.08.2010
        Copyright 2010 Norbert Manthey
*/

#ifndef _MONITOR_H
#define _MONITOR_H

/** This file implements the interface for the graphical analysis tool for the
 * SAT Solver.
*/

#include "defines.h"

#ifndef USE_DATAMONITOR

#define MON_REG(name, type, dataT)
#define MON_EVENT(name, adress)
#define MON_WAIT_GUI()
#define MON_WAIT_WINDOW()

#else

// include path has to be added to build
#include "DataMonitorLib.hh"

extern DataMonitor dm;

// call this at least once in all the work loops!
#define MON_WAIT_GUI() dm.waitForGUI()

// call this if the program would terminate without the monitor
#define MON_WAIT_WINDOW() dm.waitForWindow()

// see documentation of DataMonitor for more help and usage!
// datatype: 'c'8 'i'32 'u'32 'l'64 'k'u64 'f'32 'd'64
#define MON_REG(name, type, dataT) dm.reg(name, type, dataT)
#define MON_EVENT(name, adress) dm.event(name, adress)

#endif

#endif
