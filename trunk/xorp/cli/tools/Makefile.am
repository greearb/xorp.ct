## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/cli/tools/Makefile.am,v 1.1 2004/05/05 05:10:24 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = send_cli_processor_xrl

# -- Test Programs
TESTS		 =

# -- No-install Programs
noinst_PROGRAMS  =

# -- Libraries and library flags to link the binary programs
send_cli_processor_xrl_LDADD	 = $(top_builddir)/xrl/interfaces/libcliprocessorxif.la
send_cli_processor_xrl_LDADD	+= $(top_builddir)/libxipc/libxipc.la
send_cli_processor_xrl_LDADD	+= $(top_builddir)/libcomm/libcomm.la
send_cli_processor_xrl_LDADD	+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
send_cli_processor_xrl_SOURCES	 = send_cli_processor_xrl.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = 


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

