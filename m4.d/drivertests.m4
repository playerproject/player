dnl Here are the tests for inclusion of Player's various device drivers

define(PLAYER_DRIVERTESTS, [

dnl the following drivers will install by default
AC_ARG_ENABLE(passthrough,
[  --disable-passthrough   Don't compile the passthrough drivers],,
enable_passthrough=yes)
if test "x$enable_passthrough" = "xyes"; then
  AC_DEFINE(INCLUDE_PASSTHROUGH, 1, [[include the passthrough driver]])
  PASSTHROUGH_LIB="libpassthrough.a"
  PASSTHROUGH_LIBPATH="drivers/shell/libpassthrough.a"
  PASSTHROUGH_EXTRA_LIB="-L../client_libs/c -lplayercclient"
fi
AC_SUBST(PASSTHROUGH_LIB)
AC_SUBST(PASSTHROUGH_LIBPATH)
AC_SUBST(PASSTHROUGH_EXTRA_LIB)

AC_ARG_ENABLE(p2os,
[  --disable-p2os          Don't compile the p2os drivers],,
enable_p2os=yes)
if test "x$enable_p2os" = "xyes"; then
  AC_DEFINE(INCLUDE_P2OS, 1, [[include the P2OS driver]])
  P2OS_LIB="libp2os.a"
  P2OS_LIBPATH="drivers/mixed/p2os/libp2os.a"
fi
AC_SUBST(P2OS_LIB)
AC_SUBST(P2OS_LIBPATH)


AC_ARG_ENABLE(sick,
[  --disable-sick          Don't compile the sicklms200 driver],,
enable_sick=yes)
if test "x$enable_sick" = "xyes"; then
  AC_DEFINE(INCLUDE_SICK, 1, [[include the SICK driver]])
  SICK_LIB="libsicklms200.a"
  SICK_LIBPATH="drivers/laser/libsicklms200.a"
fi
AC_SUBST(SICK_LIB)
AC_SUBST(SICK_LIBPATH)

AC_ARG_ENABLE(acts,
[  --disable-acts          Don't compile the acts driver],,
enable_acts=yes)
if test "x$enable_acts" = "xyes"; then
  AC_DEFINE(INCLUDE_ACTS, 1, [[include the ACTS driver]])
  ACTS_LIB="libacts.a"
  ACTS_LIBPATH="drivers/blobfinder/libacts.a"
fi
AC_SUBST(ACTS_LIB)
AC_SUBST(ACTS_LIBPATH)


AC_ARG_ENABLE(festival,
[  --disable-festival      Don't compile the festival driver],,
enable_festival=yes)
if test "x$enable_festival" = "xyes"; then
  AC_DEFINE(INCLUDE_FESTIVAL, 1, [[include the FESTIVAL driver]])
  FESTIVAL_LIB="libfestival.a"
  FESTIVAL_LIBPATH="drivers/speech/libfestival.a"
fi
AC_SUBST(FESTIVAL_LIB)
AC_SUBST(FESTIVAL_LIBPATH)

AC_ARG_ENABLE(sony,
[  --disable-sony          Don't compile the sonyevid30 driver],,
enable_sony=yes)
if test "x$enable_sony" = "xyes"; then
  AC_DEFINE(INCLUDE_SONY, 1, [[include the SONY driver]])
  SONY_LIB="libsonyevid30.a"
  SONY_LIBPATH="drivers/ptz/libsonyevid30.a"
fi
AC_SUBST(SONY_LIB)
AC_SUBST(SONY_LIBPATH)

AC_ARG_ENABLE(stage,
[  --disable-stage         Don't compile the stage driver],,
enable_stage=yes)
if test "x$enable_stage" = "xyes"; then
  AC_DEFINE(INCLUDE_STAGE, 1, [[include the STAGE driver]])
  STAGE_LIB="libstage.a"
  STAGE_LIBPATH="drivers/stage/libstage.a"
fi
AC_SUBST(STAGE_LIB)
AC_SUBST(STAGE_LIBPATH)

AC_ARG_ENABLE(udpbcast,
[  --disable-udpbcast      Don't compile the udpbroadcast driver],,
enable_udpbcast=yes)
if test "x$enable_udpbcast" = "xyes"; then
  AC_DEFINE(INCLUDE_UDPBCAST, 1, [[include the UDPBCAST driver]])
  UDPBCAST_LIB="libudpbroadcast.a"
  UDPBCAST_LIBPATH="drivers/comms/libudpbroadcast.a"
fi
AC_SUBST(UDPBCAST_LIB)
AC_SUBST(UDPBCAST_LIBPATH)

AC_ARG_ENABLE(laser,
[  --disable-laser         Don't compile the laser-based fiducial drivers],,
enable_laser=yes)
if test "x$enable_laser" = "xyes"; then
  AC_DEFINE(INCLUDE_LASERFIDUCIAL, 1, [[include the LASER-based fiducial drivers]])
  LASERFIDUCIAL_LIBS="liblaserbar.a liblaserbarcode.a liblaservisualbarcode.a"
  LASERFIDUCIAL_LIBSPATH="drivers/fiducial/liblaserbar.a drivers/fiducial/liblaserbarcode.a drivers/fiducial/liblaservisualbarcode.a"
fi
AC_SUBST(LASERFIDUCIAL_LIBS)
AC_SUBST(LASERFIDUCIAL_LIBSPATH)

AC_ARG_ENABLE(lasercspace, 
[  --disable-lasercspace   Don't compile the lasercspace driver],,
enable_lasercspace=yes)
if test "x$enable_lasercspace" = "xyes"; then
  AC_DEFINE(INCLUDE_LASERCSPACE, 1, [[include the lasercspace driver]])
  LASERCSPACE_LIB="liblasercspace.a"
  LASERCSPACE_LIBPATH="drivers/laser/liblasercspace.a"
fi
AC_SUBST(LASERCSPACE_LIB)
AC_SUBST(LASERCSPACE_LIBPATH)

AC_ARG_ENABLE(linuxwifi, 
[  --disable-linuxwifi     Don't compile the linuxwifi driver],,
[AC_CHECK_HEADER(linux/wireless.h, enable_linuxwifi=yes,,)])
if test "x$enable_linuxwifi" = "xyes"; then
  AC_DEFINE(INCLUDE_WIFI, 1, [[include the WiFi driver]])
  LINUXWIFI_LIB="liblinuxwifi.a"
  LINUXWIFI_LIBPATH="drivers/wifi/liblinuxwifi.a"
fi
AC_SUBST(LINUXWIFI_LIB)
AC_SUBST(LINUXWIFI_LIBPATH)

dnl check for the fftw library (by way of one of its headers) and compile
dnl the fixed tones driver if its found
AC_ARG_ENABLE(fixedtones,
[  --disable-fixedtones    Don't compile the fixedtones driver],,
[AC_CHECK_HEADER(rfftw.h, enable_fixedtones=yes,,)])
if test "x$enable_fixedtones" = xyes; then
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
AC_ARG_ENABLE(rwi,
[  --disable-rwi           Don't compile the rwi drivers],,
[AC_CHECK_HEADER(mobilitycomponents_i.h, enable_rwi=yes,,)])
if test "x$enable_rwi" = "xyes"; then
  AC_DEFINE(INCLUDE_RWI,1,[include the RWI driver])
  AC_DEFINE(USE_MOBILITY,1,[use the RWI Mobility interface])
  RWI_LIB="librwi.a"
  RWI_LIBPATH="drivers/mixed/rwi/librwi.a"
fi
AC_SUBST(RWI_LIB)
AC_SUBST(RWI_LIBPATH)

dnl check for the isense library (by way of one of its headers) and compile
dnl the Intersense drivers if foune.
AC_ARG_ENABLE(isense,
[  --disable-isense        Don't compile the isense drivers],,
[AC_CHECK_HEADER(isense/isense.h, enable_isense=yes,,)])
if test "x$enable_isense" = "xyes"; then
  AC_DEFINE(INCLUDE_ISENSE,1,[include the InterSense driver])
  ISENSE_LIB="libisense.a"
  ISENSE_LIBPATH="drivers/position/isense/libisense.a"
  ISENSE_EXTRA_LIB="-lisense"
fi
AC_SUBST(ISENSE_LIB)
AC_SUBST(ISENSE_LIBPATH)
AC_SUBST(ISENSE_EXTRA_LIB)

dnl the following must use --with to be built
AC_ARG_ENABLE(aodv, 
[  --disable-aodv          Compile the aodv driver],,
enable_aodv=no)
if test "x$enable_aodv" = "xyes"; then
  AC_DEFINE(INCLUDE_AODV, 1, [[include the AODV driver]])
  AODV_LIB="libaodv.a"
  AODV_LIBPATH="drivers/wifi/libaodv.a"
fi
AC_SUBST(AODV_LIB)
AC_SUBST(AODV_LIBPATH)

AC_ARG_ENABLE(iwspy, 
[  --enable-iwspy          Compile the iwspy driver],,
enable_iwspy=no)
if test "x$enable_iwspy" = "xyes"; then
  AC_DEFINE(INCLUDE_IWSPY, 1, [[include the iwspy driver]])
  IWSPY_LIB="libiwspy.a"
  IWSPY_LIBPATH="drivers/wifi/libiwspy.a"
fi
AC_SUBST(IWSPY_LIB)
AC_SUBST(IWSPY_LIBPATH)

AC_ARG_ENABLE(reb,
[  --enable-reb            Compile the reb drivers],,
enable_reb=no)
if test "x$enable_reb" = "xyes"; then
  AC_DEFINE(INCLUDE_REB, 1, [[include the REB driver]])
  REB_LIB="libreb.a"
  REB_LIBPATH="drivers/mixed/reb/libreb.a"
fi
AC_SUBST(REB_LIB)
AC_SUBST(REB_LIBPATH)


dnl optionally compile MicroStrain IMU drivers - disabled by default
AC_ARG_ENABLE(microstrain,
[  --enable-microstrain    Compile the MicroStrain IMU drivers],,
enable_microstrain=no)
if test "x$enable_microstrain" = "xyes"; then
  AC_DEFINE(INCLUDE_MICROSTRAIN, 1, [[include the MICROSTRAIN driver]])
  MICROSTRAIN_LIB="libmicrostrain.a"
  MICROSTRAIN_LIBPATH="drivers/position/microstrain/libmicrostrain.a"
fi
AC_SUBST(MICROSTRAIN_LIB)
AC_SUBST(MICROSTRAIN_LIBPATH)

dnl optionally compile the audio waveform driver - disabled by default
AC_ARG_ENABLE(waveaudio,
[  --enable-waveaudio      Compile the wave audio driver],,enable_waveaudio=no)
if test "x$enable_waveaudio" = "xyes"; then
  dnl check that we have the soundcard header available
  AC_CHECK_HEADER(sys/soundcard.h, enable_waveaudio=yes, enable_waveaudio=no,)
  if test "x$enable_waveaudio" = "xyes"; then
    AC_DEFINE(INCLUDE_WAVEAUDIO, 1, [[include the WAVEAUDIO driver]])
    WAVEAUDIO_LIB="libwaveaudio.a"
    WAVEAUDIO_LIBPATH="drivers/waveform/libwaveaudio.a"
  fi
fi
AC_SUBST(WAVEAUDIO_LIB)
AC_SUBST(WAVEAUDIO_LIBPATH)

dnl optionally compile the mcl driver -- disabled by default
AC_ARG_ENABLE(mcl,
[  --enable-mcl            Compile the mcl driver],,enable_mcl=no)
if test "x$enable_mcl" = "xyes"; then
  AC_DEFINE(INCLUDE_MCL, 1, [[include the MCL driver]])
  MCL_LIB="libmcl.a"
  MCL_LIBPATH="drivers/localization/mcl/libmcl.a"
fi
AC_SUBST(MCL_LIB)
AC_SUBST(MCL_LIBPATH)

dnl optionally compile the amcl driver -- disabled by default
AC_ARG_ENABLE(amcl,
[  --enable-amcl           Compile the amcl driver],,enable_amcl=no)
if test "x$enable_amcl" = "xyes"; then
  AC_CHECK_HEADER(gsl/gsl_version.h,,
    AC_MSG_ERROR([The GNU Scientific Library (gsl) is required to build the
                  amcl driver; pass --disable-amcl to configure.]))
  AC_DEFINE(INCLUDE_AMCL, 1, [[include the AMCL driver]])
  AMCL_LIB="libamcl.a"
  AMCL_LIBPATH="drivers/localization/amcl/libamcl.a"
  AMCL_PF_LIB="libpf.a"
  AMCL_MAP_LIB="libmap.a"
  AMCL_MODELS_LIB="libmodels.a"
  AMCL_EXTRA_LIB="-lgsl -lgslcblas"
fi
AC_SUBST(AMCL_LIB)
AC_SUBST(AMCL_LIBPATH)
AC_SUBST(AMCL_PF_LIB)
AC_SUBST(AMCL_MAP_LIB)
AC_SUBST(AMCL_MODELS_LIB)
AC_SUBST(AMCL_EXTRA_LIB)

dnl optionally compile the incremental nav driver -- disabled by default
AC_ARG_ENABLE(inav,
[  --enable-inav           Compile the inav driver],,enable_inav=no)
if test "x$enable_inav" = "xyes"; then
  AC_CHECK_HEADER(gsl/gsl_version.h,,
    AC_MSG_ERROR([The GNU Scientific Library (gsl) is required to build the
                  inav driver; pass --disable-inav to configure.]))
  AC_DEFINE(INCLUDE_INAV, 1, [[include the INAV driver]])
  INAV_LIB="libinav.a"
  INAV_LIBPATH="drivers/position/inav/libinav.a"
  INAV_IMAP_LIB="libimap.a"
  INAV_EXTRA_LIB="-lgsl -lgslcblas"
fi
AC_SUBST(INAV_LIB)
AC_SUBST(INAV_LIBPATH)
AC_SUBST(INAV_EXTRA_LIB)
AC_SUBST(INAV_IMAP_LIB)

PLAYER_DRIVER_LIBS="$PASSTHROUGH_LIB $P2OS_LIB $SICK_LIB $ACTS_LIB $FESTIVAL_LIB $SONY_LIB $STAGE_LIB $UDPBCAST_LIB $LASERFIDUCIAL_LIBS $LASERCSPACE_LIB $LINUXWIFI_LIB $AODV_LIB $IWSPY_LIB $FIXEDTONES_LIB $RWI_LIB $ISENSE_LIB $REB_LIB $MICROSTRAIN_LIB $WAVEAUDIO_LIB $MCL_LIB $AMCL_LIB $INAV_LIB"

PLAYER_DRIVER_LIBPATHS="$PASSTHROUGH_LIBPATH $P2OS_LIBPATH $SICK_LIBPATH $ACTS_LIBPATH $FESTIVAL_LIBPATH $SONY_LIBPATH $STAGE_LIBPATH $UDPBCAST_LIBPATH $LASERFIDUCIAL_LIBSPATH $LASERCSPACE_LIBPATH $LINUXWIFI_LIBPATH $AODV_LIBPATH $IWSPY_LIBPATH $FIXEDTONES_LIBPATH $RWI_LIBPATH $ISENSE_LIBPATH $REB_LIBPATH $MICROSTRAIN_LIBPATH $WAVEAUDIO_LIBPATH $MCL_LIBPATH $AMCL_LIBPATH $INAV_LIBPATH"

PLAYER_DRIVER_EXTRA_LIBS="$PASSTHROUGH_EXTRA_LIB $FIXEDTONES_EXTRA_LIB $ISENSE_EXTRA_LIB $AMCL_EXTRA_LIB $INAV_EXTRA_LIB"

AC_SUBST(PLAYER_DRIVER_LIBS)
AC_SUBST(PLAYER_DRIVER_LIBPATHS)
AC_SUBST(PLAYER_DRIVER_EXTRA_LIBS)

])

