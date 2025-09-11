#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>  // Use the official Adafruit SH110x library
#include <Adafruit_MPU6050.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define PLAYER_WIDTH 12
#define PLAYER_HEIGHT 4
#define MAX_BULLETS 5
#define OLED_RESET -1  // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MPU6050 mpu;

// Player
int playerX = SCREEN_WIDTH/2 - PLAYER_WIDTH/2;
int playerY = SCREEN_HEIGHT - PLAYER_HEIGHT;
int score = 0;
int activeBullets = 2;

struct Bullet {
  int x, y, speed;
  bool active;
};
Bullet bullets[MAX_BULLETS];

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialize display
  if(!display.begin(0x3C, true)) { // Address 0x3C for 128x64
    Serial.println(F("SH1106 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.display();

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println(F("MPU6050 not found!"));
    while (1);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  initBullets();
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Tilt controls - inverted for more intuitive movement
  if (a.acceleration.x > 2) playerX += 4;
  else if (a.acceleration.x < -2) playerX -= 4;
  playerX = constrain(playerX, 0, SCREEN_WIDTH - PLAYER_WIDTH);

  // Update bullets
  for (int i = 0; i < activeBullets; i++) {
    if (bullets[i].active) {
      bullets[i].y += bullets[i].speed;
      
      // Check if bullet reached bottom
      if (bullets[i].y > SCREEN_HEIGHT) {
        score++;
        respawnBullet(i);
        if (score % 10 == 0 && activeBullets < MAX_BULLETS) activeBullets++;
        if (score % 5 == 0) increaseSpeed();
      }

      // Collision check - more precise
      if (bullets[i].active && 
          bullets[i].y + 2 >= playerY &&
          bullets[i].y - 2 <= playerY + PLAYER_HEIGHT &&
          bullets[i].x >= playerX - 2 &&
          bullets[i].x <= playerX + PLAYER_WIDTH + 2) {
        gameOver();
        return;
      }
    }
  }

  // Render
  display.clearDisplay();
  
  // Draw player
  display.fillRect(playerX, playerY, PLAYER_WIDTH, PLAYER_HEIGHT, SH110X_WHITE);
  
  // Draw bullets
  for (int i = 0; i < activeBullets; i++) {
    if (bullets[i].active) {
      display.fillCircle(bullets[i].x, bullets[i].y, 2, SH110X_WHITE);
    }
  }
  
  // Draw score
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.print(F("Score: "));
  display.print(score);
  
  display.display();
  delay(30);
}

void initBullets() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    bullets[i].active = true;
    bullets[i].x = random(0, SCREEN_WIDTH);
    bullets[i].y = random(-40, -5);
    bullets[i].speed = random(3, 6);
  }
}

void respawnBullet(int i) {
  bullets[i].active = true;
  bullets[i].x = random(0, SCREEN_WIDTH);
  bullets[i].y = random(-40, -5);
  bullets[i].speed = random(3, 6);
}

void increaseSpeed() {
  for (int i = 0; i < activeBullets; i++) {
    if (bullets[i].active) {
      bullets[i].speed += 1;
    }
  }
}

void gameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(20, 20);
  display.println(F("GAME OVER"));
  display.setTextSize(1);
  display.setCursor(30, 50);
  display.print(F("Score: "));
  display.print(score);
  display.display();

  delay(3000);

  // Reset game
  playerX = SCREEN_WIDTH/2 - PLAYER_WIDTH/2;
  score = 0;
  activeBullets = 2;
  initBullets();
}