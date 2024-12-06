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

// TODO: VANAF HIER COMMUNICATIE VAR AND FUNCS VOOR IN EEN CLASS ZO
#define IR_LED_PIN PD6         // Pin 6 voor IR LED (OC0A)
#define IR_RECEIVER_PIN PD2    // Pin 2 voor IR Receiver (INT0)
#define DATABITCOUNT 32        // bits in een databus
#define COMMUNICATIONSPEED 200 // snelheid van timer 0 interrupts
#define OCSILLATIONSPEED 209   // 38kHz oscilleer snelheid led pin
#define COMMUNICATIONOFFSETMIN 0

// LCD object
Adafruit_ILI9341 screen(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

NunChuk nunchuck;
const uint8_t NUNCHUCK_ADDRESS = 0x52;

// Create Snake object
Snake snake(GRID_SIZE, TFT_WIDTH / GRID_SIZE, TFT_WIDTH / GRID_SIZE, screen,
            MAGENTA);

// game state tracken
enum gameState { MENU, START, INGAME, DEATH };
volatile gameState currentState = MENU;
volatile gameState previousState = DEATH;

// communication
volatile uint32_t firstSyncCheck =
    0x80000000; // 3827391077 // unieke bit volgorde die niet eerder 0xE4215A65
                // gedetecteerd kan worden zoals bijvoorbeeld 0x33333333
volatile uint32_t secondSyncCheck = 0xAAAAAAAA; // 2863311530
volatile uint32_t thirdSyncCheck = 0xAE6CB249;  // 2926359113
volatile uint32_t outBus =
    firstSyncCheck;          // Uitgaande data bus begint als firstSyncCheck om
                             // communicatie te synchroniseren
volatile uint32_t inBus = 0; // Binnenkomende data bus
volatile uint8_t busBitIndex = 0; // huidige bit index inBus

volatile bool isSender = true; // player1 begint met zenden en zetten timer

volatile uint8_t senderOffset = 0; // offset voor het lezen van de sender
                                   // arduino (kijkt over de start bit heen)

volatile bool IRWaiting =
    false; // boolean die on/off getriggered word om de IR-reciever tijd te
           // geven om niet overloaded te worden

volatile bool printBus = true; // boolean die bepaalt of de inBus klaar is om
                               // te printen naar de serial monitor

volatile bool communicationInitialized =
    false; // boolean die checkt of de communicatie aan beide kanten is
           // geinitialiseerd na het synchroniseren
volatile bool communicationSynced =
    false; // boolean die bepaalt of de communicatie synchroon loopt na het
           // opstarten
volatile uint8_t SyncingIndex = 0;
volatile bool SyncingIndexFound = false;

volatile uint8_t communicationFrameCounter = 0;
volatile uint8_t communicationFrameCount = 5;
volatile bool runFrame = true;

// settings
volatile uint8_t posSnake = 0;
volatile uint8_t posApple = 0;
volatile bool isPlayer1 = false;
volatile bool isSmallField = false;
volatile bool appleGatheredByPlayer2 = false;
volatile bool gamePaused = true;
volatile bool isAlive = false;
volatile uint8_t checksum;

void setupPins() {
  DDRD |= (1 << PD6); // PD6 Ouptut

  DDRD &= ~(1 << IR_RECEIVER_PIN);  // Set PD2 as input
  PORTD &= ~(1 << IR_RECEIVER_PIN); // Enable pull-up resistor

  DDRD |= (1 << PD7);   // PD7 Ouptut
  PORTD &= ~(1 << PD7); // PD7 begint LOW
}

void setupTimers() {
  // Correct Timer 1 setup for CTC mode with prescaler 64
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << WGM12);              // Correct: WGM12 is in TCCR1B
  TCCR1B |= (1 << CS11) | (1 << CS10); // Prescaler 64
  OCR1A = COMMUNICATIONSPEED;

  // Correct Timer 0 setup
  TCCR0A = 0;
  TCCR0B = 0;
  TCCR0A |= (1 << WGM01);  // CTC mode
  TCCR0B |= (1 << CS00);   // No prescaler
  TCCR0A |= (1 << COM0A0); // Zet oscillatie aan
  OCR0A = OCSILLATIONSPEED;
}

void SetupInterrupts() {
  EICRA &= ~(1 << ISC01);
  EICRA &= ~(1 << ISC00); // Trigger bij LOW
  EIMSK &= ~(1 << INT0);  // INT0 interrupt disable
}

void initializeCommunication() {
  busBitIndex = 0;
  TCNT1 = 0;
  TIMSK1 |= (1 << OCIE1A); // Enable Timer1 Compare Match A interrupt
  if (isSender) {
    senderOffset = 1;
  }
}

void synchronise(uint32_t bus) {
  if (bus != 0) {
    for (uint8_t i = 0; i <= DATABITCOUNT; i++) {
      // Check each bit starting from the most significant bit (MSB)
      if ((bus >> (DATABITCOUNT - 1 - i)) & 1) {
        if (i <= 30) {
          SyncingIndex = DATABITCOUNT - i + 2;
        } else {
          SyncingIndex = 0;
        }
        SyncingIndexFound = true;
        break; // Exit the loop after finding the first '1' bit
      }
    }
  }
}

uint8_t constructChecksum(uint32_t value) {
  uint8_t checksum = 0;
  for (uint8_t i = 3; i < DATABITCOUNT; i++) { // Start bij bit 3
    checksum ^= (value >> i) & 0x01;
  }
  return checksum & 0x07; // Return 3-bit checksum
}

uint32_t constructBus() {
  uint32_t out = 0;

  out |= ((uint32_t)posSnake) << 24;                 // Bit 31–24: posSnake
  out |= ((uint32_t)snake.snakeLength & 0xFF) << 16; // Bit 23–16: lengthSnake
  out |= ((uint32_t)posApple & 0xFF) << 8;           // Bit 15–8: posApple
  out |=
      ((isPlayer1 & 0x01) << 7) |              // Bit 7: isPlayer1
      ((isSmallField & 0x01) << 6) |           // Bit 6: isSmallField
      ((appleGatheredByPlayer2 & 0x01) << 5) | // Bit 5: appleGatheredByPlayer2
      ((gamePaused & 0x01) << 4) |             // Bit 4: gamePaused
      ((isAlive & 0x01) << 3);                 // Bit 3: isAlive

  // checksum toevoegen aan laatste 3 data bits
  uint8_t checksum = constructChecksum(out);
  out |= (uint32_t)(checksum & 0x07); // Bits 2–0: checksum

  return out;
}

void deconstructBus(uint32_t bus) {
  uint8_t checksum = (uint8_t)(bus & 0x07); // checksum pakken uit de bus
  uint8_t calculatedChecksum =
      constructChecksum(bus & 0xFFFFFFF8); // checksum berekenen voor de bus
  if (checksum == calculatedChecksum) {    // check of checksum overeenkomt

    posSnake = (uint8_t)((bus >> 24) & 0xFF);          // Bit 31-24: posSnake
    snake.snakeLength = (uint8_t)((bus >> 16) & 0xFF); // Bit 23–16: lengthSnake
    posApple = (uint8_t)((bus >> 8) & 0xFF);           // Bit 15–8: posApple

    isPlayer1 = (bus >> 7) & 0x01;              // Bit 7: isPlayer1
    isSmallField = (bus >> 6) & 0x01;           // Bit 6: isSmallField
    appleGatheredByPlayer2 = (bus >> 5) & 0x01; // Bit 5: appleGatheredByPlayer2
    gamePaused = (bus >> 4) & 0x01;             // Bit 4: gamePaused
    isAlive = (bus >> 3) & 0x01;                // Bit 3: isAlive
  }
}

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
void directionHandler() {
  if (nunchuck.getState(NUNCHUCK_ADDRESS)) {
    snake.updateDirection(nunchuck.state.joy_x_axis, nunchuck.state.joy_y_axis);
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
  Wire.begin(); // start wire for nunchuck

  setupPins();
  setupTimers();
  SetupInterrupts();
  initializeCommunication();

  // LCD setup
  screen.begin();
  screen.setTextSize(2);

  sei();

  while (1) {
    directionHandler(); // update direction tussen frames

    if (runFrame) { // runt iedere 167ms
      handleStateChange();
      handleState();
      runFrame = false;
    }
  }

  return 0;
}

void communicate() {
  if (!IRWaiting) // checkt of de IR-reciever een pauze nodig heeft
  {
    if (busBitIndex == 0) // checkt of de start bit gestuurd moet worden
    {
      inBus = 0x00000000;
      TCCR0A |= (1 << COM0A0); // Turn on oscillation
    }
    if (busBitIndex >= 1 &&
        busBitIndex <=
            DATABITCOUNT + 1) // 1-33 voor het sturen en lezen van de bits
    {
      if (!isSender) {
        PORTD ^= (1 << PD7);
        bool bit = (PIND & (1 << IR_RECEIVER_PIN)) ==
                   0; // LOW is logische 1 in IR communicatie
        if (bit) {
          inBus |= (1UL << (DATABITCOUNT -
                            (busBitIndex -
                             senderOffset))); // bepaalt welke index van de
                                              // inBus de gelezen waarde in moet
                                              // sender begint 1 later
        } else {
          inBus &= ~(1UL << (DATABITCOUNT -
                             (busBitIndex -
                              senderOffset))); // bepaalt welke index van de
                                               // inBus de gelezen waarde in
                                               // moet sender begint 1 later
        }
        PORTD ^= (1 << PD7);
      }

      // Transmit current bit
      bool outBit = (outBus >> (DATABITCOUNT - (busBitIndex))) &
                    0x01; // bepaalt of de volgende bit in de outBus en 1 of 0
                          // moet transmitten
      if (outBit) {
        TCCR0A |= (1 << COM0A0); // Turn on oscillation
      } else {
        TCCR0A &= ~(1 << COM0A0);
        PORTD &= ~(1 << PD6); // Ensure PD6 is LOW
      }
    }

    busBitIndex++; // verhoogt de index

    if (busBitIndex >
        DATABITCOUNT + 2) // checkt of de laatste bit is geweest en checkt of
                          // hij alles nu mag resetten
    {

      busBitIndex = 0;

      if (!communicationSynced && !isSender &&
          !SyncingIndexFound) // Sneller de arduino's synchroniseren
      {
        synchronise(inBus);
        busBitIndex = SyncingIndex;
      } else if (!communicationSynced && !isSender && SyncingIndexFound) {
        busBitIndex = 0;
        SyncingIndexFound = false;
      }
      if (!communicationInitialized) {
        if (inBus == firstSyncCheck && !communicationSynced &&
            !isSender) // checkt of de communicatie synchroon loopt of
                       // nog synchroon moet gaan lopen
        {
          communicationSynced = true;
          outBus = secondSyncCheck;
        }
        if (inBus == secondSyncCheck &&
            isSender) //  geedt aan de andere kant aan dat de
                      //  communicatie is geïnitializeerd
        {
          outBus = secondSyncCheck;
        }
        if (inBus == secondSyncCheck && !isSender) {
          outBus = thirdSyncCheck;
          communicationInitialized = true;
        }
        if (inBus == thirdSyncCheck && isSender) {
          communicationInitialized = true;
        }
      }
      if (communicationSynced &&
          !isSender) // zet int 0 interrupt aan om te zorgen dat de !isSender de
                     // startbit binnen krijgt om de timers synchroon te laten
                     // lopen
      {
        EIMSK |= (1 << INT0); // INT0 interrupt enable
        TIMSK1 &= ~(1 << OCIE1A);
      }
      if (communicationFrameCounter >= communicationFrameCount) {
        runFrame = true;
        communicationFrameCounter = 0;
      } else {
        communicationFrameCounter = communicationFrameCounter + 1;
      }
      TCCR0A &= ~(1 << COM0A0);
      PORTD |= (1 << PD6); // Ensure PD6 is LOW
      printBus = true;
    }
    IRWaiting = true; // zorgt ervoor dat de IR-reciever een pauze krijgt
  } else {
    TCCR0A &= ~(1 << COM0A0);
    PORTD |= (1 << PD6); // Ensure PD6 is LOW
    if (isSender && busBitIndex >= 2 && busBitIndex <= DATABITCOUNT + 2) {
      PORTD ^= (1 << PD7);
      bool bit = (PIND & (1 << IR_RECEIVER_PIN)) ==
                 0; // LOW is logische 1 in IR communicatie
      if (bit) {
        inBus |= (1UL << (DATABITCOUNT -
                          (busBitIndex -
                           senderOffset))); // bepaalt welke index van de inBus
                                            // de gelezen waarde in moet sender
                                            // begint 1 later
      } else {
        inBus &= ~(
            1UL
            << (DATABITCOUNT -
                (busBitIndex -
                 senderOffset))); // bepaalt welke index van de inBus de gelezen
                                  // waarde in moet sender begint 1 later
      }
      PORTD ^= (1 << PD7);
    }
    IRWaiting = false;
  }
}

ISR(TIMER1_COMPA_vect) { communicate(); }

ISR(INT0_vect) {
  TCNT1 = 5;
  busBitIndex = 0;
  EIMSK &= ~(1 << INT0); // INT0 interrupt disable
  TIMSK1 |= (1 << OCIE1A);
  communicate();
}
