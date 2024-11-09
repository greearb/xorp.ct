# Xrl targets build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later


file(GLOB tgt_srcs cli.tgt
              coord.tgt
              fea.tgt
              fea_ifmgr_mirror.tgt
              fib2mrib.tgt
              finder.tgt
              finder_client.tgt
              mfea.tgt
              mld6igmp.tgt
              pim.tgt
              policy.tgt
              rib.tgt
              ribclient.tgt
              rtrmgr.tgt
              show_distances.tgt
              show_routes.tgt
              static_routes.tgt
)

if(TESTS_PROGRAMS)
    list(APPEND tgt_srcs
                          test.tgt
                          test_fea_ifmgr_mirror.tgt
                          test_fea_rawlink.tgt
                          test_finder_events.tgt
                          test_socket4.tgt
                          test_xrls.tgt
        )
endif()

if(BGP)
    list(APPEND tgt_srcs bgp.tgt)
    if (TESTS_PROGRAMS)
        list(APPEND tgt_srcs test_peer.tgt)
    endif()
endif()

if(ENABLE_IPV6)
    #list(APPEND tgt_srcs test_socket6.tgt)
    if(OSPF)
        list(APPEND tgt_srcs ospfv3.tgt)
    endif()
endif()


if(ENABLE_PROFILE)
    list(APPEND tgt_srcs profiler.tgt)
endif()

# if(FIREWALL)
#     list(APPEND tgt_srcs fea_firewall.xif)
# endif()


if(OLSR)
    list(APPEND tgt_srcs olsr4.tgt)
endif()

if (WRAPPER)
    list(APPEND tgt_srcs wrapper4.tgt)
endif()

if (RIP)
    list(APPEND tgt_srcs rip.tgt
                         ripng.tgt)
endif()


if (VRRP)
    list(APPEND tgt_srcs vrrp.tgt)
endif()

if (OSPF)
    list(APPEND tgt_srcs ospfv2.tgt)
endif()

if (XORPSH)
    list(APPEND tgt_srcs xorpsh.tgt)
endif()

# message(STATUS "${tgt_srcs}")
set(TARGETS_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)
set(tgt_cxx "")
foreach(src_tgt ${tgt_srcs})
    get_filename_component(tgt_fname ${src_tgt} NAME_WLE)
    # message(STATUS "[tgt] Filename from ${src_tgt} is ${tgt_fname}")
    string(CONCAT src_cxx ${tgt_fname} "_base.cc")
    # message(STATUS "[tgt] Creating ${CMAKE_CURRENT_BINARY_DIR}/${src_cxx} from ${src_tgt}")
    add_custom_command(
        OUTPUT ${src_cxx}
        # ALL
        # BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${src_cxx}
        COMMAND ${Python3_EXECUTABLE} "../scripts/tgt-gen" "-I../interfaces" "-I${MAIN_DIR}"
        "-I${CMAKE_CURRENT_LIST_DIR}" "--output-dir=${CMAKE_CURRENT_BINARY_DIR}" "${src_tgt}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "[tgt] Generating ${src_cxx} file from ${src_tgt}"
        # DEPENDS interfaces
    )
    list(APPEND tgt_cxx ${src_cxx})
    add_library(tgts_${tgt_fname} ${src_cxx})
    target_link_libraries(tgts_${tgt_fname} xorp comm proto xipc)
    install(TARGETS tgts_${tgt_fname} LIBRARY DESTINATION ${XORP_LIBDIR})
    if (XRLDB)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${tgt_fname}.xrls DESTINATION ${XORP_XRLSDIR})
    endif()
    if (RTLD_ORIGIN)
        set_target_properties(tgts_${tgt_fname} PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                           INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    endif()
    if (BUILDDIR_RUN)
        add_custom_command(TARGET tgts_${tgt_fname} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:tgts_${tgt_fname}>
                            ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:tgts_${tgt_fname}>)
    endif()
endforeach()
add_library(tgts_base OBJECT finder_client_base.cc)
set_target_properties(tgts_base PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
add_custom_target(targets DEPENDS ${tgt_cxx})
