if WITH_RTK_GUI
if HAVE_GTK
PLAYERV = playerv
endif
endif

AM_CPPFLAGS = -I$(top_srcdir)/rtk2 -I$(top_srcdir)/client_libs/libplayerc -I$(top_srcdir)/server `gtk-config --cflags`

bin_PROGRAMS = $(PLAYERV)

playerv_SOURCES = playerv.c \
                  playerv.h \
                  registry.c \
                  mainwnd.c \
                  dev_blobfinder.c \
                  dev_fiducial.c \
                  dev_laser.c \
                  dev_position.c \
                  dev_ptz.c \
								  dev_sonar.c \
                  error.h \
                  opt.c \
                  opt.h

playerv_LDADD = -L$(top_builddir)/rtk2 -L$(top_builddir)/client_libs/libplayerc -lrtk -lplayerc $(LIBJPEG)
playerv_LDFLAGS = `gtk-config --libs gtk gthread` 
