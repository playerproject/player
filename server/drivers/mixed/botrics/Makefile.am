AM_CPPFLAGS = -Wall -I$(top_srcdir)/server -I$(top_srcdir)/replace

noinst_LTLIBRARIES = 
if INCLUDE_OBOT
noinst_LTLIBRARIES += libobot.la
endif

libobot_la_SOURCES = obot.cc obot_constants.h
