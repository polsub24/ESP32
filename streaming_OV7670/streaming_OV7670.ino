#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#include <Wire.h>
#include <OV7670.h>
#include <I2SCamera.h>

// ===================== WiFi CONFIG =====================
const char *ap_ssid     = "OV7670_ESP32";
const char *ap_password = "12345678";     // can be "" if you want open AP

// ===================== CAMERA PINOUT ===================
// Adjust to match your actual wiring
#define SIOD 21   // SDA
#define SIOC 22   // SCL
#define VSYNC 25
#define HREF  23
#define PCLK  27
#define XCLK  32

#define D0    15
#define D1    4
#define D2    16
#define D3    17
#define D4    5
#define D5    18
#define D6    19
#define D7    13  // make sure you wired D7 here or change this define

// ===================== GLOBALS =========================
WebServer server(80);
WebSocketsServer webSocket(81);

OV7670 *camera = nullptr;

// Flags used in WebSocket stream protocol
static const uint8_t FRAME_START_FLAG = 0xA0;
static const uint8_t FRAME_END_FLAG   = 0xAF;

// HTML page served by ESP32
// Raw string literal to avoid escaping
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8" />
<title>OV7670 Live Stream (ESP32)</title>
<style>
  body {
    background: #111;
    color: #eee;
    font-family: sans-serif;
    text-align: center;
  }
  #videoCanvas {
    background: #000;
    image-rendering: pixelated;
    margin-top: 20px;
    border: 2px solid #444;
  }
  #status {
    margin-top: 10px;
    font-size: 14px;
  }
</style>
</head>
<body>
<h2>OV7670 Live Stream (ESP32)</h2>
<canvas id="videoCanvas" width="160" height="120"></canvas>
<div id="status">Connecting...</div>

<script>
const canvas = document.getElementById('videoCanvas');
const ctx = canvas.getContext('2d');
const statusEl = document.getElementById('status');

const width = canvas.width;
const height = canvas.height;
const expectedPixels = width * height;

// We'll accumulate binary chunks between FRAME_START and FRAME_END
let frameBuffer = [];
let collecting = false;
let socket;

function connectWS() {
  const wsUrl = 'ws://' + window.location.hostname + ':81/';
  socket = new WebSocket(wsUrl);
  socket.binaryType = 'arraybuffer';

  socket.onopen = () => {
    statusEl.textContent = 'WebSocket connected. Requesting frames...';
    // Tell ESP32 we want QQVGA (160x120) mode
    socket.send('QQVGA');
    requestNextFrame();
  };

  socket.onclose = () => {
    statusEl.textContent = 'WebSocket disconnected. Reconnecting in 2s...';
    setTimeout(connectWS, 2000);
  };

  socket.onerror = (e) => {
    console.error(e);
  };

  socket.onmessage = (event) => {
    if (typeof event.data === 'string') {
      // text control messages (if any)
      // console.log('TEXT:', event.data);
      return;
    }

    const data = new Uint8Array(event.data);
    if (data.length === 1 && data[0] === 0xA0) {
      // frame start
      frameBuffer = [];
      collecting = true;
      return;
    } else if (data.length === 1 && data[0] === 0xAF) {
      // frame end
      collecting = false;
      renderFrame();
      requestNextFrame();  // immediately request the next one
      return;
    }

    if (collecting) {
      frameBuffer.push(data);
    }
  };
}

function requestNextFrame() {
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send('FRAME');
  }
}

function renderFrame() {
  // Concatenate all chunks
  let totalLength = 0;
  for (let i = 0; i < frameBuffer.length; i++) {
    totalLength += frameBuffer[i].length;
  }
  const rgb565 = new Uint8Array(totalLength);
  let offset = 0;
  for (let i = 0; i < frameBuffer.length; i++) {
    rgb565.set(frameBuffer[i], offset);
    offset += frameBuffer[i].length;
  }

  const numPixels = rgb565.length / 2;
  if (numPixels !== expectedPixels) {
    statusEl.textContent = 'Frame size mismatch: ' + numPixels + ' px';
    console.warn('Expected', expectedPixels, 'pixels, got', numPixels);
    return;
  }

  const imgData = ctx.createImageData(width, height);
  const data = imgData.data;

  // Convert RGB565 -> RGBA8888
  let j = 0;
  for (let i = 0; i < rgb565.length; i += 2) {
    const byte1 = rgb565[i];
    const byte2 = rgb565[i + 1];
    const value = (byte1 << 8) | byte2;

    const r5 = (value >> 11) & 0x1F;
    const g6 = (value >> 5)  & 0x3F;
    const b5 =  value        & 0x1F;

    const r8 = (r5 * 255 / 31) | 0;
    const g8 = (g6 * 255 / 63) | 0;
    const b8 = (b5 * 255 / 31) | 0;

    data[j++] = r8;
    data[j++] = g8;
    data[j++] = b8;
    data[j++] = 255;  // alpha
  }

  ctx.putImageData(imgData, 0, 0);
  statusEl.textContent = 'Streaming...';
}

connectWS();
</script>
</body>
</html>
)rawliteral";

// ===================== HTTP HANDLERS ===================
void handleRoot()
{
  server.send(200, "text/html", index_html);
}

// ===================== WEBSOCKET CALLBACK ===============
void handleWebSocketMessage(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  switch (type)
  {
    case WStype_CONNECTED:
    {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[WS] Client %u connected from %s\n",
                    num, ip.toString().c_str());
      break;
    }

    case WStype_DISCONNECTED:
      Serial.printf("[WS] Client %u disconnected\n", num);
      break;

    case WStype_TEXT:
    {
      String cmd = String((char*)payload).substring(0, length);
      Serial.printf("[WS] Text from %u: %s\n", num, cmd.c_str());

      if (cmd == "QQVGA")
      {
        // Initialize camera in 160x120 RGB565 mode (using Bitluni OV7670)
        if (camera != nullptr)
        {
          delete camera;
          camera = nullptr;
        }

        camera = new OV7670(
          OV7670::Mode::QVGA_RGB565,
          SIOD, SIOC,
          VSYNC, HREF, XCLK, PCLK,
          D0, D1, D2, D3, D4, D5, D6, D7
        );

        if (!camera)
        {
          Serial.println("ERROR: Camera allocation failed");
        }
        else
        {
          Serial.println("Camera initialized in QQVGA_RGB565");
        }
      }
      else if (cmd == "FRAME")
{
  if (!camera)
  {
    Serial.println("FRAME requested but camera not ready");
    break;
  }

  // Start and end flags so the browser knows frame boundaries
  uint8_t startFlag = FRAME_START_FLAG;
  uint8_t endFlag   = FRAME_END_FLAG;

  // Notify client that a frame is starting
  webSocket.sendBIN(num, &startFlag, 1);

  // Capture a full frame into camera->frame
  camera->oneFrame();   // in most OV7670 libraries this fills full xres*yres frame

  // Send the entire frame buffer in a single WebSocket binary message
  size_t frameBytes = camera->xres * camera->yres * 2;   // RGB565 → 2 bytes per pixel
  webSocket.sendBIN(num, camera->frame, frameBytes);

  // Notify frame end
  webSocket.sendBIN(num, &endFlag, 1);
}


      break;
    }

    default:
      break;
  }
}

// WebSocket event wrapper
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  handleWebSocketMessage(num, type, payload, length);
}

// ===================== SETUP ============================
void setup()
{
  Serial.begin(115200);
  delay(2000);
  Serial.println();
  Serial.println("OV7670 ESP32 WebSocket Streamer starting...");

  // Start WiFi in AP mode (self-hosted network)
  WiFi.mode(WIFI_AP);
  bool apOK = WiFi.softAP(ap_ssid, ap_password);
  if (!apOK)
  {
    Serial.println("ERROR: Failed to start SoftAP");
  }
  else
  {
    Serial.print("AP started: ");
    Serial.println(ap_ssid);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  }

  // HTTP server
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started on port 80");

  // WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started on port 81");

  // Camera will be instantiated lazily when "QQVGA" command arrives
}

// ===================== LOOP =============================
void loop()
{
  webSocket.loop();
  server.handleClient();
}
