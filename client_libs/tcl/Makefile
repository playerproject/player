#
# $Id$
#

include ../../Makefile.common

TCLLIBDIR = $(INSTALL_LIB)/tclPlayer

# make the package index file so the client utils can be loaded via 
# 'package require'
all: 
	echo "pkg_mkIndex ." | tclsh

install:
	$(MKDIR) -p $(TCLLIBDIR)
	$(INSTALL) -m 644 playerclient.tcl $(TCLLIBDIR)
	$(INSTALL) -m 644 pkgIndex.tcl $(TCLLIBDIR)

uninstall:
	$(RM) -f $(TCLLIBDIR)/playerclient.tcl 
	$(RM) -f $(TCLLIBDIR)/pkgIndex.tcl 

clean:
	$(RM) -f pkgIndex.tcl

dep:

clean_dep:

