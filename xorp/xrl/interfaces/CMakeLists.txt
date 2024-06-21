file(GLOB interface_srcs cli_manager.xif
                         cli_processor.xif
                         common.xif
                         coord.xif
                         datain.xif
                         fea_fib_client.xif
                         fea_fib.xif
                         fea_ifmgr_mirror.xif
                         fea_ifmgr_replicator.xif
                         fea_ifmgr.xif
                         fea_rawlink_client.xif
                         fea_rawlink.xif
                         fea_rawpkt4_client.xif
                         fea_rawpkt4.xif
                         fib2mrib.xif
                         finder_client.xif
                         finder_event_notifier.xif
                         finder_event_observer.xif
                         finder.xif
                         fti.xif
                         mfea_client.xif
                         mfea.xif
                         mld6igmp_client.xif
                         mld6igmp.xif
                         pim.xif
                         policy_backend.xif
                         policy_redist4.xif
                         policy_redist6.xif
                         policy.xif
                         redist4.xif
                         redist6.xif
                         redist_transaction4.xif
                         redist_transaction6.xif
                         rib_client.xif
                         rib.xif
                         rtrmgr_client.xif
                         rtrmgr.xif
                         socket4_user.xif
                         socket4.xif
                         static_routes.xif
                         test_peer.xif
                         test.xif
                         test_xrls.xif)
if(BGP)
    list(APPEND interface_srcs "${CMAKE_CURRENT_LIST_DIR}/bgp.xif")
endif()

if(ENABLE_IPV6)
    list(APPEND interface_srcs "${CMAKE_CURRENT_LIST_DIR}/socket6.xif"
                               "${CMAKE_CURRENT_LIST_DIR}/socket6_user.xif"
                               "${CMAKE_CURRENT_LIST_DIR}/fea_rawpkt6_client.xif"
                               "${CMAKE_CURRENT_LIST_DIR}/fea_rawpkt6.xif")
    if(OSPF)
        list(APPEND interface_srcs "${CMAKE_CURRENT_LIST_DIR}/ospfv3.xif")
    endif()
endif()


if(NOT DISABLE_PROFILE)
    list(APPEND interface_srcs "${CMAKE_CURRENT_LIST_DIR}/profile_client.xif"
                               "${CMAKE_CURRENT_LIST_DIR}/profile.xif")
endif()

if(FIREWALL)
    list(APPEND interface_srcs "${CMAKE_CURRENT_LIST_DIR}/fea_firewall.xif")
endif()

if(OLSR)
    list(APPEND interface_srcs "${CMAKE_CURRENT_LIST_DIR}/olsr4.xif")
endif()

if (WRAPPER)
    list(APPEND interface_srcs "${CMAKE_CURRENT_LIST_DIR}/wrapper4.xif")
endif()

if (RIP)
    list(APPEND interface_srcs "${CMAKE_CURRENT_LIST_DIR}/rip.xif"
                               "${CMAKE_CURRENT_LIST_DIR}/ripng.xif")
endif()


if (VRRP)
    list(APPEND interface_srcs "${CMAKE_CURRENT_LIST_DIR}/vrrp.xif")
endif()

if (OSPF)
    list(APPEND interface_srcs "${CMAKE_CURRENT_LIST_DIR}/ospfv2.xif")
endif()


message(STATUS "${interface_srcs}")
set(INTERFACES_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)
set(xif_cxx "")

foreach(src_xif ${interface_srcs})
    get_filename_component(xif_fname ${src_xif} NAME_WLE)
    message(STATUS "[xif] Filename from ${src_xif} is ${xif_fname}")
    string(CONCAT src_cxx ${xif_fname} "_xif.cc")
    # string(REGEX REPLACE ".xif" "_xif.cc" src_cxx ${src_xif})
    message(STATUS "[xif] Creating ${CMAKE_CURRENT_BINARY_DIR}/${src_cxx} from ${src_xif}")
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${src_cxx}
        # ALL
        BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${src_cxx}
        COMMAND /usr/bin/python3 "../scripts/clnt-gen" "-I${MAIN_DIR}" "--output-dir=${CMAKE_CURRENT_BINARY_DIR}" "${src_xif}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMENT "[xif] Generating ${CMAKE_CURRENT_BINARY_DIR}/${src_cxx} file from ${src_xif}"
    )
    list(APPEND xif_cxx ${src_cxx})
    add_library(xif_${xif_fname} SHARED ${CMAKE_CURRENT_BINARY_DIR}/${src_cxx})
    install(TARGETS xif_${xif_fname} LIBRARY DESTINATION ${XORP_LIBDIR})
endforeach()
# message(STATUS "Created interfaces: ${xif_cxx}")
add_custom_target(interfaces DEPENDS ${xif_cxx})
# target_sources(interfaces PRIVATE xif_cxx)
