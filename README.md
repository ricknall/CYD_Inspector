# CYD Board Inspector — PlatformIO Edition

CYD Board Inspector is a native PlatformIO diagnostic utility for ESP32 Cheap
Yellow Display boards. It reports the ESP32, flash, memory, display, storage,
and peripheral findings on both the LCD and the 115200-baud serial console.

The currently verified peripheral mappings apply to the tested classic
single-USB CYD. Selecting a board profile is human-assisted and does not assume
that a visually similar board uses the same wiring.

## Verified on the classic single-USB CYD

- Display SPI path and controller probing
- XPT2046-compatible resistive touch:
  - MOSI 32, MISO 39, SCLK 25, CS 33, IRQ 36
  - five-point calibration with persistent storage
  - mapped screen-coordinate monitor
- Active-low RGB LED:
  - GPIO 4 red
  - GPIO 16 green
  - GPIO 17 blue
- Light sensor on GPIO 34; darker conditions produce higher raw ADC readings
- Audio on GPIO 26 / ESP32 DAC2 through the SC8002B amplifier to P4
- MicroSD SPI wiring: MOSI 23, MISO 19, SCLK 18, CS 5

The SD interface is reported, but physical media validation remains deferred.
Peripheral mappings for dual-USB CYD variants are not yet claimed.

## Open the PlatformIO project

1. Extract the project to a normal directory. Do not work inside a ZIP viewer.
2. In VS Code choose **File > Open Folder**.
3. Select the folder containing `platformio.ini`.
4. Allow PlatformIO to download the pinned ESP32 platform and toolchain on the
   first build.

## Build, upload, and monitor

1. Connect the CYD through the USB connector that appears in Windows as a COM
   port.
2. Open **PlatformIO > Project Tasks > cyd2usb > General**.
3. Run **Build**, then **Upload**.
4. Open **Monitor** at 115200 baud.

PlatformIO normally detects the port. To force one temporarily from PowerShell:

```powershell
pio run -e cyd2usb -t upload --upload-port COM9
pio device monitor --port COM9 --baud 115200
```

Replace `COM9` with the port currently assigned by Windows.

## Serial commands

```text
help              Show the command list
report            Print the complete human-readable report
profile            Show profile-selection instructions
profile 1          Select classic CYD with one USB connector
profile 2          Select CYD2USB with two USB connectors
profile clear      Clear the saved board selection
system             Print MCU, memory, reset, and software details
display            Print display profile and probe details
peripherals        Print SD and peripheral details
touch              Start or stop the mapped touch monitor
calibrate          Run five-point touch calibration and save it
calibrate clear    Delete saved touch calibration
rgb                Show confirmed RGB mapping and current state
rgb 4              Turn red on
rgb 16             Turn green on
rgb 17             Turn blue on
rgb off            Turn all RGB channels off
light              Read the confirmed GPIO 34 light sensor
speaker            Show confirmed GPIO 26 speaker instructions
speaker test       Play three brief, low-level DAC beeps
speaker off        Return GPIO 26 to high-impedance input mode
json               Print the complete report as JSON
```

### Speaker safety

P4 is the amplifier's bridged output. Connect a speaker between P4 VO1 and VO2;
do not connect either P4 pin to board ground. For the conservative first test,
place about 100 ohms in series with a small 4–8 ohm speaker.

## Browser installer

The public installer is designed for GitHub Pages at:

```text
https://ricknall.github.io/CYD_Inspector/
```

Use desktop Chrome or Edge with a data-capable USB cable. Close PlatformIO's
Serial Monitor before connecting. Installing erases the board's existing
firmware and saved settings.

### Generate the browser firmware

After a normal build succeeds, run:

```powershell
pio run -e cyd2usb -t merged
```

Or select **Project Tasks > cyd2usb > Custom > merged**. This creates:

```text
dist/CYD_Board_Inspector.merged.bin
dist/CYD_Board_Inspector.bin
docs/CYD_Board_Inspector.merged.bin
```

The generated `docs/CYD_Board_Inspector.merged.bin` must be committed with the
installer page when publishing a browser-installable version.

## Version 1.2.0

Version 1.2.0 completes the tested classic single-USB CYD peripheral pass:
persistent touch calibration, confirmed RGB channels, confirmed GPIO 34 light
sensor, and the safe GPIO 26 DAC speaker test.

The project pins the pioarduino Espressif platform corresponding to
Arduino-ESP32 3.3.1. No external libraries are required.
