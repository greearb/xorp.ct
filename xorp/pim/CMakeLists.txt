# pim build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(PIM_LIBS fea_client
             xif_rib
             xif_mld6igmp
             xif_mfea
             xif_fea_rawpkt4
             xif_fea_ifmgr_mirror
             xif_fea_ifmgr_replicator
             xif_cli_manager
             xif_finder_event_notifier
             tgts_fea_ifmgr_mirror
             tgts_pim
             xorp_mrt
             proto
             xipc
             xorp
             comm
)
set(PIM_SRCS "")

if (ENABLE_IPV6)
    list(APPEND PIM_LIBS xif_fea_rawpkt6)
endif()

add_library(pim pim_bsr.cc
                pim_config.cc
                pim_mfc.cc
                pim_mre.cc
                pim_mre_assert.cc
                pim_mre_data.cc
                pim_mre_join_prune.cc
                pim_mre_register.cc
                pim_mre_rpf.cc
                pim_mre_task.cc
                pim_mre_track_state.cc
                pim_mrib_table.cc
                pim_mrt.cc
                pim_mrt_mfc.cc
                pim_mrt_task.cc
                pim_nbr.cc
                pim_node.cc
                pim_node_cli.cc
                pim_proto_assert.cc
                pim_proto_bootstrap.cc
                pim_proto_cand_rp_adv.cc
                pim_proto_graft.cc
                pim_proto_graft_ack.cc
                pim_proto_hello.cc
                pim_proto_join_prune.cc
                pim_proto_join_prune_message.cc
                pim_proto_register.cc
                pim_proto_register_stop.cc
                pim_rp.cc
                pim_scope_zone_table.cc
                pim_vif.cc
                xrl_pim_node.cc)

target_link_libraries(pim ${PIM_LIBS})
install(TARGETS pim LIBRARY DESTINATION ${XORP_LIBDIR})

add_executable(xorp_pimsm4 xorp_pimsm4.cc)
target_link_libraries(xorp_pimsm4 pim ${PIM_LIBS})
install(TARGETS xorp_pimsm4 RUNTIME DESTINATION ${XORP_MODULEDIR})

if(ENABLE_IPV6)
    add_executable(xorp_pimsm6 xorp_pimsm6.cc)
    target_link_libraries(xorp_pimsm6 pim ${PIM_LIBS})
    install(TARGETS xorp_pimsm6 RUNTIME DESTINATION ${XORP_MODULEDIR})
    if (RTLD_ORIGIN)
        set_target_properties(xorp_pimsm6 PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                     INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    endif()
    if (BUILDDIR_RUN)
        add_custom_command(TARGET xorp_pimsm6 POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp_pimsm6>
                            ${XORP_ALIAS_MODULEDIR}/$<TARGET_FILE_NAME:xorp_pimsm6>)
    endif()
endif()

if (RTLD_ORIGIN)
    set_target_properties(pim PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                 INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    set_target_properties(xorp_pimsm4 PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                 INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
endif()
if (BUILDDIR_RUN)
    add_custom_command(TARGET pim POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:pim>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:pim>)
    add_custom_command(TARGET xorp_pimsm4 POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp_pimsm4>
                        ${XORP_ALIAS_MODULEDIR}/$<TARGET_FILE_NAME:xorp_pimsm4>)
endif()
