# libxorp build
# Copyright (c) 2021-2024 Michał Zagórski (zagura)
# License: see ${XORP}/LICENCE.gpl

if(TESTS_PROGRAMS)
    add_subdirectory(tests)
endif()

set(XORP_CXX
                # C sources
                daemon.c
                debug.c
                ether_compat.c
                gai_strerror.c
                getopt.c
                inet_ntop.c
                inet_pton.c
                random.c
                strlcpy.c
                strptime.c
                utility.c
                win_io.c
                xlog.c
                # C++ sources
                asyncio.cc
                buffered_asyncio.cc
                bug_catcher.cc
                c_format.cc
                callback.cc
                clock.cc
                eventloop.cc
                exceptions.cc
                heap.cc
                ipnet.cc
                ipv4.cc
                ipv6.cc
                ipvx.cc
                mac.cc
                nexthop.cc
                popen.cc
                ref_ptr.cc
                round_robin.cc
                run_command.cc
                safe_callback_obj.cc
                selector.cc
                service.cc
                task.cc
                time_slice.cc
                timer.cc
                timeval.cc
                token.cc
                transaction.cc
                utils.cc
                vif.cc
                win_dispatcher.cc
)

if (BUILD_INFO)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/build_info.cc
                       COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/create_buildinfo.sh
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                       COMMENT "Generating build info")
    add_library(xorp ${XORP_CXX} ${CMAKE_CURRENT_BINARY_DIR}/build_info.cc)
    target_include_directories(xorp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
else()
    add_library(xorp ${XORP_CXX})
endif()

if (NOT XORP_DISABLE_PROFILE)
    target_sources(xorp PUBLIC profile.cc)
endif()


install(TARGETS xorp LIBRARY DESTINATION ${XORP_LIBDIR})
if (BUILDDIR_RUN)
    add_custom_command(TARGET xorp POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:xorp>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:xorp>)
endif()
