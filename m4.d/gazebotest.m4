dnl Find Gazebo
AC_DEFUN([GAZEBO_FIND],[

dnl Include Gazebo?
AC_ARG_ENABLE(gazebo,
[  --disable-gazebo           Don't compile the Gazebo driver],,
enable_gazebo=yes)

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
PLAYER_DRIVER_LIBPATHS="$PLAYER_DRIVER_LIBPATHS $GAZEBO_LIB_PATH"
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
GAZEBO_TEST_DRIVER([gripper],[gripper])
GAZEBO_TEST_DRIVER([sonars],[sonars])
GAZEBO_TEST_DRIVER([hud],[hud])

])
