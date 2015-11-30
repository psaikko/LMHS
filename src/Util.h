#ifndef maxsat_util_h
#define maxsat_util_h

#include <string>
#include <vector>
#include <time.h>   		 // clock, CLOCKS_PER_SEC
#include <limits.h> 		 // INT_MAX etc
#include <float.h>  		 // FLT_MAX
#include <cstdarg>

#define SECONDS(clocks) (((double)(clocks)) / CLOCKS_PER_SEC)
#define EPS 1e-7

bool string_contains(const std::string & a, const std::string & b);

bool string_beginsWith(const std::string & a, const std::string & b);

bool string_endsWith(const std::string & a, const std::string & b);

// a subset of b ?
bool core_subset(const std::vector<int> & a, const std::vector<int> & b);

std::string trim(const std::string & s);

void terminate(const int code, const char * fmt, ...);

void condTerminate(const bool cond, const int code, const char * fmt, ...);

void log(const int level, const char * fmt, ...);

void condLog(const bool cond, const int level, const char * fmt, ...);

void logCore(const int level, std::vector<int> & core);

void logHS(const int level, std::vector<int> & hs, bool optimal);

#endif