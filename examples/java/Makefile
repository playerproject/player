#
# $Id$
#

include ../../Makefile.common

JAVAC = javac

GOLEMCLASSPATH = -classpath ../../client_libs/java:$(INSTALL_PREFIX)/lib:$(JAVA_HOME)/lib

BINARIES =  ObstacleAvoidanceUsingLaser.class \
            ObstacleAvoidanceUsingSonar.class

all: $(BINARIES)

%.class: %.java
	$(JAVAC) $(GOLEMCLASSPATH) $<

install: $(BINARIES)
	$(MKDIR) -p $(INSTALL_EXAMPLES)/java
	$(INSTALL) -m 644 ObstacleAvoidanceUsingSonar.java $(INSTALL_EXAMPLES)/java
	$(INSTALL) -m 644 ObstacleAvoidanceUsingSonar.class $(INSTALL_EXAMPLES)/java
	$(INSTALL) -m 644 ObstacleAvoidanceUsingLaser.java $(INSTALL_EXAMPLES)/java
	$(INSTALL) -m 644 ObstacleAvoidanceUsingLaser.class $(INSTALL_EXAMPLES)/java

uninstall: 
	$(RM) -f $(INSTALL_EXAMPLES)/java/ObstacleAvoidanceUsingSonar.java
	$(RM) -f $(INSTALL_EXAMPLES)/java/ObstacleAvoidanceUsingSonar.class
	$(RM) -f $(INSTALL_EXAMPLES)/java/ObstacleAvoidanceUsingLaser.java
	$(RM) -f $(INSTALL_EXAMPLES)/java/ObstacleAvoidanceUsingLaser.class
	$(RM) -f $(INSTALL_EXAMPLES)/java/PlayerClient.class

clean:
	$(RM) -f *.class
