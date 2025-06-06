#ifndef ETH_MANAGER_H
#define ETH_MANAGER_H

#include <Ethernet.h>
#include "ConfigManager.h"
IPAddress currentDeviceIp;
IPAddress currentSubnetMask;
IPAddress currentGateway;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
extern Config config; // From main sketch
extern byte mac[];
extern LiquidCrystal_I2C lcd; // For LCD updates
extern bool ethConnected;
void updateEthernetStatus(LiquidCrystal_I2C& lcd);

// Function to initialize Ethernet
void attemptEthernetInit() {
  // Use the device IP, gateway, and subnet mask from the global config
  Ethernet.begin(mac, config.deviceIp, config.gateway, config.gateway, config.subnetMask);
  lcd.setCursor(0, 0);
  lcd.print("Applying network");
  lcd.setCursor(0, 1);
  lcd.print("configuration...");
  delay(3000);
  lcd.clear();
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield not found.");
    lcd.setCursor(0, 1);
    lcd.print("Link:HW error   ");
    ethConnected = false;
    return;
  }
  currentDeviceIp = config.deviceIp;
  currentSubnetMask = config.subnetMask;
  currentGateway = config.gateway;
  lcd.setCursor(0, 0);
  lcd.print(config.deviceIp);
  updateEthernetStatus(lcd); // Check initial link status
}

// Function to update Ethernet status on LCD
void updateEthernetStatus(LiquidCrystal_I2C& lcd) {
  
  // We found that the ip was changed
  if(config.deviceIp != currentDeviceIp || config.gateway != currentGateway || config.subnetMask != currentSubnetMask ){
    attemptEthernetInit();
  }

  if (Ethernet.linkStatus() == LinkON) {
    ethConnected = true;
    lcd.setCursor(0, 1);
    lcd.print("Link:OK         ");
  } else {
    ethConnected = false;
    lcd.setCursor(0, 1);
    lcd.print("Link:Offline    ");
  }
}

#endif