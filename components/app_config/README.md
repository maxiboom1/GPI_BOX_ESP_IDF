# App Config Module - ESP32 GPIO Box

## Overview
The `app_config` module handles the **storage, retrieval, and management** of device configuration settings using **NVS (Non-Volatile Storage)**. It ensures persistent configuration between reboots and allows **dynamic updates** to network and service settings.

## Flow

1. **Initialization**
   - `init_config()` initializes NVS storage.
   - If NVS is unformatted or corrupt, it **erases and resets** storage.

2. **Loading Configuration**
   - `load_config()` fetches the stored configuration from NVS.
   - If no valid config exists, `set_default_config()` applies **factory defaults** and saves them.

3. **Saving Configuration**
   - `save_config()` updates NVS with the latest settings.
   - Ensures **all changes are persisted** across reboots.

## Global Config Object
The module maintains a **global configuration object**:

```c
extern AppConfig globalConfig;
```
- `globalConfig` is accessible across the entire application, allowing modules to read configuration values dynamically.
- It is automatically loaded at startup and synchronized with NVS whenever `save_config()` is called.


## Functionality

| **Function**          | **Description** |
|----------------------|----------------|
| `init_config()`      | Initializes the NVS storage. |
| `load_config()`      | Loads the configuration from NVS or applies defaults. |
| `save_config()`      | Saves the updated configuration to NVS. |
| `set_default_config()` | Resets the configuration to factory defaults. |

## Summary
- **Persistent storage** of network and system settings.
- **Factory reset support** for default values.
- **Ensures data integrity** using NVS commit operations.

This module provides a **centralized and reliable configuration system** for the ESP32 GPIO Box.
