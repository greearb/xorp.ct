# rib build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later
if (TEST_PROGRAMS)
    add_subdirectory(tests)
endif()

set(RIB_SRCS "")
set(RIB_LIBS "")

if (NOT DISABLE_PROFILE)
    list(APPEND RIB_SRCS profile_vars.cc)
    list(APPEND RIB_LIBS xif_profile_client)
endif()


add_executable(xorp_rib
                    protocol.cc
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
                    main_rib.cc
                    ${RIB_SRCS}
              )

target_link_libraries(xorp_rib
                            fea_client
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
                            comm
                            ${RIB_LIBS}
                      )

target_compile_options(xorp_rib PRIVATE -ftemplate-depth=50)
install(TARGETS xorp_rib RUNTIME DESTINATION ${XORP_MODULEDIR})

add_subdirectory(tools)
