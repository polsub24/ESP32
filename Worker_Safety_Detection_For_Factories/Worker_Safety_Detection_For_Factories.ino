#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPU6050.h>
#include "DHT.h"
#include <WiFi.h>
#include <HTTPClient.h>

// Define pins
#define PIR_PIN 14
#define DHTPIN 27
#define DHTTYPE DHT11

// Sensor objects
DHT dht(DHTPIN, DHTTYPE);
Adafruit_MPU6050 mpu;

// WiFi credentials (change accordingly)
const char* ssid = "Subham's F25 Pro 5G";
const char* password = "Sark@21Subh";

// Logging server or endpoint
const char* serverName = "https://factory-monitor.onrender.com/log"; // Replace with actual server/API

// State thresholds
const float VIBRATION_THRESHOLD = 0.5;  // Adjust empirically

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  
  // Initialize sensors
  dht.begin();
  if (!mpu.begin()) {
    Serial.println("MPU6050 not detected!");
    while (1) delay(10);
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("Connected!");
}

void loop() {
  // Read sensors
  bool workerPresent = digitalRead(PIR_PIN);
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();

  sensors_event_t a, g, temp_mpu;
  mpu.getEvent(&a, &g, &temp_mpu);

  float vibrationMagnitude = sqrt(a.acceleration.x * a.acceleration.x +
                                  a.acceleration.y * a.acceleration.y +
                                  a.acceleration.z * a.acceleration.z);

  bool machineActive = (vibrationMagnitude > VIBRATION_THRESHOLD);

  // Safety logic
  String alertMessage = "None";
  if (workerPresent && !machineActive) {
    alertMessage = "Worker near inactive machine!";
  } else if (!workerPresent && machineActive) {
    alertMessage = "Machine running unattended!";
  }

  // ✅ Debug Output
  Serial.println("---------------");
  Serial.print("Temp: "); Serial.print(temp); Serial.println(" °C");
  Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");
  Serial.print("Worker present: "); Serial.println(workerPresent);
  Serial.print("Machine active: "); Serial.println(machineActive);
  Serial.print("Vibration magnitude: "); Serial.println(vibrationMagnitude);
  Serial.print("Alert: "); Serial.println(alertMessage);

  // Log or send alert
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    String postData = "{";
    postData += "\"temp\":" + String(temp) + ",";
    postData += "\"humidity\":" + String(humidity) + ",";
    postData += "\"worker_present\":" + String(workerPresent) + ",";
    postData += "\"machine_active\":" + String(machineActive) + ",";
    postData += "\"alert\":\"" + alertMessage + "\"";
    postData += "}";

    int httpResponseCode = http.POST(postData);
    Serial.print("Data sent: ");
    Serial.println(httpResponseCode);
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }

  delay(2000);
}

