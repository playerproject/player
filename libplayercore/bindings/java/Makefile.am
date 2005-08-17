AM_CPPFLAGS = -g -Wall -fPIC @JNI_CFLAGS@

# files that must be built first
BUILT_SOURCES = playercore_java_wrap.cc
SWIG_SOURCES = playercore_java.i

EXTRA_DIST = $(SWIG_SOURCES)
CLEANFILES = playercore_java_wrap.cc

pkglibdir = $(libdir)/player-@VERSION@
pkglib_LTLIBRARIES = libplayercore_java.la

playercore_java_wrap.cc: $(SWIG_SOURCES) ../../player.h
	swig -v -java -c++ -o $@ -I../../.. $<

all-local:
	javac *.java

libplayercore_java_la_SOURCES = playercore_java_wrap.cc
libplayercore_java_la_LIBADD = $(top_builddir)/libplayercore/libplayercore.la $(top_builddir)/server/libplayerdrivers/libplayerdrivers.la
#libplayercore_java_la_LDFLAGS = $(top_builddir)/libplayercore/*.lo `find $(top_builddir)/server/drivers -name "*.lo"`

clean-local:
	mv main.java main.java.bak
	rm -f *.class *.java
	mv main.java.bak main.java
