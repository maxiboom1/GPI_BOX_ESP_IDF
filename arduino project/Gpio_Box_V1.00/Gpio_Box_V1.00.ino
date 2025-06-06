#include <SPI.h>
#include <Ethernet.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include "HttpClient.h"
#include "EthManager.h"
#include "ConfigManager.h"
#include "WebServer.h"
#include "MessageBuilder.h"
#include "TcpClient.h"  

#define RESET_BUTTON 0  // GPIO 0 (BOOT Button)
#define RESET_HOLD_TIME 5000  // 5 seconds

Config config; // Global config

#define W5500_CS 15

// Define monitored GPIO Inputs (According to project documentation)
#define TOTAL_GPI 8
const int inputPins[TOTAL_GPI] = {4, 5, 12, 13, 14, 16, 17, 25}; 
const char* gpiNames[TOTAL_GPI] = {"GPI-1", "GPI-2", "GPI-3", "GPI-4", "GPI-5", "GPI-6", "GPI-7", "GPI-8"};
int lastPinState[TOTAL_GPI] = {HIGH}; // Ensure default HIGH to prevent false triggers

#define I2C_SDA 21
#define I2C_SCL 22
LiquidCrystal_I2C lcd(0x27, 16, 2);

bool ethConnected = false;
unsigned long lastEthCheck = 0;
const unsigned long ethCheckInterval = 5000;

class MyEthernetServer : public EthernetServer {
  public:
    MyEthernetServer(uint16_t port) : EthernetServer(port) {}
    void begin(uint16_t port = 0) override {
      if (port != 0) {
        EthernetServer::begin();
      } else {
        EthernetServer::begin();
      }
      Serial.println("Server begin called");
    }
};

MyEthernetServer webServer(80);

void setup() {
  
  pinMode(RESET_BUTTON, INPUT_PULLUP);  // Set GPIO 0 as input with pull-up
  Serial.begin(9600);

  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("I/O Systems");
  lcd.setCursor(0, 1);
  lcd.print("GPIO Box V1.00");

  // Initialize GPIO Inputs with Pull-ups to avoid floating state
  for (int i = 0; i < TOTAL_GPI; i++) {
      pinMode(inputPins[i], INPUT_PULLUP); 
      lastPinState[i] = digitalRead(inputPins[i]); // Initialize with current state
  }

  initConfig(config);

  Ethernet.init(W5500_CS);
  attemptEthernetInit();
  Serial.print("Ethernet IP after init: ");
  Serial.println(Ethernet.localIP());
  Serial.print("Link status: ");
  Serial.println(Ethernet.linkStatus() == LinkON ? "ON" : "OFF");

  if (Ethernet.localIP() == IPAddress(0, 0, 0, 0)) {
    Serial.println("Warning: Ethernet failed to set IP. Attempting DHCP...");
    if (Ethernet.begin(mac)) {
      Serial.print("DHCP IP assigned: ");
      Serial.println(Ethernet.localIP());
    } else {
      Serial.println("DHCP failed. Ethernet not connected.");
      ethConnected = false;
    }
  } else {
    ethConnected = true;
  }

  webServer.begin();
  Serial.println("Web server started at: ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  handleWebClient(webServer);
  TcpClient::manageConnection();  // Maintain TCP connection

  unsigned long currentMillis = millis();
  if (currentMillis - lastEthCheck >= ethCheckInterval) {
      updateEthernetStatus(lcd);
      lastEthCheck = currentMillis;
  }

  if (Serial.available()) {
      String received = Serial.readStringUntil('\n');
      heartbeatHandler(received);
  }

  for (int i = 0; i < TOTAL_GPI; i++) {
      int currentState = digitalRead(inputPins[i]);

      if (lastPinState[i] != currentState) {
          delay(50); // Debounce
          int stableState = digitalRead(inputPins[i]);

          if (stableState == currentState) {
              lastPinState[i] = currentState;
              String stateStr = (currentState == HIGH) ? "HIGH" : "LOW";

              if (ethConnected) {
                  sendTcpMessage(i, stateStr);
                  sendHttpMessage(i, stateStr);
                  sendSerialMessage(i, stateStr);

                  lcd.setCursor(0, 1);
                  lcd.print("Sent: " + String(gpiNames[i]) + " " + stateStr);
              } else {
                  Serial.println("tick (not sent - no Ethernet)");
                  lcd.setCursor(0, 1);
                  lcd.print("No Eth - Tick");
              }
          }
      }
  }

  // ------------- Reset logic ------------- //
  static unsigned long buttonPressTime = 0;
  static bool resetTriggered = false;

  if (digitalRead(RESET_BUTTON) == LOW) {
    if (buttonPressTime == 0) {
      buttonPressTime = millis();  // Start timing
    } else if (millis() - buttonPressTime >= RESET_HOLD_TIME && !resetTriggered) {
      Serial.println("Got reset command");
      setDefaultConfig(config);          // Explicitly reset to known defaults
      saveConfig(config);                // Save to EEPROM
      lcd.clear();
      lcd.print("Factory Reset");
      delay(2000);
      ESP.restart();                     // Restart device to apply defaults
    }
  }
}

void sendTcpMessage(int index, const String& stateStr) {
    if (config.tcpEnabled) {
        String tcpUser = config.tcpSecure ? String(config.tcpUser) : "";
        String tcpPassword = config.tcpSecure ? String(config.tcpPassword) : "";
        String message = MessageBuilder::constructMessage(gpiNames[index], stateStr, tcpUser, tcpPassword);
        TcpClient::sendTcpMessage(message);
    }
}

void sendHttpMessage(int index, const String& stateStr) {
    if (config.httpEnabled) {
        String httpUser = config.httpSecure ? String(config.httpUser) : "";
        String httpPassword = config.httpSecure ? String(config.httpPassword) : "";
        String message = MessageBuilder::constructMessage(gpiNames[index], stateStr, httpUser, httpPassword);
        sendHttpPost(message);
    }
}

void sendSerialMessage(int index, const String& stateStr) {
    if (config.serialEnabled) {
        String message = MessageBuilder::constructMessage(gpiNames[index], stateStr, "", "");
        Serial.println(message);
    }
}

void heartbeatHandler(String received) {
  received.trim();
  if (received == "heartbeat") {
    Serial.println("ack");
    lcd.setCursor(0, 1);
    lcd.print("Heartbeat      ");
  }
}
