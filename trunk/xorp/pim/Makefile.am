## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/pim/Makefile.am,v 1.24 2004/07/26 02:57:24 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Real Programs
bin_PROGRAMS	 = xorp_pimsm4 xorp_pimsm6

# -- Test Programs
TESTS		 =

# -- No-install Programs
noinst_PROGRAMS  = test_pim

# -- No-install Libraries
noinst_LIBRARIES = libpim.a

# -- Libraries and library flags to link the binary programs
### PIM libraries
LDADD		 = $(noinst_LIBRARIES)
#LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.a
#LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaxif.a
#LDADD		+= $(top_builddir)/xrl/interfaces/libmld6igmpclientxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libmld6igmpxif.a
LDADD		+= $(top_builddir)/xrl/targets/libpimbase.a
### MLD6IGMP libraries
LDADD		+= $(top_builddir)/mld6igmp/libmld6igmp.a
#LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libmld6igmpclientxif.a
LDADD		+= $(top_builddir)/xrl/targets/libmld6igmpbase.a
### RIB libraries
LDADD		+= $(top_builddir)/rib/librib.a
LDADD		+= $(top_builddir)/xrl/interfaces/libftixif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libribclientxif.a
LDADD		+= $(top_builddir)/xrl/targets/libribbase.a
LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.a
### MFEA libraries
LDADD		+= $(top_builddir)/fea/libfea.a
# FEA-specific begin
LDADD		+= $(top_builddir)/xrl/interfaces/libfeafibclientxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrclientxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpktclient.a
LDADD		+= $(top_builddir)/xrl/targets/libfeabase.a
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrxif.a
# FEA-specific end
LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaclientxif.a
LDADD		+= $(top_builddir)/xrl/targets/libmfeabase.a
### MRT libraries
LDADD		+= $(top_builddir)/mrt/libmrt.a
### CLI libraries
LDADD		+= $(top_builddir)/cli/libcli.a
LDADD		+= $(top_builddir)/cli/libtecla/libtecla.a
LDADD		+= @CLI_CURSES_LIB@
LDADD		+= $(top_builddir)/xrl/interfaces/libcliprocessorxif.a
LDADD		+= $(top_builddir)/xrl/targets/libclibase.a
### XORP misc libraries
LDADD		+= $(top_builddir)/libproto/libproto.a
LDADD		+= $(top_builddir)/libxipc/libfinder.a
LDADD		+= $(top_builddir)/libxipc/libxipc.a
LDADD		+= $(top_builddir)/libcomm/libcomm.a
LDADD		+= $(top_builddir)/libxorp/libxorp.a

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
test_pim_LDADD	+= $(top_builddir)/policy/backend/libpolicybackend.a
test_pim_LDADD	+= $(top_builddir)/libfeaclient/libfeaclient.a
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.a
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.a
test_pim_LDADD	+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.a

test_pim_LDADD	+= $(top_builddir)/fib2mrib/libfib2mrib.a
test_pim_LDADD	+= $(top_builddir)/xrl/targets/libfib2mribbase.a
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libftixif.a
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libfeafibxif.a

test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket4userxif.a
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket4xif.a
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libredist4xif.a
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libredisttransaction4xif.a
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket6userxif.a
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket6xif.a
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libredist6xif.a
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libpolicyredist4xif.a
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libpolicyredist6xif.a
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libredisttransaction6xif.a
test_pim_LDADD	+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.a
test_pim_LDADD	+= $(top_builddir)/xrl/targets/libsocketserverbase.a
#test_pim_LDADD	+= $(top_builddir)/libxorp/libxorp.a

# -- Library Sources
libpim_a_SOURCES  = pim_module.h
libpim_a_SOURCES += pim_proto.h

libpim_a_SOURCES += pim_bsr.hh
libpim_a_SOURCES += pim_mfc.hh
libpim_a_SOURCES += pim_mre.hh
libpim_a_SOURCES += pim_mre_task.hh
libpim_a_SOURCES += pim_mre_track_state.hh
libpim_a_SOURCES += pim_mrib_table.hh
libpim_a_SOURCES += pim_mrt.hh
libpim_a_SOURCES += pim_nbr.hh
libpim_a_SOURCES += pim_node.hh
libpim_a_SOURCES += pim_node_cli.hh
libpim_a_SOURCES += pim_private.hh
libpim_a_SOURCES += pim_proto_assert.hh
libpim_a_SOURCES += pim_proto_join_prune_message.hh
libpim_a_SOURCES += pim_rp.hh
libpim_a_SOURCES += pim_scope_zone_table.hh
libpim_a_SOURCES += pim_vif.hh
libpim_a_SOURCES += xrl_pim_node.hh

libpim_a_SOURCES += pim_bsr.cc
libpim_a_SOURCES += pim_config.cc
libpim_a_SOURCES += pim_mfc.cc
libpim_a_SOURCES += pim_mre.cc
libpim_a_SOURCES += pim_mre_assert.cc
libpim_a_SOURCES += pim_mre_data.cc
libpim_a_SOURCES += pim_mre_join_prune.cc
libpim_a_SOURCES += pim_mre_register.cc
libpim_a_SOURCES += pim_mre_rpf.cc
libpim_a_SOURCES += pim_mre_task.cc
libpim_a_SOURCES += pim_mre_track_state.cc
libpim_a_SOURCES += pim_mrib_table.cc
libpim_a_SOURCES += pim_mrt.cc
libpim_a_SOURCES += pim_mrt_mfc.cc
libpim_a_SOURCES += pim_mrt_task.cc
libpim_a_SOURCES += pim_nbr.cc
libpim_a_SOURCES += pim_node.cc
libpim_a_SOURCES += pim_node_cli.cc
libpim_a_SOURCES += pim_proto_assert.cc
libpim_a_SOURCES += pim_proto_bootstrap.cc
libpim_a_SOURCES += pim_proto_cand_rp_adv.cc
libpim_a_SOURCES += pim_proto_graft.cc
libpim_a_SOURCES += pim_proto_graft_ack.cc
libpim_a_SOURCES += pim_proto_hello.cc
libpim_a_SOURCES += pim_proto_join_prune.cc
libpim_a_SOURCES += pim_proto_join_prune_message.cc
libpim_a_SOURCES += pim_proto_register.cc
libpim_a_SOURCES += pim_proto_register_stop.cc
libpim_a_SOURCES += pim_rp.cc
libpim_a_SOURCES += pim_scope_zone_table.cc
libpim_a_SOURCES += pim_vif.cc
libpim_a_SOURCES += xrl_pim_node.cc

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

