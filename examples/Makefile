#
# $Id$
#

include ../Makefile.common

all:
	cd c && make all
	cd c++ && make all
	cd tcl && make all

#cd java && make -i all

install:
	cd c && make install
	cd c++ && make install
	cd tcl && make install

#cd java && make -i install

uninstall:
	cd c && make uninstall
	cd c++ && make uninstall
	cd tcl && make uninstall
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_EXAMPLES)/tcl
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_EXAMPLES)/c++
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_EXAMPLES)/c
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_EXAMPLES)

#cd java && make uninstall
#$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_EXAMPLES)/java

clean:
	cd c && make clean
	cd c++ && make clean
	cd tcl && make clean

#cd java && make clean
