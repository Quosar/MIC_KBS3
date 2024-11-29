#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Nunchuk.h"
#include "Snake.h"
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

const int8_t NUNCHUCK_ADDRESS = 0x52;

// Screen dimensions
const uint16_t TFT_WIDTH = 240;
const uint16_t TFT_HEIGHT = 320;
const uint8_t GRID_SIZE = 16;

// Create TFT and Nunchuk objects
Adafruit_ILI9341 screen(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
NunChuk nunchuck;

// Create Snake object
Snake snake(GRID_SIZE, TFT_WIDTH / GRID_SIZE, TFT_HEIGHT / GRID_SIZE, screen);
bool gameOver = false;

void initialiseScreen() {
  screen.begin();
  screen.fillScreen(BLACK);
}

void restartGame() {
  gameOver = false;
  snake.reset();
}

int main() {
  init();
  Wire.begin();       // start wire for nunchuck
  initialiseScreen(); // init the screen

  snake.start(); // start snake on middle of the screen

  while (1) {
    if (nunchuck.getState(NUNCHUCK_ADDRESS)) {
      if (gameOver && nunchuck.state.z_button) {
        restartGame();
      } else if (!gameOver) {
        snake.updateDirection(nunchuck.state.joy_x_axis,
                              nunchuck.state.joy_y_axis);
      }
    }
    if (!gameOver) {
      snake.move();
      snake.eatApple(snake.appleX, snake.appleY);
      if (snake.checkCollision()) {
        gameOver = true;
        screen.fillScreen(BLACK);
        screen.setCursor(60, TFT_HEIGHT / 2);
        screen.setTextColor(RED);
        screen.setTextSize(2);
        screen.println("Game Over!");
        screen.println("PRESS Z TO CONTINUE");
      } else {
        snake.draw();
      }
    }
    _delay_ms(200); // game speed
  }

  return 0;
}
