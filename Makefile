#
# $Id$
#

#
# player will be installed to this dir with VERSION appended
# (e.g. '/usr/local/player0.8c'). change the base directory name
# if you want, but try to leave the version identifier on there; makes
# things easier
#
PWD=$(shell pwd)
INSTALL_BASE = /usr/local/player
# get version from VERSION file
VERSION=$(shell awk '{print substr($$3,2,length($$3)-2);}' $(PWD)/VERSION)
INSTALL_PREFIX=$(INSTALL_BASE)$(VERSION)

INSTALL_BIN = $(INSTALL_PREFIX)/bin
INSTALL_INCLUDE = $(INSTALL_PREFIX)/include

RMDIR = rmdir

#######################################
# Platform selection
#

# for Linux, uncomment the following line
PLAYER_PLATFORM = -DPLAYER_LINUX

# for Solaris, uncomment the following 2 lines
#PLAYER_PLATFORM = -DPLAYER_SOLARIS
#PLATFORM_LIBS = -lxnet
#######################################

all: server client_libs examples

server: 
	cd src && make PLAYER_PLATFORM=$(PLAYER_PLATFORM) PLATFORM_LIBS=$(PLATFORM_LIBS) all

.PHONY: client_libs
client_libs:
	cd client_libs && make PLAYER_PLATFORM=$(PLAYER_PLATFORM) PLATFORM_LIBS=$(PLATFORM_LIBS) all

.PHONY: examples
examples:
	cd examples && make -i PLAYER_PLATFORM=$(PLAYER_PLATFORM) PLATFORM_LIBS=$(PLATFORM_LIBS) all

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
	cd examples && make -i INSTALL_PREFIX=$(INSTALL_PREFIX) uninstall
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_BIN) 
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_INCLUDE)
	$(RMDIR) --ignore-fail-on-non-empty $(INSTALL_PREFIX)


#manual:
	#cd tex && make install

distro: clean
	cd .. && $(PWD)/distro.sh `echo $(PWD) | awk -F "/" '{print $$NF}'` `awk '{print substr($$3,2,length($$3)-2);}' $(PWD)/VERSION`


clean:
	cd src && make clean
	cd client_libs && make clean
	cd examples && make clean
