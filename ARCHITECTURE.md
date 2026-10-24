# Architecture of microTelegramAPI

**English version** | [Русская версия](ARCHITECTURE_RU.md)

This document details the internal design, architectural principles, and engineering decisions behind the **microTelegramAPI** library.

---

## 1. Architectural Philosophy

Most existing Arduino Telegram bot libraries (such as FastBot, UniversalTelegramBot, etc.) were designed strictly for ESP8266/ESP32 microcontrollers with built-in Wi-Fi, tightly coupled to `WiFiClientSecure.h`.

When developers attempt to deploy these libraries in real-world industrial, telemetry, or remote field applications, they invariably face major limitations:
1. **Inability to swap transports**: connecting a GSM modem (SIM800L, SIM7600), wired Ethernet controller (W5500, ENC28J60), or cellular LTE module requires completely rewriting third-party code.
2. **Long Polling instability over cellular networks**: long-lived TCP sessions (waiting 20–30 seconds for server responses) are frequently severed by cellular carriers, causing embedded modem sockets to hang indefinitely.
3. **High memory consumption**: bloated parsers and complex internal queues consume valuable kilobytes of static RAM.

**microTelegramAPI** resolves these challenges based on three core tenets:
- **Total transport decoupling**: the library neither knows nor needs to know the underlying byte transmission medium (cellular modem, Ethernet, or radio link).
- **Synchronous determinism**: a predictable, robust cycle of "brief HTTP POST -> parse response -> close connection".
- **Zero-overhead templating**: eliminating dynamic dispatch and virtual table overhead.

---

## 2. Three-Layer Network Stack Model

Device communication with the Telegram API is separated into three decoupled architectural tiers:

```
┌─────────────────────────────────────────────────────────────────┐
│                    LAYER 3: APPLICATION                         │
│                   TelegramAPI<SSLTransport>                     │
│  - JSON construction (sendMessage, editMessage, getUpdates)     │
│  - Incoming message parsing (ArduinoJson v7)                    │
│  - Inline keyboard menus ([MENU]), auto HTML detection          │
│  - Access filtering (strict, whiteList, threadId)               │
└────────────────────────────────▲────────────────────────────────┘
                                 │
                                 │ SSLTransport (Stream / Client API)
                                 │
┌────────────────────────────────▼────────────────────────────────┐
│                    LAYER 2: SECURITY (TLS/SSL)                  │
│  Provides HTTPS encryption over TCP port 443                    │
│  - Option A (ESP): WiFiClientSecure / BearSSL                   │
│  - Option B (GSM/Ethernet): SSLClient (software TLS)            │
└────────────────────────────────▲────────────────────────────────┘
                                 │
                                 │ Client API (TCP socket)
                                 │
┌────────────────────────────────▼────────────────────────────────┐
│                    LAYER 1: PHYSICAL TRANSPORT                  │
│  Physical carrier and hardware socket management                │
│  - WiFiClient (ESP8266 / ESP32)                                 │
│  - TinyGsmClient (SIM800L, SIM7000, SIM7600, A9G)               │
│  - EthernetClient (W5100, W5500, ENC28J60)                      │
└─────────────────────────────────────────────────────────────────┘
```

### Advantages of This Decoupling
- **Modularity**: transitioning from Wi-Fi to GSM or Ethernet requires changing only the bottom-layer client initialization. The high-level bot code and `callback` logic remain 100% unchanged.
- **Portability**: the library compiles across any hardware platform (ESP32, ESP8266, STM32, RP2040, SAMD) that provides a basic `Client` class and C++ template support.

---

## 3. C++ Templating vs Virtual Polymorphism

In traditional OOP, transport abstraction is typically implemented via interfaces with virtual methods:

```cpp
// Traditional approach (NOT used in microTelegramAPI):
class TelegramAPI {
    Client* client; // Virtual calls -> vtable runtime overhead
};
```

In resource-constrained microcontrollers, virtual dispatch carries concrete costs:
1. Every method call resolves through a virtual method table (`vtable`), preventing the compiler from inlining functions.
2. The class footprint increases due to the hidden `vptr` pointer.
3. Third-party classes must inherit from a common base class.

In **microTelegramAPI**, parametric polymorphism is achieved via **C++ Templates**:

```cpp
template <typename SSLTransport>
class TelegramAPI {
public:
    TelegramAPI(SSLTransport& sslClient) : client(sslClient), apiHost(API_HOST) {}
    ...
private:
    SSLTransport& client;
};
```

### Architectural Benefits:
- **Direct inlining**: calls to `client.connect()`, `client.print()`, and `client.read()` are inlined directly by the compiler with zero indirection.
- **Static type checking**: any interface mismatches are caught at compile time.
- **Convenient TelegramESP Wrapper**: for common ESP32 and ESP8266 targets, `TelegramESP` simply derives from `TelegramAPI<WiFiClientSecure>` or `TelegramAPI<BearSSL::WiFiClientSecure>`, giving users an effortless zero-setup entry point.

---

## 4. Synchronous Loop and Adaptive Interactive Window

### Rollover-Safe Millis Timing
Timing intervals are evaluated using signed integer arithmetic:

```cpp
bool isReady() {
    if ((int32_t)(millis() - nextUpdate) > 0) {
        resetTimer();
        return true;
    }
    return false;
}
```

By casting the difference to `int32_t`, interval checks remain mathematically sound even across hardware `millis()` timer rollovers (occurring every 49.7 days of continuous runtime).

### Adaptive Interactive Window (Dynamic Polling)

Unlike Long Polling (where connections are held open for dozens of seconds), synchronous polling with brief HTTP requests prevents GSM carrier timeouts. However, polling too frequently exhausts cellular data quotas, while polling too slowly compromises interactivity.

The **Interactive Window** bridges both needs:

```
             User Conversation
       ──────────────────────────────
       Incoming / Outgoing Message
                   │
                   ▼
Base Polling     ┌──────────────────────────────┐     Base Polling
(e.g. every 60s) │ Fast Polling (1–2 seconds)   │     (e.g. every 60s)
───────────────► │ Active Window: 60 seconds    │ ──────────────────►
  (Idle State)   └──────────────────────────────┘      (Idle State)
```

1. **Idle State**: `nextUpdate = millis() + updatePeriod` (e.g. 60 seconds). Sockets are closed, cellular data consumption is zero, and the modem remains cool.
2. **Activity Trigger**:
   - Authorized incoming message received via `checkMessage`;
   - Outgoing message or menu sent via `sendMessage`;
   - Explicit manual call to `bot.triggerInteractive()`.
3. **Acceleration**: the active window boundary shifts to `interactiveWindowEnd = millis() + windowDuration`. Polling immediately accelerates to `interactivePeriod` (e.g. 2 seconds).
4. **Restoration**: once conversation ends and the window duration expires, `resetTimer()` automatically reverts back to the extended idle baseline.

---

## 5. Memory Management Strategy

The library is optimized for strict microcontroller RAM constraints:

1. **Flash String Storage (`F()` and `PSTR()`)**:
   All JSON keys (`F("chat_id")`, `F("parse_mode")`, `F("message_thread_id")`) and debug log strings are stored in Flash program memory, keeping RAM free.

2. **ArduinoJson v7 Integration**:
   Utilizes stack-allocated `JsonDocument` structures, avoiding heap allocation churn and preventing heap fragmentation over prolonged runtimes.

3. **Zero-Copy Parsing**:
   - `indexOf(',', pos)` operates on single characters without creating intermediate `String(",")` objects.
   - HTML detection inspects `strstr_P(message, PSTR("</"))` directly within the caller's buffer.
   - Whitelist validation parses 64-bit IDs using `atoll()` based on character indices without dynamically allocated lists.

---

## 6. Comparison: microTelegramAPI vs asyncTelegramAPI

| Metric | `microTelegramAPI` | `asyncTelegramAPI` |
|---|---|---|
| **I/O Model** | Synchronous, blocking per request | Asynchronous, non-blocking Finite State Machine (FSM) |
| **Supported Transports** | **Any transport** (GSM/GPRS SIM800L, LTE, Ethernet, Wi-Fi, SSLClient) | **Wi-Fi only** (ESP32 `WiFiClientSecure` / ESP8266 BearSSL) |
| **Connection Strategy** | Short HTTP POST with `Connection: close` | Persistent HTTP Keep-Alive (up to 30s connection reuse) |
| **Background Queues** | None (invoked directly in user code) | Yes (thread-safe FreeRTOS task queue on ESP32) |
| **RAM Footprint** | **Minimal (~1–2 KB)** | Moderate (~8–16 KB for queues and task stack) |
| **Interactive Mode** | Dynamic polling interval acceleration | Adaptive switch between Long Polling ↔ Short Poll |
| **Primary Use Case** | GSM modems, low-memory MCUs, battery/field telemetry | Feature-rich home automation bots on ESP32 with fast Wi-Fi |

---

## 7. Summary

**microTelegramAPI** provides a benchmark reference architecture for embedded systems:
- Hardware independence through C++ templates.
- Adaptive data conservation over cellular networks via interactive windows.
- Maximum stability, deterministic execution, and minimal resource usage.
