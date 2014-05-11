##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

CXXFLAGS:=-DENABLE_THREADS -emit-llvm -g -O2 -fno-vectorize -fno-slp-vectorize
CXX:=clang++

all: annealer_thread.cpp rng.cpp netlist.cpp main.cpp netlist_elem.cpp
	@$(CXX) $(CXXFLAGS)  annealer_thread.cpp -c -o annealer_thread.bc
	@$(CXX) $(CXXFLAGS)  rng.cpp -c -o rng.bc
	@$(CXX) $(CXXFLAGS)  netlist.cpp -c -o netlist.bc 
	@$(CXX) $(CXXFLAGS)  main.cpp -c -o main.bc
	@$(CXX) $(CXXFLAGS)  netlist_elem.cpp -c -o netlist_elem.bc  
	@llvm-link *.bc -o canneal.bc

clean:
	@$(RM) *.bc *.o *.ll 

install:
	@echo -n ""
