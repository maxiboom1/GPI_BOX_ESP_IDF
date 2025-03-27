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
- **Development Board**: ESP32-WROOM-32 (30-pin layout).
- **Current Usage**:
  - **LEDs**: GPIO 21 (SDA, I2C for 1602 LCD), GPIO 22 (SCL, I2C for 1602 LCD).
  - **W5500 Ethernet Module**:
    - GPIO 15 (CS, chip select, defined as W5500_CS).
    - GPIO 18 (SCK, SPI clock).
    - GPIO 19 (MISO, SPI data in).
    - GPIO 23 (MOSI, SPI data out).
    - GPIO 4 (RST, reset, confirmed unused after disconnection—freed for GPIO).
  - **Reset Button**: GPIO 0 (preferred, check if tied to EN; fallback GPIO 26 if needed).
- **Available GPIO Pins** (after freeing GPIO 4): GPIO 2, 5, 12, 13, 14, 16, 17, 25, 32, 33, 34, 35, 4, 26, 27 (15 pins total).
  - **RX0/TX0** (GPIO 1, 3): UART0, reserved for Serial debugging unless repurposed.
  - **VN/VP** (GPIO 36, 39): ADC-only, not usable as GPIOs.

### Assigned Pins:
| **GPI Inputs (GPI-1 to GPI-8)** | **GPO Outputs (GPO-1 to GPO-8)** |
|---------------------------------|----------------------------------|
| GPI-1: GPIO 4                   | GPO-1: GPIO 32                   |
| GPI-2: GPIO 5                   | GPO-2: GPIO 33                   |
| GPI-3: GPIO 12                  | GPO-3: GPIO 34                   |
| GPI-4: GPIO 13                  | GPO-4: GPIO 35                   |
| GPI-5: GPIO 14                  | GPO-5: GPIO 26  |
| GPI-6: GPIO 16                  | GPO-6: GPIO 27                   |
| GPI-7: GPIO 17                  | GPO-7: GPIO 0 (May conflict with reset)                  |
| GPI-8: GPIO 25                  | GPO-8: n/a |

- **Reset Button**: GPIO 0 (preferred, check if tied to EN; fallback GPIO 26).
- **Note**: This is working version. The final pin mapping can be different.

#### Notes:
- Verify GPIO 0 isn’t the EN pin (typically tied to EN on some boards, requiring pull-up). If it is, use GPIO 26 for reset.
- RX0/TX0 (GPIO 1, 3) can be repurposed later if Serial isn’t needed, but keep for debugging now.
- All GPI/GPO pins support input/output, PWM, interrupt—adequate for our needs.

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

## Future Enhancements

- **GPO Control**: Remotely trigger GPO-1 to GPO-8 via TCP, HTTP, or Serial commands (e.g., `{"event": "GPO-1", "state": "HIGH", "user": "", "password": ""}`).
- **Error Handling**: Define behavior for unreachable servers or invalid configs.

## Change Log

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