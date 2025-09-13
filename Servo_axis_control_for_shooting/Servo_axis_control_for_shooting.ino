#include <ESP32Servo.h>

// Pin definitions
#define X_SERVO_PIN 13
#define Y_SERVO_PIN 12
#define FIRE_SERVO_PIN 14
#define FIRE_BUTTON_PIN 27
#define LEFT_BUTTON_PIN 26
#define RIGHT_BUTTON_PIN 25
#define UP_BUTTON_PIN 33
#define DOWN_BUTTON_PIN 32

// Servo objects
Servo xServo;  // Horizontal movement (pan)
Servo yServo;  // Vertical movement (tilt)
Servo fireServo; // Firing mechanism

// Servo limits (adjust based on your mechanical setup)
#define X_MIN_ANGLE 0
#define X_MAX_ANGLE 180
#define Y_MIN_ANGLE 0
#define Y_MAX_ANGLE 120
#define FIRE_READY_ANGLE 0    // Position when ready to fire
#define FIRE_RELEASE_ANGLE 90 // Position to release rubber band

// Movement parameters
#define MOVE_INCREMENT 5      // Degrees to move per button press
#define MOVE_DELAY 150        // ms between movements when holding button
#define DEBOUNCE_DELAY 50     // ms for button debouncing

// Current positions
int xPos = 90;  // Start at center position
int yPos = 60;  // Start at mid elevation

// Control variables
bool isFiring = false;
unsigned long fireStartTime = 0;
const unsigned long FIRE_DURATION = 1000; // Time to complete fire sequence

// Button state variables
bool leftPressed = false;
bool rightPressed = false;
bool upPressed = false;
bool downPressed = false;
bool firePressed = false;
unsigned long lastMoveTime = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize servos
  xServo.attach(X_SERVO_PIN);
  yServo.attach(Y_SERVO_PIN);
  fireServo.attach(FIRE_SERVO_PIN);
  
  // Initialize button pins
  pinMode(FIRE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);
  
  // Move to initial position
  xServo.write(xPos);
  yServo.write(yPos);
  fireServo.write(FIRE_READY_ANGLE);
  
  Serial.println("Dart Shooter Initialized");
  Serial.println("Controls:");
  Serial.println("Left/Right - Adjust X aim");
  Serial.println("Up/Down - Adjust Y aim");
  Serial.println("Fire - Launch dart");
}

void loop() {
  // Read all buttons
  readButtons();
  
  // Handle aiming movements
  handleAiming();
  
  // Handle firing
  handleFiring();
  
  // Handle firing sequence
  if (isFiring) {
    handleFiringSequence();
  }
  
  // Small delay to prevent overwhelming the system
  delay(10);
}

void readButtons() {
  leftPressed = (digitalRead(LEFT_BUTTON_PIN) == LOW);
  rightPressed = (digitalRead(RIGHT_BUTTON_PIN) == LOW);
  upPressed = (digitalRead(UP_BUTTON_PIN) == LOW);
  downPressed = (digitalRead(DOWN_BUTTON_PIN) == LOW);
  firePressed = (digitalRead(FIRE_BUTTON_PIN) == LOW);
}

void handleAiming() {
  unsigned long currentTime = millis();
  
  // Check if enough time has passed since last movement
  if (currentTime - lastMoveTime >= MOVE_DELAY) {
    bool moved = false;
    
    // Handle left/right movement
    if (leftPressed) {
      xPos = max(X_MIN_ANGLE, xPos - MOVE_INCREMENT);
      moved = true;
    } else if (rightPressed) {
      xPos = min(X_MAX_ANGLE, xPos + MOVE_INCREMENT);
      moved = true;
    }
    
    // Handle up/down movement
    if (upPressed) {
      yPos = min(Y_MAX_ANGLE, yPos + MOVE_INCREMENT);
      moved = true;
    } else if (downPressed) {
      yPos = max(Y_MIN_ANGLE, yPos - MOVE_INCREMENT);
      moved = true;
    }
    
    // Update servos if movement occurred
    if (moved) {
      xServo.write(xPos);
      yServo.write(yPos);
      lastMoveTime = currentTime;
      
      // Print position for debugging
      Serial.print("Position - X: ");
      Serial.print(xPos);
      Serial.print(", Y: ");
      Serial.println(yPos);
    }
  }
}

void handleFiring() {
  if (firePressed && !isFiring) {
    triggerFire();
  }
}

void triggerFire() {
  Serial.println("Firing!");
  isFiring = true;
  fireStartTime = millis();
  fireServo.write(FIRE_RELEASE_ANGLE);
}

void handleFiringSequence() {
  unsigned long currentTime = millis();
  
  if (currentTime - fireStartTime >= FIRE_DURATION) {
    // Return to ready position
    fireServo.write(FIRE_READY_ANGLE);
    isFiring = false;
    Serial.println("Ready to fire again");
  }
}