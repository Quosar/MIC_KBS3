#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define TFT_CLK 13
#define TFT_MISO 12
#define TFT_MOSI 11
#define TFT_DC 9
#define TFT_CS 10
#define TFT_RST 8

//#define TFT_WIDTH 240
//#define TFT_HEIGHT 320

#define PADDLE_OFFSET_X 6
#define PADDLE_OFFSET_Y 6

const uint8_t TFT_WIDTH = 240;
const uint16_t TFT_HEIGHT = 320;

/*
#define TFT_D0        34 // Data bit 0 pin (MUST be on PORT byte boundary)
#define TFT_WR        26 // Write-strobe pin (CCL-inverted timer output)
#define TFT_DC        10 // Data/command pin
#define TFT_CS        11 // Chip-select pin
#define TFT_RST       24 // Reset pin
#define TFT_RD         9 // Read-strobe pin
#define TFT_BACKLIGHT 25
*/

#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF

//Adafruit_ILI9341 screen = Adafruit_ILI9341(TFT_CS, TFT_DC);


int main() {
  Serial.begin(9600);
  while(!Serial);
  Serial.println("ILI9341 Test!"); 

  Adafruit_ILI9341 screen = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

  screen.begin();

  Serial.println("Screen booted");

  screen.fillScreen(BLACK);

  uint8_t dirX = 1;
  uint8_t dirY = 1;
  uint8_t posX = 1;
  uint8_t posY = 1;

  uint16_t lastPosX = 1;
  uint16_t lastPosY = 1;

  //screen.fillRect(200, 300, 20, 20, BLUE);

  // Superloop
  while (1) {
    for (int i=0; i < 999; i++) {
      if (posX >= TFT_WIDTH / 2) {
        dirX = -1;
      } else if (posX <= 1) {
        dirX = 1;
      }

      if (posY >= TFT_HEIGHT / 2) {
        //Serial.println("no");
        dirY = -1;
      } else if (posY <= 1) {
        dirY = 1;
      }

      posX += dirX;
      posY += dirY;

      //Serial.println(posY);

      screen.fillRect(lastPosX, lastPosY, PADDLE_OFFSET_X * 2, PADDLE_OFFSET_Y * 2, BLACK);

      screen.fillRect(posX * 2 - PADDLE_OFFSET_X, posY * 2 - PADDLE_OFFSET_Y, PADDLE_OFFSET_X * 2, PADDLE_OFFSET_Y * 2, RED);
      
      lastPosX = posX * 2 - PADDLE_OFFSET_X;
      lastPosY = posY * 2 - PADDLE_OFFSET_Y;
      
      //Serial.println("Drawn Square");
      _delay_ms(10);
    }
  }
  
  // Never reach
  return 0;
}