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
    AC_CHECK_LIB(rtk,rtk_init,
      AC_CHECK_LIB(rtk,LIBRTK_VERSION_2_2,
        with_rtk=yes,
        AC_MSG_WARN([Your librtk installation is too old (< 2.2).])
        AC_MSG_WARN([Either download a newer version of librtk])
        AC_MSG_WARN([or pass --without-rtk to configure.])
        AC_MSG_ERROR([RTK is too old]),
        [$LIBJPEG `gtk-config --libs gtk gthread`]),
      with_rtk=no,
      [$LIBJPEG `gtk-config --libs gtk gthread`])
    RTK_CPPFLAGS="`gtk-config --cflags`"
    RTK_LDADD="-lrtk $LIBJPEG `gtk-config --libs gtk gthread`"
  elif test ! "x$RTK_DIR" = "xno"; then
    AC_CHECK_LIB(rtk,rtk_init,
      AC_CHECK_LIB(rtk,LIBRTK_VERSION_2_2,
        with_rtk=yes
        RTK_CPPFLAGS="-I$RTK_DIR/include `gtk-config --cflags`"
        RTK_LDADD="-L$RTK_DIR/lib -lrtk $LIBJPEG `gtk-config --libs gtk gthread`",
        AC_MSG_WARN([Your librtk installation is too old (< 2.2).])
        AC_MSG_WARN([Either download a newer version of librtk])
        AC_MSG_WARN([or pass --without-rtk to configure.])
        AC_MSG_ERROR([RTK is too old]),
        [-L$RTK_DIR/lib $LIBJPEG `gtk-config --libs gtk gthread`]),
      with_rtk=no,
      [-L$RTK_DIR/lib $LIBJPEG `gtk-config --libs gtk gthread`])
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


