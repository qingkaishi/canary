#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include alias-Makefile.mk

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/lib/dyckaa/AAAnalyzer.o \
	${OBJECTDIR}/lib/dyckaa/DyckAliasAnalysis.o \
	${OBJECTDIR}/lib/dyckaa/Transformer4Replay.o \
	${OBJECTDIR}/lib/dyckaa/Transformer4Trace.o \
	${OBJECTDIR}/lib/dyckgraph/DyckGraph.o \
	${OBJECTDIR}/lib/dyckgraph/DyckVertex.o \
	${OBJECTDIR}/lib/transformer/Transformer.o \
	${OBJECTDIR}/lib/wrapper/FunctionWrapper.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/alias

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/alias: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/alias ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/lib/dyckaa/AAAnalyzer.o: lib/dyckaa/AAAnalyzer.cpp 
	${MKDIR} -p ${OBJECTDIR}/lib/dyckaa
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/lib/dyckaa/AAAnalyzer.o lib/dyckaa/AAAnalyzer.cpp

${OBJECTDIR}/lib/dyckaa/DyckAliasAnalysis.o: lib/dyckaa/DyckAliasAnalysis.cpp 
	${MKDIR} -p ${OBJECTDIR}/lib/dyckaa
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/lib/dyckaa/DyckAliasAnalysis.o lib/dyckaa/DyckAliasAnalysis.cpp

${OBJECTDIR}/lib/dyckaa/Transformer4Replay.o: lib/dyckaa/Transformer4Replay.cpp 
	${MKDIR} -p ${OBJECTDIR}/lib/dyckaa
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/lib/dyckaa/Transformer4Replay.o lib/dyckaa/Transformer4Replay.cpp

${OBJECTDIR}/lib/dyckaa/Transformer4Trace.o: lib/dyckaa/Transformer4Trace.cpp 
	${MKDIR} -p ${OBJECTDIR}/lib/dyckaa
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/lib/dyckaa/Transformer4Trace.o lib/dyckaa/Transformer4Trace.cpp

${OBJECTDIR}/lib/dyckgraph/DyckGraph.o: lib/dyckgraph/DyckGraph.cpp 
	${MKDIR} -p ${OBJECTDIR}/lib/dyckgraph
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/lib/dyckgraph/DyckGraph.o lib/dyckgraph/DyckGraph.cpp

${OBJECTDIR}/lib/dyckgraph/DyckVertex.o: lib/dyckgraph/DyckVertex.cpp 
	${MKDIR} -p ${OBJECTDIR}/lib/dyckgraph
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/lib/dyckgraph/DyckVertex.o lib/dyckgraph/DyckVertex.cpp

${OBJECTDIR}/lib/transformer/Transformer.o: lib/transformer/Transformer.cpp 
	${MKDIR} -p ${OBJECTDIR}/lib/transformer
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/lib/transformer/Transformer.o lib/transformer/Transformer.cpp

${OBJECTDIR}/lib/wrapper/FunctionWrapper.o: lib/wrapper/FunctionWrapper.cpp 
	${MKDIR} -p ${OBJECTDIR}/lib/wrapper
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/lib/wrapper/FunctionWrapper.o lib/wrapper/FunctionWrapper.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/alias

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
