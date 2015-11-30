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
        coprocessor2bve.cc
        This file is part of riss.

        08.12.2011
        Copyright 2011 Norbert Manthey
*/

#include "preprocessor/coprocessor2.h"

bool Coprocessor2::variableElimination(bool firstCall) {
  techniqueStart(do_bve);
  bool didSomething = false;

  // only for initialization
  if (firstCall) {
    bveTouchedVars.clear(search.var_cnt + 1);
    for (uint32_t i = 0; i < (*formula).size(); ++i) {
      // activate all variables
      Clause& cl = search.gsa.get((*formula)[i]);
      if (cl.isIgnored()) continue;
      for (uint32_t j = 0; j < cl.size(); ++j)
        bveTouchedVars.set(cl.get_literal(j).toVar(), true);
    }

    bveHeap.clear();
    // add all to queue, that do have an occurrence to be resolved
    for (Var v = 1; v <= search.var_cnt; ++v) {
      if (varOcc[v] > 0) bveHeap.insert_item(v);
    }
  }

  while ((subsumptionQueue.size() > 0 || !bveHeap.is_empty()) &&
         !isInterupted() && (unlimited || bveLimit > 0)) {
    // do subsumption
    selfSubsumption();
    if (solution != UNKNOWN) break;

    // apply propagation if set in parameter or if formula is simplified during
    // search
    propagateTopLevel();
    if (solution != UNKNOWN) break;

    while (!bveHeap.is_empty() && !isInterupted() &&
           (unlimited || bveLimit > 0)) {
      const Var variable =
          bveHeap.item_at(randomized ? rand() % ((bveHeap.size() + 1) / 2) : 0);
      bveHeap.remove_item(variable);

      bveLimit = (bveLimit > 0) ? bveLimit - 1 : 0;

      didSomething = resolveVar(variable) || didSomething;
      if (solution != UNKNOWN) {
        goto endBVE;
      }
      // TODO do fast loop here?
      if (didSomething && fastLoop > 2) goto endBVE;
    }
    // assure that there are no more possibilit1ies to subsume something
    // adding clauses here prevents from adding clauses multiple times
    addActiveBveClauses();
  }
endBVE:
  techniqueStop(do_bve);
  return didSomething;
}

void Coprocessor2::addActiveBveClauses() {
  // add all literals of clauses that are not ignored to heap
  // add clause to subsQ
  for (uint32_t i = 0; i < (*formula).size(); ++i) {
    Clause& cl = search.gsa.get((*formula)[i]);
    if (!cl.isIgnored()) {
      for (uint32_t j = 0; j < cl.size(); ++j) {
        if (bveTouchedVars.get(cl.get_literal(j).toVar())) {
          updateChangeClause((*formula)[i]);
          break;
        }
      }
    }
  }

  for (uint32_t v = 1; v <= search.var_cnt; ++v) {
    if (bveTouchedVars.get(v)) {
      if (bveHeap.contains(v))
        bveHeap.update_item(v);
      else
        bveHeap.insert_item(v);
    }
    bveTouchedVars.set(v, false);
  }
}

bool Coprocessor2::bveFindITEGate(Var x) {
  Lit ite[4];  // 4 literals of the ITE (x,s,f,t)
  Lit px = Lit(x, POS);

  for (uint32_t i = 0; i < list(px).size(); ++i) {
    const CL_REF ref = list(px)[i];
    const Clause& clause = search.gsa.get(ref);
    if (clause.isIgnored() || clause.size() != 3) continue;

    // choose x!
    uint32_t j = 0;
    for (; j < 3; ++j) {
      ite[0] = clause.get_literal(j);
      if (ite[0] != px) continue;
      // try to select a 's'
      uint32_t k = 0;
      for (; k < 3; k++) {
        if (k == j) continue;
        ite[1] = clause.get_literal(k);
        if (!ite[1].isPositive()) continue;
        ite[2] = clause.get_literal(3 - k - j)
                     .complement();  // remaining position -- currently the
                                     // variable is negated!
        // current clause is the clause [x,s,f] (by definition)

        // check whether a clause [x,s, -f] can be found!
        bool found = false;
        CL_REF fCand = 0;
        uint32_t fPosition = 0;
        for (uint32_t i2 = 0; i2 < list(ite[0].complement()).size(); ++i2) {
          const CL_REF ref2 = list(ite[0].complement())[i2];  // clause [s,f,-x]
          if (ref == ref2) continue;
          const Clause& clause2 = search.gsa.get(ref2);  // clause [s,f,-x]
          if (clause2.isIgnored() || clause2.size() > 3)
            continue;  // use only clauses of size 3! (size 2 is welcome as well
                       // (subsumed ITE gate))

          found = true;
          for (uint32_t j2 = 0; j2 < clause2.size(); ++j2)  // go through clause
          {
            const Lit checkL = clause2.get_literal(j2);
            uint32_t k2 =
                1;  // no need to check for -x in clauses of the list for -x!
            for (; k2 < 3; ++k2) {  // go through check list
              if (checkL == ite[k2]) break;
            }
            if (k2 == 3) {
              if (checkL != ite[0].complement()) {
                found = false;
                break;
              } else {
              }
            }
          }
          if (found) {
            fCand = ref2;
            fPosition = i2;
            break;
          }
        }

        if (!found) {
          continue;
        }
        // found gate! current clause represents [x,s,-f], current ite variable
        // represents [x,s,f, ? ]
        // try to find [-s,-t,x]

        found = false;

        for (uint32_t i2 = 0; i2 < list(ite[0].complement()).size(); ++i2) {
          const CL_REF ref4 = list(ite[0].complement())[i2];  // [-s,-t,x]
          assert(ref4 != fCand &&
                 "these two clauses cannot be in the same list, or there are "
                 "tautologies");
          const Clause& clause2 = search.gsa.get(ref4);  // [-s,-t,x]
          if (clause2.isIgnored() || clause2.size() != 3)
            continue;  // use only clauses of size 3!
          ite[3] = NO_LIT;
          uint32_t j2 = 0;
          for (; j2 < 3; ++j2) {
            const Lit checkL = clause2.get_literal(j2);
            if (checkL == ite[1]) {
              found = true;
              break;
            }  // check for -s
            else if (checkL != ite[0])
              ite[3] = checkL.complement();  // or x?
          }
          if (!found) continue;

          // found a t!
          if (ite[3] == NO_LIT) {
            for (; j2 < 3; ++j2) {
              const Lit checkL = clause2.get_literal(j2);
              if (checkL != ite[1].complement()) ite[3] = checkL.complement();
            }
          }
          assert(ite[3] != NO_LIT &&
                 "there has to be a third different literal in the candidate "
                 "clause for [-s,-t,x]");

          // do not consider the case where t is negative (this clause will be
          // found later!, symmetric!
          if (!ite[3].isPositive() || ite[3].toVar() == ite[1].toVar() ||
              ite[3].toVar() == ite[2].toVar() ||
              ite[3].toVar() == ite[0].toVar())
            continue;

          // try to find [-s,t,-x]

          // found => add ITEgate
          for (uint32_t i3 = 0; i3 < list(ite[0].complement()).size(); ++i3) {
            const CL_REF ref3 = list(ite[0].complement())[i3];  // [-s,t,-x]
            if (ref == ref3) continue;
            if (ref3 == ref4) continue;
            const Clause& clause2 = search.gsa.get(ref3);  // [-s,t,-x]
            if (clause2.isIgnored() || clause2.size() != 3)
              continue;  // use only clauses of size 3!
            found = false;
            for (uint32_t j2 = 0; j2 < 3; ++j2) {
              if (clause2.get_literal(j2) == ite[1].complement()) {
                found = true;
                break;
              }
            }
            if (!found) continue;
            found = false;
            for (uint32_t j2 = 0; j2 < 3; ++j2) {
              if (clause2.get_literal(j2) == ite[3].complement()) {
                found = true;
                break;
              }
            }
            if (!found) continue;

            assert(
                list(px.complement())[i2] == ref4 &&
                "the index of the clause in the list has still to be the same");
            assert(
                list(px.complement())[i3] == ref3 &&
                "the index of the clause in the list has still to be the same");
            assert(list(px)[i] == ref && "index in list has to be the same");
            assert(list(px)[fPosition] == fCand &&
                   "index in the list has to be the same");

            // move clauses to the front
            CL_REF t = list(px)[0];
            list(px)[0] = list(px)[i];
            list(px)[i] = t;
            t = list(px)[1];
            list(px)[1] = list(px)[fPosition];
            list(px)[fPosition] = t;
            t = list(px.complement())[0];
            list(px.complement())[0] = list(px.complement())[i2];
            list(px.complement())[i2] = t;
            t = list(px.complement())[1];
            list(px.complement())[1] = list(px.complement())[i3];
            list(px.complement())[i3] = t;
            return true;
            // break;
          }
        }

      }  // check s

    }  // check x
  }    // check clause

  return false;
}

bool Coprocessor2::bveFindGate(const Var variable, uint32_t& pos,
                               uint32_t& neg) {
  // do not touch lists that are too small for benefit
  if (list(Lit(variable, POS)).size() < 3 &&
      list(Lit(variable, NEG)).size() < 3)
    return false;
  if (list(Lit(variable, POS)).size() < 2 ||
      list(Lit(variable, NEG)).size() < 2)
    return false;

  for (uint32_t pn = 0; pn < 2; ++pn) {
    vector<CL_REF>& pList = list(Lit(variable, pn == 0 ? POS : NEG));
    vector<CL_REF>& nList = list(Lit(variable, pn == 0 ? NEG : POS));
    const Lit pLit = Lit(variable, pn == 0 ? POS : NEG);
    const Lit nLit = Lit(variable, pn == 0 ? NEG : POS);
    uint32_t& pClauses = pn == 0 ? pos : neg;
    uint32_t& nClauses = pn == 0 ? neg : pos;

    // check binary of pos variable
    markArray.nextStep();
    for (uint32_t i = 0; i < pList.size(); ++i) {
      const Clause& clause = search.gsa.get(pList[i]);
      if (clause.isIgnored() || clause.isLearnt() || clause.size() != 2)
        continue;  // do not use learned clauses for gate detection!
      Lit other = clause.get_literal(0) == pLit ? clause.get_literal(1)
                                                : clause.get_literal(0);
      markArray.setCurrentStep(other.complement().toIndex());
    }
    for (uint32_t i = 0; i < nList.size(); ++i) {
      const Clause& clause = search.gsa.get(nList[i]);
      if (clause.isIgnored() || clause.isLearnt()) continue;
      uint32_t j = 0;
      for (; j < clause.size(); ++j) {
        const Lit cLit = clause.get_literal(j);
        if (cLit == nLit)
          continue;  // do not consider variable that is to eliminate
        if (!markArray.isCurrentStep(cLit.toIndex())) break;
      }
      if (j == clause.size()) {
        assert(!clause.isIgnored() &&
               "a participating clause of the gate cannot be learned, because "
               "learned clauses will be removed completely during BVE");
        // setup values
        pClauses = clause.size() - 1;
        nClauses = 1;
        // do not add unnecessary clauses
        for (uint32_t k = 0; k < clause.size(); ++k)
          markArray.reset(clause.get_literal(k).toIndex());
        CL_REF tmp = nList[0];
        nList[0] = nList[i];
        nList[i] = tmp;  // swap responsible clause in list to front
        // swap responsible clauses in list to front
        uint32_t placedClauses = 0;
        for (uint32_t k = 0; k < pList.size(); ++k) {
          const Clause& clause = search.gsa.get(pList[k]);
          if (clause.isLearnt() || clause.isIgnored() || clause.size() != 2)
            continue;
          Lit other = clause.get_literal(0) == pLit ? clause.get_literal(1)
                                                    : clause.get_literal(0);
          if (!markArray.isCurrentStep(other.complement().toIndex())) {
            CL_REF tmp = pList[placedClauses];
            pList[placedClauses++] = pList[k];
            pList[k] = tmp;
            markArray.setCurrentStep(other.complement().toIndex());  // no need
                                                                     // to add
                                                                     // the same
                                                                     // binary
                                                                     // twice
          }
        }

        assert(pClauses == placedClauses &&
               "number of moved binary clauses and number of participating "
               "clauses has to be the same");
        return true;
      }
    }
  }

  // ITE gate recognition
  if (bveGateIte) {
    if (bveFindITEGate(variable)) {
      pos = 2;
      neg = 2;
      return true;
    }
  }

  // xor gate recognition
  if (bveGateXor) {
    // no need to sort if empty
    const Lit pLit = Lit(variable, POS);
    const Lit nLit = Lit(variable, NEG);
    sortListLength(pLit);
    sortListLength(nLit);

    // if there have been only ignored clauses, the list can be empty afterwards
    assert(list(pLit).size() != 0 && list(nLit).size() != 0 &&
           "there have to be clauses in the list");

    uint32_t pPos = 0, nPos = 0, cLength = 0;
    while (true) {
      if (pPos >= list(pLit).size() || nPos >= list(nLit).size()) break;
      cLength = search.gsa.get(list(pLit)[pPos]).size();
      // positions and length for next check are prepared here, limits?
      if (cLength > 10) break;
      if (cLength < 3) {
        pPos++;
        continue;
      }

      // check whether there are enough clauses of the specified lenght in pList
      // ( 2^(len-2), other half for other polarity )
      uint32_t shift = 1 << (cLength - 2);

      uint32_t lastPLenPos = pPos;
      for (; lastPLenPos < list(pLit).size(); lastPLenPos++) {
        const Clause& clause = search.gsa.get(list(pLit)[lastPLenPos]);
        if (clause.size() > cLength) {
          --lastPLenPos;
          break;
        }
      }
      lastPLenPos =
          (lastPLenPos == list(pLit).size()) ? lastPLenPos - 1 : lastPLenPos;
      uint32_t diff = lastPLenPos - pPos + 1;
      // not enough clauses of the same length, proceed with the next length
      if (diff < shift) {
        pPos = lastPLenPos + 1;
        continue;
      }
      for (; nPos < list(nLit).size(); nPos++) {
        const Clause& clause = search.gsa.get(list(nLit)[nPos]);
        if (clause.size() >= cLength) break;
      }
      // continue, if there are no clauses of the required length in the
      // negative clauses
      if (search.gsa.get(list(nLit)[nPos]).size() != cLength) {
        pPos = lastPLenPos + 1;
        continue;
      }
      uint32_t lastNLenPos = nPos;
      for (; lastNLenPos < list(nLit).size(); lastNLenPos++) {
        const Clause& clause = search.gsa.get(list(nLit)[lastNLenPos]);
        if (clause.size() > cLength) {
          --lastNLenPos;
          break;
        }
      }
      lastNLenPos =
          (lastNLenPos == list(nLit).size()) ? lastNLenPos - 1 : lastNLenPos;
      diff = lastNLenPos - nPos + 1;
      // not enough clauses of the same length, proceed with the next length
      if (diff < shift) {
        pPos = lastPLenPos + 1;
        continue;
      }
      // apply xor detection for the current length and subset of the occurrence
      // lists
      tmpClauses.clear();
      for (uint32_t i = pPos; i <= lastPLenPos; ++i) {
        search.gsa.get(list(pLit)[i]).sort();
        assert(search.gsa.get(list(pLit)[i]).size() == cLength);
        tmpClauses.push_back(list(pLit)[i]);
      }
      for (uint32_t i = nPos; i <= lastNLenPos; ++i) {
        search.gsa.get(list(nLit)[i]).sort();
        assert(search.gsa.get(list(nLit)[i]).size() == cLength);
        tmpClauses.push_back(list(nLit)[i]);
      }
      shift = shift << 1;  // now, consider the full set of clauses
      uint32_t candidates = 2 + lastNLenPos - nPos + lastPLenPos - pPos - shift;
      assert(candidates < (lastNLenPos + lastPLenPos) &&
             "there cannot be more candidates than clauses");
      sortClauseList(tmpClauses);
      CL_REF candidateBuffer[tmpClauses.size()];
      for (uint32_t currentCandidate = 0; currentCandidate <= candidates;
           ++currentCandidate) {
        const CL_REF c = tmpClauses[currentCandidate];
        const Clause& cl = search.gsa.get(c);
        markArray.nextStep();  // mark all variables of the current clause to
                               // detect how many clauses with the same
                               // variables are there
        for (uint32_t j = 0; j < cl.size(); ++j) {
          markArray.setCurrentStep(cl.get_literal(j).toVar());
        }
        uint32_t stop = currentCandidate + 1;
        // find last clause that contains the same variables as the first clause
        for (; stop < tmpClauses.size(); ++stop) {
          const CL_REF c2 = tmpClauses[stop];
          const Clause& cl2 = search.gsa.get(c2);
          uint32_t k = 0;
          for (; k < cl2.size(); k++) {
            if (!markArray.isCurrentStep(cl2.get_literal(k).toVar())) break;
          }
          if (k != cl2.size()) break;
        }
        // do only consider sets of clauses, that contain enough clauses to be
        // an xor
        if (stop - currentCandidate < shift) {
          currentCandidate = stop;
          continue;
        }
        // count all clauses between currentCandidate and stop, whether they
        // have:
        // odd/even number of literals (depending on first clause!)
        uint32_t odd = 0;
        uint32_t count = 0;
        // create odd number of original clause
        for (uint32_t j = 0; j < cl.size(); ++j) {
          if (cl.get_literal(j).getPolarity() == NEG) odd = odd ^ 1;
        }
        candidateBuffer[count++] = c;
        // check all clauses with the same variables
        for (uint32_t j = currentCandidate + 1; j < stop; ++j) {
          const CL_REF c2 = tmpClauses[j];
          const Clause& cl2 = search.gsa.get(c2);
          uint32_t o = 0;
          for (uint32_t k = 0; k < cl2.size(); k++)
            if (cl2.get_literal(k).getPolarity() == NEG) o = o ^ 1;
          if (o == odd) candidateBuffer[count++] = c2;
        }

        if (count == shift) {
          // found xor with the given size
          pos = 0;
          neg = 0;
          for (uint32_t i = 0; i < count; ++i) {
            const CL_REF sRef = candidateBuffer[i];
            // move participating clauses to the front in the two lists
            for (uint32_t pn = 0; pn < 2; ++pn) {
              vector<CL_REF>& clList = list(pn == 0 ? pLit : nLit);
              uint32_t& position = (pn == 0) ? pos : neg;
              for (uint32_t li = position; li < clList.size(); ++li) {
                // if element has been found, there is no need to do the second
                // iteration of the outer for loop, thus, use goto
                if (clList[li] == sRef) {
                  clList[li] = clList[position];
                  clList[position++] = sRef;
                  goto nextSrefSearch;
                }
              }
            }
          nextSrefSearch:
            ;
          }

          assert(pos == neg && pos == shift >> 1 &&
                 "number of clauses have to be distributed evenly for both "
                 "polarities and have to be the exact amount");
          return true;
        }
        // set pointer to the next clause that does not contain the same
        // variables!!
        currentCandidate = stop;
      }  // end (there are enough candidates)
      // setup next clause to jump to
      pPos = lastPLenPos + 1;
    }  // end while true
  }    // end if xor gate
  return false;
}

bool Coprocessor2::resolveVar(const Var variable, int force) {
  bool didSomething = false;
  // variable already set to some value?
  if (!search.assignment.is_undef(variable)) return didSomething;
  if (search.eliminated.get(variable)) return didSomething;
  // variable is not allowed to be altered?
  if (doNotTouch.get(variable)) return didSomething;

  vector<CL_REF>& clauses_pos = list(Lit(variable, POS));
  vector<CL_REF>& clauses_neg = list(Lit(variable, NEG));
  uint32_t cores = 0;
  uint32_t total = 0;
  uint32_t totalLiterals = 0;
  uint32_t newLiterals = 0;
  uint32_t maxPositiveClauseSize = 0;
  uint32_t maxNegativeClauseSize = 0;
  uint32_t posClauses = 0, negClauses = 0;
  for (uint32_t i = 0; i < clauses_pos.size(); i++) {
    const Clause& c = search.gsa.get(clauses_pos[i]);
    if (c.isIgnored())
      continue;  // { removeListIndex( Lit(variable,POS),i); -- i ; }
    else {
      if (c.isLearnt()) continue;
      posClauses++;
      totalLiterals += c.size();
      maxPositiveClauseSize = maxPositiveClauseSize >= c.size()
                                  ? maxPositiveClauseSize
                                  : c.size();  // store the biggest clause size
    }
  }
  for (uint32_t i = 0; i < clauses_neg.size(); i++) {
    const Clause& c = search.gsa.get(clauses_neg[i]);
    if (c.isIgnored())
      continue;
    else {
      if (c.isLearnt()) continue;
      negClauses++;
      totalLiterals += c.size();
      maxNegativeClauseSize = maxNegativeClauseSize >= c.size()
                                  ? maxNegativeClauseSize
                                  : c.size();  // store the biggest clause size
    }
  }

  total = negClauses + posClauses;
  uint32_t newCls = 0;  // number of generated resolvents

  uint32_t pSize = clauses_pos.size();
  uint32_t nSize = clauses_neg.size();
  if (posClauses > 0 && negClauses > 0) {

    bool foundGate = false;
    if (bveGate) {
      // gate extraction, based on binary clauses of variable
      foundGate = bveFindGate(variable, pSize, nSize);
    }

    uint32_t negCount[clauses_neg.size()];  // count how many resolvents are
                                            // generated by a clause with
                                            // negative variable occurrence
    memset(negCount, 0, sizeof(uint32_t) * negClauses);

    bool posClause_is_core = false;
    bool negClause_is_core = false;
    bool resolvesInOneCore = false;

    for (uint32_t i = 0; i < clauses_pos.size(); i++) {
      Clause& c1 = search.gsa.get(clauses_pos[i]);

      if (c1.isIgnored() || c1.isLearnt()) continue;

      posClause_is_core = true;

      // use step array to mark literals from current clause
      uint32_t c1_lit_index = 0;  // index of the resolution variable in c1
      markArray.nextStep();
      for (uint32_t j = 0; j < c1.size(); ++j) {
        const Lit l = c1.get_literal(j);
        if (l.toVar() == variable) {
          c1_lit_index = j;
          continue;
        }  // ignore resolve variable (store index)
        if (!doNotTouch.get(l.toVar())) {
          posClause_is_core = false;
        }
        markArray.setCurrentStep(l.toIndex());
      }

      // iterate over clauses with negative variable
      uint32_t beforeNewCls =
          newCls;  // store backup value for to be able to undo count quickly
      for (uint32_t j = 0; j < clauses_neg.size(); j++) {
        if (i > pSize && j > nSize)
          break;  // there is no need to resolve clauses that are not in the
                  // gate definition (SatElite paper)
        techniqueDoCheck(do_bve);  // get some statistics
        // TODO: handle ignored here?
        Clause& c2 = search.gsa.get(clauses_neg[j]);
        if (c2.isIgnored() || c2.isLearnt()) continue;

        negClause_is_core = true;

        uint32_t c2_lit_index = 0;
        // use mark array to search for complementary literal
        bool complementLiteral = false;
        uint32_t hitLiterals = 0;
        for (uint32_t k = 0; k < c2.size(); k++) {
          const Lit l = c2.get_literal(k);
          if (l.toVar() == variable) {
            c2_lit_index = k;
            continue;
          }  // ignore resolve variable (store index)
          if (markArray.isCurrentStep(l.complement().toIndex())) {
            complementLiteral = true;
            break;
          } else
            hitLiterals = markArray.isCurrentStep(l.toIndex())
                              ? hitLiterals + 1
                              : hitLiterals;  // count literals that appear in
                                              // both clauses
          if (!doNotTouch.get(l.toVar())) {
            negClause_is_core = false;
          }
        }

        if (posClause_is_core && negClause_is_core) {
          resolvesInOneCore = true;
          cores++;
        }

        if (!complementLiteral) {  //  there is a non core resolvent
          if (bveOTFSS) {          // on the fly self subsumption?
            if (hitLiterals + 1 == c1.size() &&
                (!foundGate || j > nSize)) {  // c2 is subsumed by resolvent
              c2.remove_index(c2_lit_index);
              // update counter and list index
              total--;
              newCls -= negCount[j];
              negCount[j] =
                  negCount[clauses_neg.size() - 1];  // move right value into
                                                     // the place that will be
                                                     // deleted!
              updateRemoveLiteral(Lit(variable, NEG));
              updateChangeClause(clauses_neg[j]);
              removeListIndex(Lit(variable, NEG), j);
              if (c2.size() == 1)
                if (!enqueTopLevelLiteral(c2.get_literal(0))) solution = UNSAT;
              j--;
              continue;
            } else if (hitLiterals + 1 == c2.size() &&
                       (!foundGate ||
                        i > pSize)) {  // c1 is subsumed by the resolvent
              c1.remove_index(c1_lit_index);
              updateRemoveLiteral(Lit(variable, POS));
              updateChangeClause(clauses_pos[i]);
              removeListIndex(Lit(variable, POS), i);
              if (c1.size() == 1)
                if (!enqueTopLevelLiteral(c1.get_literal(0))) solution = UNSAT;
              // update counter and list index
              newCls = beforeNewCls;
              total--;
              i--;
              goto bveNextC1;
            }
          }
          ++newCls;
          newLiterals += c1.size() + c2.size() - hitLiterals - 2;
          negCount[j]++;
        }
        // if too much, stop analysis. TODO: but check clauses that have been
        // analyzed so far whether they are blocked ?
        if ((bveShrink != 1) && newCls > total && force == 0) {
          if (resolvesInOneCore) missedCores += cores;
          return didSomething;
        }
        if ((bveShrink != 0) && newLiterals > totalLiterals && force == 0) {
          if (resolvesInOneCore) missedCores += cores;
          return didSomething;
        }
      }  // end for c2
    bveNextC1:
      ;
    }

  } else {  // end if( posClauses > 0 && negClauses > 0 )
    assert((posClauses == 0 || negClauses == 0) &&
           "at this point, one of the two polarities cannot be in the formula");
    newCls = posClauses + negClauses;  // at least one of them is 0!
  }
  addedCores += cores;

  if (!mod_cores) mod_cores = newCls + cores > total;

  techniqueSuccessEvent(do_bve);
  techniqueChangedLiteralNumber(do_bve,
                                (int32_t)newLiterals - (int32_t)totalLiterals);
  // apply resolution!
  Lit resolventLiterals
      [maxPositiveClauseSize + maxNegativeClauseSize];  // storage for resolvent
  uint32_t resolventSize = 0;  // number of literals in resolvent
  const uint32_t oldFormulaSize = (*formula).size();
  for (uint32_t i = 0; i < clauses_pos.size(); i++) {
    const Clause& c = search.gsa.get(clauses_pos[i]);
    if (c.isIgnored() || c.isLearnt()) continue;
    resolventSize = 0;
    // use markArray
    markArray.nextStep();
    for (uint32_t j = 0; j < c.size(); ++j) {
      const Lit l = c.get_literal(j);
      if (l.toVar() == variable) continue;
      markArray.setCurrentStep(l.toIndex());
      resolventLiterals[resolventSize++] = l;
    }
    const uint32_t c1Size = resolventSize;

    for (uint32_t j = 0; j < clauses_neg.size(); j++) {
      if (i > pSize && j > nSize)
        continue;  // there is no need to resolve clauses that are not in the
                   // gate definition (SatElite paper)
      const Clause& c2 = search.gsa.get(clauses_neg[j]);
      if (c2.isIgnored() || c2.isLearnt()) continue;
      resolventSize = c1Size;
      bool complementLiteral = false;
      for (uint32_t k = 0; k < c2.size(); ++k) {
        const Lit l = c2.get_literal(k);
        if (l.toVar() == variable) continue;
        if (markArray.isCurrentStep(l.complement().toIndex())) {
          complementLiteral = true;
          break;
        }
        // add only new variables
        if (!markArray.isCurrentStep(l.toIndex()))
          resolventLiterals[resolventSize++] = l;
      }
      // there is a resolvent, add it to the formula
      if (!complementLiteral) {
        CL_REF resolve_clause =
            search.gsa.create(resolventLiterals, resolventSize, false);
        (*formula).push_back(resolve_clause);
      }
    }
  }  // end for c1 in clauses_pos
  didSomething = true;
  // update structures
  search.eliminated.set(variable, true);
  uint32_t ci = 0;
  postEle elimination;

  if (force < 2) {
    elimination.kind = 'v';
    elimination.e.var1 = variable;
    elimination.e.number = total;
    elimination.e.clauses = (CL_REF*)malloc(total * sizeof(CL_REF));
  }
  for (uint32_t i = 0; i < clauses_pos.size(); ++i) {
    const CL_REF clause = clauses_pos[i];
    Clause& icl = search.gsa.get(clause);
    if (icl.isIgnored()) continue;
#ifdef REGIONALLOCATOR
    icl.markToDelete();
#else
    icl.ignore();
#endif
    updateRemoveClause(icl);
    if (icl.size() == 2)
      search.big.removeClauseEdges(icl.get_literal(0), icl.get_literal(1));
    if (icl.isLearnt()) continue;  // also ignore learned clauses!
                                   // use preprocessor intern clause storage
#ifdef REGIONALLOCATOR
    const uint32_t newRef = postClauses.create(icl);
#else
    const uint32_t newRef = clause;
#endif
    if (force < 2) elimination.e.clauses[ci++] = newRef;
  }
  while (!clauses_pos.empty()) removeClause(clauses_pos[0]);
  for (uint32_t i = 0; i < clauses_neg.size(); ++i) {
    const CL_REF clause = clauses_neg[i];
    Clause& icl = search.gsa.get(clause);
    if (icl.isIgnored()) continue;
#ifdef REGIONALLOCATOR
    icl.markToDelete();
#else
    icl.ignore();
#endif
    updateRemoveClause(icl);
    if (icl.size() == 2)
      search.big.removeClauseEdges(icl.get_literal(0), icl.get_literal(1));
    if (icl.isLearnt()) continue;  // also ignore learned clauses!

#ifdef REGIONALLOCATOR
    const uint32_t newRef = postClauses.create(icl);
#else
    const uint32_t newRef = clause;
#endif
    if (force < 2) elimination.e.clauses[ci++] = newRef;
  }
  while (!clauses_neg.empty()) removeClause(clauses_neg[0]);

  techniqueChangedClauseNumber(do_bve, -(int32_t)ci);
  techniqueChangedClauseNumber(do_bve, (*formula).size() - oldFormulaSize);
  // update new clauses
  for (uint32_t i = oldFormulaSize; i < (*formula).size(); ++i) {
    const CL_REF clause = (*formula)[i];
    if (!addClause(clause, true)) {
      solution = UNSAT;
      return didSomething;
    }  // subsumption queue, bveHeap, lists, occs, heaps
  }

  // store the right number in the field
  if (force < 2) {
    elimination.e.number = total;
    postProcessStack.push_back(elimination);
  }

  if (force > 2)
    return didSomething;
  else
    return selfSubsumption() || didSomething;
}
