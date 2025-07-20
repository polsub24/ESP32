# 🔐 ESP32-Based Medicine Packaging Authenticator

This project uses an **ESP32** microcontroller to securely log packaging data during medicine production. It ensures that each medicine package is traceable and verifiable by uploading tamper-evident data to a central Google Sheet in real time.



## 📦 Purpose

In sterile pharmaceutical packaging, maintaining traceability and authenticity is critical. This ESP32 module captures:

* 📍 **Location Code**
* ⚙️ **Machine Number**
* 🌡️ **Temperature**
* 💧 **Humidity**

...and securely logs the data along with a **SHA-256 hash** to ensure integrity and verify authenticity later.



## 🔧 Hardware Used

* ✅ ESP32 Dev Board
* ✅ DHT11 / DHT22 Sensor (Temperature & Humidity)
* ✅ Pushbutton (for data trigger)
* ✅ Wi-Fi (for Internet access)
* ✅ Optional: OLED display for on-device feedback



## 📡 Features

* Collects **environmental & machine** data with a single button press
* Generates a **SHA-256 hash** for secure logging
* Sends data to a **Google Sheets** endpoint via Wi-Fi
* Designed to work under **sterile packaging conditions**
* Ensures **end-to-end authenticity** of packaged medicines



## 📁 Folder Structure

```
ESP32_Firmware/
│
├── sketch.ino               # Main firmware
├── SHA256.h                 # SHA256 library used
├── credentials.h            # Wi-Fi and Google Script credentials
└── README.md
```



## 🔌 Setup Instructions

1. **Install Libraries**

   * `SHA256.h` (from [https://github.com/amosnier/sha256](https://github.com/amosnier/sha256))
   * `DHT.h` (Adafruit DHT Sensor Library)

2. **Configure Wi-Fi**

   * Create a `credentials.h` file:

     ```cpp
     #define WIFI_SSID "YourWiFiName"
     #define WIFI_PASSWORD "YourWiFiPassword"
     #define GOOGLE_SCRIPT_URL "Your Google Apps Script Web App URL"
     ```

3. **Upload to ESP32**

   * Use Arduino IDE or PlatformIO.
   * Select the correct COM port and ESP32 board.

4. **Trigger Logging**

   * Press the button to capture and log a new entry.



## 🌐 Example Google Sheet Entry

| Location | Machine | Temp | Humidity | SHA256 Hash  |
| -------- | ------- | ---- | -------- | ------------ |
| A1       | M2      | 24.5 | 60       | `9f8e...a0b` |



## 📌 Applications

* ✅ Medicine packaging line traceability
* ✅ Anti-counterfeiting in pharmaceuticals
* ✅ Real-time factory floor audit trail
* ✅ Any use-case requiring hash-based verification of IoT sensor data



## 📞 Contact

For queries or collaboration:
📧 Subham Sarkar
🔗 [LinkedIn](https://linkedin.com/in/subhamsarkar06)
