
from distutils.core import setup, Extension


module = Extension('_playerc',
                   sources = ['playerc_wrap.c'],
                   include_dirs = ['../..', '../../../../server'],
                   library_dirs = ['../..'],
                   libraries = ['playerc'])


setup(name = 'playerc',
      version = 'X.X',
      description = 'Bindings for libplayerc',
      py_modules = ['playerc'],
      ext_modules = [module])
