# microTelegramAPI

**English version** | [Русская версия](README.md)

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-blue.svg)](https://www.arduino.cc/)
[![License: LGPL 3.0](https://img.shields.io/badge/License-LGPL_3.0-green.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.2.0-blue.svg)](library.properties)

**microTelegramAPI** is a compact, lightweight, and 100% synchronous Telegram Bot API library for Arduino, ESP8266, ESP32, and STM32 microcontrollers.

The distinctive feature of this library is **complete network transport independence**. It is engineered to operate reliably not only over Wi-Fi, but also over cellular GSM/GPRS/LTE modems (SIM800L, SIM7000, SIM7600, etc.), wired Ethernet, or custom data links using SSLClient encryption.

> 💡 **Looking for an Asynchronous / FreeRTOS version for ESP32/ESP8266?**  
> If your project targets ESP32 or ESP8266 and requires non-blocking I/O, persistent keep-alive HTTPS connections, FreeRTOS background queues, and asynchronous message delivery, check out our companion library [asyncTelegramAPI](https://github.com/SerhiiLe/asyncTelegramAPI/).  
> **microTelegramAPI** is dedicated to maximum simplicity, minimal memory footprint, and universal transport compatibility.

---

## Table of Contents

1. [Key Features](#key-features)
2. [Transport Architecture](#transport-architecture)
3. [Installation](#installation)
4. [Quick Start](#quick-start)
   - [For ESP8266 and ESP32 (TelegramESP)](#1-for-esp8266-and-esp32-telegramesp)
   - [For GSM/SIM800L via SSLClient](#2-for-gsmsim800l-via-sslclient)
5. [Interactive Window (Adaptive Polling)](#interactive-window-adaptive-polling)
6. [API Reference](#api-reference)
   - [Initialization and Configuration](#initialization-and-configuration)
   - [Sending Messages](#sending-messages)
   - [Editing Messages](#editing-messages)
   - [Deleting Messages](#deleting-messages)
   - [Receiving Messages and Callback](#receiving-messages-and-callback)
   - [Whitelist and Security](#whitelist-and-security)
   - [Keyboard Menu [MENU]](#keyboard-menu-menu)
   - [Automatic HTML Mode](#automatic-html-mode)
7. [Detailed Architecture](#detailed-architecture)
8. [Changelog](#changelog)
9. [License](#license)

---

## Key Features

- 🚀 **Transport Agnostic**: runs over Wi-Fi, TinyGSM (SIM800L, LTE), EthernetClient via `SSLClient`.
- 🪶 **Minimal Memory Footprint**: no virtual table overhead (C++ templates), all keys and static strings in Flash (`F()` and `PSTR()`), saving up to 8–10 KB RAM compared to heavy libraries.
- ⚡ **Adaptive Interactive Window (`setInteractiveWindow`)**: polls infrequently in idle mode (e.g. every 60s to conserve GSM cellular data and battery), then temporarily accelerates to 1–2s during active user dialogs.
- 💬 **Supergroup Forum Topics**: full `threadId` parameter support and automatic routing of callback replies back to the origin topic.
- ✏️ **Message Editing (`editMessage`)**: update existing bot messages dynamically on the fly.
- 🗑️ **Bulk Deletion (`deleteMessages`)**: delete messages in batches using initializer lists `{id1, id2}` or `std::vector<int64_t>`.
- 🛡️ **Access Control**: flexible whitelist (`whiteList`), robust 64-bit ID parsing via `atoll()`, and `strict` authorization mode.
- ⌨️ **Embedded Menus**: intuitive Reply-Keyboard creation directly in message strings using the `[MENU]` tag.
- 🌐 **Automatic HTML Mode**: any text containing `</...>` tags automatically sends `"parse_mode": "HTML"`.

---

## Transport Architecture

The library separates network responsibilities into three decoupled layers:

```
┌────────────────────────────────────────────────────────┐
│             User Application / Callback Logic          │
└───────────────────────────▲────────────────────────────┘
                            │
┌───────────────────────────▼────────────────────────────┐
│          TelegramAPI<SSLTransport> (Protocol)          │
│     JSON construction, getUpdates parser, [MENU], HTML │
└───────────────────────────▲────────────────────────────┘
                            │ print() / read()
┌───────────────────────────▼────────────────────────────┐
│            SSL/TLS Layer (Encryption)                  │
│    WiFiClientSecure (ESP)  /  SSLClient (GSM/Ethernet) │
└───────────────────────────▲────────────────────────────┘
                            │
┌───────────────────────────▼────────────────────────────┐
│            Physical Transport Layer (Socket)           │
│        WiFiClient / TinyGsmClient / EthernetClient     │
└────────────────────────────────────────────────────────┘
```

---

## Installation

### PlatformIO

Add the library and `ArduinoJson` dependency to your `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    bblanchon/ArduinoJson @ ^7.0.0
    https://github.com/SerhiiLe/microTelegramAPI
```

### Arduino IDE

1. Install the **ArduinoJson** library (version 7.x) via Library Manager.
2. Download the [microTelegramAPI](https://github.com/SerhiiLe/microTelegramAPI) repository ZIP and extract it into your `libraries/` directory.

---

## Quick Start

### 1. For ESP8266 and ESP32 (TelegramESP)

For ESP boards, the library includes a ready-to-use `TelegramESP` wrapper that encapsulates the built-in TLS client (`WiFiClientSecure` / BearSSL):

```cpp
#include <Arduino.h>
#include <TelegramESP.h>

TelegramESP bot;

String callback(TResult &msg) {
    if (msg.text == "/start") {
        return "Hello, " + msg.from + "! Bot online.[MENU]Button 1\tButton 2\nHelp";
    }
    if (msg.text == "Button 1") {
        return "You tapped button 1!";
    }
    return "Echo: " + msg.text;
}

void setup() {
    Serial.begin(115200);
    WiFi.begin("SSID", "PASSWORD");
    while (WiFi.status() != WL_CONNECTED) delay(500);

    bot.setBotToken("XXXXXXXXXX:YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY");
    bot.setChatID(123456789);        // Default chat ID
    bot.setInterval(10);             // Polling interval in idle (10s)
    bot.setInteractiveWindow(60, 2); // Accelerate to 2s polling for 60s upon activity
    bot.attachCallback(callback);

    bot.sendMessage("Bot online!");
}

void loop() {
    bot.checkMessage();
}
```

### 2. For GSM/SIM800L via SSLClient

For cellular modems, connect `TinyGSM` with `SSLClient`:

```cpp
#include <Arduino.h>
#include <TelegramAPI.h>

#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#include <SSLClient.h>

// Modem UART pins
HardwareSerial gsmSerial(1);
TinyGsm modem(gsmSerial);
TinyGsmClient gsmTransport(modem);
SSLClient secureClient(&gsmTransport);

// Initialize bot with SSLClient transport
TelegramAPI<SSLClient> bot(secureClient);

String callback(TResult &msg) {
    return "GSM Reply: " + msg.text;
}

void setup() {
    Serial.begin(115200);
    gsmSerial.begin(9600, SERIAL_8N1, 16, 17);

    modem.restart();
    modem.gprsConnect("internet", "", "");

    bot.setBotToken("TOKEN");
    bot.setChatID(123456789);
    bot.setInterval(60);             // Conserve mobile data: poll every 60s
    bot.setInteractiveWindow(60, 2); // Accelerate to 2s during conversations
    bot.attachCallback(callback);
}

void loop() {
    bot.checkMessage();
}
```

---

## Interactive Window (Adaptive Polling)

On cellular data plans (SIM800L, LTE), frequent polling (every 1–2s) burns through data limits and battery. Conversely, a 60-second fixed poll makes interactive bot conversations sluggish.

`microTelegramAPI` solves this with an **Adaptive Interactive Window**:

```cpp
bot.setInterval(60);             // Idle baseline: 60 seconds
bot.setInteractiveWindow(60, 2); // Active window: 60 seconds with 2s polling
```

- **How it functions**:
  - While idle, the bot executes a single brief HTTP request once every minute.
  - As soon as the user sends a message or the bot transmits an alert/menu, the interactive window extends for 60 seconds, accelerating polling to once every 2 seconds.
  - If no further interaction occurs within 60 seconds, polling seamlessly drops back to the 60-second idle rate.

---

## API Reference

### Initialization and Configuration

| Method | Description |
|---|---|
| `TelegramAPI(SSLTransport& client)` | Bot constructor binding to an SSL/TLS client instance. |
| `void setBotToken(const char* / String)` | Sets the bot authorization token from @BotFather. |
| `void setChatID(int64_t chatId)` | Sets default chat ID (for reports and targetless sends). |
| `void setInterval(int interval)` | Polling interval in seconds (resets current timer). |
| `void setNext(uint32_t next = 0)` | Sets a one-time delay in seconds until the next poll. |
| `void setInteractiveWindow(uint32_t window, uint32_t fastInterval = 1)` | Configures the accelerated polling window upon activity. |
| `void triggerInteractive()` | Manually starts or extends the interactive window. |
| `void attachCallback(String (*handler)(TResult& tr))` | Registers incoming message handler. |
| `void detachCallback()` | Detaches the message handler. |
| `void attachCheckConnection(bool (*handler)())` | Registers an external network connectivity check function. |
| `void detachCheckConnection()` | Detaches the connectivity check. |
| `void skipUpdates()` | Discards all pending backlog updates on the server. |

---

### Sending Messages

```cpp
// Direct sending (with optional supergroup threadId)
bool sendMessage(int64_t chatId, const char* message, int64_t threadId = 0);
bool sendMessage(int64_t chatId, const String& message, int64_t threadId = 0);

// Default chat sending (setChatID)
bool sendMessage(const char* message, int64_t threadId = 0);
bool sendMessage(const String& message, int64_t threadId = 0);

// Broadcast to comma-separated ID list: "11111,22222,33333"
bool sendMessageToAll(const String& list, const char* message, int64_t threadId = 0);
bool sendMessageToAll(const String& list, const String& message, int64_t threadId = 0);
bool sendMessageToAll(const char* message, int64_t threadId = 0); // uses bot.whiteList
bool sendMessageToAll(const String& message, int64_t threadId = 0);
```

---

### Editing Messages

```cpp
// Edit message in specified chat
bool editMessage(int64_t chatId, int64_t messageId, const char* message);
bool editMessage(int64_t chatId, int64_t messageId, const String& message);

// Edit message in default chat (setChatID)
bool editMessage(int64_t messageId, const char* message);
bool editMessage(int64_t messageId, const String& message);
```

---

### Deleting Messages

```cpp
// Delete single message
bool deleteMessage(int64_t chatId, int64_t messageId);
bool deleteMessage(int64_t messageId); // default chat

// Delete multiple messages using initializer list
bool deleteMessages(int64_t chatId, std::initializer_list<int64_t> messageIDs);
bool deleteMessages(std::initializer_list<int64_t> messageIDs);

// Delete multiple messages using std::vector
bool deleteMessages(int64_t chatId, const std::vector<int64_t>& messageIDs);
bool deleteMessages(const std::vector<int64_t>& messageIDs);
```

---

### Receiving Messages and Callback

Call `checkMessage(bool force = false)` in your main `loop()`. Returns the number of processed messages (0 if not due, or `< 0` on network error).

Incoming message structure:

```cpp
struct TResult {
    int64_t chatId;        // Sender chat ID
    int64_t messageId;     // Message ID
    int64_t threadId = 0;  // Supergroup topic ID (0 for direct chats / general topic)
    String text;           // Message text
    String from;           // Sender first_name
};
```

Callback signature:
```cpp
String callback(TResult &msg) {
    // Return text to reply directly to the same chat and topic.
    // Return empty string "" if no response is needed.
    if (msg.text == "ping") return "pong";
    return "";
}
```

---

### Whitelist and Security

- **`bot.strict = true;`** — authorization filter. When enabled, the bot only responds to messages from `_chatId` **OR** IDs listed in `whiteList`.
- **`bot.whiteList = "11111111,22222222,33333333";`** — comma-separated allowed chat IDs.

Helper methods:
```cpp
// Check if chat ID is allowed
bool ok = bot.isWhitelisted(msg.chatId);                          // uses bot.whiteList
bool ok = TelegramAPI::isWhitelisted(msg.chatId, "11111,22222");  // uses explicit string

// Extract first ID
int64_t firstId = bot.extractFirstNumber();                       // from bot.whiteList
int64_t firstId = TelegramAPI::extractFirstNumber("11111,22222");

// Command matching (handles commands with or without '/')
if (bot.is_command(msg.text, "help")) { ... } // matches "help" and "/help"
```

---

### Keyboard Menu [MENU]

Menus are embedded directly into message strings using the `[MENU]` tag:
- `\t` — separates buttons in the same row;
- `\n` — moves to the next row of buttons.

```cpp
// Sends a message and generates a 2x2 Reply Keyboard
bot.sendMessage("Choose action:[MENU]Button 1\tButton 2\nButton 3\tButton 4");

// Removes the keyboard from the user's view
bot.sendMessage("Menu closed[MENU]");
```

---

### Automatic HTML Mode

You do not need to configure `parse_mode` manually. If any message sent to `sendMessage` or `editMessage` contains a closing HTML tag `</`, the library automatically attaches `"parse_mode": "HTML"`.

Supported Telegram HTML tags:
- `<b>Bold</b>`
- `<i>Italic</i>`
- `<code>Monospace code</code>`
- `<a href="https://github.com">Link</a>`

---

## Detailed Architecture

For an in-depth breakdown of the internal design, network abstraction layers, C++ template rationale, and resource allocation, see [ARCHITECTURE.md](ARCHITECTURE.md) (in English) or [ARCHITECTURE_RU.md](ARCHITECTURE_RU.md) (на русском).

---

## Changelog

### v1.2.0
- **Interactive Window (`setInteractiveWindow`)**: adaptive accelerated polling (e.g. 1–2s) during conversations and after transmission, maintaining minimal idle traffic (e.g. 60s).
- **Message Editing**: added `editMessage` for custom chats and default chat.
- **Auto HTML**: automatic `"parse_mode": "HTML"` detection upon encountering `</`.
- **Forum Topics**: added `threadId` support to `TResult` and `sendMessage`.
- **Bulk Deletion**: `deleteMessages` overload for `std::vector<int64_t>`.
- **Whitelist**: public `whiteList` property, parameterless `isWhitelisted(chatId)` and `extractFirstNumber()`, 64-bit `atoll()` parsing, `strict` authorization matching `_chatId || whiteList`.
- **Memory Optimizations**: Flash macros `F()` and `PSTR()`, single-character search `indexOf(',', pos)`.

### v1.1.1
- Added `is_command` method for flexible command detection.
- Added `setNext` method for one-time poll delays.

### v1.1.0
- Added `strict` property and whitelist helpers `isWhitelisted`, `extractFirstNumber`.
- Added broadcast method `sendMessageToAll`.

---

## License

This library is licensed under **LGPL-3.0**. You may freely use it in open-source and commercial microcontroller projects subject to the terms of the license.
