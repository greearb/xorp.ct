# fea build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_subdirectory(data_plane)
if (TESTS_PROGRAMS)
    add_subdirectory(tests)
endif()
add_subdirectory(tools)

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
             xorp_mrt
             proto
             xipc
             comm
             xorp
             $<TARGET_NAME:fea_managers>
             $<TARGET_NAME:fibconfig>
             $<TARGET_NAME:fea_io>
             $<TARGET_NAME:control_socket>
             $<TARGET_NAME:ifconfig>
             $<TARGET_NAME:xfdp>
             tecla)

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
    list(APPEND FEA_LIBS $<TARGET_NAME:firewall>)
    list(APPEND FEA_SRCS firewall_entry.cc
                         firewall_manager.cc
                         firewall_transaction.cc)
endif()

# With external tecla library, there will be other libtecla.so
# if (EXTRNAL_TECLA)
#     list(APPEND FEA_LIBS tecla)
# endif()

if (ENABLE_PROFILE)
    list(APPEND FEA_SRCS profile_vars.cc)
    list(APPEND FEA_LIBS xif_profile_client)
endif()

if (ENABLE_IPV6)
    list(APPEND FEA_LIBS xif_fea_rawpkt6_client
                         xif_socket6_user)
endif()

add_library(xfdp OBJECT
                 fea_data_plane_manager.cc
                 fibconfig_forwarding.cc
                 ifconfig_property.cc
                 iftree.cc
                 io_ip.cc
                 io_link.cc
                 io_tcpudp.cc)
set_target_properties(xfdp PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
# install(TARGETS xfdp LIBRARY DESTINATION ${XORP_LIBDIR})

add_library(fea ${FEA_SRCS})
target_link_libraries(fea ${FEA_LIBS})
install(TARGETS fea LIBRARY DESTINATION ${XORP_LIBDIR})
add_executable(xorp_fea xorp_fea.cc)
target_link_libraries(xorp_fea PUBLIC ${FEA_LIBS} fea)
install(TARGETS xorp_fea RUNTIME DESTINATION ${XORP_MODULEDIR})

if (FEA_DUMMY)
    add_executable(fea_dummy ${FEA_SRCS})
    target_link_libraries(fea_dummy PUBLIC ${FEA_LIBS})
    target_compile_definitions(fea_dummy PRIVATE "FEA_DUMMY=1")
    install(TARGETS fea_dummy RUNTIME DESTINATION ${XORP_MODULEDIR})
endif()

if (CLICK)
    install(PROGRAMS fea_click_config_generator DESTINATION ${XORP_TOOLDIR})
endif()

install(PROGRAMS fea_xrl_shell_funcs.sh DESTINATION ${XORP_SBINDIR})

if (RTLD_ORIGIN)
    set_target_properties(fea PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                   INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    set_target_properties(xorp_fea PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                              INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    if (FEA_DUMMY)
        set_target_properties(fea_dummy PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                   INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    endif()
endif()

if (BUILDDIR_RUN)
    add_custom_command(TARGET xorp_fea POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp_fea>
                        ${XORP_ALIAS_MODULEDIR}/$<TARGET_FILE_NAME:xorp_fea>)
    add_custom_command(TARGET fea POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:fea>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:fea>)
    if (FEA_DUMMY)
        add_custom_command(TARGET fea_dummy POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:fea_dummy>
                            ${XORP_ALIAS_MODULEDIR}/$<TARGET_FILE_NAME:fea_dummy>)
    endif()
    file(COPY fea_xrl_shell_funcs.sh DESTINATION ${XORP_ALIAS_SBINDIR})
endif()
