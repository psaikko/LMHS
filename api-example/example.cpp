#include "LMHS_CPP_API.h"
#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char **argv) {

	//
	// Command line arguments to the solver can be passed through LMHS_setArgs.
	// As an example, setting verbosity to 0 here to stop solver output to stdout.
	//
	int maxsat_argc = 2;
  	const char* maxsat_argv[] = {"--verb", "0"};
  	LMHS::setArgs(maxsat_argc, maxsat_argv);

	if (argc != 2) {
		cout << "this example needs a wcnf file as input" << endl;
		return 1;
	}

	fstream file(argv[1]);
	LMHS::initialize(file);
	file.close();

	//
	// Getting a solution from the LMHS solver:
	//
	double weight;
	vector<int> solution;
	LMHS::getOptimalSolution(weight, solution);
	
	cout << "v";
	for (int i : solution) cout << " " << i;
	cout << endl << "o " << weight << endl;

	//
	// Incrementally find additional solutions
	//
	while (true) {
		//
		// Adds a constraint blocking the current set of unsatisfied soft clauses
		//
		LMHS::forbidLastHS();

		//
		// LMHS_addSoftClause and LMHS::addHardClause can also be called between 
		// successive calls to LMHS::getOptimalSolution here.
		//
		LMHS::getOptimalSolution(weight, solution);
		if (solution.size() > 0) {
			cout << "v";
			for (int i : solution) cout << " " << i;
			cout << endl << "o " << weight << endl;
		} else {
			break;
		}
	}

	LMHS::reset(); // unnecessary here

	return 0;
}
