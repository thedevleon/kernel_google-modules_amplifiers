# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 Cirrus Logic, Inc.
#
# CMake file for CS40L26 Zephyr module integration

# Set module name
set(ZEPHYR_CURRENT_MODULE_NAME cs40l26)
set(ZEPHYR_CURRENT_MODULE_DIR ${CMAKE_CURRENT_LIST_DIR})

# Add the driver directory
zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/drivers/haptic)

# The actual driver source will be compiled when CONFIG_CS40L26=y
# via the driver's own CMakeLists.txt
