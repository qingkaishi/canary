cmake_minimum_required(VERSION 2.8)
include_directories(${INCLUDE_DIR}/DyckGraph)
add_library(DyckGraph STATIC DyckGraph.cpp DyckVertex.cpp)
