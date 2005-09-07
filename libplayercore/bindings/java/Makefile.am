AM_CPPFLAGS = -g -Wall -fPIC @JNI_CFLAGS@

# files that must be built first
BUILT_SOURCES =
if ENABLE_JAVA
BUILT_SOURCES += playercore_java_wrap.cc playercore_casts.i playercore_arraysofclasses.i
endif

EXTRA_DIST = playercore_java.i parse.py
CLEANFILES = playercore_java_wrap.cc playercore_casts.i playercore_arraysofclasses.i

lib_LTLIBRARIES =
if ENABLE_JAVA
lib_LTLIBRARIES += libplayercore_java.la
endif

playercore_arraysofclasses.i: playercore_casts.i

playercore_casts.i: $(srcdir)/../../player.h parse.py
	python $(srcdir)/parse.py $(srcdir)/../../player.h playercore_casts.i playercore_arraysofclasses.i net/sourceforge/playerstage/Jplayercore net/sourceforge/playerstage/Jplayercore player.java


playercore_java_wrap.cc: playercore_java.i playercore_casts.i \
                         playercore_arraysofclasses.i\
                         $(srcdir)/../../configfile.h \
                         $(srcdir)/../../device.h \
                         $(srcdir)/../../devicetable.h \
                         $(srcdir)/../../driver.h \
                         $(srcdir)/../../drivertable.h \
                         $(srcdir)/../../error.h \
                         $(srcdir)/../../globals.h \
                         $(srcdir)/../../message.h \
                         $(srcdir)/../../player.h \
                         $(srcdir)/../../playercommon.h \
                         $(srcdir)/../../playertime.h \
                         $(srcdir)/../../addr_util.h
	swig -v -java -package net.sourceforge.playerstage.Jplayercore -outdir net/sourceforge/playerstage/Jplayercore -c++ -o $@ -I$(srcdir)/../../.. $<

#all-local:
#	javac *.java

libplayercore_java_la_SOURCES = playercore_java_wrap.cc
libplayercore_java_la_LDFLAGS = $(top_builddir)/libplayercore/libplayererror.la $(top_builddir)/libplayercore/libplayercore.la $(top_builddir)/server/libplayerdrivers/libplayerdrivers.la -version-info 2:0:0 @PLAYER_DRIVER_EXTRA_LIBS@

if DARWIN
install-exec-hook:
	ln -sf $(libdir)/libplayercore_java.dylib $(libdir)/libplayercore_java.jnilib 
endif

clean-local:
	rm -rf net
