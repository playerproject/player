

noinst_LIBRARIES = @AMCL_PF_LIB@

EXTRA_LIBRARIES = libpf.a

if INCLUDE_RTKGUI
AM_CPPFLAGS = -Wall -I$(top_srcdir)/server -I@RTK_DIR@/include `gtk-config --cflags`
else
AM_CPPFLAGS = -Wall -I$(top_srcdir)/server
endif

libpf_a_SOURCES = pf.c pf_kdtree.c pf_pdf.c pf_vector.c pf_draw.c gsl_discrete.c pf.h pf_pdf.h pf_kdtree.h pf_vector.h