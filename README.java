A note on the Java client utilites:
-----------------------------------

They were written by Esben Ostergaard and questions should
go to him (esben@robotics.usc.edu).

We compiled with JDK 1.3, as available from Sun.

Not everybody has Java, so the makefiles will quietly ignore
any problems having to do with building the Java client utilites.

However, if you do have Java, make sure your JAVA_HOME environment
variable is set correctly (e.g. /usr/java/jdk1.3). We use this
variable in the Java examples Makefile.

The client class PlayerClient.class will be installed in 
/usr/local/player/lib, so you might find it useful to add that 
directory (and the current directory, while you're at it)
to your CLASSPATH:
  (ksh/zsh/bash):
    export CLASSPATH=$JAVA_HOME/lib:/usr/local/player/lib:.
  (csh/tcsh):
    setenv CLASSPATH $JAVA_HOME/lib:/usr/local/player/lib:.

    best of luck,
    brian.


