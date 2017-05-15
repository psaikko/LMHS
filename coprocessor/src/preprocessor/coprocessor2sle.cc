#include "preprocessor/coprocessor2.h"
#include <sstream>
#include <iostream>
#include <map>
#include <algorithm>
#include <limits>

bool vectorSizeComp(const vector<Var> & a, const vector<Var> & b) {
  return a.size() < b.size();
}

weight_t Coprocessor2::exactHS(unordered_set<Var> & hs, vector<vector<Var>> & sets, weight_t lim, weight_t headWeight, int d) {
  weight_t minWeight = lim;
  if (d >= sets.size()) return headWeight;
  if (headWeight > lim) return minWeight;

  for (auto v : sets[d]) {
    weight_t tailWeight;
    if (hs.count(v)) {
      tailWeight = exactHS(hs, sets, lim, headWeight + 0, d+1);  
    } else {
      weight_t thisWeight = abs(labelWeight[v]);
      hs.insert(v);
      tailWeight = exactHS(hs, sets, lim, headWeight + thisWeight, d+1);
      hs.erase(v);
    }
    minWeight = min(minWeight, tailWeight);
  }

  return minWeight;
}

bool Coprocessor2::generalizedSLE_exact() {

  bool didSomething = false;
  unordered_set<Var> eliminated;

  cout << "c [CP2] gSLE-e checking " << SLE_todo.size() << "/"
       << labelWeight.size() << " vars" << endl;

  unordered_set<Var> old;
  for (auto v : SLE_todo) old.insert(v);
  SLE_todo.clear();

  for (Var t : old) {

    vector<vector<Var>> clauseLabelSets;

    weight_t tWeight = abs(labelWeight[t]);
    vector<CL_REF> tClauses = list(Lit(t, labelWeight[t] < 0 ? POS : NEG));

    if (tClauses.size() == 0) continue;

    bool covered = true;
    for (uint32_t i = 0; i < tClauses.size(); ++i) {
      vector<Var> clauseLabels;
      Clause& cur = search.gsa.get(tClauses[i]);
      for (uint32_t j = 0; j < cur.size(); ++j) {
        Var v = cur.get_literal(j).toVar();
        if (v != t && doNotTouch.get(v) && abs(labelWeight[v]) <= tWeight &&
            !eliminated.count(v)) {
          clauseLabels.push_back(v);
        }
      }
      if (clauseLabels.size() == 0) {
        covered = false;
        break;
      }
      clauseLabelSets.push_back(clauseLabels);
    }

    if (!covered) { continue; }

    std::sort(clauseLabelSets.begin(), clauseLabelSets.end(), vectorSizeComp);

    unordered_set<Var> hs;
    weight_t hsWeight = exactHS(hs, clauseLabelSets, tWeight+1);

#ifdef SLE_DEBUG
    cout << "ce w=" << hsWeight << " lim=" << tWeight << endl;
    cout << "ce hs";
    for (auto v: hs)
      cout << " " << v << ":" << abs(labelWeight[v]);
    cout << endl;
#endif
  
    if (hsWeight <= tWeight) {
      // exact hs with weight <= tWeight covers t
      cout << "e " << t << endl;
      didSomething = true;
      eliminated.insert(t);
      if (!enqueTopLevelLiteral(Lit(t, labelWeight[t] < 0 ? NEG : POS))) {
        solution = UNSAT;
        break;
      }
      ++gSLE_exact_removed;
    }

  }

  if (didSomething) propagateTopLevel();

  return didSomething;
}

bool Coprocessor2::generalizedSLE_greedy() {

  bool didSomething = false;
  unordered_set<Var> eliminated;

  cout << "c [CP2] gSLE-g checking " << SLE_todo.size() << " vars vs "
       << labelWeight.size() << " total" << endl;

  unordered_set<Var> old;
  for (auto v : SLE_todo) old.insert(v);
  SLE_todo.clear();

  for (Var t : old) {

    vector<vector<Var>> clauseLabelSets;
    unordered_map<int, int> labelClauseCounts;

    weight_t tWeight = abs(labelWeight[t]);
    vector<CL_REF> tClauses = list(Lit(t, labelWeight[t] < 0 ? POS : NEG));

    if (tClauses.size() == 0) continue;

    bool covered = true;
    for (uint32_t i = 0; i < tClauses.size(); ++i) {
      vector<Var> clauseLabels;
      Clause& cur = search.gsa.get(tClauses[i]);
      for (uint32_t j = 0; j < cur.size(); ++j) {
        Var v = cur.get_literal(j).toVar();
        if (v != t && doNotTouch.get(v) && abs(labelWeight[v]) <= tWeight &&
            !eliminated.count(v)) {
          clauseLabels.push_back(v);
          labelClauseCounts[v]++;
        }
      }
      if (clauseLabels.size() == 0) {
        covered = false;
        break;
      }
      clauseLabelSets.push_back(clauseLabels);
    }

    if (!covered) {  
      continue;
    }

    // greedy hs

    // keep track of the number of occurrences of each label in the clauses
    // and also the variable weight, to reduce lookups
    unordered_map<int, pair<int, weight_t>> label_count_weight;

    // remember which clauses each label occurs in
    unordered_map<int, vector<int>> label_clauses;

    for (unsigned i = 0; i < clauseLabelSets.size(); ++i) {
      for (int v : clauseLabelSets[i]) {
        if (!label_clauses.count(v)) {
          label_clauses[v] = vector<int>();
          label_count_weight[v] = make_pair(labelClauseCounts.at(v), abs(labelWeight[v]));
        }
        label_clauses[v].push_back(i);
      }
    }

    // keep track of which cores are hit by the current vector hs
    vector<bool> hit(clauseLabelSets.size(), false);
    vector<Var> hs; // debug only

    unsigned unhits = clauseLabelSets.size();
    weight_t hsWeight = 0;

    // repeat until all cores are hit
    while (unhits) {
      // find the variable v with lowest (weight / #occurrences)
      Var v = t;
      weight_t o = numeric_limits<weight_t>::max();
      weight_t w = numeric_limits<weight_t>::max();
      for (auto& e : label_count_weight) {

        int count = e.second.first;
        weight_t wt = e.second.second;

        if (count > 0 && wt / count < o) {
          o = wt / count;
          w = wt;
          v = e.first;
        }
      }

      if (v == t || hsWeight + w > tWeight) goto next_var;

      hs.push_back(v);
      hsWeight += w;

      // update variable counts
      for (int c : label_clauses[v]) {
        // if that core hasn't been hit yet
        if (!hit[c]) {
          // for each variable in the core, decrement occurrences
          for (Var l : clauseLabelSets[c]) {
            label_count_weight[l].first -= 1;
          }
        }
      }
      // track hit cores
      for (int c : label_clauses[v]) {
        if (!hit[c]) {
          hit[c] = true;
          --unhits;
        }
      }
    }

#ifdef SLE_DEBUG
    cout << "c t=" << t  << " w=" << tWeight << endl;
    for (auto s : clauseLabelSets) {
      cout << "c set";
      for (auto v : s)
        cout << " " << v << "(" << abs(labelWeight[v]) << ")";
      cout << endl;
    }

    cout << "c " << t << " covered by" << endl;
    cout << "c hs";
    for (Var v : hs) cout << " " << v;
    cout << endl << "c" << endl;
#endif

    // now greedy hs covers t
    didSomething = true;
    eliminated.insert(t);
    if (!enqueTopLevelLiteral(Lit(t, labelWeight[t] < 0 ? NEG : POS))) {
      solution = UNSAT;
      break;
    }
    ++gSLE_greedy_removed;

    next_var: continue;
  }

  if (didSomething) propagateTopLevel();

  return didSomething;
}


bool Coprocessor2::generalizedSLE_probe() {
  bool didSomething = false;
  unordered_set<Var> eliminated;

  cout << "c [CP2] gSLE-p checking " << SLE_todo.size() << " vars vs "
       << labelWeight.size() << " total" << endl;

  unordered_set<Var> old;
  for (auto v : SLE_todo) old.insert(v);
  SLE_todo.clear();

  // for each variable in todo
  for (Var t : old) {
    
    weight_t tWeight = abs(labelWeight[t]);
    vector<CL_REF> labelClauses = list(Lit(t, labelWeight[t] < 0 ? POS : NEG));

    if (labelClauses.size() == 0 || !doNotTouch.get(t)) {
      //cout << "c " << t << " in no clauses" << endl;
      continue;
    }

    CL_REF shortest = labelClauses[0];
    uint32_t leastLbls = numeric_limits<uint32_t>::max();
    // find clause with least labels of weight < t_weight
    for (uint32_t i = 0; i < labelClauses.size(); ++i) {
      Clause& cur = search.gsa.get(labelClauses[i]);
      uint32_t lbls = 0;
      for (uint32_t j = 0; j < cur.size(); ++j) {

        Var v = cur.get_literal(j).toVar();
        if (v != t && doNotTouch.get(v) && abs(labelWeight[v]) <= tWeight)
          lbls++;
      }
      if (lbls < leastLbls) {
        shortest = labelClauses[i];
        leastLbls = lbls;
      }
    }

    // for each label of clause, greedy probe
    // return explaining set if exists
    
    Clause& root = search.gsa.get(shortest);
    vector<pair<weight_t, Var>> rootLbls;

    for (uint32_t i = 0; i < root.size(); ++i) {
      Var rootVar = root.get_literal(i).toVar();
      if (rootVar != t && doNotTouch.get(rootVar) 
          && abs(labelWeight[rootVar]) <= tWeight 
          && !eliminated.count(rootVar)) {
        rootLbls.push_back({abs(labelWeight[rootVar]), rootVar});
      }
    }

    //cout << "c " << t << " rootLbls " << rootLbls.size() << endl;

    std::sort(rootLbls.begin(), rootLbls.end());

    // starting from lighest root label
    for (auto w_v : rootLbls) {

      unordered_set<Var> explainingSet;
      explainingSet.insert(w_v.second);
      weight_t setWeight = w_v.first;
      bool skip = false;

      // greedy probe from root label ->
      for (uint32_t i = 0; i < labelClauses.size(); ++i) {
        Clause& cur = search.gsa.get(labelClauses[i]);
        if (labelClauses[i] == shortest) continue;

          weight_t leastWeight = numeric_limits<weight_t>::max();
          Var lightestLbl = t;

          for (uint32_t j = 0; j < cur.size(); ++j) {
            Var curVar = cur.get_literal(j).toVar();
            if (explainingSet.count(curVar)) {
              goto explained;
            }

            if (curVar != t && doNotTouch.get(curVar)) {
              weight_t curWeight = abs(labelWeight[curVar]);
              if (curWeight <= leastWeight && curWeight + setWeight <= tWeight && !eliminated.count(curVar)) {
                leastWeight = curWeight;
                lightestLbl = curVar;
              }
            }
          }

          if (lightestLbl == t) {
            skip = true; break;
          }

          setWeight += leastWeight;
          explainingSet.insert(lightestLbl);

        explained:
          continue;
      }

      if (skip) continue;

      // now set covers t
      eliminated.insert(t);
      cout << "e " << t << endl;
      didSomething = true;
      const Lit l = Lit(t, labelWeight[t] < 0 ? NEG : POS);
      if (!enqueTopLevelLiteral(l)) {
        solution = UNSAT;
        break;
      }
      ++gSLE_probe_removed;
      break;
    }
  }

  if (didSomething) propagateTopLevel();

  return didSomething;
}


bool Coprocessor2::generalizedSLE() {
  bool change = false;
  std::unordered_set<Var> explainingSet;
  std::unordered_set<Var> varsToRemove;
  weight_t minWinClause;
  Var bestExplainer;
  bool alreadyExplained;
  bool labelExplained;
  weight_t weightOfExplainingSet;
  bool didSomething = false;

  cout << "c [CP2] GSLE checking " << SLE_todo.size() << " vars vs "
       << labelWeight.size() << " total" << endl;

  unordered_set<Var> old;
  for (auto v : SLE_todo) old.insert(v);
  SLE_todo.clear();    

  for (Var t : old) {

    weight_t weight = labelWeight[t];
    labelExplained = true;
    vector<CL_REF> labelClauses = list(Lit(t, weight < 0 ? POS : NEG));

    if (labelClauses.size() > 0 && doNotTouch.get(t)) {  // A Label literal
      explainingSet.clear();
      weightOfExplainingSet = 0;

      for (uint32_t k = 0; k < labelClauses.size(); k++) {
        minWinClause = 0;
        bestExplainer = t;  // Slight hack in order to reconize that best explainer not seen yet
        Clause& cur = search.gsa.get(labelClauses[k]);
        alreadyExplained = false;

        for (uint32_t j = 0; j < cur.size(); ++j) {
          Var tmp = cur.get_literal(j).toVar();

          if (varsToRemove.count(tmp)) continue;

          weight_t lweight = abs(labelWeight[tmp]);

          if (explainingSet.count(tmp)) {  // We already found this one
            alreadyExplained = true;
            break;
          }

          if (doNotTouch.get(tmp) && tmp != t &&
              (abs(weight) >= (lweight + weightOfExplainingSet))) {
            if (bestExplainer == t) {
              bestExplainer = tmp;
              minWinClause = lweight;
            } else if (lweight <= minWinClause) {
              bestExplainer = tmp;
              minWinClause = lweight;
            }
          }
        }

        if (!alreadyExplained) {
          if (bestExplainer == t) {
            labelExplained = false;
            break;
          } else {
            explainingSet.insert(bestExplainer);
            weightOfExplainingSet += minWinClause;
          }
        }
      }

      if (labelExplained) {
        change = true;
        varsToRemove.insert(t);
        //// HERE t'
        const Lit l = Lit(t, weight < 0 ? NEG : POS);
        if (!enqueTopLevelLiteral(l)) {
          solution = UNSAT;
          break;
        }

        didSomething = true;
        ++gSLE_removed;
      }
      if (debug > 1) cerr << endl;
    }
  }

  if (didSomething) propagateTopLevel();

  return change;
}

bool Coprocessor2::SLE() {
  bool change = false;
  bool didSomething = false;

  unordered_set<Var> old;
  for (auto v : SLE_todo) old.insert(v);
  SLE_todo.clear();

  cout << "c SLE Checking " << old.size() << " of " << labelWeight.size()
       << " labels " << endl;

  for (Var t : old) {

    weight_t weight = labelWeight[t];

    vector<CL_REF> labelClauses = list(Lit(t, weight < 0 ? POS : NEG));
    if (labelClauses.size() > 0 && doNotTouch.get(t)) {
      // If there is an explaining literal, it has to be in the first clause
      Clause& firstClause = search.gsa.get(labelClauses.front());

      for (uint32_t j = 0; j < firstClause.size(); ++j) {
        Var tmp = firstClause.get_literal(j).toVar();
        // check if tmp is already explained by something, in that case tmp
        // cannot explain t and we will find the same explainer for t later
        if (explainedVars.count(tmp)) continue;

        bool labelExplained = false;

        weight_t lweight = labelWeight[tmp];
        if (doNotTouch.get(tmp) && tmp != t && abs(weight) >= abs(lweight)) {

          // Check if its true explainer

            labelExplained = true;

          for (uint32_t k = 1; k < labelClauses.size(); ++k) {
            Clause& check = search.gsa.get(labelClauses.at(k));

            bool checkOk = false;
            for (uint32_t i = 0; i < check.size(); ++i) {
              if (check.get_literal(i).toVar() == tmp) {
                // The explainer is in this clause
                checkOk = true;
                break;
              }
            }

            if (!checkOk) {
              // The explainer is not in all clauses
              labelExplained = false;
              break;
            }
          }
        }

        if (labelExplained) {

          change = true;
          explainedVars[t] = tmp;
          //if (abs(weight) == abs(lweight)) potentialModelsChanged++;
          ++SLE_removed;
          const Lit l = Lit(t, weight < 0 ? NEG : POS);
          if (!enqueTopLevelLiteral(l)) {
            solution = UNSAT;
            break;
          }
          didSomething = true;
          break;
        }
      }
    }
  }

  if (didSomething) propagateTopLevel();
  return change;
}