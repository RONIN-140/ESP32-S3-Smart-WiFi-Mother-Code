# ESP32-S3 Smart Mother Code Template 🤖✨

This is a production-ready template for the **ESP32-S3-WROOM (Dual Type-C)** and **0.96" SSD1306 OLED Display**. It runs your project code smoothly while handling Wi-Fi network shifting and OTA in the background.

## 🚀 Features
- **Smart Wi-Fi Manager:** Automatically starts a Captive Portal Webpage (`ESP32-S3-Setup`) if no saved Wi-Fi is found. Scans nearby networks smoothly.
- **Wireless Programming (OTA):** Upload your sketch from ArduinoDroid/Arduino IDE without cables over Wi-Fi.
- **Dynamic Face Expressions:** 
  - 😊 *Happy Eyes:* When Wi-Fi is successfully connected (Shows IP for 5 seconds).
  - 🔄 *Loading Eyes:* When a new code is uploading wirelessly.
  - 👁️ *Blinking Eyes:* Standard cute blinking mode when offline.

## 🔌 Pin Connections
- **SDA** ➔ GPIO 8
- **SCL** ➔ GPIO 9
- **VCC** ➔ 3.3V
- **GND** ➔ GND
- 
