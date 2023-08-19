add_library(CanaryNullPointer STATIC
        LocalNullCheckAnalysis.cpp
        NullCheckAnalysis.cpp
        NullEquivalenceAnalysis.cpp
        NullFlowAnalysis.cpp
)