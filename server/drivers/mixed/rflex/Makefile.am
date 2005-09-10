
noinst_LTLIBRARIES = 
if INCLUDE_RFLEX
noinst_LTLIBRARIES += librflex.la
endif

AM_CPPFLAGS = -Wall -I$(top_srcdir)

librflex_la_SOURCES = rflex.cc rflex_commands.cc rflex-io.cc rflex_commands.h \
                      rflex_configs.h rflex.h rflex-info.h rflex-io.h
