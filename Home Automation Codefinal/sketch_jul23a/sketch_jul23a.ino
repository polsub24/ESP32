#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>

// Pin Definitions
#define buzzerPin 33
#define ledPin_red 16
#define ledPin_green 2
#define dhtPin 26
#define DHT_TYPE DHT11
#define SS_PIN 21
#define RST_PIN 22
#define mqPin 15
#define fanPin 32
#define servoPin 34

// Object Instantiations
MFRC522 rfid(SS_PIN, RST_PIN);
Servo servo;
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(dhtPin, DHT_TYPE);

// Variables
unsigned long previousMillis = 0;
int displayState = 0;
byte authorizedUID[4] = {0xD3, 0x3E, 0xEF, 0xA0};  // Authorized RFID tag UID
bool doorOpen = false;
String doorStatus = "Closed";

void setup() {
  Serial.begin(115200);

  SPI.begin();
  rfid.PCD_Init();
  servo.attach(servoPin);
  dht.begin();

  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin_red, OUTPUT);
  pinMode(ledPin_green, OUTPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(mqPin, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Night Hack");
  lcd.setCursor(0, 1);
  lcd.print("IEEE CAS");
  delay(3000);
  lcd.clear();

  servo.write(0);  // Initial door position
  Serial.println("Place authorized RFID tag to open the door.");
}

void loop() {
  //Show message on LCD
  lcd.setCursor(0, 0);
  lcd.print("Place         ");
  lcd.setCursor(0, 1);
  lcd.print("RFID Key      ");

  // Wait for RFID tag
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    // delay(100);
    return;
  }

  // Check UID
  bool authorized = true;
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != authorizedUID[i]) {
      authorized = false;
      break;
    }
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reading...");

  if (authorized) {
    if (!doorOpen) {
      Serial.println("Access Granted: Door opened!");
      digitalWrite(buzzerPin,LOW);
      digitalWrite(ledPin_green,HIGH);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("DOOR");
      lcd.setCursor(0, 1);
      lcd.print("OPEN");
      servo.write(90);
      doorOpen = true;
      doorStatus = "Open";

      digitalWrite(fanPin, HIGH);

      int airAnalog = analogRead(mqPin);

      if (airAnalog <= 2500) {
        digitalWrite(ledPin_red, LOW);
        digitalWrite(ledPin_green, HIGH);
        digitalWrite(buzzerPin, LOW);
      } else {
        digitalWrite(ledPin_red, HIGH);
        digitalWrite(ledPin_green, LOW);
        digitalWrite(buzzerPin, HIGH);
      }

      float h = dht.readHumidity();
      float t = dht.readTemperature();
      float roundedTemp = round(t * 10) / 10.0;

      if (isnan(h) || isnan(t)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
      }

      Serial.print(F("Humidity: "));
      Serial.print(h);
      Serial.print(F("%  Temperature: "));
      Serial.print(roundedTemp);
      Serial.println(F("°C"));

      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= 3000) {
         previousMillis = currentMillis;
         // lcd.clear();
         if (displayState == 0) {
           lcd.setCursor(0, 0);
           lcd.print("AIR:");
           lcd.print(airAnalog);

           lcd.setCursor(0, 1);
           lcd.print("T:");
           lcd.print(roundedTemp, 1);
           lcd.print("C H:");
           lcd.print(h, 0);
           lcd.print("%");

           displayState = 1;
         } else {
           lcd.setCursor(0, 0);
           lcd.print("DOOR STATUS:");
           lcd.setCursor(0, 1);
           lcd.print(doorStatus);
           displayState = 0;
         }
       }

    } else {
      Serial.println("Door already open.");
    }

  } else {
    Serial.println("Access Denied: Unauthorized tag.");
    digitalWrite(ledPin_red,HIGH);
    digitalWrite(buzzerPin,HIGH);
    digitalWrite(ledPin_green,LOW);
    digitalWrite(fanPin, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Denied");
  }
}
