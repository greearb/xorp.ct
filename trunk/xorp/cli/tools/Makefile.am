## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/pim/Makefile.am,v 1.19 2004/05/04 00:14:46 pavlin Exp $
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
send_cli_processor_xrl_LDADD	 = $(top_builddir)/xrl/interfaces/libcliprocessorxif.a
send_cli_processor_xrl_LDADD	+= $(top_builddir)/libxipc/libxipc.a
send_cli_processor_xrl_LDADD	+= $(top_builddir)/libcomm/libcomm.a
send_cli_processor_xrl_LDADD	+= $(top_builddir)/libxorp/libxorp.a

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

