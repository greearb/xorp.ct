# bgp/tools build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(BGP_DEPS bgp
             xif_bgp
             policy_backend
             policy_common
             xipc
             xorp
             comm
             xif_rib
             fea_client
             xif_fea_ifmgr_mirror
             tgts_fea_ifmgr_mirror
             xif_fea_ifmgr_replicator
             tgts_bgp
             xif_finder_event_notifier
            )
if (PROFILER)
    list(APPEND BGP_DEPS xif_profile_client)
endif()

add_executable(print_peers print_peer.cc print_peers.cc)
target_link_libraries(print_peers ${BGP_DEPS})
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    install(TARGETS print_peers RUNTIME DESTINATION ${XORP_TOOLDIR})
endif()

add_executable(print_routes print_routes.cc print_routes_main.cc)
target_link_libraries(print_routes ${BGP_DEPS})
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    install(TARGETS print_routes RUNTIME DESTINATION ${XORP_TOOLDIR})
endif()

add_executable(bgp_print_peers print_peer.cc xorpsh_print_peers.cc)
target_link_libraries(bgp_print_peers ${BGP_DEPS})
install(TARGETS bgp_print_peers RUNTIME DESTINATION ${XORP_TOOLDIR})

add_executable(bgp_print_routes print_routes.cc xorpsh_print_routes.cc)
target_link_libraries(bgp_print_routes ${BGP_DEPS})
install(TARGETS bgp_print_routes RUNTIME DESTINATION ${XORP_TOOLDIR})
