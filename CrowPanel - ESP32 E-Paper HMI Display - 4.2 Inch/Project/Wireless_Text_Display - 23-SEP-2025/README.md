
# Wireless Text Display for ESP32 E-Paper HMI (4.2")

This project provides a **web-based wireless text display** for the Elecrow 4.2-inch ESP32 E-Paper HMI Display. It allows you to control and update up to 8 rows of text on the e-paper screen from any device on your WiFi network, with real-time preview and customization options.

## Features

- WiFi-enabled web server for remote control
- Up to 8 configurable text rows with selectable font sizes
- Color inversion (black-on-white or white-on-black)
- Display border toggle
- Real-time preview in the web interface
- Input validation and error handling

## Hardware

- ESP32 (tested on ESP32-S3)
- 4.2" e-paper display (GxEPD2_420_GYE042A87 or compatible)

## Getting Started

### 1. Clone or Download

Clone this repository or copy the `wireless_text_display` folder to your Arduino projects directory.

### 2. Install Dependencies

- [GxEPD2](https://github.com/ZinggJM/GxEPD2) library for e-paper display
- [ArduinoJson](https://arduinojson.org/)
- [WiFi](https://www.arduino.cc/en/Reference/WiFi)

Install these libraries via the Arduino Library Manager.

### 3. Configure WiFi

Edit `config.h` and set your WiFi SSID and password:

```cpp
const char* WIFI_SSID = "YourSSID";
const char* WIFI_PASSWORD = "YourPassword";
```

### 4. Upload to ESP32

Open `wireless_text_display.ino` in Arduino IDE, select your ESP32 board, and upload.

### 5. Access the Web Interface

After boot, the device will connect to your WiFi. The IP address will be shown on the e-paper display for a few seconds. Enter this IP in your browser to access the control panel.

## File Overview

- `wireless_text_display.ino` – Main application code
- `config.h` – Configuration for WiFi, display, and fonts
- `debug.cfg`, `debug_custom.json` – Debugging configs for OpenOCD/GDB
- `esp32s3.svd` – SVD file for ESP32-S3 (for advanced debugging)

## License

MIT License. See source files for details.

---
<p align="center">
  <span style="font-size: 1.1em; color: #FFD700; font-weight: bold;">✨ Enjoying this project? Support our work! ✨</span>
</p>
<p align="center" style="margin: 15px 0;">
  <a href="https://buymeacoffee.com/pylin" target="_blank">
    <img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me a Coffee" style="height: 40px; width: 150px;">
  </a>
</p>
<p align="center" style="margin: 15px 0;">
  <a href="https://www.youtube.com/channel/UCKKhdFV0q8CV5vWUDfiDfTw" target="_blank">
    <img src="https://img.shields.io/badge/SUBSCRIBE%20ON%20YOUTUBE-FF0000?style=for-the-badge&logo=youtube&logoColor=white" alt="Subscribe on YouTube" style="height: 40px;">
  </a>
</p>
---
