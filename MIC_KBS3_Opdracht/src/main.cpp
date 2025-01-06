#include "Display.h"
#include "HardwareSerial.h"
#include "Nunchuk.h"
#include <Arduino.h>
#include <Communication.h>
#include <Snake.h>
#include <Wire.h>
#include <avr/interrupt.h>
#include <avr/io.h>

// TFT defines
const uint8_t LARGE_FIELD_GRID_SIZE = 16;
const uint8_t SMALL_FIELD_GRID_SIZE = 8;
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

#define DSCREEN_MENU_X 35
#define DSCREEN_MENU_Y 180
#define DSCREEN_PAGAIN_X 125
#define DSCREEN_PAGAIN_Y 180

// LCD object
Display screen;

// Nunchuk Object
NunChuk nunchuck;
const uint8_t NUNCHUCK_ADDRESS = 0x52;

Communication communication;

// Create Snake object
Snake largeFieldSnake(LARGE_FIELD_GRID_SIZE, TFT_WIDTH / LARGE_FIELD_GRID_SIZE,
                      TFT_WIDTH / LARGE_FIELD_GRID_SIZE, screen, GREEN,
                      communication.getSender(), communication.getSender());

Snake smallFieldSnake(SMALL_FIELD_GRID_SIZE, TFT_WIDTH / SMALL_FIELD_GRID_SIZE,
                      TFT_WIDTH / SMALL_FIELD_GRID_SIZE, screen, GREEN,
                      communication.getSender(), !communication.getSender());

Snake largeFieldSnakeOther(LARGE_FIELD_GRID_SIZE,
                           TFT_WIDTH / LARGE_FIELD_GRID_SIZE,
                           TFT_WIDTH / LARGE_FIELD_GRID_SIZE, screen, MAGENTA,
                           !communication.getSender(),
                           communication.getSender());
// Snake smallFieldSnakeOther(SMALL_FIELD_GRID_SIZE, TFT_WIDTH /
// SMALL_FIELD_GRID_SIZE,
//                       TFT_WIDTH / SMALL_FIELD_GRID_SIZE, screen, MAGENTA,
//                       !communication.getSender(),
//                       !communication.getSender());

// Game Size (8x8 / 16x16)
enum gameSize { SIZE8x8, SIZE16x16 };
gameSize currentGameSize = SIZE16x16;
gameSize previousGameSize = SIZE16x16;

enum gameSpeed { NORMAL, FAST };
gameSpeed currentGameSpeed = NORMAL;
gameSpeed previousGameSpeed = NORMAL;

// Game Speed (Normal / Fast)
volatile bool isFastMode = false;
volatile bool previousFastMode = false;

volatile bool isWinner = false;

// Touch Screen Positions
volatile uint16_t touchX = 0;
volatile uint16_t touchY = 0;
volatile bool isTouching = false;
volatile bool previousTouch = false;

enum gameState { MENU, START, INGAME, DEATH, REDRAW };
volatile gameState currentState = MENU;
volatile gameState previousState = DEATH;

bool multiplayer = true;

void updateGame(Snake &snake) {
  snake.move(); // Move snake based on received direction
  snake.draw();
  snake.drawScore();
  if (communication.appleGatheredByPlayer2) {
    communication.appleGatheredByPlayer2 = false;
    if (communication.getSender()) {
      largeFieldSnake.spawnRandApple();
      communication.posApple = largeFieldSnake.getApplePosition();
      largeFieldSnakeOther.setApplePosition(communication.posApple);
    }
  }
  if (!communication.getSender()) {
    largeFieldSnake.setApplePosition(communication.posAppleOther);
    largeFieldSnakeOther.setApplePosition(communication.posAppleOther);
  }
  if (snake.eatApple(snake.appleX, snake.appleY)) {
    snake.grow();
    if (communication.getSender()) {
      largeFieldSnake.spawnRandApple();
      communication.posApple = largeFieldSnake.getApplePosition();
      largeFieldSnakeOther.setApplePosition(communication.posApple);
    } else {
      communication.appleGatheredByPlayer2 = true;
    }
  }
  if (communication.getSender()) {
    if (snake.checkCollision(largeFieldSnake)) {
      communication.gameRunning = false;
      if (communication.getSender()) {
        if (snake.isPrimarySnake) {
          isWinner = true;
        } else {
          isWinner = false;
        }
      } else {
        if (snake.isPrimarySnake) {
          isWinner = false;
        } else {
          isWinner = true;
        }
      }
      currentState = DEATH;
    }
  } else {
    if (snake.checkCollision(largeFieldSnakeOther)) {
      communication.gameRunning = false;
      if (communication.getSender()) {
        if (snake.isPrimarySnake) {
          isWinner = true;
        } else {
          isWinner = false;
        }
      } else {
        if (snake.isPrimarySnake) {
          isWinner = false;
        } else {
          isWinner = true;
        }
      }
      currentState = DEATH;
    }
  }
}

void directionHandler() {
  if (nunchuck.getState(NUNCHUCK_ADDRESS)) {
    if (currentGameSize == SIZE16x16) {
      if (communication.getSender()) {
        largeFieldSnakeOther.updateDirection(nunchuck.state.joy_x_axis,
                                             nunchuck.state.joy_y_axis);
        largeFieldSnakeOther.validateDirection();
        if (communication.snakeDirectionOther == 0) {
          largeFieldSnake.bufferedDirection = largeFieldSnake.UP;
        } else if (communication.snakeDirectionOther == 1) {
          largeFieldSnake.bufferedDirection = largeFieldSnake.DOWN;
        } else if (communication.snakeDirectionOther == 2) {
          largeFieldSnake.bufferedDirection = largeFieldSnake.LEFT;
        } else if (communication.snakeDirectionOther == 3) {
          largeFieldSnake.bufferedDirection = largeFieldSnake.RIGHT;
        }
        largeFieldSnake.validateDirection();
      } else {
        largeFieldSnake.updateDirection(nunchuck.state.joy_x_axis,
                                        nunchuck.state.joy_y_axis);
        largeFieldSnake.validateDirection();
        if (communication.snakeDirectionOther == 0) {
          largeFieldSnakeOther.bufferedDirection = largeFieldSnakeOther.UP;
        } else if (communication.snakeDirectionOther == 1) {
          largeFieldSnakeOther.bufferedDirection = largeFieldSnakeOther.DOWN;
        } else if (communication.snakeDirectionOther == 2) {
          largeFieldSnakeOther.bufferedDirection = largeFieldSnakeOther.LEFT;
        } else if (communication.snakeDirectionOther == 3) {
          largeFieldSnakeOther.bufferedDirection = largeFieldSnakeOther.RIGHT;
        }
        largeFieldSnakeOther.validateDirection();
      }
    } else {
      smallFieldSnake.updateDirection(nunchuck.state.joy_x_axis,
                                      nunchuck.state.joy_y_axis);
      smallFieldSnake.validateDirection();
    }
  }
}

// TODO: magic numbers weghalen
int8_t calculateFrameCount(uint8_t snakeLength) {
  int8_t frameCount;
  if (currentGameSpeed == NORMAL) {
    frameCount = 10 - (snakeLength / 5);
  } else if (currentGameSpeed == FAST) {
    frameCount = 5 - (snakeLength / 5);
  }
  if (frameCount < 2)
    frameCount = 2; // maximum snelheid
  if (frameCount > 8)
    frameCount = 8; // minimum snelheid

  return (int8_t)frameCount;
}

// game logic die geloopt moet worden
void handleState() {
  switch (currentState) {
  case MENU:
    break;

  case INGAME:
    // snelheid aanpassen op lengte slang
    if (currentGameSize == SIZE16x16) {
      // communication.communicationFrameCount =
      //     calculateFrameCount(largeFieldSnake.snakeLength);
      communication.communicationFrameCount = 12;
      updateGame(largeFieldSnakeOther);
      updateGame(largeFieldSnake);
    } else {
      // communication.communicationFrameCount =
      //     calculateFrameCount(smallFieldSnake.snakeLength);
      updateGame(smallFieldSnake);
    }

    break;
  case DEATH:
    break;
  case REDRAW:
    break;
  case START:
    break;
  }
  communication.runFrame = false;
}

long mapValue(long x, long inMin, long inMax, long outMin, long outMax) {
  return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

const uint8_t SEVEN_SEGMENT_ADRES = 0x21;
const uint8_t SEVEN_SEGMENT_DIGITS[] = {
    0xC0, // 0
    0xF9, // 1
    0xA4, // 2
    0xB0, // 3
    0x99, // 4
    0x92, // 5
    0x82, // 6
    0xF8, // 7
    0x80, // 8
    0x90, // 9
    0x88, // A
    0x83, // B
    0xC6, // C
    0xA1, // D
    0x86, // E
    0x8E  // F
};

void updateSevenSegmentDisplay(uint8_t value) {
  Wire.beginTransmission(SEVEN_SEGMENT_ADRES);
  Wire.write(SEVEN_SEGMENT_DIGITS[value]);
  Wire.endTransmission(SEVEN_SEGMENT_ADRES);
}

// game logic die alleen gedaan moet worden wanneer je net in deze gamestate
// komt
void handleStateChange() {
  if (currentState != previousState) {
    switch (currentState) {
    case MENU:
      updateSevenSegmentDisplay(0);
      if (previousState == REDRAW) {
        if (currentGameSize == SIZE8x8) {
          screen.drawElement(3, true, false, false, false);
          screen.drawElement(4, false, false, false, false);
        } else if (currentGameSize == SIZE16x16) {
          screen.drawElement(3, false, false, false, false);
          screen.drawElement(4, true, false, false, false);
        }

        if (isFastMode != previousFastMode) {
          if (isFastMode) {
            screen.drawElement(5, true, false, true, false);
          } else {
            screen.drawElement(5, false, false, true, false);
          }
          previousFastMode = isFastMode;
        }
      } else {
        screen.drawStartMenu();
      }
      break;

    case START:
      screen.fillScreen(BLACK);
      if (currentGameSize == SIZE16x16) {
        largeFieldSnakeOther.reset();
        largeFieldSnakeOther.start((LARGE_FIELD_GRID_SIZE - 2),
                                   (LARGE_FIELD_GRID_SIZE - 2));
        largeFieldSnake.reset();
        largeFieldSnake.start(2, 2);
        if (currentGameSpeed == NORMAL) {
          updateSevenSegmentDisplay(1);
        } else {
          updateSevenSegmentDisplay(2);
        }
      } else {
        if (currentGameSpeed == NORMAL) {
          updateSevenSegmentDisplay(3);
        } else {
          updateSevenSegmentDisplay(4);
        }
        smallFieldSnake.reset();
        smallFieldSnake.start(SMALL_FIELD_GRID_SIZE / 1,
                              SMALL_FIELD_GRID_SIZE / 1);
      }

      // teken border 1 pixel onder speelveld
      screen.drawLine(0, TFT_WIDTH + 1, TFT_WIDTH, TFT_WIDTH + 1, WHITE);
      previousGameSpeed = currentGameSpeed;
      previousGameSize = currentGameSize;
      currentState = INGAME;

      break;

    case DEATH:
      communication.gameRunning = false;
      if (communication.getSender()) {
        screen.drawDeathScreen(isWinner, largeFieldSnakeOther.snakeLength,
                               largeFieldSnake.snakeLength);
      } else {
        screen.drawDeathScreen(isWinner, largeFieldSnake.snakeLength,
                               largeFieldSnakeOther.snakeLength);
      }
      currentGameSpeed = NORMAL;
      currentGameSize = SIZE16x16;
      break;

    case INGAME:
      break;

    case REDRAW:
      break;
    }
    previousState = currentState;
  }
}

void handleDeathscreenTouch() {
  if (isTouching) {
    if (isTouching != previousTouch) {
      // Check in bounds

      // Mode 1
      if ((touchX >= DSCREEN_MENU_X && touchX <= DSCREEN_MENU_X + 80) &&
          (touchY >= DSCREEN_MENU_Y && touchY <= DSCREEN_MENU_Y + 60)) {
        currentState = MENU;
      }

      // Mode 2
      if ((touchX >= DSCREEN_PAGAIN_X && touchX <= DSCREEN_PAGAIN_X + 80) &&
          (touchY >= DSCREEN_PAGAIN_Y && touchY <= DSCREEN_PAGAIN_Y + 60)) {
        currentGameSize = previousGameSize;
        currentGameSpeed = previousGameSpeed;
        currentState = START;
      }

      previousTouch = true;
    }

  } else {
    previousTouch = false;
  }
}

void handleMenuTouch() {
  if (isTouching) {
    if (isTouching != previousTouch) {
      // Check in bounds

      // Mode 1
      if ((touchX >= MENU_MODE1_X && touchX <= MENU_MODE1_X + 100) &&
          (touchY >= MENU_MODE1_Y && touchY <= MENU_MODE1_Y + 20)) {
        currentGameSize = SIZE8x8;
        previousState = REDRAW;
      }

      // Mode 2
      if ((touchX >= MENU_MODE2_X && touchX <= MENU_MODE2_X + 100) &&
          (touchY >= MENU_MODE2_Y && touchY <= MENU_MODE2_Y + 20)) {
        currentGameSize = SIZE16x16;
        previousState = REDRAW;
      }

      // Mode 3
      if ((touchX >= MENU_MODE3_X && touchX <= MENU_MODE3_X + 100) &&
          (touchY >= MENU_MODE3_Y && touchY <= MENU_MODE3_Y + 20)) {
        isFastMode = !isFastMode;
        if (isFastMode) {
          currentGameSpeed = FAST;
        } else {
          currentGameSpeed = NORMAL;
        }
        previousState = REDRAW;
      }

      // Select Player 1 + Show Start Button
      if ((touchX >= MENU_PLR1_X && touchX <= MENU_PLR1_X + 100) &&
          (touchY >= MENU_PLR1_Y && touchY <= MENU_PLR1_Y + 20)) {
        // communication.isPlayer1 = true;
        screen.drawElement(1, true, true, true, false);
      }

      // For Testing, this does not require player1/player2 selection and is
      // prerendered
      if ((touchX >= MENU_START_X && touchX <= MENU_START_X + 140) &&
          (touchY >= MENU_START_Y && touchY <= MENU_START_Y + 70)) {
        // Detection
        communication.gameRunning = true;
        currentState = START;
      }

      previousTouch = true;
    }

  } else {
    previousTouch = false;
  }
}

void touchHandler() {
  TS_Point p = screen.getPoint();
  touchX = mapValue(p.x, 0, 240, 240, 0);
  touchY = mapValue(p.y, 0, 320, 320, 0);
  isTouching = p.z > 0;

  switch (currentState) {
  case MENU:
    handleMenuTouch(); // handle touch van menu input elke loop
    break;
  case INGAME:
    directionHandler();
    break;
  case DEATH:
    handleDeathscreenTouch();
    break;
  case REDRAW:
    break;
  case START:
    break;
  }
}

int main() {
  init();
  Wire.begin();
  Serial.begin(9600);

  // aparte setups voor testen
  communication.setupPins();
  communication.setupTimers();
  communication.SetupInterrupts();
  communication.initializeCommunication();

  screen.begin();

  sei();

  while (1) {
    if (communication.communicationInitialized) {
      touchHandler();
      screen.refreshBacklight();
      if (communication.printBus) {
        if (communication.getSender()) {
          communication.outBus =
              communication.constructBus(largeFieldSnakeOther);
          communication.deconstructBus(communication.inBus, largeFieldSnake);
          largeFieldSnake.validateDirection();
          Serial.println(largeFieldSnake.direction);
        } else {
          communication.outBus = communication.constructBus(largeFieldSnake);
          communication.deconstructBus(communication.inBus,
                                       largeFieldSnakeOther);
          largeFieldSnakeOther.validateDirection();
        }
      }
      if (communication.runFrame) {
        if (communication.gameRunning && currentState != INGAME) {
          currentState = START;
          communication.gameRunning = false;
        }
        handleStateChange();
        handleState();
      }
    }
  }
  return 0;
}

ISR(TIMER1_COMPA_vect) { communication.communicate(); }

ISR(INT0_vect) {
  TCNT1 = 100;
  communication.busBitIndex = 0;
  EIMSK &= ~(1 << INT0); // INT0 interrupt disable
  TIMSK1 |= (1 << OCIE1A);
}
