LATEX	= latex
DVIPS	= dvips -j0
PS2PDF	= ps2pdf14 -sPAPERSIZE=a4 -dEncodeColorImages=false


all: allps allpdf
	
allps: how-to-mibs.ps

allpdf: how-to-mibs.pdf


%.dvi: %.tex
	@if [ ! -r $(subst tex,aux,$<) ]; then $(LATEX) $<; fi
	$(LATEX) $<
%.ps: %.dvi
	$(DVIPS) -t landscape -o $@ $<
%.pdf: %.ps
	$(PS2PDF) $< $@

clean:
	/bin/rm -f *.log *.aux *.dvi *.pdf *.ps *.lof *.lot *.toc *~ \

.PHONY: all allps allpdf clean 
