
from distutils.core import setup, Extension


m_scan = Extension('playermap.cmodules.scan',
                   sources = ['cmodules/pyscan.c',
                              'cmodules/pyscan_group.c',
                              'cmodules/pyscan_match.c',
                              'cmodules/scan.c',
                              'cmodules/scan_contour.c',
                              'cmodules/scan_solid.c',
                              'cmodules/scan_group.c',
                              'cmodules/scan_match.c',
                              'cmodules/gpc.c',
                              'cmodules/vector.c',
                              'cmodules/geom.c'],
                   include_dirs = [],
                   library_dirs = [],
                   libraries = ['gsl', 'gslcblas'])

m_relax = Extension('playermap.cmodules.relax',
                    sources = ['cmodules/pyrelax.c',
                               'cmodules/relax.c',
                               'cmodules/geom.c',
                               'cmodules/vector.c'],
                    include_dirs = [],
                    library_dirs = [],
                    libraries = ['gsl', 'gslcblas'])

m_grid = Extension('playermap.cmodules.grid',
                    sources = ['cmodules/pygrid.c',
                               'cmodules/grid.c',
                               'cmodules/grid_range.c',
                               'cmodules/grid_store.c'],
                    include_dirs = [],
                    library_dirs = [],
                    libraries = [])

setup(name = 'playermap',
      version = 'X.X',
      description = 'Simple Map Maker',
      author = 'Andrew Howard',
      packages = ['playermap', 'playermap.cmodules'],
      package_dir = {'playermap' : '.', 'playermap.cmodules' : './cmodules'},
      ext_modules = [m_scan, m_relax, m_grid],
      scripts = ['playermap'],
      data_files = [('share/playermap', ['playermap.glade'])], 
      )

