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

#define IR_LED_PIN PD6         // Pin 6 voor IR LED (OC0A)
#define IR_RECEIVER_PIN PD2    // Pin 2 voor IR Receiver (INT0)
#define DATABITCOUNT 32        // bits in een databus
#define COMMUNICATIONSPEED 200 // snelheid van timer 0 interrupts
#define OCSILLATIONSPEED 209   // 38kHz oscilleer snelheid led pin
#define COMMUNICATIONOFFSETMIN 0

// LCD object
Display screen;

// Nunchuk Object
NunChuk nunchuck;
const uint8_t NUNCHUCK_ADDRESS = 0x52;

Communication communication;

// Create Snake object
Snake largeFieldSnake(LARGE_FIELD_GRID_SIZE, TFT_WIDTH / LARGE_FIELD_GRID_SIZE,
                      TFT_WIDTH / LARGE_FIELD_GRID_SIZE, screen, GREEN);

Snake smallFieldSnake(SMALL_FIELD_GRID_SIZE, TFT_WIDTH / SMALL_FIELD_GRID_SIZE,
                      TFT_WIDTH / SMALL_FIELD_GRID_SIZE, screen, GREEN);

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

// Touch Screen Positions
volatile uint16_t touchX = 0;
volatile uint16_t touchY = 0;
volatile bool isTouching = false;
volatile bool previousTouch = false;

// Sound Variable
volatile uint8_t soundFrames = 1;             // Pitch of the sound (higher is lower pitch)
volatile uint8_t currentSoundFrame = 1;       // Current frame of the sound
volatile uint8_t currentSoundIndex = 0;       // 
volatile uint8_t currentSoundFrameIndex = 0;

const uint8_t soundPitch = 150; // The pitch of the audio (the higher the number, the lower the pitch)
const uint8_t soundSpeedFactor = 1; // How fast the audio playback is (higher is slower)

const uint8_t soundMenu[][3] = {
  {1, 1, 20},
  {0, 1, 20},
  {1, 1, 20},
};

const uint8_t soundMenuSelect8x8[][3] = {
  {1, 3, 30},
  {0, 3, 30},
  {1, 2, 30},
};

const uint8_t soundMenuSelect16x16[][3] = {
  {1, 4, 20},
  {0, 4, 20},
  {1, 3, 20},
  {0, 3, 20},
  {1, 2, 20},
  {0, 2, 20},
};

const uint8_t soundStart[][3] = {
  {1, 4, 30},
  {1, 3, 45},
  {1, 2, 60},
  {1, 1, 75},
};

const uint8_t soundGrow[][3] = {
  {1, 3, 15},
  {0, 3, 15},
  {1, 2, 30},
  {0, 2, 30},
  {1, 1, 45},
  {0, 1, 45},
};

const uint8_t soundMove[][3] = {
  {1, 2, 25},
};

const uint8_t soundDeath[][3] = {
  {1, 1, 40},
  {0, 1, 40},
  {1, 2, 30},
  {0, 2, 30},
  {1, 3, 20},
  {0, 3, 20},
  {1, 4, 10},
  {0, 4, 10},
};

const uint8_t (*soundQueue)[3] = nullptr;

enum gameState { MENU, START, INGAME, DEATH, REDRAW };
volatile gameState currentState = MENU;
volatile gameState previousState = DEATH;

enum Sound {
  SOUND_EAT,
  SOUND_DEATH,
  SOUND_MOVE,
  SOUND_START,
  SOUND_MENU,
  SOUND_MENU8X8,
  SOUND_MENU16X16,
  SOUND_MENUNORMAL,
  SOUND_MENUFAST
};


void copyArrayArray(const uint8_t arrayOG[10][3], uint8_t newArray[10][3],
                    uint8_t arraySize, uint8_t playbackSpeed) {
  for (int i = 0; i < arraySize; i++) {
    for (int j = 0; j < 3; j++) {
      if (j == 2) {
        newArray[i][j] = arrayOG[i][j] * playbackSpeed;
      } else {
        newArray[i][j] = arrayOG[i][j];
      }
    }
  }
}

void selectSoundArray (Sound sound) {
  if (sound == SOUND_EAT) {
    soundQueue = soundGrow;
  } else if (sound == SOUND_START) {
    soundQueue = soundStart;
  } else if (sound == SOUND_MENU8X8) {
    soundQueue = soundMenuSelect8x8;
  } else if (sound == SOUND_MENU16X16) {
    soundQueue = soundMenuSelect16x16;
  } else if (sound == SOUND_MENU) {
    soundQueue = soundMenu;
  } else if (sound == SOUND_MOVE) {
    soundQueue = soundMove;
  } else if (sound == SOUND_DEATH) {
    soundQueue = soundDeath;
  } else if (sound == SOUND_MENUFAST) {
    soundQueue = soundMenu;
  } else if (sound == SOUND_MENUNORMAL) {
    soundQueue = soundMenu;
  }
}

void playSound(Sound sound) {
  TIMSK2 &= ~(1 << OCIE2A);

  soundQueue = nullptr;

  selectSoundArray(sound);

  // Copy over the sound array to a temporary array
  

  // Initialize sound array
  soundFrames = soundQueue[currentSoundIndex][1];

  // Enable Timer Compare
  // Floor Timer2 Val
  TCNT2 = 0;
  TIMSK2 |= (1 << OCIE2A);
}

void updateGame(Snake &snake) {
  snake.move(); // Move snake based on received direction
  snake.draw();
  snake.drawScore();
  playSound(SOUND_MOVE);
  if (snake.eatApple(snake.appleX, snake.appleY)) {
    playSound(SOUND_EAT);
    snake.grow();
  }
  if (snake.checkCollision()) {
    currentState = DEATH;
    playSound(SOUND_DEATH);
  }
}

void directionHandler() {
  if (nunchuck.getState(NUNCHUCK_ADDRESS)) {
    if (currentGameSize == SIZE16x16) {
      largeFieldSnake.updateDirection(nunchuck.state.joy_x_axis,
                                      nunchuck.state.joy_y_axis);
    } else {
      smallFieldSnake.updateDirection(nunchuck.state.joy_x_axis,
                                      nunchuck.state.joy_y_axis);
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
      communication.communicationFrameCount =
          calculateFrameCount(largeFieldSnake.snakeLength);
      updateGame(largeFieldSnake);
    } else {
      communication.communicationFrameCount =
          calculateFrameCount(smallFieldSnake.snakeLength);
      updateGame(smallFieldSnake);
    }

    break;
  case DEATH:
    break;
  case REDRAW:
    break;
  }
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
void handleStateChange(Snake &snake) {
  if (currentState != previousState) {
    switch (currentState) {
    case MENU:
      updateSevenSegmentDisplay(0);
      if (previousState == REDRAW) {
        if (currentGameSize == SIZE8x8) {
          screen.drawElement(3, true, false, false, false);
          screen.drawElement(4, false, false, false, false);
          playSound(SOUND_MENU8X8);
        } else if (currentGameSize == SIZE16x16) {
          screen.drawElement(3, false, false, false, false);
          screen.drawElement(4, true, false, false, false);
          playSound(SOUND_MENU16X16);
        }

        if (isFastMode != previousFastMode) {
          if (isFastMode) {
            screen.drawElement(5, true, false, true, false);
            playSound(SOUND_MENUFAST);
          } else {
            screen.drawElement(5, false, false, true, false);
            playSound(SOUND_MENUNORMAL);
          }
          previousFastMode = isFastMode;
        }
      } else {
        playSound(SOUND_MENU);
        screen.drawStartMenu();
      }
      break;

    case START:
      playSound(SOUND_START);
      screen.fillScreen(BLACK);
      if (currentGameSize == SIZE16x16) {
        if (currentGameSpeed == NORMAL) {
          updateSevenSegmentDisplay(1);
        } else {
          updateSevenSegmentDisplay(2);
        }

        largeFieldSnake.reset();
        largeFieldSnake.start(6, 6);
      } else {
        if (currentGameSpeed == NORMAL) {
          updateSevenSegmentDisplay(3);
        } else {
          updateSevenSegmentDisplay(4);
        }
        smallFieldSnake.reset();
        smallFieldSnake.start(SMALL_FIELD_GRID_SIZE / 2,
                              SMALL_FIELD_GRID_SIZE / 2);
      }

      // teken border 1 pixel onder speelveld
      screen.drawLine(0, TFT_WIDTH + 1, TFT_WIDTH, TFT_WIDTH + 1, WHITE);
      previousGameSpeed = currentGameSpeed;
      previousGameSize = currentGameSize;
      currentState = INGAME;

      break;

    case DEATH:
      playSound(SOUND_DEATH);
      screen.drawDeathScreen(true, 20, 20);
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
  }
}

int main() {
  init();
  Wire.begin();

  // aparte setups voor testen
  communication.setupPins();
  communication.setupTimers();
  communication.SetupInterrupts();
  communication.initializeCommunication();

  screen.begin();

  sei();

  while (1) {
    touchHandler();
    screen.refreshBacklight();
    if (communication.runFrame)
    { 
      handleStateChange(largeFieldSnake);
      handleState();
      //playSound(SOUND_MENU);
      communication.runFrame = false;
    }

    //Serial.println(currentSoundFrame);
  }
  return 0;
}

ISR(TIMER2_COMPA_vect) {
  // Add 1 to the current Sound Frame
  currentSoundFrame += 1;

  // Check if frame equals or exceeds sound frame (pitch)
  if (currentSoundFrame >= soundFrames) {

    // 0 = No Sound; 1 = Play Sound
    if (soundQueue[currentSoundIndex][0] == 1) {
      PORTD ^= (1 << PD3);
    } else {
      PORTD &= ~(1 << PD3);
    }

    // Add 1 to the currentSoundFrameIndex up to the given frames in the array
    // Reset current Sound Frame
    currentSoundFrameIndex += 1;
    currentSoundFrame = 0;

    // Check if all frames of the sound have played
    if (currentSoundFrameIndex >= soundQueue[currentSoundIndex][2]) {

      // Add 1 to the sound index to move to the next array in the sound
      // Reset the sound frame index
      // Set the pitch of the sound
      currentSoundIndex += 1;
      currentSoundFrameIndex = 0;
      soundFrames = soundQueue[currentSoundIndex][1];

      // If all arrays in the sound have played (max is maximum allowed sounds in sound array)
      if (currentSoundIndex >= 6) {

        // Reset all variables
        // Disable Speaker
        // Disable Timer Compare
        soundFrames = 1;
        currentSoundFrame = 1;
        currentSoundFrameIndex = 0;
        currentSoundIndex = 0;

        soundQueue = nullptr;

        PORTD &= ~(1 << PD3);
        TIMSK2 &= ~(1 << OCIE2A);
      }
    }
  }

}

ISR(TIMER1_COMPA_vect) { communication.communicate(); }

ISR(INT0_vect) {
  TCNT1 = 5;
  communication.busBitIndex = 0;
  EIMSK &= ~(1 << INT0); // INT0 interrupt disable
  TIMSK1 |= (1 << OCIE1A);
}

