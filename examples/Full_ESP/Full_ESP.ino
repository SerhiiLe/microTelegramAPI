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
  return "Echo: " + msg.text + 
    "\nFrom: " + msg.from + 
    "\nChatID: " + String(msg.chatId) + 
    "\nMessageID: " + String(msg.messageId) +
    "\nIn White List: " + String( bot.isWhitelisted(msg.chatId, "1111111,2222222,3333333") ? "yes": "no" ); 
                                                      // Replace 11111, 222222 and 33333 with real chat IDs
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

  // Accept messages only from the chat specified in chatId. Only if necessary.
  bot.strict = true;

  // Отправка сообщения в чат по умолчани, установенный setChatID
  // Send a message to the default chat set by setChatID
  bot.sendMessage("first message");

  // Отправка сообщения в чат с нужным ID
  // Sending a message to the chat with the required ID
  bot.sendMessage(CHAT_ID, "target message");

  // Рассылка сообщения в несколько чатов. Замените 11111 и 222222 на реальные ID чатов
  // Broadcasting a message to multiple chats. Replace 11111 and 222222 with real chat IDs
  // bot.sendMessageToAll("1111111, 2222222", "test mass message");

}

void loop() {

  // Проверка новых сообщений. Если есть, то быдет вызвана зарегестрированная callback функция. Обязательно должна быть в loop()
  // Check for new messages. If there are any, the registered callback function will be called. Must be in loop()
  bot.checkMessage();

  // Если произошла ошибка при обращении к телеграм, то отложить следующий опрос на 5 минут (5*60=300)
  // If an error occurs when accessing Telegram, delay the next request for 5 minutes (5 * 60 = 300).
  // if (bot.checkMessage() < 1 ) bot.setNext(300);

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
