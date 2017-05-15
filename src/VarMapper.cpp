#include "VarMapper.h"
#include "GlobalConfig.h"

#include <string> 
#include <istream>
#include <ostream>
#include <sstream>
#include <cstdio> // EOF
#include <cctype> // isdigit, isspace
#include <vector>
#include <algorithm> // sort
#include <ctime>
#include <cfloat> // DBL_MAX
#include <iomanip> // setprecision

#include <iostream>

using namespace std;

void VarMapper::map(istream & in, ostream & out) {

	string line;
	long var_max = 0;
	double top = DBL_MAX;

	while (in.peek() != EOF) {
		int c;
		while (isspace(c = in.peek())) in.get();
		if (c == 'p') {
			stringstream linein;
			getline(in, line);
			linein.str(line);
			string type;
			long vars, clauses;
			linein >> type >> type;
			linein >> vars >> clauses;
			weighted = (type == "wcnf");
			out << "p " << "wcnf" << " " << vars << " " << clauses;

			if (!(linein >> top)) {
				linein.clear();
				partial = false;
			} else {
				out << " " << top;
				partial = true;
			}
			out << endl;
			cout << "c top " << top << endl;
		} else if (c == 'c') {
			getline(in, line);
		} else if (isdigit(c) || (!weighted && c == '-')) {
			double w;
			long l;
			
			if (weighted) {
				in >> w;
			} else {
				w = 1;
			}

			out << w << " ";

			while (in >> l) {
				if (l == 0) break;
				long v = abs(l);
				bool s = l < 0;
				if (var_map.count(v)) {
					long mv = var_map[v];
					out << (s ? -mv : mv) << " ";
				} else {
					var_map[v] = ++var_max;
					inv_var_map[var_max] = v;
					out << (s ? -var_max : var_max) << " ";
				}
			}
			out << "0" << endl;
		} else {
			in.clear();
			getline(in, line);
		}
	}
}

void VarMapper::unmap(istream & in, ostream & out) {
	string line;

	while(in.peek() != EOF) {
		char c = in.peek();
		if (c == 'o' || c == 'c' || c == 's') {
			getline(in, line);
			out << line << endl;
		} else if (c == 'v') {
			vector<long> model;

			in.get();
			long l;
			while (in >> l) {
				long mv = abs(l);
				if (inv_var_map.count(mv)) {
					bool s = l < 0;
					long v = inv_var_map[mv];
					model.push_back(s ? -v : v);
				}
			}

			in.clear();

			sort(model.begin(), model.end(), [] (const long &a, const long &b) { return abs(a) < abs(b); } );

			out << "v";
			for (long l : model) out << " " << l;
			out << endl;
		} else {
			getline(in, line);
		}
	}
}
