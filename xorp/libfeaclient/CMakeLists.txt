# libfeaclient build
# Copyright (c) 2023 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

# add_subdirectory(tests)


add_library(fea_client 
                      ifmgr_atoms.cc
                      ifmgr_cmds.cc
                      ifmgr_cmd_queue.cc
                      ifmgr_xrl_replicator.cc
                      ifmgr_xrl_mirror.cc)

target_link_libraries(fea_client INTERFACE xif_fea_ifmgr_mirror xif_fea_ifmgr_replicator tgts_fea_ifmgr_mirror)

install(TARGETS fea_client RUNTIME DESTINATION lib)


