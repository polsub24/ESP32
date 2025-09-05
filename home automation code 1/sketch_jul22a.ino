#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>


#define buzzerPin 33
#define ledPin_red 16
#define ledPin_green 2
#define dhtPin 26
#define DHT_TYPE DHT11
// #define ldrPin 5;
// rfid pins
#define SS_PIN 21
#define RST_PIN 22

#define mqPin 15
#define fanPin 32
#define servoPin 34


MFRC522 rfid(SS_PIN, RST_PIN);
Servo servo;

unsigned long previousMillis = 0;  
int displayState = 0;
byte authorizedUID[4] = {0xD3, 0x3E, 0xEF, 0xA0};  //key uid
bool doorOpen = false;
String doorStatus ="Closed";

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(dhtPin, DHT_TYPE); 


void setup() {
  // Pin Definitions
  Serial.begin(115200);
  SPI.begin();         // Start SPI bus
  rfid.PCD_Init();     // Init RC522
  servo.attach(servoPin);

  dht.begin();
  pinMode(buzzerPin,OUTPUT);
  pinMode(ledPin_red,OUTPUT);
  pinMode(ledPin_green,OUTPUT);
  pinMode(fanPin,OUTPUT);
  pinMode(servoPin,OUTPUT);
  pinMode(dhtPin,INPUT);
//  pinMode(rfid_reader,INPUT);
  pinMode(mqPin,INPUT);

  // LCD Setup
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Night Hack");
  lcd.setCursor(0,1);
  lcd.print("IEEE CAS");
  delay(3000);
  lcd.clear();


  servo.write(0);      
  Serial.println("Place authorized RFID tag to open the door.");
}


void loop() {
// Door unlock rfid

// lcd opening msg
  lcd.setCursor(0,0);
  lcd.print("Place");
  lcd.setCursor(0,1);
  lcd.print("RFID Key");

// Look for new tags
  if ( !rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial() ) {
    delay(100);
    return;
  }

  // Check if tag UID is authorized
  bool authorized = true;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Reading...");
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != authorizedUID[i]) {
      authorized = false;
      break;
    }
  }

  if (authorized) {
    if (!doorOpen) {
      Serial.println("Access Granted: Door opened!");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("DOOR");
      lcd.setCursor(0,1);
      lcd.print("OPEN");
      servo.write(90);        // Open position (adjust as needed)
      doorOpen = true;
      doorStatus="Open";

    // Activating all the sensors

    // fan
    digitalWrite(fanPin,HIGH);

    // mq sensor
    int airAnalog = analogRead(mqPin);

  if (airAnalog <= 2500) {
    digitalWrite(ledPin_red, LOW);
    digitalWrite(ledPin_green, HIGH);
    digitalWrite(buzzerPin,LOW);
  } else {
    digitalWrite(ledPin_red, HIGH);
    digitalWrite(ledPin_green, LOW);
    digitalWrite(buzzerPin,HIGH);
  }

  // dht sensor
  float h = dht.readHumidity();       // Read humidity
  float t = dht.readTemperature();    // Read temperature in Celsius
  float roundedTemp=round(t*10)/(10.0);

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(roundedTemp);
  Serial.print(F("°C  "));


//loop lcd display
unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 3000) {  
    previousMillis = currentMillis;
    lcd.clear();
    lcd.setCursor(0, 0);

    if (displayState==0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("AIR");
      lcd.setCursor(0,1);
      lcd.print(airAnalog);
      lcd.print("%");

      lcd.setCursor(4,0);
      lcd.print("|");
      lcd.setCursor(4,1);
      lcd.print("|");

      lcd.setCursor(5, 0);
      lcd.print("TEMP");
      lcd.setCursor(5,1);
      lcd.print(roundedTemp,1);

      lcd.setCursor(10,0);
      lcd.print("|");
      lcd.setCursor(10,1);
      lcd.print("|");

      lcd.setCursor(11, 0);
      lcd.print("HUMID");
      lcd.setCursor(11,1);
      lcd.print(h);
      displayState = 1; 
  
    }
     else if (displayState==1){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("DOOR :");
      lcd.setCursor(0,1);
      lcd.print(doorStatus);
      displayState=0;
    }
  }
}

    } 
    else {
      Serial.println("Door already open.");
    }
  else {
    Serial.println("Access Denied: Unauthorized tag.");
  }
}