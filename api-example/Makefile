WGT = int
LIBDIR = ../lib # location of libLMHS-$(WGT).so
INCDIR = ../include # location of LMHS_C_API.h
c:
	gcc -I$(INCDIR) example.c -L$(LIBDIR) -lLMHS-$(WGT) -Wl,-rpath,$(LIBDIR) -o c-example
cpp:
	g++ -std=c++11 -I$(INCDIR) example.cpp -L$(LIBDIR) -lLMHS-$(WGT) -Wl,-rpath,$(LIBDIR) -o cpp-example
incremental:
	g++ -std=c++11 -I$(INCDIR) incremental.cpp -L$(LIBDIR) -lLMHS-$(WGT) -Wl,-rpath,$(LIBDIR) -o incremental-example