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

// LCD Display Width and Height Limits
const uint16_t TFT_WIDTH = 240;
const uint16_t TFT_HEIGHT = 320;

// Offset to set center of ball from top-left corner
const uint8_t BALL_OFFSET_X = 6;
const uint8_t BALL_OFFSET_Y = 6;

const int NUNCHUCK_ADDRESS = 0x52;
NunChuk nunchuck;

// Function to map joystick value to screen coordinates
uint16_t mapJoystickToScreen(uint8_t joystickValue, uint16_t screenSize) {
  return map(joystickValue, 0, 255, BALL_OFFSET_X, screenSize - BALL_OFFSET_X);
}

int main() {
  init();
  Serial.begin(9600); // Start Serial

  // Initialize I2C for Nunchuk
  Wire.begin();
  // Initialize the Nunchuk
  if (!nunchuck.begin(NUNCHUCK_ADDRESS)) {
    nunchuck.begin(NUNCHUCK_ADDRESS);
  }

  // Create Screen Object
  Adafruit_ILI9341 screen =
      Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

  // Initialize Screen
  screen.begin();
  screen.fillScreen(BLACK);

  // Ball Position
  uint16_t posX = TFT_WIDTH / 2;
  uint16_t posY = TFT_HEIGHT / 2;

  // Last X/Y position of Ball
  uint16_t lastPosX = posX;
  uint16_t lastPosY = posY;

  // Superloop
  while (1) {
    // Update Nunchuk state
    if (nunchuck.getState(NUNCHUCK_ADDRESS)) {
      // Get joystick positions
      uint8_t joyX = nunchuck.state.joy_x_axis;
      uint8_t joyY = nunchuck.state.joy_y_axis;

      // Map joystick values to screen coordinates
      posX = mapJoystickToScreen(joyX, TFT_WIDTH);
      posY = mapJoystickToScreen(joyY, TFT_HEIGHT);

      // Clear the last ball position
      screen.fillRect(lastPosX - BALL_OFFSET_X, lastPosY - BALL_OFFSET_Y,
                      BALL_OFFSET_X * 2, BALL_OFFSET_Y * 2, BLACK);

      // Draw the ball at the new position
      screen.fillRect(posX - BALL_OFFSET_X, posY - BALL_OFFSET_Y,
                      BALL_OFFSET_X * 2, BALL_OFFSET_Y * 2, RED);

      // Update last position
      lastPosX = posX;
      lastPosY = posY;
    }

    // Small delay to avoid overwhelming updates
    delay(10);
  }

  // Never reach
  return 0;
}
