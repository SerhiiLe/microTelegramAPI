# microTelegramAPI
A simple basic implementation of the Telegram API for Arduino on GSM

Простая базовая реализация Telegram API для Arduino на GSM

## Table of Contents

1. [Description](#description) - An overview of the microTelegramAPI library.
2. [Installation](#installation) - How to install the library using Arduino or PlatformIO.
3. [Usage](#usage) - Basic usage examples for the SSLClient library.
4. [Overview of Functions](#object-methods) - An overview of the API.

## Description

A very simple library for implementing sending and receiving messages via Telegram.
It only has a few functions:
- send messages
- receive messages
- display a menu
- delete messages

These are the minimum functions needed to create a bot for remote control of microcontrollers.

The library isn't tied to a specific transport and can therefore work with both WiFi and a GSM modem, among other options. To work, the following link is required: "lower-layer transport" -> "SSL encapsulation" -> "the library itself."

Fewer functions mean easier to use and takes up less memory.

## Описание
Очень простая библиотека для реализации отправки приёма сообщений через Телеграм.  
Умеет всего несколько функций:
- отправлять сообщения
- принимать сообщения
- отображать меню
- удалять сообщения

Это функции минимально необходимые для создания бота для дистанционного управления микроконтроллерами.

Несмотря на то, что существует множество других библиотек, я не смог найти ту, которая сможет работать с любым транспортом. Конкретно мне нужена была работа с GSM модемом SIM800L. Пришлось делать самому. Библиотека, несмотря на простототу немного сложнее в использовании из-за того, что надо самостоятельно настраивать транспорт.

Если нужен более богатый функционал, то можно подобрать что-то более функциональное. Наверное. Когда я пробовал другие библиотеки, то они хорошо работали через WiFi, но ломались на GSM, хотя по описанию должны были с ним работать. Я не старался объять необъятное, а реализовал только те функции, которые мне были реально нужны, например возможность вызывать функцию проверки соединения.

Основная проблема всех библиотек в том, что они жестко привязаны к какому либо транспорту. Реально под капотом всех библитек используется связка "транспорт нижнего уровня" -> "инкапсуляция SSL" -> "собственно библиотека". Первый уровень привязан к платформе. Обычно это "WiFi.h", но может быть что-то другое, например "TinyGsmClient.h". К нему должен подключаться библиотека, которая понимает нижний уровень, например "WiFiClientSecure.h" или "SSLClient.h". И несмотря на общие названия они могут иметь разных авторов и разную реализацию. И вот по верх всего этого работает билиотека с telegram API.

И как бонус, за счёт очень маленьгого функционала и маленького кода занимает меньше памяти. Так переход с FastBot на плате esp8266 дал экономию в 8k RAM и более стабильную работу. Не скорость.

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

Для ArduinoIDE - в менеджере библитек найти "microTelegramAPI" и установить.
Или скачать архив с библиотекой и развернуть в папку Arduino/librarie . Для работы должна быть установлена библиотека ArduinoJson из менеджера библиотек ArduinoIDE.

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

Если работа будет тоолько с ESP8266/ESP32, то лучше использовать обёртку TelegramESP

```cpp
#include <TelegramESP.h>
TelegramESP bot;
```

Детальнее в примере [Full_ESP](https://github.com/SerhiiLe/microTelegramAPI/tree/main/examples/Full_ESP)

Для работы с нестандартным транспортом надо самомстоятельно инициализировать библиотеку

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
TelegramAPI(SSLClient& sslClient)

// Install a token, it won't work without it.
// установить токен, без него работать не будет	(String)
void setBotToken(const String& token)
// установить токен, без него работать не будет
void setBotToken(const char* token)

// Set the default chat ID. This is only used for sending messages; it replies to everyone.
// установить ID чата по умолчанию. Именно в него будут отправляться уведомления. Отвечать бот будет тот чат из которого пришел запрос
void setChatID(int64_t chatID)

// Polling period in seconds. Each installation resets the countdown timer.
// период опроса в секундах. Каждая установка сбрасывает время отсчёта.
void setInterval(int interval)

// set a delay until the next poll, one-time
// выставить задержку до следующего срабатывания, разово
void setNext(uint32_t next = 0)

// attach a message handler
// подключение обработчика сообщений
void attachCallback(String (*handler)(TResult& tr))
// отключение обработчика сообщений
void detachCallback()

// Attaching a connection status check function
// подключение функции проверки состояния соединения
void attachCheckConnection(bool (*handler)())
// отключение функции проверки состояния соединения
void detachCheckConnection()

// skip unread messages
// пропустить непрочитанные сообщения
void skipUpdates()

// Sending a message to a specified chat
// Отправка сообщения в указанный напрямую чат
bool sendMessage(int64_t chatId, const char* message)
// Отправка сообщения в указанный напрямую чат (String)
bool sendMessage(int64_t chatId, const String message)
// Отправка сообщения в указанный setChatID чат
bool sendMessage(const char* message)
// Отправка сообщения в указанный setChatID чат (String)
bool sendMessage(const String message)

// Delete messages with the specified IDs from the specified chat
// Удаление сообщений с указанными ID из указанного чата. Список ID в формате: {20,21,22}
bool deleteMessages(int64_t chatId, std::initializer_list<int64_t> messageIDs)
// Удаление сообщений с указанными ID из указанного чата по умолчанию. Список ID в формате: {20,21,22}
bool deleteMessages(std::initializer_list<int64_t> messageIDs)
// Удаление одного сообщения по его ID из чата с chatId
bool deleteMessage(int64_t chatId, int64_t messageId)
// Удаление одного сообщения по его ID из чата с chat_id по умолчанию
bool deleteMessage(int64_t messageId)

// Check for new messages. By this point, the message handling function attachCallback must be attached.
// Returns the number of messages received, or -1 if an error occurred.
// Проверка новых сообщеий. К этотму моменту должена быть установлена функция для обработки сообщений attach
// возвращает число полученных сообщений, или -1 если ошибка
int checkMessage(bool force=false)

// Additional, optional methods for working with lists of chat numbers
// Дополнительные, не обязательные методы для работы со списками номеров чатов

// check if the chat is in the allowed list
// Проверка, есть ли чат в списке разрешенных
bool isWhitelisted(const int64_t& chatId, const String& whiteList)

// Only the first number from the chat list
// Только первый номер из списка чатов
int64_t extractFirstNumber(String whiteList)

// Send messages to everyone on the list. List is a string of numbers separated by commas.
// Отослать сообщения всем, кто в списке. Список - строка с числами разделённая запятыми
bool sendMessageToAll(const String& whiteList, const char* message)
// Отослать сообщения всем, кто в списке номеров разделённых запятыми
bool sendMessageToAll(const String& whiteList, const String& message)

// both options, with / at the beginning and without
// поиск параметра не зависимо от наличия "/" в начале
bool is_command(const String& in, const String& command)

// Accept messages only from the chat specified in chatId
// Принимать сообщения только от чата указанного в chatId
bool strict = false
```

There's no separate method for adding a menu (keyboard). Instead, you need to use the [MENU] tag in your bot's message.

Для добавления меню (клавиатура) нет отделного метода. Вместо этого в сообщении от бота надо использовать тег [MENU]

```cpp
// Sends text and creates a menu with two lines and four items.
// \t - new item (button), \n - first item of the new line
// отошлёт тект и создаст меню из двух строк и четырёх пунктов.
// \t - новый пукт (кнопка), \n - первый пункт новой строки
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
	String text;
	String from;
};
```

The checkMessage method, called without parameters, runs according to its own schedule, set by the setInterval method. However, you can create your own schedule and call checkMessage(true), in which case the internal schedule is ignored.

Метод checkMessage, вызванный без параметров, работает по собственному расписанию, установленному методом setInterval. Но можно сделать своё расписание и вызывать checkMessage(true), тогда внутреннее расписание игнорируется.

## Changes

### v1.1.0

The "strict" property controls whether a user can reply to all messages or only those from a designated chat. However, this isn't always convenient, so the "isWhitelisted" and "extractFirstNumber" methods have been added, allowing you to build your own security database. The sendMessageToAll method has also been added for sending messages to multiple chats listed in a regular string. The numbers must be separated by a comma. If the number is one, it will work similarly to sendMessage, but the number will be in a String rather than an int64_t.

Разрешение отвечать на все сообщения или только из назначенного чата регулируется свойством "strict". Но это не всегда удобно, по этому добавлены методы "isWhitelisted" и "extractFirstNumber", с помощью которых можно построить свою могику безопасности. Так-же добавлен метод sendMessageToAll для рассылки в несколько чатов по списку в обычной строке. Номера должны быть разделены запятой. Если номер один, то будет работать аналогично sendMessage, но номер не в int64_t, а в String.

### v1.1.1

Added an optional is_command method that allows you to find commands both with and without a leading /.
Added an optional setNext method for setting a one-time delay before the next server poll.

Добавлен необязательный метод is_command, который позволяет находить команды как с / в начале, так и без.
Добавлен необязательный метод setNext, для разовой установки задержки перед очередеым опросом сервера.

## LICENSE

LGPL-3.0 license