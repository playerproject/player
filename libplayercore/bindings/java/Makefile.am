AM_CPPFLAGS = -g -Wall -fPIC @JNI_CFLAGS@

# files that must be built first
BUILT_SOURCES =
if ENABLE_JAVA
BUILT_SOURCES += playercore_java_wrap.cc
endif
SWIG_SOURCES = playercore_java.i

EXTRA_DIST = $(SWIG_SOURCES)
CLEANFILES = playercore_java_wrap.cc

pkglibdir = $(libdir)/player-@VERSION@
pkglib_LTLIBRARIES =
if ENABLE_JAVA
pkglib_LTLIBRARIES += libplayercore_java.la
endif

playercore_java_wrap.cc: $(SWIG_SOURCES) ../../player.h
	swig -v -java -c++ -o $@ -I../../.. $<

#all-local:
#	javac *.java

libplayercore_java_la_SOURCES = playercore_java_wrap.cc
libplayercore_java_la_LDFLAGS = $(top_builddir)/libplayercore/*.lo $(top_builddir)/server/libplayerdrivers/*.lo `find $(top_builddir)/server/drivers -name "*.lo"`

if DARWIN
install-exec-hook:
	ln -sf $(pkglibdir)/libplayercore_java.dylib $(pkglibdir)/libplayercore_java.jnilib 
endif

clean-local:
	mv main.java main.java.bak
	rm -f *.class *.java
	mv main.java.bak main.java
