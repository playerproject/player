EXTRA_DIST = configfile.html \
             footer.html \
             footer.php \
             header.html \
             header.php \
             player.dox \
	     player.txt \
             architecture.txt \
             install.txt \
             player.txt \
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
             plugin_driver.html \
             protocol.html \
             upload_www.sh \
             utils.html

CVS_DEST=Player-cvs
NAME=player

doc:
	doxygen player.dox

#doxygen playerweb.dox
webdoc:
	doxygen player.dox

upload_cvs: webdoc
	./upload_www.sh $(CVS_DEST) $(NAME)
