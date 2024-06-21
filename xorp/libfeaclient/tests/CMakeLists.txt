# libfeaclient/tests build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

foreach(T IN ITEMS "local_copy" "remote_copy")
    add_executable(test_${T} test_${T}.cc)
    target_include_directories(test_${T} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../")
    add_test(${T} COMMAND test_${T})
endforeach()

target_link_libraries(test_local_copy xorp
                                      xipc
                                      comm
                                      fea_client
                                    #   tgts_test_fea_rawlink
                                    #   tgts_test_socket4
                                    #   xif_socket4
                                    #   xif_fea_rawlink
                                    #   xif_fea_ifmgr
                                      xif_fea_ifmgr_mirror
                                      xif_fea_ifmgr_replicator
                                      tgts_fea_ifmgr_mirror
                                      )


target_link_libraries(test_remote_copy finder
                                       tgts_test_fea_ifmgr_mirror
                                       xif_fea_ifmgr_replicator
                                       xif_rib
                                       tgts_ribclient
                                       fea_client
                                       xorp
                                       xipc
                                       comm
                                       )
