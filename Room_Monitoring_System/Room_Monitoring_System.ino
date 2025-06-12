#define ENABLE_USER_AUTH
#define ENABLE_DATABASE


#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <DHT.h>

// Wi-Fi credentials
#define WIFI_SSID "Subham's F25 Pro 5G"
#define WIFI_PASSWORD ""

// Firebase credentials
#define API_KEY "Refer to env file before use"       
#define USER_EMAIL "Refer to env file before use"    
#define USER_PASSWORD "Refer to env file before use" 
#define DATABASE_URL "Refer to env file before use"

// Sensor Pins
#define DHTPIN 14
#define DHTTYPE DHT11
#define FLAME_SENSOR_PIN 27
#define LDR_SENSOR_PIN 35
#define MQ135_PIN 34

DHT dht(DHTPIN, DHTTYPE);


// Firebase Objects
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);
FirebaseApp app;

WiFiClientSecure auth_client;
AsyncClientClass async_auth(auth_client);

WiFiClientSecure db_client;
AsyncClientClass async_db(db_client);

RealtimeDatabase Database;

// Firebase Response Handler
void processData(AsyncResult &aResult)
{
  if (!aResult.isResult())
  {
    Serial.println("[processData] No result available.");
    return;
  }


  if (aResult.isError())
  {
    Serial.printf("[processData] Error (%s): %s\n", aResult.uid().c_str(), aResult.error().message().c_str());
  }
  else if (aResult.available())
  {
    Serial.printf("[processData] Success (%s): %s\n", aResult.uid().c_str(), aResult.c_str());
  }
}
// Timing variables
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 3000; // 3 seconds


void setup()
{
  Serial.begin(115200);
  delay(1000);
  dht.begin();


  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(LDR_SENSOR_PIN, INPUT);
  pinMode(MQ135_PIN, INPUT);


  Serial.println("=== Setup Start ===");


  // Wi-Fi Setup
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);


  Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);


  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30)
  {
    Serial.print(".");
    delay(500);
    attempts++;
  }
  Serial.println();


  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[WiFi] Failed to connect!");
    while (true)
      ; // Halt
  }


  Serial.print("[WiFi] Connected! IP: ");
  Serial.println(WiFi.localIP());


  // SSL Setup
  auth_client.setInsecure(); //to override assignment of SSL certificate as not production ready, for it we hve to use one og github, google etc
  db_client.setInsecure(); //to override assignment of SSL certificate as not production ready, for it we hve to use one og github, google etc


  // Firebase App Init
  Serial.println("[Firebase] Initializing app...");
  initializeApp(async_auth, app, getAuth(user_auth), processData, "AuthTask");


  // 🔁 Wait until app is ready
  Serial.println("[Firebase] Waiting for app to be ready...");
  while (!app.ready())
  {
    Serial.print(".");
    app.loop();
    delay(100);
  }


  Serial.println("\n[Firebase] App is ready!");


  // Bind Database
  Serial.println("[Firebase] Binding Realtime Database...");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
}

void loop()
{
  app.loop(); // must be called continuously


  unsigned long currentTime = millis();


  if (app.ready() && currentTime - lastSendTime >= sendInterval)
  {
    lastSendTime = currentTime;
    Serial.println("[loop] Reading sensors and sending data...");


    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    int flame = digitalRead(FLAME_SENSOR_PIN);
    int ldr = digitalRead(LDR_SENSOR_PIN);
    int gasValue = analogRead(MQ135_PIN);


    if (isnan(temp) || isnan(hum))
    {
      Serial.println("[DHT] Failed to read from sensor!");
      return;
    }


    Serial.printf("[DHT] Temp: %.2f °C, Humidity: %.2f %%\n", temp, hum);
    Serial.printf("[Flame] Value: %d\n", flame);
    Serial.printf("[LDR] Value: %d\n", ldr);
    Serial.printf("[Gas MQ135] Analog Value: %d\n", gasValue);


    // Upload to Firebase
    Database.set<float>(async_db, "/iot/device/temperature", temp, processData, "SetTemp");
    Database.set<float>(async_db, "/iot/device/humidity", hum, processData, "SetHumidity");
    String flameStatus = (flame == 1) ? "Flame" : "No flame :D";
    Database.set<String>(async_db, "/iot/device/flame", flameStatus, processData, "SetFlame");
    String lightStatus = (ldr == 1) ? "Light" : "Dark";
    Database.set<String>(async_db, "/iot/device/ldr", lightStatus, processData, "SetLDR");
    Database.set<int>(async_db, "/iot/device/gas", gasValue, processData, "SetGas");
  }
  delay(50);
}

