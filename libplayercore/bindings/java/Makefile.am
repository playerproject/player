AM_CPPFLAGS = -g -Wall -fPIC @JNI_CFLAGS@

# files that must be built first
BUILT_SOURCES =
if ENABLE_JAVA
BUILT_SOURCES += playercore_java_wrap.cc playercore_casts.i
endif

EXTRA_DIST = playercore_java.i parse.py
CLEANFILES = playercore_java_wrap.cc playercore_casts.i

lib_LTLIBRARIES =
if ENABLE_JAVA
lib_LTLIBRARIES += libplayercore_java.la
endif

playercore_casts.i: $(srcdir)/../../player.h
	python $(srcdir)/parse.py $(srcdir)/../../player.h $@

playercore_java_wrap.cc: playercore_java.i playercore_casts.i \
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
                         $(srcdir)/../../playertime.h
	mkdir -p playercore
	swig -v -java -package playercore -outdir playercore -c++ -o $@ -I$(srcdir)/../../.. $<

#all-local:
#	javac *.java

libplayercore_java_la_SOURCES = playercore_java_wrap.cc
libplayercore_java_la_LDFLAGS = $(top_builddir)/libplayercore/*.lo $(top_builddir)/server/libplayerdrivers/*.lo `find $(top_builddir)/server/drivers -name "*.lo"` -version-info 2:0:0

if DARWIN
install-exec-hook:
	ln -sf $(pkglibdir)/libplayercore_java.dylib $(pkglibdir)/libplayercore_java.jnilib 
endif

clean-local:
	rm -f playercore/*.class playercore/*.java
