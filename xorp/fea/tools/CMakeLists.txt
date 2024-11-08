# fea tools build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(fea_show_interfaces show_interfaces.cc)
target_include_directories(fea_show_interfaces PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fea_show_interfaces
                                          xif_fea_ifmgr_mirror
                                          xif_fea_ifmgr_replicator
                                          tgts_fea_ifmgr_mirror
                                          fea_client
                                          xorp
                                          xipc
                                          proto
                                          comm)
install(TARGETS fea_show_interfaces RUNTIME DESTINATION ${XORP_TOOLDIR})


if (RTLD_ORIGIN)
    set_target_properties(fea_show_interfaces PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                              INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")

endif()

if (BUILDDIR_RUN)
    add_custom_command(TARGET fea_show_interfaces POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:fea_show_interfaces>
                        ${XORP_ALIAS_TOOLDIR}/$<TARGET_FILE_NAME:fea_show_interfaces>)
endif()
