#include "LingelingSolver.h"
#include <cstdio>
#include <iostream>

using namespace std;

LingelingSolver::LingelingSolver() {
  lgl = lglinit();
  lglsetout(lgl, stderr);
}

LingelingSolver::~LingelingSolver() { lglrelease(lgl); }

void LingelingSolver::addVariable(int var) {
  int max_var = lglmaxvar(lgl);
  while (max_var < var) {
    max_var = lglincvar(lgl);
  }
}

void LingelingSolver::findCore(vector<int>& out_core) {
  out_core.clear();
  if (!solve()) getConflict(out_core);
}

void LingelingSolver::getConflict(vector<int>& conflict) {
  for (int a : assumptions) {
    if (lglfailed(lgl, a)) {
      conflict.push_back(abs(a));
    }
  }
}

void LingelingSolver::findCores(vector<vector<int>>& out_cores) {
  out_cores.clear();
  vector<int> core;
  findCore(core);
  if (core.size()) out_cores.push_back(core);
}

bool LingelingSolver::findCoreLimited(vector<int>& /*out_core*/) {
  terminate(1, "LingelingSolver::findCoreLimited not yet implemented\n");
  return false;
}

bool LingelingSolver::solve() {

  start = clock();

  for (int a : assumptions) {
    lglassume(lgl, a);
    lglfreeze(lgl, a);
  }

  int res = lglsat(lgl);

  if (res == LGL_UNKNOWN) {
    terminate(1, "lglsat returned unknown\n");
  } else if (res != LGL_SATISFIABLE && res != LGL_UNSATISFIABLE) {
    terminate(1, "lglsat unexpected return code\n");
  }

  end = clock();
  solver_time += (end - start);
  solver_calls++;

  return res == LGL_SATISFIABLE;
}

int LingelingSolver::newVar() {
  return lglincvar(lgl);
}

void LingelingSolver::addConstraint(vector<int>& constr) {
  for (int l : constr) {
    lgladd(lgl, l);
  }
  lgladd(lgl, 0);
}

void LingelingSolver::getModel(vector<bool>& model) {
  int max_var = lglmaxvar(lgl);
  model.resize(max_var);
  for (int i = 1; i < max_var; ++i) {
    model[i] = lglderef(lgl, i) > 0;
  }
}

double LingelingSolver::getModelWeight(
    unordered_map<int, double>& bvar_weights) {

  condTerminate(lglinconsistent(lgl), 1,
                "LingelingSolver::getModelWeight: no model exists\n");

  double opt = 0;
  for (auto kv : bvar_weights)
    if (lglderef(lgl, kv.first) > 0) opt += kv.second;
  return opt;
}

void LingelingSolver::activateClauses() {
  for (unsigned i = 0; i < assumptions.size(); i++)
    assumptions[i] = -abs(assumptions[i]);
}

void LingelingSolver::deactivateClauses() {
  for (unsigned i = 0; i < assumptions.size(); i++)
    assumptions[i] = abs(assumptions[i]);
}

void LingelingSolver::activateClause(int bvar) {
  int i = var_assumptionIdx[bvar];
  assumptions[i] = -abs(assumptions[i]);
}

void LingelingSolver::deactivateClause(int bvar) {
  int i = var_assumptionIdx[bvar];
  assumptions[i] = abs(assumptions[i]);
}

void LingelingSolver::addBvarAssumption(int bvar) {
  var_assumptionIdx[bvar] = assumptions.size();
  assumptions.push_back(bvar);
}

void LingelingSolver::removeBvarAssumption(int bVar) {
  int tailIdx = assumptions.size() - 1;
  var_assumptionIdx[abs(assumptions[tailIdx])] = var_assumptionIdx[bVar];
  assumptions[var_assumptionIdx[bVar]] = assumptions[tailIdx];
  assumptions.pop_back();
}

int LingelingSolver::nVars() { 
  return lglmaxvar(lgl);
}

void LingelingSolver::deleteLearnts() {
  lglforgetlearned(lgl);
}

void LingelingSolver::invertActivity() {
  lglinvertvsids(lgl);
}

void LingelingSolver::setVarPolarity(int var, bool polarity) {
  lglsetphase(lgl, polarity ? var : -var);
}