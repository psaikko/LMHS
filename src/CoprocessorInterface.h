#ifndef MAXSAT_COPROCESSOR_IFACE
#define MAXSAT_COPROCESSOR_IFACE

#include <iosfwd>
#include <string>
#include "utils/stringmap.h"

static const int CI_ERR   = -1;
static const int CI_UNDEF = 1;
static const int CI_SAT   = 2;
static const int CI_UNSAT = 3;

class CoprocessorInterface {

 public:
  CoprocessorInterface();
  ~CoprocessorInterface() {};

  int CP_preprocess(std::istream & in, std::ostream & out);

  void CP_completeModel(std::istream & in, std::ostream & out);

 protected:

  StringMap commandline;
  std::string mapFileString;
};

#endif