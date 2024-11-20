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

Snake::Snake(uint8_t gridSize, uint16_t cellWidth, uint16_t cellHeight,
             Adafruit_ILI9341 screen)
    : gridSize(gridSize), cellWidth(cellWidth), cellHeight(cellHeight),
      screen(screen) {
  snakeLength = 6;                           // starting length of the snake
  snakeX = new uint8_t[gridSize * gridSize]; // maxsize of the snake as size
  snakeY = new uint8_t[gridSize * gridSize]; // maxsize of the snake as size
  direction = RIGHT;                         // start moving to the right
}

void Snake::start() {
  // start the snake at the center of the screen
  snakeX[0] = gridSize / 2;
  snakeY[0] = gridSize / 2;
}

// update direction with joystick readings
void Snake::updateDirection(
    uint8_t joyX,
    uint8_t joyY) { // TODO: remove magic numbers from the joystick angles
  if (joyX < 100 && direction != RIGHT)
    direction = LEFT;
  else if (joyX > 150 && direction != LEFT)
    direction = RIGHT;
  else if (joyY < 100 && direction != UP)
    direction = DOWN;
  else if (joyY > 150 && direction != DOWN)
    direction = UP;
}

void Snake::move() {
  // store position off tail
  uint8_t tailX = snakeX[snakeLength - 1];
  uint8_t tailY = snakeY[snakeLength - 1];

  // shifting the body
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  // move the head
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

  // clear old tail before drawing new
  clearTail(tailX, tailY);
}

void Snake::draw() {
  for (uint8_t i = 0; i < snakeLength; i++) { // for leght
    drawCell(snakeX[i], snakeY[i], MAGENTA);  // draw snake cell
  }
}

// check colision with itself orr the border
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
  return false; // if no collision, return false
}

// grow size/length of snake
void Snake::grow() { snakeLength++; }

// clearint the tail instead of clearing the whole screen
void Snake::clearTail(uint8_t tailX, uint8_t tailY) {
  drawCell(tailX, tailY, BLACK);
}

// draw snake cell
void Snake::drawCell(uint16_t x, uint16_t y, uint16_t colour) {
  screen.fillRect(x * cellWidth, y * cellHeight, cellWidth, cellHeight, colour);
}
