SUBDIRS = . cmodules

# only build if necessary stuff (Python,RTK3) was found
if WITH_PLAYERMAP
noinst_DATA = pythonbuild
PYTHONINSTALL = pythoninstall
endif

MAPPER_FILES = setup.py __init__.py main.py gui.py logfile.py map.py robot_mapper.py team_mapper.py geo_mapper.py geom.py

# These files need to be added to the distro
EXTRA_DIST = playermap.in playermap.glade $(MAPPER_FILES)

# Use distutils to build the Python stuff
pythonbuild: $(MAPPER_FILES)
	sed -e "s+GLADEFILE+$(prefix)/share/playermap+" playermap.in > playermap
	@PYTHON_INTERP@ $(srcdir)/setup.py build

# Do our own cleaning
clean-local:
	rm -rf build

install-data-hook: $(PYTHONINSTALL)

# use distutils to install
pythoninstall:
	@PYTHON_INTERP@ $(srcdir)/setup.py install --prefix=$(prefix)

# seems that distutils doesn't support 'uninstall'; oh well
uninstall-local:
	echo 'Uninstall not implemented (sorry)'
