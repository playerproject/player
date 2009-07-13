from distutils.core import setup, Extension

srcdir = '${CMAKE_CURRENT_SOURCE_DIR}'
top_srcdir = '${PROJECT_SOURCE_DIR}'
builddir = '${CMAKE_CURRENT_BINARY_DIR}'
top_builddir = '${PROJECT_BINARY_DIR}'

#TODO: handle linking to libjpeg conditionally, depending on whether or not
#      it was found during configuration.  The easiest way to do this may
#      be to use libplayerpacket's pkgconfig info.  Until then, I'm
#      disabling the use of camera decompression in the libplayerc python
#      bindings.
module = Extension('_playerc',
                   sources = ['playerc.i'],
                   include_dirs = [srcdir + '/../..', top_srcdir, top_builddir, top_builddir + '/libplayerinterface', top_builddir + '/client_libs'],
                   library_dirs = [builddir + '/../../.libs',
                                   top_builddir + '/libplayerinterface/.libs',
                                   top_builddir + '/libplayerjpeg/.libs'],
                   libraries = ['playerxdr', 'playerc', 'playerjpeg', 'jpeg', 'playererror', 'playerwkb'])


setup(name = 'playerc',
      version = '${PLAYER_VERSION}',
      description = 'Bindings for playerc',
      py_modules = ['playerc'],
      ext_modules = [module])
