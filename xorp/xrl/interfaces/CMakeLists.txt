# Xrl interfaces
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

set(interface_srcs       cli_manager.xif
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
    list(APPEND interface_srcs bgp.xif)
endif()

if(ENABLE_IPV6)
    list(APPEND interface_srcs "socket6.xif"
                               "socket6_user.xif"
                               "fea_rawpkt6_client.xif"
                               "fea_rawpkt6.xif")
    if(OSPF)
        list(APPEND interface_srcs "ospfv3.xif")
    endif()
endif()


if(NOT DISABLE_PROFILE)
    list(APPEND interface_srcs "profile_client.xif"
                               "profile.xif")
endif()

if(FIREWALL)
    list(APPEND interface_srcs "fea_firewall.xif")
endif()

if(OLSR)
    list(APPEND interface_srcs "olsr4.xif")
endif()

if (WRAPPER)
    list(APPEND interface_srcs "wrapper4.xif")
endif()

if (RIP)
    list(APPEND interface_srcs "rip.xif"
                               "ripng.xif")
endif()


if (VRRP)
    list(APPEND interface_srcs "vrrp.xif")
endif()

if (OSPF)
    list(APPEND interface_srcs "ospfv2.xif")
endif()


message(STATUS "${interface_srcs}")
#set(INTERFACES_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)
set(xif_cxx "")

foreach(src_xif ${interface_srcs})
    get_filename_component(xif_fname ${src_xif} NAME_WLE)
    message(STATUS "[xif] Filename from ${src_xif} is ${xif_fname}")
    string(CONCAT src_cxx ${xif_fname} "_xif.cc")
    # string(REGEX REPLACE ".xif" "_xif.cc" src_cxx ${src_xif})
    message(STATUS "[xif] Creating ${src_cxx} from ${src_xif}")
    add_custom_command(
        OUTPUT ${src_cxx}
        # ALL
        DEPENDS ${src_xif}
        COMMAND /usr/bin/python3 "../scripts/clnt-gen" "-I${MAIN_DIR}" "--output-dir=${CMAKE_CURRENT_BINARY_DIR}" "${src_xif}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "[xif] Generating ${src_cxx} file from ${src_xif}"
    )
    list(APPEND xif_cxx ${src_cxx})
    add_library(xif_${xif_fname} ${src_cxx})
    target_link_libraries(xif_${xif_fname} PUBLIC xorp comm proto xipc)
    install(TARGETS xif_${xif_fname} LIBRARY DESTINATION ${XORP_LIBDIR})
endforeach()

add_library(xif_base OBJECT common_xif.cc finder_xif.cc)
# message(STATUS "Created interfaces: ${xif_cxx}")
add_custom_target(interfaces DEPENDS ${xif_cxx})
# target_sources(interfaces PRIVATE xif_cxx)
