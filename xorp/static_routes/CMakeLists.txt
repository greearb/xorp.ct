# static routes build
# Copyright (c) 2022-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(STATIC_ROUTES_LIBS fea_client
                       xipc
                       xorp
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
                       xif_finder_event_notifier)

add_library(static_routes static_routes_node.cc
                          static_routes_varrw.cc
                          xrl_static_routes_node.cc)

target_compile_options(static_routes PRIVATE -ftemplate-depth=50)
target_link_libraries(static_routes PRIVATE ${STATIC_ROUTES_LIBS})
install(TARGETS static_routes LIBRARY DESTINATION ${XORP_LIBDIR})

add_executable(xorp_static_routes xorp_static_routes.cc)
target_compile_options(xorp_static_routes PRIVATE -ftemplate-depth=50)
target_link_libraries(xorp_static_routes PRIVATE ${STATIC_ROUTES_LIBS} static_routes)
install(TARGETS xorp_static_routes RUNTIME DESTINATION ${XORP_MODULEDIR})
