## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/fib2mrib/Makefile.am,v 1.5 2004/06/02 18:28:16 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = xorp_fib2mrib

# -- Test Programs and Scripts
TESTS		 = 

test_scripts	 = test_add_route.sh

# -- No-install Programs and Scripts
noinst_PROGRAMS  = 
noinst_SCRIPTS	 = $(test_scripts)

# -- No-install Libraries
noinst_LTLIBRARIES = libfib2mrib.la

# -- Libraries and library flags to link the binary programs
LDFLAGS		 = 
LDADD		 = $(noinst_LTLIBRARIES)
### XRL libraries
LDADD		+= $(top_builddir)/xrl/targets/libfib2mribbase.la
### XORP misc libraries
LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libftixif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeafibxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.la
LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
xorp_fib2mrib_SOURCES		 = xorp_fib2mrib.cc

# -- Library Sources
libfib2mrib_la_SOURCES		 = fib2mrib_module.h
libfib2mrib_la_SOURCES		+= fib2mrib_node.hh
libfib2mrib_la_SOURCES		+= xrl_fib2mrib_node.hh

libfib2mrib_la_SOURCES		+= fib2mrib_node.cc
libfib2mrib_la_SOURCES		+= xrl_fib2mrib_node.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = command_fib2mrib


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

# Test scripts dependency
test_add_route.sh: $(top_builddir)/fib2mrib/xrl_fib2mrib_shell_funcs.sh

# XRL wrapper scripts dependency
$(top_builddir)/fib2mrib/xrl_fib2mrib_shell_funcs.sh:			\
    $(top_srcdir)/xrl/targets/fib2mrib.xrls				\
    $(top_srcdir)/utils/xrl_wrappers_generator.sh
	$(top_srcdir)/utils/xrl_wrappers_generator.sh $(top_srcdir)/xrl/targets/fib2mrib.xrls > $(top_builddir)/fib2mrib/xrl_fib2mrib_shell_funcs.sh
