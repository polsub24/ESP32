#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Crypto.h>
#include <SHA256.h>  // rweather's Crypto library

#define DHTPIN 15
#define DHTTYPE DHT11
#define PIRPIN 14
#define TRIGPIN 12
#define ECHOPIN 13

#define RED_LED 26
#define GREEN_LED 27
#define BUTTON_PIN 25

const char* ssid = "YourWiFiName";
const char* password = "YourWiFiPassword";
const char* scriptURL = "Your Google Apps Script Web App URL";

const String locationCode = "LOC321";
const String machineNumber = "MACH01";

bool loggingPaused = false;

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(PIRPIN, INPUT);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected.");
  digitalWrite(GREEN_LED, HIGH);  // Indicate ready state
}

bool isTrayPresent(float threshold_cm = 10.0) {
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);

  long duration = pulseIn(ECHOPIN, HIGH, 30000);
  float distance = duration * 0.034 / 2.0;
  return (distance > 0 && distance < threshold_cm);
}

String generateSHA256(String input) {
  SHA256 hasher;
  hasher.reset();
  hasher.update((const uint8_t*)input.c_str(), input.length());

  uint8_t hash[32];
  hasher.finalize(hash, sizeof(hash));

  String hex = "";
  for (int i = 0; i < 32; i++) {
    if (hash[i] < 16) hex += "0";
    hex += String(hash[i], HEX);
  }
  return hex;
}

void loop() {
  // Detect motion to pause logging
  if (digitalRead(PIRPIN) == HIGH && !loggingPaused) {
    loggingPaused = true;
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    Serial.println("⚠️ Motion detected. Logging paused.");
  }

  // Wait for user to resume via button
  if (loggingPaused) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      delay(500); // debounce
      while (digitalRead(BUTTON_PIN) == LOW); // wait for release
      delay(500); // debounce
      loggingPaused = false;
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, HIGH);
      Serial.println("✅ Logging resumed by button.");
    }
    delay(100);
    return;  // Skip rest of loop
  }

  if (isTrayPresent() && digitalRead(PIRPIN) == LOW) {
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {
      Serial.println("Sensor read error.");
      return;
    }

    String data = locationCode + "|" + machineNumber + "|" + String(temp) + "|" + String(hum);
    String hash = generateSHA256(data);

    String json = "{";
    json += "\"location\":\"" + locationCode + "\",";
    json += "\"machine\":\"" + machineNumber + "\",";
    json += "\"temperature\":\"" + String(temp) + "\",";
    json += "\"humidity\":\"" + String(hum) + "\",";
    json += "\"hash\":\"" + hash + "\"}";

    Serial.println("Sending: " + json);

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(scriptURL);
      http.addHeader("Content-Type", "application/json");

      int httpCode = http.POST(json);
      String response = http.getString();

      Serial.print("HTTP Response: ");
      Serial.println(response);

      http.end();
    } else {
      Serial.println("WiFi disconnected.");
    }

  }
}

