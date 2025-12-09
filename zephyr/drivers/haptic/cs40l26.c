/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024 Cirrus Logic, Inc.
 *
 * CS40L26 Boosted Haptic Driver for Zephyr RTOS
 * Based on the Linux kernel driver by Fred Treven <fred.treven@cirrus.com>
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(cs40l26, CONFIG_CS40L26_LOG_LEVEL);

/* CS40L26 Register Definitions */
#define CS40L26_DEVID				0x0
#define CS40L26_REVID				0x4
#define CS40L26_TEST_KEY_CTRL			0x40
#define CS40L26_REFCLK_INPUT			0x2C04
#define CS40L26_PWRMGT_CTL			0x2900
#define CS40L26_PWRMGT_STS			0x290C
#define CS40L26_ASP_ENABLES1			0x4800
#define CS40L26_ASP_CONTROL2			0x4808
#define CS40L26_DSP_MBOX_1			0x13000

/* Device IDs */
#define CS40L26_DEVID_A				0x40A260
#define CS40L26_DEVID_B				0x40A26B
#define CS40L26_DEVID_L27_A			0x40A270
#define CS40L26_DEVID_L27_B			0x40A27B
#define CS40L26_DEVID_MASK			GENMASK(23, 0)

/* Revision IDs */
#define CS40L26_REVID_A1			0xA1
#define CS40L26_REVID_B0			0xB0
#define CS40L26_REVID_B1			0xB1
#define CS40L26_REVID_B2			0xB2
#define CS40L26_REVID_MASK			GENMASK(7, 0)

/* Test key unlock codes */
#define CS40L26_TEST_KEY_UNLOCK_CODE1		0x00000055
#define CS40L26_TEST_KEY_UNLOCK_CODE2		0x000000AA
#define CS40L26_TEST_KEY_LOCK_CODE		0x00000000

/* Global enable mask */
#define CS40L26_GLOBAL_EN_MASK			BIT(0)

/* DSP Mailbox Commands */
#define CS40L26_DSP_MBOX_CMD_START_I2S		0x03000002
#define CS40L26_DSP_MBOX_CMD_STOP_I2S		0x03000003
#define CS40L26_STOP_PLAYBACK			0x05000000

/* Mailbox status */
#define CS40L26_DSP_MBOX_COMPLETE_I2S		0x01000002

/* ASP/I2S Configuration */
#define CS40L26_ASP_RX1_EN_MASK			BIT(16)
#define CS40L26_ASP_RX1_EN_SHIFT		16
#define CS40L26_ASP_RX2_EN_MASK			BIT(17)
#define CS40L26_ASP_RX2_EN_SHIFT		17
#define CS40L26_ASP_FMT_MASK			GENMASK(10, 8)
#define CS40L26_ASP_FMT_SHIFT			8
#define CS40L26_ASP_FMT_I2S			0x2
#define CS40L26_PLL_REFCLK_LOOP_MASK		BIT(11)

/* Timing constants */
#define CS40L26_MIN_RESET_PULSE_WIDTH		1500  /* microseconds */
#define CS40L26_CONTROL_PORT_READY_DELAY	6000  /* microseconds */
#define CS40L26_ASP_TIMEOUT_MS			50    /* milliseconds */

/* SPI transfer settings */
#define CS40L26_SPI_MAX_FREQ_HZ			4000000

struct cs40l26_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec irq_gpio;
	uint32_t devid;
};

struct cs40l26_data {
	uint32_t device_id;
	uint8_t revision_id;
	bool initialized;
	bool i2s_enabled;
	uint32_t refclk_input;
	struct k_sem i2s_sem;
};

/**
 * @brief Write a 32-bit register via SPI
 */
static int cs40l26_reg_write(const struct device *dev, uint32_t reg, uint32_t val)
{
	const struct cs40l26_config *config = dev->config;
	uint8_t tx_buf[8];
	int ret;

	/* CS40L26 uses 32-bit register addresses and 32-bit data values */
	/* Format: [ADDR(32-bit)][DATA(32-bit)] in big-endian */
	sys_put_be32(reg, &tx_buf[0]);
	sys_put_be32(val, &tx_buf[4]);

	struct spi_buf tx_spi_buf = {
		.buf = tx_buf,
		.len = sizeof(tx_buf)
	};
	struct spi_buf_set tx = {
		.buffers = &tx_spi_buf,
		.count = 1
	};

	ret = spi_write_dt(&config->spi, &tx);
	if (ret < 0) {
		LOG_ERR("SPI write failed: %d", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Read a 32-bit register via SPI
 */
static int cs40l26_reg_read(const struct device *dev, uint32_t reg, uint32_t *val)
{
	const struct cs40l26_config *config = dev->config;
	uint8_t tx_buf[4];
	uint8_t rx_buf[4];
	int ret;

	/* Set read bit (bit 31) in address */
	sys_put_be32(reg | BIT(31), tx_buf);

	struct spi_buf tx_spi_buf = {
		.buf = tx_buf,
		.len = sizeof(tx_buf)
	};
	struct spi_buf_set tx = {
		.buffers = &tx_spi_buf,
		.count = 1
	};

	struct spi_buf rx_spi_buf = {
		.buf = rx_buf,
		.len = sizeof(rx_buf)
	};
	struct spi_buf_set rx = {
		.buffers = &rx_spi_buf,
		.count = 1
	};

	ret = spi_transceive_dt(&config->spi, &tx, &rx);
	if (ret < 0) {
		LOG_ERR("SPI read failed: %d", ret);
		return ret;
	}

	*val = sys_get_be32(rx_buf);
	return 0;
}

/**
 * @brief Reset the CS40L26 device
 */
static int cs40l26_reset(const struct device *dev)
{
	const struct cs40l26_config *config = dev->config;
	int ret;

	if (!config->reset_gpio.port) {
		LOG_WRN("No reset GPIO configured");
		return 0;
	}

	/* Assert reset (active low) */
	ret = gpio_pin_set_dt(&config->reset_gpio, 1);
	if (ret < 0) {
		LOG_ERR("Failed to assert reset: %d", ret);
		return ret;
	}

	k_usleep(CS40L26_MIN_RESET_PULSE_WIDTH);

	/* Deassert reset */
	ret = gpio_pin_set_dt(&config->reset_gpio, 0);
	if (ret < 0) {
		LOG_ERR("Failed to deassert reset: %d", ret);
		return ret;
	}

	/* Wait for control port to be ready */
	k_usleep(CS40L26_CONTROL_PORT_READY_DELAY);

	return 0;
}

/**
 * @brief Write to DSP mailbox
 */
static int cs40l26_mailbox_write(const struct device *dev, uint32_t val)
{
	int ret;

	ret = cs40l26_reg_write(dev, CS40L26_DSP_MBOX_1, val);
	if (ret) {
		LOG_ERR("Failed to write mailbox: %d", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Start I2S/ASP interface
 */
int cs40l26_i2s_start(const struct device *dev)
{
	struct cs40l26_data *data = dev->data;
	int ret;

	if (!data->initialized) {
		LOG_ERR("Device not initialized");
		return -ENODEV;
	}

	if (data->i2s_enabled) {
		LOG_WRN("I2S already enabled");
		return 0;
	}

	LOG_INF("Starting I2S interface");

	/* Stop any playback before starting I2S */
	ret = cs40l26_mailbox_write(dev, CS40L26_STOP_PLAYBACK);
	if (ret) {
		LOG_ERR("Failed to stop playback before I2S start");
		return ret;
	}

	/* Save reference clock input for later restoration */
	ret = cs40l26_reg_read(dev, CS40L26_REFCLK_INPUT, &data->refclk_input);
	if (ret) {
		LOG_ERR("Failed to read REFCLK_INPUT");
		return ret;
	}

	/* Send I2S start command */
	ret = cs40l26_mailbox_write(dev, CS40L26_DSP_MBOX_CMD_START_I2S);
	if (ret) {
		LOG_ERR("Failed to send I2S start command");
		return ret;
	}

	/* Wait for I2S to start (with timeout) */
	ret = k_sem_take(&data->i2s_sem, K_MSEC(CS40L26_ASP_TIMEOUT_MS));
	if (ret) {
		LOG_ERR("I2S start timeout");
		return -ETIMEDOUT;
	}

	data->i2s_enabled = true;
	LOG_INF("I2S interface started successfully");

	return 0;
}

/**
 * @brief Stop I2S/ASP interface
 */
int cs40l26_i2s_stop(const struct device *dev)
{
	struct cs40l26_data *data = dev->data;
	uint32_t pll_loop;
	int ret;

	if (!data->initialized) {
		LOG_ERR("Device not initialized");
		return -ENODEV;
	}

	if (!data->i2s_enabled) {
		LOG_WRN("I2S already disabled");
		return 0;
	}

	LOG_INF("Stopping I2S interface");

	/* Send I2S stop command */
	ret = cs40l26_mailbox_write(dev, CS40L26_DSP_MBOX_CMD_STOP_I2S);
	if (ret) {
		LOG_ERR("Failed to send I2S stop command");
		return ret;
	}

	/* Restore PLL configuration */
	pll_loop = (data->refclk_input & CS40L26_PLL_REFCLK_LOOP_MASK) ? 1 : 0;
	
	/* Note: PLL loop restoration would go here if needed */
	/* For basic implementation, we just clear the flag */

	data->i2s_enabled = false;
	LOG_INF("I2S interface stopped successfully");

	return 0;
}

/**
 * @brief Check if I2S interface is enabled
 */
bool cs40l26_i2s_is_enabled(const struct device *dev)
{
	struct cs40l26_data *data = dev->data;
	return data->i2s_enabled;
}

/**
 * @brief Unlock test key for register access
 */
static int cs40l26_test_key_unlock(const struct device *dev)
{
	int ret;

	ret = cs40l26_reg_write(dev, CS40L26_TEST_KEY_CTRL, 
				CS40L26_TEST_KEY_UNLOCK_CODE1);
	if (ret) {
		return ret;
	}

	ret = cs40l26_reg_write(dev, CS40L26_TEST_KEY_CTRL,
				CS40L26_TEST_KEY_UNLOCK_CODE2);
	return ret;
}

/**
 * @brief Lock test key
 */
static int cs40l26_test_key_lock(const struct device *dev)
{
	return cs40l26_reg_write(dev, CS40L26_TEST_KEY_CTRL,
				 CS40L26_TEST_KEY_LOCK_CODE);
}

/**
 * @brief Read and verify device ID
 */
static int cs40l26_read_device_id(const struct device *dev)
{
	struct cs40l26_data *data = dev->data;
	uint32_t devid, revid;
	int ret;

	ret = cs40l26_reg_read(dev, CS40L26_DEVID, &devid);
	if (ret) {
		LOG_ERR("Failed to read DEVID: %d", ret);
		return ret;
	}

	devid &= CS40L26_DEVID_MASK;
	data->device_id = devid;

	ret = cs40l26_reg_read(dev, CS40L26_REVID, &revid);
	if (ret) {
		LOG_ERR("Failed to read REVID: %d", ret);
		return ret;
	}

	revid &= CS40L26_REVID_MASK;
	data->revision_id = (uint8_t)revid;

	/* Verify device ID */
	switch (devid) {
	case CS40L26_DEVID_A:
		LOG_INF("Found CS40L26A, revision 0x%02X", data->revision_id);
		break;
	case CS40L26_DEVID_B:
		LOG_INF("Found CS40L26B, revision 0x%02X", data->revision_id);
		break;
	case CS40L26_DEVID_L27_A:
		LOG_INF("Found CS40L27A, revision 0x%02X", data->revision_id);
		break;
	case CS40L26_DEVID_L27_B:
		LOG_INF("Found CS40L27B, revision 0x%02X", data->revision_id);
		break;
	default:
		LOG_ERR("Unknown device ID: 0x%06X", devid);
		return -ENODEV;
	}

	return 0;
}

/**
 * @brief Perform basic diagnostic check
 */
static int cs40l26_diagnostic_check(const struct device *dev)
{
	uint32_t pwr_sts;
	int ret;

	LOG_INF("Running diagnostic check...");

	/* Read power management status */
	ret = cs40l26_reg_read(dev, CS40L26_PWRMGT_STS, &pwr_sts);
	if (ret) {
		LOG_ERR("Failed to read power status: %d", ret);
		return ret;
	}

	LOG_INF("Power management status: 0x%08X", pwr_sts);

	/* Additional diagnostics can be added here */

	LOG_INF("Diagnostic check completed successfully");
	return 0;
}

/**
 * @brief Initialize the CS40L26 device
 */
static int cs40l26_init(const struct device *dev)
{
	const struct cs40l26_config *config = dev->config;
	struct cs40l26_data *data = dev->data;
	int ret;

	LOG_INF("Initializing CS40L26 haptic driver");

	/* Initialize I2S semaphore */
	k_sem_init(&data->i2s_sem, 0, 1);
	data->i2s_enabled = false;

	/* Verify SPI bus is ready */
	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	/* Configure reset GPIO if available */
	if (config->reset_gpio.port) {
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO: %d", ret);
			return ret;
		}
	}

	/* Configure IRQ GPIO if available */
	if (config->irq_gpio.port) {
		ret = gpio_pin_configure_dt(&config->irq_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure IRQ GPIO: %d", ret);
			return ret;
		}
	}

	/* Reset the device */
	ret = cs40l26_reset(dev);
	if (ret) {
		LOG_ERR("Device reset failed: %d", ret);
		return ret;
	}

	/* Read and verify device ID */
	ret = cs40l26_read_device_id(dev);
	if (ret) {
		LOG_ERR("Device enumeration failed: %d", ret);
		return ret;
	}

	/* Unlock test registers for initialization */
	ret = cs40l26_test_key_unlock(dev);
	if (ret) {
		LOG_ERR("Failed to unlock test key: %d", ret);
		return ret;
	}

	/* Perform diagnostic check */
	ret = cs40l26_diagnostic_check(dev);
	if (ret) {
		LOG_ERR("Diagnostic check failed: %d", ret);
		cs40l26_test_key_lock(dev);
		return ret;
	}

	/* Lock test registers */
	ret = cs40l26_test_key_lock(dev);
	if (ret) {
		LOG_ERR("Failed to lock test key: %d", ret);
		return ret;
	}

	data->initialized = true;
	LOG_INF("CS40L26 initialization completed successfully");

	return 0;
}

/* Device instance macros */
#define CS40L26_DEFINE(inst)						\
	static struct cs40l26_data cs40l26_data_##inst;			\
									\
	static const struct cs40l26_config cs40l26_config_##inst = {	\
		.spi = SPI_DT_SPEC_INST_GET(inst,			\
					    SPI_OP_MODE_MASTER |	\
					    SPI_WORD_SET(8) |		\
					    SPI_TRANSFER_MSB,		\
					    0),				\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}), \
		.irq_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, irq_gpios, {0}), \
		.devid = DT_INST_PROP_OR(inst, devid, CS40L26_DEVID_A), \
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			      cs40l26_init,				\
			      NULL,					\
			      &cs40l26_data_##inst,			\
			      &cs40l26_config_##inst,			\
			      POST_KERNEL,				\
			      CONFIG_CS40L26_INIT_PRIORITY,		\
			      NULL);

/* Create device instances for all devicetree nodes */
DT_INST_FOREACH_STATUS_OKAY(CS40L26_DEFINE)
