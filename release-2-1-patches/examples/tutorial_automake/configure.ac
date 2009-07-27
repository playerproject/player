AC_INIT()
AM_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE(example,0.1)

dnl Check for c
AC_PROG_CC

dnl Checks for c++
AC_LANG_CPLUSPLUS
AC_PROG_CXX

# Player C++ Library
PKG_CHECK_MODULES(PLAYERCC, playerc++)
AC_SUBST(PLAYERCC_CFLAGS)
AC_SUBST(PLAYERCC_LIBS)


AC_OUTPUT([Makefile
           doc/Makefile
           src/Makefile
          ])