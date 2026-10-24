# microTelegramAPI
A simple basic implementation of the Telegram API for Arduino on GSM

Простая базовая реализация Telegram API для Arduino на GSM

## Table of Contents

1. [Description](#description) - An overview of the microTelegramAPI library.
2. [Installation](#installation) - How to install the library using Arduino or PlatformIO.
3. [Usage](#usage) - Basic usage examples for the SSLClient library.
4. [Overview of Functions](#object-methods) - An overview of the API.

## Description

A very simple, lightweight library for sending and receiving messages via Telegram.
It supports essential bot functionality:
- send messages (with optional forum topic/thread ID and automatic HTML mode)
- edit existing messages (`editMessage`)
- receive messages (with automatic topic routing in callbacks)
- display and hide reply menus (keyboards)
- delete individual or multiple messages (`deleteMessage`, `deleteMessages`)
- strict sender authorization with `strict` and `whiteList`

The library is completely transport-agnostic and works seamlessly with GSM modems (SIM800L, LTE), Ethernet, SSLClient, and Wi-Fi.

> **Need Non-Blocking Asynchronous Operation or FreeRTOS Background Queue?**  
> If you are building a project for ESP8266 or ESP32 and need non-blocking I/O, keep-alive connections, background task workers, and interactive send queues, check out [asyncTelegramAPI](https://github.com/SerhiiLe/asyncTelegramAPI/).  
> `microTelegramAPI` is dedicated to minimal, synchronous, and transport-independent deployments.

## Описание
Очень простая и легковесная библиотека для отправки и приёма сообщений через Телеграм.  
Библиотека предоставляет ключевой набор функций:
- отправка сообщений (с поддержкой топиков/тредов супергрупп и автоопределением HTML)
- редактирование отправленных сообщений (`editMessage`)
- приём сообщений (с автоматической маршрутизацией ответов в тот же тред)
- отображение и скрытие клавиатурного меню
- удаление одиночных и группы сообщений (`deleteMessage`, `deleteMessages`)
- фильтрация доступа по `strict` и белому списку `whiteList`

Библиотека не привязана к конкретному транспорту и работает с GSM модемами (SIM800L, LTE), Ethernet, SSLClient и Wi-Fi.

> **Нужна неблокирующая асинхронность или очередь FreeRTOS в фоне?**  
> Если проект разрабатывается для ESP8266 или ESP32 и вам требуются неблокирующий опрос, постоянные keep-alive соединения, фоновые очереди FreeRTOS и доставка по таймерам — используйте библиотеку [asyncTelegramAPI](https://github.com/SerhiiLe/asyncTelegramAPI/).  
> `microTelegramAPI` остаётся компактным, строго синхронным и универсальным решением для любого транспорта.

Несмотря на то, что существует множество других библиотек, я не смог найти ту, которая сможет работать с любым транспортом. Конкретно мне нужна была работа с GSM модемом SIM800L. Пришлось делать самому. Библиотека, несмотря на простоту немного сложнее в использовании из-за того, что надо самостоятельно настраивать транспорт.

Если нужен более богатый функционал, то можно подобрать что-то более функциональное. Наверное. Когда я пробовал другие библиотеки, то они хорошо работали через WiFi, но ломались на GSM, хотя по описанию должны были с ним работать. Я не старался объять необъятное, а реализовал только те функции, которые мне были реально нужны, например возможность вызывать функцию проверки соединения.

Основная проблема всех библиотек в том, что они жестко привязаны к какому либо транспорту. Реально под капотом всех библиотек используется связка "транспорт нижнего уровня" -> "инкапсуляция SSL" -> "собственно библиотека". Первый уровень привязан к платформе. Обычно это "WiFi.h", но может быть что-то другое, например "TinyGsmClient.h". К нему должен подключаться библиотека, которая понимает нижний уровень, например "WiFiClientSecure.h" или "SSLClient.h". И несмотря на общие названия они могут иметь разных авторов и разную реализацию. И вот по верх всего этого работает библиотека с telegram API.

И как бонус, за счёт очень маленького функционала и маленького кода занимает меньше памяти. Так переход с FastBot на плате esp8266 дал экономию в 8k RAM и более стабильную работу. Не скорость.

## Installation

For ArduinoIDE, find "microTelegramAPI" in the library manager and install it.
Alternatively, download the library archive and unzip it to the Arduino/library folder. The ArduinoJson library must be installed from the ArduinoIDE library manager for this to work.

For PlatformIO, add it to platformio.ini  
```
lib_deps =
    https://github.com/bblanchon/ArduinoJson
    https://github.com/SerhiiLe/microTelegramAPI
```

## Установка

Для ArduinoIDE - в менеджере библиотек найти "microTelegramAPI" и установить.
Или скачать архив с библиотекой и развернуть в папку Arduino/libraries . Для работы должна быть установлена библиотека ArduinoJson из менеджера библиотек ArduinoIDE.

Для PlatformIO надо добавить в platformio.ini   
```
lib_deps =
    https://github.com/bblanchon/ArduinoJson
    https://github.com/SerhiiLe/microTelegramAPI
```

## Usage

If you're only working with the ESP8266/ESP32, it's better to use the TelegramESP wrapper.

```cpp
#include <TelegramESP.h>
TelegramESP bot;
```

More details in the [Full_ESP](https://github.com/SerhiiLe/microTelegramAPI/tree/main/examples/Full_ESP) example

To work with non-standard transport, you must initialize the library yourself.

```cpp
#include <TelegramAPI.h>
// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800 // Modem is SIM800
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#include <TinyGsmClient.h> // must be after specifying the modem type used

// Layers stack
TinyGsm modem(gsmSerial);
TinyGsmClient gsmTransportLayer(modem);
SSLClient securePresentationLayer(&gsmTransportLayer);

TelegramAPI<SSLClient> bot(securePresentationLayer);
```

For more details, see the example [On_SSLClient_GSM](https://github.com/SerhiiLe/microTelegramAPI/tree/main/examples/On_SSLClient_GSM)

The library is not platform-specific, but libraries providing transport and encryption can be. In the examples, this is the ESP32.

## Использование

Если работа будет только с ESP8266/ESP32, то лучше использовать обёртку TelegramESP

```cpp
#include <TelegramESP.h>
TelegramESP bot;
```

Детальнее в примере [Full_ESP](https://github.com/SerhiiLe/microTelegramAPI/tree/main/examples/Full_ESP)

Для работы с нестандартным транспортом надо самостоятельно инициализировать библиотеку

```cpp
#include <TelegramAPI.h>
// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800   // Modem is SIM800
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#include <TinyGsmClient.h>  // должен быть после указания, какой тип модема используется

// Layers stack
TinyGsm modem(gsmSerial);
TinyGsmClient gsmTransportLayer(modem);
SSLClient securePresentationLayer(&gsmTransportLayer);

TelegramAPI<SSLClient> bot(securePresentationLayer);
```

Детальнее в примере [On_SSLClient_GSM](https://github.com/SerhiiLe/microTelegramAPI/tree/main/examples/On_SSLClient_GSM)

Библиотека не привязана к конкретной платформе, но библиотеки обеспечивающие транспорт и шифрования могут быть привязаны. В примерах это ESP32.

### Object methods

```cpp
// Конструктор / Constructor
TelegramAPI(SSLTransport& sslClient)

// Install a token, it won't work without it.
// Установить токен, без него работать не будет (String)
void setBotToken(const String& token)
// Установить токен, без него работать не будет
void setBotToken(const char* token)

// Set the default chat ID. This is used for default sends.
// Установить ID чата по умолчанию. Именно в него будут отправляться уведомления.
void setChatID(int64_t chatID)

// Polling period in seconds. Each call resets the timer.
// Период опроса в секундах. Каждая установка сбрасывает время отсчёта.
void setInterval(int interval)

// Set a one-time delay until the next poll in seconds. Calling without arguments resets the interval.
// Выставить задержку до следующего срабатывания, разово, в секундах. Вызов без аргументов сбрасывает отсчёт интервала.
void setNext(uint32_t next = 0)

// Configure interactive window: temporarily accelerate polling interval during activity
// Настройка интерактивного окна: временное ускорение опроса при активности
// window - duration of the active window in seconds (0 = disabled)
// fastInterval - polling interval in seconds during active window (default: 1)
void setInteractiveWindow(uint32_t window, uint32_t fastInterval = 1)

// Manually trigger or extend interactive accelerated polling window
// Активация или продление окна интерактивного (ускоренного) опроса вручную
void triggerInteractive()

// Attach an incoming message handler callback
// Подключение обработчика сообщений
void attachCallback(String (*handler)(TResult& tr))
// Отключение обработчика сообщений
void detachCallback()

// Attach connection health check function
// Подключение функции проверки состояния соединения
void attachCheckConnection(bool (*handler)())
// Отключение функции проверки состояния соединения
void detachCheckConnection()

// Skip unread messages
// Пропустить непрочитанные сообщения
void skipUpdates()

// Sending a message to a specified chat (with optional forum threadId)
// Отправка сообщения в указанный напрямую чат (с опциональным threadId)
bool sendMessage(int64_t chatId, const char* message, int64_t threadId = 0)
bool sendMessage(int64_t chatId, const String& message, int64_t threadId = 0)
// Sending a message to the default chat set by setChatID (with optional forum threadId)
// Отправка сообщения в указанный setChatID чат (с опциональным threadId)
bool sendMessage(const char* message, int64_t threadId = 0)
bool sendMessage(const String& message, int64_t threadId = 0)

// Editing previously sent message text (HTML tags are auto-detected)
// Редактирование текста ранее отправленного сообщения (HTML теги определяются автоматически)
bool editMessage(int64_t chatId, int64_t messageId, const char* message)
bool editMessage(int64_t chatId, int64_t messageId, const String& message)
bool editMessage(int64_t messageId, const char* message)
bool editMessage(int64_t messageId, const String& message)

// Delete messages with the specified IDs from the specified chat
// Удаление сообщений с указанными ID из указанного чата. Список ID в формате: {20,21,22} или std::vector
bool deleteMessages(int64_t chatId, std::initializer_list<int64_t> messageIDs)
bool deleteMessages(std::initializer_list<int64_t> messageIDs)
bool deleteMessages(int64_t chatId, const std::vector<int64_t>& messageIDs)
bool deleteMessages(const std::vector<int64_t>& messageIDs)

// Delete a single message by ID
// Удаление одного сообщения по его ID из чата с chatId
bool deleteMessage(int64_t chatId, int64_t messageId)
// Удаление одного сообщения по его ID из чата по умолчанию
bool deleteMessage(int64_t messageId)

// Check for new messages. By this point, attachCallback must be configured.
// Returns the number of messages received, 0 if not due, or negative on error.
// Проверка новых сообщений. К этому моменту должна быть установлена callback функция.
// Возвращает число полученных сообщений, 0 если время не пришло, или <0 если ошибка.
int checkMessage(bool force = false)

// Methods for whitelist filtering and bulk sending:
// Методы для фильтрации по белому списку и рассылки:

// Check if chatId is in a given list or in bot.whiteList
// Проверка, есть ли чат в переданном списке или в свойстве whiteList
static bool isWhitelisted(const int64_t& chatId, const String& list)
bool isWhitelisted(const int64_t& chatId) const

// Extract the first ID from a given list or from bot.whiteList
// Только первый номер из переданного списка или из свойства whiteList
static int64_t extractFirstNumber(const String& list)
int64_t extractFirstNumber() const

// Broadcast messages to all chat IDs in comma-separated list
// Отослать сообщения всем, кто в списке номеров через запятую
bool sendMessageToAll(const String& list, const char* message, int64_t threadId = 0)
bool sendMessageToAll(const String& list, const String& message, int64_t threadId = 0)
bool sendMessageToAll(const char* message, int64_t threadId = 0)
bool sendMessageToAll(const String& message, int64_t threadId = 0)

// Check if string starts with command (with or without '/')
// Поиск команды независимо от наличия '/' в начале
bool is_command(const String& in, const String& command)

// Restrict incoming messages: if strict == true, only chatId OR whiteList are allowed
// Принимать сообщения только от чата _chatId или номеров из whiteList
bool strict = false;

// Comma-separated list of allowed chat IDs
// Список разрешенных chat_id (через запятую)
String whiteList;
```

There's no separate method for adding a menu (keyboard). Instead, you need to use the [MENU] tag in your bot's message.

Для добавления меню (клавиатура) нет отдельного метода. Вместо этого в сообщении от бота надо использовать тег [MENU]

```cpp
// Sends text and creates a menu with two lines and four items.
// \t - new item (button), \n - first item of the new line
// отошлёт текст и создаст меню из двух строк и четырёх пунктов.
// \t - новый пункт (кнопка), \n - первый пункт новой строки
bot.sendMessage("just some text[MENU]first item \t second item \n second line \t fourth item");

// removes the menu by printing the text
// удалит меню, напечатав текст
bot.sendMessage("just some text[MENU]");
```

The callback function is required to process incoming messages. It must be of the String type, and its return value will be sent as a response to the same chat from which the request originated. The logic is that the bot must respond to any request.

Callback функция нужна для обработки входящий сообщений, она должна быть типа String и то, что она вернёт будет отправлено как ответ в тот-же чат, из которого пришёл запрос. Логика в том, что на любой запрос бот обязан ответить.

The function that will be assigned to the callback must be declared like this (any name):

Функция которая будет назначена для callback должна быть объявлена так (название любое):

```cpp
String telegramCallback(TResult& msg);
```

В функцию передаётся структура TResult:

```cpp
struct TResult {
	int64_t chatId;
	int64_t messageId;
	int64_t threadId = 0; // ID топика супергруппы (0, если обычный чат) / Forum topic ID
	String text;
	String from;
};
```

The checkMessage method, called without parameters, runs according to its own schedule, set by the setInterval method. However, you can create your own schedule and call checkMessage(true), in which case the internal schedule is ignored.

Метод checkMessage, вызванный без параметров, работает по собственному расписанию, установленному методом setInterval. Но можно сделать своё расписание и вызывать checkMessage(true), тогда внутреннее расписание игнорируется.

## Changes

### v1.2.0

- Added interactive window: `setInteractiveWindow(window, fastInterval)` and `triggerInteractive()` to temporarily accelerate polling frequency (e.g. 1-2s) upon activity or message send, keeping idle GSM/LTE traffic minimal (e.g. 60s).
- Added message editing: `editMessage(chatId, messageId, text)` and `editMessage(messageId, text)` with automatic HTML mode detection.
- Automatic HTML formatting: detecting `</` in the message automatically applies `"parse_mode": "HTML"` in `sendMessage` and `editMessage`.
- Telegram forum supergroup topic support: `threadId` field in `TResult`, `threadId` parameter in `sendMessage`, and automatic topic replies in callbacks.
- Added `std::vector<int64_t>` overloads for `deleteMessages`.
- Enhanced Whitelist and Access Control:
  - Public `whiteList` property.
  - Parameterless member overloads `isWhitelisted(chatId)` and `extractFirstNumber()`.
  - 64-bit ID parsing via `atoll()` replacing naive substring matching.
  - In `strict` mode, messages are accepted if they match `_chatId` OR are present in `whiteList`.
- Flash and Memory Optimizations: JSON keys and format strings stored in Flash with `F()` and `PSTR()`, character searching via `indexOf(',', pos)`.

---

- Добавлена поддержка интерактивного окна: `setInteractiveWindow(window, fastInterval)` и `triggerInteractive()` для динамического ускорения опроса (например, до 1-2 сек) во время диалога или после отправки сообщения при сохранении экономного интервала в покое (например, 60 сек).
- Добавлено редактирование сообщений: `editMessage(chatId, messageId, text)` и `editMessage(messageId, text)` с автоопределением HTML.
- Автоматический режим HTML: наличие тега `</` в тексте сообщения автоматически включает `"parse_mode": "HTML"` в `sendMessage` и `editMessage`.
- Поддержка топиков/тредов форумов Telegram: поле `threadId` в структуре `TResult`, параметр `threadId` в `sendMessage`, автоматическая отправка ответов callback в тот же топик.
- Добавлена перегрузка `deleteMessages` для `std::vector<int64_t>`.
- Улучшенный белый список и контроль доступа:
  - Публичное свойство `whiteList`.
  - Перегрузки `isWhitelisted(chatId)` и `extractFirstNumber()` без параметров (использующие `whiteList`).
  - Парсинг 64-битных ID через `atoll()` вместо неточного строкового поиска подстроки.
  - В режиме `strict` бот отвечает, если сообщение пришло из чата по умолчанию `_chatId` ИЛИ из списка `whiteList`.
- Оптимизация памяти: строковые константы и ключи JSON вынесены во Flash с помощью `F()` и `PSTR()`, поиск разделителей через `indexOf(',', pos)`.

### v1.1.1

Added an optional is_command method that allows you to find commands both with and without a leading /.
Added an optional setNext method for setting a one-time delay before the next server poll.

Добавлен необязательный метод is_command, который позволяет находить команды как с / в начале, так и без.
Добавлен необязательный метод setNext, для разовой установки задержки перед очередным опросом сервера.

### v1.1.0

The "strict" property controls whether a user can reply to all messages or only those from a designated chat. However, this isn't always convenient, so the "isWhitelisted" and "extractFirstNumber" methods have been added, allowing you to build your own security database. The sendMessageToAll method has also been added for sending messages to multiple chats listed in a regular string. The numbers must be separated by a comma. If the number is one, it will work similarly to sendMessage, but the number will be in a String rather than an int64_t.

Разрешение отвечать на все сообщения или только из назначенного чата регулируется свойством "strict". Но это не всегда удобно, по этому добавлены методы "isWhitelisted" и "extractFirstNumber", с помощью которых можно построить свою логику безопасности. Так-же добавлен метод sendMessageToAll для рассылки в несколько чатов по списку в обычной строке. Номера должны быть разделены запятой. Если номер один, то будет работать аналогично sendMessage, но номер не в int64_t, а в String.

## LICENSE

LGPL-3.0 license