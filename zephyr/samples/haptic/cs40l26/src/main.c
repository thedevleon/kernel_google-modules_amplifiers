/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024 Cirrus Logic, Inc.
 *
 * CS40L26 Haptic Driver Sample Application
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cs40l26_sample, LOG_LEVEL_INF);

/* Get the CS40L26 device from device tree */
#define CS40L26_NODE DT_NODELABEL(cs40l26)

#if !DT_NODE_EXISTS(CS40L26_NODE)
#error "No cs40l26 device found in device tree"
#endif

int main(void)
{
	const struct device *dev;

	LOG_INF("CS40L26 Haptic Driver Sample");

	/* Get device binding */
	dev = DEVICE_DT_GET(CS40L26_NODE);
	if (!device_is_ready(dev)) {
		LOG_ERR("CS40L26 device not ready");
		return -ENODEV;
	}

	LOG_INF("Device enumeration successful");
	
	/* The device was already initialized and diagnostics run during init */
	/* Additional functionality can be added here */

	LOG_INF("Sample application running...");

	/* Keep application running */
	while (1) {
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
