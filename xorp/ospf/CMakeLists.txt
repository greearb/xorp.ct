# ospf build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later
if (TESTS_PROGRAMS)
    add_subdirectory(tests)
endif()

add_subdirectory(tools)

set(OSPFLIB_SRCS "")
set(OSPFLIB_LIBS tgts_ospfv2
                 policy_backend
                 policy_common
                 fea_client
                 xif_rib
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
    list(APPEND OSPFLIB_LIBS xif_fea_rawpkt6
                             tgts_ospfv3)
    list(APPEND OSPFLIB_SRCS xrl_target3.cc)
endif()

if (ENABLE_PROFILE)
    list(APPEND OSPFLIB_LIBS xif_profile_client)
endif()

add_library(ospf auth.cc
                 area_router.cc
                 external.cc
                 fletcher_checksum.cc
                 lsa.cc
                 ospf.cc
                 packet.cc
                 peer_manager.cc
                 peer.cc
                 policy_varrw.cc
                 routing_table.cc
                 xrl_io.cc
                 xrl_target.cc
                 vlink.cc
                 ${OSPFLIB_SRCS})

target_link_libraries(ospf ${OSPFLIB_LIBS})
install(TARGETS ospf LIBRARY DESTINATION ${XORP_LIBDIR})

add_executable(xorp_ospfv2 xorp_ospfv2.cc)
target_link_libraries(xorp_ospfv2 ospf ${OSPFLIB_LIBS})
install(TARGETS xorp_ospfv2 RUNTIME DESTINATION ${XORP_MODULEDIR})

if (ENABLE_IPV6)
    add_executable(xorp_ospfv3 xorp_ospfv3.cc)
    target_link_libraries(xorp_ospfv3 ospf ${OSPFLIB_LIBS})
    install(TARGETS xorp_ospfv3 RUNTIME DESTINATION ${XORP_MODULEDIR})
endif()

if (RTLD_ORIGIN)
    set_target_properties(ospf PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                          INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    set_target_properties(xorp_ospfv2 PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                 INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    if (ENABLE_IPV6)
        set_target_properties(xorp_ospfv3 PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                     INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    endif()
endif()
if (BUILDDIR_RUN)
    add_custom_command(TARGET ospf POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:ospf>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:ospf>)
    add_custom_command(TARGET xorp_ospfv2 POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp_ospfv2>
                        ${XORP_ALIAS_MODULEDIR}/$<TARGET_FILE_NAME:xorp_ospfv2>)
    if (ENABLE_IPV6)
        add_custom_command(TARGET xorp_ospfv3 POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp_ospfv3>
                            ${XORP_ALIAS_MODULEDIR}/$<TARGET_FILE_NAME:xorp_ospfv3>)
    endif()
endif()
