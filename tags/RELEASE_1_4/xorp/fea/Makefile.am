## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/fea/Makefile.am,v 1.66 2006/06/29 19:10:56 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Subdirectories
SUBDIRS = . tools

# -- Real Programs
bin_PROGRAMS		 = xorp_fea xorp_fea_dummy
bin_SCRIPTS		 = xorp_fea_click_config_generator

# -- No-install Programs and Scripts
noinst_PROGRAMS 	 = demo_fea_ifmgr_client

check_PROGRAMS		 = test_ifmanager_transaction
check_PROGRAMS 		+= test_mfea
check_PROGRAMS		+= test_rawsock4
check_PROGRAMS		+= test_xrl_sockets4_udp

check_SCRIPTS		 = test_add_route.sh
check_SCRIPTS		+= test_config_interface.sh
check_SCRIPTS		+= test_xrl_sockets4_udp.sh

# -- Test Programs and Scripts
#TESTS			 = $(check_SCRIPTS)
TESTS			 = test_xrl_sockets4_udp.sh
TESTS			+= test_rawsock4$(EXEEXT)
TESTS			+= test_ifmanager_transaction$(EXEEXT)

# -- No-install Libraries
noinst_LTLIBRARIES	 = libfea.la

# -- Sources and Linkage for xorp_fea
xorp_fea_SOURCES	 = fea.cc
xorp_fea_LDADD		 = $(noinst_LTLIBRARIES)

### FEA XRL libraries
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libfeafibclientxif.la
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrclientxif.la
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpkt4client.la
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpkt6client.la
xorp_fea_LDADD		+= $(top_builddir)/xrl/targets/libfeabase.la
### MFEA XRL libraries
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrxif.la
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.la
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaclientxif.la
xorp_fea_LDADD		+= $(top_builddir)/xrl/targets/libmfeabase.la
### MRT libraries
xorp_fea_LDADD		+= $(top_builddir)/mrt/libmrt.la
### CLI libraries
xorp_fea_LDADD		+= $(top_builddir)/cli/libcli.la
xorp_fea_LDADD		+= $(top_builddir)/cli/libtecla/libtecla.a
xorp_fea_LDADD		+= @CLI_CURSES_LIB@
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libcliprocessorxif.la
xorp_fea_LDADD		+= $(top_builddir)/xrl/targets/libclibase.la
### libfeaclient libraries
xorp_fea_LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.la
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
xorp_fea_LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
### Packet ACL front-end XRL libraries
xorp_fea_LDADD		+= $(top_builddir)/xrl/targets/libpacketaclbase.la
### socket server libraries and related
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libsocket4userxif.la
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libsocket4xif.la
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libsocket6userxif.la
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libsocket6xif.la
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
xorp_fea_LDADD		+= $(top_builddir)/xrl/targets/libsocketserverbase.la
### XRL client for Profile
xorp_fea_LDADD		+= $(top_builddir)/xrl/interfaces/libprofileclientxif.la
### XORP misc libraries
xorp_fea_LDADD		+= $(top_builddir)/libproto/libproto.la
xorp_fea_LDADD		+= $(top_builddir)/libxipc/libxipc.la
xorp_fea_LDADD		+= $(top_builddir)/libcomm/libcomm.la
xorp_fea_LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Sources and Linkage for xorp_fea_dummy
xorp_fea_dummy_CXXFLAGS	 = -DFEA_DUMMY
xorp_fea_dummy_SOURCES	 = fea.cc
xorp_fea_dummy_LDADD	 = $(xorp_fea_LDADD)

# -- Sources and Linkage for test_mfea
test_mfea_SOURCES	 = test_mfea.cc
test_mfea_LDADD		 = $(noinst_LTLIBRARIES)

# FEA-specific begin
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libfeafibclientxif.la
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrclientxif.la
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpkt4client.la
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpkt6client.la
test_mfea_LDADD		+= $(top_builddir)/xrl/targets/libfeabase.la
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrxif.la
# FEA-specific end
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libclimanagerxif.la
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libmfeaclientxif.la
test_mfea_LDADD		+= $(top_builddir)/xrl/targets/libmfeabase.la
### MRT libraries
test_mfea_LDADD		+= $(top_builddir)/mrt/libmrt.la
### CLI libraries
test_mfea_LDADD		+= $(top_builddir)/cli/libcli.la
test_mfea_LDADD		+= $(top_builddir)/cli/libtecla/libtecla.a
test_mfea_LDADD		+= @CLI_CURSES_LIB@
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libcliprocessorxif.la
test_mfea_LDADD		+= $(top_builddir)/xrl/targets/libclibase.la
### libfeaclient libraries
test_mfea_LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.la
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
test_mfea_LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
### socket server libraries and releated
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libsocket4userxif.la
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libsocket4xif.la
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libsocket6userxif.la
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libsocket6xif.la
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
test_mfea_LDADD		+= $(top_builddir)/xrl/targets/libsocketserverbase.la
### XRL client for Profile
test_mfea_LDADD		+= $(top_builddir)/xrl/interfaces/libprofileclientxif.la
### XORP misc libraries
test_mfea_LDADD		+= $(top_builddir)/libproto/libproto.la
test_mfea_LDADD		+= $(top_builddir)/libxipc/libfinder.la
test_mfea_LDADD		+= $(top_builddir)/libxipc/libxipc.la
test_mfea_LDADD		+= $(top_builddir)/libcomm/libcomm.la
test_mfea_LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Sources and Linkage for test_ifmanager_transaction
test_ifmanager_transaction_SOURCES	 = test_ifmanager_transaction.cc
test_ifmanager_transaction_LDADD	 = $(noinst_LTLIBRARIES)
test_ifmanager_transaction_LDADD	+= $(top_builddir)/libxorp/libxorp.la

# -- Sources and Linkages for test_rawsock4
test_rawsock4_SOURCES	 = test_rawsock4.cc
test_rawsock4_LDADD	 = $(noinst_LTLIBRARIES)
test_rawsock4_LDADD	+= $(top_builddir)/libproto/libproto.la
test_rawsock4_LDADD	+= $(top_builddir)/libcomm/libcomm.la
test_rawsock4_LDADD	+= $(top_builddir)/libxorp/libxorp.la


# -- Sources and Linkage for test_xrl_sockets
test_xrl_sockets4_udp_SOURCES	 = test_xrl_sockets4_udp.cc

test_xrl_sockets4_udp_LDADD	 = $(noinst_LTLIBRARIES)
test_xrl_sockets4_udp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket4userxif.la
test_xrl_sockets4_udp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket4xif.la
test_xrl_sockets4_udp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket6userxif.la
test_xrl_sockets4_udp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket6xif.la
test_xrl_sockets4_udp_LDADD	+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
test_xrl_sockets4_udp_LDADD	+= $(top_builddir)/xrl/targets/libsocketserverbase.la
test_xrl_sockets4_udp_LDADD	+= $(top_builddir)/xrl/targets/libtestsocket4.la
test_xrl_sockets4_udp_LDADD	+= $(top_builddir)/libxipc/libxipc.la
test_xrl_sockets4_udp_LDADD	+= $(top_builddir)/libcomm/libcomm.la
test_xrl_sockets4_udp_LDADD	+= $(top_builddir)/libxorp/libxorp.la

# -- Sources and Linkage for demo_fea_ifmgr_client
demo_fea_ifmgr_client_SOURCES	 = demo_fea_ifmgr_client.cc
demo_fea_ifmgr_client_LDADD	 = $(top_builddir)/xrl/targets/libdemofeaifmgrclientbase.la
demo_fea_ifmgr_client_LDADD	+= $(top_builddir)/xrl/interfaces/libfeaifmgrxif.la
demo_fea_ifmgr_client_LDADD	+= $(top_builddir)/libxipc/libxipc.la
demo_fea_ifmgr_client_LDADD	+= $(top_builddir)/libcomm/libcomm.la
demo_fea_ifmgr_client_LDADD	+= $(top_builddir)/libxorp/libxorp.la

# -- Library Sources
libfea_la_SOURCES	 = fea_module.h
libfea_la_SOURCES	+= mfea_module.h
libfea_la_SOURCES	+= addr_table.hh
libfea_la_SOURCES	+= click_socket.hh
libfea_la_SOURCES	+= fte.hh
libfea_la_SOURCES	+= fti.hh
libfea_la_SOURCES	+= fti_transaction.hh
libfea_la_SOURCES	+= fticonfig.hh
libfea_la_SOURCES	+= fticonfig_entry_get.hh
libfea_la_SOURCES	+= fticonfig_entry_observer.hh
libfea_la_SOURCES	+= fticonfig_entry_set.hh
libfea_la_SOURCES	+= fticonfig_table_get.hh
libfea_la_SOURCES	+= fticonfig_table_observer.hh
libfea_la_SOURCES	+= fticonfig_table_set.hh
libfea_la_SOURCES	+= ifconfig.hh
libfea_la_SOURCES	+= ifconfig_addr_table.hh
libfea_la_SOURCES	+= ifconfig_get.hh
libfea_la_SOURCES	+= ifconfig_media.hh
libfea_la_SOURCES	+= ifconfig_observer.hh
libfea_la_SOURCES	+= ifconfig_set.hh
libfea_la_SOURCES	+= ifmanager.hh
libfea_la_SOURCES	+= ifmanager_transaction.hh
libfea_la_SOURCES	+= iftree.hh
libfea_la_SOURCES	+= kernel_utils.hh
libfea_la_SOURCES	+= libfeaclient_bridge.hh
libfea_la_SOURCES	+= mfea_dataflow.hh
libfea_la_SOURCES	+= mfea_kernel_messages.hh
libfea_la_SOURCES	+= mfea_mrouter.hh
libfea_la_SOURCES	+= mfea_node.hh
libfea_la_SOURCES	+= mfea_node_cli.hh
libfea_la_SOURCES	+= mfea_osdep.hh
libfea_la_SOURCES	+= mfea_proto_comm.hh
libfea_la_SOURCES	+= mfea_vif.hh
libfea_la_SOURCES	+= netlink_socket.hh
libfea_la_SOURCES	+= netlink_socket_utils.hh
libfea_la_SOURCES	+= nexthop_port_mapper.hh
libfea_la_SOURCES	+= pa_backend.hh
libfea_la_SOURCES	+= pa_backend_dummy.hh
libfea_la_SOURCES	+= pa_backend_ipfw2.hh
libfea_la_SOURCES	+= pa_entry.hh
libfea_la_SOURCES	+= pa_table.hh
libfea_la_SOURCES	+= pa_transaction.hh
libfea_la_SOURCES	+= profile_vars.hh
libfea_la_SOURCES	+= rawsock.hh
libfea_la_SOURCES	+= rawsock4.hh
libfea_la_SOURCES	+= rawsock6.hh
libfea_la_SOURCES	+= routing_socket.hh
libfea_la_SOURCES	+= routing_socket_utils.hh
libfea_la_SOURCES	+= win_rtm_pipe.hh
libfea_la_SOURCES	+= win_rtsock.h
libfea_la_SOURCES	+= win_support.hh
libfea_la_SOURCES	+= xrl_fti.hh
libfea_la_SOURCES	+= xrl_ifmanager.hh
libfea_la_SOURCES	+= xrl_ifupdate.hh
libfea_la_SOURCES	+= xrl_mfea_node.hh
libfea_la_SOURCES	+= xrl_packet_acl.hh
libfea_la_SOURCES	+= xrl_rawsock4.hh
libfea_la_SOURCES	+= xrl_rawsock6.hh
libfea_la_SOURCES	+= xrl_socket_cmds.hh
libfea_la_SOURCES	+= xrl_socket_server.hh
libfea_la_SOURCES	+= xrl_target.hh

libfea_la_SOURCES	+= addr_table.cc
libfea_la_SOURCES	+= click_socket.cc
libfea_la_SOURCES	+= fti_transaction.cc
libfea_la_SOURCES	+= fticonfig.cc
libfea_la_SOURCES	+= fticonfig_entry_get.cc
libfea_la_SOURCES	+= fticonfig_entry_get_click.cc
libfea_la_SOURCES	+= fticonfig_entry_get_dummy.cc
libfea_la_SOURCES	+= fticonfig_entry_get_iphelper.cc
libfea_la_SOURCES	+= fticonfig_entry_get_netlink.cc
libfea_la_SOURCES	+= fticonfig_entry_get_rtmv2.cc
libfea_la_SOURCES	+= fticonfig_entry_get_rtsock.cc
libfea_la_SOURCES	+= fticonfig_entry_observer.cc
libfea_la_SOURCES	+= fticonfig_entry_observer_dummy.cc
libfea_la_SOURCES	+= fticonfig_entry_observer_iphelper.cc
libfea_la_SOURCES	+= fticonfig_entry_observer_netlink.cc
libfea_la_SOURCES	+= fticonfig_entry_observer_rtmv2.cc
libfea_la_SOURCES	+= fticonfig_entry_observer_rtsock.cc
libfea_la_SOURCES	+= fticonfig_entry_parse_nlm.cc
libfea_la_SOURCES	+= fticonfig_entry_parse_rtm.cc
libfea_la_SOURCES	+= fticonfig_entry_set.cc
libfea_la_SOURCES	+= fticonfig_entry_set_click.cc
libfea_la_SOURCES	+= fticonfig_entry_set_dummy.cc
libfea_la_SOURCES	+= fticonfig_entry_set_iphelper.cc
libfea_la_SOURCES	+= fticonfig_entry_set_netlink.cc
libfea_la_SOURCES	+= fticonfig_entry_set_rtmv2.cc
libfea_la_SOURCES	+= fticonfig_entry_set_rtsock.cc
libfea_la_SOURCES	+= fticonfig_table_get.cc
libfea_la_SOURCES	+= fticonfig_table_get_click.cc
libfea_la_SOURCES	+= fticonfig_table_get_dummy.cc
libfea_la_SOURCES	+= fticonfig_table_get_iphelper.cc
libfea_la_SOURCES	+= fticonfig_table_get_netlink.cc
libfea_la_SOURCES	+= fticonfig_table_get_sysctl.cc
libfea_la_SOURCES	+= fticonfig_table_observer.cc
libfea_la_SOURCES	+= fticonfig_table_observer_dummy.cc
libfea_la_SOURCES	+= fticonfig_table_observer_iphelper.cc
libfea_la_SOURCES	+= fticonfig_table_observer_netlink.cc
libfea_la_SOURCES	+= fticonfig_table_observer_rtmv2.cc
libfea_la_SOURCES	+= fticonfig_table_observer_rtsock.cc
libfea_la_SOURCES	+= fticonfig_table_parse_nlm.cc
libfea_la_SOURCES	+= fticonfig_table_parse_rtm.cc
libfea_la_SOURCES	+= fticonfig_table_set.cc
libfea_la_SOURCES	+= fticonfig_table_set_click.cc
libfea_la_SOURCES	+= fticonfig_table_set_dummy.cc
libfea_la_SOURCES	+= fticonfig_table_set_iphelper.cc
libfea_la_SOURCES	+= fticonfig_table_set_netlink.cc
libfea_la_SOURCES	+= fticonfig_table_set_rtmv2.cc
libfea_la_SOURCES	+= fticonfig_table_set_rtsock.cc
libfea_la_SOURCES	+= ifconfig.cc
libfea_la_SOURCES	+= ifconfig_addr_table.cc
libfea_la_SOURCES	+= ifconfig_get.cc
libfea_la_SOURCES	+= ifconfig_get_click.cc
libfea_la_SOURCES	+= ifconfig_get_dummy.cc
libfea_la_SOURCES	+= ifconfig_get_getifaddrs.cc
libfea_la_SOURCES	+= ifconfig_get_ioctl.cc
libfea_la_SOURCES	+= ifconfig_get_iphelper.cc
libfea_la_SOURCES	+= ifconfig_get_netlink.cc
libfea_la_SOURCES	+= ifconfig_get_proc_linux.cc
libfea_la_SOURCES	+= ifconfig_get_sysctl.cc
libfea_la_SOURCES	+= ifconfig_media.cc
libfea_la_SOURCES	+= ifconfig_observer.cc
libfea_la_SOURCES	+= ifconfig_observer_dummy.cc
libfea_la_SOURCES	+= ifconfig_observer_iphelper.cc
libfea_la_SOURCES	+= ifconfig_observer_netlink.cc
libfea_la_SOURCES	+= ifconfig_observer_rtsock.cc
libfea_la_SOURCES	+= ifconfig_parse_ifaddrs.cc
libfea_la_SOURCES	+= ifconfig_parse_ifreq.cc
libfea_la_SOURCES	+= ifconfig_parse_nlm.cc
libfea_la_SOURCES	+= ifconfig_parse_rtm.cc
libfea_la_SOURCES	+= ifconfig_set.cc
libfea_la_SOURCES	+= ifconfig_set_click.cc
libfea_la_SOURCES	+= ifconfig_set_dummy.cc
libfea_la_SOURCES	+= ifconfig_set_ioctl.cc
libfea_la_SOURCES	+= ifconfig_set_iphelper.cc
libfea_la_SOURCES	+= ifconfig_set_netlink.cc
libfea_la_SOURCES	+= ifmanager_transaction.cc
libfea_la_SOURCES	+= iftree.cc
libfea_la_SOURCES	+= libfeaclient_bridge.cc
libfea_la_SOURCES	+= mfea_config.cc
libfea_la_SOURCES	+= mfea_dataflow.cc
libfea_la_SOURCES	+= mfea_mrouter.cc
libfea_la_SOURCES	+= mfea_node.cc
libfea_la_SOURCES	+= mfea_node_cli.cc
libfea_la_SOURCES	+= mfea_proto_comm.cc
libfea_la_SOURCES	+= mfea_vif.cc
libfea_la_SOURCES	+= netlink_socket.cc
libfea_la_SOURCES	+= netlink_socket_utils.cc
libfea_la_SOURCES	+= nexthop_port_mapper.cc
libfea_la_SOURCES	+= pa_backend_dummy.cc
libfea_la_SOURCES	+= pa_backend_ipfw2.cc
libfea_la_SOURCES	+= pa_entry.cc
libfea_la_SOURCES	+= pa_table.cc
libfea_la_SOURCES	+= pa_transaction.cc
libfea_la_SOURCES	+= profile_vars.cc
libfea_la_SOURCES	+= rawsock.cc
libfea_la_SOURCES	+= rawsock4.cc
libfea_la_SOURCES	+= rawsock6.cc
libfea_la_SOURCES	+= routing_socket.cc
libfea_la_SOURCES	+= routing_socket_utils.cc
libfea_la_SOURCES	+= win_rtm_pipe.cc
libfea_la_SOURCES	+= win_support.cc
libfea_la_SOURCES	+= xrl_packet_acl.cc
libfea_la_SOURCES	+= xrl_fti.cc
libfea_la_SOURCES	+= xrl_ifmanager.cc
libfea_la_SOURCES	+= xrl_ifupdate.cc
libfea_la_SOURCES	+= xrl_mfea_node.cc
libfea_la_SOURCES	+= xrl_rawsock4.cc
libfea_la_SOURCES	+= xrl_rawsock6.cc
libfea_la_SOURCES	+= xrl_socket_cmds.cc
libfea_la_SOURCES	+= xrl_socket_server.cc
libfea_la_SOURCES	+= xrl_target.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = DEVNOTES.mfea
EXTRA_DIST	+= command_mfea
EXTRA_DIST	+= configure_mfea
EXTRA_DIST	+= test_xrls.sh
EXTRA_DIST	+= xorp_fea_click_config_generator
EXTRA_DIST	+= xrl_mfea_shell_funcs.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

# Test scripts dependency
test_add_route.sh: $(top_builddir)/fea/xrl_fea_shell_funcs.sh
test_config_interface.sh: $(top_builddir)/fea/xrl_fea_shell_funcs.sh

# XRL wrapper scripts dependency
$(top_builddir)/fea/xrl_fea_shell_funcs.sh:				\
    $(top_srcdir)/xrl/targets/fea.xrls					\
    $(top_srcdir)/utils/xrl_wrappers_generator.sh
	$(top_srcdir)/utils/xrl_wrappers_generator.sh $(top_srcdir)/xrl/targets/fea.xrls > $(top_builddir)/fea/xrl_fea_shell_funcs.sh
