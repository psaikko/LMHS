#
#	Makefile
#	This file is part of riss.
#	
#	26.08.2010
#	Copyright 2010 Norbert Manthey
#
DATE="`date +%y%m%d%H%M%S`"

#commandline
CMD = -O3
ARGS = 
DEBUG = -DNDEBUG
SPACE = " "

#compiler, linker, archiver
CPP = g++-4.8 -std=c++11
#/usr/local/bin/g++ -std=c++0x
#	icpc -fast -Wall -DCOMPILE_INTEL
AR  = ar

#compiler flags
LIBS		=	
CPPINCLUDES 	= -I./include -I../libPD/src/runtime/include
COMMON_CPPFLAGS = $(CPPINCLUDES) -static -static-libgcc -static-libstdc++ -fPIC
CPPFLAGS 	= $(COMMON_CPPFLAGS) -Wno-narrowing -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-packed -Wno-unused-variable -fno-exceptions -fno-strict-aliasing $(DEBUG)
LDFLAGS 	= -L. -lpthread -ldl 
#-stack_size 0x10000000 -stack_addr 0xc0000000

# compile preprocessor only?
coprocessor: CPPFLAGS += -D COPROCESSOR

#
# section below includes all the necessay rules for building riss, priss, coprocessor
# furthermore, there are some useful developer hints
#

#build variables
SRCS = $(wildcard src/*/*.cc src/*/*/*.cc src/*.cc ssa/sls/sat/*/*.cc)
SRCS := $(filter-out src/info.cc, $(SRCS))
SRCS += src/info.cc
OBJS = $(SRCS:.cc=.o)
DEPS = $(SRCS:.cc=.d)
# for building riss as a library
LIBRISSOBJ := $(filter-out src/main.o, $(OBJS))
# for building the component libraries
LIBSRCS = $(wildcard libsrc/*.cc)
DEPS += $(LIBSRCS:.cc=.d)
LIBOBJS = $(LIBSRCS:.cc=.lo)
LIBFILES = $(LIBSRCS:.cc=.so)

all: coprocessor

coprocessor: $(OBJS)
	@echo "-> linking "$@
	@$(CPP) -o $@ $(OBJS) $(LIBS) $(LDFLAGS) $(CMD) $(CPPFLAGS) $(ARGS)

src/info.cc: always

# special handling for multithreading
src/utils/waitfunc.o: src/utils/waitfunc.cc
	@$(CPP) $(CMD) $(CPPFLAGS) $(ARGS) -c -o $@ $< -O0 -MD
	
#build object file and dependencies files
.cc.o:
	@echo "-> compiling "$@
	@$(CPP) $(CMD) $(CPPFLAGS) $(ARGS) -c -o $@ $< -MD

always:
#	echo "#include \"info.h\"" > src/info.cc
	@echo "const char* builttime = \"`date +%y%m%d%H%M%S`\";" > src/info.cc
	@echo "const char* kernel=\"`uname -r`\";" >> src/info.cc
	@echo "const char* revision=\"\";" >> src/info.cc
	@echo "const char* buildflags=\"$(CPP) $(CPPFLAGS) $(LDFLAGS) $(CMD) $(ARGS)\";" >> src/info.cc
	@echo "const char* hostname=\"`hostname`\";" >> src/info.cc

# clean up backups and old files
clean:
	@rm -f *~ */*~ */*/*~ 
	@rm -f $(OBJS) $(DEPS) $(LIBOBJS) $(LIBFILES) $(LIBRISSOBJ)
	@rm -f src/*/*.lo
	@rm -f coprocessor
	@rm -f log.txt
	@echo "" > src/info.cc
	@cd doc; rm -fR html
	@echo "done"

# doxygen doc
doc:	clean
	cd doc; doxygen solver.config
	
# include headerfile dependencies for sources
-include $(DEPS)

