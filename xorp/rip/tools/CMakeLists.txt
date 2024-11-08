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
add_executable(rip_show_stats show_stats.cc common.cc)

foreach(TOOL IN ITEMS rip_show_stats rip_show_peer_stats)
    target_link_libraries(${TOOL} PRIVATE ${RIPTOOLS_LIBS})
    install(TARGETS ${TOOL} RUNTIME DESTINATION ${XORP_TOOLDIR})
    if (RTLD_ORIGIN)
        set_target_properties(${TOOL} PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                 INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    endif()
endforeach()

if (BUILDDIR_RUN)
    add_custom_command(TARGET rip_show_peer_stats POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:rip_show_peer_stats>
                        ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:rip_show_peer_stats>)
    add_custom_command(TARGET rip_show_stats POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:rip_show_stats>
                        ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:rip_show_stats>)
endif()
