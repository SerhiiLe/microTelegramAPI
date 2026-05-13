#ifndef TELEGRAMAPI_H
#define TELEGRAMAPI_H

#include "TelegramAPI.h"

#if defined(ESP8266)

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>

class TelegramESP : public TelegramAPI<BearSSL::WiFiClientSecure> {

#elif defined(ESP32)
#include <WiFi.h>
#include <WiFiClientSecure.h>

class TelegramESP : public TelegramAPI<WiFiClientSecure> {

#endif

public:

    TelegramESP() : TelegramAPI(client) {
        client.setInsecure();
        #if defined(ESP8266)
        client.setBufferSizes(512, 512);
        #endif
    }


    #if defined(ESP8266)
    BearSSL::WiFiClientSecure client;
    #elif defined(ESP32)
    WiFiClientSecure client;
    #endif

private:

};

#endif