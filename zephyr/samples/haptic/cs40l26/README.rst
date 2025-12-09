# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 Cirrus Logic, Inc.

# CS40L26 Haptic Driver Sample

This sample demonstrates the basic usage of the CS40L26 haptic driver for Zephyr RTOS.

## Overview

The CS40L26 is a Boosted Haptic Driver with Integrated DSP and Waveform Memory. This sample:
- Enumerates the CS40L26 device on the SPI bus
- Reads the device ID and revision
- Performs a basic diagnostic check
- **Demonstrates I2S/ASP audio interface control**
- **Tests starting and stopping I2S streaming**
- Displays the results via console

## Requirements

* A board with SPI support
* CS40L26 haptic device connected via SPI
* Device tree overlay configured for your board

## Building and Running

### Device Tree Configuration

Create a device tree overlay for your board (e.g., `boards/your_board.overlay`):

```dts
&spi1 {
    status = "okay";
    cs-gpios = <&gpio0 17 GPIO_ACTIVE_LOW>;

    cs40l26: cs40l26@0 {
        compatible = "cirrus,cs40l26";
        reg = <0>;
        spi-max-frequency = <4000000>;
        reset-gpios = <&gpio0 18 GPIO_ACTIVE_HIGH>;
        irq-gpios = <&gpio0 19 GPIO_ACTIVE_LOW>;
    };
};
```

### Build

```bash
west build -b your_board samples/haptic/cs40l26
```

### Flash

```bash
west flash
```

## Sample Output

```
*** Booting Zephyr OS ***
[00:00:00.000,000] <inf> cs40l26_sample: CS40L26 Haptic Driver Sample with I2S Support
[00:00:00.100,000] <inf> cs40l26: Initializing CS40L26 haptic driver
[00:00:00.200,000] <inf> cs40l26: Found CS40L26A, revision 0xB0
[00:00:00.300,000] <inf> cs40l26: Running diagnostic check...
[00:00:00.350,000] <inf> cs40l26: Power management status: 0x00000001
[00:00:00.400,000] <inf> cs40l26: Diagnostic check completed successfully
[00:00:00.450,000] <inf> cs40l26: CS40L26 initialization completed successfully
[00:00:00.500,000] <inf> cs40l26_sample: Device enumeration successful
[00:00:00.550,000] <inf> cs40l26_sample: Testing I2S interface...
[00:00:00.600,000] <inf> cs40l26: Starting I2S interface
[00:00:00.650,000] <inf> cs40l26: I2S interface started successfully
[00:00:00.700,000] <inf> cs40l26_sample: I2S started successfully
[00:00:00.750,000] <inf> cs40l26_sample: I2S is currently enabled
[00:00:02.750,000] <inf> cs40l26: Stopping I2S interface
[00:00:02.800,000] <inf> cs40l26: I2S interface stopped successfully
[00:00:02.850,000] <inf> cs40l26_sample: I2S stopped successfully
[00:00:02.900,000] <inf> cs40l26_sample: Sample application running...
```

## Troubleshooting

### Device Not Found
- Verify SPI connections (MOSI, MISO, SCK, CS)
- Check power supply to the device
- Verify reset GPIO is correctly configured
- Ensure SPI bus speed is within device limits (max 4MHz)

### SPI Communication Errors
- Check SPI bus configuration in device tree
- Verify chip select polarity
- Ensure proper grounding

### Incorrect Device ID
- Verify the correct CS40L26 variant is connected
- Check for communication errors
- Ensure device is powered and out of reset
