#
# $Id$
#

include ../Makefile.common

all:
	cd c && make all
	cd c++ && make all
	cd tcl && make all
	cd libplayerc && make all


install:
	cd c && make install
	cd c++ && make install
	cd tcl && make install
	cd libplayerc && make install


uninstall:
	cd c && make uninstall
	cd c++ && make uninstall
	cd tcl && make uninstall
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_LIB)
	cd libplayerc && make uninstall


dep:
	cd c && make dep
	cd c++ && make dep
	cd tcl && make dep
	cd libplayerc && make dep

clean:
	cd c && make clean
	cd c++ && make clean
	cd tcl && make clean
	cd libplayerc && make clean

clean_dep:
	cd c && make clean_dep
	cd c++ && make clean_dep
	cd tcl && make clean_dep
	cd libplayerc && make clean_dep

