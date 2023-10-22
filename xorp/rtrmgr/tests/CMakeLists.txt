# rtrmgr/tests build
# Copyright (c) 2023 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(test_module_manager test_module_manager.cc)
target_link_libraries(test_module_manager xorp_rtrmgr finder xipc xorp comm)

add_executable(test_sample_config test_sample_config.cc ../xrldb.cc)
target_link_libraries(test_sample_config xorp_rtrmgr finder xipc xorp comm)

add_executable(test_templates test_templates.cc)
target_link_libraries(test_templates xorp_rtrmgr finder xipc xorp comm)

add_test(module_manager COMMAND test_module_manager)
add_test(test_sample_config COMMAND test_sample_config)
add_test(test_templates COMMAND test_templates)

# ~zagura 2023-10-22
# @todo: fix the tests paths for templates and modules
