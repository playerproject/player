#
#
# $Id$
#
# generic LaTeX makefile
#

LATEX = latex
BIBTEX = bibtex
DVIPS = dvips
PS2PDF = ps2pdf

# Manual name
NAME = libplayerc

# Library include file
INCLUDE = ../playerc.h

# Destination for the manual when its done (for distribution)
MANPATH = ../../../doc

dvi: $(NAME).dvi

$(NAME).dvi: $(NAME).tex \
						dev_laser_t.tex dev_laser_f.tex
	$(LATEX) $< && $(LATEX) $<

light: $(NAME).tex
	$(LATEX) $< 

html:
	rm -rf html_docs
	latex2html -split 0 -nonavigation -html_version 3.2,table,math -show_section_numbers -mkdir -dir html_docs $(NAME).tex

ps: $(NAME).ps

$(NAME).ps: $(NAME).dvi
	$(DVIPS) -f -t letter $(NAME).dvi > $@

pdf: $(NAME).pdf

$(NAME).pdf: $(NAME).ps
	$(PS2PDF) $<

manual: ps
	cp $(NAME).ps $(MANPATH)

clean:
	rm -rf $(NAME).ps $(NAME).dvi $(NAME).log \
	      $(NAME).aux $(NAME).blg $(NAME).toc \
	      $(NAME).lot $(NAME).lof \
	      missfont.log $(NAME).bbl $(NAME).pdf *.bak html_docs

dev_laser_t.tex dev_laser_f.tex : $(INCLUDE)
	csplit $(INCLUDE)  %Laser\ %+2  "/playerc_laser_t/"+1 && cp xx00 dev_laser_t.tex
	csplit xx01 "//\*\*/" && cp xx00 dev_laser_f.tex

#	csplit $(INCLUDE)  %Laser\ %+2  "//\*\*/" && cp xx00 dev_laser.tex
