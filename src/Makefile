#
# $Id$
#
 
.SUFFIXES: .cc .o

SERVERNAME = player
DEVICELIBNAME = libplayerdevice.a

INCLUDES = -I../include
CPP = g++
CFLAGS = -g -Wall
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
INCLUDE_AUDIO = -DINCLUDE_AUDIO
# the following libs are only needed for the audio device
EXTRA_LIBS = -lrfftw -lfftw

INCLUDED_DEVICES = $(INCLUDE_LASER) $(INCLUDE_SONAR) $(INCLUDE_VISION) \
                   $(INCLUDE_POSITION) $(INCLUDE_GRIPPER) $(INCLUDE_MISC) \
		   $(INCLUDE_MISC) $(INCLUDE_PTZ) $(INCLUDE_STAGE) \
                   $(INCLUDE_AUDIO)

INSTALL_BIN = $(INSTALL_PREFIX)/bin
INSTALL_INCLUDE = $(INSTALL_PREFIX)/include

OBS = nodevice.o clientdata.o laserdevice.o p2osdevice.o lock.o sonardevice.o positiondevice.o pubsub_util.o visiondevice.o gripperdevice.o miscdevice.o packet.o sip.o arenalock.o devicetable.o stagedevice.o ptzdevice.o audiodevice.o

all: $(SERVERNAME)

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND" *.cc 2>/dev/null
	$(RM) -f Makefile.bak

clean:
	rm -f *.o *~ $(SERVERNAME) $(DEVICELIBNAME) core

$(DEVICELIBNAME): $(OBS)
	$(AR) cr $@ $(OBS)

$(SERVERNAME): main.cc $(DEVICELIBNAME) 
	$(CPP) $(CFLAGS) $(INCLUDES) $(INCLUDED_DEVICES) -o $@ $< $(LIBS) $(EXTRA_LIBS)

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
arenalock..o: ../include/lock.h /usr/include/_G_config.h /usr/include/alloca.h
arenalock..o: /usr/include/bits/ipc.h /usr/include/bits/pthreadtypes.h
arenalock..o: /usr/include/bits/sched.h /usr/include/bits/sem.h
arenalock..o: /usr/include/bits/sigset.h /usr/include/bits/sigthread.h
arenalock..o: /usr/include/bits/types.h /usr/include/bits/waitstatus.h
arenalock..o: /usr/include/endian.h /usr/include/features.h
arenalock..o: /usr/include/gconv.h /usr/include/libio.h /usr/include/pthread.h
arenalock..o: /usr/include/sched.h /usr/include/signal.h /usr/include/stdio.h
arenalock..o: /usr/include/stdlib.h /usr/include/sys/ipc.h
arenalock..o: /usr/include/sys/select.h /usr/include/sys/sem.h
arenalock..o: /usr/include/sys/types.h /usr/include/time.h
arenalock..o: /usr/include/wchar.h /usr/include/xlocale.h
arenalock..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
arenalock..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
audiodevice..o: ../include/audiodevice.h ../include/device.h ../include/lock.h
audiodevice..o: /usr/include/_G_config.h /usr/include/alloca.h
audiodevice..o: /usr/include/asm/ioctls.h /usr/include/asm/sigcontext.h
audiodevice..o: /usr/include/asm/socket.h /usr/include/bits/confname.h
audiodevice..o: /usr/include/bits/environments.h /usr/include/bits/fcntl.h
audiodevice..o: /usr/include/bits/huge_val.h /usr/include/bits/in.h
audiodevice..o: /usr/include/bits/ioctl-types.h /usr/include/bits/ioctls.h
audiodevice..o: /usr/include/bits/local_lim.h /usr/include/bits/mathcalls.h
audiodevice..o: /usr/include/bits/mathdef.h /usr/include/bits/nan.h
audiodevice..o: /usr/include/bits/posix1_lim.h
audiodevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
audiodevice..o: /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h
audiodevice..o: /usr/include/bits/siginfo.h /usr/include/bits/sigset.h
audiodevice..o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
audiodevice..o: /usr/include/bits/sockaddr.h /usr/include/bits/socket.h
audiodevice..o: /usr/include/bits/stat.h /usr/include/bits/termios.h
audiodevice..o: /usr/include/bits/time.h /usr/include/bits/types.h
audiodevice..o: /usr/include/bits/waitstatus.h /usr/include/bits/xopen_lim.h
audiodevice..o: /usr/include/endian.h /usr/include/fcntl.h
audiodevice..o: /usr/include/features.h /usr/include/gconv.h
audiodevice..o: /usr/include/getopt.h /usr/include/libio.h
audiodevice..o: /usr/include/limits.h /usr/include/linux/soundcard.h
audiodevice..o: /usr/include/math.h /usr/include/netinet/in.h
audiodevice..o: /usr/include/pthread.h /usr/include/sched.h
audiodevice..o: /usr/include/signal.h /usr/include/stdint.h
audiodevice..o: /usr/include/stdio.h /usr/include/stdlib.h
audiodevice..o: /usr/include/string.h /usr/include/sys/ioctl.h
audiodevice..o: /usr/include/sys/select.h /usr/include/sys/stat.h
audiodevice..o: /usr/include/sys/time.h /usr/include/sys/types.h
audiodevice..o: /usr/include/sys/ucontext.h /usr/include/termios.h
audiodevice..o: /usr/include/time.h /usr/include/ucontext.h
audiodevice..o: /usr/include/unistd.h /usr/include/wchar.h
audiodevice..o: /usr/include/xlocale.h
audiodevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/float.h
audiodevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
audiodevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
audiodevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
audiodevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
clientdata..o: ../include/clientdata.h ../include/counter.h
clientdata..o: ../include/device.h ../include/devicetable.h ../include/lock.h
clientdata..o: ../include/messages.h ../include/packet.h
clientdata..o: ../include/playercommon.h /usr/include/_G_config.h
clientdata..o: /usr/include/asm/socket.h /usr/include/assert.h
clientdata..o: /usr/include/bits/confname.h /usr/include/bits/environments.h
clientdata..o: /usr/include/bits/in.h /usr/include/bits/local_lim.h
clientdata..o: /usr/include/bits/posix1_lim.h /usr/include/bits/pthreadtypes.h
clientdata..o: /usr/include/bits/sched.h /usr/include/bits/sigset.h
clientdata..o: /usr/include/bits/sigthread.h /usr/include/bits/sockaddr.h
clientdata..o: /usr/include/bits/socket.h /usr/include/bits/time.h
clientdata..o: /usr/include/bits/types.h /usr/include/bits/xopen_lim.h
clientdata..o: /usr/include/endian.h /usr/include/features.h
clientdata..o: /usr/include/gconv.h /usr/include/getopt.h /usr/include/libio.h
clientdata..o: /usr/include/limits.h /usr/include/netinet/in.h
clientdata..o: /usr/include/pthread.h /usr/include/sched.h
clientdata..o: /usr/include/signal.h /usr/include/stdint.h
clientdata..o: /usr/include/stdio.h /usr/include/string.h
clientdata..o: /usr/include/sys/select.h /usr/include/sys/time.h
clientdata..o: /usr/include/sys/types.h /usr/include/time.h
clientdata..o: /usr/include/unistd.h /usr/include/wchar.h
clientdata..o: /usr/include/xlocale.h
clientdata..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
clientdata..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
clientdata..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
clientdata..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
devicetable..o: ../include/device.h ../include/devicetable.h ../include/lock.h
devicetable..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
devicetable..o: /usr/include/bits/sigset.h /usr/include/bits/sigthread.h
devicetable..o: /usr/include/bits/types.h /usr/include/features.h
devicetable..o: /usr/include/pthread.h /usr/include/sched.h
devicetable..o: /usr/include/signal.h /usr/include/time.h
devicetable..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
gripperdevice..o: ../include/device.h ../include/gripperdevice.h
gripperdevice..o: ../include/lock.h ../include/messages.h
gripperdevice..o: ../include/p2osdevice.h ../include/packet.h
gripperdevice..o: ../include/playercommon.h ../include/sip.h
gripperdevice..o: /usr/include/_G_config.h /usr/include/assert.h
gripperdevice..o: /usr/include/bits/local_lim.h /usr/include/bits/posix1_lim.h
gripperdevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
gripperdevice..o: /usr/include/bits/sigset.h /usr/include/bits/sigthread.h
gripperdevice..o: /usr/include/bits/time.h /usr/include/bits/types.h
gripperdevice..o: /usr/include/bits/xopen_lim.h /usr/include/features.h
gripperdevice..o: /usr/include/gconv.h /usr/include/libio.h
gripperdevice..o: /usr/include/limits.h /usr/include/pthread.h
gripperdevice..o: /usr/include/sched.h /usr/include/signal.h
gripperdevice..o: /usr/include/stdint.h /usr/include/stdio.h
gripperdevice..o: /usr/include/string.h /usr/include/sys/select.h
gripperdevice..o: /usr/include/sys/time.h /usr/include/time.h
gripperdevice..o: /usr/include/values.h /usr/include/wchar.h
gripperdevice..o: /usr/include/xlocale.h
gripperdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/float.h
gripperdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
gripperdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
gripperdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
gripperdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
laserdevice..o: ../include/device.h ../include/laserdevice.h ../include/lock.h
laserdevice..o: ../include/messages.h ../include/playercommon.h
laserdevice..o: /usr/include/_G_config.h /usr/include/asm/sigcontext.h
laserdevice..o: /usr/include/asm/socket.h /usr/include/assert.h
laserdevice..o: /usr/include/bits/confname.h /usr/include/bits/environments.h
laserdevice..o: /usr/include/bits/fcntl.h /usr/include/bits/in.h
laserdevice..o: /usr/include/bits/local_lim.h /usr/include/bits/posix1_lim.h
laserdevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
laserdevice..o: /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h
laserdevice..o: /usr/include/bits/siginfo.h /usr/include/bits/sigset.h
laserdevice..o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
laserdevice..o: /usr/include/bits/sockaddr.h /usr/include/bits/socket.h
laserdevice..o: /usr/include/bits/stat.h /usr/include/bits/termios.h
laserdevice..o: /usr/include/bits/time.h /usr/include/bits/types.h
laserdevice..o: /usr/include/bits/xopen_lim.h /usr/include/endian.h
laserdevice..o: /usr/include/fcntl.h /usr/include/features.h
laserdevice..o: /usr/include/gconv.h /usr/include/getopt.h
laserdevice..o: /usr/include/libio.h /usr/include/limits.h
laserdevice..o: /usr/include/netinet/in.h /usr/include/pthread.h
laserdevice..o: /usr/include/sched.h /usr/include/signal.h
laserdevice..o: /usr/include/stdint.h /usr/include/stdio.h
laserdevice..o: /usr/include/string.h /usr/include/sys/select.h
laserdevice..o: /usr/include/sys/stat.h /usr/include/sys/time.h
laserdevice..o: /usr/include/sys/types.h /usr/include/sys/ucontext.h
laserdevice..o: /usr/include/termios.h /usr/include/time.h
laserdevice..o: /usr/include/ucontext.h /usr/include/unistd.h
laserdevice..o: /usr/include/wchar.h /usr/include/xlocale.h
laserdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
laserdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
laserdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
laserdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
lock..o: ../include/device.h ../include/errors.h ../include/lock.h
lock..o: /usr/include/_G_config.h /usr/include/bits/pthreadtypes.h
lock..o: /usr/include/bits/sched.h /usr/include/bits/sigset.h
lock..o: /usr/include/bits/sigthread.h /usr/include/bits/types.h
lock..o: /usr/include/features.h /usr/include/gconv.h /usr/include/libio.h
lock..o: /usr/include/pthread.h /usr/include/sched.h /usr/include/signal.h
lock..o: /usr/include/stdio.h /usr/include/time.h /usr/include/wchar.h
lock..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
lock..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
main..o: ../include/clientdata.h ../include/counter.h ../include/device.h
main..o: ../include/devicetable.h ../include/lock.h ../include/messages.h
main..o: ../include/nodevice.h ../include/playercommon.h
main..o: ../include/pubsub_util.h /usr/include/_G_config.h
main..o: /usr/include/alloca.h /usr/include/asm/sigcontext.h
main..o: /usr/include/asm/socket.h /usr/include/assert.h
main..o: /usr/include/bits/confname.h /usr/include/bits/environments.h
main..o: /usr/include/bits/errno.h /usr/include/bits/fcntl.h
main..o: /usr/include/bits/in.h /usr/include/bits/local_lim.h
main..o: /usr/include/bits/posix1_lim.h /usr/include/bits/pthreadtypes.h
main..o: /usr/include/bits/sched.h /usr/include/bits/sigaction.h
main..o: /usr/include/bits/sigcontext.h /usr/include/bits/siginfo.h
main..o: /usr/include/bits/sigset.h /usr/include/bits/sigstack.h
main..o: /usr/include/bits/sigthread.h /usr/include/bits/sockaddr.h
main..o: /usr/include/bits/socket.h /usr/include/bits/stat.h
main..o: /usr/include/bits/types.h /usr/include/bits/waitstatus.h
main..o: /usr/include/bits/xopen_lim.h /usr/include/endian.h
main..o: /usr/include/errno.h /usr/include/fcntl.h /usr/include/features.h
main..o: /usr/include/gconv.h /usr/include/getopt.h /usr/include/libio.h
main..o: /usr/include/limits.h /usr/include/linux/errno.h
main..o: /usr/include/netinet/in.h /usr/include/pthread.h /usr/include/sched.h
main..o: /usr/include/signal.h /usr/include/stdint.h /usr/include/stdio.h
main..o: /usr/include/stdlib.h /usr/include/string.h /usr/include/sys/mman.h
main..o: /usr/include/sys/select.h /usr/include/sys/socket.h
main..o: /usr/include/sys/stat.h /usr/include/sys/types.h
main..o: /usr/include/sys/ucontext.h /usr/include/time.h
main..o: /usr/include/ucontext.h /usr/include/unistd.h /usr/include/wchar.h
main..o: /usr/include/xlocale.h
main..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
main..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
main..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
main..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
miscdevice..o: ../include/device.h ../include/lock.h ../include/messages.h
miscdevice..o: ../include/miscdevice.h ../include/p2osdevice.h
miscdevice..o: ../include/packet.h ../include/playercommon.h ../include/sip.h
miscdevice..o: /usr/include/_G_config.h /usr/include/assert.h
miscdevice..o: /usr/include/bits/local_lim.h /usr/include/bits/posix1_lim.h
miscdevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
miscdevice..o: /usr/include/bits/sigset.h /usr/include/bits/sigthread.h
miscdevice..o: /usr/include/bits/time.h /usr/include/bits/types.h
miscdevice..o: /usr/include/bits/xopen_lim.h /usr/include/features.h
miscdevice..o: /usr/include/gconv.h /usr/include/libio.h /usr/include/limits.h
miscdevice..o: /usr/include/pthread.h /usr/include/sched.h
miscdevice..o: /usr/include/signal.h /usr/include/stdint.h
miscdevice..o: /usr/include/stdio.h /usr/include/string.h
miscdevice..o: /usr/include/sys/select.h /usr/include/sys/time.h
miscdevice..o: /usr/include/time.h /usr/include/values.h /usr/include/wchar.h
miscdevice..o: /usr/include/xlocale.h
miscdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/float.h
miscdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
miscdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
miscdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
miscdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
nodevice..o: ../include/device.h ../include/lock.h ../include/nodevice.h
nodevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
nodevice..o: /usr/include/bits/sigset.h /usr/include/bits/sigthread.h
nodevice..o: /usr/include/bits/types.h /usr/include/endian.h
nodevice..o: /usr/include/features.h /usr/include/pthread.h
nodevice..o: /usr/include/sched.h /usr/include/signal.h
nodevice..o: /usr/include/sys/select.h /usr/include/sys/types.h
nodevice..o: /usr/include/time.h
nodevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
p2osdevice..o: ../include/device.h ../include/lock.h ../include/messages.h
p2osdevice..o: ../include/p2osdevice.h ../include/packet.h
p2osdevice..o: ../include/playercommon.h ../include/sip.h
p2osdevice..o: /usr/include/_G_config.h /usr/include/alloca.h
p2osdevice..o: /usr/include/asm/sigcontext.h /usr/include/asm/socket.h
p2osdevice..o: /usr/include/assert.h /usr/include/bits/confname.h
p2osdevice..o: /usr/include/bits/environments.h /usr/include/bits/fcntl.h
p2osdevice..o: /usr/include/bits/huge_val.h /usr/include/bits/in.h
p2osdevice..o: /usr/include/bits/local_lim.h /usr/include/bits/mathcalls.h
p2osdevice..o: /usr/include/bits/mathdef.h /usr/include/bits/nan.h
p2osdevice..o: /usr/include/bits/posix1_lim.h /usr/include/bits/pthreadtypes.h
p2osdevice..o: /usr/include/bits/sched.h /usr/include/bits/sigaction.h
p2osdevice..o: /usr/include/bits/sigcontext.h /usr/include/bits/siginfo.h
p2osdevice..o: /usr/include/bits/sigset.h /usr/include/bits/sigstack.h
p2osdevice..o: /usr/include/bits/sigthread.h /usr/include/bits/sockaddr.h
p2osdevice..o: /usr/include/bits/socket.h /usr/include/bits/stat.h
p2osdevice..o: /usr/include/bits/termios.h /usr/include/bits/time.h
p2osdevice..o: /usr/include/bits/types.h /usr/include/bits/waitstatus.h
p2osdevice..o: /usr/include/bits/xopen_lim.h /usr/include/endian.h
p2osdevice..o: /usr/include/fcntl.h /usr/include/features.h
p2osdevice..o: /usr/include/gconv.h /usr/include/getopt.h /usr/include/libio.h
p2osdevice..o: /usr/include/limits.h /usr/include/math.h
p2osdevice..o: /usr/include/netinet/in.h /usr/include/pthread.h
p2osdevice..o: /usr/include/sched.h /usr/include/signal.h
p2osdevice..o: /usr/include/stdint.h /usr/include/stdio.h
p2osdevice..o: /usr/include/stdlib.h /usr/include/string.h
p2osdevice..o: /usr/include/sys/select.h /usr/include/sys/stat.h
p2osdevice..o: /usr/include/sys/time.h /usr/include/sys/types.h
p2osdevice..o: /usr/include/sys/ucontext.h /usr/include/termios.h
p2osdevice..o: /usr/include/time.h /usr/include/ucontext.h
p2osdevice..o: /usr/include/unistd.h /usr/include/values.h
p2osdevice..o: /usr/include/wchar.h /usr/include/xlocale.h
p2osdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/float.h
p2osdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
p2osdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
p2osdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
p2osdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
packet..o: ../include/packet.h /usr/include/_G_config.h /usr/include/alloca.h
packet..o: /usr/include/bits/confname.h /usr/include/bits/environments.h
packet..o: /usr/include/bits/errno.h /usr/include/bits/pthreadtypes.h
packet..o: /usr/include/bits/sched.h /usr/include/bits/sigset.h
packet..o: /usr/include/bits/time.h /usr/include/bits/types.h
packet..o: /usr/include/bits/waitstatus.h /usr/include/endian.h
packet..o: /usr/include/errno.h /usr/include/features.h /usr/include/gconv.h
packet..o: /usr/include/getopt.h /usr/include/libio.h
packet..o: /usr/include/linux/errno.h /usr/include/stdio.h
packet..o: /usr/include/stdlib.h /usr/include/string.h
packet..o: /usr/include/sys/select.h /usr/include/sys/time.h
packet..o: /usr/include/sys/types.h /usr/include/time.h /usr/include/unistd.h
packet..o: /usr/include/wchar.h /usr/include/xlocale.h
packet..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
packet..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
positiondevice..o: ../include/device.h ../include/lock.h ../include/messages.h
positiondevice..o: ../include/p2osdevice.h ../include/packet.h
positiondevice..o: ../include/playercommon.h ../include/positiondevice.h
positiondevice..o: ../include/sip.h /usr/include/_G_config.h
positiondevice..o: /usr/include/asm/socket.h /usr/include/assert.h
positiondevice..o: /usr/include/bits/in.h /usr/include/bits/local_lim.h
positiondevice..o: /usr/include/bits/posix1_lim.h
positiondevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
positiondevice..o: /usr/include/bits/sigset.h /usr/include/bits/sigthread.h
positiondevice..o: /usr/include/bits/sockaddr.h /usr/include/bits/socket.h
positiondevice..o: /usr/include/bits/time.h /usr/include/bits/types.h
positiondevice..o: /usr/include/bits/xopen_lim.h /usr/include/endian.h
positiondevice..o: /usr/include/features.h /usr/include/gconv.h
positiondevice..o: /usr/include/libio.h /usr/include/limits.h
positiondevice..o: /usr/include/netinet/in.h /usr/include/pthread.h
positiondevice..o: /usr/include/sched.h /usr/include/signal.h
positiondevice..o: /usr/include/stdint.h /usr/include/stdio.h
positiondevice..o: /usr/include/string.h /usr/include/sys/select.h
positiondevice..o: /usr/include/sys/time.h /usr/include/sys/types.h
positiondevice..o: /usr/include/time.h /usr/include/values.h
positiondevice..o: /usr/include/wchar.h /usr/include/xlocale.h
positiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/float.h
positiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
positiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
positiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
positiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
ptzdevice..o: ../include/device.h ../include/lock.h ../include/messages.h
ptzdevice..o: ../include/playercommon.h ../include/ptzdevice.h
ptzdevice..o: /usr/include/_G_config.h /usr/include/alloca.h
ptzdevice..o: /usr/include/asm/sigcontext.h /usr/include/asm/socket.h
ptzdevice..o: /usr/include/assert.h /usr/include/bits/confname.h
ptzdevice..o: /usr/include/bits/environments.h /usr/include/bits/fcntl.h
ptzdevice..o: /usr/include/bits/in.h /usr/include/bits/local_lim.h
ptzdevice..o: /usr/include/bits/posix1_lim.h /usr/include/bits/pthreadtypes.h
ptzdevice..o: /usr/include/bits/sched.h /usr/include/bits/sigaction.h
ptzdevice..o: /usr/include/bits/sigcontext.h /usr/include/bits/siginfo.h
ptzdevice..o: /usr/include/bits/sigset.h /usr/include/bits/sigstack.h
ptzdevice..o: /usr/include/bits/sigthread.h /usr/include/bits/sockaddr.h
ptzdevice..o: /usr/include/bits/socket.h /usr/include/bits/stat.h
ptzdevice..o: /usr/include/bits/termios.h /usr/include/bits/types.h
ptzdevice..o: /usr/include/bits/waitstatus.h /usr/include/bits/xopen_lim.h
ptzdevice..o: /usr/include/endian.h /usr/include/fcntl.h
ptzdevice..o: /usr/include/features.h /usr/include/gconv.h
ptzdevice..o: /usr/include/getopt.h /usr/include/libio.h /usr/include/limits.h
ptzdevice..o: /usr/include/netinet/in.h /usr/include/pthread.h
ptzdevice..o: /usr/include/sched.h /usr/include/signal.h /usr/include/stdint.h
ptzdevice..o: /usr/include/stdio.h /usr/include/stdlib.h /usr/include/string.h
ptzdevice..o: /usr/include/sys/select.h /usr/include/sys/stat.h
ptzdevice..o: /usr/include/sys/types.h /usr/include/sys/ucontext.h
ptzdevice..o: /usr/include/termios.h /usr/include/time.h
ptzdevice..o: /usr/include/ucontext.h /usr/include/unistd.h
ptzdevice..o: /usr/include/wchar.h /usr/include/xlocale.h
ptzdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
ptzdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
ptzdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
ptzdevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
pubsub_util..o: /usr/include/_G_config.h /usr/include/asm/socket.h
pubsub_util..o: /usr/include/bits/confname.h /usr/include/bits/environments.h
pubsub_util..o: /usr/include/bits/fcntl.h /usr/include/bits/in.h
pubsub_util..o: /usr/include/bits/local_lim.h /usr/include/bits/netdb.h
pubsub_util..o: /usr/include/bits/posix1_lim.h
pubsub_util..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
pubsub_util..o: /usr/include/bits/sigset.h /usr/include/bits/sockaddr.h
pubsub_util..o: /usr/include/bits/socket.h /usr/include/bits/stat.h
pubsub_util..o: /usr/include/bits/types.h /usr/include/bits/xopen_lim.h
pubsub_util..o: /usr/include/endian.h /usr/include/fcntl.h
pubsub_util..o: /usr/include/features.h /usr/include/gconv.h
pubsub_util..o: /usr/include/getopt.h /usr/include/libio.h
pubsub_util..o: /usr/include/limits.h /usr/include/netdb.h
pubsub_util..o: /usr/include/netinet/in.h /usr/include/rpc/netdb.h
pubsub_util..o: /usr/include/stdint.h /usr/include/stdio.h
pubsub_util..o: /usr/include/string.h /usr/include/sys/select.h
pubsub_util..o: /usr/include/sys/socket.h /usr/include/sys/stat.h
pubsub_util..o: /usr/include/sys/types.h /usr/include/time.h
pubsub_util..o: /usr/include/unistd.h /usr/include/wchar.h
pubsub_util..o: /usr/include/xlocale.h
pubsub_util..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
pubsub_util..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
pubsub_util..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
pubsub_util..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
sip..o: ../include/device.h ../include/lock.h ../include/messages.h
sip..o: ../include/p2osdevice.h ../include/packet.h ../include/playercommon.h
sip..o: ../include/sip.h /usr/include/_G_config.h /usr/include/alloca.h
sip..o: /usr/include/asm/socket.h /usr/include/assert.h
sip..o: /usr/include/bits/huge_val.h /usr/include/bits/in.h
sip..o: /usr/include/bits/local_lim.h /usr/include/bits/mathcalls.h
sip..o: /usr/include/bits/mathdef.h /usr/include/bits/nan.h
sip..o: /usr/include/bits/posix1_lim.h /usr/include/bits/pthreadtypes.h
sip..o: /usr/include/bits/sched.h /usr/include/bits/sigset.h
sip..o: /usr/include/bits/sigthread.h /usr/include/bits/sockaddr.h
sip..o: /usr/include/bits/socket.h /usr/include/bits/time.h
sip..o: /usr/include/bits/types.h /usr/include/bits/waitstatus.h
sip..o: /usr/include/bits/xopen_lim.h /usr/include/endian.h
sip..o: /usr/include/features.h /usr/include/gconv.h /usr/include/libio.h
sip..o: /usr/include/limits.h /usr/include/math.h /usr/include/netinet/in.h
sip..o: /usr/include/pthread.h /usr/include/sched.h /usr/include/signal.h
sip..o: /usr/include/stdint.h /usr/include/stdio.h /usr/include/stdlib.h
sip..o: /usr/include/string.h /usr/include/sys/select.h
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
sonardevice..o: ../include/sonardevice.h /usr/include/_G_config.h
sonardevice..o: /usr/include/asm/socket.h /usr/include/assert.h
sonardevice..o: /usr/include/bits/in.h /usr/include/bits/local_lim.h
sonardevice..o: /usr/include/bits/posix1_lim.h
sonardevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
sonardevice..o: /usr/include/bits/sigset.h /usr/include/bits/sigthread.h
sonardevice..o: /usr/include/bits/sockaddr.h /usr/include/bits/socket.h
sonardevice..o: /usr/include/bits/time.h /usr/include/bits/types.h
sonardevice..o: /usr/include/bits/xopen_lim.h /usr/include/endian.h
sonardevice..o: /usr/include/features.h /usr/include/gconv.h
sonardevice..o: /usr/include/libio.h /usr/include/limits.h
sonardevice..o: /usr/include/netinet/in.h /usr/include/pthread.h
sonardevice..o: /usr/include/sched.h /usr/include/signal.h
sonardevice..o: /usr/include/stdint.h /usr/include/stdio.h
sonardevice..o: /usr/include/string.h /usr/include/sys/select.h
sonardevice..o: /usr/include/sys/time.h /usr/include/sys/types.h
sonardevice..o: /usr/include/time.h /usr/include/values.h /usr/include/wchar.h
sonardevice..o: /usr/include/xlocale.h
sonardevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/float.h
sonardevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
sonardevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
sonardevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
sonardevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
stagedevice..o: ../include/arenalock.h ../include/device.h ../include/lock.h
stagedevice..o: ../include/messages.h ../include/playercommon.h
stagedevice..o: ../include/stage.h ../include/stagedevice.h
stagedevice..o: /usr/include/_G_config.h /usr/include/assert.h
stagedevice..o: /usr/include/bits/ipc.h /usr/include/bits/pthreadtypes.h
stagedevice..o: /usr/include/bits/sched.h /usr/include/bits/sem.h
stagedevice..o: /usr/include/bits/sigset.h /usr/include/bits/sigthread.h
stagedevice..o: /usr/include/bits/types.h /usr/include/endian.h
stagedevice..o: /usr/include/features.h /usr/include/gconv.h
stagedevice..o: /usr/include/libio.h /usr/include/pthread.h
stagedevice..o: /usr/include/sched.h /usr/include/signal.h
stagedevice..o: /usr/include/stdint.h /usr/include/stdio.h
stagedevice..o: /usr/include/string.h /usr/include/sys/ipc.h
stagedevice..o: /usr/include/sys/select.h /usr/include/sys/sem.h
stagedevice..o: /usr/include/sys/types.h /usr/include/time.h
stagedevice..o: /usr/include/wchar.h /usr/include/xlocale.h
stagedevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
stagedevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
visiondevice..o: ../include/device.h ../include/lock.h ../include/messages.h
visiondevice..o: ../include/playercommon.h ../include/pubsub_util.h
visiondevice..o: ../include/visiondevice.h /usr/include/_G_config.h
visiondevice..o: /usr/include/alloca.h /usr/include/asm/sigcontext.h
visiondevice..o: /usr/include/asm/socket.h /usr/include/assert.h
visiondevice..o: /usr/include/bits/confname.h /usr/include/bits/environments.h
visiondevice..o: /usr/include/bits/fcntl.h /usr/include/bits/in.h
visiondevice..o: /usr/include/bits/local_lim.h /usr/include/bits/netdb.h
visiondevice..o: /usr/include/bits/posix1_lim.h
visiondevice..o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
visiondevice..o: /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h
visiondevice..o: /usr/include/bits/siginfo.h /usr/include/bits/sigset.h
visiondevice..o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
visiondevice..o: /usr/include/bits/sockaddr.h /usr/include/bits/socket.h
visiondevice..o: /usr/include/bits/stat.h /usr/include/bits/types.h
visiondevice..o: /usr/include/bits/waitstatus.h /usr/include/bits/xopen_lim.h
visiondevice..o: /usr/include/endian.h /usr/include/fcntl.h
visiondevice..o: /usr/include/features.h /usr/include/gconv.h
visiondevice..o: /usr/include/getopt.h /usr/include/libio.h
visiondevice..o: /usr/include/limits.h /usr/include/netdb.h
visiondevice..o: /usr/include/netinet/in.h /usr/include/pthread.h
visiondevice..o: /usr/include/rpc/netdb.h /usr/include/sched.h
visiondevice..o: /usr/include/signal.h /usr/include/stdint.h
visiondevice..o: /usr/include/stdio.h /usr/include/stdlib.h
visiondevice..o: /usr/include/string.h /usr/include/sys/select.h
visiondevice..o: /usr/include/sys/socket.h /usr/include/sys/stat.h
visiondevice..o: /usr/include/sys/types.h /usr/include/sys/ucontext.h
visiondevice..o: /usr/include/time.h /usr/include/ucontext.h
visiondevice..o: /usr/include/unistd.h /usr/include/wchar.h
visiondevice..o: /usr/include/xlocale.h
visiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/limits.h
visiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h
visiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h
visiondevice..o: /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/syslimits.h
