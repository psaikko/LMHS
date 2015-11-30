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
        binaryImplicationGraph.h
        This file is part of riss.

        17.10.2011
        Copyright 2011 Norbert Manthey
*/

#ifndef _BINARYIMPLICATIONGRAPH_H
#define _BINARYIMPLICATIONGRAPH_H

#include <vector>
#include "types.h"
#include "structures/literal_system.h"

using namespace std;

/** This graph represents all the clauses of the formula that are binary
 *  An edge (a,b) in this graph means that b is implied by a.
 *  Furthermore, this expresses that there is a clause [-a,b].
 *  Thus, there is also an edge (-b,-a).
 */
class BIG {
  uint32_t varCnt;
  vector<vector<Lit> > graph;

 public:
  BIG();

  /** initialize graph for nuber of variables
   * NOTE: necessary
   */
  void init(uint32_t variables);

  /** extend the size of the graph
   */
  void extend(uint32_t newVariables);

  /** adds an edge to the graph
   */
  void addEdge(const Lit& a, const Lit& b);

  /** adds the two edges that result from the given clause to the graph
   */
  void addClauseEdges(const Lit& a, const Lit& b);

  /** checks the graph for an edge (a,b), which is also (b,a)
   */
  bool hasEdge(const Lit& a, const Lit& b) const;

  /** removes an edge from the graph
   */
  void removeEdge(const Lit& a, const Lit& b);

  /** removes the edges that result from the given clause from the graph
  */
  void removeClauseEdges(const Lit& a, const Lit& b);

  /** removes all edges that have to do with literal a
  */
  void removeLiteral(const Lit& a);

  /** clear the adjacency list of a literal
   */
  void clearAdjacency(const Lit& a);

  /** removes all transitive edges (keeps shortest)
   */
  void removeTransitive();

  /** returns the adjacency list of the specified literal
   */
  const vector<Lit>& getAdjacencyList(const Lit& a) const;
  vector<Lit>& getAdjacencyList(const Lit& a);
};

inline BIG::BIG() { varCnt = 0; }

inline void BIG::init(uint32_t variables) {
  varCnt = variables;
  graph.resize(max_index(varCnt));
}

inline void BIG::extend(uint32_t newVariables) {
  varCnt = newVariables;
  graph.resize(max_index(varCnt));
}

inline void BIG::addEdge(const Lit& a, const Lit& b) {

  // linear search + insert
  vector<Lit>& list = graph[a.toIndex()];

  uint32_t i = 0;
  for (; i < list.size(); ++i) {
    if (list[i] == b) break;
  }
  if (i == list.size()) list.push_back(b);
}

inline void BIG::addClauseEdges(const Lit& a, const Lit& b) {
  addEdge(a.complement(), b);
  addEdge(b.complement(), a);
}

inline bool BIG::hasEdge(const Lit& a, const Lit& b) const {
  // linear search
  const vector<Lit>& list = graph[a.toIndex()];

  uint32_t i = 0;
  for (; i < list.size(); ++i) {
    if (list[i] == b) return true;
  }
  return false;
}

inline void BIG::removeEdge(const Lit& a, const Lit& b) {
  // linear search
  vector<Lit>& list = graph[a.toIndex()];

  uint32_t i = 0;
  for (; i < list.size(); ++i) {
    if (list[i] == b) {
      list[i] = list[list.size() - 1];
      list.pop_back();
      return;
    }
  }
}

inline void BIG::removeClauseEdges(const Lit& a, const Lit& b) {
  removeEdge(a.complement(), b);
  removeEdge(b.complement(), a);
}

inline void BIG::removeLiteral(const Lit& a) {
  for (uint32_t i = 0; i < graph[a.toIndex()].size(); ++i) {
    const Lit b = graph[a.toIndex()][i];
    removeEdge(b.complement(), a.complement());
  }
  graph[a.toIndex()].clear();
}

inline void BIG::clearAdjacency(const Lit& a) { graph[a.toIndex()].clear(); }

inline void BIG::removeTransitive() {}

inline vector<Lit>& BIG::getAdjacencyList(const Lit& a) {
  return graph[a.toIndex()];
}

inline const vector<Lit>& BIG::getAdjacencyList(const Lit& a) const {
  return graph[a.toIndex()];
}

#endif
