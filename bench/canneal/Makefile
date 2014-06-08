##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

CXXFLAGS:=-DENABLE_THREADS -emit-llvm -g -O2 -fno-vectorize -fno-slp-vectorize
CXX:=clang++

all: canneal.bc

canneal.bc: annealer_thread.bc rng.bc netlist.bc main.bc netlist_elem.bc
	@llvm-link annealer_thread.bc rng.bc netlist.bc main.bc netlist_elem.bc -o canneal.bc

annealer_thread.bc: annealer_thread.cpp
	@$(CXX) $(CXXFLAGS)  annealer_thread.cpp -c -o annealer_thread.bc

rng.bc: rng.cpp
	@$(CXX) $(CXXFLAGS)  rng.cpp -c -o rng.bc

netlist.bc: netlist.cpp
	@$(CXX) $(CXXFLAGS)  netlist.cpp -c -o netlist.bc

main.bc: main.cpp
	@$(CXX) $(CXXFLAGS)  main.cpp -c -o main.bc

netlist_elem.bc: netlist_elem.cpp
	@$(CXX) $(CXXFLAGS)  netlist_elem.cpp -c -o netlist_elem.bc


clean:
	@$(RM) *.bc *.o *.ll *.exe canary.zip 

install:
	@echo -n ""
