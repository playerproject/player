#
# makefile for the player documentation
#

LATEX = latex
BIBTEX = bibtex
DVIPS = dvips

all: player.ps

player.ps: player.tex player.bib
	$(LATEX) $< && $(BIBTEX) player && $(LATEX) $< && $(LATEX) $<
	dvips -f -t letter player.dvi > $@

draft: player.ps
	./draftify.pl player.ps > player.ps.tmp
	mv -f player.ps.tmp player.ps

install: player.ps
	cp -f $< ../doc

clean:
	rm -f player.ps player.dvi player.log player.aux player.blg \
	      player.toc missfont.log player.bbl ../doc/player.ps

