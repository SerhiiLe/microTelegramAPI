#ifndef TELEGRAM_API_HPP
#define TELEGRAM_API_HPP

#include <Arduino.h>
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

#define API_HOST "api.telegram.org"
#define API_PORT 443

#ifdef LOG_TELEGRAM_API
#undef LOG_TELEGRAM_API
#endif

#ifdef DEBUG
	#define LOG_TELEGRAM_API(func, ...) Serial.func(__VA_ARGS__)
#else
	#define LOG_TELEGRAM_API(func, ...) ;
#endif

#ifdef ESP32
	#define TELEGRAM_DELAY vTaskDelay
#else
	#define TELEGRAM_DELAY delay
#endif

struct TResult {
	int64_t chatId;
	int64_t messageId;
	String text;
	String from;
};

template <typename SSLTransport>
class TelegramAPI {

	public:

	// Конструктор
	TelegramAPI(SSLTransport& sslClient) : client(sslClient), apiHost(API_HOST)
	{}

	// установить токен, без него работать не будет	(String)
	void setBotToken(const String& token) {
		_botToken = token;
	}
	// установить токен, без него работать не будет
	void setBotToken(const char* token) {
		_botToken = String(token);
	}

	// установить ID чата по умолчанию. Именно в него будут отправляться уведомления. Отвечать бот будет тот чат из которого пришел запрос
	void setChatID(int64_t chatID) {
		_chatId = chatID;
	}

	// период опроса в секундах. Каждая установка сбрасывает время отсчёта.
	void setInterval(int interval) {
		if ( interval > 1 ) updatePeriod = interval * 1000UL; // чаще, чем раз в секунду
		resetTimer();
	}
	
	// подключение обработчика сообщений
    void attachCallback(String (*handler)(TResult& tr)) {
        _callback = handler;
    }

    // отключение обработчика сообщений
    void detachCallback() {
        _callback = nullptr;
    }

	// подключение функции проверки состояния соединения
	void attachCheckConnection(bool (*handler)()) {
		_checkConnection = handler;
	}

	// отключение функции проверки состояния соединения
	void detachCheckConnection() {
		_checkConnection = nullptr;
	}

	// пропустить непрочитанные сообщения
	void skipUpdates() {
		lastUpdateId = -1;
	}

	// Отправка сообщения в указанный напрямую чат
	bool sendMessage(int64_t chatId, const char* message) {
		if( !chatId ) return false; // не указан ID чата, куда надо отослать сообщение
		if ( _checkConnection && ! _checkConnection() ) return false; // канал связи недоступен

		JsonDocument doc;
		doc[F("chat_id")] = chatId;

		const char *menu = "[MENU]";
		char* token = strstr(message, menu);
		if (token) { // похоже это строка с меню
			char text[strlen(message)+1]; // выделили память под капию строки
			strcpy(text, message); // скопировали, теперь можно уточнять запрос и разбирать на токены
			char* source = text;

			token = strstr(source, menu); // ещё раз ищем, но уже в копии
			*token = 0; // отсечь строку от меню
			doc[F("text")] = source; // и вставить в запрос как текст

			JsonObject reply_markup = doc[F("reply_markup")].to<JsonObject>();
			source = token + 6; // пропустить [MENU]
			if (*source) { // добавить то, что есть как меню
				// добавление базовых параметров
				reply_markup[F("resize_keyboard")] = true;
				reply_markup[F("persistent")] = true;
				JsonArray reply_markup_keyboard = reply_markup[F("keyboard")].to<JsonArray>();
				// подготовка к парсингу
				const char *delim = "\n\t";
				uint8_t row = 0, col = 0;
				bool new_row = false;
				token = strpbrk(source, delim);
				while (token) {
					if (*token == '\n') new_row = true;
					*token = 0;
					reply_markup_keyboard[row][col][F("text")] = source;
					source = token + 1;
					if (new_row) {
						col = 0; row++; new_row = false;
					} else col++;
					token = strpbrk(source, delim);
				}
				// последний параметр не ловится, это остаток строки
				reply_markup_keyboard[row][col][F("text")] = source;
			} else { // пустое меню, убрать его
				reply_markup[F("remove_keyboard")] = true;
			}
		} else { // просто вставить строку как есть
			doc[F("text")] = message;
		}

		String jsonPayload;
		serializeJson(doc, jsonPayload);
		LOG_TELEGRAM_API(println, jsonPayload);

		ApiResult r = apiRequest(F("sendMessage"), jsonPayload);
		return r.result; // при отправке в общем то и проверять нечего... Может потом придумаю.
	}
	// Отправка сообщения в указанный напрямую чат (String)
	bool sendMessage(int64_t chatId, const String message) {
		return sendMessage(chatId, message.c_str());
	}
	// Отправка сообщения в указанный setChatID чат
	bool sendMessage(const char* message) {
		return _chatId > 0 ? sendMessage(_chatId, message): false;
	}
	// Отправка сообщения в указанный setChatID чат (String)
	bool sendMessage(const String message) {
		return _chatId > 0 ? sendMessage(_chatId, message.c_str()): false;
	}

	// Удаление сообщений с указанными ID из указанного чата. Список ID в формате: {20,21,22}
	bool deleteMessages(int64_t chatId, std::initializer_list<int64_t> messageIDs) {
		if (!chatId) return false;
		
		JsonDocument doc;
		doc[F("chat_id")] = chatId;
		JsonArray message_ids = doc[F("message_ids")].to<JsonArray>();
		for (auto id : messageIDs) {
			message_ids.add(id); // Извлечение следующего аргумента
		}

		String jsonPayload;
		serializeJson(doc, jsonPayload);
		LOG_TELEGRAM_API(println, jsonPayload);

		ApiResult r = apiRequest(F("deleteMessages"), jsonPayload);
		return r.result;
	}
	// Удаление сообщений с указанными ID из указанного чата по умолчанию. Список ID в формате: {20,21,22}
	bool deleteMessages(std::initializer_list<int64_t> messageIDs) {
		return _chatId > 0 ? deleteMessages(_chatId, messageIDs): false;
	}

	// Удаление одного сообщения по его ID из чата с chatId
	bool deleteMessage(int64_t chatId, int64_t messageId) {
		if (!chatId) return false;
		JsonDocument doc;
		doc[F("chat_id")] = chatId;
		doc[F("message_id")] = messageId;

		String jsonPayload;
		serializeJson(doc, jsonPayload);
		LOG_TELEGRAM_API(println, jsonPayload);

		ApiResult r = apiRequest(F("deleteMessage"), jsonPayload);
		return r.result;
	}
	// Удаление одного сообщения по его ID из чата с chat_id по умолчанию
	bool deleteMessage(int64_t messageId) {
		return _chatId > 0 ? deleteMessage(_chatId, messageId): false;
	}

	// Проверка новых сообщеий. К этотму моменту должена быть установлена функция для обработки сообщений attach
	// возвращает число полученных сообщений, или <0 если ошибка
	int checkMessage(bool force=false) {
		int result = 0;

		if ( !force && !isReady() ) return 0; // время запроса ещё не пришло
		if ( !_callback ) return -2; // если callback функция не установлена, то сразу выйти, проверка не имеет смысла
		if ( _checkConnection && ! _checkConnection() ) return -3; // канал связи недоступен

		JsonDocument doc;
		// doc["timeout"] = requestTimeout;
		doc[F("limit")] = 5;
		doc[F("offset")] = lastUpdateId + 1;

		String jsonPayload;
		serializeJson(doc, jsonPayload);

		ApiResult ar = apiRequest(F("getUpdates"), jsonPayload);
		if ( ar.result ) {
			JsonArray all_msg = ar.json[F("result")];
			for (JsonObject upd : all_msg) {
				int64_t uid = upd[F("update_id")].as<int64_t>();
				if (uid <= lastUpdateId) continue;
				lastUpdateId = uid;

				if (upd[F("message")].is<JsonObject>()) {
					JsonObject msg = upd[F("message")];
					TResult r;
					r.chatId    = msg[F("chat")][F("id")].as<int64_t>();
					r.messageId = msg[F("message_id")].as<int64_t>();
					r.text      = msg[F("text")]                  | "";
					r.from      = msg[F("from")][F("first_name")] | "";

					LOG_TELEGRAM_API(printf_P, PSTR("[MSG] %s (%lld): %s\n"), r.from.c_str(), r.chatId, r.text.c_str());
					if (_callback) {
						if (strict && _chatId && r.chatId != _chatId) continue; // пришло сообщение не из чата по умолчанию
						TELEGRAM_DELAY(10);
						String toSend = _callback(r);
						if (toSend.length() > 0) {
							TELEGRAM_DELAY(30); // небольшая задержка между приёмом и отправкой сообщения (>30ms), за одно сброс watchdog
							sendMessage(r.chatId, toSend);
						}
					}
					result++;
				}
			}
		}
		else return -1; 
		return result;
	}

	// проверка, есть ли чат в списке разрешенных
	bool isWhitelisted(const int64_t& chatId, const String& whiteList) {
		// простая, но не точная реализация
		return (chatId && whiteList.length() > 0 && whiteList.indexOf(String(chatId)) >= 0);
	}

	// только первый номер из списка чатов
	int64_t extractFirstNumber(String whiteList) {
		whiteList.trim();
		return whiteList.toInt();
	}

	// отослать сообщения всем, кто в списке. Список - строка с числами разделённая запятыми
	bool sendMessageToAll(const String& whiteList, const char* message) {
		int pos1 = 0, pos2 = 0;
		pos2 = whiteList.indexOf(",", pos1);
		while ( pos2 >=0 ) {
			// нашли запятую :)
			String tmp = whiteList.substring(pos1, pos2);
			tmp.trim();
			int64_t id = tmp.toInt();
			if (id)
				if (!sendMessage(id, message)) return false;
			pos1 = pos2+1;
			pos2 = whiteList.indexOf(",", pos1);
			TELEGRAM_DELAY(30); // Звдержка, чтобы не попасть на бан от Telegram за слишком быструю рассылку.
		}
		// Последнее число в списке
		String tmp = whiteList.substring(pos1);
		tmp.trim();
		int64_t id = tmp.toInt();
		if (id)
			if (!sendMessage(id, message)) return false;

		return true;
	}
	// отослать сообщения всем, кто в списке номеров разделённых запятыми
	bool sendMessageToAll(const String& whiteList, const String& message) {
		return sendMessage(whiteList, message.c_str());
	}

	// Принимать сообщения только от чата указанного в chatId
	bool strict = false;

	private:

	struct ApiResult {
		bool result;
		JsonDocument json;
	};

	// запрос к API telegram. Всегда POST, в payload должна быть сформированная строка Json
	// в type должна бытьвызываемую функцию: sendMessage, getUpdates...
	ApiResult apiRequest(const String &type, const String &payload) {
		int statusCode = -1;
		String response;
		JsonDocument responseDoc;

		if (_botToken.length()<38) return {false, responseDoc}; // нет токена - нет запроса

		String url = String(F("/bot")) + _botToken + String(F("/")) + type;
		String request = String(F("POST ")) + url + String(F(" HTTP/1.1\r\n"
						"Host: ")) + apiHost + String(F("\r\n"
						"Content-Type: application/json\r\n"
						"Content-Length: ")) + String(payload.length()) + String(F("\r\n"
						"Connection: close\r\n\r\n")) +
						payload;

		for (int attempt = 1; attempt <= maxRetries; attempt++) {
			if( !checkConnection() ) { // попытка соединится с сервером, если нет, то повтор
				LOG_TELEGRAM_API(printf_P, PSTR("Connection failed, attempt %d/%d\n"), attempt, maxRetries);
				TELEGRAM_DELAY(retryDelay);
				continue;
			}

			client.print(request);

			uint32_t timeout = millis() + 10000L;
			int contentLength = -1;
			
			// ожидание соединения с сервером
			while (client.connected() && !client.available() && (millis() < timeout))
				TELEGRAM_DELAY(10);

			// ответ получен, чтение. Если ответ не получен и был таймаут, то дальше каскадом дойдёт до выхода "Network error"
			while (client.connected() || client.available()) {
				// чтение заголовков, построчно
				response = client.readStringUntil('\n');

				if ( response.startsWith(F("HTTP/1")) ) statusCode = getHttpStatusCode(response); // поиск статуса во всех строках, но сработает только в первой
				if ( response.startsWith(F("Content-Length: ")) ) contentLength = response.substring(16).toInt() + 1; // впереди будет один лишний '\n'
				if ( response.length() < 2 ) break; // пустая строка отделяет заголовок от тела. Cодержит пару символов '\r\n', один из них уже вырезан
			}

			if (contentLength > 0) {
				// Получен нормальный ответ, осталось его прочесть
				response.reserve(contentLength);
				while ((int)response.length() < contentLength) {
					if (client.available()) {
						response += (char)client.read();
					} else if (millis() > timeout) {
						break; // таймаут при чтении тела, лимит на 10 секунд от _начала_ запроса 
					}
				}
			} else {
				// Не получен нормальный ответ с Content-Length, вероятно ошибка сети
				LOG_TELEGRAM_API(println, F("Network error"));
				client.stop();
				continue;
			}

			client.stop();

			LOG_TELEGRAM_API(printf_P, PSTR("status code: %d\nanswer\n"), statusCode);
			LOG_TELEGRAM_API(println, response);

			if (statusCode != 200) { // что-то пошло не так, показ сообщения и заход на вторую попытку
				LOG_TELEGRAM_API(printf_P, PSTR("HTTP error: %d\n"), statusCode);
				if (statusCode == 429) {
					JsonDocument errorDoc;
					deserializeJson(errorDoc, response);
					int retryAfter = errorDoc[F("parameters")][F("retry_after")] | 60;
					LOG_TELEGRAM_API(printf_P, PSTR("Rate limit exceeded, retry after %d seconds\n"), retryAfter);
					TELEGRAM_DELAY(retryAfter * 1000);
				}
				continue;
			}

			DeserializationError error = deserializeJson(responseDoc, response.c_str());
			if (error) {
			    LOG_TELEGRAM_API(printf_P, PSTR("deserializeJson() failed: %s\n"), error.f_str());
				continue;
			}

			if (!responseDoc[F("ok")].as<bool>()) {
				int errorCode = responseDoc[F("error_code")] | 0;
				String description = responseDoc[F("description")] | String(F("Unknown error"));
				LOG_TELEGRAM_API(printf_P, PSTR("Telegram API error %d: %s\n"), errorCode, description.c_str());
				if (errorCode == 401) {
					LOG_TELEGRAM_API(println, PSTR("Invalid bot token, stopping"));
					return {false, responseDoc}; // досрочное прерывание, нет смыла повторять запрос, если токен не принят
				}
				continue;
			}

			return {true, responseDoc};
		}
		// при ошибках запрос повторяется. Если дошло до этого блока, то все попытки завершились неудачей.
		return {false, responseDoc};
	}

	// Проверка соединения
	bool checkConnection() {
		if ( !client.connect(apiHost, apiPort) ) {
			LOG_TELEGRAM_API(println, PSTR("No connection to Telegram server"));
			client.stop();
			return false;
		}
		return true;
	}

	// Чтение HTTP-кода из ответа
	int getHttpStatusCode(const String& response) {
		if (response.startsWith(F("HTTP/1.1 "))) {
			int code = response.substring(9, 12).toInt();
			return code;
		}
		return -1;
	}
	
	// возвращает true, когда пришло время.
	bool isReady() {
		uint32_t time = millis();
		if((int32_t)(time - nextUpdate) > 0) {
			resetTimer();
			return true;
		}
		return false;
	}
	// сброс таймера на установленный интервал
	void resetTimer() {
		lastUpdate = millis();
		nextUpdate = lastUpdate + updatePeriod;
	}

	SSLTransport& client;    // Ссылка на SSLClient

	String _botToken;
	const int maxRetries = 3; // Максимум попыток при ошибке соединения
	const int retryDelay = 1000; // Задержка между попытками (мс)
	const int requestTimeout = 35000; // Таймаут для long polling (35 секунд, учитывая timeout=30)

	int64_t lastUpdateId = 0, _chatId = 0;
	uint32_t lastUpdate = 0, nextUpdate = 0;
	uint32_t updatePeriod = 10000;
	bool _overflow = false;

	const char* apiHost; // = "api.telegram.org";
	const uint16_t apiPort = API_PORT;

	String (*_callback)(TResult& tr) = nullptr;
	bool (*_checkConnection)() = nullptr;

};

#undef LOG_TELEGRAM_API
#undef TELEGRAM_DELAY
#endif