# CS40L26 Zephyr Driver Integration Guide

This guide explains how to integrate the CS40L26 Zephyr driver into your Zephyr-based project.

## Quick Start

### Method 1: Using West Manifest (Recommended)

Add this repository as a module in your west manifest (`west.yml`):

```yaml
manifest:
  projects:
    - name: cs40l26-zephyr
      url: https://github.com/thedevleon/kernel_google-modules_amplifiers
      revision: copilot/convert-cs40l26-to-zephyr-driver
      path: modules/cs40l26
```

Then run:
```bash
west update
```

### Method 2: Manual Integration

1. Copy the Zephyr driver files to your project:

```bash
cp -r zephyr/drivers/haptic <your-zephyr-base>/drivers/
cp -r zephyr/dts/bindings/haptic <your-zephyr-base>/dts/bindings/
```

2. Update your build files to include the driver.

## Configuration

### 1. Device Tree Configuration

Create or update your board overlay file (e.g., `boards/your_board.overlay`):

```dts
/ {
    aliases {
        haptic0 = &cs40l26;
    };
};

&spi1 {
    status = "okay";
    cs-gpios = <&gpio0 17 GPIO_ACTIVE_LOW>;

    cs40l26: cs40l26@0 {
        compatible = "cirrus,cs40l26";
        reg = <0>;
        spi-max-frequency = <4000000>;
        reset-gpios = <&gpio0 18 GPIO_ACTIVE_HIGH>;
        irq-gpios = <&gpio0 19 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
        label = "CS40L26_HAPTIC";
    };
};
```

**Important Notes:**
- Adjust SPI bus number (`&spi1`) for your board
- Modify GPIO pins and ports to match your hardware
- Maximum SPI frequency: 4 MHz
- Reset GPIO is active HIGH
- IRQ GPIO should have pull-up enabled

### 2. Project Configuration

Add to your `prj.conf`:

```ini
# Enable CS40L26 Haptic Driver
CONFIG_CS40L26=y

# Required dependencies
CONFIG_SPI=y
CONFIG_GPIO=y

# Optional: Enable logging for debugging
CONFIG_LOG=y
CONFIG_CS40L26_LOG_LEVEL_INF=y
```

### 3. Build Configuration

Update your project's CMakeLists.txt if necessary:

```cmake
# If using external module location
list(APPEND ZEPHYR_EXTRA_MODULES
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/cs40l26/zephyr
)
```

## Building

### Build the Sample Application

```bash
cd zephyr/samples/haptic/cs40l26
west build -b <your_board>
west flash
```

### Build Your Own Application

```bash
west build -b <your_board> -p
```

## Using the Driver in Your Application

### Basic Usage Example

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define HAPTIC_DEV DT_ALIAS(haptic0)

int main(void)
{
    const struct device *haptic_dev;
    
    LOG_INF("Starting haptic application");
    
    /* Get device binding */
    haptic_dev = DEVICE_DT_GET(HAPTIC_DEV);
    if (!device_is_ready(haptic_dev)) {
        LOG_ERR("Haptic device not ready");
        return -ENODEV;
    }
    
    LOG_INF("CS40L26 haptic device ready");
    
    /* Device is initialized and diagnostics are complete */
    /* Add your haptic control code here */
    
    return 0;
}
```

### Error Handling

```c
if (!device_is_ready(haptic_dev)) {
    switch (errno) {
    case ENODEV:
        LOG_ERR("Device not found - check device tree");
        break;
    case ETIMEDOUT:
        LOG_ERR("SPI communication timeout");
        break;
    default:
        LOG_ERR("Unknown error: %d", errno);
    }
    return errno;
}
```

## Hardware Setup

### Pin Connections

| CS40L26 Pin | Function | Connect To |
|-------------|----------|------------|
| SCK         | SPI Clock | MCU SPI SCK |
| SDI (MOSI)  | SPI Data In | MCU SPI MOSI |
| SDO (MISO)  | SPI Data Out | MCU SPI MISO |
| CS          | Chip Select | MCU GPIO (CS) |
| RESET       | Hardware Reset | MCU GPIO |
| INT         | Interrupt | MCU GPIO |
| VP          | Power Supply | 2.5V - 5.5V |
| VA          | Analog Supply | 1.8V |
| GND         | Ground | Ground |
| LRA+        | LRA Positive | Haptic Actuator + |
| LRA-        | LRA Negative | Haptic Actuator - |

### Power Supply Requirements

- VP: 2.5V to 5.5V (boost converter input)
- VA: 1.71V to 1.98V (analog supply)
- Digital I/O: Compatible with 1.8V and 3.3V logic levels

## Testing and Verification

### Expected Console Output

```
*** Booting Zephyr OS ***
[00:00:00.000] <inf> cs40l26_sample: CS40L26 Haptic Driver Sample
[00:00:00.100] <inf> cs40l26: Initializing CS40L26 haptic driver
[00:00:00.200] <inf> cs40l26: Found CS40L26A, revision 0xB0
[00:00:00.300] <inf> cs40l26: Running diagnostic check...
[00:00:00.350] <inf> cs40l26: Power management status: 0x00000001
[00:00:00.400] <inf> cs40l26: Diagnostic check completed successfully
[00:00:00.450] <inf> cs40l26: CS40L26 initialization completed successfully
[00:00:00.500] <inf> cs40l26_sample: Device enumeration successful
```

### Debugging

Enable debug logging:

```ini
CONFIG_CS40L26_LOG_LEVEL_DBG=y
CONFIG_SPI_LOG_LEVEL_DBG=y
CONFIG_GPIO_LOG_LEVEL_DBG=y
```

## Troubleshooting

### Problem: "CS40L26 device not ready"

**Solutions:**
1. Verify SPI bus is enabled in device tree
2. Check GPIO configurations
3. Ensure device tree label matches code
4. Verify SPI bus number is correct

### Problem: "Failed to read DEVID"

**Solutions:**
1. Check SPI wiring (SCK, MOSI, MISO, CS)
2. Verify chip select polarity
3. Ensure device is powered (VP and VA)
4. Check SPI clock frequency (max 4 MHz)
5. Verify ground connection

### Problem: "Unknown device ID"

**Solutions:**
1. Verify correct CS40L26 variant is connected
2. Check for SPI communication errors
3. Ensure device is out of reset
4. Verify power supply voltages

### Problem: "SPI communication timeout"

**Solutions:**
1. Reduce SPI clock frequency
2. Check for electrical noise
3. Verify cable lengths are appropriate
4. Check pull-up/pull-down resistors

## Supported Boards

The driver should work on any Zephyr-supported board with:
- SPI controller
- GPIO support
- Sufficient RAM (minimum 2KB stack)
- Power supply compatible with CS40L26 requirements

### Tested Boards

_(To be updated with actual test results)_

## API Reference

### Device Tree Compatible String

```
compatible = "cirrus,cs40l26";
```

### Device Tree Properties

| Property | Type | Required | Default | Description |
|----------|------|----------|---------|-------------|
| `reg` | int | Yes | - | SPI chip select number |
| `spi-max-frequency` | int | Yes | - | Max SPI clock frequency (≤4000000) |
| `reset-gpios` | phandle | No | - | Reset GPIO pin specification |
| `irq-gpios` | phandle | No | - | Interrupt GPIO pin specification |
| `devid` | int | No | 0x40A260 | Expected device ID |

### Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `CONFIG_CS40L26` | bool | n | Enable CS40L26 driver |
| `CONFIG_CS40L26_INIT_PRIORITY` | int | 80 | Driver init priority |
| `CONFIG_CS40L26_LOG_LEVEL` | choice | INF | Log verbosity level |

## Known Limitations

1. **Basic Functionality Only**: This initial port provides:
   - Device enumeration
   - Device identification
   - Basic diagnostics
   
2. **Not Yet Implemented**:
   - Haptic effect playback
   - Firmware loading
   - Waveform memory management
   - Interrupt handling
   - Advanced power management
   - Audio interface (I2S)

3. **Platform Requirements**:
   - Requires hardware SPI controller
   - GPIO support mandatory for reset
   - Minimum 2KB stack recommended

## Future Development

Planned enhancements:
- [ ] Haptic effect API
- [ ] Firmware loading support
- [ ] Interrupt-driven operation
- [ ] Power management integration
- [ ] Waveform programming
- [ ] Calibration support
- [ ] I2S audio interface
- [ ] GPIO triggering

## Contributing

To contribute enhancements or report issues:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Support

For questions or issues:
- Check the troubleshooting section
- Review Zephyr documentation
- Consult CS40L26 datasheet
- Open an issue on GitHub

## License

This driver is released under the Apache-2.0 license.

## Credits

- Original Linux kernel driver: Fred Treven (Cirrus Logic)
- Zephyr port: Developed for Zephyr RTOS integration
- Based on CS40L26 Linux driver from kernel_google-modules_amplifiers

## Version History

- v1.0.0 (Initial Release)
  - Basic SPI communication
  - Device enumeration
  - Device identification
  - Simple diagnostics
  - Sample application

## References

- [CS40L26 Product Page](https://www.cirrus.com/products/cs40l26/)
- [Zephyr Project](https://www.zephyrproject.org/)
- [Zephyr SPI API Documentation](https://docs.zephyrproject.org/latest/hardware/peripherals/spi.html)
- [Zephyr Device Tree Guide](https://docs.zephyrproject.org/latest/build/dts/index.html)
