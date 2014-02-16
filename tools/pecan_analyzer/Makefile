LEVEL=../..

include $(LEVEL)/Makefile.common

all:: Pecan.cpp
	$(CXX) Pecan.cpp -o pecan_log_analyzer -I$(LEVEL)/include

install-local:: all
	cp pecan_log_analyzer /usr/local/bin

uninstall-local::
	if [ -f /usr/local/bin/pecan_log_analyzer ]; then rm /usr/local/bin/pecan_log_analyzer; fi 
