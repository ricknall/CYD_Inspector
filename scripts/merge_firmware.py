"""PlatformIO custom target: build a browser-flashable merged ESP32 image.

Run from a terminal:
    pio run -e cyd2usb -t merged

Or use PlatformIO > Project Tasks > cyd2usb > Custom > merged.
"""

from pathlib import Path
import shutil
import subprocess

Import("env")  # type: ignore[name-defined]  # Supplied by PlatformIO/SCons.


def _first_existing(paths):
    for path in paths:
        if path and Path(path).is_file():
            return Path(path)
    return None


def merge_firmware(source, target, build_env):
    project_dir = Path(build_env.subst("$PROJECT_DIR"))
    build_dir = Path(build_env.subst("$BUILD_DIR"))

    platform = build_env.PioPlatform()
    esptool_package = Path(platform.get_package_dir("tool-esptoolpy"))
    framework_package = Path(
        platform.get_package_dir("framework-arduinoespressif32")
    )

    esptool = _first_existing(
        [
            esptool_package / "esptool.py",
            esptool_package / "esptool" / "__main__.py",
        ]
    )
    boot_app0 = _first_existing(
        [
            framework_package / "tools" / "partitions" / "boot_app0.bin",
            framework_package / "tools" / "partitions" / "boot_app0.bin",
        ]
    )

    required = {
        "esptool": esptool,
        "bootloader": build_dir / "bootloader.bin",
        "partition table": build_dir / "partitions.bin",
        "boot_app0": boot_app0,
        "application": build_dir / "firmware.bin",
    }
    missing = [name for name, path in required.items() if not path or not Path(path).is_file()]
    if missing:
        raise RuntimeError("Cannot create merged image; missing: " + ", ".join(missing))

    dist_dir = project_dir / "dist"
    web_dir = project_dir / "web"
    dist_dir.mkdir(exist_ok=True)
    web_dir.mkdir(exist_ok=True)

    merged = dist_dir / "CYD_Board_Inspector.merged.bin"
    command = [
        build_env.subst("$PYTHONEXE"),
        str(esptool),
        "--chip",
        "esp32",
        "merge_bin",
        "-o",
        str(merged),
        "--flash_mode",
        "dio",
        "--flash_freq",
        "40m",
        "--flash_size",
        "4MB",
        "0x1000",
        str(required["bootloader"]),
        "0x8000",
        str(required["partition table"]),
        "0xe000",
        str(required["boot_app0"]),
        "0x10000",
        str(required["application"]),
    ]

    print("Creating browser-flashable merged image...")
    subprocess.run(command, check=True)
    shutil.copy2(merged, web_dir / merged.name)
    shutil.copy2(required["application"], dist_dir / "CYD_Board_Inspector.bin")
    print(f"Merged image: {merged}")
    print(f"Web installer copy: {web_dir / merged.name}")


env.AddCustomTarget(
    name="merged",
    dependencies="$BUILD_DIR/${PROGNAME}.bin",
    actions=merge_firmware,
    title="Merged browser firmware",
    description="Create a single ESP32 image flashable at address 0x0000",
)
