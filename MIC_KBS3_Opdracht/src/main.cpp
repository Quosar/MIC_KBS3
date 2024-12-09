#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Adafruit_FT6206.h"
#include "Nunchuk.h"
#include <Arduino.h>
#include <Snake.h>
#include <Wire.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "HardwareSerial.h"

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

// Menu Position Defines
// TODO: Convert to consts
#define MENU_PLR1_X 10
#define MENU_PLR1_Y 45
#define MENU_PLR2_X 130
#define MENU_PLR2_Y 45
#define MENU_MODE1_X 70
#define MENU_MODE1_Y 90
#define MENU_MODE2_X 70
#define MENU_MODE2_Y 115
#define MENU_MODE3_X 70
#define MENU_MODE3_Y 140
#define MENU_START_X 70
#define MENU_START_Y 180

// IR Pins
#define IR_TRANSMITTER_PIN PD6
#define IR_RECEIVER_PIN PD2

// LCD object
Adafruit_ILI9341 screen(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

// Touchscreen object
Adafruit_FT6206 ts;

// Nunchuk Object
NunChuk nunchuck;
const uint8_t NUNCHUCK_ADDRESS = 0x52;

// Create Snake object
Snake snake(GRID_SIZE, TFT_WIDTH / GRID_SIZE, TFT_WIDTH / GRID_SIZE, screen,
            MAGENTA);

// game state tracken
enum gameState { MENU, START, INGAME, DEATH, REDRAW };
gameState currentState = MENU;
gameState previousState = DEATH;

// Game Size (8x8 / 16x16)
enum gameMode {SIZE8x8, SIZE16x16};
gameMode currentMode = SIZE16x16;

// Game Speed (Normal / Fast)
volatile bool isFastMode = false;
volatile bool previousFastMode = false;

// Touch Screen Positions
volatile uint16_t touchX = 0;
volatile uint16_t touchY = 0;
volatile bool isTouching = false;
volatile bool previousTouch = false;

volatile bool isPlayer1 = false;

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
    if (isTouching) {
      if (isTouching != previousTouch) {
        // Check in bounds

        // Mode 1
        if ((touchX >= MENU_MODE1_X && touchX <= MENU_MODE1_X + 100) && (touchY >= MENU_MODE1_Y && touchY <= MENU_MODE1_Y + 20)) {
          currentMode = SIZE8x8;
          previousState = REDRAW;
        }
        
        // Mode 2
        if ((touchX >= MENU_MODE2_X && touchX <= MENU_MODE2_X + 100) && (touchY >= MENU_MODE2_Y && touchY <= MENU_MODE2_Y + 20)) {
          currentMode = SIZE16x16;
          previousState = REDRAW;
        }

        // Mode 3
        if ((touchX >= MENU_MODE3_X && touchX <= MENU_MODE3_X + 100) && (touchY >= MENU_MODE3_Y && touchY <= MENU_MODE3_Y + 20)) {
          isFastMode = !isFastMode;
          previousState = REDRAW;
        }

        // Select Player 1 + Show Start Button
        if ((touchX >= MENU_PLR1_X && touchX <= MENU_PLR1_X + 100) && (touchY >= MENU_PLR1_Y && touchY <= MENU_PLR1_Y + 20)) {
          isPlayer1 = true;
          snake.drawElement(1, true, true, true, false);
          //snake.drawElement(6, false, true, true, false);   // For testing, this is disabled
        }

        // For Testing, this does not require player1/player2 selection and is prerendered
        if ((touchX >= MENU_START_X && touchX <= MENU_START_X + 140) && (touchY >= MENU_START_Y && touchY <= MENU_START_Y + 70)) {
          // Detection 
        }

        previousTouch = true;
      }
      
    } else {
      previousTouch = false;
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
  case REDRAW:
    break;
  }
  

}

// game logic die alleen gedaan moet worden wanneer je net in deze gamestate
// komt
void handleStateChange() {
  if (currentState != previousState) {
    switch (currentState) {
    case MENU:
      if (previousState == REDRAW) {
        if (currentMode == SIZE8x8) {
          snake.drawElement(3, true, false, false, false);
          snake.drawElement(4, false, false, false, false);
        } else if (currentMode == SIZE16x16) {
          snake.drawElement(3, false, false, false, false);
          snake.drawElement(4, true, false, false, false);
        }

        if (isFastMode != previousFastMode) {
          if (isFastMode) {
            snake.drawElement(5, true, false, true, false);
          } else {
            snake.drawElement(5, false, false, true, false);
          }
          previousFastMode = isFastMode;
        }
      } else {
        snake.drawStartMenu();
        //snake.drawMode1(true);
      }
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

    case INGAME:
      break;
    
    case REDRAW:
      break;
    }
    previousState = currentState;
  }
}

int main() {
  init();
  Wire.begin();

  //Serial.begin(9600);
  sei(); // Globale interrupts aan

  // Zet PD2 als input
  DDRD &= ~(1 << IR_RECEIVER_PIN);

  // LCD setup

  screen.begin();
  screen.setTextSize(2);

  ts.begin();

  while (1) {
    TS_Point p = ts.getPoint();

    p.x = map(p.x, 0, 240, 240, 0);
    p.y = map(p.y, 0, 320, 320, 0);

    touchX = p.x;
    touchY = p.y;

    //Serial.println(touchX);
    //Serial.println(touchY);

    if (p.z > 0) {
      isTouching = true;
    } else {
      isTouching = false;
    }
    
    handleStateChange();
    handleState();

    _delay_ms(150); // game speed
  }

  return 0;
}
