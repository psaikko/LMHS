// adapted from minisat dimacs.h
#include <algorithm>
#include <istream>
#include <ostream>
#include <iostream>
#include <string>
#include <sstream>
#include <cassert>
#include <unordered_set>

#include "WCNFParser.h"
#include "Util.h"
#include "GlobalConfig.h"
#include "Weights.h"

using namespace std;

bool weighted;

void skipLine(istream &in) {
  for (;;) {
    if (in.eof()) return;
    if (in.peek() == '\n') {
      in.get();
      return;
    }
    in.get();
  }
}

bool eagerMatch(istream & in, const char* str) {
  for (; *str != '\0'; ++str, in.get())
    if (*str != in.peek()) return false;
  return true;
}

void skipWhitespace(istream & in) {
  char c = in.peek();
  while (!in.eof() && ((c >= 9 && c <= 13) || c == 32)) {
    in.get();
    c = in.peek();
  }
}

void scanNext(istream & in) { 
  char c = in.peek();
  while (!in.eof() && (c == ' ' || c == '\t')) {
    in.get();
    c = in.peek(); 
  }
}

void readClause(istream & in, weight_t& out_w, vector<int>& out_lits, int& n_vars, unordered_set<int>& prob_vars) {
  int parsed_lit;

  if (weighted) {
    if (!(in >> out_w)) {
      terminate(1, "WCNF parse error - bad clause weight\n");
    }
  } else {
    out_w = 1;
  }

  int lits = 0;
  for (;;) {
    in >> parsed_lit;
    if (parsed_lit == 0) {
      assert(lits);
      break;
    }
    prob_vars.insert(abs(parsed_lit));
    ++lits;
    n_vars = max(abs(parsed_lit), n_vars);
    out_lits.push_back(parsed_lit);
  }
}

void parseWCNF(istream & wcnf_in, vector<weight_t>& out_weights,
               vector<int>& out_branchVars, weight_t& out_top,
               vector<vector<int> >& out_clauses, 
               vector<int>& file_assumptions, int& n_vars) {
  unsigned n_clauses = 0, cnt = 0, soft_ct = 0, hard_ct = 0;

  weight_t w = 0;

  bool p_line_parsed = false;

  unordered_set<int> parsed_vars;

  for (;;) {
    skipWhitespace(wcnf_in);
    if (wcnf_in.eof()) {
      break;
    } else {
      switch(wcnf_in.peek()) {
        case 'p':
        {
          string line;
          getline(wcnf_in, line);
          stringstream pline(line);
          pline.get(); // chomp 'p'
          string type;
          pline >> type >> n_vars >> n_clauses;
          weighted = (type == "wcnf");
          if (weighted) {
            if (!(pline >> out_top)) out_top = WEIGHT_MAX;
          } else if (type == "cnf") {
            out_top = WEIGHT_MAX;
          } else {
            terminate(1, "WCNF parse error - unexpected 'p' line:\n%s\n", line.c_str());
          }
          out_clauses.reserve(n_clauses);
          p_line_parsed = true;
          break;
        }
        case 'c':
        {
          if (eagerMatch(wcnf_in, "c assumptions")) {
            scanNext(wcnf_in);        
            int c = wcnf_in.peek();
            while(!wcnf_in.eof() && c != '\n' && c != 0) {
              int a;
              wcnf_in >> a;
              file_assumptions.push_back(a);
              scanNext(wcnf_in);
              c = wcnf_in.peek();
            }
          }
          skipLine(wcnf_in);
          break;
        }
        case 'b':
        {
          wcnf_in.get();
          for (;;) {
            int v;
            wcnf_in >> v;
            if (v == 0) break;
            out_branchVars.push_back(v);
          }
          break;
        }
        default:
        {
          if (!p_line_parsed) 
            terminate(1, "WCNF parse error - clause read attempt before 'p' line\n");
          cnt++;
          out_clauses.push_back(vector<int>());
          readClause(wcnf_in, w, out_clauses.back(), n_vars, parsed_vars);

          if (w != 0) {
            // skip all 0-weight clauses
            if (w == out_top) ++hard_ct;
            else              ++soft_ct;

            out_weights.push_back(w);
          } else {
            out_clauses.pop_back();
          }
        }
      }
    }
  }

  printf("c Parsed %lu vars\n", parsed_vars.size());
}