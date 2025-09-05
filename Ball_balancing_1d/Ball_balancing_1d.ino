#include <Servo.h>

// Pin definitions
#define TRIG_PIN 5
#define ECHO_PIN 6
#define SERVO_PIN 9   // must be PWM-capable

Servo servo;

// Target distance (cm from ultrasonic sensor)
const float setpoint = 10.0;   // ✅ balance point = 10 cm

// PID parameters (tune experimentally)
float Kp = 2.0;
float Ki = 0.1;
float Kd = 0.8;

// PID variables
float error, prevError = 0;
float integral = 0;
float distance;
long duration;

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Attach servo
  servo.attach(SERVO_PIN);
  servo.write(90);  // Neutral (flat beam)
}

// Function to read distance (cm)
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH, 20000UL); // 20ms timeout
  if (duration == 0) return -1; // No reading
  return (duration * 0.034 / 2.0); // cm
}

void loop() {
  distance = getDistance();

  // Safety check: invalid or out of expected range
  if (distance < 2 || distance > 40 || distance == -1) {
    servo.write(90); // Neutral position
    Serial.println("⚠️ Out of range → Reset to neutral");
    delay(100);
    return;
  }

  // PID calculations
  error = setpoint - distance;

  // If ball rolled too far away (error too big), reset
  if (abs(error) > 15) {
    servo.write(90); // Neutral
    Serial.println("⚠️ Error too big → Reset to neutral");
    delay(100);
    return;
  }

  integral += error;
  float derivative = error - prevError;

  float output = Kp*error + Ki*integral + Kd*derivative;

  // Servo control (center = 90°)
  int angle = 90 + output;

  // Limit servo movement
  angle = constrain(angle, 60, 120);

  servo.write(angle);

  // Debugging
  Serial.print("Distance: "); Serial.print(distance);
  Serial.print(" cm | Error: "); Serial.print(error);
  Serial.print(" | Servo Angle: "); Serial.println(angle);

  prevError = error;

  delay(40); // ~25 Hz loop
}


