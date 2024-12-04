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
#define IR_LED_PIN PD6        // Pin 6 voor IR LED (OC0A)
#define IR_RECEIVER_PIN PD2   // Pin 2 voor IR Receiver (INT0)
#define DATABITCOUNT 32       // bits in een databus
#define COMMUNICATIONSPEED 100 // snelheid van timer 0 interrupts
#define OCSILLATIONSPEED 209  // 38kHz oscilleer snelheid led pin
#define COMMUNICATIONOFFSETMIN 0
#define COMMUNICATIONOFFSETMAX 100

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

// communication
volatile uint32_t firstSyncCheck = 0xE4215A65;  // 3827391077 // unieke bit volgorde die niet eerder gedetecteerd kan worden zoals bijvoorbeeld 0x33333333
volatile uint32_t secondSyncCheck = 0xAAAAAAAA; // 2863311530
volatile uint32_t outBus = firstSyncCheck;      // Uitgaande data bus begint als firstSyncCheck om communicatie te synchroniseren
volatile uint32_t inBus = 0;                    // Binnenkomende data bus
volatile uint8_t busBitIndex = 0;               // huidige bit index inBus

volatile bool isSender = true; // player1 begint met zenden en zetten timer

volatile uint8_t senderOffset = 0; // offset voor het lezen van de sender arduino (kijkt over de start bit heen)

volatile bool IRWaiting = false; // boolean die on/off getriggered word om de IR-reciever tijd te geven om niet overloaded te worden

volatile bool printBus = false; // boolean die bepaalt of de inBus klaar is om te printen naar de serial monitor

volatile uint8_t communicationOffset = COMMUNICATIONOFFSETMIN;
volatile bool communicationOffsetDetermined = false;
volatile uint32_t previousInBus = 0x00000000;
volatile uint8_t syncCheckCounter = 0;
volatile uint8_t syncCheckCount = 10;

volatile bool communicationInitialized = false; // boolean die checkt of de communicatie aan beide kanten is geinitialiseerd na het synchroniseren
volatile bool communicationSynced = false;      // boolean die bepaalt of de communicatie synchroon loopt na het opstarten
volatile uint8_t syncCounter = 0;               // counter die optelt tot 50 om te zorgen dat de communicatie aan beide kanten goed loopt
volatile uint8_t syncCount = 50;                // 50 keer dezelfde inBus binnen krijgen om te bepalen of de communicatie gesynchroniseerd is

volatile uint8_t communicationFrameCounter = 0;
volatile uint8_t communicationFrameCount = 3;
volatile bool runFrame = true;

// settings
volatile uint8_t posSnake = 0;
volatile uint8_t posApple = 0;
volatile bool isPlayer1 = true;
volatile bool isSmallField = true;
volatile bool appleGatheredByPlayer2 = false;
volatile bool gamePaused = false;
volatile bool isAlive = true;
volatile uint8_t checksum;

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
  OCR0A = COMMUNICATIONSPEED;

  // Correct Timer 2 setup
  TCCR2A = (1 << WGM21);   // CTC mode
  TCCR2B = (1 << CS20);    // No prescaler
  TCCR2A |= (1 << COM2A0); // Toggle OC2A on compare match
  OCR2A = OCSILLATIONSPEED;
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
  TIMSK0 &= ~(1 << OCIE0A); // Enable Timer0 Compare Match A interrupt
}

uint8_t constructChecksum(uint32_t value)
{
  uint8_t checksum = 0;
  for (uint8_t i = 3; i < DATABITCOUNT; i++)
  { // Start bij bit 3
    checksum ^= (value >> i) & 0x01;
  }
  return checksum & 0x07; // Return 3-bit checksum
}

uint32_t constructBus()
{
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
    uint8_t checksum = (uint8_t)(bus & 0x07); //checksum pakken uit de bus
    uint8_t calculatedChecksum = constructChecksum(bus & 0xFFFFFFF8); // checksum berekenen voor de bus
    if (checksum == calculatedChecksum) { // check of checksum overeenkomt

    posSnake = (uint8_t)((bus >> 24) & 0xFF); // Bit 31-24: posSnake
    snake.snakeLength = (uint8_t)((bus >> 16) & 0xFF); // Bit 23–16: lengthSnake
    posApple = (uint8_t)((bus >> 8) & 0xFF); // Bit 15–8: posApple

    isPlayer1 = (bus >> 7) & 0x01;             // Bit 7: isPlayer1
    isSmallField = (bus >> 6) & 0x01;          // Bit 6: isSmallField
    appleGatheredByPlayer2 = (bus >> 5) & 0x01;// Bit 5: appleGatheredByPlayer2
    gamePaused = (bus >> 4) & 0x01;            // Bit 4: gamePaused
    isAlive = (bus >> 3) & 0x01;               // Bit 3: isAlive
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


int main()
{
  init();
  Serial.begin(9600);
  Wire.begin();       // start wire for nunchuck

  // communication setup
  setupPins();
  setupTimers();
  SetupInterrupts();
  initializeCommunication();

  // LCD setup
  screen.begin();
  screen.setTextSize(2);
  sei();


  while (1) {
    TIMSK0 &= ~(1 << OCIE0A); // Enable Timer0 Compare Match A interrupt
    // if(runFrame){ // runt iedere 167ms
    handleStateChange();
    handleState();
    _delay_ms(150);
    // runFrame = false;
    // }
  }

  return 0;
}

ISR(TIMER0_COMPA_vect)
{
  if (!IRWaiting) // checkt of de IR-reciever een pauze nodig heeft
  {
    if (busBitIndex == 0) // checkt of de start bit gestuurd moet worden
    {
      inBus = 0x00000000;
      TIMSK2 |= (1 << OCIE2A); // Enable Timer 2 Compare Match A interrupt
    }
    if (busBitIndex >= 1 && busBitIndex <= DATABITCOUNT + 1) // 1-33 voor het sturen en lezen van de bits
    {
      PORTD ^= (1 << PD7);
      bool bit = (PIND & (1 << IR_RECEIVER_PIN)) == 0; // LOW is logische 1 in IR communicatie
      if (bit)
      {
        inBus |= (1UL << (DATABITCOUNT - (busBitIndex - senderOffset))); // bepaalt welke index van de inBus de gelezen waarde in moet sender begint 1 later
      }
      else
      {
        inBus &= ~(1UL << (DATABITCOUNT - (busBitIndex - senderOffset))); // bepaalt welke index van de inBus de gelezen waarde in moet sender begint 1 later
      }
      PORTD ^= (1 << PD7);

      // Transmit current bit
      bool outBit = (outBus >> (DATABITCOUNT - (busBitIndex))) & 0x01; // bepaalt of de volgende bit in de outBus en 1 of 0 moet transmitten
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

    busBitIndex++; // verhoogt de index

    if (busBitIndex > DATABITCOUNT + 2) // checkt of de laatste bit is geweest en checkt of hij alles nu mag resetten
    {
      if (inBus == firstSyncCheck && !communicationSynced) // checkt of de communicatie synchroon loopt of nog synchroon moet gaan lopen
      {
        communicationSynced = true;
        if (isSender) // zorgt ervoor dat de sender 1tje later leest in de inBus
        {
          senderOffset = 1;
        }
      }
      if (communicationSynced && !isSender) // zet int 0 interrupt aan om te zorgen dat de !isSender de startbit binnen krijgt om de timers synchroon te laten lopen
      {
        EIMSK |= (1 << INT0); // INT0 interrupt enable
      }
      if (communicationSynced && !communicationInitialized) // checkt of de communicatie synchroon loopt en begint met het initializeren ervan
      {
        if (isSender)
        {
          if (syncCounter == syncCount) // telt tot 50 om te zorgen dat de andere kant ook de synchronisatie detecteert
          {
            outBus = secondSyncCheck; //  veranderd de outbus om aan te geven dat deze kant is geïnitializeerd
            communicationInitialized = true;
          }
          else
          {
            syncCounter++; // verhoogt de counter als hij nog geen 50 is
          }
        }
        else if (inBus == secondSyncCheck && !isSender) //  geedt aan de andere kant aan dat de communicatie is geïnitializeerd
        {
          outBus = secondSyncCheck;
          communicationInitialized = true;
        }
      }
      if(communicationFrameCounter >= communicationFrameCount){
        runFrame = true;
        communicationFrameCounter = 0;
      } else {
        communicationFrameCounter = communicationFrameCounter + 1;
        runFrame = false;
      }
      TIMSK2 &= ~(1 << OCIE2A); // Disable Timer 2 Compare Match A interrupt
      PORTD |= (1 << PD6);      // Set PD6 HIGH
      busBitIndex = 0;
      previousInBus = inBus;
      printBus = true;
    }
    IRWaiting = true; // zorgt ervoor dat de IR-reciever een pauze krijgt
    communicationOffset = TCNT0;
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
  PORTD ^= (1 << PD6); // oscilleer de IR-led met 38kHz
}

ISR(INT0_vect)
{
  TCNT0 = (COMMUNICATIONOFFSETMAX - (communicationOffset * 5)); // wisselende tijd tussen de 1 timers
  busBitIndex = 0;
  EIMSK &= ~(1 << INT0); // INT0 interrupt disable
}
