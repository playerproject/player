AM_CPPFLAGS = -DINCLUDE_GUI -I$(top_srcdir)/rtk2 -I$(top_srcdir)/client_libs/libplayerc -I$(top_srcdir)/server `gtk-config --cflags`

bin_PROGRAMS = playerv

playerv_SOURCES = dev_laser.c \
                  dev_lbd.c \
                  dev_position.c \
                  dev_ptz.c \
                  dev_sonar.c \
                  dev_vision.c \
                  error.h \
                  mainwnd.c \
                  opt.c \
                  opt.h \
                  playerv.c \
                  playerv.h

playerv_LDADD = -L$(top_builddir)/rtk2 -L$(top_builddir)/client_libs/libplayerc -lrtk -lplayerc 
playerv_LDFLAGS = `gtk-config --libs gtk gthread`
