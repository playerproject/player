
from distutils.core import setup, Extension


#TODO: handle linking to libjpeg conditionally, depending on whether or not
#      it was found during configuration.  The easiest way to do this may
#      be to use libplayerpacket's pkgconfig info.  Until then, I'm
#      disabling the use of camera decompression in the libplayerc python
#      bindings.
module = Extension('_playerc',
                   sources = ['playerc.i'],
                   include_dirs = ['../..', '../../../../server'],
                   library_dirs = ['../..', '../../../libplayerpacket'],
                   libraries = ['playerc', 'playerpacket', 'jpeg'])


setup(name = 'playerc',
      version = 'X.X',
      description = 'Bindings for playerc',
      py_modules = ['playerc'],
      ext_modules = [module])
