## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/cli/tools/Makefile.am,v 1.4 2008/10/09 17:51:24 abittau Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = cli_generic send_cli_processor_xrl

# -- Test Programs
TESTS		 =

# -- No-install Programs
noinst_PROGRAMS  =

# -- Libraries and library flags to link the binary programs

# -- Program Sources
send_cli_processor_xrl_SOURCES	 = send_cli_processor_xrl.cc
send_cli_processor_xrl_LDADD	 = $(top_builddir)/xrl/interfaces/libcliprocessorxif.la
send_cli_processor_xrl_LDADD	+= $(top_builddir)/libxipc/libxipc.la
send_cli_processor_xrl_LDADD	+= $(top_builddir)/libcomm/libcomm.la
send_cli_processor_xrl_LDADD	+= $(top_builddir)/libxorp/libxorp.la

cli_generic_SOURCES		 = cli_generic.cc
cli_generic_LDADD		 = $(top_builddir)/libxipc/libxipc.la
cli_generic_LDADD		+= $(top_builddir)/libcomm/libcomm.la
cli_generic_LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = 


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
