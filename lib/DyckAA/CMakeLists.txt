cmake_minimum_required(VERSION 2.8)
add_library (DyckAA STATIC DyckAliasAnalysis.cpp AAAnalyzer.cpp EdgeLabel.cpp ProgressBar.cpp)
include_directories (DyckAA ${INCLUDE_DIR}/DyckAA)
