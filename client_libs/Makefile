RMDIR = rmdir
INSTALL_LIB = $(INSTALL_PREFIX)/lib

all:
	cd c++ && make all
	cd tcl && make all
	cd java && make -i all

install:
	cd c++ && make INSTALL_PREFIX=$(INSTALL_PREFIX) install
	cd tcl && make INSTALL_PREFIX=$(INSTALL_PREFIX) install
	cd java && make INSTALL_PREFIX=$(INSTALL_PREFIX) -i install

uninstall:
	cd c++ && make INSTALL_PREFIX=$(INSTALL_PREFIX) uninstall
	cd tcl && make INSTALL_PREFIX=$(INSTALL_PREFIX) uninstall
	cd java && make INSTALL_PREFIX=$(INSTALL_PREFIX) -i uninstall
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_LIB)

clean:
	cd c++ && make clean
	cd tcl && make clean
	cd java && make clean
