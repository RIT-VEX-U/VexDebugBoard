# VexDebugBoard

ESP-IDF firmware built and uploaded using PlatformIO. The starter application
prints a heartbeat every second over the serial console at 115200 baud.
The project pins Espressif32 platform 7.1.3, which supplies ESP-IDF 6.1.0.

## Board selection

`platformio.ini` currently targets `esp32dev`, a generic classic ESP32 development
board (typically ESP32-WROOM-32 with 4 MB flash). Confirm your hardware before
uploading. ESP32-S3, ESP32-C3, and other variants need their own PlatformIO board
ID; change `board` to match your exact board.
The flash size in `sdkconfig.defaults` must also match the board. If changing
defaults after a build, update the existing configuration through `menuconfig`
or regenerate the ignored `sdkconfig.esp32dev` file.

## Command line

Run these in a PlatformIO terminal (VS Code command **PlatformIO: New Terminal**),
or a shell with PlatformIO Core installed:

```sh
pio run
pio device list
pio run -t upload
pio device monitor
```

Exit the serial monitor with Ctrl+C before uploading again. To select a specific
port, use `pio run -t upload --upload-port /dev/ttyUSB0` and
`pio device monitor --port /dev/ttyUSB0`, substituting your actual port, or set
`upload_port` and `monitor_port` in `platformio.ini`.

If uploading stays at `Connecting...`, hold the board's BOOT button while the
upload connects, then release it when writing begins. If necessary, press EN/RESET
while holding BOOT. A missing port usually means a charge-only USB cable or a
missing USB-to-serial driver. For Linux permission errors, follow PlatformIO's
[serial-port permissions instructions](https://docs.platformio.org/en/latest/core/installation/udev-rules.html).

## Neovim / clangd

Generate the compilation database for the current Arduino / ESP32-C3 environment:

```sh
pio run -e esp32c3_custom -t compiledb
```

The project's `.clangd` reads `compile_commands.json` from the project root and
removes GCC-only flags from editor analysis. Regenerate the database after
changing build flags, libraries, source files, or the PlatformIO environment.
The generated database contains local paths and is ignored by Git.

Allow clangd to query the ESP32-C3 compiler for system headers in your Neovim
clangd launch arguments:

```lua
"--query-driver=" .. vim.fn.expand("~") .. "/.platformio/packages/toolchain-riscv32-esp/bin/riscv32-esp-elf-*"
```

If you work with other boards, keep their compiler patterns in the same argument,
separated by commas. Do not force `--compile-commands-dir=build`.
Restart Neovim after changing its language-server launch arguments.

## Editing firmware

Start in `src/main.c`. ESP-IDF enters the application through `app_main()`.
Register new source files in `src/CMakeLists.txt`. Run `pio run -t menuconfig`
for ESP-IDF configuration; keep intentional shared settings in
`sdkconfig.defaults` rather than committing generated `sdkconfig` files.

Reference: [PlatformIO's ESP-IDF guide](https://docs.platformio.org/en/latest/frameworks/espidf.html).
