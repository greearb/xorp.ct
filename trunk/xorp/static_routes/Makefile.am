## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/static_routes/Makefile.am,v 1.7 2004/09/17 19:44:16 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = xorp_static_routes

# -- Test Programs and Scripts
TESTS		 = 

test_scripts	 = test_add_route.sh

# -- No-install Programs and Scripts
noinst_PROGRAMS  = 
noinst_SCRIPTS	 = $(test_scripts)

# -- No-install Libraries
noinst_LTLIBRARIES = libstaticroutes.la

# -- Libraries and library flags to link the binary programs
LDFLAGS		 = 
LDADD		 = $(noinst_LTLIBRARIES)
### XRL libraries
LDADD		+= $(top_builddir)/xrl/targets/libstaticroutesbase.la
### XORP misc libraries
LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.la
LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la
LDADD		+= $(top_builddir)/policy/backend/libpolicybackend.la

# -- Program Sources
xorp_static_routes_SOURCES = xorp_static_routes.cc

# -- Library Sources
libstaticroutes_la_SOURCES	 = static_routes_module.h
libstaticroutes_la_SOURCES	+= static_routes_node.hh
libstaticroutes_la_SOURCES	+= static_routes_varrw.hh
libstaticroutes_la_SOURCES	+= xrl_static_routes_node.hh

libstaticroutes_la_SOURCES	+= static_routes_node.cc
libstaticroutes_la_SOURCES	+= static_routes_varrw.cc
libstaticroutes_la_SOURCES	+= xrl_static_routes_node.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = command_static_routes


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

# Test scripts dependency
test_add_route.sh: $(top_builddir)/static_routes/xrl_static_routes_shell_funcs.sh

# XRL wrapper scripts dependency
$(top_builddir)/static_routes/xrl_static_routes_shell_funcs.sh:		\
    $(top_srcdir)/xrl/targets/static_routes.xrls			\
    $(top_srcdir)/utils/xrl_wrappers_generator.sh
	$(top_srcdir)/utils/xrl_wrappers_generator.sh $(top_srcdir)/xrl/targets/static_routes.xrls > $(top_builddir)/static_routes/xrl_static_routes_shell_funcs.sh
