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

install: player.ps
	cp -f $< ../doc

clean:
	rm -f player.ps player.dvi player.log player.aux player.blg \
	      player.toc missfont.log player.bbl ../doc/player.ps

