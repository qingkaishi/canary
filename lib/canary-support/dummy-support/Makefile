##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../../..

#
# Give the name of a library.  This will build a dynamic version.
#
LIBRARYNAME=dummyrecord


#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

CFLAGS += -DNO_TIME_CMD
CXXFLAGS += -DNO_TIME_CMD

ifeq (${DEBUG}, 1)
    CFLAGS += -DDEBUG
    CXXFLAGS += -DDEBUG
endif

ifeq (${LDEBUG}, 1)
    CFLAGS += -DLDEBUG
    CXXFLAGS += -DLDEBUG
endif

ifeq (${CACHE}, 1)
    CFLAGS += -DCACHE_ENABLED
    CXXFLAGS += -DCACHE_ENABLED
endif
