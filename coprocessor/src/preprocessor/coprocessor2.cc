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
        coprocessor2.cc
        This file is part of riss.

        05.12.2011
        Copyright 2011 Norbert Manthey
*/

#include "preprocessor/coprocessor2.h"
#include <sstream>
#include <iostream>
#include <map>

Coprocessor2::Coprocessor2(vector<CL_REF>* clause_set, searchData& sd,
                           const StringMap& commandline, long tW)
    : weightRemoved(0),
      postClauses(1024),
      timeoutTime(0),
      maintainTime(0),
      prPositive((uint32_t)0),
      prNegative((uint32_t)0),
      prTemporary((uint32_t)0),
      stampInfo(0),
      debug(0),
      print_dimacs(false),
      group_encode(true),
      only_group(false),
      sort(false),
      pline(false),
      enabled(true),
      fastSAT(true),
      do_compress(false),
      rewriteMode(0),
      do_rewrite1(false),
      do_rewritek(false),
      unlimited(false),
      variableLimit(2000000),
      clauseLimit(20000000),
      fastLoop(0),
      randomized(false),
      techniques(""),
      up(true),
      pure(true),
      pureSimp(false),
      subSimp(true),
      susiLimit(6000000),
      bve(true),
      bveGate(true),
      bveGateXor(false),
      bveGateIte(false),
      bveOTFSS(false),
      bveShrink(0),
      bveLimit(500000),
      ee(true),
      eeBehindBve(false),
      eeLimit(5000000),
      hte(true),
      hteLimit(1000000),
      hbce(false),
      bce(true),
      bceLimit(300000),
      hbceLimit(100000),
      probe(true),
      prDouble(4),
      prPure(false),
      prPureClause(true),
      prAutarky(false),
      prBinary(true),
      prLHBR(true),
      probeLimit(100000),
      probePureLimit(50),
      probeDoubleLimit(30000),
      probeBinaryLimit(10000),
      vivi(true),
      viviLenPercent(70),
      viviLimit(5000000),
      hyper(true),
      unhide(true),
#ifdef CP2RELEASE
      uhdTransitive(true),
#else
      uhdTransitive(false),
#endif
      unhideIter(2),
      advancedStamping(true),
      unhideLimit(2),
      doUHLE(3),
      doUHTE(true),
      uhdNoShuffle(false),
      uhdEE(true),
      // deleted stuff here
      search(sd),
      solution(UNKNOWN),
      numberOfSolutions(1),
      lastTrailSize(0),
      simpRequests(0),
      simpRequestLimit(9),
      simpRequestLimitInc(
          0),  // do not increase the limit, the restart schedule will do!
      simpRequestLimitIncInc(0),
      simplifying(false),
      simplifications(0),
      verbose(0),
      blockedLearnt(0) {

  varOcc = 0;
  litOcc = 0;
  occurrenceList.reserve(max_index(sd.var_cnt));

  // multiply with the number of techniques
  techniqueTime =
      (uint64_t*)malloc(sizeof(uint64_t) * (numberOfTechniques + 1));
  memset(techniqueTime, 0, sizeof(uint64_t) * (numberOfTechniques + 1));
  techniqueClaues =
      (int32_t*)malloc(sizeof(int32_t) * (numberOfTechniques + 1));
  memset(techniqueClaues, 0, sizeof(int32_t) * (numberOfTechniques + 1));
  techniqueEvents =
      (uint32_t*)malloc(sizeof(uint32_t) * (numberOfTechniques + 1));
  memset(techniqueEvents, 0, sizeof(uint32_t) * (numberOfTechniques + 1));
  techniqueCalls =
      (uint32_t*)malloc(sizeof(uint32_t) * (numberOfTechniques + 1));
  memset(techniqueCalls, 0, sizeof(uint32_t) * (numberOfTechniques + 1));
  techniqueCheck =
      (uint64_t*)malloc(sizeof(uint64_t) * (numberOfTechniques + 1));
  memset(techniqueCheck, 0, sizeof(uint64_t) * (numberOfTechniques + 1));
  techniqueLits = (int32_t*)malloc(sizeof(int32_t) * (numberOfTechniques + 1));
  memset(techniqueLits, 0, sizeof(int32_t) * (numberOfTechniques + 1));

  hlaQueue = 0;
  topW = tW;
  missedCores = 0;
  doNotTouch.init(search.capacity() +
                  2);  // todo: check whether boolarray has the right size
  addedCores = 0;
  mod_cores = false;

  // subsumptionQueue.reserve( clause_set->size() );

  // extendStructures( search.var_cnt, true );

  trackClause = 0;

#ifdef CP2RELEASE
  if (verbose > 0) cerr << "c [CP2] use Coprocessor 2.0" << endl;
#else
  if (verbose > 0) cerr << "c [CP2] use Coprocessor 2.1" << endl;
#endif
  set_parameter(commandline);
}

Coprocessor2::~Coprocessor2() {
  // free ignored clauses!
  for (int32_t j = postProcessStack.size() - 1; j >= 0; --j) {
    postEle& pe = postProcessStack[j];
    if (pe.kind == 'b') {
      Clause& cl = search.gsa.get(pe.b.clause);
      const uint32_t s = cl.getWords();
      cl.Clause::~Clause();
      search.gsa.release(pe.b.clause, s);
    } else if (pe.kind == 'v') {
      const elimination_t& elim = pe.e;
      for (uint32_t k = 0; k < elim.number; ++k) {
        uint32_t ind;
        Clause& cl = search.gsa.get(elim.clauses[k]);
        const uint32_t s = cl.getWords();
        cl.Clause::~Clause();
        search.gsa.release(elim.clauses[k], s);
      }
      free(elim.clauses);
    } else if (pe.kind == 'd') {
      pe.d.destroy();
    }
  }

  if (verbose > 1) showStatistics();

  if (techniqueTime != 0) free(techniqueTime);
  if (techniqueClaues != 0) free(techniqueClaues);
  if (techniqueEvents != 0) free(techniqueEvents);
  if (techniqueCalls != 0) free(techniqueCalls);
  if (techniqueCheck != 0) free(techniqueCheck);
  if (techniqueLits != 0) free(techniqueLits);

  // free all the structures that have been necessary for preprocessing

  if (varOcc != 0) free(varOcc);
  if (litOcc != 0) free(litOcc);
  if (hlaQueue != 0) free(hlaQueue);

  doNotTouch.destroy();
  bveTouchedVars.destroy();
  eqTouched.destroy();
  eqLitInStack.destroy();
  eqInSCC.destroy();
  eqReplaced.destroy();
  prArray.destroy();
  prPositive.destroy();
  prNegative.destroy();
  prTemporary.destroy();

  occurrenceList.clear();
  bveTouchedVars.destroy();
  variableHeap.clear();
  bveHeap.clear();
  // deleted something here
  hteHeap.clear();
  bceHeap.clear();
  literalHeap.clear();

  // deleted something here

  prReason.clear();

  eqNodeLowLinks.clear();
  eqNodeIndex.clear();
}

void Coprocessor2::freeResources() {
  free(varOcc);
  varOcc = 0;
  free(litOcc);
  litOcc = 0;

  hlaPositive.destroy();
  hlaNegative.destroy();
  free(hlaQueue);
  hlaQueue = 0;
  free(stampInfo);
  stampInfo = 0;

  bveTouchedVars.destroy();
  eqTouched.destroy();
  eqReplaced.destroy();
  eqLitInStack.destroy();
  eqInSCC.destroy();
  prArray.destroy();
  prPositive.destroy();
  prNegative.destroy();
  prTemporary.destroy();

  // clear vectors and free memory!
  occurrenceList.clear();
  occurrenceList.swap(occurrenceList);
  bveTouchedVars.destroy();
  variableHeap.free();
  literalHeap.free();

  bveHeap.free();
  // deleted something here
  hteHeap.free();
  bceHeap.free();

  // deleted something here

  prReason.clear();
  prReason.swap(prReason);

  eqNodeLowLinks.clear();
  eqNodeLowLinks.swap(eqNodeLowLinks);
  eqNodeIndex.clear();
  eqNodeIndex.swap(eqNodeIndex);

  markArray.destroy();
}

void Coprocessor2::extendStructures(uint32_t newVar, bool initialize) {
  uint32_t oldVarCnt = search.var_cnt;
  uint32_t oldCapacity = search.capacity();

  // if structures are increased, than use capacity, s.t. not always all
  // structures have to be increased
  search.extend(newVar);
  newVar = search.capacity();

  if (varOcc == 0) {
    varOcc = (uint32_t*)malloc(sizeof(uint32_t) * (newVar + 1));
    memset(varOcc, 0, sizeof(uint32_t) * (newVar + 1));
  } else if (newVar > oldCapacity) {
    varOcc = (uint32_t*)realloc(varOcc, sizeof(uint32_t) * (newVar + 1));
    uint32_t diff = newVar - oldVarCnt;
    memset(&(varOcc[oldCapacity + 1]), 0, sizeof(uint32_t) * diff);
  }
  if (litOcc == 0) {
    litOcc = (uint32_t*)malloc(sizeof(uint32_t) * max_index(newVar));
    memset(litOcc, 0, sizeof(uint32_t) * max_index(newVar));
  } else if (newVar > oldCapacity) {
    litOcc = (uint32_t*)realloc(litOcc, sizeof(uint32_t) * max_index(newVar));
    uint32_t diff = max_index(newVar) - max_index(oldCapacity);
    memset(&(litOcc[max_index(oldCapacity)]), 0, sizeof(uint32_t) * diff);
  }

  // if( initialize ) hlaPositive.create( max_index(newVar) );
  // else
  hlaPositive.resize(max_index(newVar));
  // if( initialize ) hlaNegative.create( max_index(newVar) );
  // else
  hlaNegative.resize(max_index(newVar));

  if (hlaQueue == 0) {
    hlaQueue = (Lit*)malloc(sizeof(Lit) * max_index(newVar));
    memset(hlaQueue, 0, sizeof(Lit) * max_index(newVar));
  } else {
    hlaQueue = (Lit*)realloc(hlaQueue, sizeof(Lit) * max_index(newVar));
    uint32_t diff = max_index(newVar) - max_index(oldCapacity);
    memset(&(hlaQueue[max_index(oldCapacity)]), 0, sizeof(Lit) * diff);
  }

  if (stampInfo == 0) {
    stampInfo = (literalData*)malloc(sizeof(literalData) * max_index(newVar));
    memset(stampInfo, 0, sizeof(uint32_t) * max_index(newVar));
  } else {
    stampInfo = (literalData*)realloc(stampInfo,
                                      sizeof(literalData) * max_index(newVar));
    uint32_t diff = max_index(newVar) - max_index(oldCapacity);
    memset(&(stampInfo[max_index(oldCapacity)]), 0, sizeof(literalData) * diff);
  }

  doNotTouch.extend(oldCapacity + 1, newVar + 1);
  bveTouchedVars.extend(oldCapacity + 1, newVar + 1);
  eqTouched.extend(max_index(oldCapacity + 1), max_index(newVar + 1));
  eqLitInStack.extend(max_index(oldCapacity + 1), max_index(newVar + 1));
  eqInSCC.extend(oldCapacity + 1, newVar + 1);
  eqReplaced.extend(oldCapacity + 1, newVar + 1);
  prArray.extend(max_index(oldCapacity + 1), max_index(newVar + 1));

  prPositive.extend(oldCapacity + 1, newVar + 1);
  prNegative.extend(oldCapacity + 1, newVar + 1);
  prTemporary.extend(oldCapacity + 1, newVar + 1);

  occurrenceList.resize(max_index(newVar));
  bveTouchedVars.extend(oldCapacity, newVar + 1);
  variableHeap.resize(newVar + 1, &varOcc);
  literalHeap.resize(max_index(newVar), &litOcc);

  if (bve) bveHeap.resize(newVar + 1, &varOcc);
  // deleted something here
  if (hte) hteHeap.resize(newVar + 1, &varOcc);
  if (bce) bceHeap.resize(newVar + 1, &varOcc);

  // deleted something here

  prReason.resize(newVar + 1);

  eqNodeLowLinks.resize(max_index(newVar), -1);
  eqNodeIndex.resize(max_index(newVar), -1);

  markArray.resize(max_index(newVar));

  //    }
}

solution_t Coprocessor2::preprocess(
    std::ostream& data_out, std::istream& data_in, std::ostream& map_out,
    std::ostream& err_out, vector<CL_REF>* formulaTopreprocess,
    vector<CL_REF>* potentialGroups, std::unordered_map<Var, long>* whiteVars,
    long init_lb) {
  weightRemoved += init_lb;

  if (!enabled) return UNKNOWN;
  formula = formulaTopreprocess;

  // reject formulas that are too large!
  if (!unlimited) {
    if (search.var_cnt > variableLimit ||
        formulaTopreprocess->size() > clauseLimit) {
      printEmptyMapfile(map_out);
      if (sort) sortClauseList(*formula);
      if (print_dimacs || pline) printFormula(data_out, err_out);
      return UNKNOWN;
    }
  }

  // start timer
  maintainTime = get_microseconds() - maintainTime;

  labelWeight = *whiteVars;

  // store the original parameter settings
  bool parameterSettings[4];
  parameterSettings[1] = hte;
  parameterSettings[2] = bce;
  parameterSettings[3] = bve;

  // hte, bce and bve are expensive wrt. clauses
  if (!unlimited) {
    if (search.var_cnt > 1200000 || formula->size() > 2000000) hte = false;
    if (search.var_cnt > 1400000 || formula->size() > 2400000) bce = false;
    if (search.var_cnt > 1600000 || formula->size() > 2600000) bve = false;
  }

  // initialize (called for the first time)
  extendStructures(search.var_cnt, simplifications < 2 || !simplifying);
  clearStructures();

  for (Var v = 1; v <= search.var_cnt; ++v) {
    if (search.eliminated.get(v)) continue;
    variableHeap.insert_item(v);
    // add to heaps only, if the corresponding technique is activated
    if (bve) bveHeap.insert_item(v);
    if (hte) hteHeap.insert_item(v);
    if (bce) bceHeap.insert_item(v);
    literalHeap.insert_item(Lit(v, POS).data());
    literalHeap.insert_item(Lit(v, NEG).data());
  }

  if (!addFormula(*formula)) solution = UNSAT;

  uint32_t new_vars = 0;
  uint32_t cur_vars = search.var_cnt;
  uint32_t add_to_vars = 0;
  uint32_t positive_labels = 0;
  uint32_t negative_labels = 0;

  for (auto pg : *potentialGroups) {
    const Clause& cl = search.gsa.get(pg);
    Lit l = cl.get_literal(0);
    if (list(l).size() > 0 || !group_encode) new_vars++;
  }

  extend(search.var_cnt + new_vars);

  for (auto pg : *potentialGroups) {
    Clause& cl = search.gsa.get(pg);
    Lit l = cl.get_literal(0);

    // NOT IN ANY OF THE SOFT CLAUSES
    if (list(l).size() == 0 && group_encode) {
      // IS GROUP
      if (l.nr() > 0)
        positive_labels++;
      else
        negative_labels++;
    } else {
      add_to_vars++;
      uint32_t labelN = cur_vars + add_to_vars;

      Lit label = Lit(labelN);
      bool added = addNexClause(l, label);

      if (!added) return UNSAT;
      long labelW = -abs(labelWeight[l.toVar()]);

      labelWeight[label.nr()] = labelW;
      labelWeight.erase(abs(l.nr()));
    }
  }

  for (auto& t_w : labelWeight) {
    Var t = t_w.first;
    if (t <= search.var_cnt) doNotTouch.set(t, true);
  }

  // end timer
  maintainTime = get_microseconds() - maintainTime;

  // rewriting?
  if (rewriteMode == 1 || rewriteMode == 3) {
    if (do_rewrite1) rewriteAMO();
    if (do_rewritek) rewriteAMK();
  }

  if (only_group) {
    err_out << "c [CP2] Only recognizing groups, skipping preprocessloop"
            << endl;
  } else {
    if (techniques.size() == 0) {
      bool change = true;  // indicate whether a method did something
      // preprocess
      uint32_t round = 0;
      bool didSubsumption = false, didBve = false, didEE = false,
           didHte = false, didBce = false, didProbe = false, didHyper = false,
           didVivi = false, didUnhide = false;
      // run iteration as long as there is change or there is more to do
      while (solution == UNKNOWN && !isInterupted()) {
        round++;
        change = false;
        if (up && solution == UNKNOWN) {
          change = propagateTopLevel() || change;
          if (verbose > 0 && solution != UNKNOWN)
            err_out << "c [CP2] after up found solution " << solution << endl;
        }
        change = false;  // if( change ) continue does not make sense after the
                         // first method!

        if (pure && solution == UNKNOWN) {
          change = eliminatePure() || change;
          if (verbose > 0 && solution != UNKNOWN)
            err_out << "c [CP2] after pure found solution " << solution << endl;
        }
        if (change) continue;  // after pure, redo unit propagation!

        if (subSimp && solution == UNKNOWN) {
          change = selfSubsumption(!didSubsumption) || change;
          if (verbose > 0 && solution != UNKNOWN)
            err_out << "c [CP2] after susi found solution " << solution << endl;
          didSubsumption = true;
        }
        // execute here, because subsumtion queue is empty
        // garbageCollect();
        if (change) continue;

        if (!eeBehindBve) {
          if (ee && solution == UNKNOWN) {
            change = equivalenceElimination(!didEE) || change;
            if (verbose > 0 && solution != UNKNOWN)
              err_out << "c [CP2] after ee found solution " << solution << endl;
            didEE = true;
          }
          if (change) continue;  // after pure, redo unit propagation!

          // deleted something here
        }

        if (unhide && !didUnhide && solution == UNKNOWN) {
          change = unhiding(!didUnhide) || change;
          if (verbose > 0 && solution != UNKNOWN)
            err_out << "c [CP2] after unhide found solution " << solution
                    << endl;
          didUnhide = true;
        }
        if (change) continue;

        // finish up,pure and susi before timing out!
        if (timedOut()) break;

        if (hte && solution == UNKNOWN) {
          change = hiddenTautologyElimination(!didHte) || change;
          if (verbose > 0 && solution != UNKNOWN)
            err_out << "c [CP2] after hte found solution " << solution << endl;
          didHte = true;
        }
        if (change) continue;  // after pure, redo unit propagation!

        if (bce && solution == UNKNOWN) {
          change = blockedClauseElimination(!didBce) || change;
          if (verbose > 0 && solution != UNKNOWN)
            err_out << "c [CP2] after bce found solution " << solution << endl;
          didBce = true;
        }

        if (bve && solution == UNKNOWN) {
          change = variableElimination(!didBve) || change;
          if (verbose > 0 && solution != UNKNOWN)
            err_out << "c [CP2] after bve found solution " << solution << endl;
          didBve = true;
        }

        if (eeBehindBve) {
          if (ee && solution == UNKNOWN) {
            change = equivalenceElimination(!didEE) || change;
            if (verbose > 0 && solution != UNKNOWN)
              err_out << "c [CP2] after ee found solution " << solution << endl;
            didEE = true;
          }
          if (change) continue;  // after pure, redo unit propagation!
        }

        if (unhide && didUnhide && solution == UNKNOWN) {
          change = unhiding(!didUnhide) || change;
          if (verbose > 0 && solution != UNKNOWN)
            err_out << "c [CP2] after unhide found solution " << solution
                    << endl;
        }
        if (change) continue;

        if (probe && solution == UNKNOWN) {
          change = probing(!didProbe) || change;
          if (verbose > 0 && solution != UNKNOWN)
            err_out << "c [CP2] after probe found solution " << solution
                    << endl;
          didProbe = true;
        }
        if (change) continue;  // after pure, redo unit propagation!

        if (vivi && solution == UNKNOWN) {
          change = randomizedVivification(!didVivi) || change;
          if (verbose > 0 && solution != UNKNOWN)
            err_out << "c [CP2] after vivi found solution " << solution << endl;
          didVivi = true;
        }
        if (change) continue;  // after pure, redo unit propagation!

        break;
      }

    } else {
      solution = preprocessScheduled();
    }
  }

  // rewriting?
  if (rewriteMode > 1) {
    if (do_rewrite1) rewriteAMO();
#ifndef CP2RELEASE
    if (do_rewritek) rewriteAMK();
#endif
  }

  if (blackFile.size() != 0) {
    VarFileParser vfp(blackFile);
    vector<Var> evars;
    evars = vector<Var>();
    vfp.extract(evars);
    for (uint32_t i = 0; i < evars.size(); ++i) {
      if (evars[i] <= search.var_cnt) {
        resolveVar(evars[i], 1);
        // doNotTouch.set( evars[i], true);
      }
      // reset assignments that have been found for black listed variables
      if (!search.assignment.is_undef(evars[i])) {
        search.assignment.undef_variable(evars[i]);
      }
    }
  }

  // start timer
  maintainTime = get_microseconds() - maintainTime;

  // finalize

  if (solution == UNKNOWN) {
    // compress the formula before doing output or finishing preprocessing
    if (do_compress && !simplifying) compress();
    reduceFormula();
    // sort the formula
    if (sort) sortClauseList(*formula);
  }

  // stop timer
  maintainTime = get_microseconds() - maintainTime;

  // print of output stat only, if preprocessing and not simplifying
  if (!simplifying) {
    // show a summary before printing or solving
    if (verbose > 0) showStatistics();
    if (verbose > 1)
      err_out << "c [CP2] preprocessed literals: " << countLiterals() << endl;

    if (print_dimacs || pline) {
      if (formula->size() == 0) {
        solution = SAT;
      }
      printFormula(data_out, err_out);
    }

    // output preprocess information
    outputPostprocessInfo(map_out, err_out);

    // reset maintain time, so that the cummulation can be traced later!
    maintainTime = 0;
    freeResources();
  }
  lastTrailSize = search.trail.size();

  // reset techniques
  if (!simplifying) {
    memset(techniqueTime, 0, sizeof(uint64_t) * (numberOfTechniques + 1));
    memset(techniqueClaues, 0, sizeof(int32_t) * (numberOfTechniques + 1));
    memset(techniqueEvents, 0, sizeof(uint32_t) * (numberOfTechniques + 1));
    memset(techniqueCalls, 0, sizeof(uint32_t) * (numberOfTechniques + 1));
    memset(techniqueCheck, 0, sizeof(uint64_t) * (numberOfTechniques + 1));
    memset(techniqueLits, 0, sizeof(int32_t) * (numberOfTechniques + 1));
  }

  // deleted something here
  hte = parameterSettings[1];
  bce = parameterSettings[2];
  bve = parameterSettings[3];

  // if there are no more clauses that need to be processed, the formula is
  // satisfiable
  if (fastSAT && formula->size() == 0) {
    solution = SAT;
    for (Var v = 1; v <= search.var_cnt; ++v) {
      if (!search.eliminated.get(v) && search.equivalentTo[v] == Lit(v, POS) &&
          search.assignment.is_undef(v))
        search.assignment.set_polarity(v, NEG);
    }
  }

  return solution;
}

solution_t Coprocessor2::preprocessScheduled() {
  vector<string> grammar;
  grammar.push_back(string());

  char c = 0;
  uint32_t pos = 0;
  uint32_t line = 0;
  uint32_t level = 0;
  while (pos < techniques.size()) {
    c = techniques[pos++];
    if (c == '[') {
      level++;
      if (level > 1) {
        exit(-2);
      } else {
        if (grammar[line].size() > 0) {
          grammar.push_back(string());
          line++;
        }
      }
      continue;
    }
    if (c == ']') {
      if (level < 1) {
        exit(-2);
      }
      if (level == 1) {
        // star behing brackets?
        if (pos < techniques.size() && techniques[pos] == '+' &&
            grammar[line].size() > 0) {
          grammar[line] += "+";
          pos++;
        }
        if (grammar[line].size() > 0) {
          grammar.push_back(string());
          line++;
        }
      }
      level--;
      continue;
    }
    if (c == '+') {
      if (level == 0) {
        continue;
      }
    }
    grammar[line] += c;
  }

  if (grammar.size() == 0) return solution;

  uint32_t currentLine = 0;
  uint32_t currentPosition = 0;
  uint32_t currentSize = grammar[currentLine].size();
  bool change = false;
  bool didSubsumption = false, didBve = false, didEE = false, didHte = false,
       didBce = false, didProbe = false, didHyper = false, didVivi = false,
       didUnhide = false;

  char techniqueChars[] = {' ', 'u', 'p', 's', 'v', 'w', 'e', 'h',
                           'b', 'r', 'y', 'a', 'o', 'g', 'x'};

  while (solution == UNKNOWN &&
         (currentLine < grammar.size() || currentPosition < currentSize)) {
    char execute = grammar[currentLine][currentPosition];
    if (execute ==
        '+') {  // if there is a star in a line and there has been change,
      if (change) {
        currentPosition = 0;
        continue;  // start current line in grammar again!
      } else {
        currentPosition++;
      }
    }

    if (currentPosition >= currentSize) {  // start with next line, if current
                                           // line has been evaluated
      currentLine++;
      if (currentLine < grammar.size()) {
        currentSize = grammar[currentLine].size();
        currentPosition = 0;
        continue;
      }
    }

    if (currentLine >= grammar.size())
      break;  // stop if all lines of the grammar have been evaluated

    if (currentPosition ==
        0) {  // if the line is started from scratch, reset the change flag
      change = false;
    }

    // in the next iteration, the position is increased
    currentPosition++;

    subsumptionQueue.clear();

    if (execute == techniqueChars[do_up] && up && solution == UNKNOWN) {
      change = propagateTopLevel() || change;
    }

    if (execute == techniqueChars[do_pure] && pure && solution == UNKNOWN) {
      change = eliminatePure() || change;
    }

    if (execute == techniqueChars[do_subSimp] && subSimp &&
        solution == UNKNOWN) {
      change = selfSubsumption(true) ||
               change;  // list will be cleared at each iteration!
      didSubsumption = true;
    }

    if (execute == techniqueChars[do_unhide] && unhide && solution == UNKNOWN) {
      change = unhiding(!didUnhide) || change;
      didUnhide = true;
    }

    if (execute == techniqueChars[do_ee] && ee && solution == UNKNOWN) {
      change = equivalenceElimination(!didEE) || change;
      didEE = true;
    }

    if (execute == techniqueChars[do_hte] && hte && solution == UNKNOWN) {
      change = hiddenTautologyElimination(!didHte) || change;
      didHte = true;
    }

    if (execute == techniqueChars[do_bce] && bce && solution == UNKNOWN) {
      change = blockedClauseElimination(!didBce) || change;
      didBce = true;
    }

    if (execute == techniqueChars[do_bve] && bve && solution == UNKNOWN) {
      change = variableElimination(!didBve) || change;
      didBve = true;
    }
    // deleted something here
    if (execute == techniqueChars[do_probe] && probe && solution == UNKNOWN) {
      change = probing(!didProbe) || change;
      didProbe = true;
    }

    if (execute == techniqueChars[do_vivi] && vivi && solution == UNKNOWN) {
      change = randomizedVivification(!didVivi) || change;
      didVivi = true;
    }
  }

  return solution;
}

bool Coprocessor2::enqueTopLevelLiteral(const Lit& l, bool evenIfSAT) {
  if (search.assignment.is_unsat(l)) {
    return false;
  }
  // enqueue the literal that is the representative for the equivalence class
  if (search.equivalentTo[l.toVar()] != Lit(l.toVar(), POS)) {
    if (!enqueTopLevelLiteral(l.isPositive()
                                  ? search.equivalentTo[l.toVar()]
                                  : search.equivalentTo[l.toVar()].complement(),
                              true))
      return false;
  }

  if (!evenIfSAT && search.assignment.is_sat(l)) return true;
  propagationQueue.push_back(l);
  search.assignment.set_polarity(l.toVar(), l.getPolarity());

  if (!evenIfSAT) {
    search.trail.push_back(l);
    search.VAR_REASON(l.toVar()) = reasonStruct();
    search.VAR_LEVEL(l.toVar()) = 0;
  }

  return true;
}

bool Coprocessor2::propagateTopLevel() {
  techniqueStart(do_up);

  bool didSomething = false;
  uint32_t propagatedLiterals = 0;
  while (!propagationQueue.empty()) {

    didSomething = true;
    const Lit l = propagationQueue.front();
    propagationQueue.pop_front();
    search.big.removeLiteral(l);
    techniqueSuccessEvent(do_up);
    int32_t extraCount = 0;
    while (list(l).size() > 0) {
      Clause& clause = search.gsa.get(list(l)[0]);
      techniqueDoCheck(do_up);  // get some statistics
      if (!clause.isIgnored()) {
        // assert( clause.size() != 1 && "units in the list have to be already
        // ignored");
        updateRemoveClause(clause);
        clause.markToDelete();
        techniqueChangedLiteralNumber(do_up, 0 - (int32_t)clause.size());
        extraCount--;
        if (clause.size() == 2)
          search.big.removeClauseEdges(clause.get_literal(0),
                                       clause.get_literal(1));
      } else {
      }
      removeClause(list(l)[0]);
    }

    const Lit notL = l.complement();
    vector<CL_REF>& notList = list(notL);
    for (uint32_t i = 0; i < notList.size(); ++i) {
      Clause& clause = search.gsa.get(notList[i]);
      techniqueDoCheck(do_up);  // get some statistics
      if (clause.isIgnored()) {
        continue;
      }
      if (clause.size() == 2) {
        search.big.removeClauseEdges(clause.get_literal(0),
                                     clause.get_literal(1));
      }
      clause.removeLin(notL);
      techniqueChangedLiteralNumber(do_up, -1);
      // sort according to frequency!
      if (clause.size() > 1) {
        updateChangeClause(notList[i]);
      } else if (clause.size() == 1) {
        clause.markToDelete();
        extraCount--;
        if (!enqueTopLevelLiteral(clause.get_literal(0))) {
          solution = UNSAT;
          break;
        }
        updateRemoveClause(clause);
      } else {
        solution = UNSAT;
        break;
      }
    }
    notList.clear();
    updateDeleteLiteral(l);
    updateDeleteLiteral(notL);
    techniqueChangedClauseNumber(do_up, extraCount);
  }
  techniqueStop(do_up);
  return didSomething;
}

bool Coprocessor2::eliminatePure(bool expensive) {
  techniqueStart(do_pure);
  bool didSomething = false;
  uint32_t count = 0;
  for (uint32_t i = 0; i < literalHeap.size(); ++i) {
    techniqueDoCheck(do_pure);  // get some statistics
    Lit l;
    l.fromData(literalHeap.item_at(i));
    if (!search.assignment.is_undef(l.toVar())) continue;
    // do not consider eliminated variables as pure!
    if (search.eliminated.get(l.toVar())) continue;
    // do not set literals as pure, if they are equivalent to some other literal
    if (eqReplaced.get(l.toVar())) continue;
    if (search.equivalentTo[l.toVar()] != Lit(l.toVar(), POS)) continue;
    // do not alter pure literals, that should not be altered
    if (doNotTouch.get(l.toVar())) {
      continue;
    }
    // abort, if a higher count is reached
    // abort, if a higher count is reached
    if (litOcc[l.toIndex()] > 0) {
      if (!expensive)
        break;
      else
        continue;
    }

#ifndef NDEBUG
    for (uint32_t j = 0; j < list(l).size(); ++j)
      assert(search.gsa.get(list(l)[j]).isIgnored() &&
             "there cannot be clauses in lists of pure literals");
#endif

    bool isPure = true;
    for (uint32_t j = 0; j < list(l).size(); ++j) {
      const Clause& c = search.gsa.get(list(l)[j]);
      if (c.isIgnored())
        continue;
      else {
        isPure = false;
        break;
      }
    }

    if (isPure) {
      // l is pure!
      const Lit complement = l.complement();
      count++;
      // since l does not occur, complement has to be pure -> enqueue as top
      // level unit
      techniqueSuccessEvent(do_pure);
      if (!enqueTopLevelLiteral(complement)) {
        solution = UNSAT;
        break;
      }
      didSomething = true;
    } else {
    }
  }
  techniqueStop(do_pure);

  if (didSomething) propagateTopLevel();
  return didSomething;
}

bool Coprocessor2::selfSubsumption(bool firstCall) {
  bool didSomething = false;
  techniqueStart(do_subSimp);
  // only in the first round change process the whole formula
  if (firstCall) {
    subsumptionQueue.clear();
    for (uint32_t i = 0; i < formula->size(); ++i)
      subsumptionQueue.push_back((*formula)[i]);
  }

  if (!unlimited && susiLimit == 0) {
    subsumptionQueue.clear();
  }

  // work till the queue is empty, early stop in case of timeout
  while (subsumptionQueue.size() > 0 && (unlimited || susiLimit > 0) &&
         !timedOut()) {
    // get the first clause from the queue and increment the ind
    CL_REF clause = subsumptionQueue[subsumptionQueue.size() - 1];
    Clause& cl = search.gsa.get(clause);
    subsumptionQueue.pop_back();

    // Take the next Clause, if this clause is ignored
    if (cl.isIgnored()) continue;
    if (simplifying && !cl.isLearnt()) continue;

    techniqueDoCheck(do_subSimp);  // get some statistics
    susiLimit = (susiLimit > 0) ? susiLimit - 1 : 0;

    uint64_t hash = 0;
    // Search the literal with the least occurence for subsumption and remind it
    Var var1 = cl.get_literal(0).toVar();
    hash = hash | (1 << (var1 & 63));
    for (uint32_t j = 1; j < cl.size(); ++j) {
      const Var cV = cl.get_literal(j).toVar();
      hash = hash | (1 << (cV & 63));
      if (varOcc[cV] < varOcc[var1]) {
        var1 = cV;
      }
    }
    markArray.nextStep();
    for (uint32_t i = 0; i < cl.size(); i++) {
      markArray.setCurrentStep(cl.get_literal(i).toIndex());
    }

    // Find subsumed clauses and clauses that can be simplified
    for (uint32_t pn = 0; pn < 2; ++pn) {
      vector<CL_REF>& clauseList =
          (pn == 0) ? list(Lit(var1, POS)) : list(Lit(var1, NEG));
      for (uint32_t i = 0; i < clauseList.size(); ++i) {
        CL_REF clause_oc = clauseList[i];
        Clause& cl_oc = search.gsa.get(clause_oc);
        // Check the clause wether it can't be subsumed or wether it is the
        // same clause as the input-clause
        Lit lit1_oc = NO_LIT;
        if (cl_oc.isIgnored() || clause_oc == clause) continue;

        // old quadratic version
        bool subOrSimp = true;
        if (false) {
          subOrSimp = cl.subsumes_or_simplifies_clause(cl_oc, &lit1_oc);
        } else {
          const uint32_t other_clauseSize = cl_oc.size();
          const uint32_t clauseSize = cl.size();
          if (clauseSize > other_clauseSize) {
            subOrSimp = false;
          } else {
            uint32_t missCount = 0;
            lit1_oc = NO_LIT;
            uint32_t hit = 0;
            for (uint32_t j = 0; j < other_clauseSize; j++) {
              const Lit otherJ = cl_oc.get_literal(j);
              missCount = ((hash & (1 << (otherJ.toVar() & 63))) != 0)
                              ? missCount
                              : missCount + 1;
              if (missCount > 1) {
                subOrSimp = false;
                break;
              }
              // nothing happens, if literal appears in both clauses
              if (markArray.isCurrentStep(otherJ.toIndex())) {
                hit++;
                continue;
              }
              // if a complementary literal appears, store it (for resolution)
              if ((lit1_oc == NO_LIT) &&
                  markArray.isCurrentStep(otherJ.complement().toIndex())) {
                lit1_oc = otherJ;
                hit++;
                continue;
              }
              if (lit1_oc != NO_LIT &&
                  markArray.isCurrentStep(otherJ.complement().toIndex())) {
                subOrSimp = false;
                break;
              }
            }
            // all variables of cl have to be in cl_oc,
            if (hit < cl.size()) subOrSimp = false;

            assert(hit <= cl.size() &&
                   "number of hitting variables can only be less equal than "
                   "the number of literals in the clause");

            // if( !subOrSimp ) cerr << "c rejected subSimp with missCount= " <<
            // missCount << endl;
          }
        }
        if (!subOrSimp) {
          continue;
        }

        didSomething = true;
        techniqueSuccessEvent(do_subSimp);
        // If the return-literal of the subsumes_or_simplifies_clause-Method is
        // NO_LIT we have found a clause which is subsumed from the input-clause
        // thus we can ignore it from now and delete it later with the
        // reduceClauses-Function
        // If the return-literal isn't NO_LIT than we can simplify the clause or
        // return the
        // solution
        // Subsumption was successful
        if (lit1_oc == NO_LIT) {
          // if subsumed and removed clause is not learnt, the one that removes
          // it must be kept -> original
          if (!cl_oc.isLearnt()) cl.setLearnt(false);

          techniqueChangedLiteralNumber(do_subSimp, 0 - (int32_t)cl_oc.size());
          cl_oc.markToDelete();
          // remove edge, if subsuming clause is not the same size, but smaller
          if (cl_oc.size() == 2 && cl.size() < 2) {
            search.big.removeClauseEdges(cl_oc.get_literal(0),
                                         cl_oc.get_literal(1));
          }

          techniqueChangedClauseNumber(do_subSimp, -1);
          removeClause(clause_oc);
          updateRemoveClause(cl_oc);
          // if the clause was deleted look again to same index in list
          i--;
        } else if (true) {
          // remove literal from clause, update structures!
          if (cl_oc.size() == 2) {
            search.big.removeClauseEdges(cl_oc.get_literal(0),
                                         cl_oc.get_literal(1));
          }
          cl_oc.removeLin(lit1_oc);
          techniqueChangedLiteralNumber(do_subSimp, -1);
          updateRemoveLiteral(lit1_oc);
          vector<CL_REF>& removeList = list(lit1_oc);
          const uint32_t ls = removeList.size();
          for (uint32_t j = 0; j < ls; ++j) {
            if (clause_oc == removeList[j]) {
              removeList[j] = removeList[removeList.size() - 1];
              removeList.pop_back();
              break;
            }
          }

          // empty or unit?
          if (cl_oc.size() == 0)
            solution = UNSAT;
          else if (cl_oc.size() == 1) {
            cl_oc.markToDelete();
            techniqueChangedClauseNumber(do_subSimp, -1);
            if (!enqueTopLevelLiteral(cl_oc.get_literal(0))) {
              solution = UNSAT;
            }
            if (fastLoop > 0) goto endSuSi;
          }
          // insert clause into queue again
          updateChangeClause(clause_oc);
        }
      }
    }
  }
endSuSi:
  ;
  subsumptionQueue.clear();
  techniqueStart(do_subSimp);
  return didSomething;
}

void Coprocessor2::outputPostprocessInfo(std::ostream& map_out,
                                         std::ostream& err_out) {
  map_out << "polarities" << endl;
  map_out << labelWeight.size();
  for (auto v_w : labelWeight) {
    Var v = v_w.first;
    long w = v_w.second;
    map_out << " " << v << " " << w;
  }
  map_out << endl;

  map_out << "removed weight" << endl;
  map_out << weightRemoved << endl;

  map_out << "original variables" << endl;
  map_out << search.original_var_cnt << " - " << search.var_cnt << endl;
  // write compress info once
  if (do_compress) {
    map_out << "compress tables" << endl;
    // variables start with 1!
    uint32_t i = 0;
    for (; i < postProcessStack.size(); ++i) {
      if (postProcessStack[i].kind == 'd') break;
    }
    if (i < postProcessStack.size()) {
      Compression& compression = postProcessStack[i].d;
      // TODO: use a variable here?
      map_out << "table " << 0 << " " << compression.variables << endl;
      // print the remaining table!
      for (uint32_t i = 1; i <= compression.variables; ++i) {
        if (compression.mapping[i] != 0) {
          map_out << compression.mapping[i] << " ";
        }
      }
      map_out << "0" << endl;
      // print units from the previous assignment
      map_out << "units " << 0 << endl;  // TODO: use a variable here?
      for (Var v = 1; v <= compression.variables; ++v) {
        if (!compression.previous.is_undef(v))
          map_out << Lit(v, compression.previous.get_polarity(v)).nr() << " ";
      }
      /// NOTE: be carefully here, for solving, this does not work!
      search.equivalentTo = compression.equivalentTo;
      map_out << "0" << endl;
      map_out << "end table" << endl;
    } else {
      map_out << "no table" << endl;
    }
  }
  // write eq info once
  map_out << "ee table" << endl;
  if (ee) {
    if (!do_compress) {
      // in case of no compression, use the shared
      // equivalence vector
      err_out << "c write ee table" << endl;
      vector<vector<Lit> > groups(search.var_cnt + 1);
      for (uint32_t i = 1; i <= search.var_cnt; ++i) {
        if (search.equivalentTo[i] != Lit(i, POS)) {
          Lit li(i, POS);
          groups[search.equivalentTo[i].toVar()].push_back(
              search.equivalentTo[i].getPolarity() == POS ? li
                                                          : li.complement());
        }
      }

      for (uint32_t i = 0; i < groups.size(); ++i) {
        if (groups[i].size() == 0) continue;
        map_out << i << " ";
        for (uint32_t j = 0; j < groups[i].size(); ++j)
          map_out << groups[i][j].nr() << " ";
        map_out << "0" << endl;
      }
    } else {
      // otherwise use the vector of the compression element
      // search for the compression object
      uint32_t i = 0;
      for (; i < postProcessStack.size(); ++i)
        if (postProcessStack[i].kind == 'd') break;
      if (i < postProcessStack.size()) {
        Compression& compression = postProcessStack[i].d;
        if (verbose > 1) err_out << "c write compression ee table" << endl;
        vector<vector<Lit> > groups(compression.variables + 1);
        for (uint32_t i = 1; i <= compression.variables; ++i) {
          if (compression.equivalentTo[i] != Lit(i, POS)) {
            Lit li(i, POS);
            groups[compression.equivalentTo[i].toVar()]
                .push_back(compression.equivalentTo[i].getPolarity() == POS
                               ? li
                               : li.complement());
          }
        }

        for (uint32_t i = 0; i < groups.size(); ++i) {
          if (groups[i].size() == 0) continue;
          map_out << i << " ";
          for (uint32_t j = 0; j < groups[i].size(); ++j)
            map_out << groups[i][j].nr() << " ";
          map_out << "0" << endl;
        }
      }
    }
  }
  err_out << "c [CP2] dump postprocessstack into file" << endl;
  map_out << "postprocess stack" << endl;
  // process postprocess stack
  // process stack from the end to the front
  for (int32_t j = 0; j < (int32_t)postProcessStack.size(); ++j) {
    const postEle pe = postProcessStack[j];
    if (pe.kind == 'b') {
      map_out << "bce " << pe.b.lit1.nr() << endl;

      // check whether clause is sat, if not, set blocked lit1 to sat!
      Clause& cl = search.gsa.get(pe.b.clause);
      for (uint32_t k = 0; k < cl.size(); ++k)
        map_out << cl.get_literal(k).nr() << " ";
      map_out << "0" << endl;
    } else {
      if (pe.kind == 'v') {
        const elimination_t& elim = pe.e;

        map_out << "ve " << elim.var1 << " " << elim.number << endl;

        for (uint32_t k = 0; k < elim.number; ++k) {
          Clause& cl = search.gsa.get(elim.clauses[k]);
          for (uint32_t ind = 0; ind < cl.size(); ++ind)
            map_out << cl.get_literal(ind).nr() << " ";
          map_out << "0" << endl;
        }
        // end 'v'
      } else {
        if (pe.kind == 'e')
          map_out << "ee" << endl;
        else if (pe.kind == 'c')
          map_out << "convert" << endl;
      }
    }
  }
}

int32_t Coprocessor2::loadPostprocessInfo(std::istream& map_in,
                                          std::ostream& err_out) {
  err_out << "c load postprocess information" << endl;

  string line;
  vector<Lit> literals;

  getline(map_in, line);  // should say: "polarities"
  // assert(line == "polarities");
  unsigned long n_var_pols;
  map_in >> n_var_pols;
  for (int i = 0; i < n_var_pols; ++i) {
    int v;
    long w;
    map_in >> v >> w;
    labelWeight[v] = w;
  }
  getline(map_in, line);  // remove newline

  getline(map_in, line);  // should say: "weight removed"
  map_in >> weightRemoved;
  getline(map_in, line);  // remove newline

  // read number of variables
  getline(map_in, line);  // should say: "original variables"
  assert(line == "original variables");
  getline(map_in, line);  // should say the number of variables
  uint32_t var_cnt = atoi(line.c_str());

  // init data
  err_out << "c load for original " << var_cnt << " variables" << endl;
  search.original_var_cnt = var_cnt;
  search.equivalentTo.resize(var_cnt + 1);
  for (uint32_t i = 0; i < search.equivalentTo.size(); ++i) {
    search.equivalentTo[i] = Lit(i);
  }
  // read compress tables
  err_out << "c read compress data" << endl;
  // read equivalence tables
  Compression* compression = 0;
  // if( do_compress )
  {
    vector<Lit> literals;
    getline(map_in, line);
    err_out << "c load compress table, starting with" << line << endl;
    // if there is no table, break!
    if (!startsWith(line, "compress table")) {
      err_out << "c did not find table in line: " << line << endl;
    } else {
      err_out << "c load found compression data" << endl;
      bool fail = false;
      do {
        // load table
        getline(map_in, line);
        err_out << "c found compress line: " << line << endl;
        // do not work on the line that states that there is no compression
        if (startsWith(line, "no table")) break;

        // number of table
        uint32_t k = atoi(line.substr(line.find(" ")).c_str());
        err_out << "c found iteration " << k << endl;
        // number of variables
        uint32_t var_cnt = atoi(line.substr(line.rfind(" ")).c_str());
        err_out << "c number of original variables " << var_cnt << endl;
        // create compression entry
        compression = new Compression();
        compression->variables = var_cnt;
        compression->mapping = new Var[var_cnt + 1];
        memset(compression->mapping, 0, sizeof(Var) * (var_cnt + 1));
        compression->previous.create(var_cnt);
        // read info
        if (!getline(map_in, line)) {
          err_out << "c could not load table " << k << endl;
          fail = true;
        } else {
          // extract inverse mapping
          parseClause(line, literals);
          for (uint32_t i = 0; i < literals.size(); ++i) {
            compression->mapping[i + 1] = literals[i].toVar();
          }
          // fill remaining parts with 0
          for (uint32_t i = literals.size(); i < var_cnt; ++i) {
            compression->mapping[i + 1] = 0;
          }
          // get units
          if (!getline(map_in, line)) {
            err_out << "c did not find unit header " << k << endl;
            fail = true;
          } else if (!getline(map_in, line)) {
            err_out << "c did not find unit information " << k << endl;
            fail = true;
          } else {
            // process units here
            literals.clear();
            // err_out << "c line to extract units from: " << line << endl;
            parseClause(line, literals);
            for (uint32_t i = 0; i < literals.size(); ++i) {
              // err_out << "c set assignment: " << literals[i].toVar() <<":"<<
              // (int)literals[i].getPolarity() << endl;
              compression->previous.set_polarity(literals[i].toVar(),
                                                 literals[i].getPolarity());
            }

            // compressions.push_back( compression );

            // are there more compressions?
            getline(map_in, line);
          }
          // handle equivalent literals here!
        }
      } while (startsWith(line, "table"));
    }
  }
  err_out << "c read equivalence tables" << endl;
  err_out << "c current line (variables)" << line << endl;
  if (line != "ee table") {
    while (getline(map_in, line)) {
      err_out << "c current line: |" << line << "|" << endl;
      if (line == "ee table") {
        err_out << "c found ee line";
        break;
      }
    }
  }
  assert(line == "ee table" &&
         "there has to be a line <ee table> in a valid map file");
  err_out << "c current line (ee table)" << line << endl;
  // read table entries
  while (getline(map_in, line)) {
    // check whether line starts with literal
    err_out << "c read next class from |" << line << "|" << endl;
    if (isNoLiteral(line)) break;
    literals.clear();
    parseClause(line, literals);
    // update replaced by values
    for (uint32_t i = 1; i < literals.size(); ++i) {
      if (literals[i].toVar() >= search.equivalentTo.size()) {
        // allocate more elements
        const uint32_t oldSize = search.equivalentTo.size();
        search.equivalentTo.resize(literals[i].toVar() + 1);
        // fill them
        for (uint32_t j = oldSize; j < search.equivalentTo.size(); ++j) {
          search.equivalentTo[j] = Lit(j, POS);
        }
        search.var_cnt = literals[i].toVar();
      }
      search.equivalentTo[literals[i].toVar()] =
          literals[i].getPolarity() == POS ? literals[0]
                                           : literals[0].complement();
    }
  }

  err_out << "c current line: (postprocess stack) " << line << endl;

  // read postprocess stack
  // err_out << "c read postprocess stack" << endl;
  Lit lastLiteral = NO_LIT;
  while (getline(map_in, line)) {
    if (startsWith(line, "ve")) {
      postEle elimination;
      elimination.kind = 'v';
      elimination.e.var1 = parseLit(line.substr(3)).toVar();
      //  err_out << "c ve " << elimination.e.var1 << endl;
      uint32_t total = 0;
      size_t pos = line.rfind(' ');
      total = parseLit(line.substr(pos)).toVar();
      elimination.e.number = total;
      elimination.e.clauses = (CL_REF*)malloc(total * sizeof(CL_REF));

      for (uint32_t i = 0; i < total; ++i) {
        if (!getline(map_in, line)) {
          err_out << "c missing clauses for ve " << elimination.e.var1 << endl;
          break;
        }
        literals.clear();
        parseClause(line, literals);
        elimination.e.clauses[i] = search.gsa.create(literals, false);
        for (uint32_t i = 0; i < literals.size(); ++i)
          search.var_cnt = (literals[i].toVar() > search.var_cnt)
                               ? literals[i].toVar()
                               : search.var_cnt;
      }
      postProcessStack.push_back(elimination);
    } else if (startsWith(line, "bce")) {
      // check whether there is a new literal
      // get variable
      size_t pos = line.rfind(' ');
      Lit tmp = parseLit(line.substr(pos));
      lastLiteral = tmp == NO_LIT ? lastLiteral : tmp;

      if (!getline(map_in, line)) {
        err_out << "c was not able to parse blocked clause for variable "
                << lastLiteral.nr() << endl;
        continue;
      }
      literals.clear();
      parseClause(line, literals);
      // create PE and push it to stackt
      postEle pe;
      pe.kind = 'b';
      pe.b.lit1 = lastLiteral;
      pe.b.clause = search.gsa.create(literals, false);
      for (uint32_t i = 0; i < literals.size(); ++i)
        search.var_cnt = (literals[i].toVar() > search.var_cnt)
                             ? literals[i].toVar()
                             : search.var_cnt;
      postProcessStack.push_back(pe);
    } else if (startsWith(line, "ee")) {
      postEle pe;
      // create PE and push it to stackt
      pe.kind = 'e';
      postProcessStack.push_back(pe);
      //      err_out << "c ee" << endl;
    } else if (startsWith(line, "convert")) {
      //      err_out << "c convert" << endl;
    } else {
      err_out << "c operation " << line << " is not known!" << endl;
    }
  }
  if (compression != 0) {
    postEle pe;
    // create PE and push it to stack
    pe.kind = 'd';
    pe.d = *compression;
    pe.d.equivalentTo = search.equivalentTo;
    postProcessStack.push_back(pe);
    delete compression;
  }

  err_out << "c return number of variables: " << var_cnt << endl;
  return var_cnt;
}

void Coprocessor2::postprocess(std::ostream& err_out,
                               vector<Assignment>* assignments, uint32_t vars,
                               bool external) {
  uint32_t currentVariables = search.var_cnt;
  // postprocess all assignments
  for (uint32_t i = 0; i < (*assignments).size(); ++i) {
    Assignment& assignment = (*assignments)[i];
    vector<Lit> equivalentTo = search.equivalentTo;

    // unit clauses are already in the assignments, no need to add them again
    // process stack from the end to the front
    for (int32_t j = postProcessStack.size() - 1; j >= 0; --j) {
      postEle& pe = postProcessStack[j];

      if (pe.kind == 'b') {
        // check whether clause is sat, if not, set blocked lit1 to sat!
        uint32_t k = 0;
#ifdef REGIONALLOCATOR
        Clause& cl = postClauses.get(pe.b.clause);
#else
        Clause& cl = search.gsa.get(pe.b.clause);
#endif
        for (; k < cl.size(); ++k) {
          const Lit l = cl.get_literal(k);
          if (assignment.is_sat(l)) break;
        }

        if (k == cl.size()) {
          assignment.set_polarity(pe.b.lit1.toVar(), pe.b.lit1.getPolarity());
          // remove this variable from being equivalent
          equivalentTo[pe.b.lit1.toVar()] = Lit(pe.b.lit1.toVar(), POS);
        }
      } else if (pe.kind == 'v') {
        const elimination_t& elim = pe.e;

        // do only work on variables, that are still undefined
        if (vars != 0 && elim.var1 > vars) {
          assignment.extend(vars, elim.var1);
          vars = elim.var1;
        }
        if (!assignment.is_undef(elim.var1)) {
          if (vars == 0)
            continue;
          else if (external)
            assignment.undef_variable(elim.var1);
        }

        uint32_t k = 0;
        for (; k < elim.number; ++k) {
          uint32_t ind;
          Lit literal = NO_LIT;
// err_out << "c use clause " << elim.clauses[k] << endl;
#ifdef REGIONALLOCATOR
          Clause& cl = postClauses.get(elim.clauses[k]);
#else
          Clause& cl = search.gsa.get(elim.clauses[k]);
#endif
          for (ind = 0; ind < cl.size(); ++ind) {
            const Lit literal_c = cl.get_literal(ind);
            const Var variable_c = literal_c.toVar();
            // break if clause is unsatisfied, otherwise set to accor. polarity
            if (variable_c == elim.var1)
              literal = literal_c;
            else if (assignment.is_sat(literal_c))
              break;
          }
          // ind only reaches size, if clause is unsatisfied -> set assignment
          if (ind == cl.size()) {
            assignment.set_polarity(literal.toVar(), literal.getPolarity());
            break;
          }
        }
        // if we have choice, check multiple solutions
        if (k == elim.number) {
          // we do have choice. set one polarity and add the other one to the
          // list as well, to get another solution
          if (numberOfSolutions == -1 ||
              numberOfSolutions > (int32_t)(*assignments).size()) {
            Assignment tmp = assignment.copy(currentVariables);
            tmp.set_polarity(elim.var1, NEG);
            (*assignments).push_back(tmp);
          }
          assignment.set_polarity(elim.var1, POS);
        }
        // end 'v'
      } else if (pe.kind == 'e') {
        const equivalence_t& ee = pe.ee;
        // undo equis, replace every assignment of replaced variable by the
        // according other assignment, for eliminated equis
        // remove only the literals that have been replaced in that round

        if (false) {  // faster, but not possible to apply in coprocessor
                      // (completeModel)
          for (uint32_t j = 0; j < ee.number; ++j) {
            Var v = ee.literals[j].toVar();
            Polarity repl_pos_polarity =
                assignment.get_polarity(equivalentTo[v].toVar());
            if (repl_pos_polarity != UNDEF) {
              if (equivalentTo[v].getPolarity() == POS) {
                assignment.set_polarity(v, repl_pos_polarity);
              } else {
                assignment.set_polarity(v, inv_pol(repl_pos_polarity));
              }
            }
          }
        } else {
          // err_out << "c analyze " << currentVariables << " vars for EE" <<
          // endl;
          for (Var var1 = 1; var1 <= currentVariables; var1++) {
            const Lit replaceLit = equivalentTo[var1];
            if (replaceLit == Lit(var1, POS)) {
              continue;
            }
            Polarity repl_pos_polarity =
                (*assignments)[i].get_polarity(replaceLit.toVar());
            if (replaceLit == Lit(0))
              err_out << "c equivalence for " << var1 << " set wrong!" << endl;
            if (repl_pos_polarity != UNDEF) {
              if (replaceLit.getPolarity() == POS) {
                (*assignments)[i].set_polarity(var1, repl_pos_polarity);
              } else {
                (*assignments)[i]
                    .set_polarity(var1, inv_pol(repl_pos_polarity));
              }
            } /*
              else {
              err_out << "c [EQ-POST] restore polarity for " << var1 << " to "
            <<
            "UNDEF" << endl;
            }
            */
          }
        }

      } else if (pe.kind == 'c') {
        const Lit* lits = pe.c.lits;
        // apply ai= (ai && -ai+1) ascending!
        for (uint32_t k = 0; k + 1 < pe.c.size; ++k) {
          const Lit l = lits[k];
          const Var v = l.toVar();
          const bool sat =
              assignment.is_sat(lits[k]) && assignment.is_unsat(lits[k + 1]);
          if (sat) {
            assignment.set_polarity(v, (l.isPositive() ? POS : NEG));
          } else {
            assignment.set_polarity(v, (l.isPositive() ? NEG : POS));
          }
        }
        // postProcessStack.push_back( pe );
      } else if (pe.kind == 'd') {
        // decompress, fill/extend assignment, set search.var_cnt value
        currentVariables = decompress(assignment, currentVariables, pe.d);
        // reconstruct the old equivalence table
        equivalentTo = pe.d.equivalentTo;

      } else {
      }

    }  // end for(stack
  }    // end for assignment
}

bool Coprocessor2::doSimplify(searchData& sd) {
  if (!enabled) return false;
  bool ret = false;
  simpRequests++;
  lastTrailSize = (lastTrailSize == 0) ? search.trail.size() : lastTrailSize;
  if (simpRequestLimit <= simpRequests) ret = true;
  uint32_t tp = (search.trail.size() * 50) / 100;  // 50 percent border
  // if( search.trail.size() > lastTrailSize + tp ) ret = true;
  // setup a higher limit if there will be a simplification

  if (ret) {
    simpRequests = 0;
    simpRequestLimit += simpRequestLimitInc;
    simpRequestLimitInc += simpRequestLimitIncInc;
    if (verbose > 2) cerr << "c [CP2] trigger simplification" << endl;
  }
  return ret;
}

bool Coprocessor2::addEquis(const std::vector<Lit>& eqs, bool explicitely) {
  if (eqs.size() == 0) return true;

  // update intern equi structure
  Lit min = eqs[0];
  Lit roots[eqs.size()];

  // find minimum replacement variable
  uint32_t j = 0;
  for (uint32_t i = 0; i < eqs.size(); ++i) {
    Lit l = eqs[i];
    assert(!search.eliminated.get(l.toVar()) &&
           "there should not be eliminated variables in BIG");
    // assert( search.assignment.is_undef( l.toVar() ) && "replace variables
    // should be unassigned" );
    // take care of the white-list!!
    if (doNotTouch.get(l.toVar())) {
      continue;
    }

    // do not consider eliminated variables!
    if (search.eliminated.get(search.equivalentTo[l.toVar()].toVar())) {
      break;
    }

    // find root element of the tree (traversing linked)
    while (l.toVar() != search.equivalentTo[l.toVar()].toVar()) {
      // do not consider eliminated variables!
      if (search.eliminated.get(search.equivalentTo[l.toVar()].toVar())) {
        break;
      }
      l = l.getPolarity() == POS ? search.equivalentTo[l.toVar()]
                                 : search.equivalentTo[l.toVar()].complement();
    }
    assert(!search.eliminated.get(l.toVar()) &&
           "there should not be eliminated variables in BIG");
    // store the root for the current literal
    assert(!doNotTouch.get(l.toVar()) &&
           "do not use do-not-touch variables during EE");
    roots[j++] = l;
    // look for minimum
    if (min == l.complement()) {
      solution = UNSAT;
      return false;
    }
    min = (l.toVar() < min.toVar()) ? l : min;
  }
  // roots contain the roots of the equivalence classes (represented by trees)
  // link the separate trees to the node with the minimal index -> combine these
  // two trees. settign the right value per variable will be done in the apply
  // method

  for (uint32_t i = 0; i < j; ++i) {
    Lit l = roots[i];
    if (roots[i].toVar() == search.equivalentTo[l.toVar()].toVar())
      search.equivalentTo[l.toVar()] =
          (roots[i].isPositive() ? min : min.complement());
    // find root element of the tree (traversing linked)
    while (l.toVar() != search.equivalentTo[l.toVar()].toVar()) {
      // do not consider eliminated variables!
      if (search.eliminated.get(search.equivalentTo[l.toVar()].toVar())) break;
      if (doNotTouch.get(l.toVar())) break;
      // set replace-literal for current literal in chain
      search.equivalentTo[l.toVar()] =
          (l.getPolarity() == POS) ? min : min.complement();
      // select next literal to analyze
      l = l.getPolarity() == POS ? search.equivalentTo[l.toVar()]
                                 : search.equivalentTo[l.toVar()].complement();
    }
  }

  // TODO check theoretically whether it is necessary to set the values for the
  // eqs literals explicitely (assumption: no)
  if (explicitely) {
    for (uint32_t i = 0; i < eqs.size(); ++i) {
      Lit l = eqs[i];
      if (doNotTouch.get(l.toVar())) continue;

      // find root element of the tree (traversing linked)
      while (l.toVar() != search.equivalentTo[l.toVar()].toVar()) {
        Lit nextL = l.isPositive()
                        ? search.equivalentTo[l.toVar()]
                        : search.equivalentTo[l.toVar()].complement();
        assert(!search.eliminated.get(nextL.toVar()) &&
               "variables in EE should not be eliminated");

        if (nextL.complement() == min) {
          solution = UNSAT;
          break;
        }
        search.equivalentTo[l.toVar()] =
            l.isPositive() ? min : min.complement();
        l = nextL;
      }
      search.equivalentTo[l.toVar()] = l.isPositive() ? min : min.complement();
    }
  }
  return true;
}

void Coprocessor2::extend(uint32_t newVariables, uint32_t suggestedCapacity) {
  extendStructures(newVariables);
}

bool Coprocessor2::runNextTechnique(availableTechniques nextTechnique,
                                    uint32_t round) {
  // if no techniques have been specified
  if (techniques.size() == 0) {
    // cerr << "c no special techniques specified" << endl;
    return true;
  }

  // if there is stated that a fixpoint should be reached, return true
  for (uint32_t i = 0; i < techniques.size() && i < round; ++i) {
    if (techniques[i] == 'f') {
      // cerr << "c run until fix point" << endl;
      return true;
    }
  }

  // if there is stated that a fixpoint should be reached, return true
  for (uint32_t i = 0; i < techniques.size() && i < round; ++i) {
    if (techniques[i] == 'n') {
      // cerr << "c run until fix point" << endl;
      return false;
    }
  }

  if (round - 1 >= techniques.size()) {
    // cerr << "c no more techniques" << endl;
    return false;
  }

  if (techniques[round - 1] == '?') {
    // cerr << "c next technique is don't care" << endl;
    return true;
  }

  // cerr << "c techniques: " << techniques << " current: " <<
  // techniques.c_str()[round-1] << endl;

  switch (nextTechnique) {
    case do_up:
      return (techniques.c_str()[round - 1] == 'u');
    case do_pure:
      return (techniques.c_str()[round - 1] == 'p');
    case do_subSimp:
      return (techniques.c_str()[round - 1] == 's');
    case do_bve:
      return (techniques.c_str()[round - 1] == 'v');
    // deleted something here
    case do_ee:
      return (techniques.c_str()[round - 1] == 'e');
    case do_hte:
      return (techniques.c_str()[round - 1] == 'h');
    case do_bce:
      return (techniques.c_str()[round - 1] == 'b');
    case do_probe:
      return (techniques.c_str()[round - 1] == 'r');
    case do_hyper:
      return (techniques.c_str()[round - 1] == 'y');
    case do_vivi:
      return (techniques.c_str()[round - 1] == 'a');
    // deleted something here
    case do_unhide:
      return (techniques.c_str()[round - 1] == 'g');
      // deleted something here
  }

  return false;
}

bool Coprocessor2::moreIterationsRequired(uint32_t round) const {
  return round < techniques.size();
}

bool Coprocessor2::stopPreprocessing(bool change, uint32_t rounds) const {
  // if no techniques have been specified
  if (techniques.size() == 0) return !change;

  // if there is stated that a fixpoint should be reached, return true
  for (uint32_t i = 0; i < techniques.size() && i < rounds; ++i) {
    if (techniques[i] == 'n') {
      return true;
    }
  }

  // if there is stated that a fixpoint should be reached, return true
  for (uint32_t i = 0; i < techniques.size() && i < rounds; ++i) {
    if (techniques[i] == 'f') {
      return !change;
    }
  }

  // if there are no more techniques to be execured
  if (rounds - 1 >= techniques.size()) return true;
  // there is more to do
  return false;
}

void Coprocessor2::set_parameter(const StringMap& cm) {
#ifdef TESTBINARY
  cm.updateParameter("CP_print", print_dimacs,
                     "output preprocessed instance and stop", false);
  cm.updateParameter("CP_comp", do_compress,
                     "output a compressed version of the formula", false);
  cm.updateParameter("CP_mapFile", mapFile,
                     "file that is used to store the post-process information",
                     false);
  cm.updateParameter("CP_whiteFile", whiteFile,
                     "file of variables that have to be kept during all steps",
                     false);
#endif
#ifndef TESTBINARY
#ifndef COMPETITION
  // display help
  if (cm.contains((const char*)"--verb-help") ||
      cm.contains((const char*)"--help"))
    cerr << endl << "=== Coprocessor2 preprocessor information ===" << endl;
#ifdef USE_COMMANDLINEPARAMETER
  cm.updateParameter("CP_enabled", enabled, "enable/disable preprocessor");
#ifndef CP2RELEASE
  cm.updateParameter("CP_fastSAT", fastSAT,
                     "if the formula is empty, return a simple model?");
  cm.updateParameter("CP_debug", debug, "debug level for preprocessor", 0, 5);
  cm.updateParameter("CP_track", trackClause,
                     "clauseID to track (run with debug > 2 first)", 0, 5,
                     debug > 2);
  cm.updateParameter("CP_verbose", verbose, "verbosity level for preprocessor",
                     0, 5);
#else
  verbose = 1;
#endif
  cm.updateParameter("CP_print", print_dimacs,
                     "output preprocessed instance and stop");
  cm.updateParameter("CP_group", group_encode, "detect and use group variables",
                     false);
  cm.updateParameter("CP_onlyGroup", only_group, "Only group encode", false);
  cm.updateParameter("CP_sort", sort, "sort the formula before it is output");
  cm.updateParameter("CP_pline", pline, "output pline of preprocessed formula");
  cm.updateParameter("CP_whiteFile", whiteFile,
                     "file of variables that have to be kept during all steps");
  cm.updateParameter(
      "CP_blackFile", blackFile,
      "file of variables that have to be eliminated after preprocessing");
  cm.updateParameter("CP_mapFile", mapFile,
                     "file that is used to store the post-process information");
#ifndef CP2RELEASE
  cm.updateParameter("CP_simpLimitInc", simpRequestLimitInc,
                     "number of conflicts between two simplifications", 0,
                     100000);
  cm.updateParameter("CP_simpLimitIncInc", simpRequestLimitIncInc,
                     "increase of distance between two simplifications", 1,
                     100000);
#endif

  cm.updateParameter("CP_comp", do_compress,
                     "output a compressed version of the formula");
  cm.updateParameter("CP_rewrite", rewriteMode,
                     "try to rewrite cardinality encodings(0=never,1=before "
                     "pp,2=after pp,3=before and after",
                     0, 3);
  cm.updateParameter("CP_rewAMO", do_rewrite1,
                     "try to rewrite at most 1 constraints");
#ifndef CP2RELEASE
  cm.updateParameter("CP_rewAMK", do_rewritek,
                     "try to rewrite at most k constraints (naive, sinz)");
// deleted something here
#endif

  uint32_t cpTime = 0;
  cm.updateParameter("CP_time", cpTime, "time in seconds for preprocessing", 0,
                     4000000000);
  if (cpTime != 0) {
    uint64_t tmpTime = cpTime;
    timeoutTime = get_microseconds() + tmpTime * 1000000;
  }

  cm.updateParameter("sol", numberOfSolutions,
                     "number of solutions that should be found", 0, 4000000000,
                     false);
  cm.updateParameter("CP_techniques", techniques,
                     "order to apply techniques, with loops e.g. 'upsvwfr' or "
                     "'[usv]+wr[us]+e");
  cm.updateParameter("CP_unlimited", unlimited,
                     "do not apply any limits for the techniques");

  cm.updateParameter("CP_variableLimit", variableLimit,
                     "number of variables that are still handled", 0,
                     2000000000);
  cm.updateParameter("CP_clauseLimit", clauseLimit,
                     "number of clauses that are still handled", 0, 2000000000);
#ifndef CP2RELEASE
  cm.updateParameter("CP_fastLoop", fastLoop,
                     "interrupt a technique, if a cheaper one can do something",
                     0, 3);
  cm.updateParameter("CP_randomized", randomized,
                     "runs techniques in randomized fashion");
#endif
  cm.updateParameter("CP_up", up, "[u] do unit propagation");
  cm.updateParameter("CP_pure", pure, "[p] do pure literal elimination");
  cm.updateParameter("CP_pureSimp", pureSimp,
                     "do pure literal elimination during simplification");

  cm.updateParameter("CP_subSimp", subSimp,
                     "[s] do subsumption and self-subsuming resolution");
  cm.updateParameter("CP_bve", bve, "[v]do bounded variable elimination");
  cm.updateParameter("CP_bveLimit", bveLimit, "checks until BVE is stopped", 0,
                     4000000000);
  cm.updateParameter("CP_bveGATES", bveGate, "apply gate rule during BVE");
#ifndef CP2RELEASE
  cm.updateParameter("CP_bveGX", bveGateXor,
                     "apply gate rule for xor during BVE");
  cm.updateParameter("CP_bveGITE", bveGateIte,
                     "apply gate rule for ITE during BVE");
  cm.updateParameter("CP_bveOTFSS", bveOTFSS,
                     "apply on the fly self subsumption during BVE");
  cm.updateParameter("CP_bveShrink", bveShrink,
                     "apply BVE only if 0=number of clauses,1=number of "
                     "literals,2=both values decrease",
                     0, 2);
// deleted something here
#endif
  cm.updateParameter("CP_ee", ee, "[e] apply equivalence elimination");
  cm.updateParameter("CP_eeBehindBVE", eeBehindBve, "run ee after bve");
  cm.updateParameter("CP_eeLimit", eeLimit, "checks until EE is stopped", 0,
                     4000000000);
  cm.updateParameter("CP_hte", hte, "[h] apply hidden tautology elimination");
  cm.updateParameter("CP_hteLimit", hteLimit, "checks until HTE is stopped", 0,
                     4000000000);
  cm.updateParameter("CP_bce", bce, "[b] apply blocked clause elimination");
  cm.updateParameter("CP_bceLimit", bceLimit, "checks until bce is stopped", 0,
                     4000000000);
#ifndef CP2RELEASE
  cm.updateParameter(
      "CP_hbce", hbce,
      "apply hidden blocked clause elimination during HTE (expensive)");
  cm.updateParameter("CP_hbceLimit", hbceLimit, "checks until hbce is stopped",
                     0, 4000000000);
#endif
  cm.updateParameter(
      "CP_probe", probe,
      "[r] apply probing (assume l, -l; detect units, equivalences)");
  cm.updateParameter("CP_prDouble", prDouble,
                     "candidates to perform double look-ahead (expensive)", 0,
                     4000000000);
#ifndef CP2RELEASE
  cm.updateParameter("CP_prPure", prPure,
                     "apply pure literal detection after propagating an "
                     "assumption (expensive)");
  cm.updateParameter("CP_prPureC", prPureClause,
                     "add the found binary clause also to the formula");
  cm.updateParameter(
      "CP_prAutarky", prAutarky,
      "apply autarky detection after propagating an assumption(expensive)");
#endif
  cm.updateParameter("CP_prLHBR", prLHBR,
                     "do lazy hyper binary resolution during probing");
  cm.updateParameter("CP_prBinary", prBinary,
                     "apply probing for binary clauses");
  cm.updateParameter("CP_prLimit", probeLimit,
                     "checks until probing is not applied any more", 0,
                     4000000000);
#ifndef CP2RELEASE
  cm.updateParameter("CP_prPLimit", probePureLimit,
                     "checks until pure-probing is not applied any more", 0,
                     4000000000);
#endif
  cm.updateParameter("CP_prDLimit", probeDoubleLimit,
                     "checks until double-probing is not applied any more", 0,
                     4000000000);
  cm.updateParameter("CP_prBLimit", probeBinaryLimit,
                     "checks until binary-probing is not applied any more", 0,
                     4000000000);
#ifndef CP2RELEASE
  cm.updateParameter("CP_hyper", hyper, "[y] apply hyper binary resolution");
#endif
  cm.updateParameter("CP_vivi", vivi, "[a] apply clause vivification");
  cm.updateParameter("CP_viviLenP", viviLenPercent,
                     "percent of maximal clause size that is analyzed", 0,
                     4000000000);
  cm.updateParameter("CP_viviLimit", viviLimit, "propagations for vivification",
                     0, 4000000000);
  // deleted something here
  cm.updateParameter("CP_unhide", unhide, "[g] unhiding algorithm");
  cm.updateParameter("CP_unhideTran", uhdTransitive,
                     "remove transitive edges from BIG during unhiding");
  cm.updateParameter("CP_unhideIter", unhideIter,
                     "number of iterations for the unhiding algorithm", 0, 10);
  cm.updateParameter("CP_unhideAdv", advancedStamping,
                     "use advanced stamping unhiding algorithm");
  cm.updateParameter("CP_unhideHLE", doUHLE,
                     "use hidden literal elimination during unhiding", 0, 3);
  cm.updateParameter("CP_unhideHTE", doUHTE,
                     "use hidden tautology elimination during unhiding");
  cm.updateParameter("CP_unhideNoShuffle", uhdNoShuffle,
                     "do not shuffle adjacency lists for unhiding");
  cm.updateParameter("CP_unhideEE", uhdEE,
                     "do EE detection during advanced stamping");
// deleted something here

#endif
#endif

#endif

  // deleted something here
  //
  //    if ( whiteFile.size() != 0 ) {
  //        VarFileParser vfp( whiteFile );
  //        vector<Var> evars;
  //        evars = vector< Var >();
  //       vfp.extract(evars);
  //  for ( uint32_t i = 0; i <  evars.size(); ++i ) {
  //          if ( evars[i] <= search.var_cnt )  doNotTouch.set( evars[i],
  //          true);
  //     }

  // }

  // activate all dependencies:
  if (unhide && advancedStamping) ee = true;
}

void Coprocessor2::scanClauses() {
  uint32_t unremovedIgnored = 0;
  uint32_t unremovedElim = 0;
  uint32_t unremovedRemoved = 0;
  uint32_t unremovedSat = 0;
  uint32_t partiallyRemoved = 0;
  uint32_t nonExistendLit = 0;
  uint32_t doubleOccurence = 0;
  uint32_t emptyClauses = 0;
  uint32_t wronLitCount = 0;
  uint32_t replaceLits = 0;
  uint32_t unvalidEdges = 0;
  uint32_t unit = 0;
  uint32_t unremovedReplaced = 0;

  for (uint32_t i = 0; i < (*formula).size(); ++i) {
    const CL_REF clause = (*formula)[i];
    Clause& cl = search.gsa.get(clause);

    uint32_t size = cl.size();

    if (size == 0) emptyClauses++;

    uint32_t found = 0;
    bool containsElim = false;
    bool containsRemoved = false;
    bool isSat = false;
    bool ignored = cl.isIgnored();

    if (!ignored && cl.size() == 1) {
      unit++;
    }

    // check, whether in all occ lists, sat, eliminated
    for (uint32_t j = 0; j < size; j++) {
      const Lit l = cl.get_literal(j);
      isSat = isSat || search.assignment.is_sat(l);
      //            containsElim = containsElim ||  eliminated.get( l.toVar() );
      //            containsRemoved = containsRemoved ||  varRemoved.get(
      //            l.toVar() );
      for (uint32_t xyz = 0; xyz < occurrenceList[l.toIndex()].size(); xyz++) {
        if (occurrenceList[l.toIndex()][xyz] == clause) {
          found++;
          break;
        }
      }

      // count double occurences of literals
      for (uint32_t k = j + 1; k < size; k++) {
        if (cl.get_literal(k) == l) {
          doubleOccurence++;
          break;
        };
      }

      // count whether in the formula there are still occurrences of replace
      // literals
      if (!ignored && search.equivalentTo[l.toVar()] != Lit(l.toVar(), POS)) {
        replaceLits++;
      }
    }

    if (isSat > 0 && found > 0) {
      unremovedSat++;
    }
    if (found > 0 && containsElim) unremovedElim++;
    if (found > 0 && containsRemoved) unremovedRemoved++;
    if (found > 0 && found < size) {
      partiallyRemoved++;
    }
    if (ignored && found > 0) unremovedIgnored++;
  }

  // check whether a clause in a list also contains the according variable
  for (int i = -(int)(search.var_cnt); i <= (int)search.var_cnt; ++i) {
    if (i == 0) continue;
    Lit literal = Lit(i);

    vector<CL_REF>& listI = occurrenceList[literal.toIndex()];
    uint32_t size = listI.size();

    uint32_t notIgnored = 0;
    for (uint32_t j = 0; j < size; j++) {
      const Clause& cl = search.gsa.get(listI[j]);
      if (cl.isIgnored()) continue;

      notIgnored++;
      uint32_t k = 0;
      for (; k < cl.size(); k++) {
        if (cl.get_literal(k) == literal) break;
      }
      if (k == cl.size()) {
        nonExistendLit++;
      }
    }
    if (litOcc[literal.toIndex()] != notIgnored) {
      wronLitCount++;
    }

    const vector<Lit>& implies = search.big.getAdjacencyList(literal);
    const vector<CL_REF>& occL = list(literal.complement());
    for (uint32_t j = 0; j < implies.size(); ++j) {
      Lit other = implies[j];
      uint32_t k = 0;
      for (; k < occL.size(); ++k) {
        const Clause& cl = search.gsa.get(occL[k]);
        if (cl.isIgnored() || cl.size() != 2) continue;
        if (other == cl.get_literal(0) || other == cl.get_literal(1)) break;
      }
      if (k == occL.size()) {
        unvalidEdges++;
      }
    }
  }

  cerr << "empty clauses:         " << emptyClauses << endl;
  cerr << "unremoved sat:         " << unremovedSat << endl;
  cerr << "unremoved eliminated:  " << unremovedElim << endl;
  cerr << "unremoved removed:     " << unremovedRemoved << endl;
  cerr << "partially removed:     " << partiallyRemoved << endl;
  cerr << "unremoved ignored:     " << unremovedIgnored << endl;
  cerr << "double occurences:     " << doubleOccurence << endl;
  cerr << "non existend lit :     " << nonExistendLit << endl;
  cerr << "wrong lit count:       " << wronLitCount << endl;
  cerr << "replaced literals:     " << replaceLits << endl;
  cerr << "invalid edges in BIG>= " << unvalidEdges << endl;
  cerr << "unit clauses:          " << unit << endl;
  cerr << "unit queue: ";
  for (uint32_t i = 0; i < propagationQueue.size(); i++)
    cerr << " " << propagationQueue.at(i).nr();
  cerr << " [" << propagationQueue.size() << "]" << endl;
}

void Coprocessor2::showStatistics() {
  int32_t sum = 0;
  for (uint32_t i = 1; i <= numberOfTechniques; ++i) {
    sum += techniqueTime[i];
  }
  if (sum == 0) return;  // do not display, if nothing happened

  string techs[] = {"",     "unit",   "pure", "susi",  "bve",   "none",
                    "ee",   "hte",    "bce",  "probe", "hyper", "vivi",
                    "none", "unhide", "none", "none"};
  cerr << "c [CP2] preprocessor technique summary" << endl;
  cerr << "c [CP2] technique "
          "\tcls\tevents\ttime\tcls/second\tcalls\tchecks\tliterals" << endl;
  for (uint32_t i = 1; i <= numberOfTechniques; ++i) {
#ifdef CP2RELEASE  // print only techniques that are visible
    if ((i == 5 || i >= 12) && i != 13) continue;
#endif
    cerr << "c " << techs[i] << "\t\t" << techniqueClaues[i] << "\t"
         << techniqueEvents[i] << "\t" << techniqueTime[i] / 1000 << "ms"
         << "\t" << (double)techniqueClaues[i] * (double)1000000.0 /
                        (double)(techniqueTime[i]) << "\t" << techniqueCalls[i]
         << "\t" << techniqueCheck[i] << "\t" << techniqueLits[i] << endl;
  }
  cerr << "c [CP2] remaining limits: " << endl;
  cerr << "c [CP2] bve: " << bveLimit
      // deleted something here
       << " susi: " << susiLimit << " ee: " << eeLimit << " hte: " << hteLimit
       << " bce: " << bceLimit
#ifndef CP2RELEASE
       << " hbce: " << hbceLimit
#endif
       << endl << "c [CP2] pr:   " << probeLimit
#ifndef CP2RELEASE
       << " ppr:  " << probePureLimit
#endif
       << " dpr:  " << probeDoubleLimit << " bpr:  " << probeBinaryLimit
       << " vivi: " << viviLimit
      // deleted something here
       << endl;
  cerr << "c [CP2] maintain time: " << maintainTime / 1000 << " ms" << endl;
}

//
//
// ==== Maintaining Formula ====
//
//

vector<CL_REF>& Coprocessor2::list(const Lit& l) {
  return occurrenceList[l.toIndex()];
}

bool Coprocessor2::addFormula(vector<CL_REF>& clauses) {
  for (uint32_t i = 0; i < clauses.size(); ++i) {
    if (!addClause(clauses[i])) return false;
  }
  return true;
}

CL_REF Coprocessor2::addNexClause(const Lit& l1, const Lit& l2) {
  Lit literals[2];
  literals[0] = l1;
  literals[1] = l2;
  CL_REF ref = search.gsa.create(literals, 2, false);
  formula->push_back(ref);
  return addClause(ref) == true ? ref : 0;
}

bool Coprocessor2::addClause(const CL_REF cl, bool callChange) {
  Clause& clause = search.gsa.get(cl);
  if (clause.isIgnored()) return true;
  if (clause.size() < 2) {
    if (clause.size() == 0) return false;
    const Lit l = clause.get_literal(0);
    list(l).push_back(cl);
    clause.markToDelete();
    return enqueTopLevelLiteral(l);
  } else {
    if (clause.size() == 2 && clause.isLearnt())
      clause.setLearnt(
          false);  // all binary clauses are declared "non-redundant"!
    for (uint32_t i = 0; i < clause.size(); ++i) {
      const Lit& l = clause.get_literal(i);
      list(l).push_back(cl);
      updateInsertLiteral(l);
    }
    updateChangeClause(cl, callChange);
  }
  return true;
}

/// remove a certain clause from the given list
bool Coprocessor2::removeFromList(const Lit& l, const CL_REF ref) {
  for (uint32_t j = 0; j < list(l).size(); ++j) {
    if (list(l)[j] == ref) {
      list(l)[j] = list(l)[list(l).size() - 1];
      list(l).pop_back();
      return true;
    }
  }
  return false;
}

void Coprocessor2::removeListIndex(const Lit& l, const uint32_t& index) {
  list(l)[index] = list(l)[list(l).size() - 1];
  list(l).pop_back();
}

void Coprocessor2::removeClause(const CL_REF cl) {
  const Clause& clause = search.gsa.get(cl);
  for (uint32_t i = 0; i < clause.size(); ++i) {
    const Lit& l = clause.get_literal(i);
    for (uint32_t j = 0; j < list(l).size(); ++j) {
      if (list(l)[j] == cl) {
        list(l)[j] = list(l)[list(l).size() - 1];
        list(l).pop_back();
        break;
      }
    }
  }
}

void Coprocessor2::removeClause(const Lit a, const Lit b, bool markDelete) {
  uint32_t ref = 0;
  for (uint32_t i = 0; i < list(a).size(); ++i) {
    Clause& cl = search.gsa.get(list(a)[i]);
    if (cl.isIgnored() || cl.size() != 2) continue;
    if (cl.get_literal(0) == b || cl.get_literal(1) == b) {
      if (markDelete) cl.markToDelete();
      ref = list(a)[i];
      removeListIndex(a, i);
      break;
    }
  }
  removeFromList(b, ref);
  if (ref != 0) {
    updateRemoveLiteral(a);
    updateRemoveLiteral(b);
  }
}

void Coprocessor2::reduceFormula() {

  vector<CL_REF>& clauses = *formula;
  uint32_t j = 0;
  for (uint32_t i = 0; i < clauses.size(); ++i) {
    const Clause& clause = search.gsa.get(clauses[i]);
    if (!clause.isIgnored()) {
      // remove units!
      if (clause.size() == 1 && search.assignment.is_sat(clause.get_literal(0)))
        continue;
      clauses[j++] = clauses[i];
    } else {
      if (clause.isToDelete()) {
        // TODO FIXME delete clause here, make sure that reference is in no
        // other structure any more!
        const uint32_t s = clause.getWords();
        clause.~Clause();
        search.gsa.release(clauses[i], s);
      }
    }
  }
  clauses.resize(j);
}

void Coprocessor2::clearStructures() {
  for (Var v = 1; v <= search.var_cnt; ++v) {
    list(Lit(v, POS)).clear();
    list(Lit(v, NEG)).clear();
  }
  memset(varOcc, 0, sizeof(uint32_t) * (search.capacity() + 1));
  memset(litOcc, 0, sizeof(uint32_t) * max_index(search.capacity()));

  bveTouchedVars.clear(search.capacity() + 1);
  eqLitInStack.clear(max_index(search.capacity() + 1));
  eqInSCC.clear(search.capacity() + 1);
  prArray.clear(max_index(search.capacity() + 1));

  variableHeap.clear();
  bveHeap.clear();
  // deleted something here
  hteHeap.clear();
  bceHeap.clear();
  literalHeap.clear();
}

void Coprocessor2::updateRemoveClause(const Clause& clause) {
  for (uint32_t i = 0; i < clause.size(); ++i) {
    updateRemoveLiteral(clause.get_literal(i));
  }
}

void Coprocessor2::updateChangeClause(const CL_REF clause, bool addToSubsume) {
  if (addToSubsume) subsumptionQueue.push_back(clause);
  Clause& cl = search.gsa.get(clause);
  if (cl.size() == 2) {  // notify that there are new literals that imply
                         // something (new edge in big)
    cl.setLearnt(false);
    const Lit lit0 = cl.get_literal(0);
    const Lit lit1 = cl.get_literal(1);
    if (!eqTouched.get(lit0.toIndex())) {
      eqDoAnalyze.push_back(lit0);
      eqTouched.set(lit0.toIndex(), true);
    }
    if (!eqTouched.get(lit1.toIndex())) {
      eqDoAnalyze.push_back(lit1);
      eqTouched.set(lit1.toIndex(), true);
    }
    search.big.addEdge(lit0.complement(), lit1);
    search.big.addEdge(lit1.complement(), lit0);
  } else {
    if (bve) {
      for (uint32_t i = 0; i < cl.size(); ++i) {
        const Var v = cl.get_literal(i).toVar();
        if (!bveHeap.contains(v)) bveHeap.insert_item(v);
      }
    }
    if (bce) {
      for (uint32_t i = 0; i < cl.size(); ++i) {
        const Var v = cl.get_literal(i).toVar();
        if (!bceHeap.contains(v)) bceHeap.insert_item(v);
      }
    }
    // deleted something here
  }
}

void Coprocessor2::updateInsertLiteral(const Lit& l) {
  litOcc[l.toIndex()]++;
  varOcc[l.toVar()]++;
  if (variableHeap.contains(l.toVar()))
    variableHeap.update_item(l.toVar());
  else
    variableHeap.insert_item(l.toVar());
  if (literalHeap.contains(l.data()))
    literalHeap.update_item(l.data());
  else
    literalHeap.insert_item(l.data());

  if (bve) {
    if (bveHeap.contains(l.toVar()))
      bveHeap.update_item(l.toVar());
    else
      bveHeap.insert_item(l.toVar());
  }

  // deleted something here

  if (hte) {
    if (hteHeap.contains(l.toVar()))
      hteHeap.update_item(l.toVar());
    else
      hteHeap.insert_item(l.toVar());
  }

  if (bce) {
    if (bceHeap.contains(l.toVar()))
      bceHeap.update_item(l.toVar());
    else
      bceHeap.insert_item(l.toVar());
  }
}

void Coprocessor2::updateRemoveLiteral(const Lit& l) {
  litOcc[l.toIndex()]--;
  varOcc[l.toVar()]--;
  // update heaps here!

  assert(variableHeap.contains(l.toVar()));
  if (variableHeap.contains(l.toVar())) variableHeap.update_item(l.toVar());
  assert(literalHeap.contains(l.data()));
  if (literalHeap.contains(l.data())) literalHeap.update_item(l.data());

  if (bve) {
    if (bveHeap.contains(l.toVar()))
      bveHeap.update_item(l.toVar());
    else
      bveHeap.insert_item(l.toVar());
  }
  // deleted something here
  if (hte && hteHeap.contains(l.toVar())) hteHeap.update_item(l.toVar());
  if (bce) {
    if (bceHeap.contains(l.toVar()))
      bceHeap.update_item(l.toVar());
    else
      bceHeap.insert_item(l.toVar());
  }
}

void Coprocessor2::updateDeleteLiteral(const Lit& l) {
  varOcc[l.toVar()] -= litOcc[l.toIndex()];
  litOcc[l.toIndex()] = 0;
  // update heaps here!

  if (!variableHeap.contains(l.toVar())) {
    scanClauses();
    assert(variableHeap.contains(l.toVar()) && "literal needs to be in heap");
  }
  if (variableHeap.contains(l.toVar())) variableHeap.update_item(l.toVar());
  if (literalHeap.contains(l.data())) literalHeap.update_item(l.data());

  if (bve && bveHeap.contains(l.toVar())) bveHeap.update_item(l.toVar());
  // deleted something here
  if (hte && hteHeap.contains(l.toVar())) hteHeap.update_item(l.toVar());
  if (bce && bceHeap.contains(l.toVar())) bceHeap.update_item(l.toVar());
}

void Coprocessor2::garbageCollect() {
  if (search.gsa.getGarbageFraction() > 0.1 && propagationQueue.empty()) {
    // remove all old Clauses from the formula
    reduceFormula();
    // do garbage collection
    // create new and bigger allocator
    uint32_t oldCap = search.gsa.capacity();
    // uint32_t more = ((oldCap >> 1) + (oldCap >> 3) + 2) & ~1;
    Allocator large(search.gsa.capacity());

    // rewrite everything from the first to the second allocator
    for (uint32_t i = 0; i < formula->size(); ++i) {
      const uint32_t ref = (*formula)[i];
      uint32_t newRef = large.alloc(search.gsa.get(ref).getWords());
      large.get(newRef).copyFrom(search.gsa.get(ref));
      (*formula)[i] = newRef;
    }
    // more the new memory to the old allocator
    large.moveTo(search.gsa);
    trackClause = 0;

    // reinit forula
    for (Var v = 1; v <= search.var_cnt; ++v) {
      list(Lit(v, POS)).clear();
      list(Lit(v, NEG)).clear();
    }
    for (uint32_t i = 0; i < formula->size(); ++i) {
      const Clause& cl = search.gsa.get((*formula)[i]);
      assert(!cl.isIgnored() &&
             "after formula reduce there cannot be ignored clauses any more");
      for (uint32_t j = 0; j < cl.size(); ++j) {
        list(cl.get_literal(j)).push_back((*formula)[i]);
      }
    }
  }
}

void Coprocessor2::sortListLength(const Lit& l) {
  const uint32_t s = list(l).size();
  uint32_t i = 0;
  // remove ignored
  for (uint32_t j = 0; j < s; ++j) {
    const Clause& clause = search.gsa.get(list(l)[j]);
    if (!clause.isIgnored()) list(l)[i++] = list(l)[j];
  }
  list(l).resize(i);
  // sort according to size
  for (uint32_t j = 1; j < s; ++j) {
    const uint32_t key = list(l)[j];
    const uint32_t keySize = search.gsa.get(list(l)[j]).size();
    int32_t i = j - 1;
    while (i >= 0 && search.gsa.get(list(l)[i]).size() > keySize) {
      list(l)[i + 1] = list(l)[i];
      i--;
    }
    list(l)[i + 1] = key;
  }
}

void Coprocessor2::sortClauseList(vector<CL_REF>& clauseList) {
  int32_t n = clauseList.size();
  int32_t m, s;
  // copy elements from vector
  CL_REF* tmpA = new CL_REF[n];
  CL_REF* a = tmpA;
  for (int32_t i = 0; i < n; i++) {
    a[i] = clauseList[i];
  }
  CL_REF* tmpB = new CL_REF[n];
  CL_REF* b = tmpB;

  // size of work fields, power of 2
  for (s = 1; s < n; s += s) {
    m = n;
    do {
      m = m - 2 * s;                         // set begin of working field
      int32_t hi = (m + s > 0) ? m + s : 0;  // set middle of working field

      int32_t i = (m > 0) ? m : 0;  // lowest position in field
      int32_t j = hi;

      int32_t stopb = m + 2 * s;  // upper bound of current work area
      int32_t currentb = i;       // current position in field for copy

      // merge two sorted fields into one
      while (i < hi && j < stopb) {
        if ((search.gsa.get(a[i])) < (search.gsa.get(a[j])))
          b[currentb++] = a[i++];
        else
          b[currentb++] = a[j++];
      }
      // copy rest of the elements
      for (; i < hi;) b[currentb++] = a[i++];

      for (; j < stopb;) b[currentb++] = a[j++];

    } while (m > 0);

    // swap fields!
    CL_REF* tmp = a;
    a = b;
    b = tmp;
  }
  // write data back into vector
  for (int32_t i = 0; i < n; i++) {
    clauseList[i] = a[i];
  }

  delete[] tmpA;
  delete[] tmpB;
  return;
}

uint32_t Coprocessor2::countLiterals() {
  if (solution == SAT)
    return search.var_cnt;
  else if (solution == UNSAT)
    return 0;
  uint32_t lits = 0;
  for (Var v = 1; v <= search.var_cnt; v++) {
    if (!search.assignment.is_undef(v)) lits++;
  }
  for (uint32_t i = 0; i < formula->size(); ++i) {
    const Clause& clause = search.gsa.get((*formula)[i]);
    if (clause.isIgnored()) continue;
    if (clause.size() < 1) continue;
    lits += clause.size();
  }
  return lits;
}

void Coprocessor2::printFormula(std::ostream& data_out, std::ostream& err_out) {
  uint32_t units = 0;
  uint32_t labelsRemoved = 0;
  uint32_t final_Pos_Labels = 0;
  uint32_t final_Neg_Labels = 0;

  for (Var v = 1; v <= search.var_cnt; v++) {
    if (!search.assignment.is_undef(v)) units++;
  }

  if (pline) {
    if (solution == SAT) {
      err_out << "p cnf " << search.var_cnt << "0" << endl;
    } else if (solution == UNSAT) {
      err_out << "p cnf 0 1" << endl;
    } else {
      uint32_t clauseCount = 0;
      for (uint32_t i = 0; i < formula->size(); ++i) {
        const Clause& clause = search.gsa.get((*formula)[i]);
        if (clause.isIgnored()) continue;
        if (clause.size() < 2) continue;
        clauseCount++;
      }
      err_out << "p cnf " << search.var_cnt << " " << clauseCount << endl;
    }
  }

  if (print_dimacs) {
    if (solution == SAT) {
      printMaxSATSolution(data_out);
    } else if (solution == UNSAT) {
      data_out << "p wcnf 0 1" << endl << "0" << endl;
    } else {
      uint32_t clauseCount = 0;
      for (uint32_t i = 0; i < formula->size(); ++i) {
        const Clause& clause = search.gsa.get((*formula)[i]);
        if (clause.isIgnored()) continue;
        if (clause.size() < 2) continue;
        clauseCount++;
      }

      data_out << "c assumptions ";
      for (auto& t_w : labelWeight) {
        Var t = t_w.first;
        long weight = t_w.second;
        if (doNotTouch.get(t) && search.assignment.is_undef(t) &&
            (list(Lit(t, weight < 0 ? POS : NEG)).size() > 0)) {
          data_out << (weight < 0 ? " -" : " ") << t;
          clauseCount++;
        }
      }
      data_out << endl;

      data_out << "p wcnf " << search.var_cnt << " " << clauseCount + units
               << " " << topW << endl;
      for (Var v = 1; v <= search.var_cnt; v++) {
        if (!search.assignment.is_undef(v))
          data_out << topW << " " << Lit(v, search.assignment.get_polarity(v))
                                         .nr() << " 0" << endl;
      }
      for (uint32_t i = 0; i < formula->size(); ++i) {
        const Clause& clause = search.gsa.get((*formula)[i]);

        if (clause.isIgnored()) continue;
        if (clause.size() < 2) continue;

        data_out << topW << " ";
        for (uint32_t j = 0; j < clause.size(); ++j)
          data_out << clause.get_literal(j).nr() << " ";
        data_out << "0" << endl;
      }
      // HERE PRINT WHITEVARS

      for (auto& t_w : labelWeight) {
        Var t = t_w.first;
        std::stringstream s;
        if (doNotTouch.get(t)) {
          long weight = t_w.second;

          if (weight == 0) continue;
          if (!search.assignment.is_undef(t)) {
            if ((weight * Lit(t, search.assignment.get_polarity(t)).nr()) < 0) {
              weightRemoved += abs(weight);
            }
          } else if ((list(Lit(t, weight < 0 ? POS : NEG)).size() == 0)) {
            labelsRemoved++;
          } else {
            data_out << abs(weight) << (weight < 0 ? " -" : " ") << t;
            data_out << " 0" << endl;
            data_out << s.str();
            if (weight < 0)
              final_Neg_Labels++;
            else
              final_Pos_Labels++;
          }
        }
      }

      /*
      err_out << "c [CP2] Weight removed " << weightRemoved << endl;
      err_out << "c [CP2] Labels removed " << labelsRemoved << endl;
      err_out << "c [CP2] Final Positive Labels " << final_Pos_Labels << endl;
      err_out << "c [CP2] Final Negative Labels " << final_Neg_Labels << endl;
      err_out << "c [CP2] Cores Not Added in VE=" << missedCores << endl;
      err_out << "c [CP2] Cores Added in VE=" << addedCores << endl;
      if (mod_cores)
        err_out << "c [CP2] Cores were added due to modification" << endl;
      err_out << "c DONE" << endl;
      */
    }
  }
}

void Coprocessor2::printMaxSATSolution(std::ostream& data_out) {
  // We get here if formula size is 0.
  data_out << "c SOLVED BY PREPROCESSOR " << endl;
  // COUNT OPTIMUM WEIGHT
  long solutionW = 0;
  for (auto& t_w : labelWeight) {
    Var t = t_w.first;
    if (doNotTouch.get(t) && !search.assignment.is_undef(t)) {
      long weight = t_w.second;
      if ((weight * Lit(t, search.assignment.get_polarity(t)).nr()) < 0) {
        solutionW += abs(weight);
      }
    }
  }
  data_out << "o " << solutionW << endl;
  data_out << "v ";
  for (Var v = 1; v <= search.var_cnt; ++v) {
    if (!search.assignment.is_undef(v)) {
      data_out << Lit(v, search.assignment.get_polarity(v)).nr() << " ";
    } else if (doNotTouch.get(v)) {
      data_out << Lit(v, labelWeight[v] < 0 ? NEG : POS).nr() << " ";
    }
  }
  data_out << endl;
  data_out << "s OPTIMUM FOUND" << endl;
}

void Coprocessor2::printEmptyMapfile(std::ostream& map_out) {
  // dump info
  map_out << "original variables" << endl;
  map_out << search.original_var_cnt << " - " << search.var_cnt << endl;
  // write compress info once
  // write eq info once
  map_out << "ee table" << endl;
  map_out << "postprocess stack" << endl;
}

Var Coprocessor2::nextVariable() {
  Var next = search.var_cnt + 1;
  // since a capacity is used, the structures are not always increased
  extendStructures(next);
  assert(next <= search.var_cnt &&
         "search data has to be large enough to cover new variable");
  return next;
}

void Coprocessor2::removeDuplicateClauses(const Lit literal) {
  for (uint32_t i = 0; i < list(literal).size(); ++i) {
    Clause& clause = search.gsa.get(list(literal)[i]);
    if (clause.isIgnored()) {
      removeListIndex(literal, i);
      --i;
      continue;
    }
    clause.sort();
  }

  for (uint32_t i = 0; i < list(literal).size(); ++i) {
    Clause& clauseI = search.gsa.get(list(literal)[i]);
    for (uint32_t j = i + 1; j < list(literal).size(); ++j) {
      CL_REF removeCandidate = list(literal)[j];
      Clause& clauseJ = search.gsa.get(removeCandidate);
      if (clauseI.size() != clauseJ.size()) continue;
      for (uint32_t k = 0; k < clauseI.size(); ++k) {
        if (clauseI.get_literal(k) != clauseJ.get_literal(k))
          goto duplicateNextJ;
      }
      removeClause(removeCandidate);
      updateRemoveClause(clauseJ);
      j--;
    duplicateNextJ:
      ;
    }
  }
}

bool Coprocessor2::timedOut() const {
  return timeoutTime != 0 && timeoutTime < get_microseconds();
}

void Coprocessor2::techniqueStart(availableTechniques nextTechnique) {
  techniqueCalls[nextTechnique]++;
  techniqueTime[nextTechnique] =
      get_microseconds() - techniqueTime[nextTechnique];
}

void Coprocessor2::techniqueStop(availableTechniques nextTechnique) {
  techniqueTime[nextTechnique] =
      get_microseconds() - techniqueTime[nextTechnique];
}

void Coprocessor2::techniqueChangedClauseNumber(
    availableTechniques nextTechnique, int32_t clauses) {
  techniqueClaues[nextTechnique] += clauses;
}

void Coprocessor2::techniqueChangedLiteralNumber(
    availableTechniques nextTechnique, int32_t literals) {
  techniqueLits[nextTechnique] += literals;
}

void Coprocessor2::techniqueSuccessEvent(availableTechniques nextTechnique) {
  techniqueEvents[nextTechnique]++;
}

void Coprocessor2::techniqueDoCheck(availableTechniques nextTechnique) {
  techniqueCheck[nextTechnique]++;
}
