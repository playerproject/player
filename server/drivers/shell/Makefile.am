noinst_LIBRARIES = @PASSTHROUGH_LIB@ @READLOG_LIB@

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server -I$(top_srcdir)/client_libs/c

EXTRA_LIBRARIES = libpassthrough.a libreadlog.a

libpassthrough_a_SOURCES = passthrough.cc 
libreadlog_a_SOURCES = readlog_manager.h readlog_manager.cc readlog_time.h readlog_time.cc readlog.cc writelog.cc
