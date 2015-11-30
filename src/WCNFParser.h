#ifndef wcnf_parser_h
#define wcnf_parser_h

#include <vector>
#include <iosfwd>

void parseWCNF(std::istream & wcnf_in, std::vector<double>& out_weights,
               std::vector<int>& out_branchVars, double& out_top,
               std::vector<std::vector<int> >& out_clauses,
               std::vector<int>& assumptions);
 
#endif