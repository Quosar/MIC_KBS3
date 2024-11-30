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
#define DATABITCOUNT 32
#define SYNCEDVALUE 75

enum Status
{ // enum om tatus te wisselen
  IDLE,
  WRITING,
  READING,
};

// TODO: Test bool voor nunchuk transmissie
volatile bool isNunchukController = true;

// communication
volatile uint32_t outBus = 3827391077; // Uitgaande data bus
volatile uint32_t inBus = 0;           // Binnenkomende data bus
volatile uint8_t busBitIndex = 0;      // huidige bit index inBus

volatile bool isSender = false; // player1 begint met zenden en zetten timer

volatile bool ledOn = false;

volatile uint32_t outBusCounter = 0;

volatile bool IRWaiting = false;

volatile bool printBus = false;

volatile bool communicationInitialized = true;
volatile bool communicationSynced = false;

// settings
volatile bool isPlayer1 = true;
volatile bool isSmallField = true;
volatile bool appleGatheredByPlayer2 = false;
volatile bool gamePaused = false;
volatile bool isAlive = true;
volatile uint8_t checksum;
uint16_t counter = 0;

void setupPins()
{
  DDRD |= (1 << PD6);   // PD6 Ouptut
  PORTD &= ~(1 << PD6); // PD6 begint LOW

  DDRD &= ~(1 << IR_RECEIVER_PIN);  // Set PD2 as input
  PORTD &= ~(1 << IR_RECEIVER_PIN); // Enable pull-up resistor

  DDRD |= (1 << PD7);   // PD7 Ouptut
  PORTD &= ~(1 << PD7); // PD7 begint LOW
}

void setupTimers()
{
  // Correct Timer 0 setup for CTC mode with prescaler 64
  TCCR0A = 0;
  TCCR0B = 0;
  TCCR0A |= (1 << WGM01);              // CTC mode
  TCCR0B |= (1 << CS01) | (1 << CS00); // Prescaler 64
  OCR0A = 75;

  // Correct Timer 2 setup
  TCCR2A = (1 << WGM21);   // CTC mode
  TCCR2B = (1 << CS20);    // No prescaler
  TCCR2A |= (1 << COM2A0); // Toggle OC2A on compare match
  OCR2A = 209;
}

void SetupInterrupts()
{
  EICRA &= ~(1 << ISC00) | ~(1 << ISC01); // Trigger bij iedere verrandering
  EIMSK &= ~(1 << INT0);                  // INT0 interrupt disable
}

void initializeCommunication()
{
  busBitIndex = 0;
  TCNT0 = 0;
  TIMSK0 |= (1 << OCIE0A); // Enable Timer0 Compare Match A interrupt
}

uint8_t constructChecksum(uint32_t value)
{
  uint8_t checksum = 0;
  for (uint8_t i = 3; i < 32; i++)
  { // Start bij bit 3
    checksum ^= (value >> i) & 0x01;
  }
  return checksum & 0x07; // Return 3-bit checksum
}

uint32_t constructBus()
{
  uint32_t out = 0;
  uint8_t posSnake;
  // posSnake = ((snake.snakeX[0] << 4) || snake.snakeY[0]);

  posSnake =
      ((0x01 << 4) |
       0x06); // sending dummy data as pos to check if we can receive this

  out |= ((uint32_t)posSnake) << 24;                 // Bit 31–24: posSnake
  out |= ((uint32_t)snake.snakeLength & 0xFF) << 16; // Bit 23–16: lengthSnake
  out |= ((uint32_t)1 & 0xFF) << 8;                  // Bit 15–8: posApple //TODO: make pos apple
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

void initialiseScreen()
{
  screen.begin();
  screen.fillScreen(BLACK);
}

int main()
{
  init();
  Serial.begin(9600);
  // Wire.begin();       // start wire for nunchuck
  // initialiseScreen(); // init the screen

  // communication setup
  cli();
  setupPins();
  setupTimers();
  SetupInterrupts();
  initializeCommunication();
  sei();

  // snake.start(); // start snake on middle of the screen

  // // TODO: deze test voor scherm testen moet later weg
  // screen.setCursor(60, TFT_WIDTH / 2);
  // screen.setTextColor(RED);
  // screen.setTextSize(3);
  // screen.println("X: " + snake.snakeX[0]);
  // screen.println("Y: " + snake.snakeY[0]);

  // Serial.println("ja hoor");

  while (1)
  {
    if (printBus)
    {
      Serial.println(inBus);
      printBus = false;
      // if (outBusCounter > 9)
      // {
      //   outBusCounter = 0;
      //   outBus = outBus + 1;
      // }
      // else
      // {
      //   outBusCounter++;
      // }
    }

    // if (!isNunchukController) {
    //   screen.println(String(inBus));

    //   uint32_t snakeX = inBus >> 28;
    //   uint32_t snakeY = inBus << 4;
    //   snakeY >> 28;

    //   screen.fillScreen(BLACK);
    //   screen.setCursor(60, TFT_WIDTH / 2);
    //   screen.println("X: " + String(snakeX));
    //   screen.println("Y: " + String(snakeY));
    // }

    // Serial.println(String(inBus));

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

ISR(TIMER0_COMPA_vect)
{
  if (!IRWaiting)
  {
    if (busBitIndex == 0)
    {
      inBus = 0x00000000;
      TIMSK2 |= (1 << OCIE2A); // Enable Timer 2 Compare Match A interrupt
      // PORTD |= (1 << PD7);
    }
    if (busBitIndex >= 1 && busBitIndex <= DATABITCOUNT)
    {
      // Read the state of the IR receiver pin
      PORTD ^= (1 << PD7);
      bool bit = (PIND & (1 << IR_RECEIVER_PIN)) == 0; // LOW is logic 1 in IR communication
      if (bit)
      {
        inBus |= (1UL << (DATABITCOUNT - (busBitIndex)));
      }
      else
      {
        inBus &= ~(1UL << (DATABITCOUNT - (busBitIndex)));
      }
      PORTD ^= (1 << PD7);

      // Transmit current bit
      bool outBit = (outBus >> (DATABITCOUNT - (busBitIndex))) & 0x01;
      if (outBit)
      {
        TIMSK2 |= (1 << OCIE2A); // Enable Timer 2 Compare Match A interrupt
      }
      else
      {
        TIMSK2 &= ~(1 << OCIE2A); // Disable Timer 2 Compare Match A interrupt
        PORTD &= ~(1 << PD6);     // Ensure PD6 is LOW
      }
    }

    busBitIndex++;
    if (busBitIndex > DATABITCOUNT + 1)
    {
      // Check if synchronization is required
      if (inBus == 3827391077 && !communicationSynced)
      {
        communicationSynced = true;
      }
      if(communicationSynced && !isSender){
        EIMSK |= (1 << INT0);                  // INT0 interrupt enable
      }
      TIMSK2 &= ~(1 << OCIE2A); // Disable Timer 2 Compare Match A interrupt
      PORTD |= (1 << PD6);      // Set PD6 HIGH
      busBitIndex = 0;
      printBus = true;
    }
    IRWaiting = true;
  }
  else
  {
    TIMSK2 &= ~(1 << OCIE2A); // Disable Timer 2 Compare Match A interrupt
    PORTD |= (1 << PD6);      // Set PD6 HIGH
    IRWaiting = false;
  }
}

ISR(TIMER2_COMPA_vect)
{
  PORTD ^= (1 << PD6);
}

ISR(INT0_vect){
  TCNT0 = 74;
  busBitIndex = 0;
  EIMSK &= ~(1 << INT0);                  // INT0 interrupt disable
}

