# policy/backend build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(POLICY_BACKEND_SRCS "")
if (NOT DISABLE_PROFILE)
    list(APPEND POLICY_BACKEND_SRCS policy_profiler.cc)
endif()

# Build to binary dir
# cmake_policy(SET CMP0088 OLD)
# cmake_policy(SET CMP0098 OLD)
find_package(BISON 3.5.1 REQUIRED)
# if (BISON_FOUND)
BISON_TARGET(backendParser backend.yy ${CMAKE_CURRENT_SOURCE_DIR}/y.policy_backend_parser_tab.cc
             COMPILE_FLAGS "-d -ppolicy_backend_parser"
             VERBOSE
             DEFINES_FILE ${CMAKE_CURRENT_SOURCE_DIR}/y.policy_backend_parser_tab.hh)

message(STATUS "BISON policy backend targets created")
# endif(BISON_FOUND)
# Build to binary dir

find_package(FLEX REQUIRED)
if (FLEX_FOUND)
# Something weird happened here, flex ignored path to binary dir - ~zagura @ 2023-10-08
flex_target(backendScanner backend.ll
                         ${CMAKE_CURRENT_SOURCE_DIR}/lex.policy_backend_parser.cc
                         COMPILE_FLAGS "-Ppolicy_backend_parser"
                         DEFINES_FILE ${CMAKE_CURRENT_SOURCE_DIR}/lex.policy_backend_parser.hh)
ADD_FLEX_BISON_DEPENDENCY(backendScanner backendParser)
message(STATUS "Flex policy backend targets built")
endif(FLEX_FOUND)

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
