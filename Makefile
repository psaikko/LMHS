MINISATDIR	=	components/minisat
PREPROCESSDIR	=	components/maxpre


GIT_HASH	=	"\"$(shell git rev-parse HEAD)\""
GIT_DATE	=	"\"$(shell git show -s --format=%ci HEAD)\""
LMHS_CPPFLAGS	+=	-DGITHASH=$(GIT_HASH) -DGITDATE=$(GIT_DATE)


SOURCEPATHS	=	$(shell find src -type f | grep cpp$)
SOURCES	=	$(SOURCEPATHS:src/%=%)
OBJFILES	=	$(addprefix obj/,$(SOURCES:.cpp=.o))


.phony: default release debug
default: release
release: OPTFLAGS	:=	-DNDEBUG -O3
release: all
debug: OPTFLAGS	:=	-DDEBUG -ggdb
debug: all

MINISAT_BUILD = release

CPP = g++
LMHS_CPPFLAGS	+=	-std=c++11 -pthread -fPIC -m64 -MMD \
-D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS -Iinclude -Ioptions -I$(PREPROCESSDIR)


PREPRO_LN	=	-L$(PREPROCESSDIR)/lib -lpreprocessor


WARNS	=	-pedantic -Wall -Wextra -W -Wpointer-arith -Wcast-align \
			-Wwrite-strings -Wdisabled-optimization \
			-Wctor-dtor-privacy -Wno-reorder -Woverloaded-virtual \
			-Wsign-promo -Wsynth -Wno-ignored-attributes -Wno-unused-parameter

SATDIR	=	$(MINISATDIR)
SAT_LNFLAGS	=	-lz
SATDEP	=	minisat
SATOBJS	=	$(MINISATDIR)/build/$(MINISAT_BUILD)/minisat/core/Solver.o \
			$(MINISATDIR)/build/$(MINISAT_BUILD)/minisat/utils/System.o \
			$(MINISATDIR)/build/$(MINISAT_BUILD)/minisat/utils/Options.o

include config.mk

CONCERTINCDIR	=	$(CONCERTDIR)/include
CPLEXINCDIR	=	$(CPLEXDIR)/include
CCOPT	=	-fno-strict-aliasing -fexceptions -DIL_STD
MIP_LNDIRS	=	-L$(CPLEXLIBDIR) -L$(CONCERTLIBDIR)
MIP_LNFLAGS	=	-lconcert -lilocplex -lcplex -lm -lpthread -ldl
LMHS_CPPFLAGS	+=	$(CCOPT) -I$(CPLEXINCDIR) -I$(CONCERTINCDIR) -DMIP_CPLEX

FLOAT_WEIGHTS	?=	0
ifeq ($(FLOAT_WEIGHTS), 1)
	LMHS_CPPFLAGS	+=	-DFLOAT_WEIGHTS
	WGHT=flt
else
	WGHT=int
endif

PP ?= $(PREPROCESSDIR)/lib/libpreprocess.a
EXE	?=	bin/LMHS-$(WGHT)
LIBNAME	=	libLMHS-$(WGHT)

STATIC	?=	0
ifeq ($(STATIC),1)
	LMHS_CPPFLAGS	+=	-static -static-libgcc -static-libstdc++
endif

.PHONY: all dirs lib clean Minisat test
all: dirs $(PP) options/regenerate $(EXE) lib

test:
	cd tests && ./run_tests.py ../$(EXE)

options/%.cpp:
	@rm -f options/regenerate

options/%.h:
	@rm -f options/regenerate

options/regenerate: options/params.csv options/generate.py
	@echo "Generating options"
	@cd options && ./generate.py
	@touch options/regenerate

dirs:
	@mkdir -p obj
	@mkdir -p bin
	@mkdir -p lib

lib: $(PP) $(SATDEP) $(MIPDEP) $(OBJFILES) $(LIB_OBJFILES)
	@echo "-> linking lib/$(LIBNAME).so"
	@$(CPP) -shared -Wl,-soname,$(LIBNAME).so -o lib/$(LIBNAME).so \
		$(OBJFILES) $(LIB_OBJFILES)\
		$(MIP_LNDIRS) $(SAT_LNDIRS) $(MIP_LNFLAGS) $(SAT_LNFLAGS) $(PREPRO_LN) $(SATOBJS) -lstdc++

clean:
	rm -f options/regenerate
	rm -rf `find . | grep "\.o$$"`
	rm -rf `find . | grep "\.d$$"`
	$(MAKE) -C $(PREPROCESSDIR) clean

minisat:
	@echo "Compiling minisat:"
	@$(MAKE) -s -C $(MINISATDIR) config prefix=.
	@$(MAKE) -s -C $(MINISATDIR) build/$(MINISAT_BUILD)/minisat/core/Solver.o
	@$(MAKE) -s -C $(MINISATDIR) build/$(MINISAT_BUILD)/minisat/utils/System.o
	@$(MAKE) -s -C $(MINISATDIR) build/$(MINISAT_BUILD)/minisat/utils/Options.o
	@echo "Minisat objects compiled."

$(PP):
	@echo "Compiling preprocessor:"
	@$(MAKE) -C $(PREPROCESSDIR) lib

-include	$(MAINDEP)

$(EXE):	dirs $(PP) $(SATDEP) $(MIPDEP) $(OBJFILES)
	@echo "-> linking $@"
	$(CPP) $(OPTFLAGS) $(LMHS_CPPFLAGS) $(OBJFILES) $(SATOBJS) \
		$(MIP_LNDIRS) $(SAT_LNFLAGS) $(MIP_LNFLAGS) $(PREPRO_LN) $(MIP_FLAGS) -o $@

obj/%.o: src/%.cpp
	@echo "-> compiling $@"
	@$(CPP) $(OPTFLAGS) $(LMHS_CPPFLAGS) -I$(SATDIR) $(MIP_FLAGS) $(WARNS) -c $< -o $@

-include $(shell find obj -type f -name "*.d")
