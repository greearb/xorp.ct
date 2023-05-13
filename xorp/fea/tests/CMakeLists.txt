# fea/tests build
# Copyright (c) 2023 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

message(NOTICE "Fea tests disabled due to missing implementations")
# Tests targets missing common_0_1_startup interface implementation
# fea_rawlink missing link deps
# xrl_test_sockets4_udp failed to compile
foreach(FEA_TEST "fea_rawlink" "xrl_sockets4_tcp" "xrl_sockets4_udp")
   add_executable("test_${FEA_TEST}" "test_${FEA_TEST}.cc")
   target_include_directories("test_${FEA_TEST}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../")
   target_link_libraries("test_${FEA_TEST}" xorp
                                            xipc
                                            fea_client
                                            tgts_test_fea_rawlink
                                            tgts_test_socket4
                                            xif_socket4
                                            xif_fea_rawlink
                                            xif_fea_ifmgr
                                            xif_fea_ifmgr_mirror
                                            )
endforeach()
