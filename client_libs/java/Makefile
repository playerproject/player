#
# $Id$
#

include ../../Makefile.common

JAVAC = javac

all: PlayerClient.class

PlayerClient.class: PlayerClient.java
	$(JAVAC) PlayerClient.java

install: PlayerClient.class
	$(MKDIR) -p $(INSTALL_LIB)
	$(INSTALL) -m 644 *.class $(INSTALL_LIB)

uninstall:
	$(RM) -f $(INSTALL_LIB)/*.class

clean:
	$(RM) -f *.class

