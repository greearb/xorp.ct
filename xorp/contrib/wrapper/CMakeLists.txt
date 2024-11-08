# contrib/wrapper build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(WRAPPER_LIBS tgts_wrapper4
                 policy_backend
                 policy_common
                 fea_client
                 xif_rib
                 xif_socket4
                 xif_finder_event_notifier
                 xif_fea_ifmgr_mirror
                 xif_fea_ifmgr_replicator
                 xif_fea_rawpkt4
                 tgts_fea_ifmgr_mirror
                 xipc
                 xorp
                 proto
                 comm)

if (ENABLE_IPV6)
    list(APPEND WRAPPER_LIBS xif_fea_rawpkt6)
endif()

if (NOT DISABLE_PROFILE)
    list(APPEND WRAPPER_LIBS xif_profile_client)
endif()

add_library(wrapper wrapper.cc wrapperpolicy.cc)
target_link_libraries(wrapper ${WRAPPER_LIBS})
install(TARGETS wrapper LIBRARY DESTINATION ${XORP_LIBDIR})

add_library(wrapperxrl xorp_io.cc xorp_wrapper4.cc)
target_link_libraries(wrapperxrl ${WRAPPER_LIBS} wrapper)
install(TARGETS wrapperxrl LIBRARY DESTINATION ${XORP_LIBDIR})


add_executable(xorp_wrapper4 wrappermain.cc)
target_link_libraries(xorp_wrapper4 ${WRAPPER_LIBS} wrapper wrapperxrl)
install(TARGETS xorp_wrapper4 RUNTIME DESTINATION ${XORP_MODULEDIR})

if (RTLD_ORIGIN)
    set_target_properties(wrapper PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                             INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    set_target_properties(wrapperxrl PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    set_target_properties(xorp_wrapper4 PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                   INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
endif()
if (BUILDDIR_RUN)
    add_custom_command(TARGET wrapper POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:wrapper>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:wrapper>)
    add_custom_command(TARGET wrapperxrl POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:wrapperxrl>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:wrapperxrl>)
    add_custom_command(TARGET xorp_wrapper4 POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp_wrapper4>
                        ${XORP_ALIAS_MODULEDIR}/$<TARGET_FILE_NAME:xorp_wrapper4>)
endif()
