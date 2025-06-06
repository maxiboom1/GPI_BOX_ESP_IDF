#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <EEPROM.h>
#include <ArduinoJson.h> // Ensure ESP32-compatible version

#define EEPROM_SIZE 248 // Matches our updated structure

// Config structure
struct Config {
  IPAddress deviceIp;         // 0-3
  IPAddress gateway;          // 4-7
  IPAddress subnetMask;       // 8-11
  bool tcpEnabled;            // 12
  IPAddress tcpIp;            // 13-16
  uint16_t tcpPort;           // 17-18
  bool tcpSecure;             // 19
  char tcpUser[32];           // 20-51
  char tcpPassword[32];       // 52-83
  bool httpEnabled;           // 84
  char httpUrl[64];           // 85-148
  bool httpSecure;            // 149
  char httpUser[32];          // 150-181
  char httpPassword[32];      // 182-213
  bool serialEnabled;         // 214
  char adminPassword[32];     // 215-246
  uint8_t configFlag;         // 247 (0xAA if configured)
};

// Forward declarations
void saveConfig(const Config& config);
void loadConfig(Config& config);
void setDefaultConfig(Config& config);

void initConfig(Config& config) {
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Failed to initialize EEPROM");
    return;
  }
  
  if (EEPROM.read(247) != 0xAA) {
    setDefaultConfig(config);
    Serial.println("EEPROM initialized with defaults");
  } else {
    loadConfig(config);
    Serial.println("EEPROM configured, loading existing config...");
  }

  Serial.print("Loaded IP from EEPROM: ");
  Serial.println(config.deviceIp);
  Serial.print("Loaded Gateway from EEPROM: ");
  Serial.println(config.gateway);
  Serial.print("Loaded Subnet Mask from EEPROM: ");
  Serial.println(config.subnetMask);
}

void loadConfig(Config& config) {
  // Device Ip, Gateway and subnetMask
  config.deviceIp[0] = EEPROM.read(0);
  config.deviceIp[1] = EEPROM.read(1);
  config.deviceIp[2] = EEPROM.read(2);
  config.deviceIp[3] = EEPROM.read(3);

  config.gateway[0] = EEPROM.read(4);
  config.gateway[1] = EEPROM.read(5);
  config.gateway[2] = EEPROM.read(6);
  config.gateway[3] = EEPROM.read(7);

  config.subnetMask[0] = EEPROM.read(8);
  config.subnetMask[1] = EEPROM.read(9);
  config.subnetMask[2] = EEPROM.read(10);
  config.subnetMask[3] = EEPROM.read(11);

  // Other configurations
  config.tcpEnabled = EEPROM.read(12);
  config.tcpIp[0] = EEPROM.read(13);
  config.tcpIp[1] = EEPROM.read(14);
  config.tcpIp[2] = EEPROM.read(15);
  config.tcpIp[3] = EEPROM.read(16);
  config.tcpPort = (EEPROM.read(18) << 8) | EEPROM.read(17); // Read as uint16_t
  config.tcpSecure = EEPROM.read(19);
  EEPROM.get(20, config.tcpUser); // Read char arrays
  EEPROM.get(52, config.tcpPassword);
  config.httpEnabled = EEPROM.read(84);
  EEPROM.get(85, config.httpUrl);
  config.httpSecure = EEPROM.read(149);
  EEPROM.get(150, config.httpUser);
  EEPROM.get(182, config.httpPassword);
  config.serialEnabled = EEPROM.read(214);
  EEPROM.get(215, config.adminPassword);
  config.configFlag = EEPROM.read(247);
}

void saveConfig(const Config& config) {
  // Manually write IPAddress byte-by-byte to ensure correctness
  EEPROM.write(0, config.deviceIp[0]);
  EEPROM.write(1, config.deviceIp[1]);
  EEPROM.write(2, config.deviceIp[2]);
  EEPROM.write(3, config.deviceIp[3]);

  EEPROM.write(4, config.gateway[0]);
  EEPROM.write(5, config.gateway[1]);
  EEPROM.write(6, config.gateway[2]);
  EEPROM.write(7, config.gateway[3]);

  EEPROM.write(8, config.subnetMask[0]);
  EEPROM.write(9, config.subnetMask[1]);
  EEPROM.write(10, config.subnetMask[2]);
  EEPROM.write(11, config.subnetMask[3]);

  // Write other fields
  EEPROM.write(12, config.tcpEnabled);
  EEPROM.write(13, config.tcpIp[0]);
  EEPROM.write(14, config.tcpIp[1]);
  EEPROM.write(15, config.tcpIp[2]);
  EEPROM.write(16, config.tcpIp[3]);
  EEPROM.write(17, (config.tcpPort & 0xFF)); // Low byte
  EEPROM.write(18, ((config.tcpPort >> 8) & 0xFF)); // High byte
  EEPROM.write(19, config.tcpSecure);
  EEPROM.put(20, config.tcpUser); // Write char arrays
  EEPROM.put(52, config.tcpPassword);
  EEPROM.write(84, config.httpEnabled);
  EEPROM.put(85, config.httpUrl);
  EEPROM.write(149, config.httpSecure);
  EEPROM.put(150, config.httpUser);
  EEPROM.put(182, config.httpPassword);
  EEPROM.write(214, config.serialEnabled);
  EEPROM.put(215, config.adminPassword);
  EEPROM.write(247, 0xAA); // Config flag
  EEPROM.commit();
  Serial.println("Config saved to EEPROM");
  Serial.print("Saved IP: ");
  Serial.println(config.deviceIp);
  Serial.print("Saved Gateway: ");
  Serial.println(config.gateway);
  Serial.print("Saved Subnet Mask: ");
  Serial.println(config.subnetMask);
}

void setDefaultConfig(Config& config){
    config.deviceIp = IPAddress(10, 168, 0, 177);
    config.subnetMask = IPAddress(255, 255, 255, 0);    
    config.gateway = IPAddress(10, 168, 0, 1);
    config.tcpEnabled = false;
    config.tcpIp = IPAddress(10, 10, 10, 10); 
    config.tcpPort = 12345;
    config.tcpSecure = false;
    memset(config.tcpUser, 0, 32);
    memset(config.tcpPassword, 0, 32);
    config.httpEnabled = false;
    memset(config.httpUrl, 0, 64); 
    config.httpSecure = false;
    memset(config.httpUser, 0, 32);
    memset(config.httpPassword, 0, 32);
    config.serialEnabled = false;
    strncpy(config.adminPassword, "admin", 32);
    config.configFlag = 0xAA;
    saveConfig(config);
}

#endif