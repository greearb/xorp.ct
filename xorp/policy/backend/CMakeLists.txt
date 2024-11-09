# policy/backend build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(POLICY_BACKEND_SRCS "")
if (ENABLE_PROFILE)
    list(APPEND POLICY_BACKEND_SRCS policy_profiler.cc)
endif()

bison_target(backendParser backend.yy ${CMAKE_CURRENT_BINARY_DIR}/y.policy_backend_parser_tab.cc
             COMPILE_FLAGS "-d -ppolicy_backend_parser"
             VERBOSE
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/y.policy_backend_parser_tab.hh)

flex_target(backendScanner backend.ll
                         ${CMAKE_CURRENT_BINARY_DIR}/lex.policy_backend_parser.cc
                         COMPILE_FLAGS "-Ppolicy_backend_parser"
                         DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/lex.policy_backend_parser.hh)
ADD_FLEX_BISON_DEPENDENCY(backendScanner backendParser)

set_source_files_properties(
                            ${BISON_backendParser_OUTPUTS}
                            ${FLEX_backendScanner_OUTPUTS}
                            PROPERTIES
                            COMPILE_FLAGS -Wno-sign-compare
                            COMPILE_FLAGS -Wno-effc++)

add_library(policy_backend
                           ${BISON_backendParser_OUTPUTS}
                           ${FLEX_backendScanner_OUTPUTS}
                           iv_exec.cc
                           policy_filter.cc
                           policy_filters.cc
                           policy_redist_map.cc
                           policytags.cc
                           set_manager.cc
                           single_varrw.cc
                           version_filter.cc
                           version_filters.cc
                           ../../bgp/aspath.cc
                           ${POLICY_BACKEND_SRCS}
                          )

target_link_libraries(policy_backend OpenSSL::Crypto
                                     xorp
                                     comm
                                     policy_common
                                     proto
                                     xipc
                      )

install(TARGETS policy_backend LIBRARY DESTINATION ${XORP_LIBDIR})
if (RTLD_ORIGIN)
    set_target_properties(policy_backend PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                    INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
endif()
if (BUILDDIR_RUN)
    add_custom_command(TARGET policy_backend POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:policy_backend>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:policy_backend>)
endif()
