

CVS_DEST=Player-cvs-html
NAME=libplayerc

all:
	doxygen libplayerc.dox

upload_cvs: all
	./upload_www.sh $(CVS_DEST) $(NAME)
