# bgp/tools build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(BGP_DEPS bgp
             xif_bgp
             policy_backend
             policy_common
             xipc
             xorp
             comm
             xif_rib
             fea_client
             xif_fea_ifmgr_mirror
             tgts_fea_ifmgr_mirror
             xif_fea_ifmgr_replicator
             tgts_bgp
             xif_finder_event_notifier
            )
if (ENABLE_PROFILE)
    list(APPEND BGP_DEPS xif_profile_client)
endif()

add_executable(print_peers print_peer.cc print_peers.cc)
target_link_libraries(print_peers ${BGP_DEPS})
if ($<CONFIG:Debug>)
    install(TARGETS print_peers RUNTIME DESTINATION ${XORP_TOOLDIR})
endif()

add_executable(print_routes print_routes.cc print_routes_main.cc)
target_link_libraries(print_routes ${BGP_DEPS})
if ($<CONFIG:Debug>)
    install(TARGETS print_routes RUNTIME DESTINATION ${XORP_TOOLDIR})
endif()

add_executable(bgp_print_peers print_peer.cc xorpsh_print_peers.cc)
target_link_libraries(bgp_print_peers ${BGP_DEPS})
install(TARGETS bgp_print_peers RUNTIME DESTINATION ${XORP_TOOLDIR})

add_executable(bgp_print_routes print_routes.cc xorpsh_print_routes.cc)
target_link_libraries(bgp_print_routes ${BGP_DEPS})
install(TARGETS bgp_print_routes RUNTIME DESTINATION ${XORP_TOOLDIR})

foreach(TARGET IN ITEMS bgp_print_peers bgp_print_routes)
    if (RTLD_ORIGIN)
        set_target_properties(${TARGET} PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                   INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    endif()
endforeach()

if($<CONFIG:Debug>)
    foreach(TARGET IN ITEMS print_peers print_routes)
        if (RTLD_ORIGIN)
            set_target_properties(${TARGET} PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                       INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
        endif()
    endforeach()
endif()

if (BUILDDIR_RUN)
    if ($<CONFIG:Debug>)
        add_custom_command(TARGET print_peers POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:print_peers>
                            ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:print_peers>)
        add_custom_command(TARGET print_routes POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:print_routes>
                            ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:print_routes>)
    endif()
    add_custom_command(TARGET bgp_print_peers POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:bgp_print_peers>
                        ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:bgp_print_peers>)
    add_custom_command(TARGET bgp_print_routes POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:bgp_print_routes>
                        ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:bgp_print_routes>)
endif()
