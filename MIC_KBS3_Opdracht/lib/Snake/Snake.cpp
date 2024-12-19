#include "Snake.h"
#include "HardwareSerial.h"
#include "string.h"

// Color defines
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define LGREY 0x4208
#define DBLUE 0x0007

// Sound Defines
#define SOUND_EAT 128
#define SOUND_GAMESTART 50
#define SOUND_DEATH 20;

const uint16_t TFT_WIDTH = 240;
const uint16_t TFT_HEIGHT = 320;

const uint8_t SNAKE_START_LENGHT = 3;

volatile bool isMainSnake = false;

Snake::Snake(uint8_t gridSize, uint16_t cellWidth, uint16_t cellHeight,
             Display &screen, uint16_t colour, bool primarySnake, bool sender)
    : screen(screen), gridSize(gridSize), cellWidth(cellWidth),
      cellHeight(cellHeight), colour(colour) {
  snakeLength = SNAKE_START_LENGHT;
  if(primarySnake){
    if(!sender){
      direction = UP;
      bufferedDirection = UP;
    } else {
      direction = DOWN;
      bufferedDirection = DOWN;
    }
  } else {
    if(!sender){
      direction = DOWN;
      bufferedDirection = DOWN;
    } else {
      direction = UP;
      bufferedDirection = UP;
    }
  }
  gridSize = gridSize;
  cellWidth = cellWidth;
  cellHeight = cellHeight;
  screen = screen;
  colour = colour;
  isMainSnake = primarySnake;
  OCR0A = 0;
}

void Snake::start(uint8_t x, uint8_t y) {
  // snake starten in center scherm
  snakeX[0] = x;
  snakeY[0] = y;
}

// joystick met richting updaten
void Snake::updateDirection(
    uint8_t joyX,
    uint8_t joyY) { // TODO: remove magic numbers from the joystick angles
  if (joyX < 105 && direction != RIGHT)
    bufferedDirection = LEFT;
  else if (joyX > 145 && direction != LEFT)
    bufferedDirection = RIGHT;
  else if (joyY < 105 && direction != UP)
    bufferedDirection = DOWN;
  else if (joyY > 145 && direction != DOWN)
    bufferedDirection = UP;
}

void Snake::move() {
  validateDirection();

  // positie van de eindstaart opslaan
  uint8_t tailX = snakeX[snakeLength - 1];
  uint8_t tailY = snakeY[snakeLength - 1];

  // het lichaam bewegen
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  // hoofd bewegen
  switch (direction) {
  case UP:
    snakeY[0]--;
    break;
  case DOWN:
    snakeY[0]++;
    break;
  case LEFT:
    snakeX[0]--;
    break;
  case RIGHT:
    snakeX[0]++;
    break;
  }

  // oude staart clearen voor het tekenen van de nieuwe
  clearTail(tailX, tailY);
}

void Snake::draw() {

  // teken hoofd
  drawHead(snakeX[0], snakeY[0]);
  // alles na hoofd veranderd naar body
  // eerst hoofd clearen
  screen.fillRect(snakeX[1] * cellWidth, snakeY[1] * cellHeight, cellWidth + 1,
                  cellHeight + 1, BLACK);
  // teken body deel
  drawCell(snakeX[1], snakeY[1], colour);

  // draw appel
  if (!(snakeX[0] == appleX && snakeY[0] == appleY)) {
    drawCell(appleX, appleY, RED);
  } else {
    drawCell(appleX, appleY, GREEN);
  }

  // draw score
  drawScore();
}

// collision checken met zichzelf en de borders
bool Snake::checkCollision() {
  // check for border collision
  if (snakeX[0] < 0 || snakeX[0] >= gridSize || snakeY[0] < 0 ||
      snakeY[0] >= gridSize) {
    return true;
  }
  // Check collision with itslef
  for (uint8_t i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      return true;
    }
  }
  return false; // als geen collision, return false
}

// lengte groeien van de slang
void Snake::grow() { snakeLength++; }

bool Snake::eatApple(uint8_t appleX, uint8_t appleY) {
  if (snakeX[0] == appleX &&
      snakeY[0] == appleY) { // als hoofd van de snake is op pos van appel
    drawHead(appleX, appleY);
    spawnRandApple();

    return true; // appel is gegeten
  }
  return false; // niet gegeten
}

void Snake::spawnRandApple() {
  srand(TCNT0); // rand seed //TODO: seed vervangen voor clock waarde
  appleX = rand() % gridSize; // random appel spawn in het veld
  appleY = rand() % gridSize; // random appel spawn in het veld
}

// alleen staart clearen ipv hele scherm
void Snake::clearTail(uint8_t tailX, uint8_t tailY) {
  drawCell(tailX, tailY, BLACK);
}

// teken snake cell
void Snake::drawCell(uint16_t x, uint16_t y, uint16_t colour) {
  // de snake cell centeren op juiste locaties binnen het grid
  uint16_t radius = (cellWidth - 1) / 2;
  uint16_t centerX = x * cellWidth + radius;
  uint16_t centerY = y * cellHeight + radius;
  screen.fillCircle(centerX, centerY, radius, colour);
}

void Snake::validateDirection() {
  if ((direction == UP && bufferedDirection != DOWN) ||
      (direction == DOWN && bufferedDirection != UP) ||
      (direction == LEFT && bufferedDirection != RIGHT) ||
      (direction == RIGHT && bufferedDirection != LEFT)) {
    direction = bufferedDirection;
  }
}

void Snake::reset() {
  // snakelengte resetten
  snakeLength = SNAKE_START_LENGHT; // start lenget snake

  // Clear Arrays om memory leaks te voorkomen
  delete[] snakeX;
  delete[] snakeY;

  // Herinitialize de arrays
  snakeX = new uint8_t[128]; // gridsize * gridsize / 2
  snakeY = new uint8_t[128];

  // start richting zetten
  direction = RIGHT;

  // nieuwe appel spawnen
  if(isPrimarySnake){
  spawnRandApple();
  }
}

// void Snake::setHighscore(uint8_t newScore) {

// }

void Snake::playSound(Sound sound) {
  TIMSK0 |= (1 << OCIE0A);
  TCNT0 = 0;

  if (sound == EAT) {
    OCR0A = SOUND_EAT;
  } else if (sound == DEATH) {
    OCR0A = SOUND_DEATH;
  } else if (sound == STARTGAME) {
    OCR0A = SOUND_GAMESTART;
  }
}

void Snake::stopSound() { TIMSK2 &= ~(1 << OCIE2B); }

uint16_t Snake::getScore() { return score; }

uint16_t Snake::getHighscore() {
  if (score > highscore) {
    highscore = score;
  }
  return highscore;
}

// Draw Score Text
void Snake::drawScore() {
  static uint8_t prevScore = -1;
  int8_t currentScore = snakeLength - SNAKE_START_LENGHT;
  score = currentScore;
  // score alleen updaten/tekenen wanneer de score verhoogd
  if (currentScore != prevScore) {
    screen.setCursor(5, TFT_WIDTH + 5);
    screen.setTextColor(WHITE, BLACK); // oude overschrijven met zwart
    screen.setTextSize(1);
    screen.print("Score: ");
    screen.print(score);
    prevScore = currentScore;
  }
}

Snake::Direction Snake::getDirection() { return direction; }

void Snake::setDirection(Direction direction) { direction = direction; }

void Snake::drawHead(uint16_t x, uint16_t y) {

  // center van driehoek berekenen
  uint16_t centerX = x * cellWidth + cellHeight / 2;
  uint16_t centerY = y * cellHeight + cellWidth / 2;

  // driehoek punten op basis van de richting van de snake
  int16_t x1, y1, x2, y2, x3, y3;
  switch (direction) {
  case UP:
    x1 = centerX - cellWidth / 2;
    y1 = centerY + cellHeight / 2;
    x2 = centerX + cellWidth / 2;
    y2 = centerY + cellHeight / 2;
    x3 = centerX;
    y3 = centerY - cellHeight / 2;
    break;
  case DOWN:
    x1 = centerX - cellWidth / 2;
    y1 = centerY - cellHeight / 2;
    x2 = centerX + cellWidth / 2;
    y2 = centerY - cellHeight / 2;
    x3 = centerX;
    y3 = centerY + cellHeight / 2;
    break;
  case LEFT:
    x1 = centerX + cellWidth / 2;
    y1 = centerY - cellHeight / 2;
    x2 = centerX + cellWidth / 2;
    y2 = centerY + cellHeight / 2;
    x3 = centerX - cellWidth / 2;
    y3 = centerY;
    break;
  case RIGHT:
    x1 = centerX - cellWidth / 2;
    y1 = centerY - cellHeight / 2;
    x2 = centerX - cellWidth / 2;
    y2 = centerY + cellHeight / 2;
    x3 = centerX + cellWidth / 2;
    y3 = centerY;
    break;
  }

  // driehoek tekenen binnen de berekende punten
  screen.fillTriangle(x1, y1, x2, y2, x3, y3, colour);

  // ogern van de snake tekkenen
  // ogen komen op de eerste kwart van het hoofd, dus daarom /4 en hebben een
  // kleine grootte dus straal van 1/8 cellwidth
  switch (direction) {
    // omhoog en omlaag linker en rechter oog tekenen
  case UP:
  case DOWN:
    screen.fillCircle(centerX - cellWidth / 4, centerY, cellWidth / 8,
                      BLACK); // linker ook
    screen.fillCircle(centerX + cellWidth / 4, centerY, cellWidth / 8,
                      BLACK); // rechter oog
    break;
  // linker en rechter richting boven en onder oog tekenen
  case LEFT:
  case RIGHT:
    screen.fillCircle(centerX, centerY - cellHeight / 4, cellHeight / 8,
                      BLACK); // bovenste oog
    screen.fillCircle(centerX, centerY + cellHeight / 4, cellHeight / 8,
                      BLACK); // onderste oog
    break;
  }
}
