dnl Here are the tests for inclusion of Player's various device drivers

define(PLAYER_DRIVERTESTS, [

dnl the following drivers will install by default
AC_ARG_WITH(p2os,
[  --without-p2os          Don't compile the p2os drivers],,
with_p2os=yes)
if test "x$with_p2os" = "xyes"; then
  AC_DEFINE(INCLUDE_P2OS, 1, [[include the P2OS driver]])
  P2OS_LIB="libp2os.a"
  P2OS_LIBPATH="drivers/mixed/p2os/libp2os.a"
fi
AC_SUBST(P2OS_LIB)
AC_SUBST(P2OS_LIBPATH)


AC_ARG_WITH(sick,
[  --without-sick          Don't compile the sicklms200 driver],,
with_sick=yes)
if test "x$with_sick" = "xyes"; then
  AC_DEFINE(INCLUDE_SICK, 1, [[include the SICK driver]])
  SICK_LIB="libsicklms200.a"
  SICK_LIBPATH="drivers/laser/libsicklms200.a"
fi
AC_SUBST(SICK_LIB)
AC_SUBST(SICK_LIBPATH)

AC_ARG_WITH(acts,
[  --without-acts          Don't compile the acts driver],,
with_acts=yes)
if test "x$with_acts" = "xyes"; then
  AC_DEFINE(INCLUDE_ACTS, 1, [[include the ACTS driver]])
  ACTS_LIB="libacts.a"
  ACTS_LIBPATH="drivers/blobfinder/libacts.a"
fi
AC_SUBST(ACTS_LIB)
AC_SUBST(ACTS_LIBPATH)


AC_ARG_WITH(festival,
[  --without-festival      Don't compile the festival driver],,
with_festival=yes)
if test "x$with_festival" = "xyes"; then
  AC_DEFINE(INCLUDE_FESTIVAL, 1, [[include the FESTIVAL driver]])
  FESTIVAL_LIB="libfestival.a"
  FESTIVAL_LIBPATH="drivers/speech/libfestival.a"
fi
AC_SUBST(FESTIVAL_LIB)
AC_SUBST(FESTIVAL_LIBPATH)

AC_ARG_WITH(sony,
[  --without-sony          Don't compile the sonyevid30 driver],,
with_sony=yes)
if test "x$with_sony" = "xyes"; then
  AC_DEFINE(INCLUDE_SONY, 1, [[include the SONY driver]])
  SONY_LIB="libsonyevid30.a"
  SONY_LIBPATH="drivers/ptz/libsonyevid30.a"
fi
AC_SUBST(SONY_LIB)
AC_SUBST(SONY_LIBPATH)

AC_ARG_WITH(stage,
[  --without-stage         Don't compile the stage driver],,
with_stage=yes)
if test "x$with_stage" = "xyes"; then
  AC_DEFINE(INCLUDE_STAGE, 1, [[include the STAGE driver]])
  STAGE_LIB="libstage.a"
  STAGE_LIBPATH="drivers/stage/libstage.a"
fi
AC_SUBST(STAGE_LIB)
AC_SUBST(STAGE_LIBPATH)

AC_ARG_WITH(udpbcast,
[  --without-udpbcast      Don't compile the udpbroadcast driver],,
with_udpbcast=yes)
if test "x$with_udpbcast" = "xyes"; then
  AC_DEFINE(INCLUDE_UDPBCAST, 1, [[include the UDPBCAST driver]])
  UDPBCAST_LIB="libudpbroadcast.a"
  UDPBCAST_LIBPATH="drivers/comms/libudpbroadcast.a"
fi
AC_SUBST(UDPBCAST_LIB)
AC_SUBST(UDPBCAST_LIBPATH)

AC_ARG_WITH(laser,
[  --without-laser         Don't compile the laser-based fiducial drivers],,
with_laser=yes)
if test "x$with_laser" = "xyes"; then
  AC_DEFINE(INCLUDE_LASERFIDUCIAL, 1, [[include the LASER-based fiducial drivers]])
  LASERFIDUCIAL_LIBS="liblaserbar.a liblaserbarcode.a liblaservisualbarcode.a"
  LASERFIDUCIAL_LIBSPATH="drivers/fiducial/liblaserbar.a drivers/fiducial/liblaserbarcode.a drivers/fiducial/liblaservisualbarcode.a"
fi
AC_SUBST(LASERFIDUCIAL_LIBS)
AC_SUBST(LASERFIDUCIAL_LIBSPATH)

AC_ARG_WITH(lasercspace, 
[  --without-lasercspace     Don't compile the lasercspace driver],,
with_lasercspace=yes)
if test "x$with_lasercspace" = "xyes"; then
  AC_DEFINE(INCLUDE_WIFI, 1, [[include the WiFi driver]])
  LASERCSPACE_LIB="liblasercspace.a"
  LASERCSPACE_LIBPATH="drivers/laser/liblasercspace.a"
fi
AC_SUBST(LASERCSPACE_LIB)
AC_SUBST(LASERCSPACE_LIBPATH)

AC_ARG_WITH(linuxwifi, 
[  --without-linuxwifi     Don't compile the linuxwifi driver],,
with_linuxwifi=yes)
if test "x$with_linuxwifi" = "xyes"; then
  AC_DEFINE(INCLUDE_WIFI, 1, [[include the WiFi driver]])
  LINUXWIFI_LIB="liblinuxwifi.a"
  LINUXWIFI_LIBPATH="drivers/wifi/liblinuxwifi.a"
fi
AC_SUBST(LINUXWIFI_LIB)
AC_SUBST(LINUXWIFI_LIBPATH)

AC_ARG_WITH(aodv, 
[  --without-aodv     Don't compile the aodv driver],,
with_aodv=no)
if test "x$with_aodv" = "xyes"; then
  AC_DEFINE(INCLUDE_AODV, 1, [[include the AODV driver]])
  AODV_LIB="libaodv.a"
  AODV_LIBPATH="drivers/wifi/libaodv.a"
fi
AC_SUBST(AODV_LIB)
AC_SUBST(AODV_LIBPATH)

AC_ARG_WITH(iwspy, 
[  --with-iwspy     Compile the iwspy driver],,
with_iwspy=no)
if test "x$with_iwspy" = "xyes"; then
  AC_DEFINE(INCLUDE_IWSPY, 1, [[include the iwspy driver]])
  IWSPY_LIB="libiwspy.a"
  IWSPY_LIBPATH="drivers/wifi/libiwspy.a"
fi
AC_SUBST(IWSPY_LIB)
AC_SUBST(IWSPY_LIBPATH)

dnl check for the fftw library (by way of one of its headers) and compile
dnl the fixed tones driver if its found
AC_ARG_WITH(fixedtones,
[  --without-fixedtones    Don't compile the fixedtones driver],,
[AC_CHECK_HEADER(rfftw.h, with_fixedtones=yes,,)])
if test "x$with_fixedtones" = xyes; then
  AC_DEFINE(INCLUDE_FIXEDTONES,1,[include the fixed tones driver])
  FIXEDTONES_LIB="libfixedtones.a"
  FIXEDTONES_LIBPATH="drivers/audio/libfixedtones.a"
  FIXEDTONES_EXTRA_LIB="-lrfftw -lfftw"
fi
AC_SUBST(FIXEDTONES_LIB)
AC_SUBST(FIXEDTONES_LIBPATH)
AC_SUBST(FIXEDTONES_EXTRA_LIB)

dnl check for Mobility (by way of one of its headers) and compile
dnl the RWI driver if it's found
AC_ARG_WITH(rwi,
[  --without-rwi           Don't compile the rwi drivers],,
[AC_CHECK_HEADER(mobilitycomponents_i.h, with_rwi=yes,,)])
if test "x$with_rwi" = "xyes"; then
  AC_DEFINE(INCLUDE_RWI,1,[include the RWI driver])
  AC_DEFINE(USE_MOBILITY,1,[use the RWI Mobility interface])
  RWI_LIB="librwi.a"
  RWI_LIBPATH="drivers/mixed/rwi/librwi.a"
fi
AC_SUBST(RWI_LIB)
AC_SUBST(RWI_LIBPATH)

dnl check for the isense library (by way of one of its headers) and compile
dnl the Intersense drivers if foune.
AC_ARG_WITH(isense,
[  --without-isense        Don't compile the isense drivers],,
[AC_CHECK_HEADER(isense/isense.h, with_isense=yes,,)])
if test "x$with_isense" = "xyes"; then
  AC_DEFINE(INCLUDE_ISENSE,1,[include the InterSense driver])
  ISENSE_LIB="libisense.a"
  ISENSE_LIBPATH="drivers/position/isense/libisense.a"
  ISENSE_EXTRA_LIB="-lisense"
fi
AC_SUBST(ISENSE_LIB)
AC_SUBST(ISENSE_LIBPATH)
AC_SUBST(ISENSE_EXTRA_LIB)

dnl the following must use --with to be built
AC_ARG_WITH(reb,
[  --with-reb              Compile the reb drivers],,
with_reb=no)
if test "x$with_reb" = "xyes"; then
  AC_DEFINE(INCLUDE_REB, 1, [[include the REB driver]])
  REB_LIB="libreb.a"
  REB_LIBPATH="drivers/mixed/reb/libreb.a"
fi
AC_SUBST(REB_LIB)
AC_SUBST(REB_LIBPATH)


dnl optionally compile MicroStrain IMU drivers - disabled by default
AC_ARG_WITH(microstrain,
[  --with-microstrain      Compile the MicroStrain IMU drivers],,
with_microstrain=no)
if test "x$with_microstrain" = "xyes"; then
  AC_DEFINE(INCLUDE_MICROSTRAIN, 1, [[include the MICROSTRAIN driver]])
  MICROSTRAIN_LIB="libmicrostrain.a"
  MICROSTRAIN_LIBPATH="drivers/position/microstrain/libmicrostrain.a"
fi
AC_SUBST(MICROSTRAIN_LIB)
AC_SUBST(MICROSTRAIN_LIBPATH)

dnl optionally compile the audio waveform driver - disabled by default
AC_ARG_WITH(waveaudio,
[  --with-waveaudio        Compile the wave audio driver],,with_waveaudio=no)
if test "x$with_waveaudio" = "xyes"; then
  dnl check that we have the soundcard header available
  AC_CHECK_HEADER(sys/soundcard.h, with_waveaudio=yes, with_waveaudio=no,)
  if test "x$with_waveaudio" = "xyes"; then
    AC_DEFINE(INCLUDE_WAVEAUDIO, 1, [[include the WAVEAUDIO driver]])
    WAVEAUDIO_LIB="libwaveaudio.a"
    WAVEAUDIO_LIBPATH="drivers/waveform/libwaveaudio.a"
  fi
fi
AC_SUBST(WAVEAUDIO_LIB)
AC_SUBST(WAVEAUDIO_LIBPATH)

])
