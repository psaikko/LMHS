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
	commandlineparser.cc
	This file is part of riss.
	
	16.10.2009
	Copyright 2009 Norbert Manthey
*/


#include "utils/commandlineparser.h"

StringMap CommandLineParser::parse(int32_t argc, char const * argv[], bool binary)
{
	map.clear();

	// setup standart configuration
	
	map.insert(  (char*)"restart_event", (char*)"luby");
	map.insert(  (char*)"removal_event", (char*)"geometric");
	map.insert(  (char*)"removal_event_geometric_max", (char*)"3000");
	map.insert(  (char*)"removal_event_geometric_inc", (char*)"1.1");
	map.insert(  (char*)"rem_keep_last", (char*)"1");

	int32_t scan_begin = 0;
	if( binary){
		// add binary
		map.insert( (char*)"binary", argv[0] );
		scan_begin++;
	}
	
	// add file ( if there is no file given, just begin scan from 1st argument
	if( scan_begin == 1 && argc > 1 )
	{
		if( argv[1][0] != '-' )
		{
			map.insert(  (char*)"file", argv[1] );
			scan_begin = 2;
		}
	}
	
	// if next is no parameter as well, assume this is the outputfile
	if( scan_begin == 2 && argc > 2 ){
		if( argv[2][0] != '-' )
		{
			map.insert(  (char*)"outfile", argv[2] );
			scan_begin = 3;
		}
	}

	// check, whether	
	for( int32_t i = scan_begin; i < argc; ++i )
	{
		if( strlen( argv[i] ) == 1 )
		{
			map.insert( argv[i], 0 );	// add single letter or symbol
			continue;
		}
		// found parameter pair
		if( argv[i][0] == '-')
		{
			string s( argv[i] );
			int32_t pos = s.find("=");
			//cerr << "found = at " << pos << endl;
			if( s.find("=") != string::npos ){
			  string key = s.substr(1,pos-1);
			  string value = s.substr(pos+1);
			  //cerr << "found key: " << key << " and value " << value << endl;
			  if( value.size() > 0 ) map.insert(  key.c_str(), value.c_str() );  
			  else {
			    // special case file:
			    if( key == "file" ){
			      // check whether next parameter could be a filename
			      if( i + 1< argc ){
							if( argv[i+1][0] != '-' ){
							  map.insert(  key.c_str(),argv[i+1] );
							  ++i;
							}
			      }
			    }
			  }
			  continue;
			}
		}
		// if there is something left, just add it without parameter
		map.insert(  argv[i], 0 );
	}
	
	// open config files
	if( map.contains( (const char*)"configfile" ) ){
		parseFile( map.get( (const char*)"configfile" ), map );
	}
	if( map.contains( (const char*)"cf" ) ){
		parseFile( map.get( (const char*)"cf" ), map );
	}	
	
	// check for -h, --help and give output
	if( map.contains( (const char*)"-h" ) || map.contains((const char*)"--help" ) || map.contains((const char*)"--verb-help" ) ){
#ifdef CSPSOLVER
		print_csp_help();
#else
		print_sat_help();
#endif
		if( !map.contains((const char*)"--help" ) && !map.contains((const char*)"--verb-help" )) exit( -1 );
	}

// setup conftime configuration
	if( map.contains((const char*)"conftime" ) ){
		// TODO FIXME
		//cerr << "c setup for " << map.get((const char*)"conftime") << "s" << endl;
		uint32_t seconds = atoi( map.get((const char*)"conftime").c_str() );
		if( seconds < 1250 ){
			map.insert(  (char*)"cdcl_probing_mode", (char*)"1" );
		}
	}


// setup confcores configuration
	if( map.contains((const char*)"confcores" ) ){
		// TODO FIXME
		//cerr << "c setup for " << map.get((const char*)"confcores") << "cores" << endl;
#ifdef PARALLEL
		// specify configuration files?

		// disable receiving for thread 0!
		map.insert( "NW_recv0", "1" );

#else
		// Pwatch?
#endif
	}

	
	if( map.contains("-h") ||  map.contains("--help") ||  map.contains("--verb-help") ){
		map.remove("file");
	}
	
	return map;
}


bool CommandLineParser::parseFile( string filename, StringMap& thisMap ){
	// parse config file!!
	vector<string> fileargv;
	ifstream file;

	// open for reading
	file.open(  filename.c_str() , ios_base::in);
	if( !file ){
		cerr << "c can not open configfile " << filename << endl;
		return false;
	}
	string line;
	while(getline (file, line)){
		fileargv.push_back(line);
	}
	// parse lines
	for( uint32_t i = 0; i < fileargv.size(); ++i ){
		if( fileargv[i].size() == 1 ){
			thisMap.insert( (char*)fileargv[i].c_str(), 0 );	// add single letter or symbol
			continue;
		}
		// found parameter pair
		if( fileargv[i][0] == '-')
		{
			string s( fileargv[i] );
			int32_t pos = s.find("=");
			// cerr << "found = at " << pos << endl;
			if( s.find("=") != string::npos ){
			  string key = s.substr(1,pos-1);
			  string value = s.substr(pos+1);
			  // cerr << "found key: " << key << " and value " << value << endl;
			  if( value.size() > 0 ) thisMap.insert(  key.c_str(), value.c_str() );  
			  continue;
			}
		}
		// if there is something left, just add it without parameter
		thisMap.insert(  fileargv[i].c_str(), 0 );
	}
	return true;
}


bool CommandLineParser::parseString( const string& commands, StringMap& thisMap ){
  Tokenizer tokenizer(commands);
  tokenizer.addSeparator(" ");
  tokenizer.addSeparator("\t");
  
  vector< string > parameterLines;
  // go through all tokens and parse for parameters
  while( tokenizer.hasNext() ){
    
	string token = tokenizer.nextToken();
	//cerr << "got token " << token << endl;
      
	if( token.size() == 1 ){
		thisMap.insert( (char*)token.c_str(), 0 );	// add single letter or symbol
		continue;
	}
	// found parameter pair
	if( token[0] == '-')
	{
		int32_t pos = token.find("=");
	//	cerr << "found = at " << pos << endl;
		if( token.find("=") != string::npos ){
		  string key = token.substr(1,pos-1);
		  string value = token.substr(pos+1);
	//	  cerr << "found key: " << key << " and value " << value << endl;
		  if( value.size() > 0 ) thisMap.insert(  key.c_str(), value.c_str() );  
		  continue;
		}
	}
	// if there is something left, just add it without parameter
	//cerr << "found parameter without value: " << token << endl;
	thisMap.insert(  token.c_str(), 0 );
  }
  return true;
}


void CommandLineParser::print_sat_help()
{
		string binary = map.get((const char*)"binary" );
		cerr << "usage:" << endl;
		cerr << binary << " [File] [Outfile] -Name=Value" << endl;
		cerr << binary << " -file=Filename -configfile=ConfigFile" << endl ;
#ifndef COPROCESSOR
		cerr << binary << " -file=Filename -outfile=Filename -Name=Value" << endl;
		cerr << endl;
		cerr << "examples:" << endl;
		cerr << binary << " sat.cnf solution.cnf" << endl;
		cerr << binary << " -restart_event=dynamic -file=sat.cnf -outfile=solution.cnf" << endl << endl;
#endif // #ifndef COPROCESSOR
		cerr << "explanation:" << endl;
		cerr << "\t\tName ... name of the parameter." << endl;
		cerr << "\t\tValue ... value of the according parameter." << endl << endl;
		cerr << "\t\tcf ,configfile ... txt file where further arguments are listed(one per line)" << endl << endl;
		cerr << "\t\t-h          ... shows this info." << endl ;
#ifndef TESTBINARY
		cerr << "\t\t--help      ... shows long help" << endl ;
		cerr << "\t\t--verb-help ... shows verbose long help" << endl ;
#endif 
#ifndef COPROCESSOR
		cerr << "\t\t-q     ... stops outputing the found satisfying assignment." << endl;
		cerr << "\t\t-sol=N ... number of solutions to find (-1 = all)." << endl;
#endif // #ifndef COPROCESSOR
#ifdef COPROCESSOR
		cerr << "\t\t-completeModel ... tell coprocessor that it should extend the model again from the mapFile (CP_mapFile)" << endl;
		  
#endif
#ifndef TESTBINARY
#ifndef COMPETITION
		cerr << "\t\t-v  ... shows more detailed info." << endl; 
		cerr << "\t\t-i  ... shows information about the build process." << endl;
		//cerr << "\t\t-analyze_formula ... dumps formula analysation into formula.csv" << endl;
		if( map.contains((const char*)"--help" ) ){
			/*
			cerr << "COMPILE FLAGS:" << endl;
			cerr << "parameter:" << endl;
			cerr << "  USE_ALL_COMPONENTS ... enables component switching after compiling(huge binary!)" << endl;
			cerr << "  USE_SOME_COMPONENTS ... enables some components" << endl;
			cerr << "  USE_COMMANDLINEPARAMETER ... enables parameter setting via commandline" << endl;
			cerr << "structures:" << endl;
			cerr << "  USE_C_XYZ with XYZ { VECTOR,STACK,RINGBUFFER } ... structure type" << endl;
			cerr << "  USE_C_HEAP, USE_CPP_HEAP ... heap as struct or object" << endl;
			cerr << "  or combined USE_C_STRUCTURES, USE_STL_STRUCTURES ... sets all structures" << endl;
			cerr << "  EXPERIMENTAL_SIZE_ASSIGNMENT ( see defines.h for detail! )" << endl;
			cerr << "  EXPERIMENTAL_PACK_ASSIGNMENT ... compress assignment representation" << endl;
			cerr << "  EXPERIMENTAL_BOOLARRAY ... store bool as single bits" << endl;
			cerr << "  USE_C_Clause,USE_CPP_Clause,USE_C_CACHE_Clause,USE_CPP_CACHE_Clause ... choose clause type" << endl;
			cerr << "  further clause types: USE_CPP_SLAB_Clause, USE_CPP_CACHE_SLAB_Clause" << endl;
			cerr << "implementation changes:" << endl;
			cerr << "  USE_PREFETCHING ... enables the prefetch function" << endl;
			cerr << "     PREFETCHINGMETHOD1 ... prefetch just before propagating" << endl;
			cerr << "     PREFETCHINGMETHOD1 ... prefetch between enqueing and propagating" << endl;
			cerr << "  COMPRESS_Clause ... packs the clause structure" << endl;
			cerr << "  COMPRESS_WATCH_STRUCTS ... packs watch list structures" << endl;
			cerr << "  ANALYZER_VEC_MEMBER ... creates the vectors for analysation only once" << endl;
			*/
		}
#endif
#endif
}


void CommandLineParser::print_csp_help()
{
		string binary = map.get((const char*)"binary" );
		cerr << "csp solving is not supportet at the moment" << endl;
		
		/*
		cerr << "usage:" << endl;
		cerr << binary << " [File] [Outfile] -PName Value" << endl;
		cerr << binary << " -file Filename -Poutfile Filename -PName Value" << endl;
		cerr << binary << " -file Filename -Pconfigfile ConfigFile" << endl << endl;
		cerr << "\t\tName ... name of the parameter." << endl;
		cerr << "\t\tValue ... value of the according parameter." << endl << endl;
		cerr << "\t\tcf ,configfile ... txt file where further arguments are listed(one per line)" << endl << endl;
		cerr << "\t\t-h          ... shows this info." << endl ;
		cerr << "\t\t--help      ... shows long help" << endl ;
		cerr << "\t\t--verb-help ... shows verbose long help" << endl ;
		cerr << "\t\t-q ... stops outputing the found satisfying assignment." << endl;
		cerr << "\t\t-Pconftime t ... setup a good configuration for runtime t (seconds)" << endl;
		cerr << "\t\t-Pconfcores c ... setup a good configuration for cores c" << endl;
#ifndef TESTBINARY
#ifndef COMPETITION
		cerr << "\t\t-v  ... shows more detailed info." << endl; 
		cerr << "\t\t-i  ... shows information about the build process." << endl;
#endif
#endif
*/
}
