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

// LCD object
Adafruit_ILI9341 screen(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

NunChuk nunchuck;
const uint8_t NUNCHUCK_ADDRESS = 0x52;

// Create Snake object
Snake snake(GRID_SIZE, TFT_WIDTH / GRID_SIZE, TFT_WIDTH / GRID_SIZE, screen,
            MAGENTA);

// game state tracken
enum gameState { MENU, START, INGAME, DEATH };
gameState currentState = MENU;
gameState previousState = DEATH;

void updateGame() {
  snake.move();      // snake pos updaten
  snake.draw();      // snake en appel tekenen
  snake.drawScore(); // score updaten
  if (snake.eatApple(snake.appleX, snake.appleY)) {
    snake.grow(); // snake groeien als appel gegeten is
  }
  if (snake.checkCollision()) {
    currentState = DEATH;
  }
}

// game logic die geloopt moet worden
void handleState() {
  switch (currentState) {
  case MENU:
    if (nunchuck.getState(NUNCHUCK_ADDRESS)) {
      if (nunchuck.state.z_button) {
        currentState = START;
      }
    }
    break;

  case INGAME:
    if (nunchuck.getState(NUNCHUCK_ADDRESS)) {
      snake.updateDirection(nunchuck.state.joy_x_axis,
                            nunchuck.state.joy_y_axis);
    }
    updateGame();
    break;

  case DEATH:
    if (nunchuck.getState(NUNCHUCK_ADDRESS)) {
      if (nunchuck.state.z_button) {
        currentState = START;
      }
    }
    break;
  }
}

// game logic die alleen gedaan moet worden wanneer je net in deze gamestate
// komt
void handleStateChange() {
  if (currentState != previousState) {
    switch (currentState) {
    case MENU:
      snake.drawStartMenu();
      break;

    case START:
      screen.fillScreen(BLACK);
      snake.start(GRID_SIZE / 2, GRID_SIZE / 2);
      currentState = INGAME;
      break;

    case DEATH:
      snake.reset();
      snake.drawDeathScreen();
      break;
    }
    previousState = currentState;
  }
}

int main() {
  init();
  Wire.begin();
  sei(); // Globale interrupts aan

  // Zet PD2 als input
  DDRD &= ~(1 << IR_RECEIVER_PIN);

  // LCD setup
  screen.begin();
  screen.setTextSize(2);

  while (1) {
    handleStateChange();
    handleState();

    _delay_ms(150); // game speed
  }

  return 0;
}
