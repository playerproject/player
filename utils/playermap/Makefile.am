SUBDIRS = . cmodules

# only build if Python was found by configure
if WITH_PYTHON
noinst_DATA = pythonbuild
PYTHONINSTALL = pythoninstall
endif

MAPPER_FILES = __init__.py main.py gui.py logfile.py map.py robot_mapper.py team_mapper.py geom.py

# These files need to be added to the distro
EXTRA_DIST = playermap.in $(MAPPER_FILES)

# Use distutils to build the Python stuff
pythonbuild: $(MAPPER_FILES)
	sed 's.GLADEFILE.$(prefix)/share/playermap.' playermap.in > playermap
	@PYTHON_INTERP@ setup.py build

# Do our own cleaning
clean-local:
	rm -rf build

install-data-hook: $(PYTHONINSTALL)

# use distutils to install
pythoninstall:
	@PYTHON_INTERP@ setup.py install --prefix=$(prefix)
