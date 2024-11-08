# policy/common build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(policy_common
                          dispatcher.cc
                          elem_set.cc
                          element.cc
                          element_base.cc
                          element_factory.cc
                          filter.cc
                          operator.cc
                          policy_utils.cc
                          register_elements.cc
                          register_operations.cc
                          varrw.cc
                          ../../bgp/aspath.cc
                          )

target_link_libraries(policy_common xorp comm proto OpenSSL::Crypto)
install(TARGETS policy_common LIBRARY DESTINATION ${XORP_LIBDIR})
if (RTLD_ORIGIN)
    set_target_properties(policy_common PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                   INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
endif()
if (BUILDDIR_RUN)
    add_custom_command(TARGET policy_common POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:policy_common>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:policy_common>)
endif()
