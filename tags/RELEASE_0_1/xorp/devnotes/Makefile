TARGETS =	 							      \
	  management.ps 						      \
	  error_handling.ps  						      \
	  urib-interface.ps 

all: $(TARGETS)

clean:
	rm -f *.aux *.bbl *.blg *.dvi *.log $(TARGETS)

urib-interface.ps:	urib-interface.dvi
	dvips -o urib-interface.ps urib-interface.dvi
urib-interface.dvi:	urib-interface.tex

error_handling.ps:	error_handling.dvi
error_handling.dvi:	error_handling.tex

management.ps:	management.dvi
	dvips -o management.ps management.dvi
management.dvi:	management.tex management_diag1.tex
management_diag1.tex: management_diag1.pic
	pic -t management_diag1.pic > management_diag1.tex

# -----------------------------------------------------------------------------
# Implicit rules
# -----------------------------------------------------------------------------

%.ps: %.dvi
	dvips -o $@ $<

%.eps: %.fig
	fig2dev -Leps $< $@

%.ps: %.fig
	fig2dev -Lps $< $@

%.dvi: %.tex
	latex $<
	-bibtex $*
	latex $<
	latex $<
