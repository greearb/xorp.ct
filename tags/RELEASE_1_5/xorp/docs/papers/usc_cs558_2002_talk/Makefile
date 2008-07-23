all: slides.ps slides.pdf

clean: 
	rm -f *.aux *.log *.dvi slides.ps slides.pdf

.SUFFIXES: .tex .bbl .dvi .ps .pdf .fig .ps .eps

.tex.dvi:
	latex $< $@
#	latex $< $@
.tex.bbl:
	latex $< $@
	bibtex $*
.dvi.ps:
	dvips $< -o $@
.ps.pdf:
	ps2pdf $< $@
.fig.ps:
	fig2dev -Lps $< $@
.fig.eps:
	fig2dev -Leps $< $@
