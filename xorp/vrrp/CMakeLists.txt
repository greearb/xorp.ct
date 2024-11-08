# vrrp build
# Copyright (c) 2021-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later
# License: see ${XORP}/LICENCE.gpl

file(GLOB VRRP_CXX_SRCS arpd.cc
                        vrrp.cc
                        vrrp_packet.cc
                        vrrp_target.cc
                        vrrp_vif.cc)

# add_executable(vrrp ${VRRP_CXX_SRCS})
# target_link_libraries(vrrp xorp xipc comm )
set(VRRP_LIBS tgts_vrrp
              fea_client
              xif_fea_ifmgr
              xif_fea_ifmgr_mirror
              xif_fea_ifmgr_replicator
              xif_fea_rawlink
              xif_fea_rawpkt4
              tgts_fea_ifmgr_mirror
              xipc
              xorp
              proto
              comm
              )

add_library(vrrp ${VRRP_CXX_SRCS})
target_link_libraries(vrrp ${VRRP_LIBS})
install(TARGETS vrrp LIBRARY DESTINATION ${XORP_LIBDIR})

add_executable(xorp_vrrp xorp_vrrp.cc)
target_link_libraries(xorp_vrrp vrrp ${VRRP_LIBS})
install(TARGETS xorp_vrrp RUNTIME DESTINATION ${XORP_MODULEDIR})
if (RTLD_ORIGIN)
    set_target_properties(xorp_vrrp PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                               INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    set_target_properties(vrrp PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                          INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
endif()
if (BUILDDIR_RUN)
    add_custom_command(TARGET vrrp POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:vrrp>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:vrrp>)
    add_custom_command(TARGET xorp_vrrp POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp_vrrp>
                        ${XORP_ALIAS_MODULEDIR}/$<TARGET_FILE_NAME:xorp_vrrp>)
endif()
