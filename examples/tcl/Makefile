#
# $Id$
#

include ../../Makefile.common

all: ptz.tcl sonarobstacleavoid.tcl viewer.tk


#
# put the right version number in
#
%.tcl: force
	${SED} s/"set PLAYER_VERSION.*"/"set PLAYER_VERSION ${PLAYER_VERSION}"/ < $@ > $@.tmp
	${MV} $@.tmp $@
	${CHMOD} 755 $@ 

%.tk: force
	${SED} s/"set PLAYER_VERSION.*"/"set PLAYER_VERSION ${PLAYER_VERSION}"/ < $@ > $@.tmp
	${MV} $@.tmp $@
	${CHMOD} 755 $@ 

force:

install: 
	$(MKDIR) -p $(INSTALL_EXAMPLES)/tcl
	$(INSTALL) -m 755 viewer.tk $(INSTALL_EXAMPLES)/tcl
	$(INSTALL) -m 755 ptz.tcl $(INSTALL_EXAMPLES)/tcl
	$(INSTALL) -m 755 sonarobstacleavoid.tcl $(INSTALL_EXAMPLES)/tcl
	$(MKDIR) -p $(INSTALL_BIN)
	$(LN) -sf $(INSTALL_EXAMPLES)/tcl/viewer.tk $(INSTALL_BIN)

uninstall: 
	$(RM) -f $(INSTALL_EXAMPLES)/tcl/viewer.tk
	$(RM) -f $(INSTALL_EXAMPLES)/tcl/ptz.tcl
	$(RM) -f $(INSTALL_EXAMPLES)/tcl/sonarobstacleavoid.tcl
	$(RM) -f $(INSTALL_BIN)/viewer.tk

clean:

