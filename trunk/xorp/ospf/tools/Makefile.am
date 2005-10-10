## Process this file with automake to produce Makefile.in.
##
## $XORP$
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = print_lsas

# -- Test Programs
TESTS		 =

# -- No-install Programs
noinst_PROGRAMS  =

# -- No-install Libtool Libraries
noinst_LTLIBRARIES =

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LTLIBRARIES)
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la
LDADD		+= $(top_builddir)/xrl/interfaces/libospfv2xif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libospfv3xif.la

# -- Program Sources
print_lsas_SOURCES  = print_lsas.cc
print_lsas_LDADD = $(LDADD)
print_lsas_LDADD += $(top_builddir)/ospf/libospf.la

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

