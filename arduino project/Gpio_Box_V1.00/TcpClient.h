#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <Ethernet.h>
#include "ConfigManager.h"
#include "MessageBuilder.h"

extern Config config;
extern bool ethConnected;

class TcpClient {
  private:
    static EthernetClient client;
    static bool isConnected;
    static unsigned long lastReconnectAttempt;

  public:
    static void manageConnection() {
        if (!config.tcpEnabled || !ethConnected) {
            if (isConnected) {
                Serial.println("TCP disabled or Ethernet lost. Closing TCP connection.");
                client.stop();
                isConnected = false;
            }
            return;
        }

        if (!client.connected()) {
            unsigned long currentMillis = millis();
            if (currentMillis - lastReconnectAttempt >= 5000) { // Retry every 5 seconds
                lastReconnectAttempt = currentMillis;
                Serial.print("Attempting to connect to TCP server: ");
                Serial.print(config.tcpIp);
                Serial.print(":");
                Serial.println(config.tcpPort);

                if (client.connect(config.tcpIp, config.tcpPort)) {
                    Serial.println("TCP connection established.");
                    isConnected = true;
                } else {
                    Serial.println("TCP connection failed. Retrying in 5 seconds...");
                    isConnected = false;
                }
            }
        }
    }

    static void sendTcpMessage(const String& message) {
        if (isConnected) {
            Serial.println("Sending TCP message...");
            client.println(message);
        } else {
            Serial.println("TCP message not sent. No active connection.");
        }
    }
};

// Initialize static variables
EthernetClient TcpClient::client;
bool TcpClient::isConnected = false;
unsigned long TcpClient::lastReconnectAttempt = 0;

#endif
