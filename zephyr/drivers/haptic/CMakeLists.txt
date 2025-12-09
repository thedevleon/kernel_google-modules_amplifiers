# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 Cirrus Logic, Inc.

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_CS40L26 cs40l26.c)
