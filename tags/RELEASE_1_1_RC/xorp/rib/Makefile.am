## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/rib/Makefile.am,v 1.37 2004/11/12 02:44:47 bms Exp $
##

SUBDIRS = . tools

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = xorp_rib

# -- Test Programs
TESTS		 = test_rib_direct.sh
TESTS		+= test_rib_xrls.sh
TESTS		+= test_redist
TESTS		+= test_register
TESTS		+= test_deletion

# -- No-install Programs
noinst_PROGRAMS	 = add_route
noinst_PROGRAMS	+= main_routemap

check_PROGRAMS	 = test_deletion
check_PROGRAMS	+= test_redist
check_PROGRAMS	+= test_register
check_PROGRAMS	+= test_register_xrls
check_PROGRAMS	+= test_rib_direct
check_PROGRAMS	+= test_rib_xrls

# -- No-install Libraries
noinst_LTLIBRARIES	 = librib.la libribxrl.la

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LTLIBRARIES)
LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libftixif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libribclientxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libredist4xif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libredist6xif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libredisttransaction4xif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libredisttransaction6xif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libpolicyredist4xif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libpolicyredist6xif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libprofileclientxif.la
LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
LDADD		+= $(top_builddir)/xrl/targets/libribbase.la
LDADD		+= $(top_builddir)/libxipc/libfinder.la
LDADD		+= $(top_builddir)/policy/backend/libpolicybackend.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libproto/libproto.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la


# -- Program Sources
xorp_rib_SOURCES		 = rib_module.h
xorp_rib_SOURCES		+= protocol.hh
xorp_rib_SOURCES		+= register_server.hh
xorp_rib_SOURCES		+= rib.hh
xorp_rib_SOURCES		+= rib_manager.hh
xorp_rib_SOURCES		+= rib_varrw.hh
xorp_rib_SOURCES		+= route.hh
xorp_rib_SOURCES		+= rt_tab_base.hh
xorp_rib_SOURCES		+= rt_tab_extint.hh
xorp_rib_SOURCES		+= rt_tab_merged.hh
xorp_rib_SOURCES		+= rt_tab_origin.hh
xorp_rib_SOURCES		+= rt_tab_pol_conn.hh
xorp_rib_SOURCES		+= rt_tab_pol_redist.hh
xorp_rib_SOURCES		+= rt_tab_redist.hh
xorp_rib_SOURCES		+= rt_tab_register.hh
xorp_rib_SOURCES		+= vifmanager.hh
xorp_rib_SOURCES		+= xrl_target.hh
xorp_rib_SOURCES		+= main_rib.cc
xorp_rib_LDADD			 = $(LDADD)

add_route_SOURCES		 = add_route.cc
add_route_LDADD			 = $(LDADD)
add_route_LDADD			+= $(top_builddir)/xrl/interfaces/libribxif.la

main_routemap_SOURCES		 = rib_module.h
main_routemap_SOURCES		+= protocol.hh
main_routemap_SOURCES		+= route.hh
main_routemap_SOURCES		+= routemap.hh
main_routemap_SOURCES		+= main_routemap.cc
main_routemap_SOURCES		+= routemap.cc
main_routemap_LDADD		 = $(LDADD)

test_deletion_SOURCES		 = rib_module.h
test_deletion_SOURCES		+= rt_tab_base.hh
test_deletion_SOURCES		+= rt_tab_expect.hh
test_deletion_SOURCES		+= rt_tab_origin.hh
test_deletion_SOURCES		+= protocol.hh
test_deletion_SOURCES		+= route.hh
test_deletion_SOURCES		+= rt_tab_expect.cc
test_deletion_SOURCES		+= test_deletion.cc
test_deletion_LDADD		 = $(LDADD)

test_redist_SOURCES		 = test_redist.cc
test_redist_LDADD		 = ./librib.la
test_redist_LDADD		+= $(top_builddir)/policy/backend/libpolicybackend.la
test_redist_LDADD		+= $(top_builddir)/libxipc/libxipc.la
test_redist_LDADD		+= $(top_builddir)/libxorp/libxorp.la

test_register_SOURCES		 = rib_module.h
test_register_SOURCES		+= dummy_register_server.hh
test_register_SOURCES		+= protocol.hh
test_register_SOURCES		+= register_server.hh
test_register_SOURCES		+= rib.hh
test_register_SOURCES		+= rib_manager.hh
test_register_SOURCES		+= route.hh
test_register_SOURCES		+= rt_tab_base.hh
test_register_SOURCES		+= rt_tab_extint.hh
test_register_SOURCES		+= rt_tab_merged.hh
test_register_SOURCES		+= rt_tab_origin.hh
test_register_SOURCES		+= rt_tab_redist.hh
test_register_SOURCES		+= rt_tab_register.hh
test_register_SOURCES		+= vifmanager.hh
test_register_SOURCES		+= xrl_target.hh
test_register_SOURCES		+= dummy_register_server.cc
test_register_SOURCES		+= test_register.cc
test_register_LDADD		 = $(LDADD)

test_register_xrls_SOURCES	 = rib_module.h
test_register_xrls_SOURCES	+= parser.hh
test_register_xrls_SOURCES	+= parser_direct_cmds.hh
test_register_xrls_SOURCES	+= parser_xrl_cmds.hh
test_register_xrls_SOURCES	+= protocol.hh
test_register_xrls_SOURCES	+= register_server.hh
test_register_xrls_SOURCES	+= rib.hh
test_register_xrls_SOURCES	+= rib_manager.hh
test_register_xrls_SOURCES	+= route.hh
test_register_xrls_SOURCES	+= rt_tab_base.hh
test_register_xrls_SOURCES	+= rt_tab_extint.hh
test_register_xrls_SOURCES	+= rt_tab_merged.hh
test_register_xrls_SOURCES	+= rt_tab_origin.hh
test_register_xrls_SOURCES	+= rt_tab_redist.hh
test_register_xrls_SOURCES	+= rt_tab_register.hh
test_register_xrls_SOURCES	+= vifmanager.hh
test_register_xrls_SOURCES	+= xrl_target.hh
test_register_xrls_SOURCES	+= test_register_xrls.cc
test_register_xrls_LDADD	 = $(LDADD)
test_register_xrls_LDADD	+= $(top_builddir)/xrl/interfaces/libribxif.la
test_register_xrls_LDADD	+= $(top_builddir)/xrl/targets/libribclientbase.la

test_rib_direct_SOURCES		 = rib_module.h
test_rib_direct_SOURCES		+= dummy_register_server.hh
test_rib_direct_SOURCES		+= parser.hh
test_rib_direct_SOURCES		+= parser_direct_cmds.hh
test_rib_direct_SOURCES		+= protocol.hh
test_rib_direct_SOURCES		+= register_server.hh
test_rib_direct_SOURCES		+= rib.hh
test_rib_direct_SOURCES		+= rib_manager.hh
test_rib_direct_SOURCES		+= route.hh
test_rib_direct_SOURCES		+= rt_tab_base.hh
test_rib_direct_SOURCES		+= rt_tab_extint.hh
test_rib_direct_SOURCES		+= rt_tab_merged.hh
test_rib_direct_SOURCES		+= rt_tab_origin.hh
test_rib_direct_SOURCES		+= rt_tab_redist.hh
test_rib_direct_SOURCES		+= rt_tab_register.hh
test_rib_direct_SOURCES		+= vifmanager.hh
test_rib_direct_SOURCES		+= xrl_target.hh
test_rib_direct_SOURCES		+= dummy_register_server.cc
test_rib_direct_SOURCES		+= parser.cc
test_rib_direct_SOURCES		+= test_rib_direct.cc
test_rib_direct_LDADD		 = $(LDADD)

test_rib_xrls_SOURCES		 = rib_module.h
test_rib_xrls_SOURCES		+= dummy_register_server.hh
test_rib_xrls_SOURCES		+= parser.hh
test_rib_xrls_SOURCES		+= parser_direct_cmds.hh
test_rib_xrls_SOURCES		+= parser_xrl_cmds.hh
test_rib_xrls_SOURCES		+= protocol.hh
test_rib_xrls_SOURCES		+= register_server.hh
test_rib_xrls_SOURCES		+= rib.hh
test_rib_xrls_SOURCES		+= rib_manager.hh
test_rib_xrls_SOURCES		+= route.hh
test_rib_xrls_SOURCES		+= rt_tab_base.hh
test_rib_xrls_SOURCES		+= rt_tab_extint.hh
test_rib_xrls_SOURCES		+= rt_tab_merged.hh
test_rib_xrls_SOURCES		+= rt_tab_origin.hh
test_rib_xrls_SOURCES		+= rt_tab_redist.hh
test_rib_xrls_SOURCES		+= rt_tab_register.hh
test_rib_xrls_SOURCES		+= vifmanager.hh
test_rib_xrls_SOURCES		+= xrl_target.hh
test_rib_xrls_SOURCES		+= dummy_register_server.cc
test_rib_xrls_SOURCES		+= parser.cc
test_rib_xrls_SOURCES		+= test_rib_xrls.cc
test_rib_xrls_LDADD		 = $(LDADD)
test_rib_xrls_LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.la

# -- Library Sources
librib_la_SOURCES		 = rib_module.h
librib_la_SOURCES		+= protocol.hh
librib_la_SOURCES		+= register_server.hh
librib_la_SOURCES		+= rib.hh
librib_la_SOURCES		+= rib_manager.hh
librib_la_SOURCES		+= rib_varrw.hh
librib_la_SOURCES		+= route.hh
librib_la_SOURCES		+= rt_tab_base.hh
librib_la_SOURCES		+= rt_tab_deletion.hh
librib_la_SOURCES		+= rt_tab_extint.hh
librib_la_SOURCES		+= rt_tab_log.hh
librib_la_SOURCES		+= rt_tab_merged.hh
librib_la_SOURCES		+= rt_tab_origin.hh
librib_la_SOURCES		+= rt_tab_pol_conn.hh
librib_la_SOURCES		+= rt_tab_pol_redist.hh
librib_la_SOURCES		+= rt_tab_redist.hh
librib_la_SOURCES		+= rt_tab_register.hh
librib_la_SOURCES		+= vifmanager.hh
librib_la_SOURCES		+= xrl_target.hh
librib_la_SOURCES		+= profile_vars.hh
librib_la_SOURCES		+= profile_vars.cc
librib_la_SOURCES		+= protocol.cc
librib_la_SOURCES		+= register_server.cc
librib_la_SOURCES		+= rib.cc
librib_la_SOURCES		+= rib_manager.cc
librib_la_SOURCES		+= rib_varrw.cc
librib_la_SOURCES		+= rt_tab_base.cc
librib_la_SOURCES		+= rt_tab_deletion.cc
librib_la_SOURCES		+= rt_tab_extint.cc
librib_la_SOURCES		+= rt_tab_log.cc
librib_la_SOURCES		+= rt_tab_merged.cc
librib_la_SOURCES		+= rt_tab_origin.cc
librib_la_SOURCES		+= rt_tab_pol_conn.cc
librib_la_SOURCES		+= rt_tab_pol_redist.cc
librib_la_SOURCES		+= rt_tab_redist.cc
librib_la_SOURCES		+= rt_tab_register.cc
librib_la_SOURCES		+= vifmanager.cc
librib_la_SOURCES		+= xrl_target.cc

libribxrl_la_SOURCES		 = redist_xrl.hh
libribxrl_la_SOURCES		+= redist_xrl.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = commands
EXTRA_DIST	+= test_rib_direct.sh.in
EXTRA_DIST	+= test_rib_xrls.sh.in
EXTRA_DIST	+= xrl_shell_funcs.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

test_rib_direct.sh: test_rib_direct

test_rib_xrls.sh: test_rib_xrls
