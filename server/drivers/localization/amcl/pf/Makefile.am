

noinst_LIBRARIES = @AMCL_PF_LIB@

EXTRA_LIBRARIES = libpf.a

if INCLUDE_RTKGUI
AM_CPPFLAGS = -I$(top_srcdir)/server -I@RTK_DIR@/include `gtk-config --cflags`
libpf_a_SOURCES = pf.c pf_kdtree.c pf_pdf.c pf_vector.c pf_draw.c
else
AM_CPPFLAGS = -I$(top_srcdir)/server
libpf_a_SOURCES = pf.c pf_kdtree.c pf_pdf.c pf_vector.c
endif
