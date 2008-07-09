## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/fea/Makefile.am,v 1.105 2008/04/26 00:59:42 pavlin Exp $
##

# Redefine the bin installation directory
bindir = $(prefix)/$(subdir)

# -- Subdirectories
SUBDIRS = data_plane . tools

# -- Real Programs
bin_PROGRAMS	 = xorp_fea xorp_fea_dummy
bin_SCRIPTS	 = xorp_fea_click_config_generator

# -- Test Programs and Scripts
check_PROGRAMS	 = test_xrl_sockets4_tcp
check_PROGRAMS	+= test_xrl_sockets4_udp
check_PROGRAMS	+= test_fea_rawlink

check_SCRIPTS	 = test_xrl_sockets4_tcp.sh
check_SCRIPTS	+= test_xrl_sockets4_udp.sh
check_SCRIPTS	+= test_fea_rawlink.sh

TESTS		 = test_xrl_sockets4_tcp.sh
TESTS		+= test_xrl_sockets4_udp.sh
TESTS		+= test_fea_rawlink.sh

# -- No-install Libraries
noinst_LTLIBRARIES = libfea.la libfea_data_plane_base.la

# -- Libraries and library flags to link the binary programs
LDADD		 = libfea.la
### Data plane libraries
LDADD		+= $(top_builddir)/fea/data_plane/managers/libxorp_data_plane_managers.la
LDADD		+= $(top_builddir)/fea/data_plane/fibconfig/libxorp_fibconfig.la
LDADD		+= $(top_builddir)/fea/data_plane/firewall/libxorp_firewall.la
LDADD		+= $(top_builddir)/fea/data_plane/ifconfig/libxorp_ifconfig.la
LDADD		+= $(top_builddir)/fea/data_plane/io/libxorp_io.la
LDADD		+= $(top_builddir)/fea/data_plane/control_socket/libxorp_control_socket.la
LDADD		+= libfea_data_plane_base.la
### XRL libraries
LDADD		+= $(top_builddir)/xrl/interfaces/libfeafibclientxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfearawlinkclientxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpkt4clientxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfearawpkt6clientxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libsocket4userxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libsocket6userxif.la
LDADD		+= $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.la
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
libfea_data_plane_base_la_SOURCES	 = fea_data_plane_manager.hh
libfea_data_plane_base_la_SOURCES	+= fibconfig_forwarding.hh
libfea_data_plane_base_la_SOURCES	+= ifconfig_property.hh
libfea_data_plane_base_la_SOURCES	+= io_ip.hh
libfea_data_plane_base_la_SOURCES	+= io_link.hh
libfea_data_plane_base_la_SOURCES	+= io_tcpudp.hh
libfea_data_plane_base_la_SOURCES	+= fea_data_plane_manager.cc
libfea_data_plane_base_la_SOURCES	+= fibconfig_forwarding.cc
libfea_data_plane_base_la_SOURCES	+= ifconfig_property.cc
libfea_data_plane_base_la_SOURCES	+= io_ip.cc
libfea_data_plane_base_la_SOURCES	+= io_link.cc
libfea_data_plane_base_la_SOURCES	+= io_tcpudp.cc

libfea_la_SOURCES	 = fea_module.h
libfea_la_SOURCES	+= mfea_module.h
libfea_la_SOURCES	+= fea_io.hh
libfea_la_SOURCES	+= fea_node.hh
libfea_la_SOURCES	+= fibconfig.hh
libfea_la_SOURCES	+= fibconfig_entry_get.hh
libfea_la_SOURCES	+= fibconfig_entry_observer.hh
libfea_la_SOURCES	+= fibconfig_entry_set.hh
libfea_la_SOURCES	+= fibconfig_table_get.hh
libfea_la_SOURCES	+= fibconfig_table_observer.hh
libfea_la_SOURCES	+= fibconfig_table_set.hh
libfea_la_SOURCES	+= fibconfig_transaction.hh
libfea_la_SOURCES	+= firewall_entry.hh
libfea_la_SOURCES	+= firewall_get.hh
libfea_la_SOURCES	+= firewall_manager.hh
libfea_la_SOURCES	+= firewall_set.hh
libfea_la_SOURCES	+= firewall_transaction.hh
libfea_la_SOURCES	+= fte.hh
libfea_la_SOURCES	+= ifconfig.hh
libfea_la_SOURCES	+= ifconfig_get.hh
libfea_la_SOURCES	+= ifconfig_observer.hh
libfea_la_SOURCES	+= ifconfig_reporter.hh
libfea_la_SOURCES	+= ifconfig_set.hh
libfea_la_SOURCES	+= ifconfig_vlan_get.hh
libfea_la_SOURCES	+= ifconfig_vlan_set.hh
libfea_la_SOURCES	+= ifconfig_transaction.hh
libfea_la_SOURCES	+= iftree.hh
libfea_la_SOURCES	+= io_ip_manager.hh
libfea_la_SOURCES	+= io_link_manager.hh
libfea_la_SOURCES	+= io_tcpudp_manager.hh
libfea_la_SOURCES	+= libfeaclient_bridge.hh
libfea_la_SOURCES	+= mfea_dataflow.hh
libfea_la_SOURCES	+= mfea_kernel_messages.hh
libfea_la_SOURCES	+= mfea_mrouter.hh
libfea_la_SOURCES	+= mfea_node.hh
libfea_la_SOURCES	+= mfea_node_cli.hh
libfea_la_SOURCES	+= mfea_osdep.hh
libfea_la_SOURCES	+= mfea_vif.hh
libfea_la_SOURCES	+= nexthop_port_mapper.hh
libfea_la_SOURCES	+= profile_vars.hh
libfea_la_SOURCES	+= xrl_fea_io.hh
libfea_la_SOURCES	+= xrl_fea_node.hh
libfea_la_SOURCES	+= xrl_fea_target.hh
libfea_la_SOURCES	+= xrl_fib_client_manager.hh
libfea_la_SOURCES	+= xrl_io_ip_manager.hh
libfea_la_SOURCES	+= xrl_io_link_manager.hh
libfea_la_SOURCES	+= xrl_io_tcpudp_manager.hh
libfea_la_SOURCES	+= xrl_mfea_node.hh

libfea_la_SOURCES	+= fea_io.cc
libfea_la_SOURCES	+= fea_node.cc
libfea_la_SOURCES	+= fibconfig.cc
libfea_la_SOURCES	+= fibconfig_transaction.cc
libfea_la_SOURCES	+= firewall_entry.cc
libfea_la_SOURCES	+= firewall_manager.cc
libfea_la_SOURCES	+= firewall_transaction.cc
libfea_la_SOURCES	+= ifconfig.cc
libfea_la_SOURCES	+= ifconfig_reporter.cc
libfea_la_SOURCES	+= ifconfig_transaction.cc
libfea_la_SOURCES	+= iftree.cc
libfea_la_SOURCES	+= io_ip_manager.cc
libfea_la_SOURCES	+= io_link_manager.cc
libfea_la_SOURCES	+= io_tcpudp_manager.cc
libfea_la_SOURCES	+= libfeaclient_bridge.cc
libfea_la_SOURCES	+= mfea_config.cc
libfea_la_SOURCES	+= mfea_dataflow.cc
libfea_la_SOURCES	+= mfea_mrouter.cc
libfea_la_SOURCES	+= mfea_node.cc
libfea_la_SOURCES	+= mfea_node_cli.cc
libfea_la_SOURCES	+= mfea_vif.cc
libfea_la_SOURCES	+= nexthop_port_mapper.cc
libfea_la_SOURCES	+= profile_vars.cc
libfea_la_SOURCES	+= xrl_fea_io.cc
libfea_la_SOURCES	+= xrl_fea_node.cc
libfea_la_SOURCES	+= xrl_fea_target.cc
libfea_la_SOURCES	+= xrl_fib_client_manager.cc
libfea_la_SOURCES	+= xrl_io_ip_manager.cc
libfea_la_SOURCES	+= xrl_io_link_manager.cc
libfea_la_SOURCES	+= xrl_io_tcpudp_manager.cc
libfea_la_SOURCES	+= xrl_mfea_node.cc

# -- Sources and Linkage for test_xrl_sockets4_tcp
test_xrl_sockets4_tcp_SOURCES	 = test_xrl_sockets4_tcp.cc
test_xrl_sockets4_tcp_LDADD	 = $(noinst_LTLIBRARIES)
test_xrl_sockets4_tcp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket4userxif.la
test_xrl_sockets4_tcp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket4xif.la
test_xrl_sockets4_tcp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket6userxif.la
test_xrl_sockets4_tcp_LDADD	+= $(top_builddir)/xrl/interfaces/libsocket6xif.la
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
test_xrl_sockets4_udp_LDADD	+= $(top_builddir)/xrl/targets/libtestsocket4.la
test_xrl_sockets4_udp_LDADD	+= $(top_builddir)/libxipc/libxipc.la
test_xrl_sockets4_udp_LDADD	+= $(top_builddir)/libcomm/libcomm.la
test_xrl_sockets4_udp_LDADD	+= $(top_builddir)/libxorp/libxorp.la

# -- Sources and Linkage for test_fea_rawlink
test_fea_rawlink_SOURCES	 = test_fea_rawlink.cc
test_fea_rawlink_LDADD		 = $(noinst_LTLIBRARIES)
test_fea_rawlink_LDADD		+= $(top_builddir)/libfeaclient/libfeaclient.la
test_fea_rawlink_LDADD		+= $(top_builddir)/xrl/interfaces/libfearawlinkxif.la
test_fea_rawlink_LDADD		+= $(top_builddir)/xrl/interfaces/libfearawlinkclientxif.la
test_fea_rawlink_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
test_fea_rawlink_LDADD		+= $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
test_fea_rawlink_LDADD		+= $(top_builddir)/xrl/targets/libtestfearawlink.la
test_fea_rawlink_LDADD		+= $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
test_fea_rawlink_LDADD		+= $(top_builddir)/libxipc/libxipc.la
test_fea_rawlink_LDADD		+= $(top_builddir)/libcomm/libcomm.la
test_fea_rawlink_LDADD		+= $(top_builddir)/libxorp/libxorp.la

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
