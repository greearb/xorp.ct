# rtrmgr/tests build
# Copyright (c) 2023-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(test_module_manager test_module_manager.cc)
add_executable(test_sample_config test_sample_config.cc ../xrldb.cc)
add_executable(test_templates test_templates.cc)

foreach(TEST IN ITEMS module_manager sample_config templates)
    target_link_libraries(test_${TEST} rtrmgr finder xipc xorp comm)
    add_test(test_${TEST} COMMAND test_${TEST})
endforeach()
