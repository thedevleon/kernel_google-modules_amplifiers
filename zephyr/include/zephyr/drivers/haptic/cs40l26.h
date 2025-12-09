/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024 Cirrus Logic, Inc.
 *
 * CS40L26 Haptic Driver Public API
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HAPTIC_CS40L26_H_
#define ZEPHYR_INCLUDE_DRIVERS_HAPTIC_CS40L26_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start I2S/ASP audio interface
 *
 * Starts the I2S/ASP (Audio Serial Port) interface on the CS40L26 device.
 * This allows audio data to be streamed to the haptic driver for
 * audio-to-haptic conversion.
 *
 * @param dev Pointer to the device structure
 * @return 0 on success, negative errno code on failure
 * @retval -ENODEV Device not initialized
 * @retval -ETIMEDOUT I2S start timeout
 */
int cs40l26_i2s_start(const struct device *dev);

/**
 * @brief Stop I2S/ASP audio interface
 *
 * Stops the I2S/ASP (Audio Serial Port) interface on the CS40L26 device.
 * This disables audio streaming and restores the PLL configuration.
 *
 * @param dev Pointer to the device structure
 * @return 0 on success, negative errno code on failure
 * @retval -ENODEV Device not initialized
 */
int cs40l26_i2s_stop(const struct device *dev);

/**
 * @brief Check if I2S interface is enabled
 *
 * Returns the current state of the I2S/ASP interface.
 *
 * @param dev Pointer to the device structure
 * @return true if I2S is enabled, false otherwise
 */
bool cs40l26_i2s_is_enabled(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_HAPTIC_CS40L26_H_ */
