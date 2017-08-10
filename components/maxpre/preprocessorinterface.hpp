#include <cstdint>
#include <iostream>

#include "preprocessor.hpp"

namespace maxPreprocessor {
class PreprocessorInterface {
private:
	int variables;
	int originalVariables;
	Preprocessor preprocessor;
	PreprocessedInstance preprocessedInstance;
	uint64_t topWeight;
	bool preprocessed;
	bool useBVEGateExtraction;
	bool useLabelMatching;
	int skipTechnique;
	std::vector<int> solverVarToPPVar;
	std::vector<int> PPVarToSolverVar;
	int litToSolver(int lit);
	int litToPP(int lit);
public:
	PreprocessorInterface(const std::vector<std::vector<int> >& clauses, const std::vector<uint64_t>& weights, uint64_t topWeight_);
	void preprocess(std::string techniques, int logLevel = 0, double timeLimit = 1e9);
	
	void addClause(const std::vector<int>& clause);
	
	void setBVEGateExtraction(bool use);
	void setLabelMatching(bool use);
	void setSkipTechnique(int value);
	
	void getInstance(std::vector<std::vector<int> >& retClauses, std::vector<uint64_t>& retWeights, std::vector<int>& retLabels);
	std::vector<int> reconstruct(const std::vector<int>& trueLiterals);
	std::vector<std::pair<int, std::pair<int, int> > > getCondEdges();
	
	void printInstance(std::ostream& output, int outputFormat = 0);
	void printSolution(const std::vector<int>& trueLiterals, std::ostream& output, uint64_t ansWeight);
	void printMap(std::ostream& output);
	void printTechniqueLog(std::ostream& output);
	void printTimeLog(std::ostream& output);
	void printInfoLog(std::ostream& output);
};
}