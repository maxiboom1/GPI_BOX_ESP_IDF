#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include <stdio.h>
#include <string.h>
#include "app_config.h"  
#include "lwip/ip4_addr.h"

static const char *TAG = "web_server";

const char* itoa_buf(int value) {
    static char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    return buf;
}

// Open  given file from spiffs, and send it chunked
static esp_err_t serve_file(httpd_req_t *req, const char *filename) {
    char filepath[64];
    snprintf(filepath, sizeof(filepath), "/spiffs_data/%s", filename);
    FILE *f = fopen(filepath, "r");
    if (!f) {
        ESP_LOGE("SPIFFS", "Failed to open %s", filepath);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    char buffer[512];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        esp_err_t res = httpd_resp_send_chunk(req, buffer, bytes_read);
        if (res != ESP_OK) {
            fclose(f);
            return res;
        }
    }

    fclose(f);
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
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request"); // Bad request
    }
    content[ret] = '\0';

    if (strstr(content, "user=admin") && strstr(content, "password=1234")) {
        httpd_resp_set_hdr(req, "Set-Cookie", "sessionToken=loggedIn; Path=/;");
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/");
        return httpd_resp_send(req, NULL, 0);
    }
    return serve_login_page(req, true); // Login failed
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

        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &post_uri);
        httpd_register_uri_handler(server, &js_uri);
        httpd_register_uri_handler(server, &css_uri);
        
        ESP_LOGI(TAG, "HTTP Server started successfully");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Failed to start HTTP server");
    return ESP_FAIL;
}