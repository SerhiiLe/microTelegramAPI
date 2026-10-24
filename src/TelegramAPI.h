#ifndef TELEGRAM_API_HPP
#define TELEGRAM_API_HPP

#include <Arduino.h>
#include <initializer_list>
#include <vector>
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
	int64_t threadId = 0; // ID топика супергруппы (0, если обычный чат или общий топик)
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

	// выставить задержку до следующего срабатывания, разово, в секундах
	void setNext(uint32_t next = 0) {
		nextUpdate = millis() + (next ? next * 1000UL: updatePeriod);
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

	// Отправка сообщения в указанный напрямую чат (с опциональным threadId топика)
	bool sendMessage(int64_t chatId, const char* message, int64_t threadId = 0) {
		if( !chatId ) return false; // не указан ID чата, куда надо отослать сообщение
		if ( _checkConnection && ! _checkConnection() ) return false; // канал связи недоступен

		JsonDocument doc;
		doc[F("chat_id")] = chatId;
		if (threadId > 0) {
			doc[F("message_thread_id")] = threadId;
		}

		const char *menu = "[MENU]";
		char* token = strstr(message, menu);
		if (token) { // похоже это строка с меню
			char text[strlen(message)+1]; // выделили память под копию строки
			strcpy(text, message); // скопировали, теперь можно уточнять запрос и разбирать на токены
			char* source = text;

			token = strstr(source, menu); // ещё раз ищем, но уже в копии
			*token = 0; // отсечь строку от меню
			doc[F("text")] = source; // и вставить в запрос как текст
			if (strstr_P(source, PSTR("</"))) doc[F("parse_mode")] = F("HTML");

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
			if (strstr_P(message, PSTR("</"))) doc[F("parse_mode")] = F("HTML");
		}

		String jsonPayload;
		serializeJson(doc, jsonPayload);
		LOG_TELEGRAM_API(println, jsonPayload);

		ApiResult r = apiRequest(F("sendMessage"), jsonPayload);
		return r.result; // при отправке в общем то и проверять нечего... Может потом придумаю.
	}
	// Отправка сообщения в указанный напрямую чат (String, с опциональным threadId)
	bool sendMessage(int64_t chatId, const String& message, int64_t threadId = 0) {
		return sendMessage(chatId, message.c_str(), threadId);
	}
	// Отправка сообщения в указанный setChatID чат (с опциональным threadId)
	bool sendMessage(const char* message, int64_t threadId = 0) {
		return _chatId > 0 ? sendMessage(_chatId, message, threadId): false;
	}
	// Отправка сообщения в указанный setChatID чат (String, с опциональным threadId)
	bool sendMessage(const String& message, int64_t threadId = 0) {
		return _chatId > 0 ? sendMessage(_chatId, message.c_str(), threadId): false;
	}

	// Редактирование текста ранее отправленного сообщения в указанном чате
	bool editMessage(int64_t chatId, int64_t messageId, const char* message) {
		if (!chatId || !messageId) return false;
		if (_checkConnection && !_checkConnection()) return false;

		JsonDocument doc;
		doc[F("chat_id")] = chatId;
		doc[F("message_id")] = messageId;
		doc[F("text")] = message;
		if (strstr_P(message, PSTR("</"))) doc[F("parse_mode")] = F("HTML");

		String jsonPayload;
		serializeJson(doc, jsonPayload);
		LOG_TELEGRAM_API(println, jsonPayload);

		ApiResult r = apiRequest(F("editMessageText"), jsonPayload);
		return r.result;
	}
	// Редактирование текста сообщения в указанном чате (String)
	bool editMessage(int64_t chatId, int64_t messageId, const String& message) {
		return editMessage(chatId, messageId, message.c_str());
	}
	// Редактирование текста сообщения в чате по умолчанию (setChatID)
	bool editMessage(int64_t messageId, const char* message) {
		return _chatId > 0 ? editMessage(_chatId, messageId, message) : false;
	}
	// Редактирование текста сообщения в чате по умолчанию (setChatID, String)
	bool editMessage(int64_t messageId, const String& message) {
		return _chatId > 0 ? editMessage(_chatId, messageId, message.c_str()) : false;
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
	// Удаление нескольких сообщений из указанного чата (вектор ID)
	bool deleteMessages(int64_t chatId, const std::vector<int64_t>& messageIDs) {
		if (!chatId || messageIDs.empty()) return false;
		
		JsonDocument doc;
		doc[F("chat_id")] = chatId;
		JsonArray message_ids = doc[F("message_ids")].to<JsonArray>();
		for (auto id : messageIDs) {
			message_ids.add(id);
		}

		String jsonPayload;
		serializeJson(doc, jsonPayload);
		LOG_TELEGRAM_API(println, jsonPayload);

		ApiResult r = apiRequest(F("deleteMessages"), jsonPayload);
		return r.result;
	}
	// Удаление нескольких сообщений из чата по умолчанию (вектор ID)
	bool deleteMessages(const std::vector<int64_t>& messageIDs) {
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

	// Проверка новых сообщений. К этому моменту должна быть установлена функция для обработки сообщений attach
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
					r.threadId  = msg[F("message_thread_id")] | 0;
					r.text      = msg[F("text")]                  | "";
					r.from      = msg[F("from")][F("first_name")] | "";

					LOG_TELEGRAM_API(printf_P, PSTR("[MSG] %s (%lld): %s\n"), r.from.c_str(), r.chatId, r.text.c_str());
					if (_callback) {
						if (strict) {
							bool allowed = (_chatId != 0 && r.chatId == _chatId);
							if (!allowed && whiteList.length() > 0) allowed = isWhitelisted(r.chatId, whiteList);
							if (!allowed) continue; // сообщение от неавторизованного чата игнорируется
						}
						TELEGRAM_DELAY(1);
						String toSend = _callback(r);
						if (toSend.length() > 0) {
							TELEGRAM_DELAY(1); // небольшая задержка между приёмом и отправкой сообщения для сброса watchdog
							sendMessage(r.chatId, toSend.c_str(), r.threadId);
						}
					}
					result++;
				}
			}
		}
		else return -1; 
		return result;
	}

	// Проверка, есть ли чат в переданном списке разрешенных
	static bool isWhitelisted(const int64_t& chatId, const String& list) {
		if (!chatId || list.length() == 0) return false;
		int pos1 = 0;
		while (pos1 < (int)list.length()) {
			int pos2 = list.indexOf(',', pos1);
			String tmp = (pos2 >= 0) ? list.substring(pos1, pos2) : list.substring(pos1);
			tmp.trim();
			if (atoll(tmp.c_str()) == chatId) return true;
			if (pos2 < 0) break;
			pos1 = pos2 + 1;
		}
		return false;
	}

	// Проверка, есть ли чат в свойстве whiteList
	bool isWhitelisted(const int64_t& chatId) const {
		return isWhitelisted(chatId, whiteList);
	}

	// Извлечение первого номера из переданного списка чатов
	static int64_t extractFirstNumber(const String& list) {
		String trimmed = list;
		trimmed.trim();
		return atoll(trimmed.c_str());
	}

	// Извлечение первого номера из свойства whiteList
	int64_t extractFirstNumber() const {
		return extractFirstNumber(whiteList);
	}

	// Отослать сообщения всем, кто в переданном списке (const char*, с опциональным threadId)
	bool sendMessageToAll(const String& list, const char* message, int64_t threadId = 0) {
		int pos1 = 0, pos2 = 0;
		pos2 = list.indexOf(',', pos1);
		while ( pos2 >= 0 ) {
			String tmp = list.substring(pos1, pos2);
			tmp.trim();
			int64_t id = atoll(tmp.c_str());
			if (id) {
				if (!sendMessage(id, message, threadId)) return false;
			}
			pos1 = pos2 + 1;
			pos2 = list.indexOf(',', pos1);
		}
		// Последнее число в списке
		String tmp = list.substring(pos1);
		tmp.trim();
		int64_t id = atoll(tmp.c_str());
		if (id) {
			if (!sendMessage(id, message, threadId)) return false;
		}
		return true;
	}

	// Отослать сообщения всем, кто в переданном списке (String, с опциональным threadId)
	bool sendMessageToAll(const String& list, const String& message, int64_t threadId = 0) {
		return sendMessageToAll(list, message.c_str(), threadId);
	}

	// Отослать сообщения всем, кто в свойстве whiteList (const char*, с опциональным threadId)
	bool sendMessageToAll(const char* message, int64_t threadId = 0) {
		return sendMessageToAll(whiteList, message, threadId);
	}

	// Отослать сообщения всем, кто в свойстве whiteList (String, с опциональным threadId)
	bool sendMessageToAll(const String& message, int64_t threadId = 0) {
		return sendMessageToAll(whiteList, message.c_str(), threadId);
	}

	// Поиск параметра независимо от наличия "/" в начале. Регистр имеет значение.
	bool is_command(const String& in, const String& command) {
		int pos = in.indexOf(command);
		return pos == 0 || pos == 1;
	}

	// Принимать сообщения только от чата указанного в _chatId или номеров из whiteList
	bool strict = false;
	// Список разрешенных chat_id (через запятую)
	String whiteList;

	private:

	struct ApiResult {
		bool result;
		JsonDocument json;
	};

	// запрос к API telegram. Всегда POST, в payload должна быть сформированная строка Json
	// в type должна быть вызываемая функция: sendMessage, getUpdates...
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
				if ( response.length() < 2 ) break; // пустая строка отделяет заголовок от тела. Содержит пару символов '\r\n', один из них уже вырезан
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
					LOG_TELEGRAM_API(println, F("Invalid bot token, stopping"));
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
			LOG_TELEGRAM_API(println, F("No connection to Telegram server"));
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
		if((int32_t)(millis() - nextUpdate) > 0) {
			resetTimer();
			return true;
		}
		return false;
	}
	// сброс таймера на установленный интервал
	void resetTimer() {
		nextUpdate = millis() + updatePeriod;
	}

	SSLTransport& client;    // Ссылка на SSLClient

	String _botToken;
	const int maxRetries = 3; // Максимум попыток при ошибке соединения
	const int retryDelay = 1000; // Задержка между попытками (мс)
	const int requestTimeout = 35000; // Таймаут для long polling (35 секунд, учитывая timeout=30)

	int64_t lastUpdateId = 0, _chatId = 0;
	uint32_t nextUpdate = 0;
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