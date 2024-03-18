# rip/tools build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(RIPTOOLS_LIBS xif_rip
                  xif_ripng
                  xipc
                  xorp
                  proto
                  comm)

add_executable(rip_show_peer_stats show_peer_stats.cc common.cc)
target_link_libraries(rip_show_peer_stats PRIVATE ${RIPTOOLS_LIBS})
install(TARGETS rip_show_peer_stats RUNTIME DESTINATION ${XORP_TOOLDIR})

add_executable(rip_show_stats show_stats.cc common.cc)
target_link_libraries(rip_show_stats PRIVATE ${RIPTOOLS_LIBS})
install(TARGETS rip_show_stats RUNTIME DESTINATION ${XORP_TOOLDIR})
