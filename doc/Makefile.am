EXTRA_DIST = classes.html \
             clients.html \
             configfile.html \
             footer.html \
             footer.php \
             header.html \
             header.php \
             install.html \
             mainpage.html \
             player.dox \
             plugin_driver.html \
             protocol.html \
             upload_www.sh \
             utils.html

CVS_DEST=Player-cvs
NAME=player

doc:
	doxygen player.dox

upload_cvs: all
	./upload_www.sh $(CVS_DEST) $(NAME)
