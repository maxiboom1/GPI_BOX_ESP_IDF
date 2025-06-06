#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <Ethernet.h>
#include "ConfigManager.h"

extern Config config; // Access stored config settings

void sendHttpPost(const String& message) {
    if (strlen(config.httpUrl) == 0) {
        Serial.println("HTTP URL is empty. Skipping HTTP send.");
        return;
    }

    // Extract host, port, and path from URL
    String url = String(config.httpUrl);
    int port = 80;  // Default HTTP port
    String host = "";
    String path = "/";

    // Check if URL starts with "http://", remove it
    if (url.startsWith("http://")) {
        url = url.substring(7);
    }

    // Find the first slash ("/") to separate the host and path
    int slashIndex = url.indexOf("/");
    if (slashIndex != -1) {
        host = url.substring(0, slashIndex);  // Extract host (including possible port)
        path = url.substring(slashIndex);     // Extract path
    } else {
        host = url;  // No path, just a hostname
    }

    // Check if a port is specified (e.g., "10.168.0.10:1234")
    int colonIndex = host.indexOf(":");
    if (colonIndex != -1) {
        port = host.substring(colonIndex + 1).toInt();  // Extract port
        host = host.substring(0, colonIndex);           // Extract only the hostname
    }

    Serial.print("Connecting to HTTP server: ");
    Serial.print(host);

    EthernetClient client;
    if (client.connect(host.c_str(), port)) {
        // Start HTTP Request
        client.print("POST ");
        client.print(path); 
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(host);
        client.println("Connection: close");
        client.println("Content-Type: application/json");

        client.print("Content-Length: ");
        client.println(message.length());
        client.println();  // End of headers

        // Send payload
        client.print(message);
        Serial.println("HTTP message sent.");

        client.stop();  // Close connection
    } else {
        Serial.println("HTTP connection failed.");
    }
}

#endif
