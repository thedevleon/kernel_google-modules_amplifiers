# CS40L26 Zephyr Driver

This directory contains the Zephyr RTOS driver port for the Cirrus Logic CS40L26 Boosted Haptic Amplifier.

## Overview

The CS40L26 is a boosted haptic driver with integrated DSP and waveform memory, featuring advanced closed-loop algorithms and LRA protection. This Zephyr driver provides basic device enumeration and diagnostics functionality.

## Features

- SPI bus communication
- Device identification and enumeration
- Hardware reset via GPIO
- Basic diagnostic checks
- Device power management status monitoring
- **I2S/ASP audio interface control**
- **Audio-to-haptic streaming support**
- Logging support

## Directory Structure

```
zephyr/
├── drivers/
│   └── haptic/
│       ├── cs40l26.c          # Main driver implementation
│       ├── CMakeLists.txt     # Driver build configuration
│       └── Kconfig            # Driver Kconfig options
├── include/
│   └── zephyr/
│       └── drivers/
│           └── haptic/
│               └── cs40l26.h  # Public API header
├── dts/
│   └── bindings/
│       └── haptic/
│           └── cirrus,cs40l26.yaml  # Device tree binding
└── samples/
    └── haptic/
        └── cs40l26/
            ├── src/
            │   └── main.c     # Sample application
            ├── CMakeLists.txt # Sample build configuration
            ├── prj.conf       # Sample project configuration
            ├── app.overlay    # Sample device tree overlay
            ├── sample.yaml    # Sample test configuration
            └── README.rst     # Sample documentation
```

## Integration with Zephyr

To integrate this driver into your Zephyr project:

### 1. Copy Driver Files

Copy the driver files to your Zephyr tree or external module:

```bash
# Option 1: Copy to Zephyr tree
cp -r zephyr/drivers/haptic $ZEPHYR_BASE/drivers/
cp -r zephyr/dts/bindings/haptic $ZEPHYR_BASE/dts/bindings/

# Option 2: Use as external module
# Add this directory to ZEPHYR_EXTRA_MODULES in CMakeLists.txt
```

### 2. Update Kconfig

Add the driver to your board or application Kconfig:

```kconfig
source "drivers/haptic/Kconfig"
```

### 3. Update CMakeLists.txt

Add the driver subdirectory:

```cmake
add_subdirectory_ifdef(CONFIG_CS40L26 haptic)
```

### 4. Device Tree Configuration

Create a device tree overlay for your board:

```dts
&spi1 {
    status = "okay";
    cs-gpios = <&gpio0 17 GPIO_ACTIVE_LOW>;

    cs40l26: cs40l26@0 {
        compatible = "cirrus,cs40l26";
        reg = <0>;
        spi-max-frequency = <4000000>;
        reset-gpios = <&gpio0 18 GPIO_ACTIVE_HIGH>;
        irq-gpios = <&gpio0 19 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
    };
};
```

### 5. Enable in Project Configuration

Add to your `prj.conf`:

```ini
CONFIG_CS40L26=y
CONFIG_SPI=y
CONFIG_GPIO=y
```

## Hardware Connections

The CS40L26 requires the following connections:

| Signal | Description | Direction |
|--------|-------------|-----------|
| SCK    | SPI Clock   | Output    |
| MOSI   | SPI MOSI    | Output    |
| MISO   | SPI MISO    | Input     |
| CS     | Chip Select | Output    |
| RESET  | Hardware Reset | Output |
| IRQ    | Interrupt   | Input     |
| VP     | Boost Power Supply | - |
| VA     | Analog Power Supply | - |
| GND    | Ground      | -         |

## Supported Devices

- CS40L26A (Device ID: 0x40A260)
- CS40L26B (Device ID: 0x40A26B)
- CS40L27A (Device ID: 0x40A270)
- CS40L27B (Device ID: 0x40A27B)

## Configuration Options

### Kconfig Options

- `CONFIG_CS40L26` - Enable CS40L26 driver
- `CONFIG_CS40L26_INIT_PRIORITY` - Initialization priority (default: 80)
- `CONFIG_CS40L26_LOG_LEVEL` - Log level for driver messages

### Device Tree Properties

- `compatible` - Must be "cirrus,cs40l26"
- `reg` - SPI chip select number
- `spi-max-frequency` - Maximum SPI frequency (max 4MHz)
- `reset-gpios` - Reset GPIO pin specification
- `irq-gpios` - Interrupt GPIO pin specification
- `devid` - Expected device ID (optional)

## API Usage

### Basic Initialization

The driver automatically initializes during system startup. To verify:

```c
#include <zephyr/device.h>

#define CS40L26_NODE DT_NODELABEL(cs40l26)

int main(void)
{
    const struct device *dev = DEVICE_DT_GET(CS40L26_NODE);
    
    if (!device_is_ready(dev)) {
        printk("CS40L26 not ready\n");
        return -ENODEV;
    }
    
    printk("CS40L26 initialized successfully\n");
    return 0;
}
```

### I2S/ASP Audio Interface

The driver provides an API to control the I2S/ASP audio interface:

```c
#include <zephyr/device.h>
#include <zephyr/drivers/haptic/cs40l26.h>

#define CS40L26_NODE DT_NODELABEL(cs40l26)

int main(void)
{
    const struct device *dev = DEVICE_DT_GET(CS40L26_NODE);
    int ret;
    
    /* Start I2S interface for audio-to-haptic streaming */
    ret = cs40l26_i2s_start(dev);
    if (ret) {
        printk("Failed to start I2S: %d\n", ret);
        return ret;
    }
    
    /* Check if I2S is enabled */
    if (cs40l26_i2s_is_enabled(dev)) {
        printk("I2S interface is active\n");
    }
    
    /* Process audio... */
    
    /* Stop I2S interface */
    ret = cs40l26_i2s_stop(dev);
    if (ret) {
        printk("Failed to stop I2S: %d\n", ret);
        return ret;
    }
    
    return 0;
}
```

### API Functions

- `int cs40l26_i2s_start(const struct device *dev)` - Start I2S/ASP interface
- `int cs40l26_i2s_stop(const struct device *dev)` - Stop I2S/ASP interface  
- `bool cs40l26_i2s_is_enabled(const struct device *dev)` - Check I2S status

## Sample Application

A sample application is provided in `zephyr/samples/haptic/cs40l26/` that demonstrates:

1. Device enumeration on SPI bus
2. Device ID verification
3. Basic diagnostic checks
4. **I2S interface control (start/stop)**
5. Continuous operation

Build and run:

```bash
cd zephyr/samples/haptic/cs40l26
west build -b your_board
west flash
```

## Troubleshooting

### SPI Communication Issues

- Verify SPI bus configuration and pin assignments
- Check SPI clock frequency (must not exceed 4MHz)
- Ensure proper chip select polarity
- Verify power supply connections

### Device Not Found

- Check reset GPIO configuration and timing
- Verify device power supplies (VP and VA)
- Ensure device is properly connected to SPI bus
- Check for short circuits or connection issues

### Initialization Failures

- Review device tree overlay configuration
- Check GPIO pin availability and configuration
- Verify SPI bus number and chip select
- Enable debug logging: `CONFIG_CS40L26_LOG_LEVEL_DBG=y`

## Future Enhancements

This basic driver can be extended with:

- Haptic effect playback
- Firmware loading support
- Advanced power management
- Interrupt handling
- Calibration support
- Waveform memory management
- GPIO triggering
- I2S audio interface support

## References

- CS40L26 Datasheet
- Zephyr SPI Driver API
- Zephyr GPIO Driver API
- Zephyr Device Tree Bindings

## License

This driver is released under the Apache-2.0 license.

## Credits

Based on the Linux kernel driver by Fred Treven <fred.treven@cirrus.com>
Ported to Zephyr RTOS.
