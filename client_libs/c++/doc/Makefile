#
# $Id$
#

# How it works :
# The manual is built when we are creating a distro, then copied to
# the player/doc directory.  The top-level distro script will then
# include the manual in the tarball.  Note that this directory is
# explicitly ignored by the distro script, so we dont really
# want/expect users to be building the manual.

VERSION = 1.4

# Name for the manual we are creating
NAME = cppclient

# Header files to scan
MANFILES = ../playerclient.h
MAKEDOC = ../../../server/codetools/makedoc/makedoc.py

PS2PDF = ps2pdf


$(NAME).ps: *.tex
	$(MAKEDOC) $(MANFILES)
	latex $(NAME) && latex $(NAME)
	dvips -t letter -f $(NAME).dvi > $(NAME).ps	

ps: $(NAME).ps

# Dummy target to make the interfaces
interfaces: $(MANFILES)
	$(MAKEDOC) $(MANFILES)

html:
	rm -rf html_docs
	latex2html -split 4 -toc_depth 3 -html_version 3.2,table,math -show_section_numbers -mkdir -dir html_docs $(NAME).tex

pdf: $(NAME).pdf

$(NAME).pdf: $(NAME).ps
	$(PS2PDF) $<

distro: ps pdf html
	gzip $(NAME).ps && mv cppclient.ps.gz Player-cppclient-$(VERSION).ps.gz
	mv cppclient.pdf Player-cppclient-$(VERSION).pdf
	mv html_docs Player-cppclient-$(VERSION)-html && tar cvzf Player-cppclient-$(VERSION)-html.tgz Player-cppclient-$(VERSION)-html

# Clean up everything
clean:
	$(RM) -rf *Client.tex *Proxy.tex $(NAME).aux $(NAME).log \
	          $(NAME).dvi $(NAME).ps

