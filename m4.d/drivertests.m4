dnl Here are the tests for inclusion of Player's various device drivers

dnl This macro does all the checking for RTK, taking into account --prefix
dnl and --with-rtk
AC_DEFUN([RTK_CHECK],[
dnl where's RTK?
AC_ARG_WITH(rtk, [  --with-rtk=dir          Location of RTK],
RTK_DIR=$with_rtk,
RTK_DIR=$prefix)

dnl RTK2 uses libjpeg to export images.
AC_CHECK_HEADER(jpeglib.h,
  AC_DEFINE(HAVE_JPEGLIB_H,1,[include jpeg support])
  LIBJPEG="-ljpeg",,)
AC_SUBST(LIBJPEG)

AC_CHECK_PROG([have_gtk], [gtk-config], [yes], [no])
if test "x$have_gtk" = "xyes"; then
  if test "x$RTK_DIR" = "xNONE"; then
    AC_CHECK_LIB(rtk,rtk_init,with_rtk=yes,with_rtk=no,
                 [$LIBJPEG `gtk-config --libs gtk gthread`])
    RTK_CPPFLAGS="`gtk-config --cflags`"
    RTK_LDADD="-lrtk $LIBJPEG `gtk-config --libs gtk gthread`"
  elif test ! "x$RTK_DIR" = "xno"; then
    AC_MSG_CHECKING(for $RTK_DIR/lib/librtk.a)
    if test -f $RTK_DIR/lib/librtk.a; then
      with_rtk=yes
      RTK_CPPFLAGS="-I$RTK_DIR/include `gtk-config --cflags`"
      RTK_LDADD="-L$RTK_DIR/lib -lrtk $LIBJPEG `gtk-config --libs gtk gthread`"
      AC_MSG_RESULT(yes)
    else
      with_rtk=no
      AC_MSG_RESULT(no)
    fi
  fi
else
  with_rtk=no
fi
if test "x$with_rtk" = "xno"; then
  AC_MSG_WARN([***************************************************])
  AC_MSG_WARN([Couldn't find RTK so I won't build RTK-based GUIs])
  AC_MSG_WARN([Maybe you should download and install RTK?])
  AC_MSG_WARN([If you have RTK installed, try --with-rtk=path])
  AC_MSG_WARN([***************************************************])
fi
AM_CONDITIONAL(WITH_RTK, test x$with_rtk = xyes)
AC_SUBST(RTK_CPPFLAGS)
AC_SUBST(RTK_LDADD)
])

PLAYER_DRIVERS=
PLAYER_DRIVER_LIBS=
PLAYER_DRIVER_LIBPATHS=
PLAYER_DRIVER_EXTRA_LIBS=
PLAYER_NODRIVERS=

dnl This macro can be used to setup the testing and associated autoconf 
dnl variables and C defines for a device driver.
dnl
dnl PLAYER_ADD_DRIVER(name,path,default,[ldadd],[header])
dnl
dnl Args:
dnl   name:    name; driver library should take the name lib<name>.a
dnl   path:    path, relative to server, to where driver library will be built
dnl   default: should this driver be included by default? ("yes" or "no")
dnl   ldadd:   link flags to be added to Player (e.g., "-lgsl -lcblas")
dnl   header:  a single header file that is required to build the driver
dnl
dnl The C define INCLUDE_<name> and the autoconf variable <name>_LIB (with 
dnl <name> capitalized) will be conditionally defined to be 1 and 
dnl lib<name>.a, respectively.
dnl
AC_DEFUN([PLAYER_ADD_DRIVER], [
AC_DEFUN([name_caps],translit($1,[a-z],[A-Z]))
ifelse($3,[yes],
  [AC_ARG_ENABLE($1,[  --disable-$1       Don't compile the $1 driver],,
                 enable_$1=yes)],
  [AC_ARG_ENABLE($1, [  --enable-$1       Compile the $1 driver],,
                 enable_$1=no)])
failed_header_check=no
if test "x$enable_$1" = "xyes" -a len($5) -gt 0; then
  if test len($5) -gt 0; then
    header_list=$5
  else
    header_list=foo
  fi
  for header in $header_list; do
    AC_CHECK_HEADER($header, 
                    enable_$1=yes,
                    enable_$1=no
                    failed_header_check=yes,)
  done
fi
if test "x$enable_$1" = "xyes"; then
  AC_DEFINE([INCLUDE_]name_caps, 1, [include the $1 driver])
  name_caps[_LIB]=[lib]$1[.a]
  name_caps[_LIBPATH]=$2/$name_caps[_LIB]
  name_caps[_EXTRA_LIB]=$4
  PLAYER_DRIVERS="$PLAYER_DRIVERS $1"
else      
  if test "x$3" = "xno"; then
    PLAYER_NODRIVERS="$PLAYER_NODRIVERS:$1 -- disabled by default; use --enable-$1 to enable"
  elif test "x$failed_header_check" = "xyes"; then
    PLAYER_NODRIVERS="$PLAYER_NODRIVERS:$1 -- couldn't find (at least one of) $header_list"
  else
    PLAYER_NODRIVERS="$PLAYER_NODRIVERS:$1 -- disabled by user"
  fi
fi        
AC_SUBST(name_caps[_LIB])
PLAYER_DRIVER_LIBS="$PLAYER_DRIVER_LIBS $name_caps[_LIB]"
PLAYER_DRIVER_LIBPATHS="$PLAYER_DRIVER_LIBPATHS $name_caps[_LIBPATH]"
PLAYER_DRIVER_EXTRA_LIBS="$PLAYER_DRIVER_EXTRA_LIBS $name_caps[_EXTRA_LIB]"
])        


dnl This macro can be used to setup the testing and associated autoconf 
dnl variables and C defines for a device driver.  This one has more options
dnl to allow for drivers with dependencies to libraries that may be in unusual
dnl places.
dnl
dnl PLAYER_ADD_DRIVER_EX(name,path,default,[header],[cppadd],[ldadd])
dnl
dnl Args:
dnl   name:    name; driver library should take the name lib<name>.a
dnl   path:    path, relative to server, to where driver library will be built
dnl   default: should this driver be included by default? ("yes" or "no")
dnl   header:  a single header file that is required to build the driver
dnl   cppadd:  compiler flags to be added to Player (e.g., "-I/somewhere_odd/include")
dnl   ldadd:   link flags to be added to Player (e.g., "-L/somwhere_odd/lib -lmylib")
dnl
dnl The C define INCLUDE_<name> and the autoconf variable <name>_LIB (with 
dnl <name> capitalized) will be conditionally defined to be 1 and 
dnl lib<name>.a, respectively.
dnl
AC_DEFUN([PLAYER_ADD_DRIVER_EX], [
AC_DEFUN([other_name_caps],translit($1,[a-z],[A-Z]))
ifelse($3,[yes],
  [AC_ARG_ENABLE($1,[  --disable-$1	  Don't compile the $1 driver],,
                 enable_$1=yes)],
  [AC_ARG_ENABLE($1, [  --enable-$1	  Compile the $1 driver],,
                 enable_$1=no)])
if test "x$enable_$1" = "xyes" -a len($4) -gt 0; then
  if test len($4) -gt 0; then
    header_list=$4
  else
    header_list=foo
  fi
  for header in $header_list; do
    AC_CHECK_HEADER($header, enable_$1=yes, enable_$1=no,)
  done
fi
if test "x$enable_$1" = "xyes"; then
  AC_DEFINE([INCLUDE_]other_name_caps, 1, [include the $1 driver])
  other_name_caps[_LIB]=[lib]$1[.a]
  other_name_caps[_LIBPATH]=$2/$other_name_caps[_LIB]
  other_name_caps[_EXTRA_CPPFLAGS]=$5
  other_name_caps[_EXTRA_LIB]=$6
  PLAYER_DRIVERS="$PLAYER_DRIVERS $1"
else
  PLAYER_NODRIVERS="$PLAYER_NODRIVERS:$1"
fi
AC_SUBST(other_name_caps[_LIB])
PLAYER_DRIVER_LIBS="$PLAYER_DRIVER_LIBS $other_name_caps[_LIB]"
PLAYER_DRIVER_LIBPATHS="$PLAYER_DRIVER_LIBPATHS $other_name_caps[_LIBPATH]"
AC_SUBST(other_name_caps[_EXTRA_CPPFLAGS])
PLAYER_DRIVER_EXTRA_LIBS="$PLAYER_DRIVER_EXTRA_LIBS $other_name_caps[_EXTRA_LIB]"
])


AC_DEFUN([PLAYER_DRIVERTESTS], [

PLAYER_ADD_DRIVER([lifomcom],[drivers/mcom],[yes],)

PLAYER_ADD_DRIVER([passthrough],[drivers/shell],[yes],
                  ["../client_libs/c/playercclient.o"])

PLAYER_ADD_DRIVER([p2os],[drivers/mixed/p2os],[yes],)

PLAYER_ADD_DRIVER([sicklms200],[drivers/laser],[yes],)

PLAYER_ADD_DRIVER([acts],[drivers/blobfinder],[yes],)

PLAYER_ADD_DRIVER([cmvision],[drivers/blobfinder/cmvision],[yes],,)
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

PLAYER_ADD_DRIVER([festival],[drivers/speech],[yes],)

PLAYER_ADD_DRIVER([sonyevid30],[drivers/ptz],[yes],)

PLAYER_ADD_DRIVER([stage],[drivers/stage],[yes],)

PLAYER_ADD_DRIVER([udpbroadcast],[drivers/comms],[yes],)

PLAYER_ADD_DRIVER([lasercspace],[drivers/laser],[yes],)

PLAYER_ADD_DRIVER([linuxwifi],[drivers/wifi],[yes],
                  [],[linux/wireless.h])

PLAYER_ADD_DRIVER([fixedtones],[drivers/audio],[yes],
                  ["-lrfftw -lfftw"],[rfftw.h])

PLAYER_ADD_DRIVER([acoustics],[drivers/audiodsp],[yes],
                  ["-lgsl -lgslcblas"],["gsl/gsl_fft_real.h sys/soundcard.h"])

PLAYER_ADD_DRIVER([mixer],[drivers/audiomixer],[yes],
                  [],[sys/soundcard.h])

PLAYER_ADD_DRIVER([rwi],[drivers/mixed/rwi],[yes],
["-L$MOBILITY_DIR/lib -L$MOBILITY_DIR/tools/lib -lmby -lidlmby -lomniDynamic2 -lomniORB2 -ltcpwrapGK -lomnithread"], [$MOBILITY_DIR/include/mbylistbase.h])

PLAYER_ADD_DRIVER([isense],[drivers/position/isense],[yes],
                  ["-lisense"],[isense/isense.h])

PLAYER_ADD_DRIVER([waveaudio],[drivers/waveform],[no],[],[sys/soundcard.h])

dnl the following must use --enable to be built
PLAYER_ADD_DRIVER([aodv],[drivers/wifi],[no],)

PLAYER_ADD_DRIVER([iwspy],[drivers/wifi],[no],)

PLAYER_ADD_DRIVER([reb],[drivers/mixed/reb],[no],)

PLAYER_ADD_DRIVER([microstrain],[drivers/position/microstrain],[no],)

dnl Deprecated
dnl PLAYER_ADD_DRIVER([mcl],[drivers/localization/mcl],[no],)

PLAYER_ADD_DRIVER([inav],[drivers/position/inav],[no],["-lgsl -lgslcblas"],[gsl/gsl_version.h])

dnl Where is Gazebo?
AC_ARG_WITH(gazebo, [  --with-gazebo=dir     Location of Gazebo],
dnl Is it somewhere odd...
GAZEBO_HEADER=$with_gazebo/include/gazebo.h
GAZEBO_EXTRA_CPPFLAGS="-I$with_gazebo/include"
GAZEBO_EXTRA_LDFLAGS="-L$with_gazebo/lib -lgazebo",
dnl ...or is is in a standard place?
GAZEBO_HEADER=$prefix/include/gazebo.h
GAZEBO_EXTRA_CPPFLAGS="-I$prefix/include"
GAZEBO_EXTRA_LDFLAGS="-L$prefix/lib -lgazebo"
)

dnl Add the Gazebo driver
PLAYER_ADD_DRIVER_EX([gazebo],[drivers/gazebo],[no],[$GAZEBO_HEADER],
                     [$GAZEBO_EXTRA_CPPFLAGS], [$GAZEBO_EXTRA_LDFLAGS])

dnl PLAYER_ADD_DRIVER doesn't handle multiple libraries in <name>_LIB, so
dnl do it manually
AC_ARG_ENABLE(laser,
[  --disable-laser         Don't compile the laser-based fiducial drivers],,
enable_laser=yes)
if test "x$enable_laser" = "xyes"; then
  AC_DEFINE(INCLUDE_LASERFIDUCIAL, 1, [[include the LASER-based fiducial drivers]])
  LASERFIDUCIAL_LIBS="liblaserbar.a liblaserbarcode.a liblaservisualbarcode.a"
  PLAYER_DRIVER_LIBS="$PLAYER_DRIVER_LIBS $LASERFIDUCIAL_LIBS"
  PLAYER_DRIVER_LIBPATHS="$PLAYER_DRIVER_LIBPATHS drivers/fiducial/liblaserbar.a drivers/fiducial/liblaserbarcode.a drivers/fiducial/liblaservisualbarcode.a"
  PLAYER_DRIVERS="$PLAYER_DRIVERS laserbar laserbarcode laservisualbarcode"
else
  PLAYER_NODRIVERS="$PLAYER_NODRIVERS:laserbar:laserbarcode:laservisualbarcode"
fi
AC_SUBST(LASERFIDUCIAL_LIBS)

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

