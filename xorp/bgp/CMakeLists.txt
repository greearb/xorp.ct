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


if (PROFILER)
    list(APPEND BGP_LIBS xif_profile_client)
    list(APPEND BGP_SRCS profile_vars.cc)
endif()


#add_library(bgp SHARED ${BGP_SRCS})
add_executable(xorp_bgp main.cc)

add_library(bgp ${BGP_SRCS})
target_compile_options(bgp PRIVATE -ftemplate-depth=35)

target_link_libraries(bgp
                          cli
                          xif_cli_processor
                          tgts_cli
                          finder
                          tecla
                          xipc
                          proto
                          comm
                          xorp
                     )

target_link_libraries(xorp_bgp
                               bgp
                               cli
                               xif_cli_processor
                               tgts_cli
                               policy_backend
                               policy_common
                               fea_client
                               tgts_bgp
                               tgts_fea_ifmgr_mirror
                               xif_rib
                               xif_finder_event_notifier
                               xif_fea_ifmgr_mirror
                               xif_fea_ifmgr_replicator
                               finder
                               tecla
                               xipc
                               proto
                               comm
                               xorp
                               ${BGP_LIBS}
                    )

install(TARGETS xorp_bgp RUNTIME DESTINATION ${XORP_MODULEDIR})
install(TARGETS bgp LIBRARY DESTINATION ${XORP_LIBDIR})
install(FILES bgp_xrl_shell_funcs.sh DESTINATION ${XORP_SBINDIR} PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE
                                                                             GROUP_READ GROUP_EXECUTE
                                                                             WORLD_EXECUTE WORLD_READ)

if (TESTS_PROGRAMS)
    add_subdirectory(tests)
    add_subdirectory(harness)
endif()
