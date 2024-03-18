# rib/tests build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later


# set(RIPTESTS tgts_rip
#              rip
#              ripng
#              xif_rip
#              xif_finder_event_notifier
#              fea_client
#              xif_fea_ifmgr_mirror
#              xif_fea_ifmgr_replicator
#              tgts_fea_ifmgr_mirror
#              xif_socket4
#              policy_backend
#              policy_common
#              xipc
#              xorp
#              comm)


# foreach(T IN ITEMS "auth" "outputs" "packets" "request" "route_walk" "timers" "update_queue")
#     add_executable(test_${T} test_${T}.cc)
#     target_link_libraries(test_${T} ${RIPTESTS})
#     add_test(${T} COMMAND test_${T})
# endforeach()
