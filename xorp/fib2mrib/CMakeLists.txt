# fib2mrib build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later



set(FIB2MRIB_SRCS fib2mrib_node.cc
                  fib2mrib_varrw.cc
                  xrl_fib2mrib_node.cc
                  xorp_fib2mrib.cc)

add_executable(xorp_fib2mrib ${FIB2MRIB_SRCS})
target_link_libraries(xorp_fib2mrib tgts_fib2mrib
                                    policy_backend
                                    policy_common
                                    fea_client
                                    xif_finder_event_notifier
                                    xif_fea_ifmgr_mirror
                                    xif_fea_ifmgr_replicator
                                    xif_fti
                                    xif_fea_fib
                                    xif_rib
                                    tgts_fea_ifmgr_mirror
                                    xipc
                                    xorp
                                    proto
                                    comm)

install(TARGETS xorp_fib2mrib RUNTIME DESTINATION ${XORP_MODULEDIR})
