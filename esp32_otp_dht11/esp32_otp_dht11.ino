#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

#define DHTPIN 4          // DHT11 connected to GPIO 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

// WiFi Credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Generate 4-digit OTP using DHT11 and timestamp
String generateOTP() {
  float temp = dht.readTemperature();
  if (isnan(temp)) {
    temp = 25.0; // fallback if sensor fails
  }

  unsigned long timeNow = millis() / 1000;
  int rawOTP = abs((int)(temp * 100 + timeNow)) % 10000;
  if (rawOTP < 1000) rawOTP += 1000;  // ensure 4-digit format
  return String(rawOTP);
}

// Handle HTTP request for /otp
void handleOtp() {
  String otp = generateOTP();
  String json = "{\"otp\":\"" + otp + "\"}";
  server.send(200, "application/json", json);
  Serial.println("OTP sent: " + otp);
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/otp", handleOtp);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
