
from distutils.core import setup, Extension


module = Extension('_libplayerc',
                   sources = ['playerc_wrap.c'],
                   include_dirs = ['../..', '../../../../server'],
                   library_dirs = ['../..'],
                   libraries = ['playerc'])


setup(name = 'libplayerc',
      version = 'X.X',
      description = 'Bindings for libplayerc',
      py_modules = ['libplayerc'],
      ext_modules = [module])
