SUBDIRS =

if INCLUDE_RTKGUI

AM_CPPFLAGS = -I$(top_srcdir)/server -I@RTK_DIR@/include `gtk-config --cflags`

libinav_a_SOURCES = inav.cc inav_vector.h inav_vector.c \
										inav_map.h inav_map.c inav_map_range.c inav_map_fit.c inav_map_store.c \
										inav_con.h inav_con.c \
										inav_map_draw.c inav_con_draw.c
else

AM_CPPFLAGS = -I$(top_srcdir)/server

endif

libinav_a_LIBADD = 

noinst_LIBRARIES = @INAV_LIB@

EXTRA_LIBRARIES = libinav.a



