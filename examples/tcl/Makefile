#
# $Id$
#

CPP = g++
RM = rm
LN = ln
INSTALL = install
MKDIR = mkdir
RMDIR = rmdir

#INSTALL_PREFIX = /usr/local/golem
INSTALL_BIN = $(INSTALL_PREFIX)/bin
INSTALL_EXAMPLES = $(INSTALL_PREFIX)/examples/tcl

all:

install: 
	$(MKDIR) -p $(INSTALL_EXAMPLES)
	$(INSTALL) -m 755 viewer.tk $(INSTALL_EXAMPLES)
	$(INSTALL) -m 755 ptz.tcl $(INSTALL_EXAMPLES)
	$(MKDIR) -p $(INSTALL_BIN)
	$(LN) -sf $(INSTALL_EXAMPLES)/viewer.tk $(INSTALL_BIN)

uninstall: 
	$(RM) -f $(INSTALL_EXAMPLES)/viewer.tk
	$(RM) -f $(INSTALL_EXAMPLES)/ptz.tcl
	$(RM) -f $(INSTALL_BIN)/viewer.tk

clean:

