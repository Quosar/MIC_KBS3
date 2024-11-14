#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// LCD Pin Defines
#define TFT_CLK 13
#define TFT_MISO 12
#define TFT_MOSI 11
#define TFT_DC 9
#define TFT_CS 10
#define TFT_RST 8

// Color defines
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF

// LCD Display Width and Height Limits
const uint16_t TFT_WIDTH = 240;
const uint16_t TFT_HEIGHT = 320;

// Offset to set centre of ball from top-left corner
const uint8_t BALL_OFFSET_X = 6;
const uint8_t BALL_OFFSET_Y = 6;

// Main
int main() {
  
  Serial.begin(9600);   // Start Serial
  while(!Serial);       // Wait for Start Serial

  // Create Screen Object
  Adafruit_ILI9341 screen = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

  // Initialize Screen
  screen.begin();

  // Set Screen color to Black
  screen.fillScreen(BLACK);

  // Direction and Position Vars for Ball Position
  uint8_t dirX = 1;
  uint8_t dirY = 1;
  uint8_t posX = 1;
  uint8_t posY = 1;

  // Last X/Y position of Ball
  uint16_t lastPosX = 1;
  uint16_t lastPosY = 1;

  // Superloop
  while (1) {
    for (int i=0; i < 999; i++) {

      // If Ball is beyond limits of X, Reverse Direction; Else dont
      if (posX >= TFT_WIDTH / 2) {
        dirX = -1;
      } else if (posX <= 1) {
        dirX = 1;
      }

      // If Ball is beyond limits of Y, Reverse Direction; Else dont
      if (posY >= TFT_HEIGHT / 2) {
        dirY = -1;
      } else if (posY <= 1) {
        dirY = 1;
      }

      // Add directional vector to position
      posX += dirX;
      posY += dirY;

      // Make the last position of the ball black
      screen.fillRect(lastPosX, lastPosY, BALL_OFFSET_X * 2, BALL_OFFSET_Y * 2, BLACK);

      // Create the new ball
      screen.fillRect(posX * 2 - BALL_OFFSET_X, posY * 2 - BALL_OFFSET_Y, BALL_OFFSET_X * 2, BALL_OFFSET_Y * 2, RED);
      
      // Set last position to current
      lastPosX = posX * 2 - BALL_OFFSET_X;
      lastPosY = posY * 2 - BALL_OFFSET_Y;
      
      // Wait 10ms
      // TODO: Remove this in favor of timer
      _delay_ms(10);
    }
  }
  
  // Never reach
  return 0;
}