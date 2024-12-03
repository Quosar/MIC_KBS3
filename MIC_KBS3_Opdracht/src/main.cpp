#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Nunchuk.h"
#include <Arduino.h>
#include <Snake.h>
#include <Wire.h>
#include <avr/interrupt.h>
#include <avr/io.h>

// LCD Pin Defines
#define TFT_CLK 13
#define TFT_MISO 12
#define TFT_MOSI 11
#define TFT_DC 9
#define TFT_CS 10
#define TFT_RST 8

// TFT defines
const uint8_t GRID_SIZE = 16;
const uint8_t TFT_WIDTH = 240;
const uint16_t TFT_HEIGHT = 320;

// Color defines
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

// IR Pins
#define IR_TRANSMITTER_PIN PD6
#define IR_RECEIVER_PIN PD2

// I2C adres voor 7-segment display
uint8_t SEVEN_SEGMENT_ADDRESS = 0x21;

// LCD object
Adafruit_ILI9341 screen(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

NunChuk nunchuck;
bool zPressed;
const uint8_t NUNCHUCK_ADDRESS = 0x52;

// Create Snake object
Snake snake(GRID_SIZE, TFT_WIDTH / GRID_SIZE, TFT_WIDTH / GRID_SIZE, screen, BLUE);
bool gameOver = false;



// Verstuur een cijfer naar het 7-segment display via I2C
void sendToSegmentDisplay(uint8_t value) {
  Wire.beginTransmission(SEVEN_SEGMENT_ADDRESS);
  if (value == 0) {
    Wire.write(0b11000000); // 0 displayen
  } else if (value == 1) {
    Wire.write(0b11111001); // 1 displayen
  }
  Wire.endTransmission();
}

void restartGame() {
  gameOver = false;
  snake.reset();
}

int main() {
  Wire.begin();
  sei(); // Globale interrupts aan

  // Zet PD2 als input
  DDRD &= ~(1 << IR_RECEIVER_PIN);

  // LCD setup
  screen.begin();
  screen.fillScreen(BLACK);
  screen.setTextSize(2);

  snake.start(); //start de snake in het midden van het scherm

  while (1) {
    // Checken of nunchuck Z-knop is ingedrukt
    if (nunchuck.getState(NUNCHUCK_ADDRESS)) {
      if (gameOver && nunchuck.state.z_button) {
        restartGame();
      } else if (!gameOver) {
        snake.updateDirection(nunchuck.state.joy_x_axis,
                              nunchuck.state.joy_y_axis);
      }
    }
    
    if (!gameOver) {
      snake.drawScore();

      if(snake.eatApple(snake.appleX, snake.appleY)){
        snake.grow(); // snakelengte groeien
      }

      snake.move();

      if (snake.checkCollision()) {
        gameOver = true;
        snake.drawDeathScreen();
      } else {
        snake.draw();
      }
    }

    _delay_ms(150); // game speed
  }

  return 0;
}
