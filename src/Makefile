#
# $Id$
#
 
.SUFFIXES: .cc .o

SERVERNAME = player
DEVICELIBNAME = libplayerdevice.a

INCLUDES = -I../include

#CPP = g++
CPP = i386-glibc21-linux-g++

CFLAGS = -g -Wall $(PLAYER_PLATFORM)
LIBDIRS = -L.
LIBS = $(LIBDIRS) -lplayerdevice -lpthread -lm
MAKEDEP = makedepend
INSTALL = install
MKDIR = mkdir
RM = rm
AR = ar
RMDIR = rmdir

# comment to exclude support for the Stage simulator
INCLUDE_STAGE = -DINCLUDE_STAGE

# comment each one to remove support for that device
INCLUDE_LASER = -DINCLUDE_LASER
INCLUDE_SONAR = -DINCLUDE_SONAR
INCLUDE_VISION = -DINCLUDE_VISION
INCLUDE_POSITION = -DINCLUDE_POSITION
INCLUDE_GRIPPER = -DINCLUDE_GRIPPER
INCLUDE_MISC = -DINCLUDE_MISC
INCLUDE_PTZ = -DINCLUDE_PTZ
INCLUDE_SPEECH = -DINCLUDE_SPEECH
#INCLUDE_AUDIO = -DINCLUDE_AUDIO
# the following libs are only needed for the audio device
#EXTRA_LIBS = -lrfftw -lfftw
INCLUDE_LASERBEACON = -DINCLUDE_LASERBEACON
INCLUDE_BROADCAST = -DINCLUDE_BROADCAST

INCLUDED_DEVICES = $(INCLUDE_LASER) $(INCLUDE_SONAR) $(INCLUDE_VISION) \
                   $(INCLUDE_POSITION) $(INCLUDE_GRIPPER) $(INCLUDE_MISC) \
                   $(INCLUDE_MISC) $(INCLUDE_PTZ) $(INCLUDE_STAGE) \
                   $(INCLUDE_AUDIO) $(INCLUDE_LASERBEACON) \
                   $(INCLUDE_BROADCAST) $(INCLUDE_SPEECH)

INSTALL_BIN = $(INSTALL_PREFIX)/bin
INSTALL_INCLUDE = $(INSTALL_PREFIX)/include

OBS = nodevice.o clientdata.o laserdevice.o p2osdevice.o lock.o sonardevice.o positiondevice.o pubsub_util.o visiondevice.o gripperdevice.o miscdevice.o packet.o sip.o arenalock.o devicetable.o stagedevice.o ptzdevice.o laserbeacondevice.o broadcastdevice.o speechdevice.o

all: $(SERVERNAME)

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND" *.cc 2>/dev/null
	$(RM) -f Makefile.bak

clean:
	rm -f *.o *~ $(SERVERNAME) $(DEVICELIBNAME) core

$(DEVICELIBNAME): $(OBS)
	$(AR) cr $@ $(OBS)

$(SERVERNAME): main.cc $(DEVICELIBNAME) 
	$(CPP) $(CFLAGS) $(INCLUDES) $(INCLUDED_DEVICES) -o $@ main.cc $(LIBS) $(EXTRA_LIBS) $(PLATFORM_LIBS)

%.o: %.cc 
	$(CPP) $(CFLAGS) $(INCLUDES) -c $<


install: $(SERVERNAME)
	$(MKDIR) -p $(INSTALL_BIN)
	$(INSTALL) -m 755 $(SERVERNAME) $(INSTALL_BIN)
	$(MKDIR) -p $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 ../include/playercommon.h $(INSTALL_INCLUDE)/playercommon.h
	$(INSTALL) -m 644 ../include/messages.h $(INSTALL_INCLUDE)/messages.h
	$(INSTALL) -m 644 ../include/stage.h $(INSTALL_INCLUDE)/stage.h

uninstall:
	$(RM) -f $(INSTALL_BIN)/$(SERVERNAME)
	$(RM) -f $(INSTALL_INCLUDE)/playercommon.h
	$(RM) -f $(INSTALL_INCLUDE)/messages.h
	$(RM) -f $(INSTALL_INCLUDE)/stage.h

# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND


arenalock..o: ../include/arenalock.h ../include/device.h ../include/errors.h
arenalock..o: ../include/lock.h ../include/playercommon.h
arenalock..o: /usr/include/alloca.h /usr/include/asm/socket.h
arenalock..o: /usr/include/asm/sockios.h /usr/include/assert.h
arenalock..o: /usr/include/bits/byteswap.h /usr/include/bits/endian.h
arenalock..o: /usr/include/bits/in.h /usr/include/bits/initspin.h
arenalock..o: /usr/include/bits/ipc.h /usr/include/bits/local_lim.h
arenalock..o: /usr/include/bits/posix1_lim.h /usr/include/bits/posix2_lim.h
arenalock..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
arenalock..o: /usr/include/bits/select.h /usr/include/bits/sem.h
arenalock..o: /usr/include/bits/sigset.h /usr/include/bits/sigthread.h
arenalock..o: /usr/include/bits/sockaddr.h /usr/include/bits/socket.h
arenalock..o: /usr/include/bits/stdio_lim.h /usr/include/bits/time.h
arenalock..o: /usr/include/bits/types.h /usr/include/bits/waitflags.h
arenalock..o: /usr/include/bits/waitstatus.h /usr/include/bits/wchar.h
arenalock..o: /usr/include/bits/wordsize.h /usr/include/bits/xopen_lim.h
arenalock..o: /usr/include/endian.h /usr/include/features.h
arenalock..o: /usr/include/_G_config.h /usr/include/gconv.h
arenalock..o: /usr/include/gnu/stubs.h /usr/include/libio.h
arenalock..o: /usr/include/limits.h /usr/include/linux/limits.h
arenalock..o: /usr/include/netinet/in.h /usr/include/pthread.h
arenalock..o: /usr/include/sched.h /usr/include/signal.h /usr/include/stdint.h
arenalock..o: /usr/include/stdio.h /usr/include/stdlib.h
arenalock..o: /usr/include/sys/cdefs.h /usr/include/sys/ipc.h
arenalock..o: /usr/include/sys/select.h /usr/include/sys/sem.h
arenalock..o: /usr/include/sys/sysmacros.h /usr/include/sys/types.h
arenalock..o: /usr/include/time.h /usr/include/wchar.h /usr/include/xlocale.h
arenalock..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
arenalock..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
arenalock..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
arenalock..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
audiodevice..o: ../include/audiodevice.h ../include/device.h ../include/lock.h
audiodevice..o: ../include/playercommon.h /usr/include/alloca.h
audiodevice..o: /usr/include/asm/ioctl.h /usr/include/asm/ioctls.h
audiodevice..o: /usr/include/asm/sigcontext.h /usr/include/asm/socket.h
audiodevice..o: /usr/include/asm/sockios.h /usr/include/assert.h
audiodevice..o: /usr/include/bits/byteswap.h /usr/include/bits/confname.h
audiodevice..o: /usr/include/bits/endian.h /usr/include/bits/environments.h
audiodevice..o: /usr/include/bits/fcntl.h /usr/include/bits/huge_val.h
audiodevice..o: /usr/include/bits/in.h /usr/include/bits/initspin.h
audiodevice..o: /usr/include/bits/ioctls.h /usr/include/bits/ioctl-types.h
audiodevice..o: /usr/include/bits/local_lim.h /usr/include/bits/mathcalls.h
audiodevice..o: /usr/include/bits/mathdef.h /usr/include/bits/nan.h
audiodevice..o: /usr/include/bits/posix1_lim.h /usr/include/bits/posix2_lim.h
audiodevice..o: /usr/include/bits/posix_opt.h /usr/include/bits/pthreadtypes.h
audiodevice..o: /usr/include/bits/sched.h /usr/include/bits/select.h
audiodevice..o: /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h
audiodevice..o: /usr/include/bits/siginfo.h /usr/include/bits/signum.h
audiodevice..o: /usr/include/bits/sigset.h /usr/include/bits/sigstack.h
audiodevice..o: /usr/include/bits/sigthread.h /usr/include/bits/sockaddr.h
audiodevice..o: /usr/include/bits/socket.h /usr/include/bits/stat.h
audiodevice..o: /usr/include/bits/stdio_lim.h /usr/include/bits/termios.h
audiodevice..o: /usr/include/bits/time.h /usr/include/bits/types.h
audiodevice..o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
audiodevice..o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
audiodevice..o: /usr/include/bits/xopen_lim.h /usr/include/endian.h
audiodevice..o: /usr/include/fcntl.h /usr/include/features.h
audiodevice..o: /usr/include/_G_config.h /usr/include/gconv.h
audiodevice..o: /usr/include/getopt.h /usr/include/gnu/stubs.h
audiodevice..o: /usr/include/libio.h /usr/include/limits.h
audiodevice..o: /usr/include/linux/ioctl.h /usr/include/linux/limits.h
audiodevice..o: /usr/include/linux/soundcard.h /usr/include/math.h
audiodevice..o: /usr/include/netinet/in.h /usr/include/pthread.h
audiodevice..o: /usr/include/sched.h /usr/include/signal.h
audiodevice..o: /usr/include/stdint.h /usr/include/stdio.h
audiodevice..o: /usr/include/stdlib.h /usr/include/string.h
audiodevice..o: /usr/include/sys/cdefs.h /usr/include/sys/ioctl.h
audiodevice..o: /usr/include/sys/select.h /usr/include/sys/stat.h
audiodevice..o: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
audiodevice..o: /usr/include/sys/ttydefaults.h /usr/include/sys/types.h
audiodevice..o: /usr/include/sys/ucontext.h /usr/include/termios.h
audiodevice..o: /usr/include/time.h /usr/include/ucontext.h
audiodevice..o: /usr/include/unistd.h /usr/include/wchar.h
audiodevice..o: /usr/include/xlocale.h
audiodevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
audiodevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
audiodevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
audiodevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
broadcastdevice..o: ../include/broadcastdevice.hh ../include/device.h
broadcastdevice..o: ../include/lock.h ../include/messages.h
broadcastdevice..o: ../include/playercommon.h /usr/include/arpa/inet.h
broadcastdevice..o: /usr/include/asm/errno.h /usr/include/asm/socket.h
broadcastdevice..o: /usr/include/asm/sockios.h /usr/include/assert.h
broadcastdevice..o: /usr/include/bits/byteswap.h /usr/include/bits/confname.h
broadcastdevice..o: /usr/include/bits/endian.h
broadcastdevice..o: /usr/include/bits/environments.h /usr/include/bits/errno.h
broadcastdevice..o: /usr/include/bits/fcntl.h /usr/include/bits/in.h
broadcastdevice..o: /usr/include/bits/initspin.h /usr/include/bits/local_lim.h
broadcastdevice..o: /usr/include/bits/netdb.h /usr/include/bits/posix1_lim.h
broadcastdevice..o: /usr/include/bits/posix2_lim.h
broadcastdevice..o: /usr/include/bits/posix_opt.h
broadcastdevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
broadcastdevice..o: /usr/include/bits/select.h /usr/include/bits/siginfo.h
broadcastdevice..o: /usr/include/bits/sigset.h /usr/include/bits/sigthread.h
broadcastdevice..o: /usr/include/bits/sockaddr.h /usr/include/bits/socket.h
broadcastdevice..o: /usr/include/bits/stat.h /usr/include/bits/stdio_lim.h
broadcastdevice..o: /usr/include/bits/time.h /usr/include/bits/types.h
broadcastdevice..o: /usr/include/bits/uio.h /usr/include/bits/wchar.h
broadcastdevice..o: /usr/include/bits/wordsize.h /usr/include/bits/xopen_lim.h
broadcastdevice..o: /usr/include/endian.h /usr/include/errno.h
broadcastdevice..o: /usr/include/fcntl.h /usr/include/features.h
broadcastdevice..o: /usr/include/_G_config.h /usr/include/gconv.h
broadcastdevice..o: /usr/include/getopt.h /usr/include/gnu/stubs.h
broadcastdevice..o: /usr/include/libio.h /usr/include/limits.h
broadcastdevice..o: /usr/include/linux/errno.h /usr/include/linux/limits.h
broadcastdevice..o: /usr/include/netdb.h /usr/include/netinet/in.h
broadcastdevice..o: /usr/include/pthread.h /usr/include/rpc/netdb.h
broadcastdevice..o: /usr/include/sched.h /usr/include/signal.h
broadcastdevice..o: /usr/include/stdint.h /usr/include/stdio.h
broadcastdevice..o: /usr/include/string.h /usr/include/sys/cdefs.h
broadcastdevice..o: /usr/include/sys/select.h /usr/include/sys/socket.h
broadcastdevice..o: /usr/include/sys/stat.h /usr/include/sys/sysmacros.h
broadcastdevice..o: /usr/include/sys/types.h /usr/include/sys/uio.h
broadcastdevice..o: /usr/include/time.h /usr/include/unistd.h
broadcastdevice..o: /usr/include/wchar.h /usr/include/xlocale.h
broadcastdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
broadcastdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
broadcastdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
broadcastdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
clientdata..o: ../include/clientdata.h ../include/counter.h
clientdata..o: ../include/device.h ../include/devicetable.h ../include/lock.h
clientdata..o: ../include/messages.h ../include/packet.h
clientdata..o: ../include/playercommon.h /usr/include/asm/socket.h
clientdata..o: /usr/include/asm/sockios.h /usr/include/assert.h
clientdata..o: /usr/include/bits/byteswap.h /usr/include/bits/confname.h
clientdata..o: /usr/include/bits/endian.h /usr/include/bits/environments.h
clientdata..o: /usr/include/bits/in.h /usr/include/bits/initspin.h
clientdata..o: /usr/include/bits/local_lim.h /usr/include/bits/posix1_lim.h
clientdata..o: /usr/include/bits/posix2_lim.h /usr/include/bits/posix_opt.h
clientdata..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
clientdata..o: /usr/include/bits/select.h /usr/include/bits/sigset.h
clientdata..o: /usr/include/bits/sigthread.h /usr/include/bits/sockaddr.h
clientdata..o: /usr/include/bits/socket.h /usr/include/bits/stdio_lim.h
clientdata..o: /usr/include/bits/time.h /usr/include/bits/types.h
clientdata..o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
clientdata..o: /usr/include/bits/xopen_lim.h /usr/include/endian.h
clientdata..o: /usr/include/features.h /usr/include/_G_config.h
clientdata..o: /usr/include/gconv.h /usr/include/getopt.h
clientdata..o: /usr/include/gnu/stubs.h /usr/include/libio.h
clientdata..o: /usr/include/limits.h /usr/include/linux/limits.h
clientdata..o: /usr/include/netinet/in.h /usr/include/pthread.h
clientdata..o: /usr/include/sched.h /usr/include/signal.h
clientdata..o: /usr/include/stdint.h /usr/include/stdio.h
clientdata..o: /usr/include/string.h /usr/include/sys/cdefs.h
clientdata..o: /usr/include/sys/select.h /usr/include/sys/sysmacros.h
clientdata..o: /usr/include/sys/time.h /usr/include/sys/types.h
clientdata..o: /usr/include/time.h /usr/include/unistd.h /usr/include/wchar.h
clientdata..o: /usr/include/xlocale.h
clientdata..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
clientdata..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
clientdata..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
clientdata..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
devicetable..o: ../include/device.h ../include/devicetable.h ../include/lock.h
devicetable..o: ../include/playercommon.h /usr/include/assert.h
devicetable..o: /usr/include/bits/initspin.h /usr/include/bits/pthreadtypes.h
devicetable..o: /usr/include/bits/sched.h /usr/include/bits/sigset.h
devicetable..o: /usr/include/bits/sigthread.h /usr/include/bits/stdio_lim.h
devicetable..o: /usr/include/bits/time.h /usr/include/bits/types.h
devicetable..o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
devicetable..o: /usr/include/features.h /usr/include/_G_config.h
devicetable..o: /usr/include/gconv.h /usr/include/gnu/stubs.h
devicetable..o: /usr/include/libio.h /usr/include/pthread.h
devicetable..o: /usr/include/sched.h /usr/include/signal.h
devicetable..o: /usr/include/stdint.h /usr/include/stdio.h
devicetable..o: /usr/include/sys/cdefs.h /usr/include/time.h
devicetable..o: /usr/include/wchar.h
devicetable..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
devicetable..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
gripperdevice..o: ../include/device.h ../include/gripperdevice.h
gripperdevice..o: ../include/lock.h ../include/messages.h
gripperdevice..o: ../include/p2osdevice.h ../include/packet.h
gripperdevice..o: ../include/playercommon.h ../include/sip.h
gripperdevice..o: /usr/include/assert.h /usr/include/bits/initspin.h
gripperdevice..o: /usr/include/bits/local_lim.h /usr/include/bits/posix1_lim.h
gripperdevice..o: /usr/include/bits/posix2_lim.h
gripperdevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
gripperdevice..o: /usr/include/bits/select.h /usr/include/bits/sigset.h
gripperdevice..o: /usr/include/bits/sigthread.h /usr/include/bits/stdio_lim.h
gripperdevice..o: /usr/include/bits/time.h /usr/include/bits/types.h
gripperdevice..o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
gripperdevice..o: /usr/include/bits/xopen_lim.h /usr/include/features.h
gripperdevice..o: /usr/include/_G_config.h /usr/include/gconv.h
gripperdevice..o: /usr/include/gnu/stubs.h /usr/include/libio.h
gripperdevice..o: /usr/include/limits.h /usr/include/linux/limits.h
gripperdevice..o: /usr/include/pthread.h /usr/include/sched.h
gripperdevice..o: /usr/include/signal.h /usr/include/stdint.h
gripperdevice..o: /usr/include/stdio.h /usr/include/string.h
gripperdevice..o: /usr/include/sys/cdefs.h /usr/include/sys/select.h
gripperdevice..o: /usr/include/sys/time.h /usr/include/time.h
gripperdevice..o: /usr/include/values.h /usr/include/wchar.h
gripperdevice..o: /usr/include/xlocale.h
gripperdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/float.h
gripperdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
gripperdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
gripperdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
gripperdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
laserbeacondevice..o: ../include/device.h ../include/laserbeacondevice.hh
laserbeacondevice..o: ../include/lock.h ../include/messages.h
laserbeacondevice..o: ../include/playercommon.h /usr/include/asm/socket.h
laserbeacondevice..o: /usr/include/asm/sockios.h /usr/include/assert.h
laserbeacondevice..o: /usr/include/bits/byteswap.h
laserbeacondevice..o: /usr/include/bits/confname.h /usr/include/bits/endian.h
laserbeacondevice..o: /usr/include/bits/environments.h
laserbeacondevice..o: /usr/include/bits/huge_val.h /usr/include/bits/in.h
laserbeacondevice..o: /usr/include/bits/initspin.h
laserbeacondevice..o: /usr/include/bits/local_lim.h
laserbeacondevice..o: /usr/include/bits/mathcalls.h
laserbeacondevice..o: /usr/include/bits/mathdef.h /usr/include/bits/nan.h
laserbeacondevice..o: /usr/include/bits/posix1_lim.h
laserbeacondevice..o: /usr/include/bits/posix2_lim.h
laserbeacondevice..o: /usr/include/bits/posix_opt.h
laserbeacondevice..o: /usr/include/bits/pthreadtypes.h
laserbeacondevice..o: /usr/include/bits/sched.h /usr/include/bits/select.h
laserbeacondevice..o: /usr/include/bits/sigset.h /usr/include/bits/sigthread.h
laserbeacondevice..o: /usr/include/bits/sockaddr.h /usr/include/bits/socket.h
laserbeacondevice..o: /usr/include/bits/stdio_lim.h /usr/include/bits/time.h
laserbeacondevice..o: /usr/include/bits/types.h /usr/include/bits/wchar.h
laserbeacondevice..o: /usr/include/bits/wordsize.h
laserbeacondevice..o: /usr/include/bits/xopen_lim.h /usr/include/endian.h
laserbeacondevice..o: /usr/include/features.h /usr/include/_G_config.h
laserbeacondevice..o: /usr/include/gconv.h /usr/include/getopt.h
laserbeacondevice..o: /usr/include/gnu/stubs.h /usr/include/libio.h
laserbeacondevice..o: /usr/include/limits.h /usr/include/linux/limits.h
laserbeacondevice..o: /usr/include/math.h /usr/include/netinet/in.h
laserbeacondevice..o: /usr/include/pthread.h /usr/include/sched.h
laserbeacondevice..o: /usr/include/signal.h /usr/include/stdint.h
laserbeacondevice..o: /usr/include/stdio.h /usr/include/string.h
laserbeacondevice..o: /usr/include/sys/cdefs.h /usr/include/sys/select.h
laserbeacondevice..o: /usr/include/sys/sysmacros.h /usr/include/sys/types.h
laserbeacondevice..o: /usr/include/time.h /usr/include/unistd.h
laserbeacondevice..o: /usr/include/wchar.h /usr/include/xlocale.h
laserbeacondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
laserbeacondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
laserbeacondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
laserbeacondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
laserdevice..o: ../include/device.h ../include/laserdevice.h ../include/lock.h
laserdevice..o: ../include/messages.h ../include/playercommon.h
laserdevice..o: /usr/include/asm/sigcontext.h /usr/include/asm/socket.h
laserdevice..o: /usr/include/asm/sockios.h /usr/include/assert.h
laserdevice..o: /usr/include/bits/byteswap.h /usr/include/bits/confname.h
laserdevice..o: /usr/include/bits/endian.h /usr/include/bits/environments.h
laserdevice..o: /usr/include/bits/fcntl.h /usr/include/bits/in.h
laserdevice..o: /usr/include/bits/initspin.h /usr/include/bits/local_lim.h
laserdevice..o: /usr/include/bits/posix1_lim.h /usr/include/bits/posix2_lim.h
laserdevice..o: /usr/include/bits/posix_opt.h /usr/include/bits/pthreadtypes.h
laserdevice..o: /usr/include/bits/sched.h /usr/include/bits/select.h
laserdevice..o: /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h
laserdevice..o: /usr/include/bits/siginfo.h /usr/include/bits/signum.h
laserdevice..o: /usr/include/bits/sigset.h /usr/include/bits/sigstack.h
laserdevice..o: /usr/include/bits/sigthread.h /usr/include/bits/sockaddr.h
laserdevice..o: /usr/include/bits/socket.h /usr/include/bits/stat.h
laserdevice..o: /usr/include/bits/stdio_lim.h /usr/include/bits/termios.h
laserdevice..o: /usr/include/bits/time.h /usr/include/bits/types.h
laserdevice..o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
laserdevice..o: /usr/include/bits/xopen_lim.h /usr/include/endian.h
laserdevice..o: /usr/include/fcntl.h /usr/include/features.h
laserdevice..o: /usr/include/_G_config.h /usr/include/gconv.h
laserdevice..o: /usr/include/getopt.h /usr/include/gnu/stubs.h
laserdevice..o: /usr/include/libio.h /usr/include/limits.h
laserdevice..o: /usr/include/linux/limits.h /usr/include/netinet/in.h
laserdevice..o: /usr/include/pthread.h /usr/include/sched.h
laserdevice..o: /usr/include/signal.h /usr/include/stdint.h
laserdevice..o: /usr/include/stdio.h /usr/include/string.h
laserdevice..o: /usr/include/sys/cdefs.h /usr/include/sys/select.h
laserdevice..o: /usr/include/sys/stat.h /usr/include/sys/sysmacros.h
laserdevice..o: /usr/include/sys/time.h /usr/include/sys/ttydefaults.h
laserdevice..o: /usr/include/sys/types.h /usr/include/sys/ucontext.h
laserdevice..o: /usr/include/termios.h /usr/include/time.h
laserdevice..o: /usr/include/ucontext.h /usr/include/unistd.h
laserdevice..o: /usr/include/wchar.h /usr/include/xlocale.h
laserdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
laserdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
laserdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
laserdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
lock..o: ../include/device.h ../include/errors.h ../include/lock.h
lock..o: ../include/playercommon.h /usr/include/asm/socket.h
lock..o: /usr/include/asm/sockios.h /usr/include/assert.h
lock..o: /usr/include/bits/byteswap.h /usr/include/bits/endian.h
lock..o: /usr/include/bits/in.h /usr/include/bits/initspin.h
lock..o: /usr/include/bits/local_lim.h /usr/include/bits/posix1_lim.h
lock..o: /usr/include/bits/posix2_lim.h /usr/include/bits/pthreadtypes.h
lock..o: /usr/include/bits/sched.h /usr/include/bits/select.h
lock..o: /usr/include/bits/sigset.h /usr/include/bits/sigthread.h
lock..o: /usr/include/bits/sockaddr.h /usr/include/bits/socket.h
lock..o: /usr/include/bits/stdio_lim.h /usr/include/bits/time.h
lock..o: /usr/include/bits/types.h /usr/include/bits/wchar.h
lock..o: /usr/include/bits/wordsize.h /usr/include/bits/xopen_lim.h
lock..o: /usr/include/endian.h /usr/include/features.h
lock..o: /usr/include/_G_config.h /usr/include/gconv.h
lock..o: /usr/include/gnu/stubs.h /usr/include/libio.h /usr/include/limits.h
lock..o: /usr/include/linux/limits.h /usr/include/netinet/in.h
lock..o: /usr/include/pthread.h /usr/include/sched.h /usr/include/signal.h
lock..o: /usr/include/stdint.h /usr/include/stdio.h /usr/include/sys/cdefs.h
lock..o: /usr/include/sys/select.h /usr/include/sys/sysmacros.h
lock..o: /usr/include/sys/time.h /usr/include/sys/types.h /usr/include/time.h
lock..o: /usr/include/wchar.h
lock..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
lock..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
lock..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
lock..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
main..o: ../include/clientdata.h ../include/counter.h ../include/device.h
main..o: ../include/devicetable.h ../include/lock.h ../include/messages.h
main..o: ../include/nodevice.h ../include/playercommon.h
main..o: ../include/pubsub_util.h /usr/include/alloca.h
main..o: /usr/include/asm/errno.h /usr/include/asm/sigcontext.h
main..o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
main..o: /usr/include/assert.h /usr/include/bits/byteswap.h
main..o: /usr/include/bits/confname.h /usr/include/bits/endian.h
main..o: /usr/include/bits/environments.h /usr/include/bits/errno.h
main..o: /usr/include/bits/fcntl.h /usr/include/bits/in.h
main..o: /usr/include/bits/initspin.h /usr/include/bits/local_lim.h
main..o: /usr/include/bits/mman.h /usr/include/bits/posix1_lim.h
main..o: /usr/include/bits/posix2_lim.h /usr/include/bits/posix_opt.h
main..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
main..o: /usr/include/bits/select.h /usr/include/bits/sigaction.h
main..o: /usr/include/bits/sigcontext.h /usr/include/bits/siginfo.h
main..o: /usr/include/bits/signum.h /usr/include/bits/sigset.h
main..o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
main..o: /usr/include/bits/sockaddr.h /usr/include/bits/socket.h
main..o: /usr/include/bits/stat.h /usr/include/bits/stdio_lim.h
main..o: /usr/include/bits/time.h /usr/include/bits/types.h
main..o: /usr/include/bits/uio.h /usr/include/bits/waitflags.h
main..o: /usr/include/bits/waitstatus.h /usr/include/bits/wchar.h
main..o: /usr/include/bits/wordsize.h /usr/include/bits/xopen_lim.h
main..o: /usr/include/endian.h /usr/include/errno.h /usr/include/fcntl.h
main..o: /usr/include/features.h /usr/include/_G_config.h /usr/include/gconv.h
main..o: /usr/include/getopt.h /usr/include/gnu/stubs.h /usr/include/libio.h
main..o: /usr/include/limits.h /usr/include/linux/errno.h
main..o: /usr/include/linux/limits.h /usr/include/netinet/in.h
main..o: /usr/include/pthread.h /usr/include/sched.h /usr/include/signal.h
main..o: /usr/include/stdint.h /usr/include/stdio.h /usr/include/stdlib.h
main..o: /usr/include/string.h /usr/include/sys/cdefs.h
main..o: /usr/include/sys/mman.h /usr/include/sys/select.h
main..o: /usr/include/sys/socket.h /usr/include/sys/stat.h
main..o: /usr/include/sys/sysmacros.h /usr/include/sys/types.h
main..o: /usr/include/sys/ucontext.h /usr/include/sys/uio.h
main..o: /usr/include/time.h /usr/include/ucontext.h /usr/include/unistd.h
main..o: /usr/include/wchar.h /usr/include/xlocale.h
main..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
main..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
main..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
main..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
main..o: ../VERSION
miscdevice..o: ../include/device.h ../include/lock.h ../include/messages.h
miscdevice..o: ../include/miscdevice.h ../include/p2osdevice.h
miscdevice..o: ../include/packet.h ../include/playercommon.h ../include/sip.h
miscdevice..o: /usr/include/assert.h /usr/include/bits/initspin.h
miscdevice..o: /usr/include/bits/local_lim.h /usr/include/bits/posix1_lim.h
miscdevice..o: /usr/include/bits/posix2_lim.h /usr/include/bits/pthreadtypes.h
miscdevice..o: /usr/include/bits/sched.h /usr/include/bits/select.h
miscdevice..o: /usr/include/bits/sigset.h /usr/include/bits/sigthread.h
miscdevice..o: /usr/include/bits/stdio_lim.h /usr/include/bits/time.h
miscdevice..o: /usr/include/bits/types.h /usr/include/bits/wchar.h
miscdevice..o: /usr/include/bits/wordsize.h /usr/include/bits/xopen_lim.h
miscdevice..o: /usr/include/features.h /usr/include/_G_config.h
miscdevice..o: /usr/include/gconv.h /usr/include/gnu/stubs.h
miscdevice..o: /usr/include/libio.h /usr/include/limits.h
miscdevice..o: /usr/include/linux/limits.h /usr/include/pthread.h
miscdevice..o: /usr/include/sched.h /usr/include/signal.h
miscdevice..o: /usr/include/stdint.h /usr/include/stdio.h
miscdevice..o: /usr/include/string.h /usr/include/sys/cdefs.h
miscdevice..o: /usr/include/sys/select.h /usr/include/sys/time.h
miscdevice..o: /usr/include/time.h /usr/include/values.h /usr/include/wchar.h
miscdevice..o: /usr/include/xlocale.h
miscdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/float.h
miscdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
miscdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
miscdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
miscdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
nodevice..o: ../include/device.h ../include/lock.h ../include/nodevice.h
nodevice..o: ../include/playercommon.h /usr/include/assert.h
nodevice..o: /usr/include/bits/endian.h /usr/include/bits/initspin.h
nodevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
nodevice..o: /usr/include/bits/select.h /usr/include/bits/sigset.h
nodevice..o: /usr/include/bits/sigthread.h /usr/include/bits/stdio_lim.h
nodevice..o: /usr/include/bits/time.h /usr/include/bits/types.h
nodevice..o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
nodevice..o: /usr/include/endian.h /usr/include/features.h
nodevice..o: /usr/include/_G_config.h /usr/include/gconv.h
nodevice..o: /usr/include/gnu/stubs.h /usr/include/libio.h
nodevice..o: /usr/include/pthread.h /usr/include/sched.h /usr/include/signal.h
nodevice..o: /usr/include/stdint.h /usr/include/stdio.h
nodevice..o: /usr/include/sys/cdefs.h /usr/include/sys/select.h
nodevice..o: /usr/include/sys/sysmacros.h /usr/include/sys/types.h
nodevice..o: /usr/include/time.h /usr/include/wchar.h
nodevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
nodevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
p2osdevice..o: ../include/device.h ../include/lock.h ../include/messages.h
p2osdevice..o: ../include/p2osdevice.h ../include/packet.h
p2osdevice..o: ../include/playercommon.h ../include/sip.h
p2osdevice..o: /usr/include/alloca.h /usr/include/asm/sigcontext.h
p2osdevice..o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
p2osdevice..o: /usr/include/assert.h /usr/include/bits/byteswap.h
p2osdevice..o: /usr/include/bits/confname.h /usr/include/bits/endian.h
p2osdevice..o: /usr/include/bits/environments.h /usr/include/bits/fcntl.h
p2osdevice..o: /usr/include/bits/huge_val.h /usr/include/bits/in.h
p2osdevice..o: /usr/include/bits/initspin.h /usr/include/bits/local_lim.h
p2osdevice..o: /usr/include/bits/mathcalls.h /usr/include/bits/mathdef.h
p2osdevice..o: /usr/include/bits/nan.h /usr/include/bits/posix1_lim.h
p2osdevice..o: /usr/include/bits/posix2_lim.h /usr/include/bits/posix_opt.h
p2osdevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
p2osdevice..o: /usr/include/bits/select.h /usr/include/bits/sigaction.h
p2osdevice..o: /usr/include/bits/sigcontext.h /usr/include/bits/siginfo.h
p2osdevice..o: /usr/include/bits/signum.h /usr/include/bits/sigset.h
p2osdevice..o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
p2osdevice..o: /usr/include/bits/sockaddr.h /usr/include/bits/socket.h
p2osdevice..o: /usr/include/bits/stat.h /usr/include/bits/stdio_lim.h
p2osdevice..o: /usr/include/bits/termios.h /usr/include/bits/time.h
p2osdevice..o: /usr/include/bits/types.h /usr/include/bits/waitflags.h
p2osdevice..o: /usr/include/bits/waitstatus.h /usr/include/bits/wchar.h
p2osdevice..o: /usr/include/bits/wordsize.h /usr/include/bits/xopen_lim.h
p2osdevice..o: /usr/include/endian.h /usr/include/fcntl.h
p2osdevice..o: /usr/include/features.h /usr/include/_G_config.h
p2osdevice..o: /usr/include/gconv.h /usr/include/getopt.h
p2osdevice..o: /usr/include/gnu/stubs.h /usr/include/libio.h
p2osdevice..o: /usr/include/limits.h /usr/include/linux/limits.h
p2osdevice..o: /usr/include/math.h /usr/include/netinet/in.h
p2osdevice..o: /usr/include/pthread.h /usr/include/sched.h
p2osdevice..o: /usr/include/signal.h /usr/include/stdint.h
p2osdevice..o: /usr/include/stdio.h /usr/include/stdlib.h
p2osdevice..o: /usr/include/string.h /usr/include/sys/cdefs.h
p2osdevice..o: /usr/include/sys/select.h /usr/include/sys/stat.h
p2osdevice..o: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
p2osdevice..o: /usr/include/sys/ttydefaults.h /usr/include/sys/types.h
p2osdevice..o: /usr/include/sys/ucontext.h /usr/include/termios.h
p2osdevice..o: /usr/include/time.h /usr/include/ucontext.h
p2osdevice..o: /usr/include/unistd.h /usr/include/values.h
p2osdevice..o: /usr/include/wchar.h /usr/include/xlocale.h
p2osdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/float.h
p2osdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
p2osdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
p2osdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
p2osdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
packet..o: ../include/packet.h /usr/include/alloca.h /usr/include/asm/errno.h
packet..o: /usr/include/bits/confname.h /usr/include/bits/endian.h
packet..o: /usr/include/bits/environments.h /usr/include/bits/errno.h
packet..o: /usr/include/bits/posix_opt.h /usr/include/bits/pthreadtypes.h
packet..o: /usr/include/bits/sched.h /usr/include/bits/select.h
packet..o: /usr/include/bits/sigset.h /usr/include/bits/stdio_lim.h
packet..o: /usr/include/bits/time.h /usr/include/bits/types.h
packet..o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
packet..o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
packet..o: /usr/include/endian.h /usr/include/errno.h /usr/include/features.h
packet..o: /usr/include/_G_config.h /usr/include/gconv.h /usr/include/getopt.h
packet..o: /usr/include/gnu/stubs.h /usr/include/libio.h
packet..o: /usr/include/linux/errno.h /usr/include/stdio.h
packet..o: /usr/include/stdlib.h /usr/include/string.h
packet..o: /usr/include/sys/cdefs.h /usr/include/sys/select.h
packet..o: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
packet..o: /usr/include/sys/types.h /usr/include/time.h /usr/include/unistd.h
packet..o: /usr/include/wchar.h /usr/include/xlocale.h
packet..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
packet..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
positiondevice..o: ../include/device.h ../include/lock.h ../include/messages.h
positiondevice..o: ../include/p2osdevice.h ../include/packet.h
positiondevice..o: ../include/playercommon.h ../include/positiondevice.h
positiondevice..o: ../include/sip.h /usr/include/asm/socket.h
positiondevice..o: /usr/include/asm/sockios.h /usr/include/assert.h
positiondevice..o: /usr/include/bits/byteswap.h /usr/include/bits/endian.h
positiondevice..o: /usr/include/bits/in.h /usr/include/bits/initspin.h
positiondevice..o: /usr/include/bits/local_lim.h
positiondevice..o: /usr/include/bits/posix1_lim.h
positiondevice..o: /usr/include/bits/posix2_lim.h
positiondevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
positiondevice..o: /usr/include/bits/select.h /usr/include/bits/sigset.h
positiondevice..o: /usr/include/bits/sigthread.h /usr/include/bits/sockaddr.h
positiondevice..o: /usr/include/bits/socket.h /usr/include/bits/stdio_lim.h
positiondevice..o: /usr/include/bits/time.h /usr/include/bits/types.h
positiondevice..o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
positiondevice..o: /usr/include/bits/xopen_lim.h /usr/include/endian.h
positiondevice..o: /usr/include/features.h /usr/include/_G_config.h
positiondevice..o: /usr/include/gconv.h /usr/include/gnu/stubs.h
positiondevice..o: /usr/include/libio.h /usr/include/limits.h
positiondevice..o: /usr/include/linux/limits.h /usr/include/netinet/in.h
positiondevice..o: /usr/include/pthread.h /usr/include/sched.h
positiondevice..o: /usr/include/signal.h /usr/include/stdint.h
positiondevice..o: /usr/include/stdio.h /usr/include/string.h
positiondevice..o: /usr/include/sys/cdefs.h /usr/include/sys/select.h
positiondevice..o: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
positiondevice..o: /usr/include/sys/types.h /usr/include/time.h
positiondevice..o: /usr/include/values.h /usr/include/wchar.h
positiondevice..o: /usr/include/xlocale.h
positiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/float.h
positiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
positiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
positiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
positiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
ptzdevice..o: ../include/device.h ../include/lock.h ../include/messages.h
ptzdevice..o: ../include/playercommon.h ../include/ptzdevice.h
ptzdevice..o: /usr/include/alloca.h /usr/include/asm/sigcontext.h
ptzdevice..o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
ptzdevice..o: /usr/include/assert.h /usr/include/bits/byteswap.h
ptzdevice..o: /usr/include/bits/confname.h /usr/include/bits/endian.h
ptzdevice..o: /usr/include/bits/environments.h /usr/include/bits/fcntl.h
ptzdevice..o: /usr/include/bits/in.h /usr/include/bits/initspin.h
ptzdevice..o: /usr/include/bits/local_lim.h /usr/include/bits/posix1_lim.h
ptzdevice..o: /usr/include/bits/posix2_lim.h /usr/include/bits/posix_opt.h
ptzdevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
ptzdevice..o: /usr/include/bits/select.h /usr/include/bits/sigaction.h
ptzdevice..o: /usr/include/bits/sigcontext.h /usr/include/bits/siginfo.h
ptzdevice..o: /usr/include/bits/signum.h /usr/include/bits/sigset.h
ptzdevice..o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
ptzdevice..o: /usr/include/bits/sockaddr.h /usr/include/bits/socket.h
ptzdevice..o: /usr/include/bits/stat.h /usr/include/bits/stdio_lim.h
ptzdevice..o: /usr/include/bits/termios.h /usr/include/bits/time.h
ptzdevice..o: /usr/include/bits/types.h /usr/include/bits/waitflags.h
ptzdevice..o: /usr/include/bits/waitstatus.h /usr/include/bits/wchar.h
ptzdevice..o: /usr/include/bits/wordsize.h /usr/include/bits/xopen_lim.h
ptzdevice..o: /usr/include/endian.h /usr/include/fcntl.h
ptzdevice..o: /usr/include/features.h /usr/include/_G_config.h
ptzdevice..o: /usr/include/gconv.h /usr/include/getopt.h
ptzdevice..o: /usr/include/gnu/stubs.h /usr/include/libio.h
ptzdevice..o: /usr/include/limits.h /usr/include/linux/limits.h
ptzdevice..o: /usr/include/netinet/in.h /usr/include/pthread.h
ptzdevice..o: /usr/include/sched.h /usr/include/signal.h /usr/include/stdint.h
ptzdevice..o: /usr/include/stdio.h /usr/include/stdlib.h /usr/include/string.h
ptzdevice..o: /usr/include/sys/cdefs.h /usr/include/sys/select.h
ptzdevice..o: /usr/include/sys/stat.h /usr/include/sys/sysmacros.h
ptzdevice..o: /usr/include/sys/ttydefaults.h /usr/include/sys/types.h
ptzdevice..o: /usr/include/sys/ucontext.h /usr/include/termios.h
ptzdevice..o: /usr/include/time.h /usr/include/ucontext.h
ptzdevice..o: /usr/include/unistd.h /usr/include/wchar.h
ptzdevice..o: /usr/include/xlocale.h
ptzdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
ptzdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
ptzdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
ptzdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
pubsub_util..o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
pubsub_util..o: /usr/include/bits/byteswap.h /usr/include/bits/confname.h
pubsub_util..o: /usr/include/bits/endian.h /usr/include/bits/environments.h
pubsub_util..o: /usr/include/bits/fcntl.h /usr/include/bits/in.h
pubsub_util..o: /usr/include/bits/local_lim.h /usr/include/bits/netdb.h
pubsub_util..o: /usr/include/bits/posix1_lim.h /usr/include/bits/posix2_lim.h
pubsub_util..o: /usr/include/bits/posix_opt.h /usr/include/bits/pthreadtypes.h
pubsub_util..o: /usr/include/bits/sched.h /usr/include/bits/select.h
pubsub_util..o: /usr/include/bits/siginfo.h /usr/include/bits/sigset.h
pubsub_util..o: /usr/include/bits/sockaddr.h /usr/include/bits/socket.h
pubsub_util..o: /usr/include/bits/stat.h /usr/include/bits/stdio_lim.h
pubsub_util..o: /usr/include/bits/time.h /usr/include/bits/types.h
pubsub_util..o: /usr/include/bits/uio.h /usr/include/bits/wchar.h
pubsub_util..o: /usr/include/bits/wordsize.h /usr/include/bits/xopen_lim.h
pubsub_util..o: /usr/include/endian.h /usr/include/fcntl.h
pubsub_util..o: /usr/include/features.h /usr/include/_G_config.h
pubsub_util..o: /usr/include/gconv.h /usr/include/getopt.h
pubsub_util..o: /usr/include/gnu/stubs.h /usr/include/libio.h
pubsub_util..o: /usr/include/limits.h /usr/include/linux/limits.h
pubsub_util..o: /usr/include/netdb.h /usr/include/netinet/in.h
pubsub_util..o: /usr/include/rpc/netdb.h /usr/include/stdint.h
pubsub_util..o: /usr/include/stdio.h /usr/include/string.h
pubsub_util..o: /usr/include/sys/cdefs.h /usr/include/sys/select.h
pubsub_util..o: /usr/include/sys/socket.h /usr/include/sys/stat.h
pubsub_util..o: /usr/include/sys/sysmacros.h /usr/include/sys/types.h
pubsub_util..o: /usr/include/sys/uio.h /usr/include/time.h
pubsub_util..o: /usr/include/unistd.h /usr/include/wchar.h
pubsub_util..o: /usr/include/xlocale.h
pubsub_util..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
pubsub_util..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
pubsub_util..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
pubsub_util..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
sip..o: ../include/device.h ../include/lock.h ../include/messages.h
sip..o: ../include/p2osdevice.h ../include/packet.h ../include/playercommon.h
sip..o: ../include/sip.h /usr/include/alloca.h /usr/include/asm/socket.h
sip..o: /usr/include/asm/sockios.h /usr/include/assert.h
sip..o: /usr/include/bits/byteswap.h /usr/include/bits/endian.h
sip..o: /usr/include/bits/huge_val.h /usr/include/bits/in.h
sip..o: /usr/include/bits/initspin.h /usr/include/bits/local_lim.h
sip..o: /usr/include/bits/mathcalls.h /usr/include/bits/mathdef.h
sip..o: /usr/include/bits/nan.h /usr/include/bits/posix1_lim.h
sip..o: /usr/include/bits/posix2_lim.h /usr/include/bits/pthreadtypes.h
sip..o: /usr/include/bits/sched.h /usr/include/bits/select.h
sip..o: /usr/include/bits/sigset.h /usr/include/bits/sigthread.h
sip..o: /usr/include/bits/sockaddr.h /usr/include/bits/socket.h
sip..o: /usr/include/bits/stdio_lim.h /usr/include/bits/time.h
sip..o: /usr/include/bits/types.h /usr/include/bits/waitflags.h
sip..o: /usr/include/bits/waitstatus.h /usr/include/bits/wchar.h
sip..o: /usr/include/bits/wordsize.h /usr/include/bits/xopen_lim.h
sip..o: /usr/include/endian.h /usr/include/features.h /usr/include/_G_config.h
sip..o: /usr/include/gconv.h /usr/include/gnu/stubs.h /usr/include/libio.h
sip..o: /usr/include/limits.h /usr/include/linux/limits.h /usr/include/math.h
sip..o: /usr/include/netinet/in.h /usr/include/pthread.h /usr/include/sched.h
sip..o: /usr/include/signal.h /usr/include/stdint.h /usr/include/stdio.h
sip..o: /usr/include/stdlib.h /usr/include/string.h /usr/include/sys/cdefs.h
sip..o: /usr/include/sys/select.h /usr/include/sys/sysmacros.h
sip..o: /usr/include/sys/time.h /usr/include/sys/types.h /usr/include/time.h
sip..o: /usr/include/values.h /usr/include/wchar.h /usr/include/xlocale.h
sip..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/float.h
sip..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
sip..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
sip..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
sip..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
sonardevice..o: ../include/device.h ../include/lock.h ../include/messages.h
sonardevice..o: ../include/p2osdevice.h ../include/packet.h
sonardevice..o: ../include/playercommon.h ../include/sip.h
sonardevice..o: ../include/sonardevice.h /usr/include/asm/socket.h
sonardevice..o: /usr/include/asm/sockios.h /usr/include/assert.h
sonardevice..o: /usr/include/bits/byteswap.h /usr/include/bits/endian.h
sonardevice..o: /usr/include/bits/in.h /usr/include/bits/initspin.h
sonardevice..o: /usr/include/bits/local_lim.h /usr/include/bits/posix1_lim.h
sonardevice..o: /usr/include/bits/posix2_lim.h
sonardevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
sonardevice..o: /usr/include/bits/select.h /usr/include/bits/sigset.h
sonardevice..o: /usr/include/bits/sigthread.h /usr/include/bits/sockaddr.h
sonardevice..o: /usr/include/bits/socket.h /usr/include/bits/stdio_lim.h
sonardevice..o: /usr/include/bits/time.h /usr/include/bits/types.h
sonardevice..o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
sonardevice..o: /usr/include/bits/xopen_lim.h /usr/include/endian.h
sonardevice..o: /usr/include/features.h /usr/include/_G_config.h
sonardevice..o: /usr/include/gconv.h /usr/include/gnu/stubs.h
sonardevice..o: /usr/include/libio.h /usr/include/limits.h
sonardevice..o: /usr/include/linux/limits.h /usr/include/netinet/in.h
sonardevice..o: /usr/include/pthread.h /usr/include/sched.h
sonardevice..o: /usr/include/signal.h /usr/include/stdint.h
sonardevice..o: /usr/include/stdio.h /usr/include/string.h
sonardevice..o: /usr/include/sys/cdefs.h /usr/include/sys/select.h
sonardevice..o: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
sonardevice..o: /usr/include/sys/types.h /usr/include/time.h
sonardevice..o: /usr/include/values.h /usr/include/wchar.h
sonardevice..o: /usr/include/xlocale.h
sonardevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/float.h
sonardevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
sonardevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
sonardevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
sonardevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
speechdevice..o: ../include/device.h ../include/lock.h ../include/messages.h
speechdevice..o: ../include/playercommon.h ../include/pubsub_util.h
speechdevice..o: ../include/speechdevice.h /usr/include/alloca.h
speechdevice..o: /usr/include/asm/errno.h /usr/include/asm/sigcontext.h
speechdevice..o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
speechdevice..o: /usr/include/assert.h /usr/include/bits/byteswap.h
speechdevice..o: /usr/include/bits/confname.h /usr/include/bits/endian.h
speechdevice..o: /usr/include/bits/environments.h /usr/include/bits/errno.h
speechdevice..o: /usr/include/bits/fcntl.h /usr/include/bits/in.h
speechdevice..o: /usr/include/bits/initspin.h /usr/include/bits/local_lim.h
speechdevice..o: /usr/include/bits/netdb.h /usr/include/bits/posix1_lim.h
speechdevice..o: /usr/include/bits/posix2_lim.h /usr/include/bits/posix_opt.h
speechdevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
speechdevice..o: /usr/include/bits/select.h /usr/include/bits/sigaction.h
speechdevice..o: /usr/include/bits/sigcontext.h /usr/include/bits/siginfo.h
speechdevice..o: /usr/include/bits/signum.h /usr/include/bits/sigset.h
speechdevice..o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
speechdevice..o: /usr/include/bits/sockaddr.h /usr/include/bits/socket.h
speechdevice..o: /usr/include/bits/stat.h /usr/include/bits/stdio_lim.h
speechdevice..o: /usr/include/bits/time.h /usr/include/bits/types.h
speechdevice..o: /usr/include/bits/uio.h /usr/include/bits/waitflags.h
speechdevice..o: /usr/include/bits/waitstatus.h /usr/include/bits/wchar.h
speechdevice..o: /usr/include/bits/wordsize.h /usr/include/bits/xopen_lim.h
speechdevice..o: /usr/include/endian.h /usr/include/errno.h
speechdevice..o: /usr/include/fcntl.h /usr/include/features.h
speechdevice..o: /usr/include/_G_config.h /usr/include/gconv.h
speechdevice..o: /usr/include/getopt.h /usr/include/gnu/stubs.h
speechdevice..o: /usr/include/libio.h /usr/include/limits.h
speechdevice..o: /usr/include/linux/errno.h /usr/include/linux/limits.h
speechdevice..o: /usr/include/netdb.h /usr/include/netinet/in.h
speechdevice..o: /usr/include/pthread.h /usr/include/rpc/netdb.h
speechdevice..o: /usr/include/sched.h /usr/include/signal.h
speechdevice..o: /usr/include/stdint.h /usr/include/stdio.h
speechdevice..o: /usr/include/stdlib.h /usr/include/string.h
speechdevice..o: /usr/include/sys/cdefs.h /usr/include/sys/select.h
speechdevice..o: /usr/include/sys/socket.h /usr/include/sys/stat.h
speechdevice..o: /usr/include/sys/sysmacros.h /usr/include/sys/types.h
speechdevice..o: /usr/include/sys/ucontext.h /usr/include/sys/uio.h
speechdevice..o: /usr/include/time.h /usr/include/ucontext.h
speechdevice..o: /usr/include/unistd.h /usr/include/wchar.h
speechdevice..o: /usr/include/xlocale.h
speechdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
speechdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
speechdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
speechdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
stagedevice..o: ../include/arenalock.h ../include/device.h ../include/lock.h
stagedevice..o: ../include/messages.h ../include/playercommon.h
stagedevice..o: ../include/stagedevice.h ../include/stage.h
stagedevice..o: /usr/include/assert.h /usr/include/bits/endian.h
stagedevice..o: /usr/include/bits/initspin.h /usr/include/bits/ipc.h
stagedevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
stagedevice..o: /usr/include/bits/select.h /usr/include/bits/sem.h
stagedevice..o: /usr/include/bits/sigset.h /usr/include/bits/sigthread.h
stagedevice..o: /usr/include/bits/stdio_lim.h /usr/include/bits/time.h
stagedevice..o: /usr/include/bits/types.h /usr/include/bits/wchar.h
stagedevice..o: /usr/include/bits/wordsize.h /usr/include/endian.h
stagedevice..o: /usr/include/features.h /usr/include/_G_config.h
stagedevice..o: /usr/include/gconv.h /usr/include/gnu/stubs.h
stagedevice..o: /usr/include/libio.h /usr/include/pthread.h
stagedevice..o: /usr/include/sched.h /usr/include/signal.h
stagedevice..o: /usr/include/stdint.h /usr/include/stdio.h
stagedevice..o: /usr/include/string.h /usr/include/sys/cdefs.h
stagedevice..o: /usr/include/sys/ipc.h /usr/include/sys/select.h
stagedevice..o: /usr/include/sys/sem.h /usr/include/sys/sysmacros.h
stagedevice..o: /usr/include/sys/types.h /usr/include/time.h
stagedevice..o: /usr/include/wchar.h /usr/include/xlocale.h
stagedevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
stagedevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
visiondevice..o: ../include/device.h ../include/lock.h ../include/messages.h
visiondevice..o: ../include/playercommon.h ../include/pubsub_util.h
visiondevice..o: ../include/visiondevice.h /usr/include/alloca.h
visiondevice..o: /usr/include/asm/sigcontext.h /usr/include/asm/socket.h
visiondevice..o: /usr/include/asm/sockios.h /usr/include/assert.h
visiondevice..o: /usr/include/bits/byteswap.h /usr/include/bits/confname.h
visiondevice..o: /usr/include/bits/endian.h /usr/include/bits/environments.h
visiondevice..o: /usr/include/bits/fcntl.h /usr/include/bits/in.h
visiondevice..o: /usr/include/bits/initspin.h /usr/include/bits/local_lim.h
visiondevice..o: /usr/include/bits/netdb.h /usr/include/bits/posix1_lim.h
visiondevice..o: /usr/include/bits/posix2_lim.h /usr/include/bits/posix_opt.h
visiondevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
visiondevice..o: /usr/include/bits/select.h /usr/include/bits/sigaction.h
visiondevice..o: /usr/include/bits/sigcontext.h /usr/include/bits/siginfo.h
visiondevice..o: /usr/include/bits/signum.h /usr/include/bits/sigset.h
visiondevice..o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
visiondevice..o: /usr/include/bits/sockaddr.h /usr/include/bits/socket.h
visiondevice..o: /usr/include/bits/stat.h /usr/include/bits/stdio_lim.h
visiondevice..o: /usr/include/bits/time.h /usr/include/bits/types.h
visiondevice..o: /usr/include/bits/uio.h /usr/include/bits/waitflags.h
visiondevice..o: /usr/include/bits/waitstatus.h /usr/include/bits/wchar.h
visiondevice..o: /usr/include/bits/wordsize.h /usr/include/bits/xopen_lim.h
visiondevice..o: /usr/include/endian.h /usr/include/fcntl.h
visiondevice..o: /usr/include/features.h /usr/include/_G_config.h
visiondevice..o: /usr/include/gconv.h /usr/include/getopt.h
visiondevice..o: /usr/include/gnu/stubs.h /usr/include/libio.h
visiondevice..o: /usr/include/limits.h /usr/include/linux/limits.h
visiondevice..o: /usr/include/netdb.h /usr/include/netinet/in.h
visiondevice..o: /usr/include/pthread.h /usr/include/rpc/netdb.h
visiondevice..o: /usr/include/sched.h /usr/include/signal.h
visiondevice..o: /usr/include/stdint.h /usr/include/stdio.h
visiondevice..o: /usr/include/stdlib.h /usr/include/string.h
visiondevice..o: /usr/include/sys/cdefs.h /usr/include/sys/select.h
visiondevice..o: /usr/include/sys/socket.h /usr/include/sys/stat.h
visiondevice..o: /usr/include/sys/sysmacros.h /usr/include/sys/types.h
visiondevice..o: /usr/include/sys/ucontext.h /usr/include/sys/uio.h
visiondevice..o: /usr/include/time.h /usr/include/ucontext.h
visiondevice..o: /usr/include/unistd.h /usr/include/wchar.h
visiondevice..o: /usr/include/xlocale.h
visiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
visiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
visiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
visiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
