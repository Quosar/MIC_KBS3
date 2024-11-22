#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Nunchuk.h"
#include "Snake.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <avr/interrupt.h>

// LCD Pin Defines
#define TFT_CLK 13
#define TFT_MISO 12
#define TFT_MOSI 11
#define TFT_DC 9
#define TFT_CS 10
#define TFT_RST 8

const int8_t NUNCHUCK_ADDRESS = 0x52;

// Screen dimensions
const uint16_t TFT_WIDTH = 240;
const uint16_t TFT_HEIGHT = 320;
const uint8_t GRID_SIZE = 16;

// Create TFT and Nunchuk objects
Adafruit_ILI9341 screen(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
NunChuk nunchuck;

// Create Snake object
Snake snake(GRID_SIZE, TFT_WIDTH / GRID_SIZE, TFT_HEIGHT / GRID_SIZE, screen);

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
#define IR_LED_PIN PD6      // Pin 6 voor IR LED (OC0A)
#define IR_RECEIVER_PIN PD2 // Pin 2 voor IR Receiver (INT0)
#define BIT_DURATION 919    // Bit tijd voor Timer1 (3.676 ms at 16 MHz / 64)
#define FRAME_BITS 34       // 1 start bit + 32 data bits + 1 stop bit
#define DATABITCOUNT 32

enum Status { // enum om tatus te wisselen
  IDLE,
  WRITING,
  READING,
};

// TODO: Test bool voor nunchuk transmissie
volatile bool isNunchukController = true;

// communication
volatile Status status = IDLE; // Naar IDLE om te beginnen met communicatie
volatile uint32_t outBus = 0x55555555; // Uitgaande data bus
volatile uint32_t inBus = 0;           // Binnenkomende data bus
volatile uint8_t inBusBit_index = 0;     // huidige bit index inBus
volatile uint8_t outBusBit_index = 0;     // huidige bit index outBus

volatile bool isSender = true; // player1 begint met senden en zetten timer

// settings
volatile bool isPlayer1 = true;
volatile bool isSmallField = true;
volatile bool appleGatheredByPlayer2 = false;
volatile bool gamePaused = false;
volatile bool isAlive = true;
volatile uint8_t checksum;
uint16_t counter = 0;

void setupPins() {
  DDRD |= (1 << PD6);   // PD6 Ouptut
  PORTD &= ~(1 << PD6); // PD6 begint LOW

  DDRD &= ~(1 << PD2);  // PD2 Input
  PORTD &= ~(1 << PD2); // pull-up resistor uit
}

void setupTimers() {
  TCCR0A |= (1 << WGM01) | (1 << COM0A0); // CTC mode, toggle OC0A
  TCCR0B |= (1 << CS01);                  // Prescaler 8
  OCR0A = 25; // Timer Compare interrupt tijd voor 58 kHz // 0x22 = 34

  TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10); // CTC mode, prescaler 64
  OCR1A = BIT_DURATION; // Timer Compare interrupt tijd voor lezen iedere bit
}

void SetupInterrupts() {
  EICRA |= (1 << ISC00); // Trigger bij iedere verrandering
  EIMSK |= (1 << INT0);  // INT0 interrupt enable
}

void start_writing(uint32_t data) {
  outBus = data;
  outBusBit_index = 0;
  status = WRITING;
  TIMSK1 |= (1 << OCIE1A); // Enable Timer1 Compare Match interrupt
}

void start_reading() {
  inBusBit_index = 0;
  inBus = 0;
  status = READING;
}

uint8_t constructChecksum(uint32_t value) {
  uint8_t checksum = 0;
  for (uint8_t i = 3; i < 32; i++) { // Start bij bit 3
    checksum ^= (value >> i) & 0x01;
  }
  return checksum & 0x07; // Return 3-bit checksum
}

uint32_t constructBus() {
  uint32_t out = 0;
  uint8_t posSnake;
  // posSnake = ((snake.snakeX[0] << 4) || snake.snakeY[0]);

  posSnake =
      ((0x01 << 4) |
       0x06); // sending dummy data as pos to check if we can receive this

  out |= ((uint32_t)posSnake) << 24;          // Bit 31–24: posSnake
  out |= ((uint32_t)snake.snakeLength & 0xFF) << 16; // Bit 23–16: lengthSnake
  out |= ((uint32_t)1 & 0xFF) << 8; // Bit 15–8: posApple //TODO: make pos apple
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

// TODO: TOT HIER COMMUNICATIE VAR AND FUNCS VOOR IN EEN CLASS ZO

void initialiseScreen() {
  screen.begin();
  screen.fillScreen(BLACK);
}

int main() {
  init();
  Serial.begin(9600);
  Wire.begin();       // start wire for nunchuck
  initialiseScreen(); // init the screen

  // communication setup
  cli();
  setupPins();
  setupTimers();
  SetupInterrupts();
  sei();

  snake.start(); // start snake on middle of the screen

  // TODO: deze test voor scherm testen moet later weg
  screen.setCursor(60, TFT_WIDTH / 2);
  screen.setTextColor(RED);
  screen.setTextSize(3);
  screen.println("X: " + snake.snakeX[0]);
  screen.println("Y: " + snake.snakeY[0]);

  //Serial.println("ja hoor");

  while (1) {

    // communication afhandeling
    switch (status) {
    case IDLE:
      if (isSender) {
        uint32_t bus = constructBus();
        start_writing(bus); // Start writing
      } else {
        start_reading(); // Start reading
      }
      //_delay_ms(1);
      isSender = !isSender; // switch tussen readen en writen na ieder frame
                            // voor full-duplex
      break;

    case WRITING: // wordt gedaan via interrupts
    case READING: // wordt gedaan via interrupts
    Serial.println(inBus);
      break;
    }



    if (!isNunchukController) {
      screen.println(String(inBus));

      uint32_t snakeX = inBus >> 28;
      uint32_t snakeY = inBus << 4;
      snakeY >> 28;

      screen.fillScreen(BLACK);
      screen.setCursor(60, TFT_WIDTH / 2);
      screen.println("X: " + String(snakeX));
      screen.println("Y: " + String(snakeY)); 
    }

    //Serial.println(String(inBus));


    // snake afhandeling
    // if (nunchuck.getState(NUNCHUCK_ADDRESS)) {
    //   snake.updateDirection(nunchuck.state.joy_x_axis,
    //                         nunchuck.state.joy_y_axis);
    // }
    // if (counter >= 64000) { //TODO: magic number weg
    //   counter = 0;
    //   // snake afhandeling
    //   if (nunchuck.getState(NUNCHUCK_ADDRESS)) {
    //     snake.updateDirection(nunchuck.state.joy_x_axis,
    //                           nunchuck.state.joy_y_axis);
    //   }

    //   snake.move();

    //   if (snake.checkCollision()) {
    //     screen.fillScreen(BLACK);
    //     screen.setCursor(60, TFT_HEIGHT / 2);
    //     screen.setTextColor(RED);
    //     screen.setTextSize(2);
    //     screen.println("Game Over!");
    //     screen.println("PRESS Z TO CONTINUE");

    //     while (1) { // TODO: REMOVE WHILE
    //       if (nunchuck.getState(NUNCHUCK_ADDRESS)) {
    //         if (nunchuck.state.z_button) { // press z to reset game
    //           snake.start(); // TODO: reset previouse snake positions. It now
    //                          // restarts with half the tail on old pos
    //           break;
    //         }
    //       }
    //     }
    //   }

    //   snake.draw();
    // }
    // counter++;
  }

  return 0;
}

ISR(TIMER1_COMPA_vect) {
  if (status == WRITING) {
    if (outBusBit_index == 0) {
      TCCR0A &= ~(1 << COM0A0); // Stuur start bit (1)
    } else if (outBusBit_index > 0 && outBusBit_index <= DATABITCOUNT) {
      uint8_t bit = (outBus >> (DATABITCOUNT - outBusBit_index)) & 0x01;
      if (bit) {
        TCCR0A |= (1 << COM0A0); // Stuur data bit waarde (1)
      } else {
        TCCR0A &= ~(1 << COM0A0); // Stuur data bit waarde (0)
      }
    } else if (outBusBit_index == FRAME_BITS - 1) {
      TCCR0A |= (1 << COM0A0); // stuur stop bit (0)
    }
    outBusBit_index++;
    if (outBusBit_index >= FRAME_BITS) {
      outBusBit_index = 0;
      status = IDLE; // Status naar idle
    }
  }

  if (status == READING) {
    if (inBusBit_index <= DATABITCOUNT) {
      if (!(PIND & (1 << IR_RECEIVER_PIN))) {
        inBus = (inBus << 1) | 1; // Bit = 1
      } else {
        inBus = (inBus << 1); // Bit = 0
      }
      inBusBit_index++;
    } else if (inBusBit_index == FRAME_BITS - 1) { // laatse bit/stop bit
      status = IDLE;
      inBusBit_index = 0;         // bus index resetten
      TIMSK1 &= ~(1 << OCIE1A); // Timer1 interrupts uit
    }
  }
}

ISR(INT0_vect) {
  if (inBusBit_index == 0) {
    inBus = 0;        // inbus clearen
    inBusBit_index = 1; // eerste bit lezen
    TCNT1 = 0;        // Reset Timer1
    status = READING;
    // Timer 1 aan ingeval van eerste keer sturen
    TIMSK1 |= (1 << OCIE1A);
  }
}