

noinst_LIBRARIES = @SEGWAYRMP_LIB@

AM_CPPFLAGS = -Wall @SEGWAYRMP_EXTRA_CPPFLAGS@ -I$(top_srcdir)/server

EXTRA_LIBRARIES = libsegwayrmp.a

libsegwayrmp_a_SOURCES = segwayrmp.h segwayrmp.cc rmp_frame.h \
													canio.h canio_kvaser.h canio_kvaser.cc 
