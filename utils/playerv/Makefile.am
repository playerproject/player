if WITH_GTK
PLAYERV = playerv
endif

AM_CPPFLAGS = -I$(top_srcdir)/client_libs/libplayerc -I$(top_srcdir)/server -I$(top_srcdir)/rtk2 @GTK_CFLAGS@

bin_PROGRAMS = $(PLAYERV)

playerv_SOURCES = playerv.c \
                  playerv.h \
                  registry.c \
                  mainwnd.c \
                  dev_blobfinder.c \
                  dev_fiducial.c \
                  dev_laser.c \
		  dev_localize.c \
                  dev_map.c \
                  dev_position.c \
                  dev_power.c \
                  dev_ptz.c \
                  dev_sonar.c \
                  dev_bumper.c \
                  dev_ir.c \
		  dev_wifi.c \
                  error.h \
                  opt.c \
                  opt.h

playerv_LDADD = -L$(top_builddir)/client_libs/libplayerc -lrtk -lplayerc -L$(top_builddir)/rtk2 -lrtk @GTK_LIBS@
