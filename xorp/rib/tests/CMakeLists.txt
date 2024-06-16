# rib/tests build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later


set(RIBTESTS tgts_rip
             rib
             # xorp_rib_xrl
             xif_rib
             xif_finder_event_notifier
             fea_client
             xif_fea_ifmgr_mirror
             xif_fea_ifmgr_replicator
             xif_fti
             xif_rib_client
             tgts_fea_ifmgr_mirror
             xif_socket4
             policy_backend
             policy_common
             proto
             finder
             xipc
             xorp
             xif_redist4
             xif_redist6
             xif_redist_transaction4
             xif_redist_transaction6
             xif_policy_redist4
             xif_policy_redist6
            #  xif_profile_client
             tgts_rib
             comm)

if (NOT DISABLE_PROFILE)
    # list(APPEND RIB_SRCS profile_vars.cc)
    list(APPEND RIBTESTS xif_profile_client)
endif()

add_executable(test_rib_deletion test_deletion.cc rt_tab_expect.cc)
target_link_libraries(test_rib_deletion ${RIBTESTS})
target_include_directories(test_rib_deletion PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../")
add_test(rib_deletion COMMAND test_rib_deletion)

add_executable(test_rib_redist test_redist.cc)
target_link_libraries(test_rib_redist ${RIBTESTS})
target_include_directories(test_rib_redist PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../")
add_test(rib_redist COMMAND test_rib_redist)

add_executable(test_rib_register test_register.cc dummy_register_server.cc)
target_link_libraries(test_rib_register ${RIBTESTS})
target_include_directories(test_rib_register PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../")
add_test(rib_register COMMAND test_rib_register)

add_executable(test_rib_direct test_rib_direct.cc ../parser.cc dummy_register_server.cc)
target_link_libraries(test_rib_direct ${RIBTESTS})
target_include_directories(test_rib_direct PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../")
add_test(rib_direct COMMAND "/bin/bash -c" test_rib_direct "< ${CMAKE_CURRENT_LIST_DIR}/commands")

add_executable(test_rib_xrls test_rib_xrls.cc ../parser.cc dummy_register_server.cc)
target_link_libraries(test_rib_xrls ${RIBTESTS})
target_include_directories(test_rib_xrls PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../")
add_test(rib_xrls COMMAND "/bin/bash -c" test_rib_xrls "< ${CMAKE_CURRENT_LIST_DIR}/commands")

