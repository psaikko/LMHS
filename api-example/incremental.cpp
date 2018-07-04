#include "LMHS_CPP_API.h"
#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char **argv) {

	//
	// Command line arguments to the solver can be passed through LMHS_setArgs.
	// As an example, setting verbosity to 0 here to stop solver output to stdout.
	//
	int maxsat_argc = 3;
  	const char* maxsat_argv[] = {"--verb", "2", "--no-preprocess"};
  	LMHS::setArgs(maxsat_argc, maxsat_argv);

	LMHS::initialize();
	
	//
	// Feed hard and soft clauses via API
	// 
	vector<int> cl;

	cl = {1, 2};
	LMHS::addHardClause(cl);

	cl = {-1, 2};
	LMHS::addHardClause(cl);

	cl = {1, -2};
	LMHS::addSoftClause(2, cl);

	cl = {-1, -2};
	LMHS::addSoftClause(1, cl);

	//
	// Getting a solution from the LMHS solver:
	//
	weight_t weight;
	vector<int> solution;
	LMHS::getSolution(weight, solution);
	
	cout << "v";
	for (int i : solution) cout << " " << i;
	cout << endl << "o " << weight << endl;

	
	//
	// Rule out solution the solution
	//

	LMHS::forbidLastModel();
	/*
	// Above is equivalent to:
	cl.clear();
	for (auto l : solution) cl.push_back(-l);
	LMHS::addHardClause(cl);
	*/

	LMHS::getSolution(weight, solution);
	
	cout << "v";
	for (int i : solution) cout << " " << i;
	cout << endl << "o " << weight << endl;	

	LMHS::reset(); // unnecessary here

	return 0;
}
