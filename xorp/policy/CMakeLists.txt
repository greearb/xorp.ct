# policy build
# Copyright (c) 2023 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_subdirectory(common)
add_subdirectory(backend)
set(POLICY_SRCS "")

# Build to binary dir
# cmake_policy(SET CMP0088 OLD)
# cmake_policy(SET CMP0098 OLD)
find_package(BISON 3.5.1 REQUIRED)
# if (BISON_FOUND)
BISON_TARGET(policyParser policy.yy ${CMAKE_CURRENT_SOURCE_DIR}/y.policy_parser_tab.cc
             COMPILE_FLAGS "-d -ppolicy_parser"
             VERBOSE
             DEFINES_FILE ${CMAKE_CURRENT_SOURCE_DIR}/y.policy_parser_tab.hh)

message(STATUS "BISON policy policy targets created")
# endif(BISON_FOUND)
# Build to binary dir

find_package(FLEX REQUIRED)
if (FLEX_FOUND)
# Something weird happened here, flex ignored path to binary dir - ~zagura @ 2023-10-08
flex_target(policyScanner policy.ll
                         ${CMAKE_CURRENT_SOURCE_DIR}/lex.policy_parser.cc
                         COMPILE_FLAGS "-Ppolicy_parser"
                         DEFINES_FILE ${CMAKE_CURRENT_SOURCE_DIR}/lex.policy_parser.hh)
# ADD_FLEX_BISON_DEPENDENCY(policyScanner policyParser)
message(STATUS "Flex policy policy targets built")
endif(FLEX_FOUND)
set_source_files_properties(
                            ${BISON_policyParser_OUTPUTS}
                            ${FLEX_policyScanner_OUTPUTS}
                            PROPERTIES
                            COMPILE_FLAGS -Wno-sign-compare
                            COMPILE_FLAGS -Wno-effc++)

add_executable(policy
                     ${BISON_policyParser_OUTPUTS}
                     ${FLEX_policyScanner_OUTPUTS}
                     code.cc
                     code_generator.cc
                     code_list.cc
                     configuration.cc
                     dependency.cc
                     export_code_generator.cc
                     filter_manager.cc
                     parser.cc
                     policy_list.cc
                     policy_map.cc
                     policy_statement.cc
                     policy_target.cc
                     process_watch.cc
                     protocol_map.cc
                     semantic_varrw.cc
                     set_map.cc
                     source_match_code_generator.cc
                     term.cc
                     test_varrw.cc
                     var_map.cc
                     visitor_dep.cc
                     visitor_printer.cc
                     visitor_semantic.cc
                     visitor_test.cc
                     xrl_target.cc
                     xorp_policy.cc)

target_link_libraries(policy policy_backend
                             policy_common
                             tgts_policy
                             xif_policy_backend
                             xif_finder_event_notifier
                             xif_rib
                             xipc
                             xorp
                             proto
                             comm
                             )

install(TARGETS policy RUNTIME DESTINATION sbin)
