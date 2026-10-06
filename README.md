# VexDebugBoard

Arduino firmware for an ESP32-C3 RS485-to-WebSocket debug bridge, built and
uploaded using PlatformIO. The RS485 port runs at **921600 baud, 8N1**; the
diagnostic console remains at 115200 baud. It connects to the Wi-Fi network
configured in `src/main.cpp` and serves `ws://<board-ip>:8080/`.

## Serial routing and buffering

The UART runs in a dedicated FreeRTOS task, independent of WebSocket servicing.
Its driver has an 8 KB receive buffer and a 2 KB transmit buffer. Two bounded
stream buffers connect it to the main loop:

| Direction | Buffer | Transfer behavior |
| --- | --- | --- |
| Serial to WebSocket | 32 KB | Binary frames of up to 1024 bytes; smaller bursts flush after 5 ms when the network loop is available |
| WebSocket to serial | 8 KB | Complete binary messages queued and written in chunks of up to 512 bytes |

At a saturated 8N1 link, 921600 baud carries 92,160 bytes/second. The 32 KB
outbound buffer covers approximately 356 ms of network delay. Buffer sizes,
chunk sizes, and the flush interval are constants in `src/router.cpp`.
UART buffer sizes are set before starting the port, as required by the
[Arduino ESP32 serial API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/serial.html).

Clients must treat binary frames as chunks of one byte stream: frame boundaries
do not correspond to serial protocol messages. The connection greeting and
transmit rejection notices are text frames. Send complete, unfragmented binary
messages; a message larger than the available 8 KB transmit queue is rejected
whole with a text notice. Existing queued commands remain in order.

Serial traffic is drained and discarded when no clients are connected. All
connected clients receive the same live stream. If the outbound buffer fills,
the newest bytes are dropped while queued bytes stay in order. The console
reports dropped RX bytes, rejected TX bytes, UART error events, and failed
WebSocket sends at most once per second. A client whose binary send fails is
disconnected because a partial frame could corrupt subsequent traffic.
These counters cover the interval
since the last report; UART error events are not a count of lost bytes.

The WebSocket library uses synchronous network writes. A slow client can stall
broadcasting to other clients, and finite buffering cannot guarantee lossless
capture during prolonged network stalls. The UART task continues draining the
port during those stalls. RS485 remains half duplex with hardware RTS controlling
DE; the sender and receiver must coordinate bus ownership before transmitting.

To validate on hardware, send a known byte pattern continuously at 921600 baud
through an RS485 adapter while a WebSocket client consumes binary frames.
Compare the concatenated received bytes to the transmitted pattern and check
the console for overload reports. Run for at least a minute, then repeat with
multiple clients and a deliberately stalled reader. Also verify small bursts,
WebSocket-to-serial byte ordering, transmit queue rejection, and reconnects.

## Board selection

`platformio.ini` targets `esp32-c3-devkitm-1` for the custom ESP32-C3-WROOM board.
RS485 uses TX GPIO 6, RX GPIO 7, and DE GPIO 10. Status and activity LEDs use
GPIOs 4 and 18. Pin definitions live in the environment's build flags.

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

Start in `src/main.cpp` for Arduino `setup()` and `loop()`, and `src/router.cpp`
for UART and WebSocket routing. PlatformIO automatically builds sources under
`src/` for the `esp32c3_custom` environment.
