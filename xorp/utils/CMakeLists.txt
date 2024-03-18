# utils build
# Copyright (c) 2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(runit runit.cc)
install(TARGETS runit RUNTIME DESTINATION ${XORP_TOOLDIR})
