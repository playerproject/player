if WITH_RTK
PLAYERV = playerv
endif

AM_CPPFLAGS = -I@RTK_DIR@/include -I$(top_srcdir)/client_libs/libplayerc -I$(top_srcdir)/server `gtk-config --cflags`

bin_PROGRAMS = $(PLAYERV)

playerv_SOURCES = playerv.c \
                  playerv.h \
                  registry.c \
                  mainwnd.c \
                  dev_blobfinder.c \
                  dev_fiducial.c \
                  dev_laser.c \
                  dev_position.c \
									dev_power.c \
                  dev_ptz.c \
                  dev_sonar.c \
								  dev_localize.c \
                  error.h \
                  opt.c \
                  opt.h

playerv_LDADD = -L@RTK_DIR@/lib -L$(top_builddir)/client_libs/libplayerc -lrtk -lplayerc $(LIBJPEG)
playerv_LDFLAGS = `gtk-config --libs gtk gthread` 
