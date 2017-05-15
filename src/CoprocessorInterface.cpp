#include "GlobalConfig.h"
#include "CoprocessorInterface.h"
#include "Util.h"

#include <cstring>
#include <ctime>
#include <sstream>
#include <fstream>
// coprocessor:
#include "sat/rissmain.h"
#include "utils/commandlineparser.h"

CoprocessorInterface::CoprocessorInterface() {
	CommandLineParser cmp;
	GlobalConfig & cfg = GlobalConfig::get();
	int  argc = 5;
	char tech[100];
	const char * group = cfg.pre_group ? "-CP_group=1" : "-CP_group=0";
	const char * groupOnly = cfg.pre_group_only ? "-CP_onlyGroup=1" : "-CP_onlyGroup=0";
	strcpy(tech, ("-CP_techniques="+cfg.pre_techniques).c_str());
	char const * argv[] = {"","-CP_print=1",group,groupOnly,tech};

	commandline = cmp.parse(argc, argv);
}

int CoprocessorInterface::CP_preprocess(std::istream & wcnf_in, std::ostream & wcnf_out) {

	clock_t start = clock();

	std::stringstream map_out;
	std::ofstream null_out("/dev/null");

	int32_t res = preprocess(wcnf_out, wcnf_in, map_out, null_out, commandline);

	mapFileString = map_out.str();

	log(1, "c preprocessing time %.2fs\n", SECONDS(clock() - start));

	if (res == SAT) {
		log(1, "c coprocessor status SAT\n");
		return CI_SAT;
	} else if (res == UNSAT) {
		log(1, "c coprocessor status UNSAT\n");
		return CI_UNSAT;
	} else if (res == UNKNOWN) {
		log(1, "c coprocessor status UNKNOWN\n");
		return CI_UNDEF;
	} else {
		log(1, "c unrecognized coprocessor result\n");
		return CI_ERR;
	}
}	

void CoprocessorInterface::CP_completeModel(std::istream & model_in, std::ostream & model_out) {

	std::stringstream map_in(mapFileString);
	std::ofstream null_out("/dev/null");

	completeModel(model_out, model_in, map_in, null_out, commandline);
}