## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/static_routes/Makefile.am,v 1.5 2004/04/15 13:17:36 pavlin Exp $
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
noinst_LIBRARIES = libstaticroutes.a

# -- Libraries and library flags to link the binary programs
LDFLAGS		 = 
LDADD		 = $(noinst_LIBRARIES)
### XRL libraries
LDADD		+= $(top_builddir)/xrl/targets/libstaticroutesbase.a
### XORP misc libraries
LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.a
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.a
LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.a
LDADD		+= $(top_builddir)/libxipc/libxipc.a
LDADD		+= $(top_builddir)/libcomm/libcomm.a
LDADD		+= $(top_builddir)/libxorp/libxorp.a
LDADD           += $(top_builddir)/policy/backend/libpolicybackend.a

# -- Program Sources
xorp_static_routes_SOURCES = xorp_static_routes.cc

# -- Library Sources
libstaticroutes_a_SOURCES	 = static_routes_module.h
libstaticroutes_a_SOURCES	+= static_routes_node.hh
libstaticroutes_a_SOURCES       += static_routes_varrw.hh
libstaticroutes_a_SOURCES	+= xrl_static_routes_node.hh

libstaticroutes_a_SOURCES	+= static_routes_node.cc
libstaticroutes_a_SOURCES       += static_routes_varrw.cc
libstaticroutes_a_SOURCES	+= xrl_static_routes_node.cc

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
