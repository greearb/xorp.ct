# Makefile.in generated automatically by automake 1.5 from Makefile.am.

# Copyright 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001
# Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.




SHELL = /usr/local/bin/bash

srcdir = .
top_srcdir = ../..

prefix = /usr/local/xorp
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include
pkgdatadir = $(datadir)/xorp
pkglibdir = $(libdir)/xorp
pkgincludedir = $(includedir)/xorp
top_builddir = ../..

ACLOCAL = ${SHELL} /home/aardvark/u0/mjh/xorp/xorp/config/missing --run aclocal
AUTOCONF = ${SHELL} /home/aardvark/u0/mjh/xorp/xorp/config/missing --run autoconf
AUTOMAKE = ${SHELL} /home/aardvark/u0/mjh/xorp/xorp/config/missing --run automake
AUTOHEADER = ${SHELL} /home/aardvark/u0/mjh/xorp/xorp/config/missing --run autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL}
INSTALL_HEADER = $(INSTALL_DATA)
transform = s,x,x,
NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
build_alias = 
build_triplet = i386-unknown-freebsd4.10
host_alias = 
host_triplet = i386-unknown-freebsd4.10
target_alias = 
target_triplet = i386-unknown-freebsd4.10
AMTAR = ${SHELL} /home/aardvark/u0/mjh/xorp/xorp/config/missing --run tar
AR = ar
AWK = nawk
CC = gcc
CLI_CURSES_LIB = -lcurses
CXX = g++
CXXCPP = g++ -E
DEPDIR = .deps
EXEEXT = 
FLOWER_MALLOC = flower_malloc
INSTALL_STRIP_PROGRAM = ${SHELL} $(install_sh) -c -s
LN_S = ln -s
MAINT = #
MIBS = 
OBJEXT = o
OSPFD = ospfd
OSPFD_OS = freebsd
PACKAGE = xorp
PYTHON = python
PY_BUILD = 
RANLIB = ranlib
VERSION = 1.0-RC
am__include = include
am__quote = 
install_sh = /home/aardvark/u0/mjh/xorp/xorp/config/install-sh
net_snmp_found = true
snmpd = /usr/local/sbin/snmpd
snmpd2 = 

TARGETS = $(DOCS:%=%.ps) $(DOCS:%=%.pdf)

DOCS_LATEX = $(DOCS:%=%_LATEX)
DOCS_FIGURES = $(DOCS:%=%_FIGURES)
DOCS_BBL = $(DOCS:%=%_BBL)

PSANON = sed -f $(top_srcdir)/utils/fig2dev-ps-anon.sed

#
# The files to remove with ``gmake clean''
# TODO: don't remove *.foo; instead consider DOCS_LATEX to select the files
#  to remove.
#
CLEANFILES = *.aux *.bbl *.blg *.dvi *.lof *.log *.lot *.toc $(TARGETS)

#
# The path to any additional TeX input files
#
TEXINPUTS_PATH = $(top_srcdir)/docs/tex:

#
# The rules to generate files based on their suffixes
#
SUFFIXES = .tex .bbl .dvi .ps .pdf .fig .ps .eps

# -- Document Names
DOCS = user_manual

user_manual_LATEX = user_manual.tex cli_intro.tex config_overview.tex \
			  preface.tex

user_manual_FIGURES = figs/xorpsh.ps
subdir = docs/user_manual
mkinstalldirs = $(SHELL) $(top_srcdir)/config/mkinstalldirs
CONFIG_HEADER = $(top_builddir)/config.h
CONFIG_CLEAN_FILES =
CFLAGS = -g  -W -Wall -Wwrite-strings -Wbad-function-cast -Wmissing-prototypes -Wcast-qual -Wmissing-declarations -Werror -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wnested-externs -Wnon-const-format
COMPILE = $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
DIST_SOURCES =
DIST_COMMON = Makefile.am Makefile.in
all: all-am

.SUFFIXES:
.SUFFIXES: .tex .bbl .dvi .ps .pdf .fig .ps .eps
$(srcdir)/Makefile.in: # Makefile.am $(top_srcdir)/docs/mk/Makefile.doc.am $(top_srcdir)/configure.in $(ACLOCAL_M4)
	cd $(top_srcdir) && \
	  $(AUTOMAKE) --gnu  docs/user_manual/Makefile
Makefile: # $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) && \
	  CONFIG_HEADERS= CONFIG_LINKS= \
	  CONFIG_FILES=$(subdir)/$@ $(SHELL) ./config.status
uninstall-info-am:
tags: TAGS
TAGS:


DISTFILES = $(DIST_COMMON) $(DIST_SOURCES) $(TEXINFOS) $(EXTRA_DIST)

top_distdir = ../..
distdir = $(top_distdir)/$(PACKAGE)-$(VERSION)

distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  if test -f $$file; then d=.; else d=$(srcdir); fi; \
	  dir=`echo "$$file" | sed -e 's,/[^/]*$$,,'`; \
	  if test "$$dir" != "$$file" && test "$$dir" != "."; then \
	    $(mkinstalldirs) "$(distdir)/$$dir"; \
	  fi; \
	  if test -d $$d/$$file; then \
	    cp -pR $$d/$$file $(distdir) \
	    || exit 1; \
	  else \
	    test -f $(distdir)/$$file \
	    || cp -p $$d/$$file $(distdir)/$$file \
	    || exit 1; \
	  fi; \
	done
check-am: all-am
check: check-am
all-am: Makefile

installdirs:

install: install-am
install-exec: install-exec-am
install-data: install-data-am
uninstall: uninstall-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am

installcheck: installcheck-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) INSTALL_PROGRAM="$(INSTALL_STRIP_PROGRAM)" \
	  `test -z '$(STRIP)' || \
	    echo "INSTALL_PROGRAM_ENV=STRIPPROG='$(STRIP)'"` install
mostlyclean-generic:

clean-generic:
	-test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES) stamp-h stamp-h[0-9]*

maintainer-clean-generic:
	@echo "This command is intended for maintainers to use"
	@echo "it deletes files that may require special tools to rebuild."
clean: clean-am

clean-am: clean-generic mostlyclean-am

distclean: distclean-am

distclean-am: clean-am distclean-generic

dvi: dvi-am

dvi-am:

info: info-am

info-am:

install-data-am:

install-exec-am:

install-info: install-info-am

install-man:

installcheck-am:

maintainer-clean: maintainer-clean-am

maintainer-clean-am: distclean-am maintainer-clean-generic

mostlyclean: mostlyclean-am

mostlyclean-am: mostlyclean-generic

uninstall-am: uninstall-info-am

.PHONY: all all-am check check-am clean clean-generic distclean \
	distclean-generic distdir dvi dvi-am info info-am install \
	install-am install-data install-data-am install-exec \
	install-exec-am install-info install-info-am install-man \
	install-strip installcheck installcheck-am installdirs \
	maintainer-clean maintainer-clean-generic mostlyclean \
	mostlyclean-generic uninstall uninstall-am uninstall-info-am


all: docs
docs: $(DOCS) $(TARGETS)

$(DOCS): %: %.dvi 

#
# If any of the *.bib *.tex or figures files is newer,
#  then run latex twice on the master *.tex file.
# If there is *.lof *.lot or *.toc file, then run latex three times instead.
# Note that we don't check whether the *.lof *.lot *.toc files are newer
#  than the *.dvi file because in practice the timestamps of those files
#  are same as the timestamp of the *.dvi file, and therefore the *.dvi file
#  is always regenerated.
#
$(DOCS:%=%.dvi): %.dvi: %_FIGURES %_BBL %_LATEX
	@is_newer=NO; \
	dep_files="$($(subst -,_,$*)_LATEX) $*.bbl $($(subst -,_,$*)_FIGURES)"; \
	for i in $$dep_files ; do \
		if [ $$i = $*.bbl -a ! -f $$i ] ; then continue; fi; \
		if [ ! -f $$i ] ; then \
			echo ERROR: cannot build $@: missing dependency file $$i; \
			exit 1; \
		fi; \
		if [ ! $$i -ot $@ ] ; then \
			is_newer="YES"; \
		fi; \
	done; \
	if [ $$is_newer = "YES" ] ; then \
		for i in $$dep_files ; do \
			file_prefix=`echo $$i | sed 's/\.tex$$//'`; \
			if [ -z $$file_prefix ] ; then continue; fi; \
			if [ $$file_prefix = $$i ] ; then continue; fi; \
			if [ $$file_prefix != $* ] ; then continue; fi; \
			(TEXINPUTS="${TEXINPUTS_PATH}"; export TEXINPUTS; latex $$i); \
			(TEXINPUTS="${TEXINPUTS_PATH}"; export TEXINPUTS; latex $$i); \
			if [ -f $*.lof -o -f $*.lot -o -f $*.toc ] ; then \
				(TEXINPUTS="${TEXINPUTS_PATH}"; export TEXINPUTS; latex $$i); \
			fi; \
		done; \
	fi;

#
# If any of the *.bib or *.tex files is newer than the *.bbl file,
#  then run latex and bibtex on the master *.tex file.
# Note that we need to run bibtex on the filename with stripped .tex suffix.
#
$(DOCS_BBL): %_BBL: %_LATEX
	@bbl_filename=$*.bbl; \
	is_newer=NO; \
	has_bib=NO; \
	dep_files="$($(subst -,_,$<))"; \
	for i in $$dep_files ; do \
		if [ ! $$i -ot $$bbl_filename ] ; then \
			is_newer="YES"; \
		fi; \
		file_prefix=`echo $$i | sed 's/\.bib$$//'`; \
		if [ -z $$file_prefix ] ; then continue; fi; \
		if [ $$file_prefix != $$i ] ; then \
			has_bib=YES; \
		fi; \
	done; \
	if [ $$is_newer = "YES" -a $$has_bib = "YES" ] ; then \
		for i in $$dep_files ; do \
			file_prefix=`echo $$i | sed 's/\.tex$$//'`; \
			if [ -z $$file_prefix ] ; then continue; fi; \
			if [ $$file_prefix = $$i ] ; then continue; fi; \
			if [ $$file_prefix != $* ] ; then continue; fi; \
			(TEXINPUTS="${TEXINPUTS_PATH}"; export TEXINPUTS; latex $$i); \
			bibtex $$file_prefix; \
		done; \
	fi;

#
# If any of the *.fig files is newer than the corresponding *.ps or *.eps,
# then run fig2dev to (re)create that file.
#
$(DOCS_FIGURES):
	@dep_files="$($(subst -,_,$@))"; \
	for i in $$dep_files ; do \
		file_prefix=`echo $$i | sed 's/\.eps$$//'`; \
		if [ -z $$file_prefix ] ; then continue; fi; \
		if [ $$file_prefix = $$i ] ; then continue; fi; \
		fig_src=$$file_prefix.fig; \
		if [ -f $$fig_src ] ; then \
			if [ ! $$fig_src -ot $$i ] ; then \
				fig2dev -Leps $$fig_src | $(PSANON) > $$i; \
			fi; \
		fi; \
	done; \
	for i in $$dep_files ; do \
		file_prefix=`echo $$i | sed 's/\.ps$$//'`; \
		if [ -z $$file_prefix ] ; then continue; fi; \
		if [ $$file_prefix = $$i ] ; then continue; fi; \
		fig_src=$$file_prefix.fig; \
		if [ -f $$fig_src ] ; then \
			if [ ! $$fig_src -ot $$i ] ; then \
				fig2dev -z Letter -Lps $$fig_src | $(PSANON) >$$i; \
			fi; \
		fi; \
	done

$(DOCS_LATEX):
	@true
.tex.dvi:
	(TEXINPUTS="${TEXINPUTS_PATH}"; export TEXINPUTS; latex $< $@)
	(TEXINPUTS="${TEXINPUTS_PATH}"; export TEXINPUTS; latex $< $@)
.tex.bbl:
	(TEXINPUTS="${TEXINPUTS_PATH}"; export TEXINPUTS; latex $< $@)
	bibtex $*
.dvi.ps:
	dvips $< -o $@
.ps.pdf:
	ps2pdf $< $@
.fig.ps:
	fig2dev -z Letter -Lps $< | $(PSANON) > $@
.fig.eps:
	fig2dev -Leps $< | $(PSANON) > $@

################################################################
# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
