## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/rib/Makefile.am,v 1.8 2003/03/29 19:03:10 pavlin Exp $
##

# -- Real Programs
bin_PROGRAMS		 = rib main_routemap

# -- Test Programs
bin_PROGRAMS		+= test_rib_client
bin_PROGRAMS		+= test_register
bin_PROGRAMS		+= test_register_xrls
bin_PROGRAMS		+= test_rib_direct
bin_PROGRAMS		+= test_rib_xrls

# -- Test scripts
TESTS 			 = test_rib_direct.sh test_rib_xrls.sh test_register

noinst_LIBRARIES	 = librib.a

# -- Libraries and library flags to link the binary programs
LDADD			 = $(noinst_LIBRARIES)
LDADD			+= $(top_builddir)/xrl/interfaces/libftixif.a
LDADD			+= $(top_builddir)/libxipc/libfinder.a
LDADD			+= $(top_builddir)/libxipc/libxipc.a
LDADD			+= $(top_builddir)/libcomm/libcomm.a
LDADD			+= $(top_builddir)/libproto/libproto.a
LDADD			+= $(top_builddir)/libxorp/libxorp.a
LDADD			+= $(top_builddir)/xrl/interfaces/libfeaifmgrxif.a
LDADD			+= $(top_builddir)/xrl/interfaces/libribclientxif.a

# -- Program Sources
main_routemap_SOURCES	 = routemap.hh routemap.cc main_routemap.cc

rib_SOURCES		 = main_rib.cc
rib_LDADD		 = $(LDADD)
rib_LDADD		+= $(top_builddir)/xrl/targets/libribbase.a

test_rib_direct_SOURCES	 = parser.hh parser_direct_cmds.hh
test_rib_direct_SOURCES	+= parser.cc test_rib_direct.cc
test_rib_direct_SOURCES	+= dummy_register_server.hh
test_rib_direct_SOURCES	+= dummy_register_server.cc

test_rib_xrls_SOURCES	 = parser.hh parser_direct_cmds.hh parser_xrl_cmds.hh
test_rib_xrls_SOURCES	+= parser.cc test_rib_xrls.cc
test_rib_xrls_SOURCES	+= dummy_register_server.hh
test_rib_xrls_SOURCES	+= dummy_register_server.cc
test_rib_xrls_LDADD	 = $(LDADD)
test_rib_xrls_LDADD	+= $(top_builddir)/xrl/interfaces/libribxif.a
test_rib_xrls_LDADD	+= $(top_builddir)/xrl/targets/libribbase.a


test_register_SOURCES	 = test_register.cc
test_register_SOURCES	+= dummy_register_server.hh
test_register_SOURCES	+= dummy_register_server.cc

test_register_xrls_SOURCES  = parser_direct_cmds.hh parser_xrl_cmds.hh
test_register_xrls_SOURCES += test_register_xrls.cc
test_register_xrls_LDADD  = $(LDADD)
test_register_xrls_LDADD += $(top_builddir)/xrl/interfaces/libribxif.a
test_register_xrls_LDADD += $(top_builddir)/xrl/targets/libribbase.a
test_register_xrls_LDADD += $(top_builddir)/xrl/targets/libribclientbase.a

test_rib_client_SOURCES  = test_rib_client.cc

# -- Library Sources
librib_a_SOURCES  = rib_module.h
librib_a_SOURCES += version.h
librib_a_SOURCES += protocol.hh
librib_a_SOURCES += register_server.hh
librib_a_SOURCES += rib.hh
librib_a_SOURCES += rib_client.hh
librib_a_SOURCES += rib_manager.hh
librib_a_SOURCES += route.hh
librib_a_SOURCES += rt_tab_base.hh
librib_a_SOURCES += rt_tab_export.hh
librib_a_SOURCES += rt_tab_extint.hh
librib_a_SOURCES += rt_tab_merged.hh
librib_a_SOURCES += rt_tab_origin.hh
librib_a_SOURCES += rt_tab_redist.hh
librib_a_SOURCES += rt_tab_register.hh
librib_a_SOURCES += vifmanager.hh
librib_a_SOURCES += xrl_target.hh

librib_a_SOURCES += protocol.cc
librib_a_SOURCES += register_server.cc
librib_a_SOURCES += rib.cc
librib_a_SOURCES += rib_client.cc
librib_a_SOURCES += rib_manager.cc
librib_a_SOURCES += route.cc
librib_a_SOURCES += rt_tab_base.cc
librib_a_SOURCES += rt_tab_export.cc
librib_a_SOURCES += rt_tab_extint.cc
librib_a_SOURCES += rt_tab_merged.cc
librib_a_SOURCES += rt_tab_origin.cc
librib_a_SOURCES += rt_tab_redist.cc
librib_a_SOURCES += rt_tab_register.cc
librib_a_SOURCES += vifmanager.cc
librib_a_SOURCES += xrl_target.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = commands
EXTRA_DIST	 += test_rib_direct.sh
EXTRA_DIST	 += test_rib_xrls.sh
EXTRA_DIST	 += xrl_shell_funcs.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##
test_rib_direct.sh: test_rib_direct

test_rib_xrls.sh: test_rib_xrls
