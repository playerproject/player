

CVS_DEST=Player-cvs-html
NAME=player

all:
	doxygen player.dox

upload_cvs: all
	./upload_www.sh $(CVS_DEST) $(NAME)
