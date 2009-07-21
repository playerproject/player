from distutils.core import setup, Extension

srcdir = '${CMAKE_CURRENT_SOURCE_DIR}'
top_srcdir = '${PROJECT_SOURCE_DIR}'
builddir = '${CMAKE_CURRENT_BINARY_DIR}'
top_builddir = '${PROJECT_BINARY_DIR}'

module = Extension('_playercpp',
                   sources = ['playercpp.i'],
                   include_dirs = [srcdir + '/../..', top_srcdir, top_builddir, top_builddir + '/libplayercore', top_builddir + '/client_libs'],
                   library_dirs = [builddir + '/../../.libs',
                                   top_builddir + '/libplayerxdr/.libs',
                                   top_builddir + '/libplayercore/.libs',
                                   top_builddir + '/libplayerjpeg/.libs'],
                   libraries = ['playerxdr', 'playerc++', 'playerc', 'playerjpeg', 'jpeg', 'playererror', 'playerwkb'])


setup(name = 'playercpp',
      version = '${PLAYER_VERSION}',
      description = 'Bindings for playerc++',
      py_modules = ['playercpp'],
      ext_modules = [module])
