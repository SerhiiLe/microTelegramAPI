#define DEBUG

#include <Arduino.h>
#include <TelegramESP.h>


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


// Создание объекта
// Creating an object
TelegramESP bot;

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
  if (msg.text == "menu") {
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

  // подключение к WiFi
  // connecting to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected");

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
