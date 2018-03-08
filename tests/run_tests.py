#!/usr/bin/env python3
import sys
import os
import subprocess
from time import perf_counter

def filepaths(directory):
	paths = []
	for root, dirs, files in os.walk(directory):
		for filename in files:
			paths.append(os.path.join(root, filename))
	return paths

def sat(clause, model):
	for literal in clause:
		if literal in model:
			return True
	return False

def verify(solver_output, instance):
	soft_clauses = []
	hard_clauses = []
	variables = set()
	topw = 0
	weighted = 1

	model = set()
	opt = 0
	opt_check = 0
	has_model = False

	for line in solver_output:
		if line[0] == 'o':
			vals = line.split(' ')
			opt = float(vals[1])
		elif line[0] == 'v':
			has_model = True
			lits = [int(v) for v in line.strip().split(' ')[1:]]
			for l in lits:
				model.add(l)

	if not has_model: return (False, "No model")

	line_tokens = list(map(str.split, instance))

	for tokens in line_tokens:
		if tokens[0] == 'p':
			if tokens[1] == 'cnf':
				weighted = 0
			if len(tokens) > 4:
				topw = float(tokens[4])
			else:
				topw = 2**63;
		elif tokens[0][0].isdigit() or (not weighted and tokens[0][0] == '-'):
			lits = list(map(int, tokens[:-1]))
			if not weighted:
				if not sat(lits, model):
					opt_check += 1
			else:
				w, lits = lits[0], lits[1:]
				if w == topw:
					if not sat(lits, model):
						return (False, "Hard clause " + str(lits) + " UNSAT")
				else:
					if not sat(lits, model):
						opt_check += w

			variables |= set(abs(l) for l in lits)

	if abs(opt - opt_check) > 1e-7:
		return (False, "Bad model weight %d != %d" % (opt, opt_check))
	else:
		return (True, str(opt_check))

solver = sys.argv[1]
solver_args = sys.argv[2:]

test_instances = filepaths("./instances")

for instance in test_instances:
	args = [solver, instance] + solver_args
	solver_process = subprocess.Popen(args , stdout=subprocess.PIPE, stderr=subprocess.PIPE)

	start = perf_counter()

	out, err = solver_process.communicate()

	end = perf_counter()
	time = end - start

	with open(instance, 'r') as instance_file:
		instance_lines = [line.strip() for line in instance_file]

	instance_lines = [line for line in instance_lines if line]

	solver_output = str(out, 'ascii').split("\n")
	solver_output = [line for line in solver_output if line]

	checks = [line[2:].strip() for line in instance_lines[:10] if line[:2] == 'cc']

	status = "OK"
	message = ""

	for check in checks:
		if check not in solver_output:
			status = "FAIL"
			message = "check \"%s\"" % (check, )
			print(solver_output)
			print(check)
			exit(1)
			break

	if status == "OK" and "s OPTIMUM FOUND" in checks:
		model_status = verify(solver_output, instance_lines)
		ok = model_status[0]
		if not ok:
			status = FAIL
			message = "\"%s\"" % (model_status[1], )

	print("[%s] (%2.2f s) \t%s %s" % (status, time, instance, message))

