#include <algorithm>  // std::sort, std::reverse, std::count
#include <cmath>

#include "NonoptHS.h"
#include "Util.h"

using namespace std;

namespace NonoptHS {

void _frac(double f, vector<int>& out_hs, const vector<vector<int>>& new_cores,
          const unordered_map<int, unsigned>& coreClauseCounts) {
  log(2, "NonoptHS: frac\n");

  for (auto core : new_cores) {
    vector<pair<int, int>> cts_vars;
    unsigned take = (int)ceil(core.size() * f);

    for (auto c : core)
      if (count(out_hs.begin(), out_hs.end(), c))
        goto skip;  // core already hit by out_hs

    for (int v : core) cts_vars.push_back(make_pair(coreClauseCounts.at(v), v));
    sort(cts_vars.begin(), cts_vars.end());
    for (auto& occ_var : cts_vars) {
      if (!take) break;
      out_hs.push_back(occ_var.second);
      --take;
    }

  skip: continue;
  }
}

void _disjoint(vector<int>& out_hs, const vector<vector<int>>& new_cores) {
  log(2, "NonoptHS: disjoint\n");

  for (auto core : new_cores)
    for (int v : core)
      if (!count(out_hs.begin(), out_hs.end(), v)) out_hs.push_back(v);
}

void _common(vector<int>& out_hs, const vector<vector<int>>& new_cores,
            const unordered_map<int, unsigned>& coreClauseCounts) {
  log(2, "NonoptHS: common\n");

  for (auto core : new_cores) {
    int maxVar = -1;
    unsigned maxCount = 0;
    for (auto c : core)
      if (count(out_hs.begin(), out_hs.end(), c))
        goto skip;  // core already hit by out_hs

    for (int v : core) {
      if (coreClauseCounts.at(v) > maxCount) {
        maxVar = v;
        maxCount = coreClauseCounts.at(v);
      }
    }

    if (maxVar != -1)
      out_hs.push_back(maxVar);

  skip: continue;
  }
}

//
// Greedy algorithm for minimum cost hitting set.
//
void _greedy(vector<int>& out_hs, const vector<vector<int>>& cores,
            const unordered_map<int, weight_t>& weights,
            const unordered_map<int, unsigned>& coreClauseCounts) {
  log(2, "NonoptHS: greedy\n");

  // keep track of the number of occurrences of each variable in the cores
  // and also the variable weight, to reduce lookups
  unordered_map<int, pair<int, weight_t>> var_count_weight;

  // remember which cores each variable occurs in
  unordered_map<int, vector<int>> var_cores;

  out_hs.clear();

  log(2, "c finding greedy hitting set of %ld cores\n", cores.size());

  for (unsigned i = 0; i < cores.size(); ++i) {
    for (int v : cores[i]) {
      if (var_cores.find(v) == var_cores.end()) {
        var_cores[v] = vector<int>();
        var_count_weight[v] = make_pair(coreClauseCounts.at(v), weights.at(v));
      }
      var_cores[v].push_back(i);
    }
  }

  // keep track of which cores are hit by the current vector hs
  vector<bool> hit(cores.size(), false);

  unsigned unhits = cores.size();
  weight_t weight = 0;

  // repeat until all cores are hit
  while (unhits) {

    // find the variable v with lowest (weight / #occurrences)
    int v = -1;
    double o = std::numeric_limits<double>::max();
    weight_t w = WEIGHT_MAX;

    for (auto& e : var_count_weight) {
      int count = e.second.first;
      weight_t wt = e.second.second;
      double _o = ((double)wt / (double)count);
      if (count != 0 && _o < o) {
        o = _o;
        w = wt;
        v = e.first;
      }
    }

    weight += w;

    // update variable counts
    for (int c : var_cores[v]) {
      // if that core hasn't been hit yet
      if (!hit[c]) {
        // for each variable in the core, decrement occurrences
        for (int b : cores[c]) {
          var_count_weight[b].first -= 1;
        }
      }
    }

    // track hit cores
    for (int c : var_cores[v]) {
      if (!hit[c]) {
        hit[c] = true;
        --unhits;
      }
    }

    out_hs.push_back(v);
  }

  log(2, "c found greedy hs with cost %" WGT_FMT "\n", weight);
}

void common(vector<int>& out_hs, 
           const vector<vector<int>>& new_cores,
           const vector<vector<int>>&,
           const unordered_map<int, weight_t>&,
           const unordered_map<int, unsigned>& coreClauseCounts) {
  _common(out_hs, new_cores, coreClauseCounts);
}

void greedy(vector<int>& out_hs, 
           const vector<vector<int>>&,
           const vector<vector<int>>& cores,
           const unordered_map<int, weight_t>& weights,
           const unordered_map<int, unsigned>& coreClauseCounts) {
  _greedy(out_hs, cores, weights, coreClauseCounts);
}

NonOptHSFunc frac(double fracSize) {
  return [&](vector<int>& out_hs, const vector<vector<int>>& new_cores,
             const vector<vector<int>>&, const unordered_map<int, weight_t>&,
             const unordered_map<int, unsigned>& coreClauseCounts) {
    _frac(fracSize, out_hs, new_cores, coreClauseCounts);
  };
}

void disjoint(vector<int>& out_hs, 
           const vector<vector<int>>& new_cores,
           const vector<vector<int>>&,
           const unordered_map<int, weight_t>&,
           const unordered_map<int, unsigned>&) {
  _disjoint(out_hs, new_cores);
}

}