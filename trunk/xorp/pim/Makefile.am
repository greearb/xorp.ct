## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/pim/Makefile.am,v 1.29 2004/11/23 22:43:46 bms Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = xorp_pimsm4 xorp_pimsm6

# -- Test Programs
TESTS		 =

# -- No-install Programs
check_PROGRAMS  = test_pim

# -- No-install Libraries
noinst_LTLIBRARIES = libpim.la

# -- Libraries and library flags to link the binary programs
### PIM libraries
LDADD		 = $(noinst_LTLIBRARIES)
#LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.la
#LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaxif.la
#LDADD		+= $(top_builddir)/xrl/interfaces/libmld6igmpclientxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libmld6igmpxif.la
LDADD		+= $(top_builddir)/xrl/targets/libpimbase.la
### MLD6IGMP libraries
LDADD		+= $(top_builddir)/mld6igmp/libmld6igmp.la
#LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libmld6igmpclientxif.la
LDADD		+= $(top_builddir)/xrl/targets/libmld6igmpbase.la
### RIB libraries
LDADD		+= $(top_builddir)/rib/librib.la
LDADD		+= $(top_builddir)/xrl/interfaces/libftixif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libribclientxif.la
LDADD		+= $(top_builddir)/xrl/targets/libribbase.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libprofileclientxif.la
### MFEA libraries
LDADD		+= $(top_builddir)/fea/libfea.la
# FEA-specific begin
LDADD		+= $(top_builddir)/xrl/interfaces/libfeafibclientxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrclientxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpkt4client.la
LDADD		+= $(top_builddir)/xrl/targets/libfeabase.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrxif.la
# FEA-specific end
LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaclientxif.la
LDADD		+= $(top_builddir)/xrl/targets/libmfeabase.la
### MRT libraries
LDADD		+= $(top_builddir)/mrt/libmrt.la
### CLI libraries
LDADD		+= $(top_builddir)/cli/libcli.la
LDADD		+= $(top_builddir)/cli/libtecla/libtecla.a
LDADD		+= @CLI_CURSES_LIB@
LDADD		+= $(top_builddir)/xrl/interfaces/libcliprocessorxif.la
LDADD		+= $(top_builddir)/xrl/targets/libclibase.la
### XRL client for Profile
LDADD		+= $(top_builddir)/xrl/interfaces/libprofileclientxif.la
### XORP misc libraries
LDADD		+= $(top_builddir)/libproto/libproto.la
LDADD		+= $(top_builddir)/libxipc/libfinder.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
xorp_pimsm4_SOURCES  = xorp_pimsm4.cc
xorp_pimsm6_SOURCES  = xorp_pimsm6.cc

test_pim_SOURCES  = test_pim.cc
test_pim_SOURCES += $(top_builddir)/rib/rib_manager.hh
test_pim_SOURCES += $(top_builddir)/rib/rib_manager.cc
test_pim_SOURCES += $(top_builddir)/rib/xrl_target.hh
test_pim_SOURCES += $(top_builddir)/rib/xrl_target.cc
test_pim_SOURCES += $(top_builddir)/rib/redist_xrl.hh
test_pim_SOURCES += $(top_builddir)/rib/redist_xrl.cc

test_pim_LDADD	 = $(LDADD)
test_pim_LDADD	+= $(top_builddir)/policy/backend/libpolicybackend.la
test_pim_LDADD	+= $(top_builddir)/libfeaclient/libfeaclient.la
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
test_pim_LDADD	+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la

test_pim_LDADD	+= $(top_builddir)/fib2mrib/libfib2mrib.la
test_pim_LDADD	+= $(top_builddir)/xrl/targets/libfib2mribbase.la
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libftixif.la
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libfeafibxif.la

test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket4userxif.la
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket4xif.la
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libredist4xif.la
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libredisttransaction4xif.la
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket6userxif.la
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket6xif.la
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libredist6xif.la
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libpolicyredist4xif.la
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libpolicyredist6xif.la
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libredisttransaction6xif.la
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
test_pim_LDADD	+= $(top_builddir)/xrl/targets/libsocketserverbase.la
#test_pim_LDADD	+= $(top_builddir)/libxorp/libxorp.la

# -- Library Sources
libpim_la_SOURCES  = pim_module.h
libpim_la_SOURCES += pim_proto.h

libpim_la_SOURCES += pim_bsr.hh
libpim_la_SOURCES += pim_mfc.hh
libpim_la_SOURCES += pim_mre.hh
libpim_la_SOURCES += pim_mre_task.hh
libpim_la_SOURCES += pim_mre_track_state.hh
libpim_la_SOURCES += pim_mrib_table.hh
libpim_la_SOURCES += pim_mrt.hh
libpim_la_SOURCES += pim_nbr.hh
libpim_la_SOURCES += pim_node.hh
libpim_la_SOURCES += pim_node_cli.hh
libpim_la_SOURCES += pim_private.hh
libpim_la_SOURCES += pim_proto_assert.hh
libpim_la_SOURCES += pim_proto_join_prune_message.hh
libpim_la_SOURCES += pim_rp.hh
libpim_la_SOURCES += pim_scope_zone_table.hh
libpim_la_SOURCES += pim_vif.hh
libpim_la_SOURCES += xrl_pim_node.hh

libpim_la_SOURCES += pim_bsr.cc
libpim_la_SOURCES += pim_config.cc
libpim_la_SOURCES += pim_mfc.cc
libpim_la_SOURCES += pim_mre.cc
libpim_la_SOURCES += pim_mre_assert.cc
libpim_la_SOURCES += pim_mre_data.cc
libpim_la_SOURCES += pim_mre_join_prune.cc
libpim_la_SOURCES += pim_mre_register.cc
libpim_la_SOURCES += pim_mre_rpf.cc
libpim_la_SOURCES += pim_mre_task.cc
libpim_la_SOURCES += pim_mre_track_state.cc
libpim_la_SOURCES += pim_mrib_table.cc
libpim_la_SOURCES += pim_mrt.cc
libpim_la_SOURCES += pim_mrt_mfc.cc
libpim_la_SOURCES += pim_mrt_task.cc
libpim_la_SOURCES += pim_nbr.cc
libpim_la_SOURCES += pim_node.cc
libpim_la_SOURCES += pim_node_cli.cc
libpim_la_SOURCES += pim_proto_assert.cc
libpim_la_SOURCES += pim_proto_bootstrap.cc
libpim_la_SOURCES += pim_proto_cand_rp_adv.cc
libpim_la_SOURCES += pim_proto_graft.cc
libpim_la_SOURCES += pim_proto_graft_ack.cc
libpim_la_SOURCES += pim_proto_hello.cc
libpim_la_SOURCES += pim_proto_join_prune.cc
libpim_la_SOURCES += pim_proto_join_prune_message.cc
libpim_la_SOURCES += pim_proto_register.cc
libpim_la_SOURCES += pim_proto_register_stop.cc
libpim_la_SOURCES += pim_rp.cc
libpim_la_SOURCES += pim_scope_zone_table.cc
libpim_la_SOURCES += pim_vif.cc
libpim_la_SOURCES += xrl_pim_node.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = DEVNOTES
EXTRA_DIST	 += command_pim
EXTRA_DIST	 += configure_pim
EXTRA_DIST	 += docs
EXTRA_DIST	 += xrl_pim_shell_funcs.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

