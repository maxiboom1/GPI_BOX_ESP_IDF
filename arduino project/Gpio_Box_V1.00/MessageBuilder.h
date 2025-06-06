#ifndef MESSAGE_BUILDER_H
#define MESSAGE_BUILDER_H

#include <ArduinoJson.h>

class MessageBuilder {
  public:
      static String constructMessage(const String& event, const String& state, const String& user, const String& password) {
          StaticJsonDocument<128> doc;
          doc["event"] = event;
          doc["state"] = state;
          doc["user"] = user;
          doc["password"] = password;

          String jsonString;
          serializeJson(doc, jsonString);
          return jsonString;
      }
};

#endif