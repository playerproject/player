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
INSTALL_EXAMPLES = $(INSTALL_PREFIX)/examples/python

all:

install: 
	$(MKDIR) -p $(INSTALL_EXAMPLES)
	$(INSTALL) -m 755 viewer.py $(INSTALL_EXAMPLES)
	$(MKDIR) -p $(INSTALL_BIN)
	$(LN) -sf $(INSTALL_EXAMPLES)/viewer.py $(INSTALL_BIN)

uninstall: 
	$(RM) -f $(INSTALL_EXAMPLES)/viewer.py
	$(RM) -f $(INSTALL_BIN)/viewer.py

clean:

