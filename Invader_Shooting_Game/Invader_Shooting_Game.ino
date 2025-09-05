#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Player setup
#define PLAYER_WIDTH 12
#define PLAYER_HEIGHT 8
int playerX = SCREEN_WIDTH / 2;
int playerY = SCREEN_HEIGHT - 12;
#define LEFT_BTN 6
#define RIGHT_BTN 7
#define FIRE_BTN 8

// Player speed
int playerSpeed = 2;

// Bullet setup
#define BULLET_SPEED 4
bool bulletActive = false;
int bulletX, bulletY;

// Enemy setup
#define ENEMY_SIZE 6
int enemyX, enemyY;
bool enemyAlive = true;

// Enemy shooting
int enemyBulletSpeed = 3;
#define ENEMY_SHOOT_INTERVAL 1000  
unsigned long lastEnemyShot = 0;
bool enemyBulletActive = false;
int enemyBulletX, enemyBulletY;
float enemyBulletDX, enemyBulletDY;

// Score + game
int score = 0;
bool gameOver = false;

// Track last checkpoint for speed increase
int lastScoreCheckpoint = 0;

void spawnEnemy() {
  enemyX = random(0, SCREEN_WIDTH - ENEMY_SIZE);
  enemyY = random(10, SCREEN_HEIGHT / 2); // avoid score overlap
  enemyAlive = true;
}

void setup() {
  pinMode(LEFT_BTN, INPUT_PULLUP);
  pinMode(RIGHT_BTN, INPUT_PULLUP);
  pinMode(FIRE_BTN, INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  randomSeed(analogRead(A0));

  spawnEnemy();
}

void loop() {
  if (gameOver) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 20);
    display.print("GAME OVER");

    display.setTextSize(1);
    display.setCursor(30, 45);
    display.print("Score: ");
    display.print(score);

    display.setCursor(1, 55);
    display.print("Press FIRE to Restart");

    display.display();

    if (digitalRead(FIRE_BTN) == LOW) {
      delay(20); // debounce
      if (digitalRead(FIRE_BTN) == LOW) {
        // reset everything
        score = 0;
        enemyBulletSpeed = 3;
        playerSpeed = 2;
        lastScoreCheckpoint = 0;
        gameOver = false;
        playerX = SCREEN_WIDTH / 2;
        spawnEnemy();
        bulletActive = false;
        enemyBulletActive = false;
      }
    }
    return;
  }

  display.clearDisplay();

  // Score
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(score);

  // Player movement
  if (digitalRead(LEFT_BTN) == LOW && playerX > 0) playerX -= playerSpeed;
  if (digitalRead(RIGHT_BTN) == LOW && playerX < SCREEN_WIDTH - PLAYER_WIDTH) playerX += playerSpeed;

  // FIRE button = shoot
  if (digitalRead(FIRE_BTN) == LOW && !bulletActive) {
    fireBullet();
  }

  // Draw Player
  drawSpaceship(playerX, playerY);

  // Bullet logic
  if (bulletActive) {
    bulletY -= BULLET_SPEED;
    if (bulletY < 0) bulletActive = false;
    display.fillRect(bulletX, bulletY, 2, 4, SSD1306_WHITE);

    // Collision with enemy
    if (enemyAlive &&
        bulletX >= enemyX && bulletX <= enemyX + ENEMY_SIZE &&
        bulletY >= enemyY && bulletY <= enemyY + ENEMY_SIZE) {
      enemyAlive = false;
      bulletActive = false;
      score++;
      spawnEnemy();

      // Increase speeds every +3 points
      if (score % 3 == 0 && score != lastScoreCheckpoint) {
        enemyBulletSpeed += 2;  // enemy gets tougher
        playerSpeed += 1;       // player gets faster
        lastScoreCheckpoint = score;
      }
    }
  }

  // Enemy
  if (enemyAlive) {
    display.fillRect(enemyX, enemyY, ENEMY_SIZE, ENEMY_SIZE, SSD1306_WHITE);

    // Enemy shooting
    if (millis() - lastEnemyShot > ENEMY_SHOOT_INTERVAL && !enemyBulletActive) {
      lastEnemyShot = millis();
      enemyBulletX = enemyX + ENEMY_SIZE / 2;
      enemyBulletY = enemyY + ENEMY_SIZE / 2;

      int targetX = playerX + PLAYER_WIDTH / 2;
      int targetY = playerY;

      float dx = targetX - enemyBulletX;
      float dy = targetY - enemyBulletY;
      float dist = sqrt(dx * dx + dy * dy);

      enemyBulletDX = (dx / dist) * enemyBulletSpeed;
      enemyBulletDY = (dy / dist) * enemyBulletSpeed;

      enemyBulletActive = true;
    }
  }

  // Enemy bullet
  if (enemyBulletActive) {
    enemyBulletX += enemyBulletDX;
    enemyBulletY += enemyBulletDY;

    display.fillRect(enemyBulletX, enemyBulletY, 2, 2, SSD1306_WHITE);

    // Collision with player
    if (enemyBulletX >= playerX && enemyBulletX <= playerX + PLAYER_WIDTH &&
        enemyBulletY >= playerY && enemyBulletY <= playerY + PLAYER_HEIGHT) {
      gameOver = true;
    }

    // Off-screen
    if (enemyBulletX < 0 || enemyBulletX > SCREEN_WIDTH || enemyBulletY > SCREEN_HEIGHT) {
      enemyBulletActive = false;
    }
  }

  display.display();
  delay(30);
}

void fireBullet() {
  bulletX = playerX + PLAYER_WIDTH / 2;
  bulletY = playerY - 6;  // nose of ship
  bulletActive = true;
}

// Spaceship drawing
void drawSpaceship(int x, int y) {
  int centerX = x + PLAYER_WIDTH / 2;

  // Nose
  display.fillTriangle(centerX, y - 6, x, y, x + PLAYER_WIDTH, y, SSD1306_WHITE);

  // Body
  display.fillRect(x + 3, y, PLAYER_WIDTH - 6, 6, SSD1306_WHITE);

  // Wings
  display.fillTriangle(x - 2, y + 2, x + 3, y + 2, x + 3, y + 6, SSD1306_WHITE);
  display.fillTriangle(x + PLAYER_WIDTH - 3, y + 2, x + PLAYER_WIDTH + 2, y + 2, x + PLAYER_WIDTH - 3, y + 6, SSD1306_WHITE);
}
