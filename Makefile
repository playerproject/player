#
# $Id$
#

# DO NOT make configuration changes here!.  Make them in Makefile.common.

include Makefile.common

MANUAL_LOCATION = player-manual

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
	cd client_libs && make dep
	cd examples && make dep

install: install_server install_client_libs install_examples install_doc

install_server:
	cd src && make install

install_client_libs:
	cd client_libs && make install

install_examples:
	cd examples && make install

install_doc:
	$(MKDIR) -p $(INSTALL_DOC)
	$(INSTALL) -m 644 doc/* $(INSTALL_DOC)

uninstall:
	cd src && make uninstall
	cd client_libs && make uninstall
	cd examples && make -i uninstall
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_BIN) 
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_INCLUDE)
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_PREFIX)
	$(RM) -f $(INSTALL_DOC)/*
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_DOC)
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_PREFIX)

distro: clean
	$(MKDIR) -p doc
	cd client_libs/c++/doc && make manual
	cd $(MANUAL_LOCATION) && make ps && cp *.ps $(PWD)/doc
	cd .. && $(PWD)/distro.sh `echo $(PWD) | awk -F "/" '{print $$NF}'` $(PLAYER_VERSION)

distro_bleeding: clean
	$(MKDIR) -p doc
	cd client_libs/c++/doc && make manual
	cd $(MANUAL_LOCATION) && make ps && cp *.ps $(PWD)/doc
	cd .. && $(PWD)/distro.sh `echo $(PWD) | awk -F "/" '{print $$NF}'` Bleeding


clean_server: 
	cd src && make clean

clean: clean_server clean_dep
	cd client_libs && make clean
	cd examples && make clean

clean_dep:
	cd src && make clean_dep
	cd client_libs && make clean_dep
	cd examples && make clean_dep



