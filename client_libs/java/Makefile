JAVAC = javac
LN = ln
RM = rm
INSTALL = install
MKDIR = mkdir
RMDIR = rmdir

INSTALL_LIB = $(INSTALL_PREFIX)/lib
#INSTALL_EXAMPLES = $(INSTALL_PREFIX)/examples/java

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

