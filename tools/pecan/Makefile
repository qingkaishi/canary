LEVEL = ../..
TOOLNAME = pecan
#LLVMLIBS = bcreader bcwriter instrument \
#           profpaths scalaropts \
#           ipo ipa.a datastructure.a transforms target.a analysis \
#	   transformutils vmcore support cwriter
LINK_COMPONENTS := bitreader bitwriter asmparser irreader instrumentation scalaropts objcarcopts ipo vectorize all-targets
USEDLIBS = dyckaa.a dyckgraph.a transformer.a wrapper.a
#TOOLLINKOPTSB = -L/home/vadve/dhurjati/libs
#TOOLLINKOPTS = -ldl -lm 
include $(LEVEL)/Makefile.common
