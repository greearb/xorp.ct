# rip build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

if (TESTS_PROGRAMS)
    add_subdirectory(tests)
endif()

add_subdirectory(tools)

set(RIP_SRCS auth.cc
             output_table.cc
             output_updates.cc
             packet_queue.cc
             peer.cc
             port.cc
             redist.cc
             rib_notifier_base.cc
             rip_varrw.cc
             route_db.cc
             route_entry.cc
             update_queue.cc
             xrl_config.cc
             xrl_port_manager.cc
             xrl_port_io.cc
             xrl_process_spy.cc
             xrl_redist_manager.cc
             xrl_rib_notifier.cc

    )

set(RIP_LIBS
             xif_rib
             xif_finder_event_notifier
             fea_client
             xif_fea_ifmgr_mirror
             xif_fea_ifmgr_replicator
             tgts_fea_ifmgr_mirror
             policy_backend
             policy_common
             proto
             xipc
             comm
             xorp
    )

add_library(rip ${RIP_SRCS})
target_link_libraries(rip ${RIP_LIBS} xif_socket4)
install(TARGETS rip LIBRARY DESTINATION ${XORP_LIBDIR})
target_compile_definitions(rip PRIVATE INSTANTIATE_IPV4)

add_executable(xorp_rip xrl_target_rip.cc xorp_rip_main.cc)
target_link_libraries(xorp_rip ${RIP_LIBS}
                               tgts_rip
                               xif_socket4
                               rip
                               )

target_compile_definitions(xorp_rip PRIVATE INSTANTIATE_IPV4)
install(TARGETS xorp_rip RUNTIME DESTINATION ${XORP_MODULEDIR})


if (RTLD_ORIGIN)
    set_target_properties(rip PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                   INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    set_target_properties(xorp_rip PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                   INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
endif()
if (BUILDDIR_RUN)
    add_custom_command(TARGET rip POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:rip>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:rip>)
    add_custom_command(TARGET xorp_rip POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp_rip>
                        ${XORP_ALIAS_MODULEDIR}/$<TARGET_FILE_NAME:xorp_rip>)
endif()

if (ENABLE_IPV6)
    add_library(ripng ${RIP_SRCS})
    target_link_libraries(ripng ${RIP_LIBS} xif_socket6)
    install(TARGETS ripng LIBRARY DESTINATION ${XORP_LIBDIR})
    target_compile_definitions(ripng PRIVATE INSTANTIATE_IPV6)

    add_executable(xorp_ripng xrl_target_ripng.cc xorp_rip_main.cc)
    target_link_libraries(xorp_ripng ${RIP_LIBS}
                                     tgts_ripng
                                     xif_socket6
                                     ripng
                                     )

    target_compile_definitions(xorp_ripng PRIVATE INSTANTIATE_IPV6)
    install(TARGETS xorp_ripng RUNTIME DESTINATION ${XORP_MODULEDIR})
    if (RTLD_ORIGIN)
        set_target_properties(ripng PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                       INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
        set_target_properties(xorp_ripng PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                       INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    endif()
    if (BUILDDIR_RUN)
        add_custom_command(TARGET ripng POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:ripng>
                            ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:ripng>)
        add_custom_command(TARGET xorp_ripng POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp_ripng>
                            ${XORP_ALIAS_MODULEDIR}/$<TARGET_FILE_NAME:xorp_ripng>)
    endif()
endif()


