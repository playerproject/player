
dnl Here are the tests for inclusion of Player's various device drivers

PLAYER_DRIVERS=
PLAYER_DRIVER_LIBS=
PLAYER_DRIVER_LIBPATHS=
PLAYER_DRIVER_EXTRA_LIBS=
PLAYER_NODRIVERS=

dnl This macro can be used to setup the testing and associated autoconf 
dnl variables and C defines for a device driver.
dnl
dnl PLAYER_ADD_DRIVER(name,path,default,[header],[cppadd],[ldadd],
dnl                   [pkgvar],[pkg],[default-includes])
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
dnl   pkgvar:  variable prefix to be used with pkg-config; if your package
dnl            is found, pkgvar_CFLAGS and pkgvar_LIBS will be set
dnl            appropriately
dnl   pkg:     name of package that is required to build the driver; it
dnl            should be a valid pkg-config spec, like [gtk+-2.0 >= 2.1]
dnl   default-includes: A chunk of code to insert in a test program when
dnl                     checking for each of the headers listed in <header>.
dnl                     For example, if one of the headers that you need in 
dnl                     turn requires <foo.h>, then supply 
dnl                     [[#include <foo.h>]] here.
dnl
dnl The C define INCLUDE_<name> and the autoconf variable <name>_LIB (with 
dnl <name> capitalized) will be conditionally defined to be 1 and 
dnl lib<name>.a, respectively.  The variable <name>_EXTRA_CPPFLAGS will be
dnl the value of <cppadd>, for use in the driver's Makefile.am.
dnl
AC_DEFUN([PLAYER_ADD_DRIVER], [
AC_DEFUN([name_caps],translit($1,[a-z],[A-Z]))

user_override=no
ifelse($3,[yes],
  [AC_ARG_ENABLE($1,[  --disable-$1       Don't compile the $1 driver],user_override=yes,enable_$1=yes)],
  [AC_ARG_ENABLE($1, [  --enable-$1       Compile the $1 driver],user_override=yes,enable_$1=no)])

if test "x$enable_alldrivers" = "xno" -a "x$user_override" = "xno"; then
  enable_$1=no
fi

failed_header_check=no
failed_package_check=no
no_pkg_config=no
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
                    failed_header_check=yes,$9)
  done
  if test "x$failed_header_check" = "xyes"; then
    enable_$1=no
  fi
fi

if test "x$enable_$1" = "xyes" -a len($7) -gt 0 -a len($8) -gt 0; then
  if test "x$have_pkg_config" = "xyes" ; then
    PKG_CHECK_MODULES($7, $8,
      enable_$1=yes,
      enable_$1=no
      failed_package_check=yes)
  else
    no_pkg_config=yes
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
  if test "x$no_pkg_config" = "xyes"; then
    PLAYER_NODRIVERS="$PLAYER_NODRIVERS:$1 -- pkg-config is required to test for dependencies"
  elif test "x$failed_package_check" = "xyes"; then
    PLAYER_NODRIVERS="$PLAYER_NODRIVERS:$1 -- couldn't find required package $8"
  elif test "x$failed_header_check" = "xyes"; then
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

dnl Where's CANLIB?
AC_ARG_WITH(canlib, [  --with-canlib=dir       Location of CANLIB],
CANLIB_DIR=$with_canlib,CANLIB_DIR=NONE)
if test "x$CANLIB_DIR" = "xNONE" -o "x$CANLIB_DIR" = "xno"; then
  SEGWAYRMP_HEADER=canlib.h
  SEGWAYRMP_EXTRA_CPPFLAGS=
  SEGWAYRMP_EXTRA_LDFLAGS=-lcanlib
else
  SEGWAYRMP_HEADER=$CANLIB_DIR/include/canlib.h
  SEGWAYRMP_EXTRA_CPPFLAGS="-I$CANLIB_DIR/include"
  SEGWAYRMP_EXTRA_LDFLAGS="-L$CANLIB_DIR/lib -lcanlib"
fi

PLAYER_ADD_DRIVER([segwayrmp],[drivers/mixed/rmp],[yes],
	[$SEGWAYRMP_HEADER], [$SEGWAYRMP_EXTRA_CPPFLAGS],
	[$SEGWAYRMP_EXTRA_LDFLAGS])

PLAYER_ADD_DRIVER([garminnmea],[drivers/gps],[yes],[],[],[])

PLAYER_ADD_DRIVER([bumpersafe],[drivers/position/bumpersafe],[yes],[],[],[])

PLAYER_ADD_DRIVER([lifomcom],[drivers/mcom],[yes],[],[],[])

PLAYER_ADD_DRIVER([dummy],[drivers/shell],[yes],[],[],[])

PLAYER_ADD_DRIVER([passthrough],[drivers/shell],[yes],[],[],
                  ["../client_libs/c/playercclient.o"])

PLAYER_ADD_DRIVER([logfile],[drivers/shell],[yes],[zlib.h],[],[-lz])

PLAYER_ADD_DRIVER([p2os],[drivers/mixed/p2os],[yes],[],[],[])

PLAYER_ADD_DRIVER([er1],[drivers/mixed/evolution/er1],[no],[asm/ioctls.h],[],[])

PLAYER_ADD_DRIVER([rflex],[drivers/mixed/rflex],[yes],[],[],[])

PLAYER_ADD_DRIVER([sicklms200],[drivers/laser],[yes],[],[],[])
if  test "x$enable_sicklms200" = "xyes"; then
	AC_CHECK_HEADERS(linux/serial.h, [], [], [])
fi

PLAYER_ADD_DRIVER([sickpls],[drivers/laser],[yes],[],[],[])
if  test "x$enable_sickpls" = "xyes"; then
        AC_CHECK_HEADERS(linux/serial.h, [], [], [])
fi
                                                                     
AC_ARG_ENABLE(highspeedsick, [  --disable-highspeedsick   Don't build support for 500Kbps comms with SICK],,enable_highspeedsick=yes)
if test "x$enable_highspeedsick" = "xno"; then
  AC_DEFINE(DISABLE_HIGHSPEEDSICK,1,[disable 500Kbps comms with SICK])
fi

AC_ARG_ENABLE(playerclient_thread, [  --disable-playerclient_thread   Build thread safe c++ client library],,disable_playerclient_thread=no)
if test "x$disable_playerclient_thread" = "xno"; then
  AC_DEFINE(PLAYERCLIENT_THREAD,1,[Thread Safe C++ Client Library])
fi


PLAYER_ADD_DRIVER([acts],[drivers/blobfinder],[yes],[],[],[])

PLAYER_ADD_DRIVER([cmucam2],[drivers/mixed/cmucam2],[yes],[],[],[])

PLAYER_ADD_DRIVER([cmvision],[drivers/blobfinder/cmvision],[yes],[],[],[])

PLAYER_ADD_DRIVER([upcbarcode],[drivers/blobfinder/upcbarcode],[yes],[],[],[],
                  [OPENCV],[opencv])

PLAYER_ADD_DRIVER([simpleshape],[drivers/blobfinder],[yes],
                    [],[],[],[OPENCV],[opencv])

PLAYER_ADD_DRIVER([festival],[drivers/speech],[yes],[],[],[])

PLAYER_ADD_DRIVER([sphinx2],[drivers/speech/recognition],[yes],["sphinx2/s2types.h"],[],["-lsphinx2 -lsphinx2fe -lsphinx2ad"])

PLAYER_ADD_DRIVER([sonyevid30],[drivers/ptz],[yes],[],[],[])

PLAYER_ADD_DRIVER([amtecpowercube],[drivers/ptz],[yes],[],[],[])

PLAYER_ADD_DRIVER([ptu46],[drivers/ptz],[yes],[],[],[])

PLAYER_ADD_DRIVER([canonvcc4],[drivers/ptz],[yes],[],[],[])

PLAYER_ADD_DRIVER([flockofbirds],[drivers/position/ascension],[no],[],[],[])

PLAYER_ADD_DRIVER([obot],[drivers/mixed/botrics],[yes],[],[],[])

PLAYER_ADD_DRIVER([clodbuster],[drivers/mixed/clodbuster],[no],[],[],[])

PLAYER_ADD_DRIVER([lasercspace],[drivers/laser],[yes],[],[],[])

PLAYER_ADD_DRIVER([linuxwifi],[drivers/wifi],[yes],[linux/wireless.h],
                  [],[],[],[],[[#include <netinet/in.h>]])

PLAYER_ADD_DRIVER([fixedtones],[drivers/audio],[no],[rfftw.h],[],
                  ["-lrfftw -lfftw"])

PLAYER_ADD_DRIVER([acoustics],[drivers/audiodsp],[yes],
                  ["gsl/gsl_fft_real.h sys/soundcard.h"],[],
                  ["-lgsl -lgslcblas"])

PLAYER_ADD_DRIVER([mixer],[drivers/audiomixer],[yes],[sys/soundcard.h],[],[])

dnl where's Mobility?
AC_ARG_WITH(mobility, [  --with-mobility=dir     Location of Mobility],
MOBILITY_DIR=$with_mobility,
MOBILITY_DIR="${HOME}/../mobility/mobility-b-1.1.7-rh6.0")

PLAYER_ADD_DRIVER([rwi],[drivers/mixed/rwi],[no],
                  [$MOBILITY_DIR/include/mbylistbase.h],
                  ["-I$MOBILITY_DIR/include -I$MOBILITY_DIR/tools/include -DUSE_MOBILITY -D__x86__ -D__linux__ -D__OSVERSION__=2"],
                  ["-L$MOBILITY_DIR/lib -L$MOBILITY_DIR/tools/lib -lmby -lidlmby -lomniDynamic2 -lomniORB2 -ltcpwrapGK -lomnithread"])

PLAYER_ADD_DRIVER([isense],[drivers/position/isense],[yes],[isense/isense.h],
                  [],["-lisense"])

PLAYER_ADD_DRIVER([wavefront],[drivers/planner/wavefront],[yes],[],[],[])
dnl The wavefront driver can make use of MD5 hash functions, if present
AC_CHECK_HEADERS(openssl/md5.h)
AC_CHECK_LIB(md5,MD5_Init)

PLAYER_ADD_DRIVER([mapfile],[drivers/map],[yes],[],
                  [],[],[GDK_PIXBUF],[gdk-pixbuf-2.0])
PLAYER_ADD_DRIVER([mapscale],[drivers/map],[yes],[],
                  [],[],[GDK_PIXBUF],[gdk-pixbuf-2.0])
PLAYER_DRIVER_EXTRA_LIBS="$PLAYER_DRIVER_EXTRA_LIBS $GDK_PIXBUF_LIBS"

PLAYER_ADD_DRIVER([mapcspace],[drivers/map],[yes],[],[],[])

PLAYER_ADD_DRIVER([waveaudio],[drivers/waveform],[yes],[sys/soundcard.h],[],[])

PLAYER_ADD_DRIVER([aodv],[drivers/wifi],[no],[],[],[])

PLAYER_ADD_DRIVER([iwspy],[drivers/wifi],[yes],[],[],[])

PLAYER_ADD_DRIVER([reb],[drivers/mixed/reb],[yes],[],[],[])

PLAYER_ADD_DRIVER([khepera],[drivers/mixed/khepera],[yes],[],[],[])

PLAYER_ADD_DRIVER([microstrain],[drivers/position/microstrain],[yes],[],[],[])

PLAYER_ADD_DRIVER([vfh],[drivers/position/vfh],[yes],)

PLAYER_ADD_DRIVER([nomad],[drivers/mixed/nomad],[no],[],[],[])

PLAYER_ADD_DRIVER([stage],[drivers/stage],[yes],[],[],[])

PLAYER_ADD_DRIVER([laserbar],[drivers/fiducial],[yes],[],[],[])
PLAYER_ADD_DRIVER([laserbarcode],[drivers/fiducial],[yes],[],[],[])
PLAYER_ADD_DRIVER([laservisualbarcode],[drivers/fiducial],[yes],[],[],[])
PLAYER_ADD_DRIVER([laservisualbw],[drivers/fiducial],[yes],[],[],[])

dnl - added a header check to stop this building on OS X - rtv
PLAYER_ADD_DRIVER([linuxjoystick],[drivers/joystick],[yes],[linux/joystick.h],[],[])

dnl Camera drivers
PLAYER_ADD_DRIVER([camerav4l],[drivers/camera/v4l],[yes],[linux/videodev.h],[],[])

dnl IEEE1394 (Firewire) camera driver
PLAYER_ADD_DRIVER([camera1394],[drivers/camera/1394],[yes],["libraw1394/raw1394.h libdc1394/dc1394_control.h"],[],["-lraw1394 -ldc1394_control"])

dnl libdc1394 has varying API's, depending on the version.  Do some checks
dnl to see what the function signatures look like
if test "x$enable_camera1394" = "xyes"; then

  dc1394_dma_setup_args="0"

  AC_COMPILE_IFELSE(AC_LANG_PROGRAM(
    [[#include "libdc1394/dc1394_control.h"]], 
    [[dc1394_dma_setup_capture(NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL)]]), 
    dc1394_dma_setup_args="11")

  AC_COMPILE_IFELSE(AC_LANG_PROGRAM(
    [[#include "libdc1394/dc1394_control.h"]], 
    [[dc1394_dma_setup_capture(NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL)]]), 
    dc1394_dma_setup_args="12")

  AC_DEFINE_UNQUOTED(DC1394_DMA_SETUP_CAPTURE_ARGS, $dc1394_dma_setup_args, 
              [arg count for dma capture function])
fi

PLAYER_ADD_DRIVER([cameracompress],[drivers/camera/compress],[yes],[jpeglib.h],[],[-ljpeg])
PLAYER_ADD_DRIVER([imageseq],[drivers/camera],[yes],[],[],[],[OPENCV],[opencv])

dnl Service Discovery with libservicediscovery
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
PLAYER_ADD_DRIVER([service_adv_lsd], [drivers/service_adv], [yes],
    [servicediscovery/servicedirectory.hh], [], [-lservicediscovery])
AC_LANG_RESTORE

dnl Service Discovery with libhowl (mdns/zeroconf/rendezvous implementation)
PLAYER_ADD_DRIVER([service_adv_mdns],[drivers/service_adv],[yes],
                  [],[],[],[HOWL],[howl >= 0.9.6])
PLAYER_DRIVER_EXTRA_LIBS="$PLAYER_DRIVER_EXTRA_LIBS $HOWL_LIBS"

dnl PLAYER_ADD_DRIVER doesn't handle building more than one library, so
dnl do it manually
user_override=no
disable_reason="disabled by user"
AC_ARG_ENABLE(amcl,
[  --disable-amcl           Don't compile the amcl driver],
  user_override=yes,
  enable_amcl=yes)
if test "x$enable_alldrivers" = "xno" -a "x$user_override" = "xno"; then
  enable_amcl=no
fi
if test "x$enable_amcl" = "xyes"; then

  PKG_CHECK_MODULES(GSL,gsl,
                    enable_amcl=yes,
                    enable_amcl=no)
  if test "x$enable_amcl" = "xno"; then
    AC_CHECK_HEADER(gsl/gsl_version.h,
                    enable_amcl=yes
                    GSL_CFLAGS=
                    GSL_LIBS="-lgsl -lgslcblas",
                    enable_amcl=no
                    disable_reason="couldn't find the GSL")
  fi
fi
if test "x$enable_amcl" = "xyes"; then
  AC_DEFINE(INCLUDE_AMCL, 1, [[include the AMCL driver]])
  AMCL_LIB="libamcl.a"
  PLAYER_DRIVER_LIBS="$PLAYER_DRIVER_LIBS $AMCL_LIB"
  PLAYER_DRIVER_LIBPATHS="$PLAYER_DRIVER_LIBPATHS drivers/localization/amcl/libamcl.a"
  AMCL_PF_LIB="libpf.a"
  AMCL_MAP_LIB="libmap.a"
  AMCL_MODELS_LIB="libmodels.a"
dnl  PLAYER_DRIVER_EXTRA_LIBS="$PLAYER_DRIVER_EXTRA_LIBS -lgsl -lgslcblas"
  PLAYER_DRIVER_EXTRA_LIBS="$PLAYER_DRIVER_EXTRA_LIBS $GSL_LIBS"
  PLAYER_DRIVERS="$PLAYER_DRIVERS amcl"
else
  PLAYER_NODRIVERS="$PLAYER_NODRIVERS:amcl -- $disable_reason"
fi
AC_SUBST(AMCL_LIB)
AC_SUBST(AMCL_PF_LIB)
AC_SUBST(AMCL_MAP_LIB)
AC_SUBST(AMCL_MODELS_LIB)
AC_SUBST(GSL_CFLAGS)

dnl Add results from driver tests to compiler and link lines
PLAYER_DRIVER_EXTRA_LIBS="$PLAYER_DRIVER_EXTRA_LIBS $OPENCV_LIBS"

AC_SUBST(PLAYER_DRIVER_LIBS)
AC_SUBST(PLAYER_DRIVER_LIBPATHS)
AC_SUBST(PLAYER_DRIVER_EXTRA_LIBS)

])

dnl Find Gazebo
AC_DEFUN([GAZEBO_FIND],[

dnl Include Gazebo?
user_override=no
AC_ARG_ENABLE(gazebo,
[  --disable-gazebo           Don't compile the Gazebo driver],
user_override=yes,
enable_gazebo=yes)

if test "x$enable_alldrivers" = "xno" -a "x$user_override" = "xno"; then
  enable_gazebo=no
fi

if test "x$enable_gazebo"="xno"; then
  gazebo_disable_reason="disabled by user"
fi

dnl What version do we support?
GAZEBO_MIN_VERSION="0.4.0"

dnl Where is Gazebo?
if test "x$enable_gazebo" = "xyes"; then
  if test "x$have_pkg_config" = "xyes" ; then
    PKG_CHECK_MODULES(GAZEBO,gazebo >= $GAZEBO_MIN_VERSION, 
	  enable_gazebo=yes, 
    enable_gazebo=no; 
    gazebo_disable_reason="could not find Gazebo >= $GAZEBO_MIN_VERSION")
  else
    enable_gazebo=no;
    gazebo_disable_reason="pkg-config unavailable; maybe you should install it"
  fi
fi

dnl Check for some functions in 0.5
if test "x$enable_gazebo" = "xyes"; then

  AC_CHECK_LIB(gazebo,gz_client_wait,have_gz_client_wait=yes)
  if test "x$have_gz_client_wait" = "xyes"; then
    AC_DEFINE(HAVE_GZ_CLIENT_WAIT,1,[gz_client_wait is defined])
  fi

  AC_CHECK_MEMBER(gz_fiducial_fid_t.pose,have_member=yes,have_member=no,[#include <gazebo.h>])
  if test "x$have_member" = "xyes"; then
    AC_DEFINE(HAVE_GZ_FID_POSE,1,[gz_fiducial_fid_t.pose is defined])
  fi

  AC_CHECK_MEMBER(gz_gps_data_t.utc_time,have_member=yes,have_member=no,[#include <gazebo.h>])
  if test "x$have_member" = "xyes"; then
    AC_DEFINE(HAVE_GZ_GPS_UTC,1,[gz_gps_data_t.utc_time is defined])
  fi

fi



dnl If gazebo is not built; make sure the correct messages are
dnl printed
if test "x$enable_gazebo" = "xyes"; then
  PLAYER_DRIVERS="$PLAYER_DRIVERS gazebo"
else
  PLAYER_NODRIVERS="$PLAYER_NODRIVERS:gazebo -- $gazebo_disable_reason"
fi

dnl Set up the necessary vars
if test "x$enable_gazebo" = "xyes"; then
  AC_DEFINE(INCLUDE_GAZEBO, 1, [include the $1 driver])
  GAZEBO_LIB=libgazebo.a
  GAZEBO_LIB_PATH=drivers/gazebo/$GAZEBO_LIB
  GAZEBO_EXTRA_CPPFLAGS=$GAZEBO_CFLAGS
  GAZEBO_EXTRA_LFLAGS=
  GAZEBO_EXTRA_LIB=$GAZEBO_LIBS
fi

AC_SUBST(GAZEBO_LIB)
PLAYER_DRIVER_LIBS="$PLAYER_DRIVER_LIBS $GAZEBO_LIB"
PLAYER_DRIVER_LIBPATHS="$GAZEBO_LIB_PATH $PLAYER_DRIVER_LIBPATHS"
AC_SUBST(GAZEBO_EXTRA_CPPFLAGS)
PLAYER_DRIVER_EXTRA_LIBS="$PLAYER_DRIVER_EXTRA_LIBS $GAZEBO_EXTRA_LIB"

])


dnl Test to see if a particular Gazebo driver is available
AC_DEFUN([GAZEBO_TEST_DRIVER], [

AC_CHECK_LIB(gazebo, gz_$2_alloc, 
  include_gazebo_$1=yes, 
  include_gazebo_$1=no,
  [$GAZEBO_LIBS]
)

if test "x$include_gazebo_$1" = "xyes"; then
  AC_DEFINE([INCLUDE_GAZEBO_]translit($1,[a-z],[A-Z]),1,[include the gazebo $1 driver])
else
  AC_MSG_WARN([gazebo $1 not found; disabled])
fi

])


dnl This macro does all the Gazebo tests
AC_DEFUN([GAZEBO_DRIVERTESTS],[

dnl Test individual drivers
GAZEBO_TEST_DRIVER([sim],[sim])
GAZEBO_TEST_DRIVER([camera],[camera])
GAZEBO_TEST_DRIVER([factory],[factory])
GAZEBO_TEST_DRIVER([fiducial],[fiducial])
GAZEBO_TEST_DRIVER([gps],[gps])
GAZEBO_TEST_DRIVER([laser],[laser])
GAZEBO_TEST_DRIVER([position],[position])
GAZEBO_TEST_DRIVER([position3d],[position])
GAZEBO_TEST_DRIVER([power],[power])
GAZEBO_TEST_DRIVER([ptz],[ptz])
GAZEBO_TEST_DRIVER([truth],[truth])
GAZEBO_TEST_DRIVER([stereo],[stereo])
GAZEBO_TEST_DRIVER([gripper],[gripper])
GAZEBO_TEST_DRIVER([sonar],[sonar])
GAZEBO_TEST_DRIVER([sonars],[sonars])  dnl Deprecated; backwards compatability

])


