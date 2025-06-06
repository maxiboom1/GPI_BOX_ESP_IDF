#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Ethernet.h>
#include "ConfigManager.h"
#include <ArduinoJson.h> // Include ArduinoJson for JSON parsing

extern Config config; // From main sketch
extern LiquidCrystal_I2C lcd; // For LCD updates


// Forward declarations
void sendConfigPage(EthernetClient& client);
void handleSaveSettings(EthernetClient& client, const String& body);
void sendLoginPage(EthernetClient& client, const String& request, bool loginFailed = false);
void handleLogin(EthernetClient& client, const String& body, const String& request);
void sendNoContentResponse(EthernetClient& client);

// Local helper function for JSON parsing
bool parseJsonConfig(String json, Config& config) {
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        Serial.print("JSON Parsing Failed: ");
        Serial.println(error.f_str());
        return false;
    }

    // Update only the fields that exist in the received JSON
    if (doc.containsKey("ip")) config.deviceIp.fromString(doc["ip"].as<const char*>());
    if (doc.containsKey("gateway")) config.gateway.fromString(doc["gateway"].as<const char*>());
    if (doc.containsKey("subnetMask")) config.subnetMask.fromString(doc["subnetMask"].as<const char*>());

    if (doc.containsKey("tcpEnabled")) config.tcpEnabled = doc["tcpEnabled"].as<bool>();
    if (doc.containsKey("tcpIp")) config.tcpIp.fromString(doc["tcpIp"].as<const char*>());
    if (doc.containsKey("tcpPort")) config.tcpPort = doc["tcpPort"].as<uint16_t>();
    if (doc.containsKey("tcpSecure")) config.tcpSecure = doc["tcpSecure"].as<bool>();
    if (doc.containsKey("tcpUser")) strncpy(config.tcpUser, doc["tcpUser"].as<const char*>(), sizeof(config.tcpUser));
    if (doc.containsKey("tcpPassword")) strncpy(config.tcpPassword, doc["tcpPassword"].as<const char*>(), sizeof(config.tcpPassword));

    if (doc.containsKey("httpEnabled")) config.httpEnabled = doc["httpEnabled"].as<bool>();
    if (doc.containsKey("httpUrl")) strncpy(config.httpUrl, doc["httpUrl"].as<const char*>(), sizeof(config.httpUrl));
    if (doc.containsKey("httpSecure")) config.httpSecure = doc["httpSecure"].as<bool>();
    if (doc.containsKey("httpUser")) strncpy(config.httpUser, doc["httpUser"].as<const char*>(), sizeof(config.httpUser));
    if (doc.containsKey("httpPassword")) strncpy(config.httpPassword, doc["httpPassword"].as<const char*>(), sizeof(config.httpPassword));

    if (doc.containsKey("serialEnabled")) config.serialEnabled = doc["serialEnabled"].as<bool>();

    // Only update the admin password if the user entered a new one
    if (doc.containsKey("adminPassword")) {
        String newPassword = doc["adminPassword"].as<const char*>();
        if (!newPassword.isEmpty()) {  // Only set if a new password is provided
            strncpy(config.adminPassword, newPassword.c_str(), sizeof(config.adminPassword));
        }
    }

    return true;
}

void handleWebClient(EthernetServer& server) {
  EthernetClient client = server.available();
  if (client) {
    String request = "";
    String body = "";
    bool headersComplete = false;
    int contentLength = -1;
    int bodyBytesRead = 0;
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;

        if (!headersComplete && request.endsWith("\r\n\r\n")) {
          headersComplete = true;

          // Extract Content-Length
          int lengthPos = request.indexOf("Content-Length: ");
          if (lengthPos != -1) {
            int lengthEnd = request.indexOf('\r', lengthPos);
            String lengthStr = request.substring(lengthPos + 16, lengthEnd);
            contentLength = lengthStr.toInt();
          }

          // Handle login form submission
          if (request.startsWith("POST / ")) {
            while (client.available() && bodyBytesRead < contentLength) {
              char b = client.read();
              body += b;
              bodyBytesRead++;
            }
            handleLogin(client, body, request);
            return;
          }

          // Handle config save request
          if (request.startsWith("POST /save ")) {
            while (client.available() && bodyBytesRead < contentLength) {
              char b = client.read();
              body += b;
              bodyBytesRead++;
            }
            handleSaveSettings(client, body);
            return;
          }
          
          // Return 404 for favicon requests
          if (request.startsWith("GET /favicon.ico ")){
            Serial.println("Got favicon req. Responsed with 404");
            sendNoContentResponse(client);
            return;
          }
          
          if (request.startsWith("GET / ")) {
              sendLoginPage(client, request);
              return;
          }
          
          Serial.println("Unknown Web request. Sending fallback response");
          sendNoContentResponse(client);
          return;

        }
      }
    }

    client.stop();
  }
}

// Handles "GET /" requests
void sendLoginPage(EthernetClient& client, const String& request, bool loginFailed) {
  
  // If Cookie found, serve config page
  if (request.indexOf("Cookie: sessionToken=loggedIn") >= 0) {
    sendConfigPage(client);  
    return;
  }
  
  // No cookie found, serve login form: 
  String errorMsg = "";
  if (loginFailed) {errorMsg = "<p style='color:red;'>No match, try again</p>";}
  String loginPage = "<!DOCTYPE html><html><head>"
                      "<style>body { font-family: Arial; } input { margin: 2.5px; }</style>"
                      "</head><body>"
                      "<h2>GPIO Box Login</h2>"
                      "<form method='POST' action='/'>"
                      "User: <input name='user'><br>"
                      "Password: <input name='password' type='password'><br>"
                      "<input type='submit' value='Login'>" + errorMsg +
                      "</form></body></html>";

  client.print("HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: " + String(loginPage.length()) + "\r\n"
                "Connection: close\r\n\r\n" + loginPage);
}

void sendNoContentResponse(EthernetClient& client){
  client.print("HTTP/1.1 404 Not Found\r\n"
               "Content-Type: text/html\r\n"
               "Content-Length: 9\r\n"
               "Connection: close\r\n\r\n"
               "Not Found");
}

void sendConfigPage(EthernetClient& client) {
  String htmlContent = "<!DOCTYPE html><html><head>"
                     "<style>body {font-family: Arial; }input {margin:2.5px;}</style></head>"
                     "<body><h2>GPIO Box Configuration</h2><hr>"
                     "<form id='configForm' onsubmit='return validateForm(event)'>"

                     "<h3>Device Network Settings</h3>"
                     "IP: <input type='text' name='ip' id='ip' value='" + config.deviceIp.toString() + "'><br>"
                     "Subnet Mask: <input type='text' name='subnetMask' id='subnetMask' value='" + config.subnetMask.toString() + "'><br>"                     
                     "Gateway: <input type='text' name='gateway' id='gateway' value='" + config.gateway.toString() + "'><br><hr>"

                     "<h3>TCP Settings</h3>"
                     "Enabled: <input type='checkbox' name='tcpEnabled' id='tcpEnabled' onchange='toggleTcpSettings()' " + (config.tcpEnabled ? "checked" : "") + "><br>"
                     "IP: <input type='text' name='tcpIp' id='tcpIp' value='" + config.tcpIp.toString() + "'><br>"
                     "Port: <input type='number' name='tcpPort' id='tcpPort' value='" + String(config.tcpPort) + "'><br>"
                     "Secure Mode: <input type='checkbox' name='tcpSecure' id='tcpSecure' onchange='toggleSecureMode(\"tcp\")' " + (config.tcpSecure ? "checked" : "") + "><br>"
                     "User: <input type='text' name='tcpUser' id='tcpUser' value='" + String(config.tcpUser) + "'><br>"
                     "Password: <input type='password' name='tcpPassword' id='tcpPassword' value='" + String(config.tcpPassword) + "'><br><hr>"

                     "<h3>HTTP Settings</h3>"
                     "Enabled: <input type='checkbox' name='httpEnabled' id='httpEnabled' onchange='toggleHttpSettings()' " + (config.httpEnabled ? "checked" : "") + "><br>"
                     "URL: <input type='text' name='httpUrl' id='httpUrl' value='" + String(config.httpUrl) + "'><br>"
                     "Secure Mode: <input type='checkbox' name='httpSecure' id='httpSecure' onchange='toggleSecureMode(\"http\")' " + (config.httpSecure ? "checked" : "") + "><br>"
                     "User: <input type='text' name='httpUser' id='httpUser' value='" + String(config.httpUser) + "'><br>"
                     "Password: <input type='password' name='httpPassword' id='httpPassword' value='" + String(config.httpPassword) + "'><br><hr>"

                     "<h3>Serial Settings</h3>"
                     "Enabled: <input type='checkbox' name='serialEnabled' id='serialEnabled' " + (config.serialEnabled ? "checked" : "") + "><br><hr>"

                     "<h3>Admin Access</h3>"
                     "New Password: <input type='password' name='adminPassword' id='adminPassword' value=''><br>"
                     "<input type='submit' value='Save'><br><br>"

                     "</form>";

  String script1 = "<script>"
                 "function validateForm(event) {"
                 "    if (event) event.preventDefault();"
                 "    function isValidIP(ip) {"
                 "        return /^((25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)$/.test(ip);"
                 "    }"
                 "    function isValidPort(port) {"
                 "        return /^[1-9][0-9]{0,4}$/.test(port) && port > 0 && port <= 65535;"
                 "    }"
                 "    function isValidUrl(url) {"
                 "        try {"
                 "            let parsedUrl = new URL(url);"
                 "            return (parsedUrl.protocol === 'http:' || parsedUrl.protocol === 'https:') && parsedUrl.hostname.length > 0;"
                 "        } catch (e) { return false; }"
                 "    }"
                 "    let ip = document.getElementById('ip').value;"
                 "    let gateway = document.getElementById('gateway').value;"
                 "    let subnetMask = document.getElementById('subnetMask').value;"
                 "    let tcpIp = document.getElementById('tcpIp').value;"
                 "    let tcpPort = document.getElementById('tcpPort').value;"
                 "    let httpUrl = document.getElementById('httpUrl').value;"
                 "    let adminPassword = document.getElementById('adminPassword').value;"

                 "    if (!isValidIP(ip) || !isValidIP(gateway) || !isValidIP(subnetMask)) {"
                 "        alert('Invalid IP Address, Gateway, or Subnet Mask!');"
                 "        return false;"
                 "    }"

                 "    if (!isValidIP(tcpIp)) {"
                 "        if (document.getElementById('tcpEnabled').checked) {"
                 "            alert('Invalid TCP IP address!');"
                 "            return false;"
                 "        }"
                 "    }"
                 "    if (!isValidPort(tcpPort)) {"
                 "        if (document.getElementById('tcpEnabled').checked) {"
                 "            alert('Invalid TCP Port! Must be between 1-65535.');"
                 "            return false;"
                 "        }"
                 "    }"

                 "    if (!isValidUrl(httpUrl)) {"
                 "        if (document.getElementById('httpEnabled').checked) {"
                 "            alert('Invalid HTTP URL!');"
                 "            return false;"
                 "        }"
                 "    }"

                 "    if (adminPassword.length > 32) {"
                 "        alert('Admin password too long! (Max 32 characters)');"
                 "        return false;"
                 "    }"

                 "    sendJson();"
                 "    return false;"
                 "}";


  String script2 = "function sendJson() {"
                 "    let data = {"
                 "        ip: document.getElementById('ip').value,"
                 "        gateway: document.getElementById('gateway').value,"
                 "        subnetMask: document.getElementById('subnetMask').value,"
                 "        tcpEnabled: document.getElementById('tcpEnabled').checked,"
                 "        httpEnabled: document.getElementById('httpEnabled').checked,"
                 "        serialEnabled: document.getElementById('serialEnabled').checked"
                 "    };"

                 "    if (data.tcpEnabled) {"
                 "        data.tcpIp = document.getElementById('tcpIp').value;"
                 "        data.tcpPort = parseInt(document.getElementById('tcpPort').value) || 0;"
                 "        data.tcpSecure = document.getElementById('tcpSecure').checked;"
                 "        if (data.tcpSecure) {"
                 "            data.tcpUser = document.getElementById('tcpUser').value;"
                 "            data.tcpPassword = document.getElementById('tcpPassword').value;"
                 "        }"
                 "    }"

                 "    if (data.httpEnabled) {"
                 "        data.httpUrl = document.getElementById('httpUrl').value;"
                 "        data.httpSecure = document.getElementById('httpSecure').checked;"
                 "        if (data.httpSecure) {"
                 "            data.httpUser = document.getElementById('httpUser').value;"
                 "            data.httpPassword = document.getElementById('httpPassword').value;"
                 "        }"
                 "    }"

                 "    let adminPassword = document.getElementById('adminPassword').value;"
                 "    if (adminPassword.length > 0) {"
                 "        data.adminPassword = adminPassword;"
                 "    }"

                 "    fetch('/save', {"
                 "        method: 'POST',"
                 "        headers: { 'Content-Type': 'application/json' },"
                 "        body: JSON.stringify(data)"
                 "    })"
                 "    .then(response => {"
                 "        if (response.ok) {"
                 "            alert('Configuration Saved Successfully!');"
                 "            location.reload();"
                 "        } else {"
                 "            alert('Error saving configuration!');"
                 "        }"
                 "    });"
                 "}";
  
  String footer = "function toggleSecureMode(type) {"
                 "    let secureCheckbox = document.getElementById(type + 'Secure');"
                 "    let userField = document.getElementById(type + 'User');"
                 "    let passwordField = document.getElementById(type + 'Password');"
                 "    if (secureCheckbox.checked) {"
                 "        userField.disabled = false;"
                 "        passwordField.disabled = false;"
                 "    } else {"
                 "        userField.disabled = true;"
                 "        passwordField.disabled = true;"
                 "    }"
                 "}"

                 "function toggleTcpSettings() {"
                 "    let tcpEnabledCheckbox = document.getElementById('tcpEnabled');"
                 "    let tcpIp = document.getElementById('tcpIp');"
                 "    let port = document.getElementById('tcpPort');"
                 "    let tcpSecureCheckbox = document.getElementById('tcpSecure');"
                 "    if (tcpEnabledCheckbox.checked) {"
                 "        tcpIp.disabled = false;"
                 "        port.disabled = false;"
                 "        tcpSecureCheckbox.disabled = false;"
                 "        toggleSecureMode('tcp');"
                 "    } else {"
                 "        tcpIp.disabled = true;"
                 "        port.disabled = true;"
                 "        tcpSecureCheckbox.disabled = true;"
                 "        document.getElementById('tcpUser').disabled = true;"
                 "        document.getElementById('tcpPassword').disabled = true;"
                 "    }"
                 "}"

                 "function toggleHttpSettings() {"
                 "    let httpEnabledCheckbox = document.getElementById('httpEnabled');"
                 "    let httpUrl = document.getElementById('httpUrl');"
                 "    let httpSecureCheckbox = document.getElementById('httpSecure');"
                 "    if (httpEnabledCheckbox.checked) {"
                 "        httpUrl.disabled = false;"
                 "        httpSecureCheckbox.disabled = false;"
                 "        toggleSecureMode('http');"
                 "    } else {"
                 "        httpUrl.disabled = true;"
                 "        httpSecureCheckbox.disabled = true;"
                 "        document.getElementById('httpUser').disabled = true;"
                 "        document.getElementById('httpPassword').disabled = true;"
                 "    }"
                 "}"

                 "window.onload = function () {"
                 "    toggleTcpSettings();"
                 "    toggleHttpSettings();"
                 "}"
                 "</script>"
                "<footer style='border-top: 1px solid #ccc; padding: 10px; text-align: center;'>GPIO Box v1.0</footer></body></html>";
  int contentLength = htmlContent.length() + script1.length() + script2.length() + footer.length();
  String header = "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: " + String(contentLength) + "\r\n"
                "Connection: close\r\n\r\n";

  
  client.print(header);
  delay(50);
  client.print(htmlContent);
  delay(50);
  client.print(script1);
  delay(50);
  client.print(script2);
  delay(50);
  client.print(footer);
}

void handleSaveSettings(EthernetClient& client, const String& body) {
    Config newConfig = config; // Copy current config to preserve existing values
    //Serial.println(config);
    // Parse the JSON body into newConfig
    if (!parseJsonConfig(body, newConfig)) {
        String response = "HTTP/1.1 400 Bad Request\r\n"
                          "Content-Type: text/html\r\n"
                          "Content-Length: 20\r\n"
                          "Connection: close\r\n"
                          "\r\n<h2>Invalid JSON</h2>";
        client.print(response);
        Serial.println("Invalid JSON with Content-Length: 20");
        return;
    }

    // Save the updated configuration
    saveConfig(newConfig);
    config = newConfig; // Apply the new configuration globally

    // Send success response
    String htmlContent = "<h2>Config Saved<br><a href=\"/\">Back</a></h2>";
    int contentLength = htmlContent.length();

    String response = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: text/html\r\n"
                      "Content-Length: " + String(contentLength) + "\r\n"
                      "Connection: close\r\n"
                      "\r\n" + htmlContent;
    
    client.print(response);
    Serial.println("Config saved and applied");
}

// ***************************** Login Handling functions ***************************** //
struct LoginCredentials {
  String username;
  String password;
  bool isValid = false;
};

LoginCredentials extractCredentials(const String& body){
    LoginCredentials creds;
    int userStart = body.indexOf("user=");
    int passStart = body.indexOf("password=");
    
    if (userStart == -1 || passStart == -1) {return creds;}

    userStart += 5;
    passStart += 9;
    
    int userEnd = body.indexOf("&", userStart);
    if (userEnd == -1) {return creds;}

    creds.username = body.substring(userStart, userEnd);
    creds.password = body.substring(passStart);
    creds.isValid = true;
    
    return creds;
}

// Handles "POST / " that comes by submitting on login page
void handleLogin(EthernetClient& client, const String& body, const String& request) {
  
  LoginCredentials creds = extractCredentials(body);
  
  if (!creds.isValid || creds.username != "admin" || creds.password != String(config.adminPassword)) {
        //sendUnauthorizedResponse(client);
        sendLoginPage(client, request, true);  // true indicates login failure
        return;
    }
  
  // Login success: Send session cookie and redirect
  client.print("HTTP/1.1 302 Found\r\n");
  client.print("Set-Cookie: sessionToken=loggedIn; HttpOnly\r\n");
  client.print("Location: /\r\n");
  client.print("Connection: close\r\n\r\n");

}


#endif