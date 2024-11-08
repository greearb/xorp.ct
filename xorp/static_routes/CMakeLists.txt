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

if (RTLD_ORIGIN)
    set_target_properties(static_routes PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                   INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    set_target_properties(xorp_static_routes PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
endif()
if (BUILDDIR_RUN)
    add_custom_command(TARGET xorp_static_routes POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp_static_routes>
                        ${XORP_ALIAS_MODULEDIR}/$<TARGET_FILE_NAME:xorp_static_routes>)
    add_custom_command(TARGET static_routes POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:static_routes>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:static_routes>)
endif()
