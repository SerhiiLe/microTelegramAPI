# microTelegramAPI

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-blue.svg)](https://www.arduino.cc/)
[![License: LGPL 3.0](https://img.shields.io/badge/License-LGPL_3.0-green.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.2.0-blue.svg)](library.properties)

**microTelegramAPI** is a compact, lightweight, and 100% synchronous Telegram Bot API library for Arduino, ESP8266, ESP32, and STM32 microcontrollers.

The distinctive feature of this library is **complete network transport independence**. It is engineered to operate reliably not only over Wi-Fi, but also over cellular GSM/GPRS/LTE modems (SIM800L, SIM7000, SIM7600, etc.), wired Ethernet, or custom data links using SSLClient encryption.

[English version](README_EN.md) | **Русская версия**

**microTelegramAPI** — компактная, легковесная и 100% синхронная библиотека Telegram Bot API для микроконтроллеров Arduino, ESP8266, ESP32 и STM32. 

Главное отличие библиотеки — **полная независимость от типа сетевого транспорта**. Она спроектирована для надежной работы не только через Wi-Fi, но и через сотовые GSM/GPRS/LTE-модемы (SIM800L, SIM7000, SIM7600 и др.), проводной Ethernet или радиомодемы с использованием слоя шифрования SSLClient.

> 💡 **Ищете асинхронную версию с FreeRTOS для ESP32/ESP8266?**  
> Если для вашего проекта на ESP32/ESP8266 требуется неблокирующий опрос, постоянные keep-alive HTTPS-соединения, фоновые очереди FreeRTOS и доставка сообщений в фоне — используйте нашу библиотеку [asyncTelegramAPI](https://github.com/SerhiiLe/asyncTelegramAPI/).  
> **microTelegramAPI** создана для максимальной простоты, минимального потребления памяти и работы с любым нестандартным транспортом.

---

## Содержание

1. [Ключевые возможности](#ключевые-возможности)
2. [Архитектура транспорта](#архитектура-транспорта)
3. [Установка](#установка)
4. [Быстрый старт](#быстрый-старт)
   - [Для ESP8266 и ESP32 (TelegramESP)](#1-для-esp8266-и-esp32-telegramesp)
   - [Для GSM/SIM800L через SSLClient](#2-для-gsmsim800l-через-sslclient)
5. [Интерактивное окно (Adaptive Polling)](#интерактивное-окно-adaptive-polling)
6. [Справочник API](#справочник-api)
   - [Инициализация и конфигурация](#инициализация-и-конфигурация)
   - [Отправка сообщений](#отправка-сообщений)
   - [Редактирование сообщений](#редактирование-сообщений)
   - [Удаление сообщений](#удаление-сообщений)
   - [Приём сообщений и Callback](#приём-сообщений-и-callback)
   - [Белый список и безопасность](#белый-список-и-безопасность)
   - [Клавиатурное меню [MENU]](#клавиатурное-меню-menu)
   - [Автоматический режим HTML](#автоматический-режим-html)
7. [Подробная архитектура](#подробная-архитектура)
8. [История изменений](#история-изменений)
9. [Лицензия](#лицензия)

---

## Ключевые возможности

- 🚀 **Универсальность транспорта**: работает с Wi-Fi, TinyGSM (SIM800L, LTE), EthernetClient через `SSLClient`.
- 🪶 **Минимум памяти**: отсутствие виртуальных таблиц в шаблоне, строки и ключи во Flash (`F()` и `PSTR()`), экономия до 8–10 КБ RAM по сравнению с монструозными библиотеками.
- ⚡ **Адаптивное интерактивное окно (`setInteractiveWindow`)**: в режиме покоя опрашивает сервер редко (например, раз в 60 сек, экономя сотовый трафик GSM и батарею), а во время диалога временно ускоряется до 1–2 сек.
- 💬 **Топики супергрупп (Forum Topics)**: поддержка параметра `threadId` и автоматическая маршрутизация ответов в тот же тред.
- ✏️ **Редактирование сообщений (`editMessage`)**: обновление ранее отправленных текстов на лету.
- 🗑️ **Групповое удаление (`deleteMessages`)**: удаление сообщений пачками через списки инициализации `{id1, id2}` или `std::vector<int64_t>`.
- 🛡️ **Контроль доступа**: гибкий белый список (`whiteList`), поддержка 64-битных ID через `atoll()` и строгий режим `strict`.
- ⌨️ **Встроенные меню**: удобная генерация Reply-клавиатур с помощью простого тега `[MENU]`.
- 🌐 **Авто-HTML**: обнаружение тегов вида `</...>` автоматически активирует режим `"parse_mode": "HTML"`.

---

## Архитектура транспорта

Библиотека разделяет сетевой стек на три независимых уровня:

```
┌────────────────────────────────────────────────────────┐
│               Ваш код / Callback-логика                │
└───────────────────────────▲────────────────────────────┘
                            │
┌───────────────────────────▼────────────────────────────┐
│          TelegramAPI<SSLTransport> (Протокол)          │
│   Формирование JSON, разбор getUpdates, [MENU], HTML   │
└───────────────────────────▲────────────────────────────┘
                            │ print() / read()
┌───────────────────────────▼────────────────────────────┐
│            Уровень SSL/TLS (Шифрование)                │
│    WiFiClientSecure (ESP)  /  SSLClient (GSM/Ethernet) │
└───────────────────────────▲────────────────────────────┘
                            │
┌───────────────────────────▼────────────────────────────┐
│            Физический транспорт (Канал связи)          │
│        WiFiClient / TinyGsmClient / EthernetClient     │
└────────────────────────────────────────────────────────┘
```

---

## Установка

### PlatformIO

Добавьте библиотеку и зависимость `ArduinoJson` в ваш `platformio.ini`:

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

1. Установите библиотеку **ArduinoJson** (версии 7.x) через менеджер библиотек.
2. Скачайте архив репозитория [microTelegramAPI](https://github.com/SerhiiLe/microTelegramAPI) и распакуйте в папку `libraries/`.

---

## Быстрый старт

### 1. Для ESP8266 и ESP32 (TelegramESP)

Для контроллеров ESP в библиотеку включена готовая удобная обёртка `TelegramESP`, которая сама настраивает встроенный защищенный клиент (`WiFiClientSecure` / BearSSL):

```cpp
#include <Arduino.h>
#include <TelegramESP.h>

TelegramESP bot;

String callback(TResult &msg) {
    if (msg.text == "/start") {
        return "Привет, " + msg.from + "! Бот на связи.[MENU]Кнопка 1\tКнопка 2\nПомощь";
    }
    if (msg.text == "Кнопка 1") {
        return "Вы нажали первую кнопку!";
    }
    return "Эхо: " + msg.text;
}

void setup() {
    Serial.begin(115200);
    WiFi.begin("SSID", "PASSWORD");
    while (WiFi.status() != WL_CONNECTED) delay(500);

    bot.setBotToken("XXXXXXXXXX:YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY");
    bot.setChatID(123456789);        // Чат по умолчанию
    bot.setInterval(10);             // Интервал опроса в покое (10 сек)
    bot.setInteractiveWindow(60, 2); // При активности опрашивать каждые 2 сек в течение 60 сек
    bot.attachCallback(callback);

    bot.sendMessage("Бот запущен!");
}

void loop() {
    bot.checkMessage();
}
```

### 2. Для GSM/SIM800L через SSLClient

Для работы через сотовые модемы свяжите `TinyGSM` и `SSLClient`:

```cpp
#include <Arduino.h>
#include <TelegramAPI.h>

#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#include <SSLClient.h>

// Порт модема и пины UART
HardwareSerial gsmSerial(1);
TinyGsm modem(gsmSerial);
TinyGsmClient gsmTransport(modem);
SSLClient secureClient(&gsmTransport);

// Создание бота поверх SSLClient
TelegramAPI<SSLClient> bot(secureClient);

String callback(TResult &msg) {
    return "GSM Ответ: " + msg.text;
}

void setup() {
    Serial.begin(115200);
    gsmSerial.begin(9600, SERIAL_8N1, 16, 17);

    modem.restart();
    modem.gprsConnect("internet", "", "");

    bot.setBotToken("TOKEN");
    bot.setChatID(123456789);
    bot.setInterval(60);             // Экономим трафик: опрос раз в 60 сек
    bot.setInteractiveWindow(60, 2); // При диалоге ускоряемся до 2 сек
    bot.attachCallback(callback);
}

void loop() {
    bot.checkMessage();
}
```

---

## Интерактивное окно (Adaptive Polling)

На сотовых тарифах (SIM800L, LTE) частый опрос Telegram (раз в 1–2 секунды) сжигает трафик и сажает батарею. С другой стороны, опрос раз в 60 секунд делает диалог невыносимо медленным.

Решение в `microTelegramAPI` — **адаптивное интерактивное окно**:

```cpp
bot.setInterval(60);             // Базовый интервал в покое: 60 сек
bot.setInteractiveWindow(60, 2); // Окно активности: 60 сек с опросом каждые 2 сек
```

- **Как это работает**:
  - Пока никто не пишет, бот делает один короткий запрос раз в минуту.
  - Как только пользователь присылает сообщение или бот отправляет уведомление, окно активности продлевается на 60 секунд, а интервал опроса переключается на 2 секунды.
  - Если в течение 60 секунд новых сообщений нет, бот плавно возвращается к интервалу покоя в 60 секунд.

---

## Справочник API

### Инициализация и конфигурация

| Метод | Описание |
|---|---|
| `TelegramAPI(SSLTransport& client)` | Конструктор бота с привязкой к объекту SSL/TLS клиента. |
| `void setBotToken(const char* / String)` | Установка токена бота от @BotFather. |
| `void setChatID(int64_t chatId)` | Установка chat_id по умолчанию (для отчетов и дефолтных отправок). |
| `void setInterval(int interval)` | Период регулярного опроса в секундах (сбрасывает текущий таймер). |
| `void setNext(uint32_t next = 0)` | Разовая задержка до следующего опроса в секундах. |
| `void setInteractiveWindow(uint32_t window, uint32_t fastInterval = 1)` | Настройка окна ускоренного опроса при активности. |
| `void triggerInteractive()` | Ручной запуск или продление ускоренного интерактивного окна. |
| `void attachCallback(String (*handler)(TResult& tr))` | Регистрация функции обработки входящих сообщений. |
| `void detachCallback()` | Отключение обработчика сообщений. |
| `void attachCheckConnection(bool (*handler)())` | Регистрация внешней функции проверки статуса сети. |
| `void detachCheckConnection()` | Отключение проверки сети. |
| `void skipUpdates()` | Пропуск всех накопившихся непрочитанных сообщений. |

---

### Отправка сообщений

```cpp
// В указанный чат (с опциональным threadId топика)
bool sendMessage(int64_t chatId, const char* message, int64_t threadId = 0);
bool sendMessage(int64_t chatId, const String& message, int64_t threadId = 0);

// В чат по умолчанию (setChatID)
bool sendMessage(const char* message, int64_t threadId = 0);
bool sendMessage(const String& message, int64_t threadId = 0);

// Рассылка по списку номеров через запятую: "11111,22222,33333"
bool sendMessageToAll(const String& list, const char* message, int64_t threadId = 0);
bool sendMessageToAll(const String& list, const String& message, int64_t threadId = 0);
bool sendMessageToAll(const char* message, int64_t threadId = 0); // по списку bot.whiteList
bool sendMessageToAll(const String& message, int64_t threadId = 0);
```

---

### Редактирование сообщений

```cpp
// Редактирование в указанном чате
bool editMessage(int64_t chatId, int64_t messageId, const char* message);
bool editMessage(int64_t chatId, int64_t messageId, const String& message);

// Редактирование в чате по умолчанию (setChatID)
bool editMessage(int64_t messageId, const char* message);
bool editMessage(int64_t messageId, const String& message);

// Получение ID последнего отправленного сообщения
int64_t id = bot.getLastMessageId();
```

---

### Удаление сообщений

```cpp
// Удаление одного сообщения
bool deleteMessage(int64_t chatId, int64_t messageId);
bool deleteMessage(int64_t messageId); // в чате по умолчанию

// Удаление группы сообщений списком инициализации
bool deleteMessages(int64_t chatId, std::initializer_list<int64_t> messageIDs);
bool deleteMessages(std::initializer_list<int64_t> messageIDs);

// Удаление группы сообщений вектором
bool deleteMessages(int64_t chatId, const std::vector<int64_t>& messageIDs);
bool deleteMessages(const std::vector<int64_t>& messageIDs);
```

---

### Приём сообщений и Callback

Метод `checkMessage(bool force = false)` вызывается в основном цикле `loop()`. Возвращает количество обработанных сообщений (0, если время не пришло, или `< 0` при сетевой ошибке).

Структура входящего сообщения:

```cpp
struct TResult {
    int64_t chatId;        // ID чата отправителя
    int64_t messageId;     // ID входящего сообщения
    int64_t threadId = 0;  // ID топика супергруппы (0, если обычный чат)
    String text;           // Текст сообщения
    String from;           // Имя отправителя (first_name)
};
```

Сигнатура обработчика:
```cpp
String callback(TResult &msg) {
    // Возвращаемый текст автоматически отсылается в тот же чат и тред.
    // Если отвечать не нужно, верните пустую строку "".
    if (msg.text == "ping") return "pong";
    return "";
}
```

---

### Белый список и безопасность

- **`bot.strict = true;`** — режим ограничения доступа. Если включен, бот отвечает только на сообщения из чата `_chatId` **ИЛИ** из списка `whiteList`.
- **`bot.whiteList = "11111111,22222222,33333333";`** — строка разрешенных номеров чатов.

Вспомогательные методы:
```cpp
// Проверка наличия чата в списке
bool ok = bot.isWhitelisted(msg.chatId);                          // по bot.whiteList
bool ok = TelegramAPI::isWhitelisted(msg.chatId, "11111,22222");  // по переданной строке

// Извлечение первого ID из списка
int64_t firstId = bot.extractFirstNumber();                       // из bot.whiteList
int64_t firstId = TelegramAPI::extractFirstNumber("11111,22222");

// Проверка команды (поддерживает варианты с '/' и без)
if (bot.is_command(msg.text, "help")) { ... } // сработает для "help" и "/help"
```

---

### Клавиатурное меню [MENU]

Клавиатуры добавляются прямо в текст сообщения с помощью тега `[MENU]`:
- `\t` — разделитель кнопок в одной строке;
- `\n` — перевод на новую строку кнопок.

```cpp
// Отправит сообщение и создаст меню из 2 строк по 2 кнопки
bot.sendMessage("Выберите действие:[MENU]Кнопка 1\tКнопка 2\nКнопка 3\tКнопка 4");

// Удаление клавиатуры у пользователя
bot.sendMessage("Меню скрыто[MENU]");
```

---

### Автоматический режим HTML

Вам не нужно вручную выставлять `parse_mode`. Если в тексте сообщения для `sendMessage` или `editMessage` встречается закрывающий тег `</`, библиотека автоматически отправляет флаг `"parse_mode": "HTML"`.

Поддерживаемые теги Telegram:
- `<b>Жирный текст</b>`
- `<i>Курсив</i>`
- `<code>Моноширинный код</code>`
- `<a href="https://github.com">Ссылка</a>`

---

## Подробная архитектура

Подробное описание внутреннего устройства, сетевого стека, шаблонизации и управления памятью доступно в документе [ARCHITECTURE_RU.md](ARCHITECTURE_RU.md) (на русском) и [ARCHITECTURE.md](ARCHITECTURE.md) (in English).

---

## История изменений

### v1.2.0
- **Интерактивное окно (`setInteractiveWindow`)**: адаптивное ускорение опроса (до 1–2 сек) во время диалога или после отправки сообщения при сохранении экономного интервала в покое (до 60 сек).
- **Редактирование сообщений**: методы `editMessage` для одиночных чатов и чата по умолчанию.
- **Получение ID последнего сообщения**: метод `getLastMessageId()` для сохранения и последующего редактирования отправленных сообщений.
- **Авто-HTML**: автоматическая установка `"parse_mode": "HTML"` при наличии тега `</`.
- **Топики супергрупп**: поддержка `threadId` в структуре `TResult` и параметрах `sendMessage`.
- **Групповое удаление**: перегрузки `deleteMessages` для `std::vector<int64_t>`.
- **Белый список**: публичное свойство `whiteList`, методы `isWhitelisted(chatId)` и `extractFirstNumber()`, парсинг 64-битных ID через `atoll()`, поддержка `strict` для связки `_chatId || whiteList`.
- **Оптимизация памяти**: строковые ключи во Flash (`F()` и `PSTR()`), быстрый поиск символов `indexOf(',', pos)`.

### v1.1.1
- Добавлен метод `is_command` для разбора команд с префиксом `/` и без него.
- Добавлен метод `setNext` для разовой установки задержки перед следующим опросом.

### v1.1.0
- Добавлено свойство `strict` и методы работы с белыми списками `isWhitelisted`, `extractFirstNumber`.
- Добавлен метод массовой рассылки `sendMessageToAll`.

---

## Лицензия

Библиотека распространяется под лицензией **LGPL-3.0**. Вы можете свободно использовать её в личных и коммерческих проектах при сохранении условий лицензии.
