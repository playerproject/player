
from distutils.core import setup, Extension


module = Extension('playerc',
                   sources = ['pyplayerc.c',
                              'dev_blobfinder.c',
                              'dev_comms.c',
                              'dev_fiducial.c',
                              'dev_gps.c',
                              'dev_laser.c',
                              'dev_localize.c',
                              'dev_position.c',
                              'dev_power.c',
                              'dev_ptz.c',
                              'dev_truth.c',
                              'dev_wifi.c'],
                   include_dirs = ['../../server', '../libplayerc'],
                   library_dirs = ['../libplayerc'],
                   libraries = ['playerc'])



setup(name = 'player',
      version = 'X.X',
      description = 'Player client support',
      ext_modules = [module])
