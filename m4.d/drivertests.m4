dnl Here are the tests for inclusion of Player's various device drivers

PLAYER_DRIVERS=
PLAYER_DRIVER_LIBS=
PLAYER_DRIVER_LIBPATHS=
PLAYER_DRIVER_EXTRA_LIBS=
PLAYER_NODRIVERS=

dnl This macro can be used to setup the testing and associated autoconf 
dnl variables and C defines for a device driver.
dnl
dnl PLAYER_ADD_DRIVER(name,path,default,[header],[cppadd],[ldadd])
dnl
dnl Args:
dnl   name:    name; driver library should take the name lib<name>.a
dnl   path:    path, relative to server, to where driver library will be built
dnl   default: should this driver be included by default? ("yes" or "no")
dnl   header:  a list of headers that are all required to build the driver
dnl   cppadd:  compiler flags to be used when building the driver 
dnl            (e.g., "-I/somewhere_odd/include")
dnl   ldadd:   link flags to be added to Player if this driver is included
dnl            (e.g., "-lgsl -lcblas")
dnl
dnl The C define INCLUDE_<name> and the autoconf variable <name>_LIB (with 
dnl <name> capitalized) will be conditionally defined to be 1 and 
dnl lib<name>.a, respectively.  The variable <name>_EXTRA_CPPFLAGS will be
dnl the value of <cppadd>, for use in the driver's Makefile.am.
dnl
AC_DEFUN([PLAYER_ADD_DRIVER], [
AC_DEFUN([name_caps],translit($1,[a-z],[A-Z]))
ifelse($3,[yes],
  [AC_ARG_ENABLE($1,[  --disable-$1       Don't compile the $1 driver],,
                 enable_$1=yes)],
  [AC_ARG_ENABLE($1, [  --enable-$1       Compile the $1 driver],,
                 enable_$1=no)])
failed_header_check=no
if test "x$enable_$1" = "xyes" -a len($4) -gt 0; then
dnl This little bit of hackery keeps us from generating invalid shell code,
dnl in the form of 'for' over an empty list.
  if test len($4) -gt 0; then
    header_list=$4
  else
    header_list=foo
  fi
  for header in $header_list; do
    AC_CHECK_HEADER($header, 
                    enable_$1=yes,
                    enable_$1=no
                    failed_header_check=yes,)
  done
  if test "x$failed_header_check" = "xyes"; then
    enable_$1=no
  fi
fi
if test "x$enable_$1" = "xyes"; then
  AC_DEFINE([INCLUDE_]name_caps, 1, [include the $1 driver])
  name_caps[_LIB]=[lib]$1[.a]
  name_caps[_LIBPATH]=$2/$name_caps[_LIB]
  name_caps[_EXTRA_CPPFLAGS]=$5
  name_caps[_EXTRA_LIB]=$6
  PLAYER_DRIVERS="$PLAYER_DRIVERS $1"
else      
  if test "x$failed_header_check" = "xyes"; then
    PLAYER_NODRIVERS="$PLAYER_NODRIVERS:$1 -- couldn't find (at least one of) $header_list"
  elif test "x$3" = "xno"; then
    PLAYER_NODRIVERS="$PLAYER_NODRIVERS:$1 -- disabled by default; use --enable-$1 to enable"
  else
    PLAYER_NODRIVERS="$PLAYER_NODRIVERS:$1 -- disabled by user"
  fi
fi        
AC_SUBST(name_caps[_LIB])
PLAYER_DRIVER_LIBS="$PLAYER_DRIVER_LIBS $name_caps[_LIB]"
PLAYER_DRIVER_LIBPATHS="$PLAYER_DRIVER_LIBPATHS $name_caps[_LIBPATH]"
AC_SUBST(name_caps[_EXTRA_CPPFLAGS])
PLAYER_DRIVER_EXTRA_LIBS="$PLAYER_DRIVER_EXTRA_LIBS $name_caps[_EXTRA_LIB]"
])        


AC_DEFUN([PLAYER_DRIVERTESTS], [

dnl Where's CANLIB??
AC_ARG_WITH(canlib, [  --with-canlib=dir       Location of CANLIB],
CANLIB_DIR=$with_canlib,CANLIB_DIR=$prefix)
if test "x$CANLIB_DIR" = "xNONE" -o "x$CANLIB_DIR" = "xno"; then
  SEGWAYRMP_HEADER=canlib.h
  SEGWAYRMP_EXTRA_CPPFLAGS=
  SEGWAYRMP_EXTRA_LDFLAGS=-lcanlib
else
  SEGWAYRMP_HEADER=$CANLIB_DIR/include/canlib.h
  SEGWAYRMP_EXTRA_CPPFLAGS="-I$CANLIB_DIR/include"
  SEGWAYRMP_EXTRA_LDFLAGS="-L$CANLIB_DIR/lib -lcanlib"
fi

PLAYER_ADD_DRIVER([segwayrmp],[drivers/mixed/rmp],[no],
	[$SEGWAYRMP_HEADER], [$SEGWAYRMP_EXTRA_CPPFLAGS],
	[$SEGWAYRMP_EXTRA_LDFLAGS])

PLAYER_ADD_DRIVER([garminnmea],[drivers/gps],[yes],[],[],[])

PLAYER_ADD_DRIVER([lifomcom],[drivers/mcom],[yes],[],[],[])

PLAYER_ADD_DRIVER([passthrough],[drivers/shell],[yes],[],[],
                  ["../client_libs/c/playercclient.o"])

PLAYER_ADD_DRIVER([logfile],[drivers/shell],[yes],[],[],[])

PLAYER_ADD_DRIVER([p2os],[drivers/mixed/p2os],[yes],[],[],[])

PLAYER_ADD_DRIVER([rflex],[drivers/mixed/rflex],[no],[],[],[])

PLAYER_ADD_DRIVER([sicklms200],[drivers/laser],[yes],[],[],[])

PLAYER_ADD_DRIVER([acts],[drivers/blobfinder],[yes],[],[],[])

PLAYER_ADD_DRIVER([cmvision],[drivers/blobfinder/cmvision],[yes],[],[],[])
if test "x$enable_cmvision" = "xyes"; then
  dnl Check for video-related headers, to see which support can be compiled into
  dnl the CMVision driver.
  AC_CHECK_HEADER(libraw1394/raw1394.h, have_raw1394=yes, have_raw1394=no)
  AC_CHECK_HEADER(libdc1394/dc1394_control.h, have_dc1394=yes,
  have_dc1394=no)
  if test "x$have_raw1394" = "xyes" -a "x$have_dc1394" = "xyes"; then
    AC_MSG_RESULT([***************************************************])
    AC_MSG_RESULT([Found the 1394 (firewire) headers. 1394 camera])
    AC_MSG_RESULT([support will be included in the CMVision driver])
    AC_MSG_RESULT([***************************************************])
    AC_DEFINE(HAVE_1394, 1, [do we have the low-level 1394 libs?])
    PLAYER_DRIVER_EXTRA_LIBS="$PLAYER_DRIVER_EXTRA_LIBS -lraw1394 -ldc1394_control"
  else
    AC_MSG_RESULT([***************************************************])
    AC_MSG_RESULT([Couldn't find the 1394 (firewire) headers. 1394])
    AC_MSG_RESULT([camera support will *NOT* be included in the])
    AC_MSG_RESULT([CMVision driver])
    AC_MSG_RESULT([***************************************************])
  fi
  
  AC_CHECK_HEADER(linux/videodev2, have_videodev2=yes, have_videodev2=no)
  if test "x$have_videodev2" = "xyes"; then
    AC_MSG_RESULT([***************************************************])
    AC_MSG_RESULT([Found the Video4Linux2 headers. V4L2 camera support])
    AC_MSG_RESULT([will be included in the CMVision driver])
    AC_MSG_RESULT([***************************************************])
    AC_DEFINE(HAVE_V4L2, 1, [do we have the V4L2 libs?])
  else
    AC_MSG_RESULT([***************************************************])
    AC_MSG_RESULT([Couldn't find the Video4Linux2 headers. V4L2 camera])
    AC_MSG_RESULT([support will *NOT* be included in the CMVision driver])
    AC_MSG_RESULT([***************************************************])
  fi
fi

PLAYER_ADD_DRIVER([festival],[drivers/speech],[yes],[],[],[])

PLAYER_ADD_DRIVER([sonyevid30],[drivers/ptz],[yes],[],[],[])

PLAYER_ADD_DRIVER([udpbroadcast],[drivers/comms],[yes],[],[],[])

PLAYER_ADD_DRIVER([lasercspace],[drivers/laser],[yes],[],[],[])

PLAYER_ADD_DRIVER([linuxwifi],[drivers/wifi],[yes],[linux/wireless.h],[],[])

PLAYER_ADD_DRIVER([fixedtones],[drivers/audio],[yes],[rfftw.h],[],
                  ["-lrfftw -lfftw"])

PLAYER_ADD_DRIVER([acoustics],[drivers/audiodsp],[yes],
                  ["gsl/gsl_fft_real.h sys/soundcard.h"],[],
                  ["-lgsl -lgslcblas"])

PLAYER_ADD_DRIVER([mixer],[drivers/audiomixer],[yes],[sys/soundcard.h],[],[])

dnl where's Mobility?
AC_ARG_WITH(mobility, [  --with-mobility=dir     Location of Mobility],
MOBILITY_DIR=$with_mobility,
MOBILITY_DIR="${HOME}/../mobility/mobility-b-1.1.7-rh6.0")

PLAYER_ADD_DRIVER([rwi],[drivers/mixed/rwi],[yes],
                  [$MOBILITY_DIR/include/mbylistbase.h],
                  ["-I$MOBILITY_DIR/include -I$MOBILITY_DIR/tools/include -DUSE_MOBILITY -D__x86__ -D__linux__ -D__OSVERSION__=2"],
                  ["-L$MOBILITY_DIR/lib -L$MOBILITY_DIR/tools/lib -lmby -lidlmby -lomniDynamic2 -lomniORB2 -ltcpwrapGK -lomnithread"])

PLAYER_ADD_DRIVER([isense],[drivers/position/isense],[yes],[isense/isense.h],
                  [],["-lisense"])

PLAYER_ADD_DRIVER([waveaudio],[drivers/waveform],[no],[sys/soundcard.h],[],[])

PLAYER_ADD_DRIVER([aodv],[drivers/wifi],[no],[],[],[])

PLAYER_ADD_DRIVER([iwspy],[drivers/wifi],[no],[],[],[])

PLAYER_ADD_DRIVER([reb],[drivers/mixed/reb],[no],[],[],[])

PLAYER_ADD_DRIVER([microstrain],[drivers/position/microstrain],[no],[],[],[])

PLAYER_ADD_DRIVER([inav],[drivers/position/inav],[no],[gsl/gsl_version.h],[],
                  ["-lgsl -lgslcblas"])

PLAYER_ADD_DRIVER([vfh],[drivers/position/vfh],[no],)

PLAYER_ADD_DRIVER([stage],[drivers/stage],[no],[],[],[])

dnl Where is Stage-1.4?
AC_ARG_WITH(stage, [  --with-stage=dir       Location of Stage],
            STAGE_DIR=$with_stage,STAGE_DIR=$prefix)
STAGE_HEADER=$STAGE_DIR/stage.h
STAGE_EXTRA_CPPFLAGS="-I$STAGE_DIR"
STAGE_EXTRA_LDFLAGS="-L$STAGE_DIR -lstageio -lstageworldfile"

dnl add the Stage-1.4 driver
PLAYER_ADD_DRIVER([stage1p4],[drivers/stage1p4],[yes],
		  [$STAGE_HEADER],[$STAGE_EXTRA_CPPFLAGS],[$STAGE_EXTRA_LDFLAGS])


dnl Where is Gazebo?
AC_ARG_WITH(gazebo, [  --with-gazebo=dir       Location of Gazebo],
            GAZEBO_DIR=$with_gazebo,GAZEBO_DIR=$prefix)
if test "x$GAZEBO_DIR" = "xNONE" -o "x$GAZEBO_DIR" = "xno"; then
  GAZEBO_HEADER=gazebo.h
  GAZEBO_EXTRA_CPPFLAGS=
  GAZEBO_EXTRA_LDFLAGS=-lgazebo
else
  GAZEBO_HEADER=$GAZEBO_DIR/include/gazebo.h
  GAZEBO_EXTRA_CPPFLAGS="-I$GAZEBO_DIR/include"
  GAZEBO_EXTRA_LDFLAGS="-L$GAZEBO_DIR/lib -lgazebo"
fi

dnl Add the Gazebo driver
PLAYER_ADD_DRIVER([gazebo],[drivers/gazebo],[no],[$GAZEBO_HEADER],
                  [$GAZEBO_EXTRA_CPPFLAGS],[$GAZEBO_EXTRA_LDFLAGS])

PLAYER_ADD_DRIVER([laserbar],[drivers/fiducial],[yes],[],[],[])
PLAYER_ADD_DRIVER([laserbarcode],[drivers/fiducial],[yes],[],[],[])
PLAYER_ADD_DRIVER([laservisualbarcode],[drivers/fiducial],[yes],[],[],[])

dnl PLAYER_ADD_DRIVER doesn't handle building more than one library, so
dnl do it manually
AC_ARG_ENABLE(amcl,
[  --enable-amcl           Compile the amcl driver],,enable_amcl=yes)
if test "x$enable_amcl" = "xyes"; then
  AC_CHECK_HEADER(gsl/gsl_version.h,enable_amcl=yes,enable_amcl=no)
fi
if test "x$enable_amcl" = "xyes"; then
  AC_DEFINE(INCLUDE_AMCL, 1, [[include the AMCL driver]])
  AMCL_LIB="libamcl.a"
  PLAYER_DRIVER_LIBS="$PLAYER_DRIVER_LIBS $AMCL_LIB"
  PLAYER_DRIVER_LIBPATHS="$PLAYER_DRIVER_LIBPATHS drivers/localization/amcl/libamcl.a"
  AMCL_PF_LIB="libpf.a"
  AMCL_MAP_LIB="libmap.a"
  AMCL_MODELS_LIB="libmodels.a"
  PLAYER_DRIVER_EXTRA_LIBS="$PLAYER_DRIVER_EXTRA_LIBS -lgsl -lgslcblas"
  PLAYER_DRIVERS="$PLAYER_DRIVERS amcl"
else
  PLAYER_NODRIVERS="$PLAYER_NODRIVERS:amcl"
fi
AC_SUBST(AMCL_LIB)
AC_SUBST(AMCL_PF_LIB)
AC_SUBST(AMCL_MAP_LIB)
AC_SUBST(AMCL_MODELS_LIB)

AC_SUBST(PLAYER_DRIVER_LIBS)
AC_SUBST(PLAYER_DRIVER_LIBPATHS)
AC_SUBST(PLAYER_DRIVER_EXTRA_LIBS)

])

