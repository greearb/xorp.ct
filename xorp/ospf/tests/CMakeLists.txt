# ospf/tests build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later


set(OSPFTESTS
            ospf
            tgts_ospfv2
            tgts_ospfv3
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
            # finder
        )

foreach(T IN ITEMS
                    "checksum"
                    "packet"
                    "peering"
                    "routing"
                    #"routing_database" # NOTYET
                    #"routing_interactive" # NOTYET
                    "routing_table")
    add_executable(test_ospf_${T} test_${T}.cc)
    target_link_libraries(test_ospf_${T} ${OSPFTESTS})
    target_include_directories(test_ospf_${T} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../")
    add_test(${T} COMMAND test_ospf_${T})
endforeach()
