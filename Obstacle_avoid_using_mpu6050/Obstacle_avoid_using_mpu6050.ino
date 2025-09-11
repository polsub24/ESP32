#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
<<<<<<< HEAD
#define OLED_ADDR    0x3C   // check with I2C scanner
=======
#define OLED_ADDR    0x3C  // check with I2C scanner
>>>>>>> 97c8e1e3f7532a87a5586c0ae8a4c0e17827e20d
#define PLAYER_WIDTH 12
#define PLAYER_HEIGHT 4
#define MAX_BULLETS 5   // number of bullets that can fall at once

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_MPU6050 mpu;

// Player
int playerX = SCREEN_WIDTH / 2;
int playerY = SCREEN_HEIGHT - PLAYER_HEIGHT;

// Bullets
struct Bullet {
  int x, y, speed;
  bool active;
};

Bullet bullets[MAX_BULLETS];
int activeBullets = 2;   // start with 2 bullets
int score = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 init failed."));
    for (;;);
  }
  display.clearDisplay();

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

  // Player control
  if (a.acceleration.x > 2) playerX -= 4;
  else if (a.acceleration.x < -2) playerX += 4;
  playerX = constrain(playerX, 0, SCREEN_WIDTH - PLAYER_WIDTH);

  // Update bullets
  for (int i = 0; i < activeBullets; i++) {
    if (bullets[i].active) {
      bullets[i].y += bullets[i].speed;
      if (bullets[i].y > SCREEN_HEIGHT) {
        score++;
        respawnBullet(i);
        if (score % 10 == 0 && activeBullets < MAX_BULLETS) activeBullets++; // more bullets
        if (score % 5 == 0) increaseSpeed(); // faster bullets
      }

      // Collision check
      if (bullets[i].y + 2 >= playerY &&
          bullets[i].x >= playerX &&
          bullets[i].x <= playerX + PLAYER_WIDTH) {
        gameOver();
      }
    }
  }

  // Draw
  display.clearDisplay();
  display.fillRect(playerX, playerY, PLAYER_WIDTH, PLAYER_HEIGHT, SSD1306_WHITE);

  for (int i = 0; i < activeBullets; i++) {
    if (bullets[i].active) {
      display.fillCircle(bullets[i].x, bullets[i].y, 2, SSD1306_WHITE);
    }
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("Score: "));
  display.print(score);
  display.display();

  delay(30); // lower delay = faster game loop
}

void initBullets() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    bullets[i].active = true;
    bullets[i].x = random(0, SCREEN_WIDTH);
    bullets[i].y = random(-20, 0); // staggered start
    bullets[i].speed = 3; // initial speed
  }
}

void respawnBullet(int i) {
  bullets[i].x = random(0, SCREEN_WIDTH);
  bullets[i].y = 0;
  bullets[i].speed = random(3, 6); // randomize speed a bit
}

void increaseSpeed() {
  for (int i = 0; i < activeBullets; i++) {
    bullets[i].speed++;
  }
}

void gameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 20);
  display.println(F("GAME OVER"));
  display.setTextSize(1);
  display.setCursor(30, 50);
  display.print(F("Score: "));
  display.print(score);
  display.display();

  delay(3000);

  playerX = SCREEN_WIDTH / 2;
  score = 0;
  activeBullets = 2;
  initBullets();
}

