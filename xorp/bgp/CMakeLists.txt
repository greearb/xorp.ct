# bgp main build
# Copyright (c) 2021-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_subdirectory(tools)

set(BGP_LIBS "")

file(GLOB BGP_SRCS
                    # 'aspath.cc',
                    attribute_manager.cc
                    bgp.cc
                    bgp_trie.cc
                    bgp_varrw.cc
                    bgp_varrw_export.cc
                    crash_dump.cc
                    damping.cc
                    dump_iterators.cc
                    internal_message.cc
                    iptuple.cc
                    local_data.cc
                    next_hop_resolver.cc
                    notification_packet.cc
                    open_packet.cc
                    packet.cc
                    parameter.cc
                    path_attribute.cc
                    peer.cc
                    peer_data.cc
                    peer_handler.cc
                    peer_list.cc
                    plumbing.cc
                    process_watch.cc
                    rib_ipc_handler.cc
                    route_queue.cc
                    route_table_aggregation.cc
                    route_table_base.cc
                    route_table_cache.cc
                    route_table_damping.cc
                    route_table_decision.cc
                    # route_table_debug.cc
                    route_table_deletion.cc
                    route_table_dump.cc
                    route_table_fanout.cc
                    route_table_filter.cc
                    route_table_nhlookup.cc
                    route_table_policy.cc
                    route_table_policy_ex.cc
                    route_table_policy_im.cc
                    route_table_policy_sm.cc
                    route_table_reader.cc
                    route_table_ribin.cc
                    route_table_ribout.cc
                    socket.cc
                    subnet_route.cc
                    update_attrib.cc
                    update_packet.cc
                    xrl_target.cc
    )

if (ENABLE_PROFILE)
    list(APPEND BGP_LIBS xif_profile_client)
    list(APPEND BGP_SRCS profile_vars.cc)
endif()



add_executable(xorp_bgp main.cc)
add_library(bgp ${BGP_SRCS})
target_compile_options(bgp PRIVATE -ftemplate-depth=35)

list(APPEND BGP_LIBS cli
                     fea_client
                     finder
                     xif_cli_processor
                     policy_backend
                     policy_common
                     xorp
                     comm
                     proto
                     xipc
                     xif_fea_ifmgr_mirror
                     xif_rib
                     xif_finder_event_notifier
    )

target_link_libraries(bgp
                          tgts_bgp
                          tgts_cli
                          tecla
                          ${BGP_LIBS}
                     )

target_link_libraries(xorp_bgp
                               bgp
                               tgts_cli
                               tgts_bgp
                               tgts_fea_ifmgr_mirror
                               xif_rib
                               xif_fea_ifmgr_replicator
                               tecla
                               ${BGP_LIBS}
                    )

install(TARGETS xorp_bgp RUNTIME DESTINATION ${XORP_MODULEDIR})
install(TARGETS bgp LIBRARY DESTINATION ${XORP_LIBDIR})
install(PROGRAMS bgp_xrl_shell_funcs.sh DESTINATION ${XORP_SBINDIR})

if (RTLD_ORIGIN)
    set_target_properties(xorp_bgp PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                              INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    set_target_properties(bgp PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                         INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
endif()
if (BUILDDIR_RUN)
    add_custom_command(TARGET xorp_bgp POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp_bgp>
                        ${XORP_ALIAS_MODULEDIR}/$<TARGET_FILE_NAME:xorp_bgp>)
    add_custom_command(TARGET bgp POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:bgp>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:bgp>)
    file(COPY bgp_xrl_shell_funcs.sh DESTINATION ${XORP_ALIAS_SBINDIR})
endif()

if (TESTS_PROGRAMS)
    add_subdirectory(tests)
    add_subdirectory(harness)
endif()
