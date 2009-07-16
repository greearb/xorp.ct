LATEX = latex
BIBTEX = bibtex
PDFLATEX = pdflatex
METAPOST = mpost
DVIPS = dvips -j0
PS2PDF = ps2pdf

.SUFFIXES: pdf ps dvi tex
.ps.pdf:
	$(PS2PDF) $< $@
.dvi.ps:
	$(DVIPS) $< -o $@
.tex.dvi:
	@if [ ! -r $(subst tex,aux,$<) ]; then $(LATEX) $<; fi
	$(LATEX) $<

# after making the figure, run perl to fix CMR font names
fig/%.1 fig/%.2 fig/%.3 fig/%.4 fig/%.5 fig/%.6 fig/%.7 fig/%.8: fig/%.mp fig/elements.mp fig/elemfig.sty .fonts.sty fig/fonts.mp
	cd fig ; TEX=$(LATEX) $(METAPOST) $(subst fig/,,$<)
	perl -pi -e 'next if $$x; $$x = 1 if /%%EndProlog/; s=/([a-z][a-z0-9]+) def=/\U$$1\E def= if !/^%%/; s=\b([a-z][a-z0-9]+)\b=\U$$1\E=g if /^%%DocumentFonts/ || /^%%\+/;' $(subst .mp,,$<).[0-9]*

FIGS =
TEXSRCS = pospaper.tex intro.tex overview.tex xorp.tex platform.tex summary.tex

all: dvi ps pdf
dvi: pospaper.dvi
ps: pospaper.ps
pdf: pospaper.pdf

pospaper.pdf: pospaper.ps
	$(PS2PDF) $< $@
pospaper.ps: pospaper.dvi
	$(DVIPS) $< -o $@
pospaper.dvi: $(TEXSRCS) $(FIGS) pospaper.bbl always
pospaper.aux: $(TEXSRCS)
	$(LATEX) $<

bib: pospaper.aux always
	$(BIBTEX) -min-crossrefs=100 pospaper
pospaper.bbl: pospaper.aux biblio.bib
	$(BIBTEX) -min-crossrefs=100 pospaper

fig/fonts.mp:
	echo 'defaultelementfont := "ptmr";' > fig/fonts.mp
.fonts.sty: fonts.sty
	rm -f fig/*.mpx
	cp fonts.sty .fonts.sty

always:
	@:
clean: cleanfig
	rm -f paper.pdf paper.dvi paper.aux paper.log paper.bbl paper.blg
cleanfig:
	/bin/rm -f $(FIGS) fig/*.log fig/*.aux fig/*.dvi fig/*.mpx

.PHONY: all dvi pdf bib always clean cleanfig allpdf
