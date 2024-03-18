# rib/tools build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later


set(RIB_TOOLS_LIBS  xif_finder_event_notifier
                    xif_rib
                    tgts_show_distances
                    tgts_show_routes
                    xipc
                    xorp
                    proto
                    comm)



add_executable(rib_show_distances show_distances.cc)
target_link_libraries(rib_show_distances ${RIB_TOOLS_LIBS})
install(TARGETS rib_show_distances RUNTIME DESTINATION ${XORP_TOOLDIR})

add_executable(rib_show_routes show_routes.cc)
target_link_libraries(rib_show_routes ${RIB_TOOLS_LIBS})
install(TARGETS rib_show_routes RUNTIME DESTINATION ${XORP_TOOLDIR})
