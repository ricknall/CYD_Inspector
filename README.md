# CYD Board Inspector — PlatformIO Edition

A native PlatformIO CYD diagnostic utility. It exercises the known-compatible
display path; reports the ESP32, flash, PSRAM, display, and SD findings; and
records a human-assisted single-USB or two-USB board-family selection without
guessing peripheral pin mappings. The selected profile is saved in ESP32
nonvolatile storage and restored after reset or power loss.

## Why this is a real PlatformIO project

- `platformio.ini` defines and pins the build environment.
- Application code lives under `src/`.
- Board-specific constants live under `include/`.
- The display driver is separated from the diagnostic application.
- No external libraries are required.
- A custom `merged` target produces the image used by the browser installer.

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

The LCD overview should show:

- `CYD BOARD INSPECTOR`
- `DISPLAY SPI: WORKING`
- the probed display-controller result
- `BOARD: UNKNOWN` until a profile is selected
- MCU, core, revision, flash, and PSRAM information

In the 115200-baud Serial Monitor, type `profile 1` for a physically observed
single-USB board or `profile 2` for a two-USB board. Type `profile clear` to
remove both the active and saved selection. Reset the board after selecting or
clearing a profile to verify that the choice was retained or removed.
Connector count identifies the physical family; it does not prove touch, RGB
LED, speaker, or light-sensor pin mappings.

## Browser installer

The public installer is designed for GitHub Pages at:

```text
https://ricknall.github.io/CYD_Inspector/
```

GitHub Pages must be configured to deploy from the `main` branch and `/docs`
folder. The installer requires desktop Chrome or Edge and a data-capable USB
cable. Installing erases the target board's existing firmware and settings.

### Build the installer firmware

After the normal build works, run the custom target:

```text
pio run -e cyd2usb -t merged
```

Or use **Project Tasks > cyd2usb > Custom > merged**.

It creates:

```text
dist/CYD_Board_Inspector.merged.bin
docs/CYD_Board_Inspector.merged.bin
```

The `docs/manifest.json` file expects the merged image in the `docs` directory.
Commit the generated `docs/CYD_Board_Inspector.merged.bin` with the installer
page when preparing a browser-installable release.

## Build environment

The original GitHub workflow used Arduino-ESP32 3.3.1. This project pins the
corresponding pioarduino platform release so the conversion is not silently
built against an older Arduino core.
