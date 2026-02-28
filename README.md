# Apple2Joy — USB Gamepad to Apple IIe Joystick Adapter

Connects USB game controllers to the Apple IIe joystick port via a Raspberry Pi Pico. Uses the [MCP4251](https://www.microchip.com/en-us/product/MCP4251) dual digital potentiometer for analog axes and GPIO outputs for buttons.

The adapter supports multiple joysticks through a USB hub and the Pico's OTG port. Controllers are identified by their report format rather than by VID/PID, so most clones work out of the box. Latency is subjectively unnoticeable even over USB 1.1.

## Supported Controllers

| Controller              | Status |
| ----------------------- | ------ |
| PS5 DualSense           | ✓      |
| PS4 DualShock 4         | ✓      |
| Xbox 360 Wired          | ✓      |
| 8BitDo SN30 Pro         | ✓      |
| Competition Pro USB     | ✓      |
| Generic USB 1.1 Gamepad | ✓      |
| Xbox One                | ✗      |
| PS3 Controller          | ✗      |

## Project Structure

```
apple2joy/
├── include/
│   ├── apple2joy.h       # Joystick output library API
│   └── mcp4251.h         # Digital potentiometer driver API
├── src/
│   ├── apple2joy.c       # D-pad mapping, button output, pot control
│   └── mcp4251.c         # SPI driver for MCP4251
├── firmware/
│   └── main.c            # USB-to-Apple-IIe adapter firmware
├── lib/
│   └── tjuh/             # USB host library (git submodule)
└── CMakeLists.txt
```

The **apple2joy library** (in `include/` + `src/`) is independent of USB — it takes axis/button values and drives the hardware. It can be reused for other input sources.

The **firmware** (in `firmware/`) wires together [TJUH](https://github.com/mwyborski/tjuh) (USB gamepad reading) and the apple2joy library into a complete adapter.

## Hardware

### Components

- Raspberry Pi Pico (RP2040)
- MCP4251 dual 8-bit digital potentiometer on SPI0
- Transmission gate for Apple IIe pushbuttons (active-high GPIO → Apple IIe PB0/PB1)
- USB OTG adapter (Micro-USB)

### Connections

| Pico Pin          | Function         | Target                   |
| ----------------- | ---------------- | ------------------------ |
| SPI0 SCK (GP18)   | SPI Clock        | MCP4251 SCK              |
| SPI0 TX (GP19)    | SPI MOSI         | MCP4251 SDI              |
| SPI0 RX (GP16)    | SPI MISO         | MCP4251 SDO              |
| SPI0 CSn (GP17)   | SPI Chip Select  | MCP4251 CS               |
| GP6               | Button 0 (PB0)   | Apple IIe PB0            |
| GP7               | Button 1 (PB1)   | Apple IIe PB1            |
| USB Micro-B       | USB Host (OTG)   | Gamepad via OTG cable    |

The MCP4251's two potentiometers map directly to the Apple IIe paddle inputs:

- Wiper 0 → PDL(0) / X axis
- Wiper 1 → PDL(1) / Y axis

The potentiometer has 8-bit resolution matching the Apple IIe joystick circuit. By selecting the correct capacitors, the full 8-bit range can be mapped 1:1.

### Button mapping (default)

| Gamepad          | Apple IIe |
| ---------------- | --------- |
| Cross / Triangle | PB0       |
| Circle / Square  | PB1       |

Edit `firmware/main.c` to change the mapping.

### Axis behavior

When the D-pad is active, it overrides analog input with fixed directional values (min/mid/max). When the D-pad is released, the left analog stick passes through directly.

## Building

### Prerequisites

- [Pico SDK](https://github.com/raspberrypi/pico-sdk) installed
- `PICO_SDK_PATH` environment variable set
- ARM GCC toolchain

### Setup

```bash
git clone https://github.com/mwyborski/apple2joy.git
cd apple2joy
git submodule add https://github.com/mwyborski/tjuh.git lib/tjuh
git submodule update --init
```

### Build

```bash
mkdir build
cd build
cmake ..
make -j
```

Flash `apple2joy_firmware.uf2` to the Pico.

### Library-only mode

To use apple2joy as a library in another project without building the firmware:

```cmake
set(APPLE2JOY_BUILD_FIRMWARE OFF)
add_subdirectory(path/to/apple2joy)
target_link_libraries(my_app apple2joy_lib hardware_spi)
```

## Configuration

Adjustable in `firmware/main.c`:

- **Button mapping**: change which gamepad buttons map to PB0/PB1 in `on_gamepad_report()`
- **GPIO pins**: change `gpio_button_base` in the `apple2joy_config_t`
- **Axis swap**: set `swap_axes = true` if X/Y are reversed on the Apple IIe side
- **Hardware test**: uncomment `apple2joy_run_test()` to sweep pots and toggle buttons on startup

## Apple IIe Joystick Resources

- [Apple IIe Technical Note #6 — Joystick/Paddle Circuits](http://www.gno.org/pub/apple2/doc/apple/technotes/aiie/tn.aiie.006)
- [Atari Archives — Computer Controller Cookbook](https://www.atariarchives.org/ccc/chapter1.php)
- [Apple II Pinouts Reference](http://www.1000bit.it/support/manuali/apple/R023PINOUTS.TXT)
- [Apple II FAQ — Keyboard/Paddles/Joysticks](https://gswv.apple2.org.za/a2zine/faqs/Csa2KBPADJS.html)
- [Converting a CH Mach III Joystick for Apple II](http://www.apple2faq.com/apple2faq/convert-ch-mach-iii-joystick-apple-ii/)

## Remarks

If you need an OTG cable, you can make one yourself:
https://www.instructables.com/Make-a-USB-OTG-host-cable-The-easy-way/

For Xbox One controller support, see:
https://github.com/Ryzee119/tusb_xinput

## License

MIT License — see [LICENSE](LICENSE).
