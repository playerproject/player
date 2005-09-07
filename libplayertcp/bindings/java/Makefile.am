AM_CPPFLAGS = -g -Wall -fPIC @JNI_CFLAGS@

# files that must be built first
BUILT_SOURCES =
if ENABLE_JAVA
BUILT_SOURCES += playertcp_java_wrap.cc
endif

EXTRA_DIST = playertcp_java.i
CLEANFILES = playertcp_java_wrap.cc

lib_LTLIBRARIES =
if ENABLE_JAVA
lib_LTLIBRARIES += libplayertcp_java.la
endif

playertcp_java_wrap.cc: playertcp_java.i \
                        $(srcdir)/../../playertcp.h
	mkdir -p net/sourceforge/playerstage/Jplayertcp
	swig -v -java -package net.sourceforge.playerstage.Jplayertcp -outdir net/sourceforge/playerstage/Jplayertcp -c++ -o $@ -I$(srcdir)/../../.. $<

libplayertcp_java_la_SOURCES = playertcp_java_wrap.cc
libplayertcp_java_la_LDFLAGS = $(top_builddir)/libplayertcp/libplayertcp.la $(top_builddir)/libplayerxdr/libplayerxdr.la $(top_builddir)/libplayercore/libplayercore.la $(top_builddir)/server/libplayerdrivers/libplayerdrivers.la $(top_builddir)/libplayercore/libplayererror.la -version-info 2:0:0

if DARWIN
install-exec-hook:
	ln -sf $(libdir)/libplayertcp_java.dylib $(libdir)/libplayertcp_java.jnilib 
endif

clean-local:
	rm -rf net
