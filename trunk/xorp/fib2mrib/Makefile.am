## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/fib2mrib/Makefile.am,v 1.1 2004/02/18 00:10:56 pavlin Exp $
##

# -- Real Programs
bin_PROGRAMS	 = xorp_fib2mrib

# -- Test Programs and Scripts
TESTS		 = 

test_scripts	 = test_add_route.sh

# -- No-install Programs and Scripts
noinst_PROGRAMS  = 
noinst_SCRIPTS	 = $(test_scripts)

# -- No-install Libraries
noinst_LIBRARIES = libfib2mrib.a

# -- Libraries and library flags to link the binary programs
LDFLAGS		 = 
LDADD		 = $(noinst_LIBRARIES)
### XRL libraries
LDADD		+= $(top_builddir)/xrl/targets/libfib2mribbase.a
### XORP misc libraries
LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.a
LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libfeafibxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.a
LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.a
LDADD		+= $(top_builddir)/libxipc/libxipc.a
LDADD		+= $(top_builddir)/libcomm/libcomm.a
LDADD		+= $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
xorp_fib2mrib_SOURCES		 = xorp_fib2mrib.cc

# -- Library Sources
libfib2mrib_a_SOURCES		 = fib2mrib_module.h
libfib2mrib_a_SOURCES		+= fib2mrib_node.hh
libfib2mrib_a_SOURCES		+= xrl_fib2mrib_node.hh

libfib2mrib_a_SOURCES		+= fib2mrib_node.cc
libfib2mrib_a_SOURCES		+= xrl_fib2mrib_node.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = 


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
