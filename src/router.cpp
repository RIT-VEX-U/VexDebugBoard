#include "router.h"

#include <algorithm>
#include <atomic>
#include <cstdio>

#include "HardwareSerial.h"
#include "WebSocketsServer.h"
#include "WiFi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"

namespace {
constexpr unsigned long kBaudRate = 921600;
constexpr size_t kUartRxBytes = 8192;
constexpr size_t kUartTxBytes = 2048;
constexpr size_t kSerialToWebsocketBytes = 32768;
constexpr size_t kWebsocketToSerialBytes = 8192;
constexpr size_t kUartChunkBytes = 512;
constexpr size_t kWebsocketChunkBytes = 1024;
constexpr uint32_t kFlushIntervalMs = 5;

WebSocketsServer websocket(8080);
HardwareSerial rs485_serial(1);
LED* status_led_ = nullptr;
LED* debug_led_ = nullptr;
StreamBufferHandle_t serial_to_websocket = nullptr;
StreamBufferHandle_t websocket_to_serial = nullptr;
std::atomic<uint32_t> rx_dropped_bytes{0};
std::atomic<uint32_t> tx_rejected_bytes{0};
std::atomic<uint32_t> uart_error_events{0};
std::atomic<uint32_t> last_rx_ms{0};
std::atomic<bool> forward_rx{false};
bool router_ready = false;
uint32_t last_flush_ms = 0;
uint32_t websocket_failed_sends = 0;

// The UART task is the sole producer of RX bytes and consumer of TX bytes.
// Network calls may block, so none run here. Each stream has one reader/writer.
void uart_task(void*) {
  uint8_t chunk[kUartChunkBytes];
  for (;;) {
    // Bound each pass so incoming traffic cannot starve outgoing traffic.
    for (size_t drained = 0; drained < kUartRxBytes; drained += kUartChunkBytes) {
      const size_t received = rs485_serial.read(chunk, sizeof(chunk));
      if (received == 0) break;
      last_rx_ms.store(millis(), std::memory_order_relaxed);
      if (!forward_rx.load(std::memory_order_relaxed)) continue;
      const size_t queued = xStreamBufferSend(serial_to_websocket, chunk, received, 0);
      // Preserve queued data and drop the newest bytes when the network falls behind.
      rx_dropped_bytes.fetch_add(received - queued, std::memory_order_relaxed);
    }

    const int writable = rs485_serial.availableForWrite();
    if (writable > 0) {
      const size_t length = xStreamBufferReceive(
          websocket_to_serial, chunk, std::min(sizeof(chunk), static_cast<size_t>(writable)), 0
      );
      if (length > 0) rs485_serial.write(chunk, length);
    }
    vTaskDelay(1);
  }
}

void websocket_event(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      printf("[%d] Disconnected!\n", num);
      if (websocket.connectedClients() == 0) {
        forward_rx.store(false, std::memory_order_relaxed);
        status_led_->set_blink();
      }
      break;
    case WStype_CONNECTED: {
      if (!forward_rx.load(std::memory_order_relaxed)) {
        // Clear any backlog from the previous session before enabling live capture.
        uint8_t discarded[kUartChunkBytes];
        while (xStreamBufferReceive(serial_to_websocket, discarded, sizeof(discarded), 0) > 0) {
        }
      }
      IPAddress ip = websocket.remoteIP(num);
      printf("[%d] Connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
      websocket.sendTXT(num, "Connected To Debug Board");
      forward_rx.store(true, std::memory_order_relaxed);
      status_led_->turn_on();
      break;
    }
    case WStype_BIN:
      // Admit complete messages only: partial commands must never reach the bus.
      // The other task can only free space between this check and the send.
      if (length > xStreamBufferSpacesAvailable(websocket_to_serial)) {
        tx_rejected_bytes.fetch_add(length, std::memory_order_relaxed);
        websocket.sendTXT(num, "Serial TX buffer full; binary message rejected");
      } else if (length > 0) {
        xStreamBufferSend(websocket_to_serial, payload, length, 0);
      }
      break;
    default:
      break;
  }
}
}  // namespace

void init_router(
    int RS485_TX_PIN, int RS485_RX_PIN, int RS485_DE_PIN, LED& status_led, LED& debug_led
) {
  status_led_ = &status_led;
  debug_led_ = &debug_led;
  serial_to_websocket = xStreamBufferCreate(kSerialToWebsocketBytes, 1);
  websocket_to_serial = xStreamBufferCreate(kWebsocketToSerialBytes, 1);
  if (!serial_to_websocket || !websocket_to_serial) {
    printf("Router buffer allocation failed\n");
    if (serial_to_websocket) vStreamBufferDelete(serial_to_websocket);
    if (websocket_to_serial) vStreamBufferDelete(websocket_to_serial);
    return;
  }

  pinMode(RS485_DE_PIN, OUTPUT);
  digitalWrite(RS485_DE_PIN, LOW);
  // Arduino requires buffer sizes to be configured before begin(). A lower FIFO
  // threshold gives the ISR more headroom at this baud rate.
  rs485_serial.setRxBufferSize(kUartRxBytes);
  rs485_serial.setTxBufferSize(kUartTxBytes);
  rs485_serial.begin(kBaudRate, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN, false, 20000, 64);
  if (!rs485_serial || !rs485_serial.setPins(RS485_RX_PIN, RS485_TX_PIN, -1, RS485_DE_PIN) ||
      !rs485_serial.setMode(UART_MODE_RS485_HALF_DUPLEX)) {
    printf("RS485 initialization failed\n");
    rs485_serial.end();
    vStreamBufferDelete(serial_to_websocket);
    vStreamBufferDelete(websocket_to_serial);
    return;
  }
  rs485_serial.onReceiveError([](hardwareSerial_error_t) {
    uart_error_events.fetch_add(1, std::memory_order_relaxed);
  });
  if (xTaskCreate(uart_task, "rs485-router", 3072, nullptr, 2, nullptr) != pdPASS) {
    printf("UART task creation failed\n");
    rs485_serial.end();
    vStreamBufferDelete(serial_to_websocket);
    vStreamBufferDelete(websocket_to_serial);
    return;
  }
  printf(
      "RS485 initialized: %lu baud, RX=%d TX=%d DE=%d\n", kBaudRate, RS485_RX_PIN, RS485_TX_PIN,
      RS485_DE_PIN
  );
  printf("Starting Websocket on ws://%s:8080/\n", WiFi.localIP().toString().c_str());
  websocket.begin();
  websocket.onEvent(websocket_event);
  router_ready = true;
}

void route_data() {
  if (!router_ready) return;
  websocket.loop();

  const uint32_t now = millis();
  const uint32_t last_rx = last_rx_ms.load(std::memory_order_relaxed);
  if (last_rx != 0 && now - last_rx < 50)
    debug_led_->turn_on();
  else
    debug_led_->turn_off();

  const size_t queued = xStreamBufferBytesAvailable(serial_to_websocket);
  if (queued >= kWebsocketChunkBytes || (queued > 0 && now - last_flush_ms >= kFlushIntervalMs)) {
    uint8_t chunk[kWebsocketChunkBytes];
    const size_t length = xStreamBufferReceive(serial_to_websocket, chunk, sizeof(chunk), 0);
    // Drain any remaining backlog when the last client has disconnected.
    if (length > 0) {
      for (uint8_t client = 0; client < WEBSOCKETS_SERVER_CLIENT_MAX; ++client) {
        if (websocket.clientIsConnected(client) && !websocket.sendBIN(client, chunk, length)) {
          ++websocket_failed_sends;
          // A short write may leave a partial frame on the wire. Reconnect
          // rather than append new frames to a corrupted connection.
          websocket.disconnect(client);
        }
      }
    }
    last_flush_ms = now;
  }

  static uint32_t last_report_ms = 0;
  if (now - last_report_ms >= 1000) {
    const uint32_t rx_dropped = rx_dropped_bytes.exchange(0);
    const uint32_t tx_rejected = tx_rejected_bytes.exchange(0);
    const uint32_t uart_errors = uart_error_events.exchange(0);
    if (rx_dropped || tx_rejected || uart_errors || websocket_failed_sends) {
      printf(
          "Router overload: RX dropped=%lu bytes, TX rejected=%lu bytes, UART errors=%lu, "
          "WebSocket failed sends=%lu\n",
          static_cast<unsigned long>(rx_dropped), static_cast<unsigned long>(tx_rejected),
          static_cast<unsigned long>(uart_errors),
          static_cast<unsigned long>(websocket_failed_sends)
      );
      websocket_failed_sends = 0;
    }
    last_report_ms = now;
  }
}
