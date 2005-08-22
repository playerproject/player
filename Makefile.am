
SUBDIRS = replace libplayercore libplayerxdr libplayertcp rtk2 server libplayercore/bindings config doc examples client_libs utils 

EXTRA_DIST = config.simulation config.p2os

clean-local:
	rm -f config.cache

# This tell distcheck to ignore files left over from distclean
# TODO: fix the python install script to clean up after itself.
distcleancheck:
	@:

# This tell distcheck to ignore files left over from uninstall
distuninstallcheck:
	@:

