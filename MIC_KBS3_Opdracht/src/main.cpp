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

#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF

/*
int main() {
  Adafruit_ILI9341 screen = Adafruit_ILI9341(TFT_CS, TFT_DC);

  Serial.begin(9600);

  screen.begin();

  screen.fillRect(50, 50, 10, 10, BLACK);

  screen.drawRect(50, 50, 10, 10, BLACK);
  
  return 0;
}
*/

void setup() {
  Adafruit_ILI9341 screen = Adafruit_ILI9341(TFT_CS, TFT_DC);

  Serial.begin(9600);

  screen.begin();

  screen.fillRect(50, 50, 10, 10, BLACK);

  screen.drawRect(50, 50, 25, 25, BLACK);
}

void loop() {

}