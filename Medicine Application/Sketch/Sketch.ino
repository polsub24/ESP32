#include <DHT.h>
#include "FS.h"
#include "SPIFFS.h"
#include <SHA256.h>  // From rweather's Arduino Crypto library

// Pin Assignments
#define DHTPIN 15
#define DHTTYPE DHT11
#define PIRPIN 14
#define TRIGPIN 12
#define ECHOPIN 13

DHT dht(DHTPIN, DHTTYPE);

// Metadata
const String locationCode = "LOC123";
const String machineNumber = "MACH05";
const char* logFile = "/logs.csv";

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(PIRPIN, INPUT);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed.");
    while (true);
  }

  Serial.println("System Initialized.");
}

// Detect tray presence with HC-SR04
bool isTrayPresent(float threshold_cm = 10.0) {
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);

  long duration = pulseIn(ECHOPIN, HIGH, 30000);  // 30 ms timeout
  float distance = duration * 0.034 / 2.0;
  return (distance > 0 && distance < threshold_cm);
}

// Generate SHA256 hash using Crypto lib
String generateSHA256(String input) {
  SHA256 hasher;
  hasher.reset();
  hasher.update((const uint8_t*)input.c_str(), input.length());

  uint8_t hash[32];
  hasher.finalize(hash, sizeof(hash));  // Corrected method name

  String hex = "";
  for (int i = 0; i < 32; i++) {
    if (hash[i] < 16) hex += "0";
    hex += String(hash[i], HEX);
  }
  return hex;
}


// Save to /logs.csv
void logToFile(String logLine) {
  File file = SPIFFS.open(logFile, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open log file.");
    return;
  }
  file.println(logLine);
  file.close();
}

void printLogFile() {
  File file = SPIFFS.open(logFile, FILE_READ);
  if (!file) {
    Serial.println("Error: Cannot open /logs.csv");
    return;
  }

  Serial.println("----- BEGIN LOG EXPORT -----");
  while (file.available()) {
    Serial.write(file.read());
  }
  Serial.println("----- END LOG EXPORT -----");
  file.close();
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "EXPORT") {
      printLogFile();
      return;
    }
  }

  if (isTrayPresent() && digitalRead(PIRPIN) == LOW) {
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {
      Serial.println("Sensor error.");
      return;
    }

    String data = locationCode + "|" + machineNumber + "|" + String(temp) + "|" + String(hum);
    String hash = generateSHA256(data);
    String logEntry = String(millis()) + "," + locationCode + "," + machineNumber + "," + String(temp) + "," + String(hum) + "," + hash;

    logToFile(logEntry);
    Serial.println("Logged: " + hash);

    delay(5000);
  }
}


