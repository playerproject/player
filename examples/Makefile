RMDIR = rmdir
INSTALL_EXAMPLES = $(INSTALL_PREFIX)/examples

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
	cd java && make INSTALL_PREFIX=$(INSTALL_PREFIX) uninstall
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_EXAMPLES)/tcl
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_EXAMPLES)/java
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_EXAMPLES)/c++
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_EXAMPLES)

clean:
	cd c++ && make clean
	cd tcl && make clean
	cd java && make clean
