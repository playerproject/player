AM_CPPFLAGS = -I$(top_srcdir)/server -I$(top_srcdir)/client_libs/c -I$(top_srcdir)/client_libs/c++
LDADD = $(top_builddir)/client_libs/c++/libplayerclient.a

noinst_PROGRAMS = test_client cb_client fid_client

test_client_SOURCES = test_client.cc

cb_client_SOURCES = cb_client.cc

fid_client_SOURCES = fid_client.cc
