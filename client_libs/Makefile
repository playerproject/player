#
# $Id$
#

include ../Makefile.common

all:
	cd c && make all
	cd c++ && make all
	cd oldc++ && make all
	cd tcl && make all


install:
	cd c && make install
	cd c++ && make install
	cd oldc++ && make install
	cd tcl && make install


uninstall:
	cd c && make uninstall
	cd oldc++ && make uninstall
	cd c++ && make uninstall
	cd tcl && make uninstall
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_LIB)


dep:
	cd c && make dep
	cd oldc++ && make dep
	cd c++ && make dep
	cd tcl && make dep
	
clean:
	cd c && make clean
	cd oldc++ && make clean
	cd c++ && make clean
	cd tcl && make clean

clean_dep:
	cd c && make clean_dep
	cd oldc++ && make clean_dep
	cd c++ && make clean_dep
	cd tcl && make clean_dep

