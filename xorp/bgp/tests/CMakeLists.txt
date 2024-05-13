# bgp/tests build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later


set(BGPTESTS
            xorp_ospf
            tgts_ospfv2
            tgts_ospfv3
            xif_rip
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

foreach(T IN ITEMS "cache" "decision" "deletion" "dump" "fanout" "filter" "main"
    "next_hop_resolver" "nhlookup" "packet" "packet_coding" "peer_data" "plumbing"
    "policy" "ribin" "ribout" "subnet_route")
    add_executable(test_bgp_${T} test_${T}.cc)
    target_link_libraries(test_bgp_${T} ${BGPTESTS})
    add_test(${T} COMMAND test_bgp_${T})
endforeach()
