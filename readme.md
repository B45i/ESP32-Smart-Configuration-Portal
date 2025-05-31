# 🔧 ESP32 Smart Configuration Portal

[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32) [![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-green.svg)](https://www.arduino.cc/) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A robust and user-friendly ESP32 configuration system with a beautiful web portal interface. Perfect as a base template for IoT projects requiring initial setup and configuration.

## 🌟 Features

- 📱 Beautiful responsive web interface
- 🔐 Secure WiFi configuration
- 💾 Persistent configuration storage using LittleFS
- 🔄 Automatic fallback to setup mode
- 🎯 Physical trigger pin for forced configuration mode
- ⚡ Async web server for better performance
- 🌐 DNS Captive Portal for seamless setup

## 🛠 How It Works

1. **Initial Boot**

   - System checks if configuration exists in LittleFS
   - Monitors trigger pin (GPIO 36) for forced configuration mode

2. **Configuration Mode** 🔨

   - Activates when:
     - No existing configuration found
     - Trigger pin (GPIO 36) is pulled LOW
     - Previous WiFi connection fails
   - Creates an Access Point named "ESP32-Setup"
   - Starts DNS server for captive portal functionality
   - Automatically opens the configuration portal (like a hotel WiFi page)
   - Provides an intuitive interface for WiFi setup
   - All web requests are redirected to the setup portal until configured

3. **Normal Operation** ✨

   - Connects to configured WiFi network
   - Runs the web server in normal operation mode
   - Maintains configuration persistence

4. **Web Portal Features** 🎨
   - Modern, responsive design
   - Real-time form validation
   - Clear success/error feedback
   - Auto-hiding success messages
   - Loading indicators for better UX

## 🚀 Getting Started

1. **Hardware Requirements**

   - ESP32 development board
   - Connection to GPIO 36 (trigger pin)

2. **Software Requirements**

   - Arduino IDE
   - Required Libraries:
     - WiFi
     - ESPAsyncWebServer
     - LittleFS
     - ArduinoJson
     - DNSServer

3. **Installation**

   ```bash
   1. Clone this repository
   2. Open firmware.ino in Arduino IDE
   3. Install required libraries
   4. Upload to your ESP32
   ```

4. **Initial Setup**
   - Power on your ESP32
   - Connect to "ESP32-Setup" WiFi network
   - Configuration portal will open automatically, if not go to `http://192.168.4.1`
   - Enter your WiFi credentials
   - Save and wait for device restart

## 📁 Project Structure

```
├── firmware.ino          # Main ESP32 firmware with configuration logic
└── data/
    └── index.html       # Responsive web portal interface
```

## 🔧 Configuration Options

- **WiFi Settings**
  - SSID
  - Password
  - Additional server settings (if needed)

## 🏷️ Tags

`#esp32` `#iot` `#wifi` `#arduino` `#configuration` `#webportal` `#async` `#setup`
