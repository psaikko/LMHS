#pragma once

#include <vector>
#include <iosfwd>

#include "Weights.h"

void parseWCNF(std::istream & wcnf_in, std::vector<weight_t>& out_weights,
               std::vector<int>& out_branchVars, weight_t& out_top,
               std::vector<std::vector<int> >& out_clauses,
               std::vector<int>& assumptions, int& n_vars);