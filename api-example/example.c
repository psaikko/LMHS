#include "LMHS_C_API.h"
#include <stdio.h>

void printSolution(MaxsatSol * sol) {
	if (sol->size > 0) {
		int i;
		printf("c solution weight: %.2f\nv ", sol->weight);
		for (i = 0; i < sol->size; ++i) {
			int val = sol->values[i];
			printf("%d ", val);
		}
		printf("\n");
	} else {
		printf("No solution\n");
	}
}

int main(int argc, char **argv) {
	FILE *file;
	char line[1000];
	int nVars, nClauses, top;
	MaxsatSol *solution;

	//
	// Command line arguments to the solver can be passed through LMHS_setArgs.
	// As an example, setting verbosity to 0 here to stop solver output to stdout.
	//
	int maxsat_argc = 2;
  	const char* maxsat_argv[] = {"--verb", "0"};
  	LMHS_setArgs(maxsat_argc, maxsat_argv);

	if (argc != 2) {
		printf("this example needs a wcnf file as input\n");
		return 0;
	}

	file = fopen(argv[1], "r");

	//
	// LMHS_initializeWithFile or LMHS_initializeWithRawData would be better choices here.
	// Adding clauses one by one instead to cover more API features.
	//
	int clauses[10000];
	double weights[1000];
	int ci = 0, wi = 0;

	//LMHS_initializeWithoutData();

	while (fgets(line, 1000, file) != NULL) {
		if (line[0] == 'p') {
			// parse wcnf header
			sscanf(line, "p wcnf %d %d %d", &nVars, &nClauses, &top);
			// nVars and nClauses often incorrect so we ignore them
			nVars = 0; nClauses = 0;
		} else if (line[0] != 'c') {
			int weight;
			int clause[1000];
			int t, i = 0;
			sscanf(line, "%d%n", &weight, &t);
			i = t;

			weights[wi++] = (double)weight;

			while(1) {
				int lit, var;
				sscanf(line + i, "%d%n", &lit, &t);
				i += t;
				var = abs(lit);
				if (lit == 0) break;
				clauses[ci++] = lit < 0 ? -var : var;
			}
			clauses[ci++] = 0;
			++nClauses;
		
		}
	}
	fclose(file);

	LMHS_initializeWithRawData(nClauses, top, weights, clauses);

	//
	// Getting a solution from the LMHS solver:
	//
	solution = LMHS_getOptimalSolution();
	printSolution(solution);

	//
	// Incrementally find additional solutions
	//
	while (solution->size > 0) {
		//
		// Adds a constraint blocking the current set of unsatisfied soft clauses
		//
		LMHS_forbidLastHS();

		//
		// LMHS_addSoftClause and LMHS_addHardClause can also be called between 
		// successive calls to LMHS_getOptimalSolution here.
		//
		solution = LMHS_getOptimalSolution();
		printSolution(solution);
	}

	LMHS_reset();

	return 0;
}
