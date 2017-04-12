cmake_minimum_required(VERSION 2.8)
include_directories(${INCLUDE_DIR}/DyckCG)
add_library(DyckCG STATIC DyckCallGraph.cpp DyckCallGraphNode.cpp)
