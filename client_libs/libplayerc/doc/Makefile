#
#
# $Id$
#
# generic LaTeX makefile
#

MAKEDOC = ../makedoc2.py
LATEX = latex
BIBTEX = bibtex
DVIPS = dvips
PS2PDF = ps2pdf

# Manual name
NAME = libplayerc

# Library include file
INCLUDE = ../../playerc.h

all: dvi ps pdf

dvi: $(NAME).dvi

$(NAME).dvi: $(NAME).tex \
#						client.f.tex \
#						device.t.tex \
#						blobfinder.t.tex blobfinder.f.tex \
#						comms.t.tex comms.f.tex \
#						fiducial.t.tex fiducial.f.tex \
#						gps.t.tex gps.f.tex \
#						laser.t.tex laser.f.tex \
#						localize.t.tex localize.f.tex \
#						position.t.tex position.f.tex \
#						ptz.t.tex ptz.f.tex \
#						sonar.t.tex sonar.f.tex \
#						truth.t.tex truth.f.tex \
#						wifi.t.tex wifi.f.tex
	cd proxies && $(MAKEDOC) $(INCLUDE)
	$(LATEX) $< && $(LATEX) $<

light: $(NAME).tex
	$(LATEX) $< 

#html:
#	rm -rf html_docs
#	latex2html -split 0 -nonavigation -html_version 3.2,table,math -show_section_numbers -mkdir -dir html_docs $(NAME).tex

html:
	rm -rf html_docs
	latex2html -split 4 -toc_depth 3 -html_version 3.2,table,math -show_section_numbers -mkdir -dir html_docs $(NAME).tex


ps: $(NAME).ps

$(NAME).ps: $(NAME).dvi
	$(DVIPS) -f -P pdf -t letter $(NAME).dvi > $@

pdf: $(NAME).pdf

$(NAME).pdf: $(NAME).ps
	$(PS2PDF) $<

manual: ps

clean:
	rm -rf $(NAME).ps $(NAME).dvi $(NAME).log \
	      $(NAME).aux $(NAME).blg $(NAME).toc \
	      $(NAME).lot $(NAME).lof \
	      missfont.log $(NAME).bbl $(NAME).pdf *.bak html_docs \
	      *.f.tex *.t.tex pass1-* xx0*

# Special case targets for extracting sections from the header file.

client.f.tex :
	csplit $(INCLUDE) %proxy\ :\ client%+2  "/proxy\ :\ /"-1 && cat -s xx00 > $@

device.t.tex :
	csplit -f "pass1-" $(INCLUDE) %typedef\ struct\ _playerc_device_t%-1  "/playerc_device_t/"+2
	csplit pass1-00 %//% && cat -s xx00 > $@

# These targets extra the data and function sections for each proxy
# from the header file.  Each section must start with a comment of
# the form "proxy : DEVICENAME".

%.t.tex :
	csplit -f "pass1-" $(INCLUDE) %proxy\ :\ $*%+2  "/playerc_"$*"_t/"+2
	csplit pass1-00 %//% && cat -s xx00 > $@

%.f.tex :
	csplit -f "pass1-" $(INCLUDE) %proxy\ :\ $*%+2  "/playerc_"$*"_t/"+2
	csplit pass1-01 %//% /"proxy\ :\ "/-1 && cat -s xx00 > $@
