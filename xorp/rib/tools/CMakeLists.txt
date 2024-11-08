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
add_executable(rib_show_routes show_routes.cc)

foreach(TOOL IN ITEMS rib_show_distances rib_show_routes)
    target_link_libraries(${TOOL} ${RIB_TOOLS_LIBS})
    install(TARGETS ${TOOL} RUNTIME DESTINATION ${XORP_TOOLDIR})
    if (RTLD_ORIGIN)
        set_target_properties(${TOOL} PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                 INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    endif()
endforeach()

if (BUILDDIR_RUN)
    add_custom_command(TARGET rib_show_distances POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:rib_show_distances>
                        ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:rib_show_distances>)
    add_custom_command(TARGET rib_show_routes POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:rib_show_routes>
                        ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:rib_show_routes>)
endif()
