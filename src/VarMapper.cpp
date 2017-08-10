#include <string> 
#include <istream>
#include <ostream>
#include <sstream>
#include <cstdio> // EOF
#include <cctype> // isdigit, isspace
#include <vector>
#include <algorithm> // sort
#include <ctime>
#include <iomanip> // setprecision
#include <iostream>

#include "VarMapper.h"
#include "GlobalConfig.h"
#include "Weights.h"

using namespace std;

void VarMapper::map(istream & in, ostream & out) {

	string line;
	long var_max = 0;
	weight_t top = WEIGHT_MAX;

	while (in.peek() != EOF) {
		int c;
		while (isspace(c = in.peek())) in.get();
		if (c == 'p') {
			stringstream linein;
			getline(in, line);
			linein.str(line);
			string type;
			
			linein >> type >> type;
			linein >> n_vars >> n_clauses;
			weighted = (type == "wcnf");
			out << "p " << "wcnf" << " " << n_vars << " " << n_clauses;

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
			weight_t w;
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
			out << line << endl;
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

			unsigned j = 0;
			for (int i = 1; i <= n_vars; ++i) {
			 	if (j >= model.size() || abs(model[j]) != i) {
					model.push_back(i);
				} else if ((j + 1) < model.size())  {
					++j;
				}
			}

			sort(model.begin(), model.end(), [] (const long &a, const long &b) { return abs(a) < abs(b); } );

			out << "v";
			for (long l : model) out << " " << l;
			out << endl;
		} else {
			getline(in, line);
		}
	}
}
