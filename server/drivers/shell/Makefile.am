noinst_LIBRARIES = @DUMMY_LIB@ @PASSTHROUGH_LIB@ @LOGFILE_LIB@

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server -I$(top_srcdir)/replace -I$(top_srcdir)/client_libs/c

EXTRA_LIBRARIES = libdummy.a libpassthrough.a liblogfile.a

libdummy_a_SOURCES = dummy.cc
libpassthrough_a_SOURCES = passthrough.cc 
liblogfile_a_SOURCES = readlog_manager.h readlog_manager.cc readlog_time.h readlog_time.cc readlog.cc writelog.cc
