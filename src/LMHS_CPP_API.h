#ifndef LMHS_CPP_API_h
#define LMHS_CPP_API_h

#include <vector>
#include <iosfwd>

namespace LMHS {

/* Pass solver command line arguments through API.
 * int argc 		number of arguments
 * char *argv[] 	argument values
 */
void setArgs(int argc, const char *argv[]);

/* Resets internal solver state.
 * Must be called between instances.
 */
void reset(void);

/* Get a solution from the solver. Can be called multiple
 * times consecutively. To prevent repeat solutions, call
 * forbidLastSolution or add an appropriate hard clause 
 * with addHardClause.
 *
 * returns: true if solution exists, false otherwise
 */
bool getSolution(double & out_weight, std::vector<int> & out_solution);

/* Adds constraint to ILP problem disllowing the current
 * hitting set. Subsequent solutions will satisfy a different
 * set of soft clauses.
 */
void forbidLastHS(void);

/* Adds single hard clause to SAT solver blocking most
 * recent variable assignment.
 */
void forbidLastModel(void);

/* Add a known core to the MAXSAT solver. Core must be a
 * constraint over true literals of blocking variables only.
 *
 * std::vector<int> &core		list of blocking variables
 */
void addCoreConstraint(std::vector<int> &core);

/* Print solver stats
 */
void printStats(void);

/* Initialize from an existing wcnf-format file.
 * std::istream & wcnf_in
 *
 * returns: 1 if initialization succeeded
 *          0 otherwise
 */
bool initialize(std::istream & wcnf_in);

/* Initialize with raw clause data.
 * double top                               The weight given to hard clauses
 * std::vector<double> & weights            vector of clause weights
 * std::vector<std::vector<int>> & clauses  vector of vectors of literals
 *
 *
 * returns: 1 if initialization succeeded
 *          0 otherwise
 */
bool initialize(double top, std::vector<double> &weights,
               std::vector<std::vector<int>> &clauses);

/* initialize the solvers without MAXSAT instance.
 * The instance must then be created using the
 * getFreeVariable, addHardClause, and
 * addSoftClause methods.
 *
 * returns: 1 if initialization succeeded
 *          0 otherwise
 */
bool initialize(void);

/* Creates and returns a new, unused variable.
 * Necessary when using addSoftClause due to
 * created blocking variables.
 */
int getNewVariable(void);

/* add a hard clause to the MAXSAT instance
 * std::vector<int> lits     vector of literals
 */
void addHardClause(std::vector<int> &lits);

/* Add a soft clause to the MAXSAT instance.
 * double w                  weight of clause
 * std::vector<int> lits     vector of literals
 *
 * returns: blocking variable for the created clause
 */
int addSoftClause(double weight, std::vector<int> &lits);
}
#endif