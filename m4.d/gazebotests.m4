
dnl Find Gazebo
AC_DEFUN([GAZEBO_FIND],[

dnl Include Gazebo?
AC_ARG_ENABLE(gazebo,
[  --disable-gazebo           Don't compile the Gazebo driver],
disable_reason="disabled by user",
enable_gazebo=yes)

dnl Where is Gazebo?
AC_ARG_WITH(gazebo, [  --with-gazebo=dir       Location of Gazebo],
            GAZEBO_DIR=$with_gazebo,GAZEBO_DIR=$prefix)
if test "x$enable_gazebo" = "xyes"; then
if test "x$GAZEBO_DIR" = "xNONE" -o "x$GAZEBO_DIR" = "xno"; then
  GAZEBO_HEADER=gazebo.h
  GAZEBO_EXTRA_CPPFLAGS=
  GAZEBO_EXTRA_LDFLAGS=-lgazebo
elif test "x$GAZEBO_DIR" = "xyes"; then
  GAZEBO_HEADER=$prefix/include/gazebo.h
  GAZEBO_EXTRA_CPPFLAGS="-I$prefix/include"
  GAZEBO_EXTRA_LDFLAGS="-L$prefix/lib -lgazebo"
else
  GAZEBO_HEADER=$GAZEBO_DIR/include/gazebo.h
  GAZEBO_EXTRA_CPPFLAGS="-I$GAZEBO_DIR/include"
  GAZEBO_EXTRA_LDFLAGS="-L$GAZEBO_DIR/lib -lgazebo"
fi
else 
GAZEBO_EXTRA_CPPFLAGS=
GAZEBO_EXTRA_LDFLAGS=
fi

AC_SUBST(GAZEBO_HEADER)
AC_SUBST(GAZEBO_EXTRA_CPPFLAGS)
AC_SUBST(GAZEBO_EXTRA_LDFLAGS)
])


dnl Test to see if a particular Gazebo driver is available
AC_DEFUN([GAZEBO_TEST_DRIVER], [

AC_CHECK_LIB(gazebo, gz_$1_alloc, 
  include_gazebo_$1=yes, 
  include_gazebo_$1=no,
  [$GAZEBO_EXTRA_LDFLAGS]
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

])

