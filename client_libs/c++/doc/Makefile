#
# $Id$
#

# How it works :
# The manual is built when we are creating a distro, then copied to
# the player/doc directory.  The top-level distro script will then
# include the manual in the tarball.  Note that this directory is
# explicitly ignored by the distro script, so we dont really
# want/expect users to be building the manual.

# Name for the manual we are creating
MANNAME = cppclient

# Header files to scan
MANFILES = ../playerclient.h


# Build the manual
manual: 
	./docker.py $(MANFILES)
	latex $(MANNAME)
	dvips -t letter -f $(MANNAME).dvi > $(MANNAME).ps	

# Clean up everything
clean:
	$(RM) -rf generated *.h.tex $(MANNAME).aux $(MANNAME).log \
	          $(MANNAME).dvi $(MANNAME).ps

