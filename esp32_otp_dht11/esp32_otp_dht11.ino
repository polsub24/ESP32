#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

#define DHTPIN 4          // DHT11 connected to GPIO 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

// WiFi Credentials
const char* ssid = "Subham's F25 Pro 5G";
const char* password = "Sark@21Subh";

// Generate 4-digit OTP using DHT11 temperature and timestamp
String generateOTP() {
  float temp = dht.readTemperature();

  if (isnan(temp)) {
    Serial.println("❌ DHT read failed. Using fallback temp = 25.0");
    temp = 25.0; // fallback temperature
  } else {
    Serial.print("🌡️ Temperature: ");
    Serial.println(temp);
  }

  unsigned long timeNow = millis() / 1000;
  int rawOTP = abs((int)(temp * 100 + timeNow)) % 10000;

  if (rawOTP < 1000) rawOTP += 1000;  // ensure 4-digit OTP
  return String(rawOTP);
}

// Handle HTTP request for /otp
void handleOtp() {
  String otp = generateOTP();
  String json = "{\"otp\":\"" + otp + "\"}";

  server.sendHeader("Access-Control-Allow-Origin", "*"); // enable CORS
  server.send(200, "application/json", json);

  Serial.println("✅ OTP sent: " + otp);
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("🔄 Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ Connected to WiFi!");
  Serial.print("📡 IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/otp", handleOtp);
  server.begin();
  Serial.println("🚀 HTTP server started");
}

void loop() {
  server.handleClient();
}

