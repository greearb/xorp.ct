# contrib/olsr build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_subdirectory(tools)
set(OLSR_LIBS tgts_olsr4
              policy_backend
              policy_common
              fea_client
              xif_rib
              xif_socket4
              xif_finder_event_notifier
              xif_fea_ifmgr_mirror
              xif_fea_ifmgr_replicator
              xif_fea_rawpkt4
              tgts_fea_ifmgr_mirror
              xipc
              xorp
              proto
              comm)

if (ENABLE_IPV6)
    list(APPEND OLSR_LIBS xif_fea_rawpkt6)
endif()

if (NOT DISABLE_PROFILE)
    list(APPEND OLSR_LIBS xif_profile_client)
endif()

add_library(olsr external.cc
                 face.cc
                 face_manager.cc
                 link.cc
                 message.cc
                 neighbor.cc
                 neighborhood.cc
                 olsr.cc
                 olsr_types.cc
                 policy_varrw.cc
                 route_manager.cc
                 topology.cc
                 twohop.cc
                 )

target_link_libraries(olsr ${OLSR_LIBS})
install(TARGETS olsr LIBRARY DESTINATION ${XORP_LIBDIR})

add_library(olsrxrl xrl_io.cc
                    xrl_port.cc
                    xrl_queue.cc
                    xrl_target.cc
                    )

target_link_libraries(olsrxrl ${OLSR_LIBS} olsr)
install(TARGETS olsrxrl LIBRARY DESTINATION ${XORP_LIBDIR})


add_executable(xorp_olsr4 xorp_olsr.cc)
target_link_libraries(xorp_olsr4 ${OLSR_LIBS} olsr olsrxrl)
install(TARGETS xorp_olsr4 RUNTIME DESTINATION ${XORP_MODULEDIR})
