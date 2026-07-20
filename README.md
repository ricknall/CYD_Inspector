# CYD Board Inspector — PlatformIO Edition

A native PlatformIO conversion of the original single-file Arduino sketch.
It identifies the two-USB Cheap Yellow Display family by exercising the known
ST7789 display path and reports the ESP32 model, core count, revision, flash
capacity, and PSRAM status.

## Why this is a real PlatformIO project

- `platformio.ini` defines and pins the build environment.
- Application code lives under `src/`.
- Board-specific constants live under `include/`.
- The display driver is separated from the diagnostic application.
- No external libraries are required.
- A custom `merged` target can produce a browser-flashable image later.

## Open it

1. Extract the ZIP to a normal directory. Do not work inside 7-Zip.
2. In VS Code choose **File > Open Folder**.
3. Select the folder containing `platformio.ini`.
4. Allow PlatformIO to download the pinned ESP32 platform and toolchain on the
   first build.

## Build and upload

1. Connect the CYD through the USB connector that appears in Windows as a COM
   port.
2. Open the PlatformIO alien-head panel.
3. Expand **Project Tasks > cyd2usb > General**.
4. Select **Build**.
5. After a successful build, select **Upload**.
6. Select **Monitor** to view the 115200-baud diagnostic output.

The normal PlatformIO toolbar buttons at the bottom of VS Code perform the same
Build, Upload, and Monitor operations.

## What success looks like

The LCD should show:

- `CYD BOARD INSPECTOR`
- `DISPLAY PATH: ST7789`
- `ESP32-2432S028`
- `CYD2USB / TWO USB`
- MCU, core, revision, flash, and PSRAM information

If that page is readable, the tested display path matches the expected two-USB
CYD hardware configuration.

## Optional browser-flash image

After the normal build works, run the custom target:

```text
pio run -e cyd2usb -t merged
```

Or use **Project Tasks > cyd2usb > Custom > merged**.

It creates:

```text
dist/CYD_Board_Inspector.merged.bin
web/CYD_Board_Inspector.merged.bin
```

The `web/manifest.json` file expects the merged image in the `web` directory.

## Build environment

The original GitHub workflow used Arduino-ESP32 3.3.1. This project pins the
corresponding pioarduino platform release so the conversion is not silently
built against an older Arduino core.
