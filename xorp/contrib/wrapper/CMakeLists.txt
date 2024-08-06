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

