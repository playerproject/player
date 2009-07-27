noinst_LTLIBRARIES =
if INCLUDE_FLEXIPORT
noinst_LTLIBRARIES += libflexiport.la
endif
if INCLUDE_SERIALSTREAM
noinst_LTLIBRARIES += libserialstream.la
endif
if INCLUDE_TCPSTREAM
noinst_LTLIBRARIES += libtcpstream.la
endif

AM_CPPFLAGS = -Wall -I$(top_srcdir)

libflexiport_la_SOURCES = flexiport.cc
libflexiport_la_CXXFLAGS = @FLEXIPORT_CFLAGS@
libflexiport_la_LIBADD = @FLEXIPORT_LIBS@

libserialstream_la_SOURCES = serialstream.cc

libtcpstream_la_SOURCES = tcpstream.cc
