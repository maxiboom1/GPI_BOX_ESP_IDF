# GPIO Box Project Characterization

## Overview

The GPIO Box is an ESP32-based device designed to monitor 8 GPIO input pins (GPI-1 to GPI-8) and send their state changes as structured JSON messages via user-configurable communication interfaces: Serial, TCP, and HTTP. It supports 8 GPIO output pins (GPO-1 to GPO-8) reserved for future remote triggering. A minimal web-based configuration page allows users to set device behavior, communication modes, and security options, with all configurations stored persistently in NVR. The device includes a reset button to restore factory defaults, ensuring robust operation and ease of recovery.

## General Purpose

The GPIO Box serves as a versatile, network-enabled input/output controller for IoT applications. It monitors GPIO inputs for state changes, reporting them via Serial, TCP, or HTTP based on user configuration, with optional secure authentication. It’s designed for integration into automation systems, providing a simple web interface for setup, persistent storage, and future expansion to control outputs (GPOs) remotely. The device is ideal for home automation, industrial monitoring, or custom sensor networks, focusing solely on configuration without a dashboard or status feedback.

## Features

### 1. GPIO Mapping and Trigger Logic
- **Inputs**: 8 GPIO inputs (GPI-1 to GPI-8), monitored for state changes (HIGH to LOW or LOW to HIGH).
- **Outputs**: 8 GPIO outputs (GPO-1 to GPO-8), reserved for future remote triggering via TCP, HTTP, or Serial commands.
- **Trigger**: Fires on state change, sending a message with the pin identifier, state, user, and password (e.g., `{"event": "GPI-1", "state": "HIGH", "user": "", "password": ""}`).

### 2. Communication Modes
- **Serial**: Sends JSON messages (e.g., `{"event": "GPI-1", "state": "HIGH", "user": "", "password": ""}`) via `Serial.println()` when enabled.
- **TCP**: Sends JSON messages (`{"event": "GPI-1", "state": "HIGH", "user": "xxx", "password": "yyy"}`) to a specified IP and port while maintaining a persistent connection. If the server is unreachable, it retries every 5 seconds. .
- **HTTP**: Sends POST requests with JSON in the body (e.g., `{"event": "GPI-1", "state": "HIGH", "user": "xxx", "password": "yyy"}` with `Content-Type: application/json`) to a specified URL when enabled.
- **Mode Selection**: Only enabled modes activate on GPIO trigger; multiple modes can be enabled simultaneously (e.g., TCP + HTTP). All interfaces are disabled by default.
- **Bitfocus-companion**: Enabling the device to work with companion plugin - if this mode enabled, all other modes is disabled automatically, and its configuration fields is hidden in config page.

### Integration Modes Overview

The GPIO Box supports two distinct integration modes for communicating GPIO state changes and handling remote control:

---

### 🔹 Third-Party API Mode

This mode enables standard integration with external systems using common protocols. It includes:

- **TCP**: Fire-and-forget socket connection to a user-defined server.
- **HTTP**: Sends JSON messages via POST to a specified URL.
- **Serial**: Outputs GPIO messages to UART (USB) for basic logging or serial-based integrations.

Each of these interfaces:
- Uses a unified message format generated by `MessageBuilder`.
- Can be enabled or disabled independently.
- Supports **GPI-triggered outbound messages**, and optionally **GPO-triggering inbound commands** (TCP and HTTP only).

This mode is flexible and suited for general-purpose IOT or system-level integration scenarios.

---

### 🔹 Companion Mode

This is a specialized mode designed for **tight integration with Bitfocus Companion**.

Key features:
- Uses a dedicated, persistent TCP socket.
- Handles **bi-directional communication**:
  - Sends GPIO events to Companion.
  - Receives remote GPO control commands.
  - Sends full GPIO state sync on connection.
- Disables all third-party APIs to ensure deterministic behavior and simplify configuration.

Companion Mode is optimized for control surfaces, broadcasting environments, or installations using Companion as the central automation system.

---

### 🔧 Mode Selection Logic

Only one mode can be active at a time:

- When **Companion Mode is enabled**, all third-party APIs (TCP, HTTP, Serial) are automatically disabled.
- When **Companion Mode is off**, users can selectively enable TCP, HTTP, or Serial interfaces as needed.



### 3. Security
- **Secure Mode**: Optional for TCP and HTTP.
  - If enabled, includes user and password in JSON messages; if disabled, includes empty strings for consistency.
  - **TCP**: `{"event": "GPI-1", "state": "HIGH", "user": "xxx", "password": "yyy"}`.
  - **HTTP**: POST body `{"event": "GPI-1", "state": "HIGH", "user": "xxx", "password": "yyy"}`.
- **Config Page Access**: Requires basic authentication with fixed user `admin` and configurable password (default `admin`).

### 4. Configuration Page
- **UI/UX**
  - The page implements a **two-step enable/disable mechanism**:
    1. **Feature Toggle:** When TCP/HTTP is disabled, all related fields (IP, Port, URL, Secure Mode) are disabled.
    2. **Secure Mode Toggle:** If Secure Mode is enabled, User and Password fields become editable; otherwise, they remain disabled.
  - On **page load**, the UI automatically syncs enable/disable states based on stored configuration.
  - Toggling **TCP or HTTP enables the main fields and then triggers Secure Mode handling**.
  - Style: Minimal HTML with CSS, featuring a footer (e.g., "GPIO Box v1.0").

- **Sections**:
  - **Device Address**: 
    - IP address (text input, default `10.168.0.177`).
    - Subnet mask (text input, default `255.255.255.0`).
    - Gateway (text input, default `10.168.0.1`).
  - **TCP Settings**:
    - Enabled (checkbox, off by default).
    - IP (text input, default `""`).
    - Port (text input, default `""`).
    - Secure Mode (checkbox, off by default).
    - User (text input, blank by default, enabled if Secure Mode checked).
    - Password (text input, blank by default, enabled if Secure Mode checked).
  - **HTTP Settings**:
    - Enabled (checkbox, off by default).
    - URL (text input, default `""`).
    - Secure Mode (checkbox, off by default).
    - User (text input, blank by default, enabled if Secure Mode checked).
    - Password (text input, blank by default, enabled if Secure Mode checked).
  - **Serial Settings**:
    - Enabled (checkbox, off by default).
  - **Admin Access**:
    - Login: Fixed `admin`, password (configurable, default `admin`).
    - Password change option via POST.

- **Validation**
  - The page validates all fields **before submission** to ensure correct formatting and prevent invalid data storage.
  - **Validated Fields:**
    - **Device Network:** IP, Gateway, Subnet Mask (must be valid IPv4)
    - **TCP:** IP (IPv4), Port (1-65535), Secure Mode fields (User, Password required if Secure Mode is enabled)
    - **HTTP:** URL (must be a valid format), Secure Mode fields (User, Password required if Secure Mode is enabled)
    - **Admin Password:** Must be ≤ 32 characters (only sent if changed)

- **Data Post**
  - The form **only sends TCP/HTTP settings if enabled** to prevent overwriting stored values.
  - If TCP/HTTP disabled, the `enabled` state is still sent.
  - **If Secure Mode is OFF**, TCP and HTTP only send **IP/Port and URL**.
  - **If Admin Password is empty**, it is **not sent** (preserving the current password).

- **Example Payloads:**
  - **TCP Enabled & Secure Mode OFF**:   
  ```
  {
    "tcpEnabled": true,
    "tcpIp": "10.168.0.109",
    "tcpPort": 5555
  }
  ```

  - **TCP Enabled & Secure Mode ON:**
  ```
    {
    "tcpEnabled": true,
    "tcpIp": "10.168.0.109",
    "tcpPort": 5555,
    "tcpSecure": true,
    "tcpUser": "SOME-USER2",
    "tcpPassword": "123123"
  }
  ```

  - **Admin Password Only Sent If Changed:**
  ```
    {
      "adminPassword": "newSecurePass123"
    }
  ```

### 5. Persistence and Reset
- **NVR Storage**: All configuration options stored in NVR, restored on boot.
- **Reset Button**: Physical button (GPIO 0) resets to factory defaults (e.g., all interfaces disabled, IP/URL/ports empty, admin password `admin`).

### 6. Session Authentication

- The GPIO Box implements a **simple, cookie-based session authentication** mechanism for securing the configuration page.
- Upon successful login, the device returns a `Set-Cookie` header (`sessionToken=loggedIn`) to the user's browser.
- The cookie is configured as a session cookie, automatically expiring when the browser/tab closes, or explicitly after **1 hour** (`Max-Age=3600`).
- Subsequent requests from the browser automatically include this cookie, allowing seamless navigation and page refresh without repeated login prompts.
- **No persistent session state** is maintained on the device, ensuring simplicity and minimal resource usage.

#### Session Lifecycle:
- **Login**: User submits valid credentials → Device sets a cookie in the browser.
- **Authenticated Requests**: Browser automatically sends the cookie → Device verifies and directly serves the configuration page.
- **Session Expiry**: Cookie expires automatically after 1 hour or when the browser/tab is closed.


## ESP32 Wiring and GPIO Mapping

### Hardware Configuration
- **Development Board**: ESP32-WROOM-32 (30-pin layout). (https://lastminuteengineers.com/esp32-pinout-reference/)
- **Current Usage**:
  - **W5500 Ethernet Module**:
    - GPIO 15 (CS, chip select, defined as W5500_CS).
    - GPIO 18 (SCK, SPI clock).
    - GPIO 19 (MISO, SPI data in).
    - GPIO 23 (MOSI, SPI data out).
  - **Reset Button**: GPIO 27

### GPIO Pin Allocation

| Function           | GPIO Pin | Notes                                  |
|--------------------|----------|----------------------------------------|
| **GPI #1**         | GPIO34   | Input-only, no pull-up/down            |
| **GPI #2**         | GPIO35   | Input-only, no pull-up/down            |
| **GPI #3**         | GPIO36   | Input-only, no pull-up/down            |
| **GPI #4**         | GPIO39   | Input-only, no pull-up/down            |
| **GPI #5**         | GPIO32   | Full-featured GPIO                     |
| **GPI #6**         | GPIO33   | Full-featured GPIO                     |
| **GPI #7**         | GPIO25   | Full-featured GPIO                     |
| **GPI #8**         | GPIO26   | Full-featured GPIO                     |
| **GPO #1**         | GPIO2    | Strapping pin (safe for output use)    |
| **GPO #2**         | GPIO4    | Strapping pin                          |
| **GPO #3**         | GPIO5    | Strapping pin                          |
| **GPO #4**         | GPIO12   | Strapping pin – avoid HIGH on boot     |
| **GPO #5**         | GPIO13   | Full-featured GPIO                     |
| **GPO #6**         | GPIO14   | Full-featured GPIO                     |
| **GPO #7**         | GPIO16   | Strapping pin                          |
| **GPO #8**         | GPIO17   | Full-featured GPIO                     |
| **Reset to Factory**| GPIO27  | Factory reset pin                      |

- H/W reference: https://lastminuteengineers.com/esp32-pinout-reference
- **Note**: This is working version. The final pin mapping can be different.

## NVR memory Structure

| **Field**      | **Size (Bytes)** | **Default Value** |
| -------------- | ---------------- | ----------------- |
| Device IP      | 4                | `10.168.0.177`    |
| Gateway        | 4                | `10.168.0.1`      |
| Subnet Mask    | 4                | `255.255.255.0`   |
| Companion Mode | 1                | 0 (off)           |
| Companion Port | 2                | `9567`            |
| Companion Addr | 4                | `0.0.0.0`         |
| TCP Enabled    | 1                | 0 (off)           |
| TCP IP         | 4                | `0.0.0.0` (empty) |
| TCP Port       | 2                | 0 (empty)         |
| TCP Secure     | 1                | 0 (off)           |
| TCP User       | 32               | `""` (empty)      |
| TCP Password   | 32               | `""` (empty)      |
| HTTP Enabled   | 1                | 0 (off)           |
| HTTP URL       | 64               | `""` (empty)      |
| HTTP Secure    | 1                | 0 (off)           |
| HTTP User      | 32               | `""` (empty)      |
| HTTP Password  | 32               | `""` (empty)      |
| Serial Enabled | 1                | 0 (off)           |
| Admin Password | 32               | `"admin"`         |
| Config Flag    | 1                | 0xAA (configured) |

- **Config Flag** ensures valid config (reset to `0x00` for defaults).
- Empty IP/URL/port defaults to 0.0.0.0, `""`, or 0, requiring user configuration.
- URL length (64 bytes, offset 77-140) allows up to 64 characters (e.g., `http://10.168.0.10/api` fits, max length for URLs in NVR).

## GPO Control via TCP Commands

- **Functionality**: Control GPO pins with JSON commands over TCP.  
  - Example: `{"event":"GPO-2","state":"HIGH"}` sets GPO-2 to HIGH.  
  - Tested with relays on GPO-1 and GPO-4 via Hercules.  
- **Implementation**:  
  - Parse commands with `cJSON`.  
  - Set GPO pins using `gpio_set_level` in `gpio_handler.c`.  
- **Edge Cases**:  
  - Handles `GPO-02` and `GPO-2`.  
  - Rapid commands may concatenate (e.g., `{"event":"GPO-4","state":"LOW"}{"event":"GPO-4","state":"LOW"}`).  
- **Current Status**:  
  - Works for single/moderate-rate commands.  
  - Successfully controls relays—solid result!  
- **Known Issues**:  
  - Rapid commands concatenate in TCP buffer, risking parse errors.  
- **Planned Improvement**:  
  - Add ACK mechanism:  
    - Device sends `{"ack":"GPO-4","status":"done"}` after each command.  
    - Sender waits for ACK before next command.  
    - Ensures sequential processing, avoids concatenation with multiple GPOs (up to 5).

## Future Enhancements

- **GPO Control**: Remotely trigger GPO-1 to GPO-8 via TCP, HTTP, or Serial commands (e.g., `{"event": "GPO-1", "state": "HIGH", "user": "", "password": ""}`).
- **Error Handling**: Define behavior for unreachable servers or invalid configs.

## Change Log

### v0.18
- Added GPO control via TCP commands.  
- Implemented `trigger_gpo` in `gpio_handler.c` for relay control.  
- Tested with Hercules; handles edge cases but noted rapid command concatenation issue.

## v0.17

- Fixed gpi debouncer logic
- Implemented TCP HTTP client instead relying on http library - this still non blocking fire and forget logic, but much more efficient and and lighter in term of systems resources.

## v0.16

- gpio_handler

## v0.15

- Implemented tcp client module - shared both for companion and raw tcp. have start and stop funcs.
- Implemented handle_config_change to analyze config change - and turn on/off relevant functionalities.
- If user saved config but device network props wasn't changed - we wont trigger network reapply anymore.

## v0.142 - minor fix

- Updated readme 
- Cleared message_builder from takes values from app-config. It should receive all the message props as external args.

## v0.141 - minor fix

- Fixed js logic for companion enabled mod
- "manual block" div is hidden by default - avoids jumping on load

## v0.14
- Implemented `http_client` component for sending JSON via HTTP POST:
  - Uses `esp_http_client`
  - Posts to a hardcoded URL for now (planned to use `globalConfig.httpUrl`)
  - Clean structure with `UrlObj` parser (`host`, `port`, `path`) ready for dynamic use

- Added `parse_http_url()` utility to extract host, port, and path from config URL

- Introduced `MessageBuilder` component:
  - Generates JSON messages using `cJSON`
  - Avoids dynamic allocation (`malloc`) in final version
  - Automatically pulls `user` and `password` from `globalConfig` for simplicity

## v0.13
- Replaced hardcoded login password with real config value:
  - `handle_login()` now validates against `globalConfig.adminPassword`
  - Input is parsed safely using C string methods with null-termination protection.


## v0.12
- Added support for dynamic config saving via `/save`:
  - Configuration form now submits JSON payload.
  - Backend parses and **merges** only provided fields into `globalConfig`.
  - Preserves existing values for omitted fields (e.g., disabled TCP/HTTP blocks).

- Introduced `get_placeholder_value()` to map dynamic values for HTML injection.

- Implemented live Ethernet reconfiguration:
  - New function `reapply_eth_config()` applies static IP changes without reboot.
  - Automatically called after saving config.
  - Safely skips DHCP stop if already disabled.

## v0.11
- Implemented dynamic placeholder injection in HTML serving:
  - `serve_file()` now reads HTML files in 512-byte chunks, detects `{{placeholder}}`, and replaces them with runtime config values.
  - Introduced `get_placeholder_value()` to map placeholder names (e.g., `{{tcpIp}}`) to actual values in `globalConfig`.
  - System safely handles edge cases such as placeholder overflow or cross-chunk boundaries.

### V0.10

- Added companion ip
- Updated readme that no eeprom - we are using now NVS
- Serving now full config page from NVS (index.html styles.css index.js)
- Added routes to index.html and styles.css
- serve_file func simple receives filename, loads it from NVR, and sends with 512b chunks. No memory overload, no matter the file size.
- Filled HTML forms with placeholders {{P01}}. Now we are ready to insert data instead the placeholders, and then handling submitting from user.