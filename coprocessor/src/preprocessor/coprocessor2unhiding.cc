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
        coprocessor2unhiding.cc
        This file is part of riss.

        18.01.2011
        Copyright 2011 Norbert Manthey
*/

#include "preprocessor/coprocessor2.h"

void shuffleVector(vector<Lit>& adj) {
  const uint32_t sz = adj.size();
  for (uint32_t i = 0; i + 1 < sz; ++i) {
    const uint32_t rnd = rand() % sz;
    const Lit tmp = adj[i];
    adj[i] = adj[rnd];
    adj[rnd] = tmp;
  }
}
// uhdNoShuffle

uint32_t Coprocessor2::linStamp(const Lit literal, uint32_t stamp,
                                bool& detectedEE) {
  int32_t level =
      0;  // corresponds to position of current literal in stampQueue

  // lines 1 to 4 of paper
  stamp++;
  stampInfo[literal.toIndex()].dsc = stamp;
  stampInfo[literal.toIndex()].obs = stamp;
  // bool flag = true;
  vector<Lit>& adj = search.big.getAdjacencyList(literal);
  if (!uhdNoShuffle) shuffleVector(adj);
  stampEE.push_back(literal);
  stampQueue.push_back(literal);
  level++;
  while (level >= unhideEEflag.size()) unhideEEflag.push_back(1);
  unhideEEflag[level] = 1;
  // end lines 1-4 of paper

  while (!stampQueue.empty()) {
    const Lit l = stampQueue.back();
    const vector<Lit>& adj = search.big.getAdjacencyList(l);
    // recover index, if some element in the adj of l has been removed by
    // transitive edge reduction
    if (adj.size() > 0 && stampInfo[l.toIndex()].lastSeen != NO_LIT &&
        stampInfo[l.toIndex()].index > 0 &&
        adj[stampInfo[l.toIndex()].index - 1] !=
            stampInfo[l.toIndex()].lastSeen) {
      for (stampInfo[l.toIndex()].index = 0;
           stampInfo[l.toIndex()].index < adj.size();
           stampInfo[l.toIndex()].index++) {
        if (adj[stampInfo[l.toIndex()].index] ==
            stampInfo[l.toIndex()].lastSeen)
          break;
      }
      stampInfo[l.toIndex()].index++;
      assert(adj[stampInfo[l.toIndex()].index - 1] ==
             stampInfo[l.toIndex()].lastSeen);
    }

    for (; stampInfo[l.toIndex()].index < adj.size();) {

      const Lit l1 = adj[stampInfo[l.toIndex()].index];
      ++stampInfo[l.toIndex()].index;

      if (uhdTransitive &&
          stampInfo[l.toIndex()].dsc < stampInfo[l1.toIndex()].obs) {
        removeClause(l.complement(), l1, true);
        search.big.removeClauseEdges(l.complement(), l1);
        --stampInfo[l.toIndex()].index;
        continue;
      }

      if (stampInfo[stampInfo[l.toIndex()].root.toIndex()].dsc <=
          stampInfo[l1.complement().toIndex()].obs) {
        Lit lfailed = l;
        while (stampInfo[lfailed.toIndex()].dsc >
               stampInfo[l1.complement().toIndex()].obs)
          lfailed = stampInfo[lfailed.toIndex()].parent;
        if (!enqueTopLevelLiteral(lfailed.complement())) {
          solution = UNSAT;
          return stamp;
        }

        if (stampInfo[l1.complement().toIndex()].dsc != 0 &&
            stampInfo[l1.complement().toIndex()].fin == 0)
          continue;
      }

      if (stampInfo[l1.toIndex()].dsc == 0) {
        stampInfo[l.toIndex()].lastSeen =
            l1;  // set information if literals are pushed back
        stampInfo[l1.toIndex()].parent = l;
        stampInfo[l1.toIndex()].root = stampInfo[l.toIndex()].root;
        /*
        stamp = recStamp( l1, stamp, detectedEE );
        */
        // lines 1 to 4 of paper
        stamp++;
        stampInfo[l1.toIndex()].dsc = stamp;
        stampInfo[l1.toIndex()].obs = stamp;
        assert(stampInfo[l1.toIndex()].index == 0 &&
               "if the literal is pushed on the queue, it should not been "
               "visited already");
        // do not set index or last seen, because they could be used somewhere
        // below in the path already!
        vector<Lit>& adj = search.big.getAdjacencyList(l1);
        if (!uhdNoShuffle) shuffleVector(adj);
        stampEE.push_back(l1);
        stampQueue.push_back(l1);
        level++;
        while (level >= unhideEEflag.size()) unhideEEflag.push_back(1);
        unhideEEflag[level] = 1;
        // end lines 1-4 of paper

        goto nextUHDqueueElement;
      }

      // detection of EE is moved to below!
      if (level > 0) {  // if there are still literals on the queue
        if (uhdEE && stampInfo[l1.toIndex()].fin == 0 &&
            stampInfo[l1.toIndex()].dsc < stampInfo[l.toIndex()].dsc &&
            !doNotTouch.get(l.toVar()) && !doNotTouch.get(l1.toVar())) {
          stampInfo[l.toIndex()].dsc = stampInfo[l1.toIndex()].dsc;
          unhideEEflag[level] = 0;
          detectedEE = true;
        }
      }
      stampInfo[l.toIndex()].obs = stamp;
    }

    if (stampInfo[l.toIndex()].index >= adj.size()) {
      // do EE detection on current level
      stampClassEE.clear();
      Lit l1 = NO_LIT;
      if (unhideEEflag[level] == 1 || level == 1) {
        stamp++;
        do {
          l1 = stampEE.back();
          stampEE.pop_back();
          stampClassEE.push_back(l1);
          stampInfo[l1.toIndex()].dsc = stampInfo[l.toIndex()].dsc;
          stampInfo[l1.toIndex()].fin = stamp;
        } while (l1 != l);
        if (stampClassEE.size() > 1) {
          // also collect all the literals from the current path
          addEquis(stampClassEE);
          detectedEE = true;
        }
      }

      // do detection for EE on previous level

      stampQueue.pop_back();  // pop the current literal
      level--;
      if (level > 0) {  // if there are still literals on the queue
        Lit parent = stampQueue.back();
        assert(
            parent == stampInfo[l.toIndex()].parent &&
            "parent of the node needs to be previous element in stamp queue");

        // do not consider do-not-touch variables for being equivalent!
        if (uhdEE && stampInfo[l.toIndex()].fin == 0 &&
            stampInfo[l.toIndex()].dsc < stampInfo[parent.toIndex()].dsc &&
            !doNotTouch.get(l.toVar()) && !doNotTouch.get(parent.toVar())) {
          stampInfo[parent.toIndex()].dsc = stampInfo[l.toIndex()].dsc;
          unhideEEflag[level] = 0;
          detectedEE = true;
        }
      }
      stampInfo[l.toIndex()].obs = stamp;
    } else {
      assert(false);
    }

  nextUHDqueueElement:
    ;
  }

  assert(level == 0 && "after leaving the sub tree, the level has to be 0");

  return stamp;
}

uint32_t Coprocessor2::recStamp(const Lit l, uint32_t stamp, bool& detectedEE) {
  stamp++;
  stampInfo[l.toIndex()].dsc = stamp;
  stampInfo[l.toIndex()].obs = stamp;
  bool flag = true;
  const vector<Lit>& adj = search.big.getAdjacencyList(l);
  stampEE.push_back(l);

  for (uint32_t i = 0; i < adj.size(); ++i) {
    const Lit l1 = adj[i];
    if (stampInfo[l.toIndex()].dsc < stampInfo[l1.toIndex()].obs) {
      removeClause(l.complement(), l1, true);
      search.big.removeClauseEdges(l.complement(),
                                   l1);  // automatically done by removeClause
      i--;
      continue;
    }

    if (stampInfo[stampInfo[l.toIndex()].root.toIndex()].dsc <=
        stampInfo[l1.complement().toIndex()].obs) {
      Lit lfailed = l;
      while (stampInfo[lfailed.toIndex()].dsc >
             stampInfo[l1.complement().toIndex()].obs)
        lfailed = stampInfo[lfailed.toIndex()].parent;
      if (!enqueTopLevelLiteral(lfailed.complement())) {
        solution = UNSAT;
        return stamp;
      }

      if (stampInfo[l1.complement().toIndex()].dsc != 0 &&
          stampInfo[l1.complement().toIndex()].fin == 0)
        continue;
    }

    if (stampInfo[l1.toIndex()].dsc == 0) {
      stampInfo[l1.toIndex()].parent = l;
      stampInfo[l1.toIndex()].root = stampInfo[l.toIndex()].root;
      stamp = recStamp(l1, stamp, detectedEE);
    }

    if (stampInfo[l1.toIndex()].fin == 0 &&
        stampInfo[l1.toIndex()].dsc < stampInfo[l.toIndex()].dsc) {
      stampInfo[l.toIndex()].dsc = stampInfo[l1.toIndex()].dsc;
      flag = false;
    }

    stampInfo[l1.toIndex()].obs = stamp;
  }

  stampClassEE.clear();
  Lit l1 = NO_LIT;
  if (flag) {
    stamp++;
    do {
      l1 = stampEE.back();
      stampEE.pop_back();
      stampClassEE.push_back(l1);
      stampInfo[l1.toIndex()].dsc = stampInfo[l.toIndex()].dsc;
      stampInfo[l1.toIndex()].fin = stamp;
    } while (l1 != l);
    if (stampClassEE.size() > 1) {
      addEquis(stampClassEE);
    }
  }
  return stamp;
}

uint32_t Coprocessor2::stampLiteral(const Lit literal, uint32_t stamp,
                                    bool& detectedEE) {
  stampQueue.clear();
  stampEE.clear();
  unhideEEflag.clear();

  // do nothing if a literal should be stamped again, but has been stamped
  // already
  if (stampInfo[literal.toIndex()].dsc != 0) return stamp;

  // for debugging
  if (advancedStamping) {
    return linStamp(literal, stamp, detectedEE);
  }

  // linearized algorithm from paper
  stamp++;

  // handle initial literal before putting it on queue
  assert(stampInfo[literal.toIndex()].dsc == 0 &&
         " a literal cannot be stamped twice");
  stampInfo[literal.toIndex()].dsc =
      stamp;  // parent and root are already set to literal
  assert(stampInfo[literal.toIndex()].obs == 0 &&
         " a literal cannot be already observed if it is the root for a stamp "
         "tree");
  stampInfo[literal.toIndex()].obs = stamp;  // observe the literal right now
  stampQueue.push_back(literal);

  shuffleVector(search.big.getAdjacencyList(literal));
  stampInfo[literal.toIndex()].index = 0;

  tmpLiterals.clear();

  if (advancedStamping) {

    stampEE.push_back(literal);
    unhideEEflag.push_back(1);

    int32_t level = 0;
    while (!stampQueue.empty()) {
      const Lit current = stampQueue.back();
      const vector<Lit>& adj = search.big.getAdjacencyList(current);
      // processed all implied literals of the current literal, thus, return
      // from the deeper level and continue on this level with the remaining
      // checks
      if (stampInfo[current.toIndex()].index == adj.size()) {

        // continue on previous level
        stampQueue.pop_back();

        // line 19 - 25 of the paper
        if (unhideEEflag[level] == 1 || level == 0) {
          stampClassEE.clear();
          stamp++;
          Lit impliedLit = stampEE[stampEE.size() - 1];
          do {
            impliedLit = stampEE[stampEE.size() - 1];
            stampEE.pop_back();
            stampClassEE.push_back(impliedLit);
            stampInfo[impliedLit.toIndex()].dsc =
                stampInfo[current.toIndex()].dsc;
            stampInfo[impliedLit.toIndex()].fin = stamp;
          } while (impliedLit != current);

          if (stampClassEE.size() > 1) {
            addEquis(stampClassEE);
          }
        }

        level--;
        if (level < 0) {
          return stamp;
        }

      } else {
        uint32_t& index = stampInfo[current.toIndex()].index;
        const Lit impliedLit =
            search.big.getAdjacencyList(current)[index];  // is l' in paper
        index++;  // for -l,l' in F2 (continued)
        // line 6 TRD of paper:
        if (stampInfo[current.toIndex()].dsc <
            stampInfo[impliedLit.toIndex()].obs) {
          // remove binary clause and edge
          removeClause(current.complement(), impliedLit, true);
          // search.big.removeClauseEdges(current.complement(), impliedLit); //
          // automatically done by removeClause
          continue;
        }

        // line 7 - 11 of paper (FLE)
        if (stampInfo[stampInfo[current.toIndex()].root.toIndex()].dsc <=
            stampInfo[impliedLit.complement().toIndex()].obs) {
          // failed literal
          Lit failed = current;
          while (stampInfo[failed.toIndex()].dsc >
                 stampInfo[impliedLit.complement().toIndex()].obs)
            failed = stampInfo[failed.toIndex()].parent;
          if (!enqueTopLevelLiteral(failed.complement())) {
            solution = UNSAT;
            return stamp;
          }
          // proceed with next literal?
          if (stampInfo[impliedLit.complement().toIndex()].dsc != 0 &&
              stampInfo[impliedLit.complement().toIndex()].fin == 0)
            continue;
        }
        // line 12 - 15 of paper
        if (stampInfo[impliedLit.toIndex()].dsc == 0) {
          stamp++;
          stampInfo[impliedLit.toIndex()].root =
              stampInfo[current.toIndex()].root;
          stampInfo[impliedLit.toIndex()].parent = current;
          stampInfo[impliedLit.toIndex()].dsc = stamp;
          stampInfo[impliedLit.toIndex()].obs = stamp;
          stampInfo[impliedLit.toIndex()].index = 0;
          stampEE.push_back(impliedLit);
          stampQueue.push_back(impliedLit);
          shuffleVector(search.big.getAdjacencyList(impliedLit));
          // proceed on next higher level
          level++;
          assert(unhideEEflag.size() + 1 >= level &&
                 " there has to be enough space in the vector (already)");
          while (level >= unhideEEflag.size()) unhideEEflag.push_back(1);
          unhideEEflag[level] = 1;
          // push, and emulate recursive call
          continue;
        }

        // line 16-17 of the paper
        // check, whether flag has to be disabled for this level
        if (level > 1) {
          if (stampInfo[impliedLit.toIndex()].fin == 0 &&
              stampInfo[impliedLit.toIndex()].dsc <
                  stampInfo[current.toIndex()].dsc) {
            // l is equivalent to l'
            stampInfo[current.toIndex()].dsc =
                stampInfo[impliedLit.toIndex()].dsc;
            // set flags in a way that the implied literal collects all
            // equivalent literals!
            unhideEEflag[level] = 0;
            detectedEE = true;
          }
        }

        // line 18 of the paper
        stampInfo[impliedLit.toIndex()].obs = stamp;
      }
    }
  } else {

    while (!stampQueue.empty()) {
      const Lit current = stampQueue.back();

      const vector<Lit>& adj = search.big.getAdjacencyList(current);

      if (stampInfo[current.toIndex()].index == adj.size()) {
        stampQueue.pop_back();
        stamp++;
        stampInfo[current.toIndex()].fin = stamp;
      } else {
        uint32_t& index = stampInfo[current.toIndex()].index;
        const Lit impliedLit = search.big.getAdjacencyList(current)[index];
        index++;
        if (stampInfo[impliedLit.toIndex()].dsc != 0) continue;

        stamp++;
        stampInfo[impliedLit.toIndex()].root =
            stampInfo[current.toIndex()].root;
        stampInfo[impliedLit.toIndex()].parent = current;
        stampInfo[impliedLit.toIndex()].dsc = stamp;
        stampInfo[impliedLit.toIndex()].index = 0;
        stampQueue.push_back(impliedLit);
        shuffleVector(search.big.getAdjacencyList(impliedLit));
      }
    }
  }
  return stamp;
}

void Coprocessor2::sortStampTime(Lit* literalArray, const uint32_t size) {
  // insertion sort
  const uint32_t s = size;
  for (uint32_t j = 1; j < s; ++j) {
    const Lit key = literalArray[j];
    const uint32_t keyDsc = stampInfo[key.toIndex()].dsc;
    int32_t i = j - 1;
    while (i >= 0 && stampInfo[literalArray[i].toIndex()].dsc > keyDsc) {
      literalArray[i + 1] = literalArray[i];
      i--;
    }
    literalArray[i + 1] = key;
  }
}

bool Coprocessor2::unhideSimplify() {
  bool didSomething = false;

  // removes ignored clauses, destroys mark-to-delete clauses
  uint32_t j = 0;
  for (uint32_t i = 0; i < formula->size(); ++i) {
    // run UHTE before UHLE !!  (remark in paper)
    const uint32_t clRef = (*formula)[i];
    Clause& clause = search.gsa.get(clRef);
    if (clause.isIgnored()) {
      continue;
    }

    // reduce formula!
    // (*formula)[j++] = (*formula)[i];

    // during simplification, each fourth simplification do not touch the input
    // formula!
    if (simplifying && ((simplifications & 3) == 0) && !clause.isLearnt())
      continue;

    const uint32_t cs = clause.size();
    Lit Splus[cs];
    Lit Sminus[cs];

    for (uint32_t ci = 0; ci < cs; ++ci) {
      Splus[ci] = clause.get_literal(ci);
      Sminus[ci] = clause.get_literal(ci).complement();
    }
    sortStampTime(Splus, cs);
    sortStampTime(Sminus, cs);

    // initial positions in the arrays
    uint32_t pp = 0, pn = 0;

    if (doUHTE) {
      techniqueDoCheck(do_unhide);  // get some statistics
      bool UHTE = false;
      // UHTE( clause ), similar to algorithm in paper
      Lit lpos = Splus[pp];
      Lit lneg = Sminus[pn];

      while (true) {
        // line 4
        if (stampInfo[lneg.toIndex()].dsc > stampInfo[lpos.toIndex()].dsc) {
          if (pp + 1 == cs) break;
          lpos = Splus[++pp];
          // line 7
        } else if ((stampInfo[lneg.toIndex()].fin <
                    stampInfo[lpos.toIndex()].fin) ||
                   (cs == 2 && ((lpos == lneg.complement() ||
                                 stampInfo[lpos.toIndex()].parent == lneg) ||
                                (stampInfo[lneg.complement().toIndex()]
                                     .parent == lpos.complement())))) {
          if (pn + 1 == cs) break;
          lneg = Sminus[++pn];
        } else {
          UHTE = true;
          break;
        }
      }

      if (UHTE) {
        clause.markToDelete();
        removeClause(clRef);
        updateRemoveClause(clause);
        if (clause.size() == 2)
          search.big.removeClauseEdges(clause.get_literal(0),
                                       clause.get_literal(1));
        // if a clause has been removed, call
        techniqueChangedClauseNumber(do_unhide, -1);
        techniqueChangedLiteralNumber(do_unhide, -(int32_t)clause.size());
        didSomething = true;
        continue;
      }
    }

    if (doUHLE != 0) {
      // C = UHLE(C)
      techniqueDoCheck(do_unhide);  // get some statistics
      bool UHLE = false;

      if (doUHLE == 1 || doUHLE == 3) {
        pp = cs;
        uint32_t finished = stampInfo[Splus[cs - 1].toIndex()].fin;
        Lit finLit = Splus[cs - 1];

        for (pp = cs - 1; pp > 0; --pp) {
          const Lit l = Splus[pp - 1];
          const uint32_t fin = stampInfo[l.toIndex()].fin;
          if (fin > finished) {
            removeFromList(l, clRef);
            updateRemoveLiteral(l);
            if (clause.size() == 2)
              search.big.removeClauseEdges(clause.get_literal(0),
                                           clause.get_literal(1));
            clause.removeLin(l);
            // if you did something useful, call
            techniqueSuccessEvent(do_unhide);
            techniqueChangedLiteralNumber(do_unhide, -1);
            didSomething = true;
            UHLE = true;
          } else {
            finished = fin;
            finLit = l;
          }
        }
      }

      if (doUHLE == 2 || doUHLE == 3) {
        const uint32_t csn = clause.size();
        if (csn !=
            cs) {  // if UHLEP has removed literals, recreate the Sminus list
          for (uint32_t ci = 0; ci < csn; ++ci) {
            Sminus[ci] = clause.get_literal(ci).complement();
          }
          sortStampTime(Sminus, csn);
        }

        pn = 0;
        uint32_t finished = stampInfo[Sminus[0].toIndex()].fin;
        Lit finLit = Sminus[0];
        for (pn = 1; pn < csn; ++pn) {
          const Lit l = Sminus[pn];
          const uint32_t fin = stampInfo[l.toIndex()].fin;
          if (fin < finished) {
            removeFromList(l.complement(), clRef);
            updateRemoveLiteral(l.complement());
            if (clause.size() == 2)
              search.big.removeClauseEdges(clause.get_literal(0),
                                           clause.get_literal(1));
            clause.removeLin(l.complement());
            // if you did something useful, call
            techniqueSuccessEvent(do_unhide);
            techniqueChangedLiteralNumber(do_unhide, -1);
            didSomething = true;
            UHLE = true;
          } else {
            finished = fin;
            finLit = l;
          }
        }
      }

      if (UHLE) {
        updateChangeClause(clRef);
        if (clause.size() == 0) {
          solution = UNSAT;
          return didSomething;
        } else {
          if (clause.size() == 1) {
            if (!enqueTopLevelLiteral(clause.get_literal(0))) {
              solution = UNSAT;
              return didSomething;
            }
          }
        }
      }
    }

  }  // end for clause in formula

  return didSomething;
}

bool Coprocessor2::unhiding(bool firstCall) {
  bool didSomething = false;
  techniqueStart(do_unhide);
  unhideLimit = unhideLimit > 0 ? unhideLimit - 1 : 0;

  if (unhideLimit == 0 && !unlimited) {
    techniqueStop(do_unhide);
    return didSomething;
  }

  for (uint32_t iteration = 0; iteration < unhideIter; ++iteration) {

    bool foundEE = false;
    uint32_t stamp = 0;

    tmpLiterals.clear();
    // reset all present variables, collect all roots in binary implication
    // graph
    for (uint32_t i = 0; i < variableHeap.size(); ++i) {
      Var v = variableHeap.item_at(i);
      const Lit pos = Lit(v, POS);
      stampInfo[pos.toIndex()].dsc = 0;
      stampInfo[pos.toIndex()].fin = 0;
      stampInfo[pos.toIndex()].obs = 0;
      stampInfo[pos.toIndex()].index = 0;
      stampInfo[pos.toIndex()].lastSeen = NO_LIT;
      stampInfo[pos.toIndex()].root = pos;
      stampInfo[pos.toIndex()].parent = pos;
      const Lit neg = Lit(v, NEG);
      stampInfo[neg.toIndex()].dsc = 0;
      stampInfo[neg.toIndex()].fin = 0;
      stampInfo[neg.toIndex()].obs = 0;
      stampInfo[neg.toIndex()].index = 0;
      stampInfo[neg.toIndex()].lastSeen = NO_LIT;
      stampInfo[neg.toIndex()].root = neg;
      stampInfo[neg.toIndex()].parent = neg;
      // a literal is a root, if its complement does not imply a literal
      if (search.big.getAdjacencyList(pos).size() == 0)
        tmpLiterals.push_back(neg);
      if (search.big.getAdjacencyList(neg).size() == 0)
        tmpLiterals.push_back(pos);
    }

    // do stamping for all roots, shuffle first
    const uint32_t ts = tmpLiterals.size();
    for (uint32_t i = 0; i < ts; i++) {
      const uint32_t rnd = rand() % ts;
      const Lit tmp = tmpLiterals[i];
      tmpLiterals[i] = tmpLiterals[rnd];
      tmpLiterals[rnd] = tmp;
    }
    // stamp shuffled tmpLiterals
    for (uint32_t i = 0; i < ts; ++i) {
      stamp = stampLiteral(tmpLiterals[i], stamp, foundEE);
    }
    // stamp all remaining literals, after shuffling
    tmpLiterals.clear();
    for (uint32_t i = 0; i < variableHeap.size(); ++i) {
      Var v = variableHeap.item_at(i);
      const Lit pos = Lit(v, POS);
      if (stampInfo[pos.toIndex()].dsc == 0) tmpLiterals.push_back(pos);
      const Lit neg = Lit(v, NEG);
      if (stampInfo[neg.toIndex()].dsc == 0) tmpLiterals.push_back(neg);
    }

    // stamp shuffled tmpLiterals
    const uint32_t ts2 = tmpLiterals.size();
    for (uint32_t i = 0; i < ts2; i++) {
      const uint32_t rnd = rand() % ts2;
      const Lit tmp = tmpLiterals[i];
      tmpLiterals[i] = tmpLiterals[rnd];
      tmpLiterals[rnd] = tmp;
    }
    for (uint32_t i = 0; i < ts2; ++i) {
      stamp = stampLiteral(tmpLiterals[i], stamp, foundEE);
    }

    if (solution == UNKNOWN) {
      enqueueAllEEunits();
      // if failed literals have been found, handle them before the
      // simplification!
      propagateTopLevel();
    }

    if (solution == UNKNOWN && unhideSimplify()) {
      didSomething = true;
      if (didSomething && solution == UNKNOWN) {
        propagateTopLevel();
      }
    }

    // run independent of simplify method

    if (foundEE) {
      equivalenceElimination();
    }

  }  // next iteration ?!

  techniqueStop(do_unhide);
  return didSomething;
}