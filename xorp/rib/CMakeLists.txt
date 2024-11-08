# rib build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later
if (TESTS_PROGRAMS)
    add_subdirectory(tests)
endif()

set(RIB_SRCS "")
set(RIB_LIBS fea_client
             xif_fea_ifmgr_mirror
             xif_fea_ifmgr_replicator
             xif_fti
             xif_rib_client
             xif_finder_event_notifier
             xif_redist4
             xif_redist6
             xif_redist_transaction4
             xif_redist_transaction6
             xif_policy_redist4
             xif_policy_redist6
             tgts_fea_ifmgr_mirror
             tgts_rib
             policy_backend
             policy_common
             finder
             xipc
             xorp
             proto
             comm)

if (NOT DISABLE_PROFILE)
    list(APPEND RIB_SRCS profile_vars.cc)
    list(APPEND RIB_LIBS xif_profile_client)
endif()

add_library(rib protocol.cc
                redist_xrl.cc
                register_server.cc
                rib.cc
                rib_manager.cc
                rib_varrw.cc
                route.cc
                rt_tab_base.cc
                rt_tab_deletion.cc
                rt_tab_extint.cc
                rt_tab_origin.cc
                rt_tab_pol_conn.cc
                rt_tab_pol_redist.cc
                rt_tab_redist.cc
                rt_tab_register.cc
                vifmanager.cc
                xrl_target.cc
                ${RIB_SRCS}
                )
target_link_libraries(rib ${RIB_LIBS})
target_compile_options(rib PRIVATE -ftemplate-depth=50)
install(TARGETS rib LIBRARY DESTINATION ${XORP_LIBDIR})

add_executable(xorp_rib main_rib.cc)
target_link_libraries(xorp_rib rib ${RIB_LIBS})
target_compile_options(xorp_rib PRIVATE -ftemplate-depth=50)
install(TARGETS xorp_rib RUNTIME DESTINATION ${XORP_MODULEDIR})

add_subdirectory(tools)

if (RTLD_ORIGIN)
    set_target_properties(xorp_rib PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                              INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    set_target_properties(rib PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                         INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
endif()
if (BUILDDIR_RUN)
    add_custom_command(TARGET rib POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:rib>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:rib>)
    add_custom_command(TARGET xorp_rib POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp_rib>
                        ${XORP_ALIAS_MODULEDIR}/$<TARGET_FILE_NAME:xorp_rib>)
endif()
