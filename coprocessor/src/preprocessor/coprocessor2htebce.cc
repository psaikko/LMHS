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
        coprocessor2htebve.cc
        This file is part of riss.

        08.12.2011
        Copyright 2011 Norbert Manthey
*/

#include "preprocessor/coprocessor2.h"

Lit Coprocessor2::fillHlaArrays(Var v) {
  Lit* head, *tail;  // maintain the hla queue

  // create both the positive and negative array!
  for (uint32_t pol = 0; pol < 2; ++pol) {
    // fill for positive variable
    const Lit i = Lit(v, pol == 0 ? POS : NEG);
    MarkArray& hlaArray = (pol == 0) ? hlaPositive : hlaNegative;
    hlaArray.nextStep();

    hlaArray.setCurrentStep(i.toIndex());
    // process all literals in list (inverse BIG!)
    const vector<Lit>& posList = search.big.getAdjacencyList(i.complement());
    for (uint32_t j = 0; j < posList.size(); ++j) {
      const Lit imp = posList[j].complement();
      if (hlaArray.isCurrentStep(imp.toIndex())) continue;

      head = hlaQueue;
      tail = hlaQueue;
      *(head++) = imp;
      hlaArray.setCurrentStep(imp.toIndex());
      // process queue
      while (tail < head) {
        const Lit lit = *(tail++);
        hteLimit = (hteLimit > 0) ? hteLimit - 1 : 0;
        const vector<Lit>& kList =
            search.big.getAdjacencyList(lit.complement());
        for (uint32_t k = 0; k < kList.size(); ++k) {
          const Lit kLit = kList[k].complement();
          if (!hlaArray.isCurrentStep(kLit.toIndex())) {
            if (hlaArray.isCurrentStep(kLit.complement().toIndex())) {
              return i;  // return the failed literal
            }

            hlaArray.setCurrentStep(kLit.toIndex());
            *(head++) = kLit;
          }
        }
      }
      hlaArray.reset(imp.toIndex());
    }  // end for pos list
  }

  return NO_LIT;
}

bool Coprocessor2::hlaMarkClause(const CL_REF clause) {
  const Clause& cl = search.gsa.get(clause);

  Lit* head, *tail;  // indicators for the hla queue
  head = hlaQueue;
  tail = hlaQueue;
  // markArray with all literals of the clause
  for (uint32_t j = 0; j < cl.size(); ++j) {
    const Lit clLit = cl.get_literal(j);
    markArray.setCurrentStep(clLit.toIndex());  // mark current literal
    *(head++) = clLit;                          // add literal to the queue
  }

  if (cl.size() < 3)
    return false;  // do not work on binary and smaller clauses!

  while (tail < head) {
    const Lit lit = *(tail++);
    const vector<Lit>& jList = search.big.getAdjacencyList(lit.complement());

    for (uint32_t j = 0; j < jList.size(); ++j) {
      const Lit jLit = jList[j].complement();
      if (cl.size() == 2) {
        // do not remove the binary clause that is responsible for the current
        // edge
        if (lit == cl.get_literal(0) && jLit.complement() == cl.get_literal(1))
          continue;
        if (lit == cl.get_literal(1) && jLit.complement() == cl.get_literal(0))
          continue;
      }

      if (!markArray.isCurrentStep(jLit.toIndex())) {
        if (markArray.isCurrentStep(jLit.complement().toIndex())) {
          if (cl.size() == 2) {
            search.big.removeEdge(cl.get_literal(0).complement(),
                                  cl.get_literal(1));
            search.big.removeEdge(cl.get_literal(1).complement(),
                                  cl.get_literal(0));
          }
          return true;
        }
        markArray.setCurrentStep(jLit.toIndex());
        *(head++) = jLit;
      }
    }
  }

  return false;
}

bool Coprocessor2::hiddenTautologyElimination(Var v) {
  bool didSomething = false;
  // run for both polarities
  for (uint32_t pol = 0; pol < 2; ++pol) {
    // fill for positive variable
    Lit i = Lit(v, pol == 0 ? POS : NEG);
    MarkArray& hlaArray = (pol == 0) ? hlaPositive : hlaNegative;
    // iterate over binary clauses with occurences of the literal i
    const vector<CL_REF>& iList = list(i);
    // transitive reduction of BIG
    for (uint32_t k = 0; k < iList.size(); k++) {
      CL_REF clsidx = iList[k];
      Clause& cl = search.gsa.get(clsidx);
      if (cl.isIgnored()) continue;
      if (cl.size() == 2) {
        bool remClause = false;
        for (uint32_t j = 0; j < 2; j++) {
          // check for tautology
          const Lit clLit = cl.get_literal(j);
          if (hlaArray.isCurrentStep(clLit.complement().toIndex())) {
            const Lit literal0 = cl.get_literal(0);
            const Lit literal1 = cl.get_literal(1);
            search.big.removeClauseEdges(literal0, literal1);
            cl.markToDelete();
            techniqueChangedLiteralNumber(do_hte, 0 - (int32_t)cl.size());
            didSomething = true;
            remClause = true;
            break;
          } else {
          }
        }
        hteLimit = (hteLimit > 0) ? hteLimit - 1 : 0;  // limit
        // if clause has been removed from its lists, update!
        if (remClause) {
          techniqueChangedClauseNumber(do_hte, -1);
          updateRemoveClause(cl);
          removeClause(clsidx);
          k--;
        }
      }
    }

    // apply HTE to formula
    // set hla array for all the binary clauses
    const vector<Lit>& binaryI = search.big.getAdjacencyList(i.complement());
    for (uint32_t j = 0; j < binaryI.size(); j++) {
      hlaArray.setCurrentStep(binaryI[j].complement().toIndex());
    }

    for (uint32_t k = 0; k < iList.size(); k++) {
      CL_REF clsidx = iList[k];
      Clause& cl = search.gsa.get(clsidx);
      if (cl.isIgnored()) continue;
      bool ignClause = false;
      bool changed = false;
      if (cl.size() > 2) {
        hteLimit = (hteLimit > 0) ? hteLimit - 1 : 0;  // limit
        techniqueDoCheck(do_hte);                      // get some statistics
        for (uint32_t j = 0; j < cl.size(); j++) {
          const Lit clauseLiteral = cl.get_literal(j);
          if (hlaArray.isCurrentStep(clauseLiteral.complement().toIndex())) {
            didSomething = true;
            ignClause = true;
            techniqueChangedLiteralNumber(do_hte, 0 - (int32_t)cl.size());
            techniqueChangedClauseNumber(do_hte, -1);
            cl.markToDelete();  // TODO remove from occurence lists?
            break;
          } else if (clauseLiteral != i &&
                     hlaArray.isCurrentStep(clauseLiteral.toIndex())) {
            didSomething = true;
            // remove the clause from the occurence list of the literal that is
            // removed
            for (uint32_t l = 0; l < list(clauseLiteral).size(); l++) {
              if (list(clauseLiteral)[l] == clsidx) {
                removeListIndex(clauseLiteral, l);
                break;
              }
            }
            // remove the literal
            changed = true;
            updateRemoveLiteral(clauseLiteral);
            cl.remove_indexLin(j);
            techniqueChangedLiteralNumber(do_hte, -1);
            // update the index
            j--;
            if (cl.size() == 1) {
              if (!enqueTopLevelLiteral(cl.get_literal(0))) {
                solution = UNSAT;
                return didSomething;
              }
            } else if (cl.size() == 0) {
              solution = UNSAT;
              return didSomething;
            }
          }
        }
      }

      // delete the clause, update the index
      if (ignClause) {
        didSomething = true;
        removeClause(clsidx);
        updateRemoveClause(cl);
        k--;
      } else if (changed) {
        // update information about the final clause (EE,BIG,SUSI)
        updateChangeClause(clsidx);
      }
    }

  }  // end for pol
  return didSomething;
}

bool Coprocessor2::hiddenTautologyElimination(bool firstCall) {
  bool didSomething = false;
  techniqueStart(do_hte);
  if (firstCall) {
    reBuildBIG();
  }

  // as long as there are items in the heap (within limit)
  while (hteHeap.size() > 0) {
    if (!unlimited && hteLimit == 0) break;
    // pick random element?
    Var v =
        hteHeap.item_at(randomized ? (rand() % (hteHeap.size() + 1) / 2) : 0);
    hteHeap.remove_item(v);
    Lit unit = fillHlaArrays(v);
    if (unit != NO_LIT) {
      // only found something new, if unit was not set before
      didSomething = search.assignment.is_undef(unit.toVar());
      if (!enqueTopLevelLiteral(unit)) {
        solution = UNSAT;
        break;
      } else {
        // do not remove HTE clauses with a unit variable
        continue;
      }
    }
    hteLimit = (hteLimit > 0) ? hteLimit - 1 : 0;
    bool currentDidSomething = hiddenTautologyElimination(v);
    if (currentDidSomething) techniqueSuccessEvent(do_hte);
    didSomething = currentDidSomething || didSomething;
    if (fastLoop > 1 && didSomething) break;
  }
  techniqueStop(do_hte);
  if (didSomething) propagateTopLevel();
  return didSomething;
}

bool Coprocessor2::blockedClauseElimination(Var v, bool hla) {
  bool didSomething = false;
  // do not work on variables that are already assigned or are not allowed to be
  // touched
  if (!search.assignment.is_undef(v)) return didSomething;
  if (doNotTouch.get(v)) return didSomething;

  for (uint32_t pol = 0; pol < 2; ++pol) {
    const Lit l = Lit(v, pol == 0 ? POS : NEG);  // literal to work with

    vector<CL_REF>& positiveList = list(l);
    vector<CL_REF>& negativeList = list(l.complement());
    // TODO size limit on the number of clauses?
    // check every clause where this literal occurs in (within limit)
    for (uint32_t i = 0; i < positiveList.size() && (unlimited || bceLimit > 0);
         i++) {
      Clause& cl = search.gsa.get(positiveList[i]);
      if (cl.isIgnored()) continue;
      // if ( cl.isLearnt() ) continue; // do not remove learned clauses here!
      // TODO: apply size threshold
      markArray.nextStep();
      bool isSmallestVar = false;
      // check whether variable is least occuring variable in clause (to avoid
      // double checks)
      if (hla) {
        isSmallestVar = true;
        for (uint32_t i = 0; i < cl.size(); ++i) {
          if (cl.get_literal(i).toVar() == v) continue;
          if (varOcc[cl.get_literal(i).toVar()] < varOcc[v]) {
            isSmallestVar = false;
            break;
          }
        }
      }
      if (hla && isSmallestVar &&
          (unlimited || hbceLimit > 0)) {  // use HLA for this clause?
        hbceLimit = hbceLimit > 0 ? hbceLimit - 1
                                  : hbceLimit;  // take care of the limit
        if (hlaMarkClause(positiveList[i])) {
          cl.markToDelete();  // is HTE clause
          updateRemoveClause(cl);
          removeClause(positiveList[i]);
          techniqueChangedClauseNumber(do_hte, -1);
          techniqueChangedLiteralNumber(do_hte, 0 - (int32_t)cl.size());
          --i;
          didSomething = true;
          continue;
        }
        // now the HLA for the clause is marked with the current step count
      } else {
        // initialize resolution by marking each literal of the clause
        for (uint32_t j = 0; j < cl.size(); ++j) {
          const Lit clLit = cl.get_literal(j);
          markArray.setCurrentStep(clLit.toIndex());
        }
      }
      postEle pe;
      bool blocked = true;
      tmpLiterals.clear();
      // use current markArray to check for resolvents, abort if there is one
      // (no need to differentiate between clause and hla
      for (uint32_t j = 0; j < negativeList.size(); j++) {
        bceLimit = (bceLimit > 0) ? bceLimit - 1 : 0;  // reduce limit
        const Clause& ncl = search.gsa.get(negativeList[j]);
        if (ncl.isIgnored()) continue;
        techniqueDoCheck(do_bce);  // get some statistics
                                   // check whether clause is tautology
        bool clauseIsBlocked = false;
        for (uint32_t k = 0; k < ncl.size(); k++) {
          const Lit kLit = ncl.get_literal(k);
          if (kLit == l.complement())
            continue;  // do not consider resolution variable
          // there needs to be a single literal that is complementry!
          if (markArray.isCurrentStep(kLit.complement().toIndex())) {
            // tmpLiterals.push_back( kLit.complement() ); // not necessary
            // according to Marijn Heule
            goto nextResolventCandidate;
          }
        }
        // if there is no such complementary clause, the clause is not blocked
        // for the literal l
        goto nextBlockCandidate;
      nextResolventCandidate:
        ;
      }

      // found blocked clause ->add information to postStack
      blockedLearnt = cl.isLearnt() ? blockedLearnt + 1 : blockedLearnt;
      didSomething = true;
      pe.kind = 'b';
      pe.b.lit1 = l;
      {
#ifdef REGIONALLOCATOR
        const uint32_t newRef = postClauses.create(cl);
#else
        const uint32_t newRef = positiveList[i];
#endif
        pe.b.clause = newRef;
      }
      postProcessStack.push_back(pe);
      techniqueChangedClauseNumber(do_bce, -1);
      techniqueChangedLiteralNumber(do_bce, 0 - (int32_t)cl.size());
// remove clause, update index
#ifdef REGIONALLOCATOR
      cl.markToDelete();
#else
      cl.ignore();
#endif
      updateRemoveClause(cl);
      removeClause(positiveList[i]);
      if (cl.size() == 2) {
        search.big.removeClauseEdges(cl.get_literal(0), cl.get_literal(1));
      }

      i--;

    nextBlockCandidate:
      ;
    }
  }  // end for pol

  return didSomething;
}

bool Coprocessor2::blockedClauseElimination(bool firstCall) {
  bool didSomething = false;
  techniqueStart(do_bce);
  // as long as there are items in the heap
  while (bceHeap.size() > 0) {
    if (!unlimited && bceLimit == 0) break;
    Var v =
        bceHeap.item_at(randomized ? (rand() % (bceHeap.size() + 1) / 2) : 0);
    bceHeap.remove_item(v);
    bool currentDidSomething = blockedClauseElimination(v, hbce);
    if (currentDidSomething) techniqueSuccessEvent(do_bce);
    didSomething = currentDidSomething || didSomething;
    if (fastLoop > 1 && didSomething) break;
  }
  techniqueStop(do_bce);
  return didSomething;
}