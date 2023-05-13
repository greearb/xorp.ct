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
#              test.tgt
#              test_fea_ifmgr_mirror.tgt
              test_fea_rawlink.tgt
#              test_finder_events.tgt
#              test_peer.tgt
              test_socket4.tgt
#              test_xrls.tgt
)
if(BGP)
    list(APPEND tgt_srcs bgp.tgt)
endif()

if(NOT DISABLE_IPV6)
    #list(APPEND tgt_srcs test_socket6.tgt)
    if(OSPF)
        list(APPEND tgt_srcs ospfv3.tgt)
    endif()
endif()


if(NOT DISABLE_PROFILE)
    list(APPEND tgt_srcs profiler.tgt)
endif()

# if(FIREWALL)
#     list(APPEND tgt_srcs fea_firewall.xif)
# endif()


if(OSLR)
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

message(STATUS "${tgt_srcs}")
set(TARGETS_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)
set(tgt_cxx "")
foreach(src_tgt ${tgt_srcs})
    get_filename_component(tgt_fname ${src_tgt} NAME_WLE)
    message(STATUS "[tgt] Filename from ${src_tgt} is ${tgt_fname}")
    string(CONCAT src_cxx ${tgt_fname} "_base.cc")
    message(STATUS "[tgt] Creating ${CMAKE_CURRENT_BINARY_DIR}/${src_cxx} from ${src_tgt}")
    # string(REGEX REPLACE ".tgt" "_base.cc" src_cxx ${src_tgt})
    # message(STATUS "Creating ${src_cxx} from ${src_tgt}")
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${src_cxx}
        # ALL
        # BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${src_cxx}
        COMMAND /usr/bin/python3 "../scripts/tgt-gen" "-I../interfaces" "-I${MAIN_DIR}"
        "-I${CMAKE_CURRENT_LIST_DIR}" "--output-dir=${CMAKE_CURRENT_BINARY_DIR}" "${src_tgt}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMENT "[tgt] Generating ${src_cxx} file from ${src_tgt}"
        # DEPENDS interfaces
    )
    list(APPEND tgt_cxx ${src_cxx})
    add_library(tgts_${tgt_fname} SHARED ${CMAKE_CURRENT_BINARY_DIR}/${src_cxx})
endforeach()
set_source_files_properties(${tgt_cxx} PROPERTIES GENERATED TRUE)
add_custom_target(targets DEPENDS ${tgt_cxx})
# target_sources(targets PRIVATE ${tgt_cxx})
