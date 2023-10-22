# static routes build
# Copyright (c) 2022-2023 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(static_routes  xorp_static_routes.cc
                            static_routes_node.cc
                            static_routes_varrw.cc
                            xrl_static_routes_node.cc
                            )



target_compile_options(static_routes PRIVATE -ftemplate-depth=50)

target_include_directories(static_routes PRIVATE ${XORP_LIBDIR})
target_link_libraries(static_routes PRIVATE fea_client
                                            xipc
                                            xorp
                                            policy
                                            policy_backend
                                            policy_common
                                            comm
                                            crypto
                                            proto
                                            xif_fea_ifmgr_mirror
                                            xif_fea_ifmgr_replicator
                                            xif_finder_event_notifier
                                            tgts_fea_ifmgr_mirror
                                            tgts_static_routes
                                            xif_rib
                                            xif_mfea
                                            xif_finder_event_notifier
                                            )

install(TARGETS static_routes RUNTIME DESTINATION sbin)
