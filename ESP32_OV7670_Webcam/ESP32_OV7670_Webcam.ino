#include <WiFi.h>
#include <Wire.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Arduino.h>
#include "driver/ledc.h"

// Camera Pins
#define SIOD 21
#define SIOC 22
#define VSYNC 25
#define HREF  23
#define PCLK  27
#define XCLK 32
// D0-D7
int dataPins[8] = {15, 4, 16, 17, 5, 18, 19, 13};

// Frame size
#define WIDTH 160
#define HEIGHT 120

uint8_t frame[WIDTH * HEIGHT];

// Wi-Fi Credentials
const char* ssid = "Subham's F25 Pro 5G";
const char* password = "Sark@21Subh";

AsyncWebServer server(80);
AsyncWebSocket ws("/stream");


void writeRegister(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(0x21); // OV7670 I2C address
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void startXCLK() {
  ledcSetup(0, 10000000, 1);    // channel 0, 10 MHz, 1-bit resolution
  ledcAttachPin(XCLK, 0);       // attach XCLK to channel 0
  ledcWrite(0, 1);              // duty cycle 1 (on)
}

void configureOV7670() {
  // Minimal grayscale config (QQVGA)
  writeRegister(0x12, 0x80); // Reset
  delay(100);

  // Grayscale mode
  writeRegister(0x12, 0x20);  // QVGA + grayscale
  writeRegister(0x40, 0x10);  // Only Y (disable UV)
  writeRegister(0x11, 0x01);  // Prescaler
  // ... add more regs for better tuning if needed
}

void initCameraPins() {
  pinMode(VSYNC, INPUT);
  pinMode(HREF, INPUT);
  pinMode(PCLK, INPUT);
  for (int i = 0; i < 8; i++) {
    pinMode(dataPins[i], INPUT);
  }
}

uint8_t readByte() {
  uint8_t val = 0;
  for (int i = 0; i < 8; i++) {
    val |= digitalRead(dataPins[i]) << i;
  }
  return val;
}

void captureFrame() {
  // Wait for VSYNC low -> high
  while (!digitalRead(VSYNC));
  while (digitalRead(VSYNC));

  int i = 0;

  while (i < WIDTH * HEIGHT) {
    if (digitalRead(HREF)) {
      // Read one pixel per HREF high and PCLK rising edge
      while (!digitalRead(PCLK));
      frame[i++] = readByte();
      while (digitalRead(PCLK));
    }
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SIOD, SIOC);
  initCameraPins();
  startXCLK();
  configureOV7670();
  startXCLK();
  Serial.println("XCLK Started");

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", R"rawliteral(
    <!DOCTYPE html><html><head><title>OV7670 Stream</title></head>
    <body><canvas id='c' width='160' height='120'></canvas>
    <script>
      var ws = new WebSocket("ws://" + location.host + "/stream");
      var c = document.getElementById("c");
      var ctx = c.getContext("2d");
      var img = ctx.createImageData(160, 120);
      ws.binaryType = "arraybuffer";
      ws.onmessage = function(e) {
        let d = new Uint8Array(e.data);
        for (let i = 0; i < d.length; i++) {
          img.data[i * 4 + 0] = d[i];
          img.data[i * 4 + 1] = d[i];
          img.data[i * 4 + 2] = d[i];
          img.data[i * 4 + 3] = 255;
        }
        ctx.putImageData(img, 0, 0);
      }
    </script></body></html>
    )rawliteral");
  });

  server.begin();
}

void loop() {
  captureFrame();
  ws.binaryAll(frame, WIDTH * HEIGHT);
  delay(100);
}


