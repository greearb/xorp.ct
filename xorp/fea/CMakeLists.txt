# fea build
# Copyright (c) 2023 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_subdirectory(data_plane)
add_subdirectory(tests)
# add_subdirectory(tools)

# add_library(static  static_routes_node.cc
#                     static_routes_varrw.cc
#                     xrl_static_routes_node.cc)

# target_include_directories(static PRIVATE ${XORP_LIBDIR})
# target_link_libraries(static PRIVATE feaclient xipc xorp interfaces policy crypto proto)
set(FEA_LIBS xif_fea_fib_client
             xif_fea_rawlink_client
             xif_fea_rawpkt4_client
             xif_socket4_user
             xif_finder_event_notifier
             cli
             xif_cli_processor
             fea_client
             xif_fea_ifmgr_mirror   # XXX?
             xif_fea_ifmgr_replicator # XXX?
             tgts_fea_ifmgr_mirror # XXX?
             xif_cli_manager
             xif_mfea_client
             tgts_cli
             tgts_fea
             tgts_mfea
             mrt
             proto
             xipc
             comm
             xorp
             fea_managers
             fibconfig
             fea_io
             control_socket
             ifconfig
             xfdp)

if (PCAP_LIB)
    list(APPEND FEA_LIBS pcap)
endif()

set(FEA_SRCS xorp_fea.cc
             fea_io.cc
             fea_node.cc
             fibconfig.cc
             fibconfig_transaction.cc
             ifconfig.cc
             ifconfig_reporter.cc
             ifconfig_transaction.cc
             io_ip_manager.cc
             io_link_manager.cc
             io_tcpudp_manager.cc
             libfeaclient_bridge.cc
             mfea_config.cc
             mfea_dataflow.cc
             mfea_mrouter.cc
             mfea_node.cc
             mfea_node_cli.cc
             mfea_vif.cc
             nexthop_port_mapper.cc
             xrl_fea_io.cc
             xrl_fea_node.cc
             xrl_fea_target.cc
             xrl_fib_client_manager.cc
             xrl_io_ip_manager.cc
             xrl_io_link_manager.cc
             xrl_io_tcpudp_manager.cc
             xrl_mfea_node.cc)

if (FIREWALL)
    list(APPEND FEA_LIBS firewall)
    list(APPEND FEA_SRCS firewall_entry.cc
                         firewall_manager.cc
                         firewall_transaction.cc)
endif()

if (EXTRNAL_TECLA)
    list(APPEND FEA_LIBS tecla)
endif()

if (ENABLE_IPV6)
    list(APPEND FEA_LIBS xif_fea_rawpkt6_client
                         xif_socket6_user)
endif()

add_library(xfdp
                 fea_data_plane_manager.cc
                 fibconfig_forwarding.cc
                 ifconfig_property.cc
                 iftree.cc
                 io_ip.cc
                 io_link.cc
                 io_tcpudp.cc)
install(TARGETS xfdp RUNTIME DESTINATION lib)

add_executable(fea ${FEA_SRCS})
target_link_libraries(fea PUBLIC ${FEA_LIBS})
install(TARGETS fea RUNTIME DESTINATION sbin)

if (FEA_DUMMY)
    add_executable(fea_dummy ${FEA_SRCS})
    target_link_libraries(fea_dummy PUBLIC ${FEA_LIBS})
    target_add_definitions(fea_dummy PRIVATE "FEA_DUMMY=1")
    install(TARGETS fea_dummy RUNTIME DESTINATION sbin)

endif()


