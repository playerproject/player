dnl Here are the tests for inclusion of Player's various device drivers

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
if test "x$enable_$1" = "xyes" -a len($5) -gt 0; then
  for header in $5; do
    AC_CHECK_HEADER($header, enable_$1=yes, enable_$1=no,)
  done
fi
if test "x$enable_$1" = "xyes"; then
  AC_DEFINE([INCLUDE_]name_caps, 1, [include the $1 driver])
  name_caps[_LIB]=[lib]$1[.a]
  name_caps[_LIBPATH]=$2/$name_caps[_LIB]
  name_caps[_EXTRA_LIB]=$4
  PLAYER_DRIVERS="$PLAYER_DRIVERS $1"
else      
  PLAYER_NODRIVERS="$PLAYER_NODRIVERS $1"
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
  AC_CHECK_HEADER($4, enable_$1=yes, enable_$1=no,)
fi
if test "x$enable_$1" = "xyes"; then
  AC_DEFINE([INCLUDE_]other_name_caps, 1, [include the $1 driver])
  other_name_caps[_LIB]=[lib]$1[.a]
  other_name_caps[_LIBPATH]=$2/$other_name_caps[_LIB]
  other_name_caps[_EXTRA_CPPFLAGS]=$5
  other_name_caps[_EXTRA_LIB]=$6
  PLAYER_DRIVERS="$PLAYER_DRIVERS $1"
else
  PLAYER_NODRIVERS="$PLAYER_NODRIVERS $1"
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

PLAYER_ADD_DRIVER([festival],[drivers/speech],[yes],)

PLAYER_ADD_DRIVER([sonyevid30],[drivers/ptz],[yes],)

PLAYER_ADD_DRIVER([stage],[drivers/stage],[yes],)

PLAYER_ADD_DRIVER([udpbroadcast],[drivers/comms],[yes],)

PLAYER_ADD_DRIVER([lasercspace],[drivers/laser],[yes],)

PLAYER_ADD_DRIVER([linuxwifi],[drivers/wifi],[yes],
                  [],[linux/wireless.h])

PLAYER_ADD_DRIVER([fixedtones],[drivers/audio],[yes],
                  ["-lrfftw -lfftw"],[rfftw.h])

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

PLAYER_ADD_DRIVER([mcl],[drivers/localization/mcl],[no],)

PLAYER_ADD_DRIVER([inav],[drivers/position/inav],[no],["-lgsl -lgslcblas"],[gsl/gsl_version.h])

dnl Where is Gazebo?
AC_ARG_WITH(gazebo, [  --with-gazebo=dir     Location of Gazebo],
dnl Is it somewhere odd...
GAZEBO_HEADER=$with_gazebo/include/gazebo.h
GAZEBO_EXTRA_CPPFLAGS="-I$with_gazebo/include"
GAZEBO_EXTRA_LDFLAGS="-L$with_gazebo/lib -lgazebo",
dnl ...or is is in a standard place?
GAZEBO_HEADER=gazebo.h
GAZEBO_EXTRA_CPPFLAGS=""
GAZEBO_EXTRA_LDFLAGS="-lgazebo"
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
  LASERFIDUCIAL_LIBSPATH="drivers/fiducial/liblaserbar.a drivers/fiducial/liblaserbarcode.a drivers/fiducial/liblaservisualbarcode.a"
  PLAYER_DRIVERS="$PLAYER_DRIVERS laserbar laserbarcode laservisualbarcode"
else
  PLAYER_NODRIVERS="$PLAYER_NODRIVERS laserbar laserbarcode laservisualbarcode"
fi
AC_SUBST(LASERFIDUCIAL_LIBS)
AC_SUBST(LASERFIDUCIAL_LIBSPATH)

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
  AMCL_LIBPATH="drivers/localization/amcl/libamcl.a"
  AMCL_PF_LIB="libpf.a"
  AMCL_MAP_LIB="libmap.a"
  AMCL_MODELS_LIB="libmodels.a"
  AMCL_EXTRA_LIB="-lgsl -lgslcblas"
  PLAYER_DRIVERS="$PLAYER_DRIVERS amcl"
else
  PLAYER_NODRIVERS="$PLAYER_NODRIVERS amcl"
fi
AC_SUBST(AMCL_LIB)
AC_SUBST(AMCL_LIBPATH)
AC_SUBST(AMCL_PF_LIB)
AC_SUBST(AMCL_MAP_LIB)
AC_SUBST(AMCL_MODELS_LIB)
AC_SUBST(AMCL_EXTRA_LIB)

dnl Manually append LIB, LIBPATH, and EXTRA_LIB vars for those drivers that
dnl the PLAYER_ADD_DRIVER macro wasn't called.
PLAYER_DRIVER_LIBS="$PLAYER_DRIVER_LIBS $LASERFIDUCIAL_LIBS $AMCL_LIB"

PLAYER_DRIVER_LIBPATHS="$PLAYER_DRIVER_LIBPATHS $LASERFIDUCIAL_LIBSPATH $AMCL_LIBPATH"

PLAYER_DRIVER_EXTRA_LIBS="$PLAYER_DRIVER_EXTRA_LIBS $AMCL_EXTRA_LIB"

AC_SUBST(PLAYER_DRIVER_LIBS)
AC_SUBST(PLAYER_DRIVER_LIBPATHS)
AC_SUBST(PLAYER_DRIVER_EXTRA_LIBS)

])

