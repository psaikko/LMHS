#include <cstdint>
#include <vector>
#include <algorithm>
#include <cassert>

#include "preprocessor.hpp"
#include "preprocessedinstance.hpp"
#include "preprocessorinterface.hpp"
#include "global.hpp"
#include "utility.hpp"

#define F first
#define S second

using namespace std;
namespace maxPreprocessor {
	PreprocessorInterface::PreprocessorInterface(const vector<vector<int> >& clauses, const vector<uint64_t>& weights, uint64_t topWeight_) 
	: preprocessor(clauses, weights, topWeight_) {
		topWeight = topWeight_;
		variables = 0;
		originalVariables = 0;
		for (auto& clause : clauses) {
			for (int lit : clause) {
				variables = max(variables, abs(lit));
			}
		}
		originalVariables = variables;
		preprocessed = false;
		useBVEGateExtraction = false;
		useLabelMatching = false;
		skipTechnique = 0;
	}
	
	void PreprocessorInterface::preprocess(string techniques, int logLevel, double timeLimit) {
		preprocessor.logLevel = logLevel;
		preprocessor.printComments = false;
		preprocessor.skipTechnique = skipTechnique;
		
		preprocessor.preprocess(techniques, timeLimit, false, useBVEGateExtraction, !preprocessed, useLabelMatching);
		preprocessed = true;
		variables = max(variables, preprocessor.pi.vars);
	}
	
	void PreprocessorInterface::getInstance(std::vector<std::vector<int> >& retClauses, std::vector<uint64_t>& retWeights, std::vector<int>& retLabels) {
		preprocessedInstance = preprocessor.getPreprocessedInstance();
		
		retClauses = preprocessedInstance.clauses;
		retWeights = preprocessedInstance.weights;
		
		for (unsigned i = 0; i < preprocessedInstance.labels.size(); i++) {
			retLabels.push_back(litToSolver(litToDimacs(preprocessedInstance.labels[i].F)));
		}
		for (auto& clause : retClauses) {
			for (int& lit : clause) {
				lit = litToDimacs(lit);
				variables = max(variables, abs(lit));
				lit = litToSolver(lit);
			}
		}
		
		for (uint64_t& w : retWeights) {
			if (w == HARDWEIGHT) {
				w = topWeight;
			}
		}
	}
	
	vector<int> PreprocessorInterface::reconstruct(const vector<int>& trueLiterals) {
		vector<int> ppTrueLiterals;
		for (int lit : trueLiterals) {
			lit = litToPP(lit);
			if (lit == 0) continue;
			ppTrueLiterals.push_back(lit);
		}
		return preprocessor.trace.getSolution(ppTrueLiterals, 0, variables, originalVariables).F;
	}
	
	void PreprocessorInterface::printSolution(const vector<int>& trueLiterals, ostream& output, uint64_t ansWeight) {
		vector<int> ppTrueLiterals;
		for (int lit : trueLiterals) {
			lit = litToPP(lit);
			if (lit == 0) continue;
			ppTrueLiterals.push_back(lit);
		}
		preprocessor.trace.printSolution(output, ppTrueLiterals, ansWeight, variables, originalVariables);
	}
	
	void PreprocessorInterface::addClause(const std::vector<int>& clause) {
		vector<int> newClause;
		for (int lit : clause) {
			lit = litToPP(lit);
			if (lit == 0) return;// Just ignore clauses that contain unknown variables
			newClause.push_back(litFromDimacs(lit));
		}
		sort(newClause.begin(), newClause.end());
		newClause.erase(unique(newClause.begin(), newClause.end()), newClause.end());
		for (int i = 1; i < (int)newClause.size(); i++) {
			if (newClause[i] == litNegation(newClause[i-1])) return;
		}
		preprocessor.pi.addClause(newClause);
	}
	
	void PreprocessorInterface::setBVEGateExtraction(bool use) {
		useBVEGateExtraction = use;
	}
	
	void PreprocessorInterface::setLabelMatching(bool use) {
		useLabelMatching = use;
	}
	
	void PreprocessorInterface::setSkipTechnique(int value) {
		skipTechnique = value;
	}
	
	int PreprocessorInterface::litToSolver(int lit) {
		if (PPVarToSolverVar.size() < abs(lit)) PPVarToSolverVar.resize(abs(lit));
		if (PPVarToSolverVar[abs(lit)-1] == 0) {
			PPVarToSolverVar[abs(lit)-1] = solverVarToPPVar.size() + 1;
			solverVarToPPVar.push_back(abs(lit));
		}
		if (lit > 0) return PPVarToSolverVar[abs(lit)-1];
		else return -PPVarToSolverVar[abs(lit)-1];
	}
	
	int PreprocessorInterface::litToPP(int lit) {
		if ((int)solverVarToPPVar.size() <= abs(lit)-1) return 0;
		if (lit > 0) return solverVarToPPVar[abs(lit)-1];
		else return -solverVarToPPVar[abs(lit)-1];
	}
	
	void PreprocessorInterface::printInstance(ostream& output, int outputFormat) {
		std::vector<std::vector<int> > clauses;
		std::vector<uint64_t> weights;
		std::vector<int> labels;
		getInstance(clauses, weights, labels);
		
		assert(outputFormat == INPUT_FORMAT_WPMS || outputFormat == INPUT_FORMAT_SAT);
		
		if (clauses.size() == 0) {
			clauses.push_back({-1, 1});
			weights.push_back(topWeight);
		}
		
		if (outputFormat == INPUT_FORMAT_WPMS) {
			output<<"c assumptions ";
			for (unsigned i = 0; i < labels.size(); i++) {
				output<<labels[i];
				if (i + 1 < labels.size()) {
					output<<" ";
				}
			}
			output<<"\n";
		}
		
		if (outputFormat == INPUT_FORMAT_WPMS) {
			output<<"p wcnf "<<max((int)solverVarToPPVar.size(), 1)<<" "<<clauses.size()<<" "<<topWeight<< '\n';
			for (unsigned i = 0; i < clauses.size(); i++) {
				output<<weights[i]<<" ";
				for (int lit : clauses[i]) {
					output<<lit<<" ";
				}
				output<<"0\n";
			}
		}
		else if (outputFormat == INPUT_FORMAT_SAT) {
			output<<"p cnf "<<max((int)solverVarToPPVar.size(), 1)<<" "<<clauses.size()<<'\n';
			for (unsigned i = 0; i < clauses.size(); i++) {
				for (int lit : clauses[i]) {
					output<<lit<<" ";
				}
				output<<"0\n";
			}
		}
		else {
			abort();
		}
		output.flush();
	}
	void PreprocessorInterface::printTechniqueLog(ostream& output) {
		preprocessor.rLog.print(output);
	}
	void PreprocessorInterface::printTimeLog(ostream& output) {
		preprocessor.rLog.printTime(output);
	}
	void PreprocessorInterface::printInfoLog(ostream& output) {
		preprocessor.rLog.printInfo(output);
	}
	void PreprocessorInterface::printMap(ostream& output) {
		output<<solverVarToPPVar.size()<<" "<<variables<<" "<<originalVariables<<'\n';
		for (int t : solverVarToPPVar) {
			output<<t<<" ";
		}
		output<<'\n';
		output<<preprocessor.trace.operations.size()<<'\n';
		for (unsigned i = 0; i < preprocessor.trace.operations.size(); i++) {
			output<<preprocessor.trace.operations[i]<<" ";
			output<<preprocessor.trace.data[i].size()<<" ";
			for (int a : preprocessor.trace.data[i]) {
				output<<a<<" ";
			}
			output<<'\n';
		}
		output.flush();
	}
}


