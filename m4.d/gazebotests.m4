
dnl Find Gazebo
AC_DEFUN([GAZEBO_FIND],[

dnl Include Gazebo?
AC_ARG_ENABLE(gazebo,
[  --disable-gazebo           Don't compile the Gazebo driver],
disable_reason="disabled by user",
enable_gazebo=yes)

dnl What version do we support?
GAZEBO_MIN_VERSION="0.3.0"

dnl Where is Gazebo?
if test "x$enable_gazebo" = "xyes"; then
  if test "x$have_pkg_config" = "xyes" ; then
    PKG_CHECK_MODULES(GAZEBO,gazebo >= $GAZEBO_MIN_VERSION, 
	  enable_gazebo=yes, 
    enable_gazebo=no 
    disable_reason="could not find gazebo >= $GAZEBO_MIN_VERSION")
  else
    enable_gazebo=no
    disable_reason="pkg-config unavailable; maybe you should install it"
  fi
fi
])

dnl Test to see if a particular Gazebo driver is available
AC_DEFUN([GAZEBO_TEST_DRIVER], [

AC_CHECK_LIB(gazebo, gz_$1_alloc, 
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
AC_DEFUN([GAZEBO_TESTS],[

GAZEBO_TEST_DRIVER([camera])
GAZEBO_TEST_DRIVER([factory])
GAZEBO_TEST_DRIVER([fiducial])
GAZEBO_TEST_DRIVER([gps])
GAZEBO_TEST_DRIVER([laser])
GAZEBO_TEST_DRIVER([position])
GAZEBO_TEST_DRIVER([power])
GAZEBO_TEST_DRIVER([ptz])
GAZEBO_TEST_DRIVER([truth])

dnl This is a complete bogus test because I cant get AC_CHECK_MEMBER to work
if test "x$include_gazebo_power" = "xyes"; then
  AC_DEFINE(HAS_GAZEBO_LASER_MAX_RANGE,1,[laser data has max range member])
fi

dnl The call to AC_CHECK_LIB above *should* append the gazebo linker flags
dnl to LIBS, but for some reason it doesn't, so we'll manually add them to
dnl PLAYER_DRIVER_EXTRA_LIBS
if test "x$enable_gazebo" = "xyes"; then
  PLAYER_DRIVER_EXTRA_LIBS="$PLAYER_DRIVER_EXTRA_LIBS $GAZEBO_LIBS"
fi
])

