

noinst_LIBRARIES = @AMCL_MAP_LIB@

EXTRA_LIBRARIES = libmap.a

if INCLUDE_RTKGUI
AM_CPPFLAGS = -I$(top_srcdir)/server -I@RTK_DIR@/include `gtk-config --cflags`
libmap_a_SOURCES = map.c map_range.c map_store.c map_draw.c
else
AM_CPPFLAGS = -I$(top_srcdir)/server
libmap_a_SOURCES = map.c map_range.c map_store.c
endif


