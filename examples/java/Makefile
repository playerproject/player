JAVAC = javac
RM = rm
MKDIR = mkdir
RMDIR = rmdir
INSTALL = install

#INSTALL_PREFIX = /usr/local/golem

GOLEMCLASSPATH = -classpath ../../client_libs/java:$(INSTALL_PREFIX)/lib:$(JAVA_HOME)/lib

INSTALL_EXAMPLES = $(INSTALL_PREFIX)/examples/java

BINARIES =  ObstacleAvoidanceUsingLaser.class \
            ObstacleAvoidanceUsingSonar.class

all: $(BINARIES)

%.class: %.java
	$(JAVAC) $(GOLEMCLASSPATH) $<

install: $(BINARIES)
	$(MKDIR) -p $(INSTALL_EXAMPLES)
	$(INSTALL) -m 644 ObstacleAvoidanceUsingSonar.java $(INSTALL_EXAMPLES)
	$(INSTALL) -m 644 ObstacleAvoidanceUsingSonar.class $(INSTALL_EXAMPLES)
	$(INSTALL) -m 644 ObstacleAvoidanceUsingLaser.java $(INSTALL_EXAMPLES)
	$(INSTALL) -m 644 ObstacleAvoidanceUsingLaser.class $(INSTALL_EXAMPLES)

uninstall: 
	$(RM) -f $(INSTALL_EXAMPLES)/ObstacleAvoidanceUsingSonar.java
	$(RM) -f $(INSTALL_EXAMPLES)/ObstacleAvoidanceUsingSonar.class
	$(RM) -f $(INSTALL_EXAMPLES)/ObstacleAvoidanceUsingLaser.java
	$(RM) -f $(INSTALL_EXAMPLES)/ObstacleAvoidanceUsingLaser.class
	$(RM) -f $(INSTALL_EXAMPLES)/PlayerClient.class

clean:
	$(RM) -f *.class
