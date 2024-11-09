# bgp/harness build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later


set(HARNESS_ADD_LIBS "")

if (ENABLE_PROFILE)
    list(APPEND HARNESS_ADD_LIBS xif_profile_client)
endif()

set(HARNESS_SCRIPTS args.sh
                    test_path_attribute1.sh
                    test_rib1.sh
                    test_routing2.sh
                    inject.sh
                    test_path_attribute2.sh
                    test_rib_fea1.sh
                    test_terminate.sh
                    notification_codes.sh
                    test_path_attribute3.sh
                    test_route_flap_damping1.sh
                    xrl_shell_funcs.sh
                    setup_paths.sh
                    soak.sh
                    test_busted
                    test_peering1.sh
                    test_route_reflection1.sh
                    test1.sh
                    test_peering2.sh
                    test_route_reflection2.sh
                    test2.sh
                    test_peering3.sh
                    test_routing1.sh
                    harness.py
                    lookup.py
                    NOTES
                    originate_routes.pl
)

foreach(SCRIPT IN ITEMS ${HARNESS_SCRIPTS})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPT}
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${SCRIPT} USE_SOURCE_PERMISSIONS)
endforeach()

set(HARNESS_LIBS    bgp
                    xif_bgp
                    xipc
                    xorp
                    comm
                    tgts_test_peer
                    xif_test_peer
                    tgts_coord
                    xif_coord
                    xif_rib
                    xif_datain
                    fea_client
                    policy_backend
                    policy_common
                    xif_fea_ifmgr_mirror
                    xif_fea_ifmgr_replicator
                    tgts_fea_ifmgr_mirror
                    tgts_bgp
                    xif_finder_event_notifier
                    ${HARNESS_ADD_LIBS}
                )

add_executable(test_coord bgppp.cc command.cc coord.cc peer.cc trie.cc)
target_link_libraries(test_coord ${HARNESS_LIBS})
add_test(coord COMMAND test_coord)

add_executable(test_peer bgppp.cc test_peer.cc)
target_link_libraries(test_peer ${HARNESS_LIBS})
add_test(peer COMMAND test_peer)


add_executable(test_trie bgppp.cc test_trie.cc trie.cc)
target_link_libraries(test_trie ${HARNESS_LIBS})
add_test(trie COMMAND test_trie)

foreach(T IN ITEMS  "busted"
                    "peering1.sh"
                    "peering2.sh"
                    "peering3.sh"
                    "routing1.sh"
                    "routing2.sh"
                    "rib1.sh"
                    "rib_fea1.sh"
                    "path_attribute1.sh"
                    "path_attribute2.sh"
                    "path_attribute3.sh"
                    "route_reflection1.sh"
                    "route_reflection2.sh"
                    "route_flap_damping1.sh"
                    "terminate.sh")
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_${T}
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_${T} USE_SOURCE_PERMISSIONS)
endforeach()
