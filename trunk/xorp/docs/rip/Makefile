LATEX   = latex
BIBTEX  = bibtex
METAPOST= mpost
DVIPS   = dvips -j0
TEXSRC  = design-talk.tex
PS2PDF  = ps2pdf13 -dEncodeColorImages=false
RM	= rm

FIGS=$(wildcard figs/*.eps)

all: design-talk.ps design-talk.pdf

%.dvi: %.tex
	@if [ ! -r $(subst tex,aux,$<) ]; then $(LATEX) $<; fi
	$(LATEX) $<

%.ps: %.dvi
	$(DVIPS) -t letter -t landscape -o $@ $<

%.pdf: %.ps
	$(PS2PDF) $< $@

design-talk.dvi: $(TEXSRCS) $(FIGS)
design-talk.aux: $(TEXSRCS) $(FIGS)

clean:
	$(RM) -rf *.log *.aux *.dvi *.bbl *.blg *.lof *.lot *.toc *~