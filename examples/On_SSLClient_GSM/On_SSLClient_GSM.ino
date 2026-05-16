#define DEBUG

#include <Arduino.h>
#include <TelegramAPI.h>
#include <WiFiClient.h>

// GovoroxSSLClient in ArduinoIDE, or https://github.com/govorox/SSLClient , ESP32 only
#include <SSLClient.h>

#include "cert.h"

// Ваши настройки WiFi
// Your WiFi settings
#define WIFI_SSID "My_WiFi"
#define WIFI_PASS "my_password"


// Токен телеграм бота
// Telegram bot token "XXXXXXXXXX:YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY"
#define BOT_TOKEN ""
// Номер чата куда будут отсылаться сообщения, если явно не указан другой номер
// The chat ID where messages will be sent unless another number is explicitly specified.
#define CHAT_ID 111111111

// Ниже блок как пример описания траспорта TinyGSM, аналогично можно подключить другой транспорт, например EthernetClient и ArduinoHttpClient
// Below is a block as an example of describing the TinyGSM transport; similarly, you can connect other transport, for example, EthernetClient and ArduinoHttpClient
#ifdef USE_GSM

#define Serial1 gsmSerial
#define PIN_gsmTX 17 // RX SM800L 4
#define PIN_gsmRX 16 // TX SM800L 5

// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800   // Modem is SIM800
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#include <TinyGsmClient.h>  // должен быть после указания, какой тип модема используется

// Layers stack
TinyGsm modem(gsmSerial);
TinyGsmClient gsmTransportLayer(modem);
SSLClient securePresentationLayer(&gsmTransportLayer);

#else

// Транспорт WiFi
WiFiClient webTransportLayer;
SSLClient securePresentationLayer(&webTransportLayer);

#endif

// Создание объекта
// Creating an object
TelegramAPI<SSLClient> bot(securePresentationLayer);


// В фукции которую вызывает TelegramAPI должна быть реализована логика бота, возможные варианты ответа и реакции
// The function that TelegramAPI calls must implement the bot's logic, possible response options, and reactions.
String callback(TResult &msg) {
  if (msg.text == "hello") {
    // что-то выполнить и вернуть сообщение
    // do something and return a message
    return "Hello, " + msg.from;
  }
  if (msg.text == "del") {
    // Удаление двух последних сообщений из чата
    // Deleting the last two messages from a chat
    bot.deleteMessages({msg.messageId-2, msg.messageId-1});
    // Удаление одного сообщения
    // Deleting a single message
    bot.deleteMessage(msg.messageId);
    return "delete 3 message and add one :)";
  }
  if (bot.is_command(msg.text, "menu")) { // both options, with / at the beginning and without ("menu"||"/menu")
    // Добавление меню из двух строк и четырёх пунктов
    // Adding a two-line, four-item menu
    return "add menu[MENU]hello\tnone\ndel\thide";
  }
  if (msg.text == "hide") {
    // Удаление меню
    // Removing a menu
    return "remove menu[MENU]";
  }
  if (msg.text == "none") {
    // Бот всегда должен отвечать, если нечего отправлять, то надо вернуть пустую строку: ""
    // The bot should always respond; if there is nothing to send, it should return an empty string: ""
    return "";
  }
  return "Echo: " + msg.text + "\nFrom: " + msg.from + "\nChatID: " + String(msg.chatId) + "\nMessageID: " + String(msg.messageId);
}


void setup() {
  Serial.begin(115200);

  #ifdef USE_GSM

  // Set SIM module baud rate and UART pins
	gsmSerial.begin(MODEM_UART_BAUD, SERIAL_8N1, PIN_gsmRX, PIN_gsmTX);

  // For a more detailed description, see the TinyGSM documentation.
  modem.waitForNetwork(500);
  modem.gprsConnect("Internet", "", "");

  #else 

  // подключение к WiFi
  // connecting to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected");

  #endif

  // Можно не устанавливать сертификат, тогда будет приниматься любой сертификат
  // You don't have to install the certificate, then any certificate will be accepted.
  securePresentationLayer.setCACert(TELEGRAM_CA_CERT);

  // Установка начальных значений. Для работы их обязательно надо установить. В любой момент их можно поменять.
  // Setting the initial values. These must be set for the program to work. They can be changed at any time.
  bot.setBotToken(BOT_TOKEN);
  bot.setChatID(CHAT_ID);
  bot.setInterval(10);
  bot.attachCallback(callback);

  // Отправка сообщения в чат по умолчани, установенный setChatID
  // Send a message to the default chat set by setChatID
  bot.sendMessage("first message");

  // Отправка сообщения в чат с нужным ID
  // Sending a message to the chat with the required ID
  bot.sendMessage(CHAT_ID, "target message");
  
}

void loop() {

  // Проверка новых сообщений. Если есть, то быдет вызвана зарегестрированная callback функция. Обязательно должна быть в loop()
  // Check for new messages. If there are any, the registered callback function will be called. Must be in loop()
  bot.checkMessage();

  // Отправлять сообщения каждые 60 секунд
  // Send messages every 60 seconds
  static int count = 0;
  static uint32_t last = millis();
  uint32_t now = millis();
  if (millis() - last > 60 * 1000UL) {
    bot.sendMessage("Number: " + String(count++));
    last = now;
  }
}
