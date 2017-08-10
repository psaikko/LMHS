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
  	const char* maxsat_argv[] = {"--verb", "0", "--no-preprocess"};
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
	weight_t weight;
	vector<int> solution;
	LMHS::getSolution(weight, solution);
	
	cout << "v";
	for (int i : solution) cout << " " << i;
	cout << endl << "o " << weight << endl;

	LMHS::reset(); // unnecessary here

	return 0;
}
