CPP = g++-4.8

MINISATDIR = minisat
LGLDIR = lingeling/code

VERSION = "\"2015.11\""
GIT_DATE = "\"$(shell git show -s --format=%ci HEAD)\""
LMHS_CPPFLAGS += -DVERSION=$(VERSION) -DGITDATE=$(GIT_DATE)

# make MIP=scip to use SCIP instead of CPLEX
MIP ?= cplex

SAT ?= minisat

# ugly hack, exclude coprocessor main due to namespace collision
COPROCESSOR_OBJS = $(shell find coprocessor -iname '*.o' | grep -v /main)

OBJFILES = obj/Solver.o obj/main.o obj/WCNFParser.o obj/ArgsParser.o obj/ProblemInstance.o \
					 obj/NonoptHS.o obj/LMHS_C_API.o obj/GlobalConfig.o obj/Util.o obj/CoprocessorInterface.o \
					 obj/VarMapper.o obj/LMHS_CPP_API.o

LMHS_CPPFLAGS += -O3 -std=c++11 -pthread -fPIC -m64 \
	-D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS -Icoprocessor/include

LMHS_DEBUG ?= 0
ifeq ($(LMHS_DEBUG),0)
	LMHS_CPPFLAGS += -DNDEBUG 
else
	LMHS_CPPFLAGS += -ggdb -Og -DDEBUG
endif

WARNS = -pedantic -Wall -Wextra -W -Wpointer-arith -Wcast-align -Wwrite-strings \
 -Wdisabled-optimization -Wctor-dtor-privacy \
 -Wreorder -Woverloaded-virtual -Wsign-promo -Wsynth

ifeq ($(SAT), minisat)
	SATDIR    = $(MINISATDIR)
	SAT_LNFLAGS	=	-lz
	LMHS_CPPFLAGS += -DSAT_MINISAT
	SATDEP 	= minisat
	SATOBJS = $(MINISATDIR)/build/release/minisat/core/Solver.o \
				$(MINISATDIR)/build/release/minisat/utils/System.o \
				$(MINISATDIR)/build/release/minisat/utils/Options.o
	OBJFILES += obj/MinisatSolver.o				
else ifeq ($(SAT), lingeling) 
	SATDIR = $(LGLDIR)
	SATDEP = lingeling
	SAT_LNDIRS += -L$(LGLDIR)
	SAT_LNFLAGS += -llgl 
	LMHS_CPPFLAGS += -DSAT_LINGELING
	OBJFILES += obj/LingelingSolver.o
else
	$(error Unknown SAT solver specified.)
endif

include config.mk

ifeq ($(MIP),scip)
	include $(SCIPDIR)/make/make.project
	MIPDEP       = $(SCIPDIR) $(SCIPLIBFILE) $(LPILIBFILE) $(NLPILIBFILE)
	LMHS_CPPFLAGS    += $(FLAGS) -DMIP_SCIP
	MIP_LNDIRS   = -L$(SCIPDIR)/lib
	MIP_LNFLAGS	 = -l$(SCIPLIB)$(LINKLIBSUFFIX) -l$(OBJSCIPLIB)$(LINKLIBSUFFIX)\
		-l$(LPILIB)$(LINKLIBSUFFIX) -l$(NLPILIB)$(LINKLIBSUFFIX)\
		-lm -lz -lzimpl.linux.x86_64.gnu.opt -lgmp -lreadline -lncurses\
		$(LPSLDFLAGS) $(SCIPLDFLAGS) -Wl,-rpath,$(SCIPDIR)/lib
	OBJFILES += obj/SCIPSolver.o
else ifeq ($(MIP),cplex)
	CONCERTINCDIR = $(CONCERTDIR)/include
	CPLEXINCDIR   = $(CPLEXDIR)/include
	CCOPT         = -fno-strict-aliasing -fexceptions -DIL_STD
	MIP_LNDIRS		=	-L$(CPLEXLIBDIR) -L$(CONCERTLIBDIR)
	MIP_LNFLAGS		=	-lconcert -lilocplex -lcplex -lm -lpthread
	LMHS_CPPFLAGS += $(CCOPT) -I$(CPLEXINCDIR) -I$(CONCERTINCDIR) -DMIP_CPLEX
	OBJFILES += obj/CPLEXSolver.o
else
	$(error Unknown MIP solver specified)
endif

EXE = bin/LMHS
LIBNAME = libLMHS

STATIC ?= 0
ifneq ($(STATIC),0)
	LMHS_CPPFLAGS += -static -static-libgcc -static-libstdc++
endif

ifeq ($(CONFIG),2015I)
	LMHS_CPPFLAGS += -DE2015I
	EXE = bin/LMHS-I-2015
endif

ifeq ($(CONFIG),2015C)
	LMHS_CPPFLAGS += -DE2015C
	EXE = bin/LMHS-C-2015
endif

.PHONY: all
all: clean $(EXE)

.PHONY: fast
fast: $(EXE)

obj:	
	@-mkdir -p obj

bin:
	@-mkdir -p bin

.PHONY: lib
lib: $(SATDEP) $(MIPDEP) obj $(OBJFILES)
	@mkdir -p lib
	@echo "Linking static library"
	@$(CPP) -shared -Wl,-soname,$(LIBNAME).so.1 -o lib/$(LIBNAME).so.1.0 \
		$(OBJFILES) $(COPROCESSOR_OBJS) \
		$(MIP_LNDIRS) $(SAT_LNDIRS) $(MIP_LNFLAGS) $(SAT_LNFLAGS) $(SATOBJS) -lstdc++
	@cd lib && ln -sf $(LIBNAME).so.1.0 $(LIBNAME).so
	@cd lib && ln -sf $(LIBNAME).so.1.0 $(LIBNAME).so.1

.PHONY: clean
clean:
	rm -rf obj
	make -s -C coprocessor clean
	make -s -C $(MINISATDIR) clean

.PHONY: minisat
minisat:
	@echo "Compiling minisat:"
	@make -s -C $(MINISATDIR) config prefix=.
	@make -s -C $(MINISATDIR) build/release/minisat/core/Solver.o
	@make -s -C $(MINISATDIR) build/release/minisat/utils/System.o
	@make -s -C $(MINISATDIR) build/release/minisat/utils/Options.o
	@echo "Minisat compiled."

.PHONY: lingeling
lingeling:
	@echo "Compiling lingeling:"
	@make -s -C $(LGLDIR) liblgl.a
	@echo "Lingeling compiled."

.PHONY: coprocessor
coprocessor:
	@echo "Compiling coprocessor:"
	@make -s -C coprocessor coprocessor
	@echo "Coprocessor compiled."

-include	$(MAINDEP)

$(EXE):	$(SATDEP) $(MIPDEP) bin obj $(OBJFILES) coprocessor
	@echo "-> linking $@"
	@$(CPP) $(LMHS_CPPFLAGS) $(OBJFILES) $(COPROCESSOR_OBJS) $(SATOBJS) \
			$(SAT_LNDIRS) $(MIP_LNDIRS) $(SAT_LNFLAGS) $(MIP_LNFLAGS) $(MIP_FLAGS) -o $@

obj/%.o: src/%.cpp
	@echo "-> compiling $@"
	@$(CPP) $(LMHS_CPPFLAGS) -I$(SATDIR) $(MIP_FLAGS) $(WARNS) -c $< -o $@
