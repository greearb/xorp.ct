# mld6igmp build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later



set(MLD6IGMP_SRCS mld6igmp_config.cc
                  mld6igmp_group_record.cc
                  mld6igmp_node.cc
                  mld6igmp_node_cli.cc
                  mld6igmp_proto.cc
                  mld6igmp_source_record.cc
                  mld6igmp_vif.cc
                  xrl_mld6igmp_node.cc
                  )


set(MLD6IGMP_LIBS fea_client
                  xif_mld6igmp_client
                  xif_fea_rawpkt4
                  xif_fea_ifmgr_mirror
                  xif_fea_ifmgr_replicator
                  xif_cli_manager
                  xif_finder_event_notifier
                  tgts_fea_ifmgr_mirror
                  tgts_mld6igmp
                  xorp_mrt
                  proto
                  xipc
                  xorp
                  comm)

if (ENABLE_IPV6)
    list(APPEND MLD6IGMP_LIBS xif_fea_rawpkt6)
endif()

add_library(xorp_mld6igmp ${MLD6IGMP_SRCS})
target_link_libraries(xorp_mld6igmp ${MLD6IGMP_LIBS})


install(TARGETS xorp_mld6igmp LIBRARY DESTINATION ${XORP_LIBDIR})

add_executable(xorp_igmp xorp_igmp.cc)
target_link_libraries(xorp_igmp xorp_mld6igmp ${MLD6IGMP_LIBS})
install(TARGETS xorp_igmp RUNTIME DESTINATION ${XORP_MODULEDIR})


add_executable(xorp_mld xorp_mld.cc)
target_link_libraries(xorp_mld xorp_mld6igmp ${MLD6IGMP_LIBS})
install(TARGETS xorp_mld RUNTIME DESTINATION ${XORP_MODULEDIR})
