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
        rissmain.cc
        This file is part of riss.

        09.08.2010
        Copyright 2009 Norbert Manthey
*/
#include "sat/rissmain.h"
#include "structures/region_memory.h"
#include "structures/boolarray.h"
#include "utils/VarShuffler.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>

#ifdef SATSOLVER

void printMemory() {
  // show memory usage after run
  char buf[100];
  uint32_t memory = 0;
  snprintf(buf, 99, "/proc/%u/stat", (unsigned)getpid());
  FILE* pf = fopen(buf, "r");
  if (pf) {
    if (fscanf(pf,
               "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u "
               "%*d %*d %*d %*d %*d %*d %*e %u",
               &memory) != 1)
      memory = 0;
    fclose(pf);
  }
  std::cerr << "c memory usage (vsize): " << memory / 1024 / 1024 << " MB"
            << std::endl;
}

int32_t rissmain(std::ostream& err_out, StringMap& commandline) {

  int32_t res;

  std::istream& data_in = std::cin;
  std::ostream& data_out = std::cout;

  std::ofstream null_out;
  null_out.open("/dev/null");

  std::string mapfile;

  if (!commandline.contains("CP_mapFile")) {
    err_out << "c CP_mapFile unspecified" << std::endl;
    mapfile = "/dev/null";
  } else {
    mapfile = commandline.get("CP_mapFile");
  }

  // postprocessing can be done in both binaries
  if (commandline.contains("-completeModel")) {

    std::ifstream map_in;
    if (mapfile == "/dev/null") exit(1);
    map_in.open(mapfile.c_str());

    res = completeModel(data_out, data_in, map_in, err_out, commandline);
    map_in.close();
  } else {
    std::ofstream map_out;
    map_out.open(mapfile.c_str());

    std::string filename = commandline.get((const char*)"file");
    if (filename.length() > 0) {
      std::ifstream file_in;
      file_in.open(filename.c_str());

      res = preprocess(data_out, file_in, map_out, err_out, commandline);

      file_in.close();
    } else {
      res = preprocess(data_out, data_in, map_out, err_out, commandline);
    }

    map_out.close();
  }

  return res;
}

void printSolution(std::ostream& data_out, std::ostream& err_out,
                   StringMap& commandline, uint32_t var_cnt,
                   std::vector<Assignment>& solutions) {

  if (isInterupted()) {
    err_out << "c solver has been interupted" << std::endl;
    // data_out << "s UNKNOWN" << std::endl;
  } else if (solutions.size() == 0) {
    // data_out << "s UNSATISFIABLE" << std::endl;
    if (commandline.contains((const char*)"outfile")) {
      std::ofstream solutionFile(
          commandline.get((const char*)"outfile").c_str());
      solutionFile << "s UNSATISFIABLE" << std::endl;
    }
  } else {
    data_out << "s OPTIMUM FOUND" << endl;
    // if solver should not be quiet print solution
    if (!commandline.contains((const char*)"-q") &&
        !commandline.contains((const char*)"outfile")) {
      for (int32_t i = solutions.size(); i > 0; --i) {
        Assignment solution = solutions[i - 1];
        data_out << "v";
        for (uint32_t i = 1; i <= var_cnt; ++i)
          data_out << " " << Lit(i, solution.get_polarity(i)).nr();
        // data_out << " 0" << std::endl;
        data_out << std::endl;
      }
    } else {
      if (commandline.contains((const char*)"outfile")) {
        std::ofstream solutionFile(
            commandline.get((const char*)"outfile").c_str());
        // solutionFile << "s SATISFIABLE" << std::endl;
        if (!commandline.contains((const char*)"-q")) {
          for (int32_t i = solutions.size(); i > 0; --i) {
            Assignment solution = solutions[i - 1];
            solutionFile << "v";
            for (uint32_t i = 1; i <= var_cnt; ++i)
              solutionFile << " " << Lit(i, solution.get_polarity(i)).nr();
            // solutionFile << " 0" << std::endl;
            solutionFile << std::endl;
          }
        }
      }
    }
  }
}

int32_t completeModel(std::ostream& data_out, std::istream& data_in,
                      std::istream& map_in, std::ostream& err_out,
                      StringMap& commandline) {
  vector<CL_REF> clause_set;
  // Section of preprocessor
  Allocator globalClauseStorage;  /// global storage object
  searchData sd(globalClauseStorage);
  Coprocessor2 preprocessor(&clause_set, sd, commandline, 0);

  err_out << "c load post process information" << std::endl;
  int32_t var_cnt = preprocessor.loadPostprocessInfo(map_in, err_out);
  if (var_cnt == -1) {
    err_out << "c error during loading post-process information" << std::endl;
    err_out << "c ABORT" << std::endl;
    exit(-1);
  } else {
    err_out << "c original variables: " << var_cnt
            << " search variables: " << sd.var_cnt << std::endl;
    sd.var_cnt = (int32_t)sd.var_cnt > var_cnt ? sd.var_cnt : var_cnt;
  }

  err_out << "c waiting for model on stdin" << std::endl;
  string line;
  string oline;
  vector<Lit> literals;
  Assignment assignment(sd.var_cnt);

  solution_t solution = UNKNOWN;
  while (getline(data_in, line)) {

    if (startsWith(line, "s")) {  // check satisfiability
      if (line.find("UNSATISFIABLE") != string::npos) {
        solution = UNSAT;
        break;
      } else if (line.find("UNKNOWN") != string::npos) {
        solution = UNKNOWN;
        break;
      } else {
        solution = SAT;
      }
    } else if (startsWith(line, "v")) {  // check model
      parseClause(line.substr(2), literals);
      for (uint32_t i = 0; i < literals.size(); ++i) {
        // consider only variables of the original formula
        if ((int32_t)literals[i].toVar() > sd.var_cnt) {
          assignment.extend(sd.var_cnt, literals[i].toVar());
          sd.var_cnt = literals[i].toVar();
        }
        // set polarity for other literals
        assignment.set_polarity(literals[i].toVar(), literals[i].getPolarity());
      }
    } else if (startsWith(line, "o")) {
      oline = line;
    }

    // ignore all the other lines
  }

  double weight = 0;

  if (oline.size() > 0) {
    stringstream ss;
    ss.str(oline);
    ss.get();
    ss >> weight;
    //data_out << setprecision(15);
    data_out << "c coprocessor removed weight " << preprocessor.weightRemoved << endl;
    data_out << "o " << (weight_t)(weight + preprocessor.weightRemoved) << endl;
  }

  std::vector<Assignment> assignments;
  if (solution == SAT) {
    // set all polarities, that have not been assigned yet.
    for (Var v = 1; v <= sd.var_cnt; ++v) {
      if (assignment.is_undef(v))
        assignment.set_polarity(v, preprocessor.labelWeight[v] < 0 ? NEG : POS);
    }
    if (sd.equivalentTo.size() <= sd.var_cnt) {
      const uint32_t oldSize = sd.equivalentTo.size();
      sd.equivalentTo.resize(sd.var_cnt + 1);
      for (Var v = oldSize; v <= sd.var_cnt; ++v) {
        err_out << "c set EE[" << v << "] to " << Lit(v, POS).nr() << std::endl;
        sd.equivalentTo[v] = Lit(v, POS);
      }
    }

    err_out << "c postprocess" << std::endl;
    assignments.push_back(assignment);
    // use preprocessor in postprocessing-mode by giving also the number of
    // variables
    preprocessor.postprocess(
        err_out, &assignments,
        (int32_t)sd.var_cnt > var_cnt ? sd.var_cnt : var_cnt, true);
  }
  err_out << "c output model on stdout" << std::endl;
  printSolution(data_out, err_out, commandline, var_cnt, assignments);
  /*
    if (weight) {
      data_out << "o " << (long)(weight + preprocessor.weightRemoved) << endl;
    }
  */
  // calculate return value
  if (isInterupted()) {
    solution = UNKNOWN;
  } else if (solution == UNSAT || assignments.size() == 0) {
    solution = UNSAT;
  } else {
    solution = SAT;
  }

  return solution;
}

#endif  // #ifdef SATSOLVER

int32_t preprocess(std::ostream& data_out, std::istream& data_in,
                   std::ostream& map_out, std::ostream& err_out,
                   StringMap& commandline) {
  // usual solve routines
  solution_t solution = UNKNOWN;  // general result of the search
  // pointer to a vector of solutions
  std::vector<Assignment>* solutions = 0;  // result of problem

  // create formula vector
  std::vector<CL_REF>* clause_set;
  (clause_set) = new std::vector<CL_REF>();
  (clause_set)->reserve(32000);

  std::vector<CL_REF>* potentialGroups;
  (potentialGroups) = new std::vector<CL_REF>();
  (potentialGroups)->reserve(320);

  std::unordered_map<Var, weight_t>* whiteVars = new std::unordered_map<Var, weight_t>();
  weight_t top_weight = 0;
  weight_t parse_lb = 0;
  Allocator globalClauseStorage;  /// global storage object
  searchData search(globalClauseStorage);

  FileParser fp(commandline);

  solution = fp.parse_file(data_in, err_out, search, clause_set, whiteVars,
                           top_weight, potentialGroups, parse_lb);
  // cerr << "c solution: " << solution << std::endl;
  if (solution != UNSAT) {
    // Section of preprocessor
    Coprocessor2 preprocessor(clause_set, search, commandline, top_weight);
    // if help should be shown, stop program here!
    if (commandline.contains((const char*)"-h") ||
        commandline.contains((const char*)"--help") ||
        commandline.contains((const char*)"--verb-help")) {
      exit(0);
    }

    solution =
        preprocessor.preprocess(data_out, data_in, map_out, err_out, clause_set,
                                potentialGroups, whiteVars, parse_lb);
  }
  return solution;
}
