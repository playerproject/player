#
# $Id$
#

include ../../Makefile.common

all:

install: 
	$(MKDIR) -p $(INSTALL_EXAMPLES)/tcl
	$(INSTALL) -m 755 viewer.tk $(INSTALL_EXAMPLES)/tcl
	$(INSTALL) -m 755 ptz.tcl $(INSTALL_EXAMPLES)/tcl
	$(MKDIR) -p $(INSTALL_BIN)
	$(LN) -sf $(INSTALL_EXAMPLES)/tcl/viewer.tk $(INSTALL_BIN)

uninstall: 
	$(RM) -f $(INSTALL_EXAMPLES)/tcl/viewer.tk
	$(RM) -f $(INSTALL_EXAMPLES)/tcl/ptz.tcl
	$(RM) -f $(INSTALL_BIN)/viewer.tk

clean:

