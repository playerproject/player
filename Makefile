#
# $Id$
#

include Makefile.common

MANUAL_LOCATION = player-manual
PWD = $(shell pwd)

all: server client_libs examples

server: 
	cd src && make all

.PHONY: client_libs
client_libs:
	cd client_libs && make all

.PHONY: examples
examples:
	cd examples && make -i all

dep:
	cd src && make dep

install: install_server install_client_libs

install_server:
	cd src && make install

install_client_libs:
	cd client_libs && make install

install_examples:
	cd examples && make install

uninstall:
	cd src && make uninstall
	cd client_libs && make uninstall
	cd examples && make -i uninstall
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_BIN) 
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_INCLUDE)
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_PREFIX)


#manual:
	#cd tex && make install

distro: clean
	$(MKDIR) doc
	cd $(MANUAL_LOCATION) && make ps && cp *.ps $(PWD)/doc
	cd .. && $(PWD)/distro.sh `echo $(PWD) | awk -F "/" '{print $$NF}'` $(PLAYER_VERSION)

distro-bleeding: clean
	$(MKDIR) doc
	cd $(MANUAL_LOCATION) && make ps && cp *.ps $(PWD)/doc
	cd .. && $(PWD)/distro.sh `echo $(PWD) | awk -F "/" '{print $$NF}'` Bleeding


clean_server: 
	cd src && make clean

clean: clean_server
	$(RM) -rf doc
	cd client_libs && make clean
	cd examples && make clean
