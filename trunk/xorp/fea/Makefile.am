## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/fea/Makefile.am,v 1.90 2007/05/03 09:32:35 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Subdirectories
SUBDIRS = forwarding_plane . tools

# -- Real Programs
bin_PROGRAMS	 = xorp_fea xorp_fea_dummy
bin_SCRIPTS	 = xorp_fea_click_config_generator

# -- Test Programs and Scripts
check_PROGRAMS	 = test_xrl_sockets4_tcp
check_PROGRAMS	+= test_xrl_sockets4_udp

check_SCRIPTS	 = test_xrl_sockets4_tcp.sh
check_SCRIPTS	+= test_xrl_sockets4_udp.sh

TESTS		 = test_xrl_sockets4_tcp.sh
TESTS		+= test_xrl_sockets4_udp.sh

# -- No-install Libraries
noinst_LTLIBRARIES = libfea.la

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LTLIBRARIES)
### Forwarding plane libraries
LDADD		+= $(top_builddir)/fea/forwarding_plane/fibconfig/libxorp_fibconfig.la
LDADD		+= $(top_builddir)/fea/forwarding_plane/ifconfig/libxorp_ifconfig.la
LDADD		+= $(top_builddir)/fea/forwarding_plane/control_socket/libxorp_control_socket.la
### XRL libraries
LDADD		+= $(top_builddir)/xrl/interfaces/libfeafibclientxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpkt4client.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpkt6client.la
LDADD		+= $(top_builddir)/xrl/targets/libfeabase.la
### MFEA XRL libraries
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
### libfeaclient libraries
LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
### Packet ACL front-end XRL libraries
LDADD		+= $(top_builddir)/xrl/targets/libpacketaclbase.la
### socket server libraries and related
LDADD		+= $(top_builddir)/xrl/interfaces/libsocket4userxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libsocket4xif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libsocket6userxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libsocket6xif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
LDADD		+= $(top_builddir)/xrl/targets/libsocketserverbase.la
### XRL client for Profile
LDADD		+= $(top_builddir)/xrl/interfaces/libprofileclientxif.la
### XORP misc libraries
LDADD		+= $(top_builddir)/libproto/libproto.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
xorp_fea_SOURCES			= xorp_fea.cc

xorp_fea_dummy_CXXFLAGS			= -DFEA_DUMMY
xorp_fea_dummy_SOURCES			= xorp_fea.cc

# -- Library Sources
libfea_la_SOURCES	 = fea_module.h
libfea_la_SOURCES	+= mfea_module.h
libfea_la_SOURCES	+= addr_table.hh
libfea_la_SOURCES	+= fea_io.hh
libfea_la_SOURCES	+= fea_node.hh
libfea_la_SOURCES	+= fte.hh
libfea_la_SOURCES	+= fibconfig.hh
libfea_la_SOURCES	+= fibconfig_entry_get.hh
libfea_la_SOURCES	+= fibconfig_entry_observer.hh
libfea_la_SOURCES	+= fibconfig_entry_set.hh
libfea_la_SOURCES	+= fibconfig_table_get.hh
libfea_la_SOURCES	+= fibconfig_table_observer.hh
libfea_la_SOURCES	+= fibconfig_table_set.hh
libfea_la_SOURCES	+= fibconfig_transaction.hh
libfea_la_SOURCES	+= ifconfig.hh
libfea_la_SOURCES	+= ifconfig_addr_table.hh
libfea_la_SOURCES	+= ifconfig_get.hh
libfea_la_SOURCES	+= ifconfig_observer.hh
libfea_la_SOURCES	+= ifconfig_reporter.hh
libfea_la_SOURCES	+= ifconfig_set.hh
libfea_la_SOURCES	+= ifconfig_transaction.hh
libfea_la_SOURCES	+= iftree.hh
libfea_la_SOURCES	+= libfeaclient_bridge.hh
libfea_la_SOURCES	+= mfea_dataflow.hh
libfea_la_SOURCES	+= mfea_kernel_messages.hh
libfea_la_SOURCES	+= mfea_mrouter.hh
libfea_la_SOURCES	+= mfea_node.hh
libfea_la_SOURCES	+= mfea_node_cli.hh
libfea_la_SOURCES	+= mfea_osdep.hh
libfea_la_SOURCES	+= mfea_vif.hh
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
libfea_la_SOURCES	+= xrl_fea_io.hh
libfea_la_SOURCES	+= xrl_fea_node.hh
libfea_la_SOURCES	+= xrl_fea_target.hh
libfea_la_SOURCES	+= xrl_fib_client_manager.hh
libfea_la_SOURCES	+= xrl_mfea_node.hh
libfea_la_SOURCES	+= xrl_packet_acl.hh
libfea_la_SOURCES	+= xrl_rawsock4.hh
libfea_la_SOURCES	+= xrl_rawsock6.hh
libfea_la_SOURCES	+= xrl_socket_cmds.hh
libfea_la_SOURCES	+= xrl_socket_server.hh

libfea_la_SOURCES	+= addr_table.cc
libfea_la_SOURCES	+= fea_io.cc
libfea_la_SOURCES	+= fea_node.cc
libfea_la_SOURCES	+= fibconfig.cc
libfea_la_SOURCES	+= fibconfig_transaction.cc
libfea_la_SOURCES	+= ifconfig.cc
libfea_la_SOURCES	+= ifconfig_addr_table.cc
libfea_la_SOURCES	+= ifconfig_reporter.cc
libfea_la_SOURCES	+= ifconfig_transaction.cc
libfea_la_SOURCES	+= iftree.cc
libfea_la_SOURCES	+= libfeaclient_bridge.cc
libfea_la_SOURCES	+= mfea_config.cc
libfea_la_SOURCES	+= mfea_dataflow.cc
libfea_la_SOURCES	+= mfea_mrouter.cc
libfea_la_SOURCES	+= mfea_node.cc
libfea_la_SOURCES	+= mfea_node_cli.cc
libfea_la_SOURCES	+= mfea_vif.cc
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
libfea_la_SOURCES	+= xrl_packet_acl.cc
libfea_la_SOURCES	+= xrl_fea_io.cc
libfea_la_SOURCES	+= xrl_fea_node.cc
libfea_la_SOURCES	+= xrl_fea_target.cc
libfea_la_SOURCES	+= xrl_fib_client_manager.cc
libfea_la_SOURCES	+= xrl_mfea_node.cc
libfea_la_SOURCES	+= xrl_rawsock4.cc
libfea_la_SOURCES	+= xrl_rawsock6.cc
libfea_la_SOURCES	+= xrl_socket_cmds.cc
libfea_la_SOURCES	+= xrl_socket_server.cc

# -- Sources and Linkage for test_xrl_sockets4_tcp
test_xrl_sockets4_tcp_SOURCES	 = test_xrl_sockets4_tcp.cc
test_xrl_sockets4_tcp_LDADD	 = $(noinst_LTLIBRARIES)
test_xrl_sockets4_tcp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket4userxif.la
test_xrl_sockets4_tcp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket4xif.la
test_xrl_sockets4_tcp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket6userxif.la
test_xrl_sockets4_tcp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket6xif.la
test_xrl_sockets4_tcp_LDADD	+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
test_xrl_sockets4_tcp_LDADD	+= $(top_builddir)/xrl/targets/libsocketserverbase.la
test_xrl_sockets4_tcp_LDADD	+= $(top_builddir)/xrl/targets/libtestsocket4.la
test_xrl_sockets4_tcp_LDADD	+= $(top_builddir)/libxipc/libxipc.la
test_xrl_sockets4_tcp_LDADD	+= $(top_builddir)/libcomm/libcomm.la
test_xrl_sockets4_tcp_LDADD	+= $(top_builddir)/libxorp/libxorp.la

# -- Sources and Linkage for test_xrl_sockets4_udp
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

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = DEVNOTES.mfea
EXTRA_DIST	+= command_mfea
EXTRA_DIST	+= xorp_fea_click_config_generator
EXTRA_DIST	+= xrl_mfea_shell_funcs.sh
EXTRA_DIST	+= xrl_shell_funcs.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
