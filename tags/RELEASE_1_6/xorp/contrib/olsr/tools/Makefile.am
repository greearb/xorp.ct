## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/contrib/olsr/tools/Makefile.am,v 1.1 2008/04/24 15:20:02 bms Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/contrib/olsr/tools

# Point preprocessor at relocated olsr module
AM_CPPFLAGS      = -I$(top_srcdir)/contrib -I$(top_srcdir)/contrib/olsr

# -- Real Programs
bin_PROGRAMS	 = clear_database print_databases

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
LDADD		+= $(top_builddir)/xrl/interfaces/libolsr4xif.la

# -- Program Sources
clear_database_SOURCES		= clear_database.cc
clear_database_LDADD		= $(LDADD)

print_databases_SOURCES		= print_databases.cc
print_databases_LDADD		= $(LDADD)

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

