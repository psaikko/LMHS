#ifndef LMHS_C_API_h
#define LMHS_C_API_h

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MaxsatSol {
  int size;
  int *values;
  double weight;
  double time;
} MaxsatSol;

/* Pass solver command line arguments through API. 
 * int argc 		number of arguments
 * char *argv[] 	argument values
 */
void LMHS_setArgs(int argc, const char *argv[]);

/* Resets internal solver state.
 * Must be called between instances.
 */
void LMHS_reset(void);

/* Get a solution from the solver. Can be called multiple 
 * times consecutively. To prevent repeat solutions, call
 * LMHS_forbidLastSolution or LMHS_forbidLastModel
 *
 * returns: a struct MaxsatSol containing the found solution and its cost.
 */ 
MaxsatSol * LMHS_getOptimalSolution(void);

/* Adds constraint to ILP problem disllowing the current 
 * hitting set. Subsequent solutions will satisfy a different
 * set of soft clauses.
 */
void LMHS_forbidLastHS(void);

/* Adds single hard clause to SAT solver blocking most
 * recent variable assignment.
 */
void LMHS_forbidLastModel(void);

/* Add a known core to the LMHS solver. Core must be a
 * constraint over true literals of blocking variables only.
 * int n			size of core
 * int *core		contents of core
 */
void LMHS_addCoreConstraint(int n, int *core);

/* Print solver stats
 */
void LMHS_printStats(void);

/* Initialize from an existing wcnf-format file.
 * char *filepath 	Filepath to a valid .wcnf file
 *
 * returns: 1 if initialization succeeded
 *			0 otherwise
 */
int LMHS_initializeWithFile(const char *filepath);

/* Initialize with raw clause data.
 * int n 			Number of clauses ín the instance
 * double top 		The weight given to hard clauses
 * double *weights	An array containing n clause weights
 * int *clauses 	An array containing n clauses, each of which ends with a zero
 *
 * returns: 1 if initialization succeeded
 *			0 otherwise
 */
int LMHS_initializeWithRawData(int n, double top, double *weights, int *clauses);

/* initialize the solvers without LMHS instance.
 * The instance must then be created using the
 * LMHS_getFreeVariable, LMHS_addHardClause, and 
 * LMHS_addSoftClause methods.
 *
 * returns: 1 if initialization succeeded 
 *			0 otherwise
 */
int LMHS_initializeWithoutData(void);

/* Creates and returns a new, unused variable.
 * Necessary when using LMHS_addSoftClause due to 
 * created blocking variables.
 */
int LMHS_getNewVariable(void);

/* add a hard clause to the LMHS instance
 * int n  			length of clause
 * int *c 			contents of clause
 */
void LMHS_addHardClause(int n, int *clause);

/* Add a soft clause to the LMHS instance.
 * double w  		weight of clause
 * int n  			length of clause
 * int *c 			contents of clause
 *
 * returns: blocking variable for the created clause
 */
int LMHS_addSoftClause(double weight, int n, int *clause);

#ifdef __cplusplus
}
#endif

#endif