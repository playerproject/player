EXTRA_DIST =  \
architecture.txt \
configfile.html \
footer.html \
footer.php \
header.html \
header.php \
install.txt \
player.dox \
player.txt \
playerweb.dox \
plugin_driver.html \
quick_start.txt \
supported_hardware.txt \
tutorial_args.txt \
tutorial_automake.txt \
tutorial_cameras.txt \
tutorial_config.txt \
tutorial_crosscompiling.txt \
tutorial_devices.txt \
tutorial_maps.txt \
tutorial_migration.txt \
tutorial_plugins.txt \
upload_www.sh 

CVS_DEST=Player-cvs
NAME=player

dist-hook: doc

doc:
	doxygen player.dox

install-data-hook:
	 cp -r -p -v player-docs $(prefix)/doc/player

#doxygen playerweb.dox
webdoc:
	doxygen player.dox

upload_cvs: webdoc
	./upload_www.sh $(CVS_DEST) $(NAME)
