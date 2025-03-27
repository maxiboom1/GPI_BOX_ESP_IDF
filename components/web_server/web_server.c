#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include <stdio.h>
#include <string.h>
#include "app_config.h"  
#include "lwip/ip4_addr.h"
#include "cJSON.h"
#include "eth_setup.h"


static const char *TAG = "web_server";

const char* itoa_buf(int value) {
    static char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    return buf;
}

const char* get_placeholder_value(const char* key, char* outBuf, size_t outSize) {
    if (strcmp(key, "deviceIp") == 0) return ip4addr_ntoa((const ip4_addr_t*)&globalConfig.deviceIp);
    if (strcmp(key, "gateway") == 0) return ip4addr_ntoa((const ip4_addr_t*)&globalConfig.gateway);
    if (strcmp(key, "subnetMask") == 0) return ip4addr_ntoa((const ip4_addr_t*)&globalConfig.subnetMask);
    if (strcmp(key, "companionIp") == 0) return ip4addr_ntoa((const ip4_addr_t*)&globalConfig.companionIp);
    if (strcmp(key, "companionPort") == 0) {
        snprintf(outBuf, outSize, "%u", globalConfig.companionPort);
        return outBuf;
    }
    if (strcmp(key, "companionMode") == 0) return globalConfig.companionMode ? "checked" : "";
    if (strcmp(key, "tcpEnabled") == 0) return globalConfig.tcpEnabled ? "checked" : "";
    if (strcmp(key, "tcpIp") == 0) return ip4addr_ntoa((const ip4_addr_t*)&globalConfig.tcpIp);
    if (strcmp(key, "tcpPort") == 0) {
        snprintf(outBuf, outSize, "%u", globalConfig.tcpPort);
        return outBuf;
    }
    if (strcmp(key, "tcpSecure") == 0) return globalConfig.tcpSecure ? "checked" : "";
    if (strcmp(key, "tcpUser") == 0) return globalConfig.tcpUser;
    if (strcmp(key, "tcpPassword") == 0) return globalConfig.tcpPassword;
    if (strcmp(key, "httpEnabled") == 0) return globalConfig.httpEnabled ? "checked" : "";
    if (strcmp(key, "httpUrl") == 0) return globalConfig.httpUrl;
    if (strcmp(key, "httpSecure") == 0) return globalConfig.httpSecure ? "checked" : "";
    if (strcmp(key, "httpUser") == 0) return globalConfig.httpUser;
    if (strcmp(key, "httpPassword") == 0) return globalConfig.httpPassword;
    if (strcmp(key, "serialEnabled") == 0) return globalConfig.serialEnabled ? "checked" : "";

    return "";
}

// Open  given file from spiffs, and send it chunked
esp_err_t serve_file(httpd_req_t *req, const char *filename) {
    FILE *f;
    char path[64];
    snprintf(path, sizeof(path), "/spiffs_data/%s", filename);
    f = fopen(path, "r");
    if (!f) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    char file_buffer[512];
    char send_buffer[512];
    char ph_buffer[64];  // for collecting placeholder names
    char temp_value[70]; // temp buffer for placeholder output

    size_t send_len = 0;
    size_t ph_len = 0;
    bool in_placeholder = false;

    size_t bytes_read;
    while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), f)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            char c = file_buffer[i];

            // Start of placeholder
            if (!in_placeholder && c == '{' && i + 1 < bytes_read && file_buffer[i + 1] == '{') {
                in_placeholder = true;
                ph_len = 0;
                i++;  // skip next '{'
                continue;
            }

            // End of placeholder
            if (in_placeholder && c == '}' && i + 1 < bytes_read && file_buffer[i + 1] == '}') {
                ph_buffer[ph_len] = '\0';
                const char *value = get_placeholder_value(ph_buffer, temp_value, sizeof(temp_value));
                size_t value_len = strlen(value);

                // flush if value won't fit
                if (send_len + value_len >= sizeof(send_buffer)) {
                    httpd_resp_send_chunk(req, send_buffer, send_len);
                    send_len = 0;
                }

                memcpy(send_buffer + send_len, value, value_len);
                send_len += value_len;

                i++;  // skip next '}'
                in_placeholder = false;
                continue;
            }

            // Inside placeholder content
            if (in_placeholder) {
                if (ph_len < sizeof(ph_buffer) - 1) {
                    ph_buffer[ph_len++] = c;
                }
                // else: overflow — silently truncate
            } else {
                // Normal char
                if (send_len >= sizeof(send_buffer)) {
                    httpd_resp_send_chunk(req, send_buffer, send_len);
                    send_len = 0;
                }
                send_buffer[send_len++] = c;
            }
        }
    }

    fclose(f);

    // Flush remaining send buffer
    if (send_len > 0) {
        httpd_resp_send_chunk(req, send_buffer, send_len);
    }

    httpd_resp_send_chunk(req, NULL, 0);  // End of response
    return ESP_OK;
}

static esp_err_t serve_login_page(httpd_req_t *req, bool loginFailed) {
    const char *errorMsg = loginFailed ? "<p style='color:red;'>No match, try again</p>" : "";
    const char *loginPage =
        "<!DOCTYPE html><html><head>"
        "<style>body { font-family: Arial; } input { margin: 2.5px; }</style>"
        "</head><body>"
        "<h2>GPIO Box Login</h2>"
        "<form method='POST' action='/'>"
        "User: <input name='user'><br>"
        "Password: <input name='password' type='password'><br>"
        "<input type='submit' value='Login'>"
        "%s"
        "</form></body></html>";

    char pageBuffer[512];
    snprintf(pageBuffer, sizeof(pageBuffer), loginPage, errorMsg);
    httpd_resp_send(req, pageBuffer, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t serve_config_page(httpd_req_t *req) {
	return serve_file(req, "index.html");
}

static esp_err_t handle_login(httpd_req_t *req) {
    char content[100];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request");
    }
    content[ret] = '\0';

    const char *userMatch = strstr(content, "user=admin");
    const char *passPos = strstr(content, "password=");

    if (userMatch && passPos) {
        // Extract password from form string
        char password[32] = {0};
        strncpy(password, passPos + strlen("password="), sizeof(password) - 1);

        // Compare against stored password
        if (strcmp(password, globalConfig.adminPassword) == 0) {
            httpd_resp_set_hdr(req, "Set-Cookie", "sessionToken=loggedIn; Path=/;");
            httpd_resp_set_status(req, "302 Found");
            httpd_resp_set_hdr(req, "Location", "/");
            return httpd_resp_send(req, NULL, 0);
        }
    }

    return serve_login_page(req, true);  // Login failed
}

static esp_err_t HTTP_get_router(httpd_req_t *req) {
    char buf[200];
    if (httpd_req_get_hdr_value_str(req, "Cookie", buf, sizeof(buf)) == ESP_OK) {
        if (strstr(buf, "sessionToken=loggedIn")) {
            return serve_config_page(req);
        }
    }

    if (req->method == HTTP_POST) {
        return handle_login(req);
    }
    return serve_login_page(req, false);
}

static esp_err_t serve_js_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/javascript");
    return serve_file(req, "index.js");
}

static esp_err_t serve_css_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/css");
    return serve_file(req, "styles.css");
}

static esp_err_t handle_save_config(httpd_req_t *req) {
    char buf[1024];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        ESP_LOGE(TAG, "Failed to receive config data");
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid data");
    }
    buf[ret] = '\0';
    ESP_LOGI(TAG, "Received config JSON:\n%s", buf);

    cJSON *json = cJSON_Parse(buf);
    if (!json) {
        ESP_LOGE(TAG, "JSON parsing failed");
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
    }

    const char* val;
    bool ip_changed = false;

    // Network
    if ((val = cJSON_GetStringValue(cJSON_GetObjectItem(json, "ip")))) {
        ip4_addr_t new_ip = { .addr = ipaddr_addr(val) };
        if (globalConfig.deviceIp != new_ip.addr) {
            globalConfig.deviceIp = new_ip.addr;
            ip_changed = true;
        }
    }

    if ((val = cJSON_GetStringValue(cJSON_GetObjectItem(json, "gateway")))) {
        ip4_addr_t new_gw = { .addr = ipaddr_addr(val) };
        if (globalConfig.gateway != new_gw.addr) {
            globalConfig.gateway = new_gw.addr;
            ip_changed = true;
        }
    }

    if ((val = cJSON_GetStringValue(cJSON_GetObjectItem(json, "subnetMask")))) {
        ip4_addr_t new_mask = { .addr = ipaddr_addr(val) };
        if (globalConfig.subnetMask != new_mask.addr) {
            globalConfig.subnetMask = new_mask.addr;
            ip_changed = true;
        }
    }

    // Companion
    if (cJSON_HasObjectItem(json, "companionMode"))
        globalConfig.companionMode = cJSON_IsTrue(cJSON_GetObjectItem(json, "companionMode")) ? 1 : 0;
    if ((val = cJSON_GetStringValue(cJSON_GetObjectItem(json, "companionIp"))))
        globalConfig.companionIp = ipaddr_addr(val);
    if ((val = cJSON_GetStringValue(cJSON_GetObjectItem(json, "companionPort"))))
        globalConfig.companionPort = atoi(val);

    // TCP
    if (cJSON_HasObjectItem(json, "tcpEnabled"))
        globalConfig.tcpEnabled = cJSON_IsTrue(cJSON_GetObjectItem(json, "tcpEnabled")) ? 1 : 0;
    if ((val = cJSON_GetStringValue(cJSON_GetObjectItem(json, "tcpIp"))))
        globalConfig.tcpIp = ipaddr_addr(val);
    if (cJSON_HasObjectItem(json, "tcpPort"))
        globalConfig.tcpPort = cJSON_GetObjectItem(json, "tcpPort")->valueint;
    if (cJSON_HasObjectItem(json, "tcpSecure"))
        globalConfig.tcpSecure = cJSON_IsTrue(cJSON_GetObjectItem(json, "tcpSecure")) ? 1 : 0;
    if ((val = cJSON_GetStringValue(cJSON_GetObjectItem(json, "tcpUser"))))
        strncpy(globalConfig.tcpUser, val, sizeof(globalConfig.tcpUser));
    if ((val = cJSON_GetStringValue(cJSON_GetObjectItem(json, "tcpPassword"))))
        strncpy(globalConfig.tcpPassword, val, sizeof(globalConfig.tcpPassword));

    // HTTP
    if (cJSON_HasObjectItem(json, "httpEnabled"))
        globalConfig.httpEnabled = cJSON_IsTrue(cJSON_GetObjectItem(json, "httpEnabled")) ? 1 : 0;
    if ((val = cJSON_GetStringValue(cJSON_GetObjectItem(json, "httpUrl"))))
        strncpy(globalConfig.httpUrl, val, sizeof(globalConfig.httpUrl));
    if (cJSON_HasObjectItem(json, "httpSecure"))
        globalConfig.httpSecure = cJSON_IsTrue(cJSON_GetObjectItem(json, "httpSecure")) ? 1 : 0;
    if ((val = cJSON_GetStringValue(cJSON_GetObjectItem(json, "httpUser"))))
        strncpy(globalConfig.httpUser, val, sizeof(globalConfig.httpUser));
    if ((val = cJSON_GetStringValue(cJSON_GetObjectItem(json, "httpPassword"))))
        strncpy(globalConfig.httpPassword, val, sizeof(globalConfig.httpPassword));

    // Serial
    if (cJSON_HasObjectItem(json, "serialEnabled"))
        globalConfig.serialEnabled = cJSON_IsTrue(cJSON_GetObjectItem(json, "serialEnabled")) ? 1 : 0;

    // Admin password (only update if not empty)
    val = cJSON_GetStringValue(cJSON_GetObjectItem(json, "adminPassword"));
    if (val && strlen(val) > 0)
        strncpy(globalConfig.adminPassword, val, sizeof(globalConfig.adminPassword));

    cJSON_Delete(json);
    save_config();
    if (ip_changed) {reapply_eth_config();}  
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
}


esp_err_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting HTTP Server");

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root_uri = {
            .uri      = "/",
            .method   = HTTP_GET,
            .handler  = HTTP_get_router,
            .user_ctx = NULL
        };
        
        httpd_uri_t post_uri = {
            .uri      = "/",
            .method   = HTTP_POST,
            .handler  = HTTP_get_router,
            .user_ctx = NULL
        };

        httpd_uri_t js_uri = {
            .uri      = "/index.js",
            .method   = HTTP_GET,
            .handler  = serve_js_handler,  // You'll define this
            .user_ctx = NULL
        };

        httpd_uri_t css_uri = {
            .uri      = "/styles.css",
            .method   = HTTP_GET,
            .handler  = serve_css_handler,  // You'll define this
            .user_ctx = NULL
        };

        httpd_uri_t save_uri = {
            .uri      = "/save",
            .method   = HTTP_POST,
            .handler  = handle_save_config,
            .user_ctx = NULL
        };
        
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &post_uri);
        httpd_register_uri_handler(server, &js_uri);
        httpd_register_uri_handler(server, &css_uri);
        httpd_register_uri_handler(server, &save_uri);
       
        ESP_LOGI(TAG, "HTTP Server started successfully");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Failed to start HTTP server");
    return ESP_FAIL;
}