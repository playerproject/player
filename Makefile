#
# $Id$
#

#
# this is where player will be installed.  change it if you like.
INSTALL_PREFIX = /usr/local/player
INSTALL_BIN = $(INSTALL_PREFIX)/bin
INSTALL_INCLUDE = $(INSTALL_PREFIX)/include
RMDIR = rmdir

all: server client_libs examples

server: 
	cd src && make all

.PHONY: client_libs
client_libs:
	cd client_libs && make all

.PHONY: examples
examples:
	cd examples && make all

dep:
	cd src && make dep

install: install_server install_client_libs

install_server:
	cd src && make INSTALL_PREFIX=$(INSTALL_PREFIX) install

install_client_libs:
	cd client_libs && make INSTALL_PREFIX=$(INSTALL_PREFIX) install

install_examples:
	cd examples && make INSTALL_PREFIX=$(INSTALL_PREFIX) install

uninstall:
	cd src && make INSTALL_PREFIX=$(INSTALL_PREFIX) uninstall
	cd client_libs && make INSTALL_PREFIX=$(INSTALL_PREFIX) uninstall
	cd examples && make INSTALL_PREFIX=$(INSTALL_PREFIX) uninstall
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_BIN) 
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_INCLUDE)
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_PREFIX)


manual:
	cd tex && make install

distro: clean manual
	cd .. && $(PWD)/distro.sh `echo $(PWD) | awk -F "/" '{print $$NF}'` `awk '{print substr($$3,2,length($$3)-2);}' $(PWD)/VERSION`


clean:
	cd src && make clean
	cd client_libs && make clean
	cd examples && make clean

veryclean: clean
	cd tex && make clean
	


