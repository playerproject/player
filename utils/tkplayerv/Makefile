###########################################################################
# Desc: Makefile for Player Viewer
# Author: Brian Gerkey
# Date: May 29 2002
# CVS: $Id$
###########################################################################

include ../../Makefile.common

###########################################################################
# Build section
###########################################################################

all:

dep:

clean_dep:

clean: 




###########################################################################
# Install
###########################################################################
install:
	$(MKDIR) -p $(INSTALL_UTILS)/tkplayerv
	$(INSTALL) viewer.tk $(INSTALL_UTILS)/tkplayerv
	$(INSTALL) README $(INSTALL_UTILS)/tkplayerv

uninstall:
	$(RM) -f $(INSTALL_UTILS)/tkplayerv/viewer.tk
	$(RM) -f $(INSTALL_UTILS)/tkplayerv/README
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_UTILS)/tkplayerv
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_UTILS)

