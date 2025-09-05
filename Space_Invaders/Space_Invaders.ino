#include <LiquidCrystal.h>  // Replace with your OLED lib if using OLED

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

const int leftBtnPin = 6;
const int rightBtnPin = 7;
const int shootBtnPin = 8;
const int buzzerPin = 9;

byte shipChar[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};

byte alienChar[8] = {
  0b01010,
  0b11111,
  0b11111,
  0b01110,
  0b11111,
  0b10101,
  0b01010,
  0b00000
};

byte bulletChar[8] = {
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00000,
  0b00000,
  0b00000
};

int shipPos = 7;
int bulletPos = -1;
int bulletRow = 1;
bool bulletActive = false;

const int MAX_ALIENS = 10;
bool aliens[MAX_ALIENS];
int alienCount = 0;

int alienStartPos = 0;
int alienDirection = 1;

unsigned long lastAlienMove = 0;
const unsigned long alienMoveInterval = 700;

int points = 0;

bool gameOver = false;

void setup() {
  pinMode(leftBtnPin, INPUT_PULLUP);
  pinMode(rightBtnPin, INPUT_PULLUP);
  pinMode(shootBtnPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);

  lcd.begin(16, 2);

  lcd.createChar(0, shipChar);
  lcd.createChar(1, alienChar);
  lcd.createChar(2, bulletChar);

  Serial.begin(9600);
  resetGame();
}

void loop() {
  if (gameOver) {
    showGameOver();
    if (digitalRead(shootBtnPin) == LOW) {  // Press shoot button to restart
      resetGame();
      delay(500);
    }
    return;
  }

  readButtons();
  moveAliens();
  moveBullet();
  delay(50);
}

void resetGame() {
  points = 0;
  shipPos = 7;
  bulletActive = false;
  alienDirection = 1;
  alienStartPos = 0;
  gameOver = false;
  generateAliens();
  Serial.println("Game Started! Points: 0");
  drawGame();
}

void generateAliens() {
  alienCount = random(3, MAX_ALIENS + 1);
  for (int i = 0; i < MAX_ALIENS; i++) {
    aliens[i] = (i < alienCount) ? true : false;  // First alienCount aliens alive
  }
  alienStartPos = 0;
}

void readButtons() {
  if (digitalRead(leftBtnPin) == LOW && shipPos > 0) {
    shipPos--;
    drawGame();
    delay(150);
  }
  if (digitalRead(rightBtnPin) == LOW && shipPos < 15) {
    shipPos++;
    drawGame();
    delay(150);
  }
  if (digitalRead(shootBtnPin) == LOW && !bulletActive) {
    bulletPos = shipPos;
    bulletRow = 1;
    bulletActive = true;
    tone(buzzerPin, 1000, 100);
    drawGame();
    delay(150);
  }
}

void moveAliens() {
  if (millis() - lastAlienMove > alienMoveInterval) {
    alienStartPos += alienDirection;
    if (alienStartPos <= 0 || alienStartPos + alienCount - 1 >= 15) {
      alienDirection = -alienDirection;
      alienStartPos += alienDirection;
    }

    // Check game over condition: alien reaches ship position
    for (int i = 0; i < alienCount; i++) {
      if (aliens[i]) {
        int alienPos = alienStartPos + i;
        if (alienPos == shipPos) {
          gameOver = true;
          tone(buzzerPin, 400, 1000);
          Serial.println("Game Over!");
          return;
        }
      }
    }
    lastAlienMove = millis();
    drawGame();
  }
}

void moveBullet() {
  if (bulletActive) {
    bulletRow--;
    if (bulletRow < 0) {
      bulletActive = false;
      drawGame();
      return;
    }
    if (bulletRow == 0) {
      int alienIndex = bulletPos - alienStartPos;
      if (alienIndex >= 0 && alienIndex < alienCount && aliens[alienIndex]) {
        aliens[alienIndex] = false;
        bulletActive = false;
        points += 10;
        Serial.print("Hit! Points: ");
        Serial.println(points);
        tone(buzzerPin, 2000, 300);
        drawGame();

        // Check if all aliens destroyed, generate new aliens
        bool allDead = true;
        for (int i=0; i<alienCount; i++) {
          if (aliens[i]) {
            allDead = false;
            break;
          }
        }
        if (allDead) {
          generateAliens();
        }
        return;
      }
    }
    drawGame();
  }
}

void drawGame() {
  lcd.clear();

  // Draw aliens on top row
  for (int i = 0; i < alienCount; i++) {
    lcd.setCursor(alienStartPos + i, 0);
    if (aliens[i]) {
      if (bulletActive && bulletRow == 0 && bulletPos == alienStartPos + i) {
        lcd.write(byte(2));  // bullet char instead of alien if bullet overlaps
      } else {
        lcd.write(byte(1));  // alien char
      }
    } else {
      lcd.print(" ");
    }
  }

  // Clear unused columns on top row
  for (int i = 0; i < alienStartPos; i++) {
    lcd.setCursor(i, 0);
    lcd.print(" ");
  }
  for (int i = alienStartPos + alienCount; i < 16; i++) {
    lcd.setCursor(i, 0);
    lcd.print(" ");
  }

  // Draw bullet and ship on bottom row
  for (int i = 0; i < 16; i++) {
    lcd.setCursor(i, 1);
    if (bulletActive && bulletRow == 1 && bulletPos == i) {
      lcd.write(byte(2));
    } else if (shipPos == i) {
      lcd.write(byte(0));
    } else {
      lcd.print(" ");
    }
  }
}

void showGameOver() {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("GAME OVER!");
  lcd.setCursor(2, 1);
  lcd.print("Points: ");
  lcd.print(points);
  lcd.setCursor(0, 1);
  lcd.print("Press Shoot to Restart");
}
