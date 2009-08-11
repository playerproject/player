INCLUDE (${PLAYER_CMAKE_DIR}/PlayerUtils.cmake)

INCLUDE (CheckFunctionExists)
INCLUDE (CheckIncludeFiles)
INCLUDE (CheckLibraryExists)
INCLUDE (CheckSymbolExists)

SET (PLAYER_EXTRA_LIB_DIRS "" CACHE STRING "List of extra library directories.")
MARK_AS_ADVANCED (PLAYER_EXTRA_LIB_DIRS)
SET (PLAYER_EXTRA_INCLUDE_DIRS "" CACHE STRING "List of extra include directories.")
MARK_AS_ADVANCED (PLAYER_EXTRA_INCLUDE_DIRS)
SET (CMAKE_REQUIRED_INCLUDES ${PLAYER_EXTRA_INCLUDE_DIRS})

IF (PLAYER_OS_QNX)
    SET (CMAKE_REQUIRED_LIBRARIES socket)
ELSEIF (PLAYER_OS_SOLARIS)
    SET (CMAKE_REQUIRED_LIBRARIES socket nsl)
ENDIF (PLAYER_OS_QNX)
CHECK_FUNCTION_EXISTS (getaddrinfo HAVE_GETADDRINFO)
SET (CMAKE_REQUIRED_LIBRARIES)

IF (PLAYER_OS_QNX)
    SET (CMAKE_REQUIRED_LIBRARIES rpc)
ELSEIF (PLAYER_OS_SOLARIS)
    SET (CMAKE_REQUIRED_LIBRARIES nsl)
ENDIF (PLAYER_OS_QNX)
CHECK_FUNCTION_EXISTS (xdr_free HAVE_XDR)
IF (HAVE_XDR)
    CHECK_FUNCTION_EXISTS (xdr_longlong_t HAVE_XDR_LONGLONG_T)
ELSE (HAVE_XDR)
    # If XDR isn't present, set this to true since we'll be replacing all of XDR anyway.
    # TODO: Maybe fold the existing replacement for xdr_longlong_t() that is in functiontable.c
    # into libreplace?
    SET (HAVE_XDR_LONGLONG_T TRUE)
ENDIF (HAVE_XDR)
SET (CMAKE_REQUIRED_LIBRARIES)

IF (CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION EQUAL 6)
    INCLUDE (CheckStructHasMember)
    CHECK_STRUCT_HAS_MEMBER ("struct timespec" tv_sec time.h HAVE_STRUCT_TIMESPEC)
ELSE (CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION EQUAL 6)
    INCLUDE (CheckCSourceCompiles)
    SET (CHECK_TIMESPEC_SOURCE_CODE "#include <time.h>
int main () { struct timespec *tmp; return 0; }")
    CHECK_C_SOURCE_COMPILES ("${CHECK_TIMESPEC_SOURCE_CODE}" HAVE_STRUCT_TIMESPEC)
ENDIF (CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION EQUAL 6)

CHECK_FUNCTION_EXISTS (gettimeofday HAVE_GETTIMEOFDAY)
CHECK_FUNCTION_EXISTS (usleep HAVE_USLEEP)
CHECK_FUNCTION_EXISTS (cfmakeraw HAVE_CFMAKERAW)
CHECK_FUNCTION_EXISTS (dirname HAVE_DIRNAME)
CHECK_FUNCTION_EXISTS (getopt HAVE_GETOPT)
CHECK_FUNCTION_EXISTS (getopt_long HAVE_GETOPT_LONG)
CHECK_INCLUDE_FILES (linux/joystick.h HAVE_LINUX_JOYSTICK_H)
CHECK_INCLUDE_FILES (stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILES (strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILES (dns_sd.h HAVE_DNS_SD)
CHECK_INCLUDE_FILES (sys/filio.h HAVE_SYS_FILIO_H)
CHECK_INCLUDE_FILES (ieeefp.h HAVE_IEEEFP_H)
IF (HAVE_DNS_SD)
    CHECK_LIBRARY_EXISTS (dns_sd DNSServiceRefDeallocate "${PLAYER_EXTRA_LIB_DIRS}" HAVE_DNS_SD)
ENDIF (HAVE_DNS_SD)
SET (CMAKE_REQUIRED_LIBRARIES dl)
CHECK_LIBRARY_EXISTS (ltdl lt_dlopenext "${PLAYER_EXTRA_LIB_DIRS}" HAVE_LIBLTDL)
SET (CMAKE_REQUIRED_LIBRARIES)
IF (PLAYER_OS_SOLARIS)
    SET (CMAKE_REQUIRED_LIBRARIES rt)
    CHECK_FUNCTION_EXISTS (nanosleep HAVE_NANOSLEEP)
    SET (CMAKE_REQUIRED_LIBRARIES)
ELSE (PLAYER_OS_SOLARIS)
    CHECK_FUNCTION_EXISTS (nanosleep HAVE_NANOSLEEP)
ENDIF (PLAYER_OS_SOLARIS)

CHECK_FUNCTION_EXISTS (poll HAVE_POLL)
IF (PLAYER_OS_WIN)
    CHECK_SYMBOL_EXISTS (POLLIN winsock2.h HAVE_POLLIN)
    # This macro will have been pulled in by the previous usage on Windows
    CHECK_STRUCT_HAS_MEMBER ("struct pollfd" fd winsock2.h HAVE_POLLFD)
ELSE (PLAYER_OS_WIN)
    SET (HAVE_POLLIN ${HAVE_POLL})
    SET (HAVE_POLLFD ${HAVE_POLL})
ENDIF (PLAYER_OS_WIN)

CHECK_LIBRARY_EXISTS (m atan2 "${PLAYER_EXTRA_LIB_DIRS}" HAVE_M)
IF (HAVE_M)
    SET (CMAKE_REQUIRED_LIBRARIES m)
    CHECK_FUNCTION_EXISTS (round HAVE_ROUND)
    CHECK_FUNCTION_EXISTS (rint HAVE_RINT)
    SET (CMAKE_REQUIRED_LIBRARIES)
ELSE (HAVE_M)
    IF (NOT PLAYER_OS_WIN)
        # Windows doesn't have libm, but has many of the functions that are found in it anyway
        MESSAGE (STATUS "Warning: libm was not found. This may or may not be a problem, depending on your platform.")
    ENDIF (NOT PLAYER_OS_WIN)
    SET (HAVE_ROUND FALSE)
ENDIF (HAVE_M)

CHECK_LIBRARY_EXISTS (jpeg jpeg_read_header "${PLAYER_EXTRA_LIB_DIRS}" HAVE_LIBJPEG)
CHECK_INCLUDE_FILES ("stdio.h;jpeglib.h" HAVE_JPEGLIB_H)
IF (HAVE_LIBJPEG AND HAVE_JPEGLIB_H)
    SET (HAVE_JPEG TRUE)
ENDIF (HAVE_LIBJPEG AND HAVE_JPEGLIB_H)

SET (CMAKE_REQUIRED_INCLUDES zlib.h)
SET (CMAKE_REQUIRED_LIBRARIES z)
CHECK_FUNCTION_EXISTS (compressBound HAVE_COMPRESSBOUND)
SET (CMAKE_REQUIRED_INCLUDES)
SET (CMAKE_REQUIRED_LIBRARIES)

CHECK_LIBRARY_EXISTS (z compress2 "${PLAYER_EXTRA_LIB_DIRS}" HAVE_LIBZ)
CHECK_INCLUDE_FILES (zlib.h HAVE_ZLIB_H)
IF (HAVE_LIBZ AND HAVE_ZLIB_H)
    SET (HAVE_Z TRUE)
ENDIF (HAVE_LIBZ AND HAVE_ZLIB_H)

CHECK_LIBRARY_EXISTS (rt clock_gettime "${PLAYER_EXTRA_LIB_DIRS}" HAVE_LIBRT)
SET (CMAKE_REQUIRED_LIBRARIES rt)
CHECK_FUNCTION_EXISTS (clock_gettime HAVE_CLOCK_GETTIME_FUNC)
SET (CMAKE_REQUIRED_LIBRARIES)
IF (HAVE_LIBRT AND HAVE_CLOCK_GETTIME_FUNC)
    SET (HAVE_CLOCK_GETTIME TRUE)
ENDIF (HAVE_LIBRT AND HAVE_CLOCK_GETTIME_FUNC)

# Geos check
CHECK_LIBRARY_EXISTS (geos_c GEOSGeomFromWKB_buf "${PLAYER_EXTRA_LIB_DIRS}" HAVE_GEOS)

# Endianess check
INCLUDE (TestBigEndian)
TEST_BIG_ENDIAN (WORDS_BIGENDIAN)

# GTK checks
INCLUDE (FindPkgConfig)
IF (NOT PKG_CONFIG_FOUND)
    MESSAGE (STATUS "WARNING: Could not find pkg-config. This will prevent searching for GTK and building many drivers.")
ELSE (NOT PKG_CONFIG_FOUND)
    pkg_check_modules (GNOMECANVAS_PKG libgnomecanvas-2.0)
    IF (GNOMECANVAS_PKG_FOUND)
        SET (WITH_GNOMECANVAS TRUE)
        # Because the FindPkgConfig module annoyingly separates *all* results, not just the dirs,
        # we have to unseparate the flags back into strings so they can be passed to the
        # SET_x_PROPERTIES functions properly. Change the LIST_TO_STRING to SET and make with
        # VERBOSE=1 to see the mess that happens otherwise.
        LIST_TO_STRING (GNOMECANVAS_LINKFLAGS "${GNOMECANVAS_PKG_LDFLAGS_OTHER}")
        LIST_TO_STRING (GNOMECANVAS_CFLAGS "${GNOMECANVAS_PKG_CFLAGS_OTHER}")
    ENDIF (GNOMECANVAS_PKG_FOUND)

    pkg_check_modules (GTK_PKG gtk+-2.0)
    IF (GTK_PKG_FOUND)
        SET (WITH_GTK TRUE)
        LIST_TO_STRING (GTK_LINKFLAGS "${GTK_PKG_LDFLAGS_OTHER}")
        LIST_TO_STRING (GTK_CFLAGS "${GTK_PKG_CFLAGS_OTHER}")
    ENDIF (GTK_PKG_FOUND)

    pkg_check_modules (GDKPIXBUF_PKG gdk-pixbuf-2.0)
    IF (GDKPIXBUF_PKG_FOUND)
        SET (WITH_GDKPIXBUF TRUE)
        LIST_TO_STRING (GDKPIXBUF_LINKFLAGS "${GDKPIXBUF_PKG_LDFLAGS_OTHER}")
        LIST_TO_STRING (GDKPIXBUF_CFLAGS "${GDKPIXBUF_PKG_CFLAGS_OTHER}")
    ENDIF (GDKPIXBUF_PKG_FOUND)
ENDIF (NOT PKG_CONFIG_FOUND)

IF (PLAYER_OS_QNX)
    SET (PTHREAD_LIB "" CACHE STRING "Name of the pthread library (e.g. empty for QNX).")
    MARK_AS_ADVANCED (PTHREAD_LIB)
    SET (SOCKET_LIBS socket)
    SET (SOCKET_LIBS_FLAGS -lsocket)
ELSEIF (PLAYER_OS_SOLARIS)
    SET (PTHREAD_LIB "pthread" CACHE STRING "Name of the pthread library (e.g. pthread for Solaris).")
    MARK_AS_ADVANCED (PTHREAD_LIB)
    SET (SOCKET_LIBS socket nsl)
    SET (SOCKET_LIBS_FLAGS "-lsocket -lnsl")
ELSEIF (PLAYER_OS_WIN)
    SET (PTHREAD_LIB "pthreadVC2" CACHE STRING "Name of the pthread library (e.g. pthreadVC2 to use pthreadVC2.dll on Windows).")
    MARK_AS_ADVANCED (PTHREAD_LIB)
    SET (SOCKET_LIBS)
    SET (SOCKET_LIBS_FLAGS)
ELSE (PLAYER_OS_QNX)
    SET (PTHREAD_LIB "pthread" CACHE STRING "Name of the pthread library (e.g. pthread for most operating systems).")
    MARK_AS_ADVANCED (PTHREAD_LIB)
    SET (SOCKET_LIBS)
    SET (SOCKET_LIBS_FLAGS)
ENDIF (PLAYER_OS_QNX)

# STL check
SET (testSTLSource ${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp/test_stl.cpp)
FILE (WRITE ${testSTLSource}
    "#include <string>\nint main () {std::string a = \"blag\"; return 0;}\n")
TRY_COMPILE (HAVE_STL ${CMAKE_CURRENT_BINARY_DIR} ${testSTLSource})

# POSIX threads check
IF (PLAYER_OS_WIN)
    SET (PTHREAD_DIR "" CACHE PATH "Root dir of pthreads-win32 installation.")
    IF (PTHREAD_DIR STREQUAL "")
        MESSAGE (FATAL_ERROR "Set PTHREAD_DIR to the directory in which you have installed pthreads-win32, then re-run CMake.")
    ENDIF (PTHREAD_DIR STREQUAL "")
    SET (PTHREAD_INCLUDE_DIR "${PTHREAD_DIR}/include" CACHE PATH "Include directory for a POSIX threads implementation providing pthread.h.")
    SET (PTHREAD_LIB_DIR "${PTHREAD_DIR}/lib" CACHE PATH "Library directory for a POSIX threads implementation providing pthread.lib.")
ELSE (PLAYER_OS_WIN)
    SET (PTHREAD_INCLUDE_DIR "" CACHE PATH "Include directory for a POSIX threads implementation providing pthread.h.")
    SET (PTHREAD_LIB_DIR "" CACHE PATH "Library directory for a POSIX threads implementation providing pthread.lib.")
ENDIF (PLAYER_OS_WIN)
MARK_AS_ADVANCED (PTHREAD_INCLUDE_DIR)
IF (NOT PTHREAD_INCLUDE_DIR STREQUAL "")
    FILE (TO_CMAKE_PATH ${PTHREAD_INCLUDE_DIR} PTHREAD_INCLUDE_DIR)
ENDIF (NOT PTHREAD_INCLUDE_DIR STREQUAL "")
MARK_AS_ADVANCED (PTHREAD_LIB_DIR)
IF (NOT PTHREAD_LIB_DIR STREQUAL "")
    FILE (TO_CMAKE_PATH ${PTHREAD_LIB_DIR} PTHREAD_LIB_DIR)
ENDIF (NOT PTHREAD_LIB_DIR STREQUAL "")
# With Visual C++, this check seems to randomly fail and randomly succeed, irrespective of what PTHREAD_INCLUDE_DIR is set to.
# The library check does the same (although it often fails even when the include check works). There is no indication why they
# fail in CMakeError.log because the command line for compiling doesn't seem to mention any stuff to do with the paths I set
# here, and doesn't give the link command at all. Putting just these checks in an otherwise-empty CMake project has them pass
# every time, which doesn't help. So instead of wasting time banging my head against a brick compiler, if the header check
# passes, assume the lib dir is set correctly as well for now.
SET (CMAKE_REQUIRED_INCLUDES "${PTHREAD_INCLUDE_DIR}")
CHECK_INCLUDE_FILES ("pthread.h" HAVE_PTHREAD_H)
IF (HAVE_PTHREAD_H)
    #CHECK_LIBRARY_EXISTS (${PTHREAD_LIB} pthread_create "${PTHREAD_LIB_DIR}" HAVE_PTHREAD_LIB)
    SET (HAVE_PTHREAD_LIB TRUE)
    IF (NOT HAVE_PTHREAD_LIB)
        MESSAGE (FATAL_ERROR "Could not find pthread${PTHREAD_LIB_SUFFIX} library. Perhaps you need to set PTHREAD_INCLUDE_DIR and PTHREAD_LIB_DIR in advanced mode.")
    ENDIF (NOT HAVE_PTHREAD_LIB)
ELSE (HAVE_PTHREAD_H)
    MESSAGE (FATAL_ERROR "Could not find pthread.h. Perhaps you need to set PTHREAD_INCLUDE_DIR and PTHREAD_LIB_DIR in advanced mode.")
ENDIF (HAVE_PTHREAD_H)
SET (CMAKE_REQUIRED_INCLUDES)

IF (PLAYER_OS_WIN)
    TRY_COMPILE (HAVE_SETDLLDIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/test_have_setdlldirectory
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/internal/setdlldirectory.c)
ENDIF (PLAYER_OS_WIN)

