#include "Snake.h"

// Color defines
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

const uint16_t TFT_WIDTH = 240;
const uint16_t TFT_HEIGHT = 320;

const uint8_t SNAKE_START_LENGHT = 3;

Snake::Snake(uint8_t gridSize, uint16_t cellWidth, uint16_t cellHeight,
             Adafruit_ILI9341 screen, uint16_t colour)
    : gridSize(gridSize), cellWidth(cellWidth), cellHeight(cellHeight),
      screen(screen), colour(colour) {
  snakeLength = SNAKE_START_LENGHT;          // start lengte van de snake
  snakeX = new uint8_t[gridSize * gridSize]; // max grootte van de snake
  snakeY = new uint8_t[gridSize * gridSize]; // max grootte van de snake
  direction = RIGHT;                         // beginrichting is rechts
  spawnRandApple();
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
    direction = LEFT;
  else if (joyX > 145 && direction != LEFT)
    direction = RIGHT;
  else if (joyY < 105 && direction != UP)
    direction = DOWN;
  else if (joyY > 145 && direction != DOWN)
    direction = UP;
}

void Snake::move() {
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
  // snake tekenen
  for (uint8_t i = 0; i < snakeLength; i++) {
    if (i == 0 || i == snakeLength - 1) { // alleen kop en staart tekenen
      drawCell(snakeX[i], snakeY[i], colour);
    }
  }

  // appel tekenen als hij niet gegeten wordt
  if(!(appleX == snakeX[0] && appleY == snakeY[0])){
    drawCell(appleX, appleY, RED);
  }

  // draw border
  screen.drawLine(0, TFT_WIDTH, TFT_WIDTH, TFT_WIDTH, WHITE);

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
    drawCell(appleX, appleY, colour);
    spawnRandApple();
    return true; // appel is gegeten
  }
  return false; // niet gegeten
}

void Snake::spawnRandApple() {
  static uint8_t seed = 1;
  srand(seed);                // rand seed //TODO: seed vervangen voor clock waarde
  appleX = rand() % gridSize; // random appel spawn in het veld
  appleY = rand() % gridSize; // random appel spawn in het veld
  seed += 69;                 // random seed
}

// alleen staart clearen ipv hele scherm
void Snake::clearTail(uint8_t tailX, uint8_t tailY) {
  drawCell(tailX, tailY, BLACK);
}

// teken snake cell
void Snake::drawCell(uint16_t x, uint16_t y, uint16_t colour) {
  screen.drawRect(x * cellWidth, y * cellHeight, cellWidth, cellHeight, colour);
}

void Snake::reset() {
  // snakelengte resetten
  snakeLength = SNAKE_START_LENGHT; // start lenget snake

  // snake position resetten naar het midden
  start(gridSize / 2, gridSize / 2);

  // alles wat niet de snake is clearen
  for (int i = 1; i < gridSize * gridSize; i++)
  {
    snakeX[i] = 0;
    snakeY[i] = 0;
  }

  // start richting zetten
  direction = RIGHT;

  // nieuwe appel spawnen
  spawnRandApple();

  // screen resetten van oude snakes en appels
  screen.fillScreen(BLACK);
}

void Snake::drawScore() {
  screen.setCursor(5 , TFT_WIDTH + 5);
  screen.setTextColor(WHITE, BLACK); // oude overschrijven met zwart
  screen.setTextSize(1);
  screen.print("Score: ");
  screen.print(snakeLength - SNAKE_START_LENGHT);
}

void Snake::drawDeathScreen() {
  screen.fillScreen(BLACK);
  screen.setCursor(60, 160);
  screen.setTextColor(RED);
  screen.setTextSize(2);
  screen.println("Game Over!");
  screen.println("PRESS Z TO CONTINUE");
}
