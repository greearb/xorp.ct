LATEX	= latex
BIBTEX	= bibtex
METAPOST= mpost
DVIPS	= dvips -j0
PS2PDF	= ps2pdf13 -dEncodeColorImages=false

TEXSRCS = talk.tex
KILLFIGS = 
SAVEFIGS = flyingpig.eps ipr.1 routingtable.ps processes3.ps
FIGS = $(KILLFIGS) $(SAVEFIGS)

all: talk.ps talk.pdf

%.dvi: %.tex
	@if [ ! -r $(subst tex,aux,$<) ]; then $(LATEX) $<; fi
	$(LATEX) $<
%.ps: %.dvi
	$(DVIPS) -t landscape -o $@ $<
%.pdf: %.ps
	$(PS2PDF) $< $@

# after making the figure, run perl to fix Computer Modern font names to
# uppercase
%.1 %.2 %.3 %.4 %.5 %.6 %.7 %.8: %.mp elements.mp elemfig.sty fonts.mp .fonts.sty
	TEX=$(LATEX) $(METAPOST) $<
	perl -pi -e 'next if $$x; $$x = 1 if /%%EndProlog/; s=/([a-z][a-z0-9]+) def=/\U$$1\E def= if !/^%%/; s=\b([a-z][a-z0-9]+)\b=\U$$1\E=g if /^%%DocumentFonts/ || /^%%\+/;' $(subst .mp,,$<).[0-9]*

.fonts.sty: fonts.sty
	rm -f *.mpx
	cp fonts.sty .fonts.sty
.natcolors.sty: natcolors.sty
	rm -f *.mpx
	cp fonts.sty .fonts.sty

talk.dvi: $(TEXSRCS) $(FIGS) sped.sty .fonts.sty fonts.mp always
talk.aux: $(TEXSRCS)


mlffr.eps: 4h-full.dat 4h-dev.dat 2h-linux.dat 2h-linux-fake.dat graphs.pl
	./graphs.pl
bargraph.eps: graphs.pl
	./graphs.pl
extensions.eps: plebic-06-polling4.dat plebic-06-red2.dat plebic-06-sfq.dat plebic-06-simple2.dat graphs.pl
	./graphs.pl
jextensions.eps tos.eps tos10.eps: graphs.pl
	./graphs.pl

paper.bbl: biblio.bib
	$(LATEX) paper
	$(BIBTEX) -min-crossrefs=100 paper

fonts.sty:
	echo '% nothing' > fonts.sty
fonts.mp:
	echo 'defaultelementfont := "cmti10";' > fonts.mp

always:
	@:

clean:
	/bin/rm -f *.log *.aux *.dvi *.bbl *.blg *.lof *.lot *.toc *~ \
	$(KILLFIGS) *.mpx mpxerr.tex

.PHONY: all bib clean clean-all always
