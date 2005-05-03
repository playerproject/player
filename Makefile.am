
SUBDIRS = replace client_libs rtk2 server utils config examples doc

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

