#
#
# $Id$
#
# generic LaTeX makefile
#

.SUFFIXES: .tex .i

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
						laser.t.tex laser.f.tex \
						position.t.tex position.f.tex
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

# These targets extra the data and function sections for each proxy
# from the header file.  Each section must start with a comment of
# the form "proxy : DEVICENAME".

%.t.tex : $(INCLUDE)
	echo $< $*
	csplit -f "pass1-" $(INCLUDE) %proxy\ :\ $*%+2  "/playerc_"$*"_t/"+2
	csplit pass1-00 %//% && cat -s xx00 > $@

%.f.tex : $(INCLUDE)
	echo $< $*
	csplit -f "pass1-" $(INCLUDE) %proxy\ :\ $*%+2  "/playerc_"$*"_t/"+2
	csplit pass1-01 %//% /"proxy\ :\ "/-1 && cat -s xx00 > $@

#%.t.tex : $(INCLUDE)
#	echo $< $*
#	csplit -f "pass1-" $(INCLUDE) %$*\ device\ :%+2  "/playerc_"$*"_t/"+2
#	csplit pass1-00 %//% && cat -s xx00 > $@

#%.f.tex : $(INCLUDE)
#	echo $< $*
#	csplit -f "pass1-" $(INCLUDE) %$*\ device\ :%+2  "/playerc_"$*"_t/"+2
#	csplit pass1-01 %//% "//\*\*/"-1 && cat -s xx00 > $@
