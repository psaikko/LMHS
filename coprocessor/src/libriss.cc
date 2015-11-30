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
        libriss.cc
        This file is part of riss.

        24.03.2011
        Copyright 2011 Norbert Manthey
*/

#ifndef COPROCESSOR

#include "libriss.h"

#include "sat/component.h"

#include "structures/assignment.h"
#include "structures/literal_system.h"
#include "types.h"

#include "structures/clause.h"
#include "utils/clause_container.h"
#include "utils/stringmap.h"

// for solve
#include "utils/network.h"

// for step by step solver
#include "preprocessor/coprocessor2.h"

/** structure for step by step search
* provides all necessary information, so that search can be called again and
*again
*
*/
#include <search/cdclsearch.h>

class Solver {
 public:
  // current search state
  searchData* data;
  // preprocessor
  Coprocessor2* preprocessor;
  // search
  CdclSearch<UnitPropagation, VariableActivityHeuristic,
             GeometricEventHeuristic, GeometricEventHeuristic, ClauseManager,
             Coprocessor2>* search;
  // parameter
  StringMap parameter;
  // network, necessary dummy for search
  Network* nw;

  // formula
  vector<CL_REF> clause_set;
  Allocator clauseStorage;  /// global storage object

 public:
  Solver() : data(0), preprocessor(0), search(0), clauseStorage(20) {}

  ~Solver() {
    if (search != 0) delete search;
    if (preprocessor != 0) delete preprocessor;

    if (nw != 0) delete nw;

    // free the clauses
    for (uint32_t i = 0; i < clause_set.size(); ++i) {
      Clause& cl = data->gsa.get(clause_set[i]);
      const uint32_t s = cl.getWords();
      cl.Clause::~Clause();
      data->gsa.release(clause_set[i], s);
    }
    clause_set.clear();

    if (data != 0) delete data;
  }
};

Riss::Riss() : pointer(0) {}

Riss::~Riss() {
  if (pointer != 0) {
    Solver* solver = (Solver*)pointer;
    delete solver;
    pointer = 0;
  }
}

int Riss::solve(const vector<vector<int> >& formula, vector<int>& solution,
                const string& commandline) {
  solution_t state = UNKNOWN;

  // parse commandline
  StringMap parameter;
  CommandLineParser cmp;
  cmp.parseString(commandline, parameter);

  // setup components
  ComponentManager cm(parameter);
  Network nw(parameter);

  // parse formula
  unsigned int variables = 0;
  vector<CL_REF> clause_set;
  vector<Lit> currentClause;

  Allocator globalClauseStorage;  /// global storage object
  searchData data(globalClauseStorage);
  for (unsigned int i = 0; i < formula.size(); ++i) {
    // empty clause?
    if (formula[i].size() == 0) {
      state = UNSAT;
      cerr << "c empty clause in input" << endl;
      break;
    }
    currentClause.clear();
    for (unsigned int j = 0; j < formula[i].size(); ++j) {
      const Lit l(formula[i][j]);
      // count highest variable
      variables = l.toVar() > variables ? l.toVar() : variables;
      currentClause.push_back(l);
    }
    // add clause to formula in solver representation
    clause_set.push_back(data.gsa.create(currentClause, false));
  }

  cerr << "c state after parsing: " << endl;
  data.extend(variables);
  // solve formula

  if (state == UNKNOWN) {
    cerr << "c solve formula" << endl;
    std::vector<Assignment>* solutions =
        nw.solve(&clause_set, data, parameter, cm);
    cerr << "c solutions: " << std::hex << solutions << std::dec << endl;
    // if( solutions != 0 ) cerr << "c size: " << solutions->size() << endl;
    if (solutions == 0 || solutions->size() == 0) {
      state = UNSAT;
      solution.clear();
    } else {
      state = SAT;
      const Assignment& s = (*solutions)[0];
      // fill solution with model
      solution.clear();
      for (uint32_t i = 1; i <= variables; ++i) {
        solution.push_back(s.get_polarity(i) == POS ? i : 0 - i);
      }
    }
    delete solutions;
  }

  // free the clauses
  for (uint32_t i = 0; i < clause_set.size(); ++i) {
    Clause& cl = data.gsa.get(clause_set[i]);
    const uint32_t s = cl.getWords();
    cl.Clause::~Clause();
    data.gsa.release(clause_set[i], s);
  }

  clause_set.clear();

  return (int)state;
}

int Riss::init(const vector<vector<int> >& formula, vector<int>& solution,
               const string& commandline) {
  solution_t state = UNKNOWN;

  Solver* solver = new Solver();
  // parse commandline

  CommandLineParser cmp;
  cmp.parseString(commandline, solver->parameter);

  // setup components
  ComponentManager cm(solver->parameter);

  // used as a dummy
  Network nw(solver->parameter);

  // parse formula
  uint32_t variables = 0;

  vector<Lit> currentClause;
  for (unsigned int i = 0; i < formula.size(); ++i) {
    // empty clause?
    if (formula[i].size() == 0) {
      state = UNSAT;
      break;
    }
    currentClause.clear();
    for (unsigned int j = 0; j < formula[i].size(); ++j) {
      const Lit l(formula[i][j]);
      // count highest variable
      variables = l.toVar() > variables ? l.toVar() : variables;
      currentClause.push_back(l);
    }
    // add clause to formula in solver representation
    solver->clause_set.push_back(
        solver->clauseStorage.create(currentClause, false));
  }

  // preprocess formula
  solver->data = new searchData(solver->clauseStorage, variables);

  if (state != UNSAT) {
    solver->preprocessor = new Coprocessor2(&(solver->clause_set),
                                            *(solver->data), solver->parameter);
    state = solver->preprocessor->preprocess(&(solver->clause_set));
  }
  if (state != UNSAT) {
    solver->nw = new Network(solver->parameter);
    solver->search = new CdclSearch<
        UnitPropagation, VariableActivityHeuristic, GeometricEventHeuristic,
        GeometricEventHeuristic, ClauseManager, Coprocessor2>(
        *(solver->data), &(solver->clause_set), *(solver->preprocessor),
        solver->parameter, *(solver->nw), -1);

    // init!
    state = solver->search->init();
  }

  if (state == UNSAT) {
    delete solver;

    return 20;
  } else {
    pointer = solver;
    return state;
  }
}

int Riss::search(vector<int>& solution, vector<int>& assumptions) {
  if (pointer == 0) return -3;
  Solver* solver = (Solver*)pointer;

  searchData& sd = *(solver->data);

  vector<Lit> assump;
  for (unsigned int i = 0; i < assumptions.size(); ++i) {
    const Lit l(assumptions[i]);
    // is there an eliminated variable in the assumptions
    if (sd.eliminated.get(l.toVar())) return -1;
    // is there a not original variable?
    if (sd.original_var_cnt < l.toVar()) return -2;
    assump.push_back(l);
  }

  vector<Assignment>* solutions = solver->search->search(assump);

  if (solutions == 0 ||
      solutions->size() == 0) {  // UNSAT -> assignment is clear!
    solution.clear();
    // TODO: do conflict analysis for very last conflict here!
  } else {
    const Assignment& s = (*solutions)[0];
    // fill solution with model
    solution.clear();
    for (uint32_t i = 1; i <= sd.var_cnt; ++i) {
      solution.push_back(s.get_polarity(i) == POS
                             ? i
                             : (s.get_polarity(i) == NEG) ? 0 - i : 0);
    }
  }

  return sd.current_level;
}

int Riss::postprocess(vector<int>& solution) {
  if (pointer == 0) return -3;
  Solver* solver = (Solver*)pointer;
  searchData& sd = *(solver->data);

  Assignment assignment(sd.var_cnt);
  vector<Assignment> sols;

  // convert assignment for postprocessor (use assignment does not contain
  // variable 0)
  for (int32_t i = 0; i < (int32_t)sd.var_cnt; ++i) {

    if (solution[i] == i + 1)
      assignment.set_polarity(i + 1, POS);
    else if (solution[i] == -i - 1)
      assignment.set_polarity(i + 1, NEG);
  }

  // apply postprocessing
  sols.push_back(assignment);
  solver->preprocessor->postprocess(&sols);

  // convert assignment back for user
  solution.clear();
  for (uint32_t i = 1; i <= sd.var_cnt; ++i) {
    solution.push_back(sols[0].get_polarity(i) == POS
                           ? i
                           : (sols[0].get_polarity(i) == NEG) ? 0 - i : 0);
  }

  return 0;
}

#endif  // #ifndef COPROCESSOR
