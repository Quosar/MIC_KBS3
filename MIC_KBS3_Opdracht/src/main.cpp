#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Nunchuk.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

// LCD Pin Defines
#define TFT_CLK 13
#define TFT_MISO 12
#define TFT_MOSI 11
#define TFT_DC 9
#define TFT_CS 10
#define TFT_RST 8

// Color defines
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

// Screen dimencions
const uint16_t TFT_WIDTH = 240;
const uint16_t TFT_HEIGHT = 320;

// Ball offset (ball so x and y offset)
const uint8_t BALL_RADIUS = 6;

// Nunchuk adres
const int NUNCHUCK_ADDRESS = 0x52;

// Create objects for TFT display and nunchuck
Adafruit_ILI9341 screen(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

NunChuk nunchuck;

// Game variables
const uint8_t GRID_SIZE = 16;
const uint16_t CELL_WIDTH = TFT_WIDTH / GRID_SIZE;
const uint16_t CELL_HEIGHT = TFT_HEIGHT / GRID_SIZE;
const uint16_t MAX_SIZE = CELL_HEIGHT * CELL_WIDTH;

// Snake directions
enum Direction { UP, DOWN, LEFT, RIGHT };

// Snake variables
const uint8_t SNAKE_CELL_HEIGHT = TFT_HEIGHT / CELL_HEIGHT;
const uint8_t SNAKE_CELL_WIDTH = TFT_WIDTH / CELL_WIDTH;
Direction snakeDirection = RIGHT;
uint8_t snakeLength = 9; // start length

// snake head starts at snakeX[0] and snakeY[0]. The end of the tail is is
// stored inn the last used index (so snakeX[length-1] and snakeY[length-1])
uint8_t snakeX[MAX_SIZE];
uint8_t snakeY[MAX_SIZE];

// Snake position
uint16_t snakePosX = TFT_WIDTH / 2, snakePosY = TFT_HEIGHT / 2;
uint16_t lastPosX = snakePosX, lastPosY = snakePosY;

// Function to draw the snakes cells
void drawSnakeCell(uint16_t x, uint16_t y, uint16_t colour) {
  screen.fillRect(x, y, SNAKE_CELL_WIDTH, SNAKE_CELL_HEIGHT, colour);
}

void updateDirection() {
  if (nunchuck.getState(NUNCHUCK_ADDRESS)) {
    uint8_t joyX = nunchuck.state.joy_x_axis;
    uint8_t joyY = nunchuck.state.joy_y_axis;

    if (joyX < 100 &&
        snakeDirection != LEFT) // TODO: magic numbers van joystick activatie
                                // weghalen voor consts
      snakeDirection = RIGHT;
    else if (joyX > 150 && snakeDirection != RIGHT) // TODO: same
      snakeDirection = LEFT;
    else if (joyY < 100 && snakeDirection != DOWN) // TODO: same
      snakeDirection = UP;
    else if (joyY > 150 && snakeDirection != UP) // TODO: same
      snakeDirection = DOWN;
  }
}

void moveSnake() {
  // Store the position of the tail (to clear later)
  uint8_t tailX = snakeX[snakeLength - 1];
  uint8_t tailY = snakeY[snakeLength - 1];

  // Shift snake body
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  // Move snake head
  switch (snakeDirection) {
  case UP:
    snakeY[0]++;
    break;
  case DOWN:
    snakeY[0]--;
    break;
  case RIGHT:
    snakeX[0]--;
    break;
  case LEFT:
    snakeX[0]++;
    break;
  }

  // Clear the old cells before drawing new snake
  drawSnakeCell(tailX * CELL_WIDTH, tailY * CELL_HEIGHT, BLACK);
}

void drawSnake() {
  // Draw the snake
  for (int i = 0; i < snakeLength; i++) {
    drawSnakeCell(snakeX[i] * CELL_WIDTH, snakeY[i] * CELL_HEIGHT, GREEN);
  }
}

// check true if snake hits itself or a border
bool checkCollision() {
  // Check for a collision with a border
  if (snakeX[0] < 0 || snakeX[0] >= GRID_SIZE || snakeY[0] < 0 ||
      snakeY[0] >= GRID_SIZE) {
    return true;
  }

  // Check collision with itslef
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      return true;
    }
  }

  return false;
}

// Function to init screen
void initializeScreen() {
  screen.begin();
  screen.fillScreen(BLACK);
}

// Function to init nunchuck
void initializeNunchuk() { Wire.begin(); }

// Function to map joystick values to screen coordinates
uint16_t mapJoystickToScreen(uint8_t joystickValue, uint16_t screenSize) {
  return map(joystickValue, 0, 255, BALL_RADIUS, screenSize - BALL_RADIUS);
}

int main() {
  init();
  Serial.begin(9600);

  initializeScreen();
  initializeNunchuk();

  snakeX[0] = GRID_SIZE / 2;
  snakeY[0] = GRID_SIZE / 2;

  // Infinite loop
  while (1) {

    updateDirection();
    moveSnake();

    // TODO: tussen het bewegen en het tekenen checken voor een collision. Dan
    // kan je het spel stoppen voordat het grafisch slecht loopt

    if (checkCollision()) {
      screen.fillScreen(BLACK);
      screen.setCursor(60, TFT_HEIGHT / 2);
      screen.setTextColor(RED);
      screen.setTextSize(2);
      screen.print("Game Over!");
    }

    drawSnake();

    // Small delay to avoid overwhelming updates
    _delay_ms(200);
  }

  // Never reach
  return 0;
}
