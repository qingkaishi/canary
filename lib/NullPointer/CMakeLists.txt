add_library(CanaryNullPointer STATIC
        LocalNullCheckAnalysis.cpp
        NullBoosterPass.cpp
        NullCheckAnalysis.cpp
        NullEquivalenceAnalysis.cpp
)