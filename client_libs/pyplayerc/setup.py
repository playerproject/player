
from distutils.core import setup, Extension


module = Extension('playerc',
                   sources = ['pyplayerc.c',
                              'dev_blobfinder.c',
                              'dev_fiducial.c',
                              'dev_laser.c',
                              'dev_position.c',
                              'dev_ptz.c'],
                   include_dirs = ['../../server', '../libplayerc'],
                   library_dirs = ['../libplayerc'],
                   libraries = ['playerc'],
                   define_macros = [('HAVE_SYS_TYPES_H', '1'), ('HAVE_STDINT_H', '1')])


setup(name = 'player',
      version = 'X.X',
      description = 'Player client support',
      ext_modules = [module])
