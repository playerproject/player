

noinst_LIBRARIES = @SEGWAYRMP_LIB@

AM_CPPFLAGS = -Wall @SEGWAYRMP_EXTRA_CPPFLAGS@ -I$(top_srcdir)/server

EXTRA_LIBRARIES = libsegwayrmp.a

libsegwayrmp_a_SOURCES = segwayrmp.cc segwayio.cc segwayio.h \
		canio.cc canio.h
