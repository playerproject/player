define([RTK2_TESTS],
[AC_ARG_WITH(rtk,    [  --without-rtk           Don't build RTK-based the GUI(s)],,
with_rtk=yes)
AM_CONDITIONAL(WITH_RTK_GUI, test x$with_rtk = xyes)

dnl we need GTK to build RTK-based GUIs (e.g., playerv)
AC_CHECK_PROG([have_gtk], [gtk-config], [yes], [no])
AM_CONDITIONAL(HAVE_GTK, test x$have_gtk = xyes)

dnl RTK2 uses libjpeg to export images.
AC_CHECK_HEADER(jpeglib.h,
  AC_DEFINE(HAVE_JPEGLIB_H,1,[include jpeg support])
  LIBJPEG="-ljpeg",,)
AC_SUBST(LIBJPEG)])

