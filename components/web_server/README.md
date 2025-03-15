# Web Server Module - ESP32 GPIO Box

## Overview
This module implements a lightweight HTTP server for the ESP32-based GPIO Box. It handles:
- Authentication via session-based cookies
- Serving dynamic HTML pages from SPIFFS
- Placeholder-based content injection for dynamic values

The web server allows users to configure the device settings via a browser interface while ensuring secure access control.

## Flow
1. **Server Initialization**
   - `start_webserver()` initializes the HTTP server and registers handlers.
   - Two endpoints (`GET /` and `POST /`) are handled by `HTTP_get_router()`.

2. **Routing & Authentication**
   - On **GET `/`**: 
     - If a valid session cookie exists, the **configuration page** is served.
     - Otherwise, the **login page** is served.
   - On **POST `/`**:
     - Credentials are validated in `handle_login()`.
     - If successful, a session cookie is set, and the user is redirected to `/`.

3. **Serving HTML Files with Dynamic Content**
   - `serve_file()` loads HTML from SPIFFS in **512-byte chunks**.
   - It scans for placeholders (`{{P:XX}}`) and replaces them with **preloaded config values**.
   - The modified content is streamed to the browser efficiently.

## Flow Diagram
```plaintext
start_webserver()
    ├── Registers HTTP handlers
    ├── HTTP_get_router(req)
        ├── If session cookie exists → serve_file("config.html")
        ├── If no session cookie → serve_file("login.html")
        ├── If POST → handle_login(req)
            ├── Validate credentials
            ├── If correct → Set session cookie, redirect to `/`
            ├── If incorrect → serve_file("login.html") with error message
    ├── serve_file(req, filename)
        ├── Reads file in 512-byte chunks from SPIFFS
        ├── Processes placeholders ({{P:XX}} → actual values)
        ├── Streams modified content to the browser
```

## File Serving & Placeholder Replacement
1. **Preloading Configuration Values**
   - On startup, values are loaded into a `placeholder_map[]`.
   - This ensures **fast lookups** during file processing.

2. **Chunked File Processing**
   - Reads **512-byte chunks** from SPIFFS to conserve RAM.
   - Uses a **small buffer** (`prev_chunk_buffer[]`) to handle placeholders split across chunks.

3. **Placeholder Replacement Logic**
   - Searches for `{{P:XX}}` format inside each chunk.
   - Fetches the corresponding value from `placeholder_map[]`.
   - Streams the modified chunk back to the client.

## Summary
- **Memory-efficient HTTP server** with session-based authentication.
- **Dynamic file serving** from SPIFFS with placeholder replacement.
- **Handles large HTML files** without loading the entire file into RAM.

This module ensures a robust and scalable web interface for the ESP32 GPIO Box.

