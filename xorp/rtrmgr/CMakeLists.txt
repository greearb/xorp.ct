# rtrmgr build
# Copyright (c) 2021-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later


# Build to binary dir
# cmake_policy(SET CMP0088 NEW)
# cmake_policy(SET CMP0098 NEW)
find_package(BISON 3.5.1 REQUIRED)
# if (BISON_FOUND)
BISON_TARGET(tpltParser template.yy ${CMAKE_CURRENT_BINARY_DIR}/y.tplt_tab.cc
             COMPILE_FLAGS "-d -ptplt"
             VERBOSE
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/y.tplt_tab.hh)
BISON_TARGET(bootParser boot.yy ${CMAKE_CURRENT_BINARY_DIR}/y.boot_tab.cc
             COMPILE_FLAGS "-d -pboot"
             VERBOSE
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/y.boot_tab.hh)
BISON_TARGET(opcmdParser op_commands.yy ${CMAKE_CURRENT_BINARY_DIR}/y.opcmd_tab.cc
             COMPILE_FLAGS "-d -popcmd"
             VERBOSE
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/y.opcmd_tab.hh)
message(STATUS "BISON rtrmgr targets created")
# endif(BISON_FOUND)

find_package(FLEX REQUIRED)
if (FLEX_FOUND)
flex_target(tpltScanner template.ll
                         ${CMAKE_CURRENT_BINARY_DIR}/lex.tplt.cc
                         COMPILE_FLAGS "-Ptplt"
                         DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/lex.tplt.hh)
flex_target(bootScanner boot.ll
                         ${CMAKE_CURRENT_BINARY_DIR}/lex.boot.cc
                         COMPILE_FLAGS "-Pboot"
                         DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/lex.boot.hh)
flex_target(opcmdScanner op_commands.ll
                          ${CMAKE_CURRENT_BINARY_DIR}/lex.opcmd.cc
                          COMPILE_FLAGS "-Popcmd"
                          DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/lex.opcmd.hh)
message(STATUS "Flex targets built")
endif(FLEX_FOUND)

# ADD_FLEX_BISON_DEPENDENCY(tpltScanner tpltParser)
# ADD_FLEX_BISON_DEPENDENCY(bootScanner bootParser)
# ADD_FLEX_BISON_DEPENDENCY(opcmdScanner opcmdParser)
set_source_files_properties(${BISON_tpltParser_OUTPUTS}
                            ${BISON_bootParser_OUTPUTS}
                            ${BISON_opcmdParser_OUTPUTS}
                            ${FLEX_tpltScanner_OUTPUTS}
                            ${FLEX_bootScanner_OUTPUTS}
                            ${FLEX_opcmdScanner_OUTPUTS}
                            PROPERTIES
                            COMPILE_FLAGS -Wno-sign-compare
                            COMPILE_FLAGS -Wno-effc++)

set(LIBRTRMGR_CXX_SRCS "")


if (XRLDB)
    list(APPEND ${LIBRTRGMR_CXX_SRCS} xrldb.cc)
    add_compile_definitions("DEBUG_XRLDB=1")
endif()


add_compile_definitions("XORP_INSTALL_ROOT=\"${CMAKE_INSTALL_PREFIX}\""
                        "XORP_BUILD_ROOT=\"${CMAKE_BINARY_DIR}\""
                        "XORP_SRC_ROOT=\"${CMAKE_SOURCE_DIR}\""
                        )
# Must be static to eradicate Rtrmgr::module_status_changed reference
# With shared library, this would cause linking error on xorpsh
add_library(rtrmgr STATIC ${BISON_tpltParser_OUTPUTS}
                        ${BISON_bootParser_OUTPUTS}
                        ${BISON_opcmdParser_OUTPUTS}
                        ${FLEX_tpltScanner_OUTPUTS}
                        ${FLEX_bootScanner_OUTPUTS}
                        ${FLEX_opcmdScanner_OUTPUTS}
                        ${LIBRTRGMR_CXX_SRCS}
                        command_tree.cc
                        conf_tree.cc
                        conf_tree_node.cc
                        config_operators.cc
                        generic_module_manager.cc
                        glob_win32.c
                        master_conf_tree.cc
                        master_conf_tree_node.cc
                        master_template_tree.cc
                        master_template_tree_node.cc
                        module_command.cc
                        module_manager.cc
                        op_commands.cc
                        randomness.cc
                        slave_conf_tree.cc
                        slave_conf_tree_node.cc
                        slave_module_manager.cc
                        task.cc
                        template_base_command.cc
                        template_commands.cc
                        template_tree.cc
                        template_tree_node.cc
                        unexpanded_program.cc
                        unexpanded_xrl.cc
                        userdb.cc
                        util.cc
                        xorp_client.cc
            )

target_link_libraries(rtrmgr  OpenSSL::Crypto) # xorp xipc)
# if (not MINGW)
#     target_link_libraries(xorp_rtrmgr rt)
# endif()

target_include_directories(rtrmgr PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR})
# add_dependencies(xorp_rtrmgr tpltParser
#                              bootParser
#                              opcmdParser
#                              tpltScanner
#                              bootScanner
#                              opcmdScanner)
add_dependencies(rtrmgr interfaces targets)

add_executable(xorp_rtrmgr main_rtrmgr.cc xrl_rtrmgr_interface.cc)

add_dependencies(xorp_rtrmgr targets interfaces)
target_link_libraries(xorp_rtrmgr
                            rtrmgr
                            finder
                            proto
                            tecla
                            xipc
                            comm
                            xorp
                            OpenSSL::Crypto
                            tgts_rtrmgr
                            xif_rtrmgr_client
                            xif_finder_event_notifier
                     )
install(TARGETS xorp_rtrmgr RUNTIME DESTINATION ${XORP_SBINDIR})

if (ENABLE_PROFILE)
add_executable(xorp_profiler profiler.cc)
target_link_libraries(xorp_profiler
                               xorp
                               OpenSSL::Crypto
                               finder
                               xipc
                               comm
                               tgts_profiler
                               xif_profile
                      )
add_dependencies(xorp_profiler targets interfaces)
install(TARGETS xorp_profiler RUNTIME DESTINATION ${XORP_SBINDIR})
endif()

if (XORPSH)
add_executable(xorpsh xorpsh_main.cc xrl_xorpsh_interface.cc cli.cc)
target_link_libraries(xorpsh
                             rtrmgr
                             cli
                             tgts_cli
                             xif_cli_processor
                             proto
                             tecla
                             xif_rtrmgr
                             tgts_xorpsh
                             xif_finder_event_notifier
                             xipc
                             comm
                             xorp
                             OpenSSL::Crypto
                             ${CURSES_CHOICE_LIB})
add_dependencies(xorpsh interfaces targets)
install(TARGETS xorpsh RUNTIME DESTINATION ${XORP_SBINDIR})
endif()

if (RTLD_ORIGIN)
    if (XORPSH)
        set_target_properties(xorpsh PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib/xorp/lib"
                                                INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib/xorp/lib")
    endif()
    if (ENABLE_PROFILE)
        set_target_properties(xorp_profiler PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib/xorp/lib"
                                                       INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib/xorp/lib")
    endif()
    set_target_properties(xorp_rtrmgr PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib/xorp/lib"
                                                 INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib/xorp/lib")
endif()

if (BUILDDIR_RUN)
    if (XORPSH)
        add_custom_command(TARGET xorpsh POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorpsh>
                            ${XORP_ALIAS_SBINDIR}/$<TARGET_FILE_NAME:xorpsh>)
    endif()
    if (ENABLE_PROFILE)
        add_custom_command(TARGET xorp_profiler POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp_profiler>
                            ${XORP_ALIAS_SBINDIR}/$<TARGET_FILE_NAME:xorp_profiler>)
    endif()
    add_custom_command(TARGET xorp_rtrmgr POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp_rtrmgr>
                        ${XORP_ALIAS_SBINDIR}/$<TARGET_FILE_NAME:xorp_rtrmgr>)
endif()



if (TESTS_PROGRAMS)
    add_subdirectory(tests)
endif()
