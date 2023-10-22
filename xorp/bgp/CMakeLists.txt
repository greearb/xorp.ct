# bgp main build
# Copyright (c) 2021 Michał Zagórski (zagura)
# License: see ${XORP}/LICENCE.gpl
# add_subdirectory(tools)
# if (BUILD_TEST)
#     add_subdirectory(tests)
# endif()

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
                    xrl_target.cc)



add_library(bgp SHARED ${BGP_SRCS})
target_compile_options(bgp PRIVATE -ftemplate-depth=35)
add_dependencies(bgp interfaces)
add_dependencies(bgp xorp)
install(TARGETS bgp RUNTIME DESTINATION lib)
install(FILES bgp_xrl_shell_funcs.sh TYPE SBIN)
