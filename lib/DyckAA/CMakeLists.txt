cmake_minimum_required(VERSION 2.8)
add_library (CanaryDyckAA STATIC DyckAliasAnalysis.cpp AAAnalyzer.cpp EdgeLabel.cpp ProgressBar.cpp)
include_directories (${INCLUDE_DIR}/DyckAA)
