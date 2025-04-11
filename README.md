# GPIO Box – ESP32 Automation Module

## Overview

GPIO Box is an ESP32-based automation controller that:

- Monitors 8 GPIO inputs for state changes

- Sends real-time events via TCP, HTTP, or Serial

- Receives remote commands to control 5 GPO outputs

- Provides a web-based configuration interface with NVS persistence

- Supports two modes: standard 3rd-party integration or Bitfocus Companion bi-directional control

- It is ideal for automation, broadcast control, or sensor relay setups.

## Features

- 8 digital inputs (GPI-1 to GPI-8), monitored with debounce and state change detection
- 5 digital outputs (GPO-1 to GPO-5), remotely controllable via TCP commands
- Two operational modes:
  - **Companion Mode**: bi-directional sync/control with Bitfocus Companion
  - **API Mode**: standard TCP/HTTP/Serial output with optional secure credentials
- Built-in Ethernet (W5500) with static IP from configuration
- Web-based configuration interface (served via SPIFFS)
- Configuration stored in NVS and restored on boot
- Factory reset button (GPIO 0) restores defaults

## Integration Modes

### 🔹 Companion Mode (default)

Optimized for Bitfocus Companion. Enables tight, bi-directional control:

- Uses a persistent TCP connection
- Sends GPI events as JSON
- Receives GPO control commands (e.g. `{"event":"GPO-1", "state":"HIGH"}`)
- Supports full sync request from Companion:
  - `{"event": "sync", "state": "request"}`
  - Responds with current GPI + GPO states:
    ```json
    {
      "event": "sync-response",
      "gpi": {"GPI-1": "LOW", "GPI-2": "HIGH", ...},
      "gpo": {"GPO-1": "HIGH", "GPO-2": "LOW", ...}
    }
    ```

🔒 When Companion Mode is enabled:
- TCP, HTTP, and Serial interfaces are automatically disabled
- Only one mode can be active at a time
- Requires no additional configuration on the Companion side except IP and port

### 🔹 API Mode (TCP / HTTP / Serial)

Flexible integration for third-party systems.

- **TCP**: Sends GPI events as JSON via persistent socket
- **HTTP**: Sends GPI events via POST to a configured URL
- **Serial**: Sends JSON to UART (USB) (for logging/integration)

Features:
- All interfaces are optional and independently toggleable
- TCP/HTTP can optionally include username/password if Secure Mode is enabled
- Only GPI → outbound messages supported (no inbound GPO control in API mode, for now)

## Communication Protocol

### GPI Event Format (All Modes)

Every input change triggers a message in this format:
```json
{
  "event": "GPI-1",
  "state": "HIGH",
  "user": "optional",
  "password": "optional"
}
```
- Sent over TCP, HTTP, or Serial depending on enabled modes

- Credentials are only included if Secure Mode is enabled

### GPO Control (Companion Mode Only)

Remote control via JSON over TCP:
```json
{ "event": "GPO-3", "state": "LOW" }
```
- Supports GPO-1 to GPO-5

- Case-insensitive, accepts formats like "GPO-02"

### Full Sync (Companion Mode)

Request full GPIO state from the device:
```json
{ "event": "sync", "state": "request" }

```

Response format:
```json
{
  "event": "sync-response",
  "gpi": {
    "GPI-1": "HIGH", "GPI-2": "LOW", ...
  },
  "gpo": {
    "GPO-1": "LOW", "GPO-2": "HIGH", ...
  }
}
```
- Reflects actual configured input/output count


## Web Configuration Interface

Accessible via browser at the device's IP address.

### 🔐 Login
- Username: `admin` (fixed)
- Password: configurable (default: `admin`)
- Uses cookie-based session (expires on browser close or after 1 hour)

---

### 🛠 Configurable Options

- **Network Settings**:  
  - IP address, subnet mask, gateway (Ethernet only)

- **Companion Mode**:  
  - Enable/Disable
  - IP + Port for Companion connection

- **TCP Settings**:
  - Enable/Disable
  - IP, Port
  - Secure Mode (optional user/password)

- **HTTP Settings**:
  - Enable/Disable
  - URL
  - Secure Mode (optional user/password)

- **Serial Output**:
  - Enable/Disable

- **Admin Password**:
  - Can be changed at any time
  - Not submitted if left empty

---

### ⚙️ Validation & Submission

- All fields validated client-side before submission:
  - IPs, URLs, ports, credentials, etc.
- Only **enabled blocks** send values
- Configuration is **merged** with existing values (no full overwrite)
- Network IP changes apply immediately (no reboot required)


## GPIO Mapping

### Inputs (GPI)

| Label   | Pin       |
|---------|-----------|
| GPI-1   | GPIO 32   |
| GPI-2   | GPIO 33   |
| GPI-3   | GPIO 25   |
| GPI-4   | GPIO 26   |
| GPI-5   | GPIO 27   |
| GPI-6   | GPIO 14   |
| GPI-7   | GPIO 12   |
| GPI-8   | GPIO 13   |

> All inputs are debounced and pulled-up. State change triggers event.

---

### Outputs (GPO)

| Label   | Pin       |
|---------|-----------|
| GPO-1   | GPIO 16   |
| GPO-2   | GPIO 17   |
| GPO-3   | GPIO 21   |
| GPO-4   | GPIO 22   |
| GPO-5   | GPIO 4    |

> Output state is cached and included in sync-response.

---

### Other Pins

| Function     | Pin       |
|--------------|-----------|
| Reset Button | GPIO 0    |
| W5500 CS     | GPIO 15   |
| W5500 SCK    | GPIO 18   |
| W5500 MISO   | GPIO 19   |
| W5500 MOSI   | GPIO 23   |



## Configuration Storage & Reset

- All settings are saved to **ESP32 NVS (non-volatile storage)**
- Automatically loaded at boot
- If no config is found, default settings are applied

---

### Factory Reset

- Hold **GPIO 0** (Reset Button) for 5 seconds
- Restores:
  - Default IP: `10.168.0.177`
  - All modes disabled
  - Admin password reset to `"admin"`
  - TCP/HTTP credentials cleared













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

## Future Enhancements

- **ACK System**: Optional confirmation for received TCP commands to prevent command overlap (especially in rapid sequences).
- **Improved Frontend UX**: Apply visual polishing to the configuration page (grouping, spacing, loading indicator, better feedback).
- **Extended Secure Mode**: Add backend validation for credentials (currently only sent blindly).
- **Wi-Fi Support**: Option to switch from Ethernet to Wi-Fi configuration via web UI.
- **GPO Control via API Mode**: Allow inbound control in HTTP/Serial modes, not just Companion.
- **Validation Recheck**: Refactor and simplify JavaScript validation logic to improve maintainability and error accuracy.
- **Sync-on-Connect (optional)**: In Companion mode, auto-send sync-response once the connection is established (not just on request).
- **Modular Configuration Loader**: Improve code modularity by breaking config logic into independent field handlers.

## Change Log

### v1.00
- Finalized full GPIO handling (input + output)
- Implemented Companion Mode:
  - Bi-directional sync via TCP
  - Full-state response to sync requests
- Merged all communication logic into modular system
- Updated web interface with dynamic config merging and live validation
- Finalized NVS config persistence and live IP reapply without reboot

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