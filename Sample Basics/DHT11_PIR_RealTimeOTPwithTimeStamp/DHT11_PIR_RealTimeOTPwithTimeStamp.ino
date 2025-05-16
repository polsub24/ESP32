#include <WiFi.h>

#include <WebServer.h>
#include <DHT.h>
#include <SHA256.h>  // Include the SHA256 library
#include <ArduinoJson.h>

// === Wi-Fi Credentials ===
const char* ssid = "Subham's F25 Pro 5G";
const char* password = "Sark@21Subh";

// === Pin Definitions ===
#define DHTPIN 4          // DHT11 data pin
#define DHTTYPE DHT11     // Use DHT11 sensor
#define PIRPIN 5          // PIR sensor pin

// === Web Server ===
WebServer server(80);
DHT dht(DHTPIN, DHTTYPE);

// === OTP Generator ===
String getOTP(String input) {
  SHA256 sha256;
  sha256.update(input.c_str(), input.length());  // Add input string to SHA256
  byte hash[SHA256::HASH_SIZE];
  sha256.finalize(hash, SHA256::HASH_SIZE);  // Get the resulting hash

  uint32_t otp_num = 0;
  for (int i = 0; i < 4; i++) {  // Use the first 4 bytes of the hash for the OTP
    otp_num = (otp_num << 8) | hash[i];
  }
  otp_num = abs((int)otp_num) % 100000000;  // Get an 8-digit OTP
  char otpStr[9];
  sprintf(otpStr, "%08u", otp_num);  // Format as 8-digit OTP
  return String(otpStr);
}

// === JSON Data Endpoint ===
void handleData() {
  float temp = dht.readTemperature();  // Read temperature
  float hum = dht.readHumidity();      // Read humidity
  int motion = digitalRead(PIRPIN);    // Read motion sensor
  unsigned long timestamp = millis(); // Timestamp for data generation
  String payload = String(temp, 1) + "," + String(hum, 1) + "," + String(motion) + "," + String(timestamp);
  String otp = getOTP(payload);

  StaticJsonDocument<200> json;
  json["temperature"] = temp;
  json["humidity"] = hum;
  json["motion"] = motion;
  json["otp"] = otp;

  String jsonStr;
  serializeJson(json, jsonStr);
  server.send(200, "application/json", jsonStr);
}

// === HTML Page with AJAX ===
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 OTP via Temp & Humidity</title>
  <style>
    :root {
      --bg-light: #f2f2f2;
      --card-light: white;
      --text-light: #333;
      --bg-dark: #1e1e1e;
      --card-dark: #2c2c2c;
      --text-dark: #f5f5f5;
    }

    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: var(--bg-light);
      color: var(--text-light);
      margin: 0;
      padding: 0;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      transition: background 0.3s, color 0.3s;
    }

    .card {
      background: var(--card-light);
      padding: 30px 40px;
      border-radius: 20px;
      box-shadow: 0 8px 20px rgba(0,0,0,0.2);
      max-width: 400px;
      width: 90%;
      text-align: center;
      animation: fadeIn 1s ease;
      transition: background 0.3s, color 0.3s;
    }

    .otp {
      font-size: 36px;
      font-weight: bold;
      background: #ecf0f1;
      padding: 12px 24px;
      border-radius: 12px;
      display: inline-block;
      margin-top: 10px;
    }

    .qr {
      margin: 15px auto 5px;
      width: 128px;
      height: 128px;
    }

    .loader {
      border: 4px solid #f3f3f3;
      border-top: 4px solid #3498db;
      border-radius: 50%;
      width: 24px;
      height: 24px;
      animation: spin 1s linear infinite;
      margin: 10px auto;
      display: none;
    }

    .toggle-btn {
      margin-top: 15px;
      padding: 8px 16px;
      font-size: 14px;
      background: #333;
      color: #fff;
      border: none;
      border-radius: 8px;
      cursor: pointer;
    }

    .dark body {
      background: var(--bg-dark);
      color: var(--text-dark);
    }

    .dark .card {
      background: var(--card-dark);
    }

    .dark .otp {
      background: #444;
      color: #f5f5f5;
    }

    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(20px); }
      to { opacity: 1; transform: translateY(0); }
    }

    @keyframes spin {
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
    }
  </style>
</head>
<body>
  <div class="card">
    <h2>Sensor-Based OTP</h2>
    <p><b>Temperature:</b> <span id="temp">--</span> &#8451;</p>
    <p><b>Humidity:</b> <span id="hum">--</span> %</p>
    <p><b>Motion:</b> <span id="motion">--</span></p>
    <p><b>OTP:</b> <span class="otp" id="otp">Loading...</span></p>
    <canvas class="qr" id="qrCanvas"></canvas>
    <div class="loader" id="loader"></div>
    <p style="font-size: 12px;">Updated every 2 seconds</p>
    <button class="toggle-btn" onclick="toggleTheme()">Toggle Dark Mode</button>
  </div>

  <script src="https://cdn.jsdelivr.net/npm/qrious@4.0.2/dist/qrious.min.js"></script>
  <script>
    const loader = document.getElementById("loader");
    const qr = new QRious({
      element: document.getElementById("qrCanvas"),
      size: 128,
      value: ""
    });

    function updateData() {
      loader.style.display = "block";
      fetch('/data')
        .then(response => response.json())
        .then(data => {
          document.getElementById('temp').textContent = data.temperature.toFixed(1);
          document.getElementById('hum').textContent = data.humidity.toFixed(1);
          document.getElementById('motion').textContent = data.motion ? "Detected" : "None";
          document.getElementById('otp').textContent = data.otp;
          qr.value = data.otp;  // Update QR
          loader.style.display = "none";
        })
        .catch(() => loader.style.display = "none");
    }

    setInterval(updateData, 2000);
    window.onload = () => {
      updateData();
      if (localStorage.getItem("dark") === "true") {
        document.documentElement.classList.add("dark");
      }
    };

    function toggleTheme() {
      document.documentElement.classList.toggle("dark");
      localStorage.setItem("dark", document.documentElement.classList.contains("dark"));
    }
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}



void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(PIRPIN, INPUT);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Route Handlers
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();
}

