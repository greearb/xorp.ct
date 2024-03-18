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
                          )

install(TARGETS policy_common LIBRARY DESTINATION ${XORP_LIBDIR})
