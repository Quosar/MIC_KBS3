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

// Function to init screen
void initializeScreen() {
  screen.begin();
  screen.fillScreen(BLACK);
}

// Function to init nunchuck
void initializeNunchuk() {
  Wire.begin();
  nunchuck.begin(NUNCHUCK_ADDRESS);
}

// Function to map joystick values to screen coordinates
uint16_t mapJoystickToScreen(uint8_t joystickValue, uint16_t screenSize) {
  return map(joystickValue, 0, 255, BALL_RADIUS, screenSize - BALL_RADIUS);
}

// Function to draw a ball
void drawBall(uint16_t x, uint16_t y, uint16_t color) { // using forloops for drawing circles found online
  for (int16_t i = -BALL_RADIUS; i <= BALL_RADIUS; i++) {
    for (int16_t j = -BALL_RADIUS; j <= BALL_RADIUS; j++) {
      if (i * i + j * j <= BALL_RADIUS * BALL_RADIUS) {
        screen.drawPixel(x + i, y + j, color); //draw pixel is less intensive than clearing and filling rect
      }
    }
  }
}

int main() {
  init();
  Serial.begin(9600);

  initializeScreen();
  initializeNunchuk();

  // Ball position
  uint16_t posX = TFT_WIDTH / 2, posY = TFT_HEIGHT / 2;
  uint16_t lastPosX = posX, lastPosY = posY;

  // Draw start ball
  drawBall(posX, posY, RED);

  // Infinite loop
  while (1) {
    // Update Nunchuk state
    if (nunchuck.getState(NUNCHUCK_ADDRESS)) {
      // Get joystick positions
      uint8_t joyX = nunchuck.state.joy_x_axis;
      uint8_t joyY = nunchuck.state.joy_y_axis;

      // Map joystick values to screen coordinates
      posX = mapJoystickToScreen(joyX, TFT_WIDTH);
      posY = mapJoystickToScreen(joyY, TFT_HEIGHT);

      // Update ball position only if it changes (gets rid of flickering)
      if (posX != lastPosX || posY != lastPosY) {
        // remove the old ball
        drawBall(lastPosX, lastPosY, BLACK);

        // Draw the new ball
        drawBall(posX, posY, RED);

        // Update last position
        lastPosX = posX;
        lastPosY = posY;
      }
    }

    // Small delay to avoid overwhelming updates
    delay(10);
  }

  // Never reach
  return 0;
}
