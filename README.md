# LMHS : A weighted partial MaxSAT Solver

LMHS uses a hybrid SAT-IP implicit hitting set approach to solve MaxSAT problem instances in WCNF format.
Benchmark instances can be downloaded from the [MaxSAT Evaluation website](http://mse17.cs.helsinki.fi/).

LMHS uses MiniSat as its underlying SAT solver and MaxPre as its preprocessor.

An IP solver must be separately installed to use LMHS. 
Currently only IBM CPLEX is supported without modification to the source code. 
A free academic license for CPLEX can be obtained through the IBM Academic Initiative.
LMHS has been tested with CPLEX 12.5 - 12.8

Run the configure.py script before compiling LMHS to set the necessary IP solver filepaths.

To compile release version:
```
make clean && make release
```

To compile debug version:
```
make clean && make debug 
```

To invoke the solver, run
```
./bin/LMHS-int <wcnf filepath> <params>
```

By default, LMHS uses its 2017 MaxSAT evaluation configuration.

For information on solver parameters, run
```
./bin/LMHS-int --help
```

See the api-example directory for LMHS API usage

To support float-weights in the wcnf input, make with `FLOAT_WEIGHTS=1`
