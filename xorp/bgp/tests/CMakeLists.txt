# bgp/tests build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(BGPTESTS
            bgp
            xif_bgp
            xif_rib
            xif_finder_event_notifier
            fea_client
            xif_fea_ifmgr_mirror
            xif_fea_ifmgr_replicator
            tgts_fea_ifmgr_mirror
            xif_fea_rawpkt4
            xif_fea_rawpkt6
            policy_backend
            policy_common
            xipc
            proto
            xorp
            comm
            finder
        )

# foreach(T IN ITEMS "cache" "decision" "deletion" "dump" "fanout" "filter" "main"
#     "next_hop_resolver" "nhlookup" "packet" "packet_coding" "peer_data" "plumbing"
#     "policy" "ribin" "ribout" "subnet_route")
foreach(T IN ITEMS "plumbing" "packet" "packet_coding" "peer_data")
    add_executable(test_bgp_${T} test_${T}.cc "${CMAKE_CURRENT_SOURCE_DIR}/../dummy_next_hop_resolver.cc")
    target_link_libraries(test_bgp_${T} ${BGPTESTS})
    target_include_directories(test_bgp_${T} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../")
    add_test(${T} COMMAND test_bgp_${T})
endforeach()

add_executable(test_bgp_all
                            test_cache.cc
                            test_decision.cc
                            test_deletion.cc
                            test_dump.cc
                            test_fanout.cc
                            test_filter.cc
                            test_main.cc
                            test_next_hop_resolver.cc
                            test_nhlookup.cc
                            # test_packet.cc
                            # test_packet_coding.cc
                            # test_peer_data.cc
                            # test_plumbing.cc
                            test_policy.cc
                            test_ribin.cc
                            test_ribout.cc
                            test_subnet_route.cc
                            test_main.cc
                            ${CMAKE_CURRENT_SOURCE_DIR}/../dummy_next_hop_resolver.cc
                            ${CMAKE_CURRENT_SOURCE_DIR}/../peer_handler_debug.cc
                            ${CMAKE_CURRENT_SOURCE_DIR}/../route_table_debug.cc
                )
target_link_libraries(test_bgp_all ${BGPTESTS})
target_include_directories(test_bgp_all PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../")
add_test(test_bgp COMMAND test_bgp_all)
