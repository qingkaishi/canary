LEVEL=../..

include $(LEVEL)/Makefile.common

Scripts = $(wildcard *.sh)

all:: Pecan.cpp
	$(CXX) Pecan.cpp -o pecan_log_analyzer -I../../include

install-local:: all
	$(Verb) for script in $(Scripts) ; do \
		echo Installing $$script ; \
		$(MKDIR) $(DESTDIR)$(PROJ_bindir) ; \
		$(ScriptInstall) $(PROJ_SRC_DIR)/$$script $(DESTDIR)$(PROJ_bindir) ; \
	done
	cp pecan_log_analyzer /usr/local/bin

uninstall-local::
	$(Verb) $(RM) -f $(addprefix $(DESTDIR)$(PROJ_bindir)/,$(Scripts))
	rm /usr/local/bin/Pecan
