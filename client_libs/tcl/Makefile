INSTALL = install
MKDIR = mkdir
RM = rm
RMDIR = rmdir

INSTALL_LIB = $(INSTALL_PREFIX)/lib

all: 

install:
	$(MKDIR) -p $(INSTALL_LIB)
	$(INSTALL) -m 644 playerclient.tcl $(INSTALL_LIB)

uninstall:
	$(RM) -f $(INSTALL_LIB)/playerclient.tcl 

clean: 
