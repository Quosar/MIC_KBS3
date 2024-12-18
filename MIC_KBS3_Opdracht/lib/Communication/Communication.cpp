#include "Communication.h"
#include <Snake.h>

#define IR_LED_PIN PD6         // Pin 6 voor IR LED (OC0A)
#define IR_RECEIVER_PIN PD2    // Pin 2 voor IR Receiver (INT0)
#define DATABITCOUNT 32        // bits in een databus
#define COMMUNICATIONSPEED 200 // snelheid van timer 1 interrupts
#define OCSILLATIONSPEED 209   // 38kHz oscilleer snelheid led pin
#define COMMUNICATIONOFFSETMIN 0

volatile bool isSender = true; // player1 begint met zenden en zetten timer

// communication
volatile uint32_t firstSyncCheck =
    0x00000000;                                 // start bit detecteren
                                                // gedetecteerd kan worden zoals bijvoorbeeld 0x33333333
volatile uint32_t secondSyncCheck = 0xAAAAAAAA; // 2863311530
volatile uint32_t thirdSyncCheck = 0xAE6CB249;  // 2926359113
volatile uint32_t outBus =
    firstSyncCheck;          // Uitgaande data bus begint als firstSyncCheck om
                             // communicatie te synchroniseren
volatile uint32_t inBus = 0; // Binnenkomende data bus
volatile uint32_t previousInBus = 0;
volatile uint8_t busBitIndex = 0; // huidige bit index inBus

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
volatile bool communicationNearlyInitialized = false;
volatile uint8_t SyncingIndex = 0;
volatile bool SyncingIndexFound = false;
volatile uint8_t syncCounter = 0;
volatile uint8_t syncCount = 20;

// GameFrame variables
volatile uint8_t communicationFrameCounter = 0;
volatile uint8_t communicationFrameCount = 15; // mag niet hoger dan 63
volatile bool runFrame = true;
volatile bool runCommunicationFrame = false;
volatile uint8_t timerOffset = 0;
volatile bool offsetFound = false;

// game variables
volatile uint8_t snakeDirection = 0;
volatile uint8_t posApple = 0;
volatile bool isPlayer1 = false;
volatile bool isSmallField = false;
volatile bool appleGatheredByPlayer2 = false;
volatile bool gamePaused = true;
volatile bool isAlive = false;
volatile uint8_t checksum;

volatile uint8_t snakeDirectionOther = 0;
volatile uint8_t posAppleOther = 0;
volatile bool isPlayer1Other = false;
volatile bool isSmallFieldOther = false;
volatile bool appleGatheredByPlayer2Other = false;
volatile bool gamePausedOther = true;
volatile bool isAliveOther = false;

volatile bool gameRunning = true;

void Communication::setupPins()
{
  DDRD |= (1 << PD6); // PD6 Ouptut

  DDRD &= ~(1 << IR_RECEIVER_PIN);  // Set PD2 as input
  PORTD &= ~(1 << IR_RECEIVER_PIN); // Enable pull-up resistor

  DDRD |= (1 << PD7);   // PD7 Ouptut
  PORTD &= ~(1 << PD7); // PD7 begint LOW
}

void Communication::setupTimers()
{
  // Correct Timer 1 setup for CTC mode with prescaler 64
  TCCR1A = 0;
  TCCR1B = 0;
  OCR1A = 0;
  TCCR1B |= (1 << WGM12);              // Correct: WGM12 is in TCCR1B
  TCCR1B |= (1 << CS11) | (1 << CS10); // Prescaler 64
  OCR1A = COMMUNICATIONSPEED;

  // Correct Timer 0 setup
  TCCR0A = 0;
  TCCR0B = 0;
  TCCR0A |= (1 << WGM01);   // CTC mode
  TCCR0B |= (1 << CS00);    // No prescaler
  TCCR0A &= ~(1 << COM0A0); // Turn on oscillation
  OCR0A = OCSILLATIONSPEED;
}

void Communication::SetupInterrupts()
{
  EICRA &= ~(1 << ISC01);
  EICRA &= ~(1 << ISC00); // Trigger bij LOW
  if (isSender)
  {
    EIMSK &= ~(1 << INT0); // INT0 interrupt disable
  }
  if (!isSender)
  {
    EIMSK |= (1 << INT0); // INT0 interrupt enable
  }
}

void Communication::initializeCommunication()
{
  busBitIndex = 0;
  TCNT1 = 0;
  TIMSK1 |= (1 << OCIE1A); // Enable Timer1 Compare Match A interrupt
  if (isSender)
  {
    senderOffset = 1;
    outBus = firstSyncCheck;
  }
  else
  {
    outBus = secondSyncCheck;
  }
}

bool Communication::getSender() {
  return isSender;
}

bool Communication::getRunFrame(){
  return runFrame;
}

uint8_t Communication::constructChecksum(uint32_t value)
{
  value = value >> 3;
  return value % 8;
}

uint32_t Communication::constructBus(Snake &snake)
{
  uint32_t out = 0;

  out |= ((uint32_t)snake.direction & 0x03) << 30;   // Bits 31–30: stickDirection
  out |= ((uint32_t)communicationFrameCounter & 0x3F) << 24; // Bits 29–24: frameCount
  out |= ((uint32_t)snake.snakeLength & 0xFF) << 16;         // Bit 23–16: lengthSnake
  out |= ((uint32_t)posApple & 0xFF) << 8;                   // Bit 15–8: posApple
  out |=
      ((isPlayer1 & 0x01) << 7) |              // Bit 7: isPlayer1
      ((isSmallField & 0x01) << 6) |           // Bit 6: isSmallField
      ((appleGatheredByPlayer2 & 0x01) << 5) | // Bit 5: appleGatheredByPlayer2
      ((gameRunning & 0x01) << 4) |            // Bit 4: gamePaused
      ((isAlive & 0x01) << 3);                 // Bit 3: isAlive

  uint8_t checksum = constructChecksum(out);
  out |= (uint32_t)(checksum & 0x07); // Bits 2–0: checksum

  return out;
}

void Communication::deconstructBus(uint32_t bus, Snake &snake)
{
  uint8_t checksum = (uint8_t)(bus & 0x07); // Extract checksum
  uint8_t calculatedChecksum = constructChecksum(bus & 0xFFFFFFF8);

  if (checksum == calculatedChecksum)
  {
    snakeDirectionOther = (uint8_t)((bus >> 30) & 0x03); // Bits 31–30: stickDirection
    if (!isSender)
    {
      communicationFrameCounter = (uint8_t)((bus >> 24) & 0x3F); // Bits 29–24: frameCount
    }
    snake.snakeLength = (uint8_t)((bus >> 16) & 0xFF); // Bit 23–16: lengthSnake //TOFO naar andere snake zetten
    posAppleOther = (uint8_t)((bus >> 8) & 0xFF);      // Bit 15–8: posApple

    isPlayer1Other = (bus >> 7) & 0x01;              // Bit 7: isPlayer1
    isSmallFieldOther = (bus >> 6) & 0x01;           // Bit 6: isSmallField
    appleGatheredByPlayer2Other = (bus >> 5) & 0x01; // Bit 5: appleGatheredByPlayer2
    if (!isSender)
    {
      gameRunning = (bus >> 4) & 0x01; // Bit 4: gamePaused
    }
    isAliveOther = (bus >> 3) & 0x01; // Bit 3: isAlive
  }
}

void Communication::communicate()
{
  if (!IRWaiting) // checkt of de IR-reciever een pauze nodig heeft
  {
    if (busBitIndex == 0 && isSender) // checkt of de start bit gestuurd moet worden
    {
      inBus = 0x00000000;
      TCCR0A |= (1 << COM0A0); // Turn on oscillation
    }
    if (busBitIndex >= 1 + senderOffset &&
        busBitIndex <=
            DATABITCOUNT + senderOffset)
    {
      PORTD ^= (1 << PD7);
      bool bit = (PIND & (1 << IR_RECEIVER_PIN)) ==
                 0; // LOW is logische 1 in IR communicatie
      if (bit)
      {
        inBus |= (1UL << (DATABITCOUNT -
                          (busBitIndex - senderOffset))); // bepaalt welke index van de inBus
                                                          // de gelezen waarde in moet sender
                                                          // begint 1 later
      }
      else
      {
        inBus &= ~(
            1UL
            << (DATABITCOUNT -
                (busBitIndex - senderOffset))); // bepaalt welke index van de inBus de gelezen
                                                // waarde in moet sender begint 1 later
      }
      PORTD ^= (1 << PD7);
    }
    if (busBitIndex >= 1 &&
        busBitIndex <=
            DATABITCOUNT) // 1-32 voor het sturen en lezen van de bits
    {
      // Transmit current bit
      bool outBit = (outBus >> (DATABITCOUNT - (busBitIndex))) &
                    0x01; // bepaalt of de volgende bit in de outBus en 1 of 0
                          // moet transmitten
      if (outBit)
      {
        TCCR0A |= (1 << COM0A0); // Turn on oscillation
      }
      else
      {
        TCCR0A &= ~(1 << COM0A0);
        PORTD &= ~(1 << PD6); // Ensure PD6 is LOW
      }
    }

    busBitIndex++; // verhoogt de index

    if (busBitIndex >
        DATABITCOUNT + 1 + (senderOffset * 3)) // checkt of de laatste bit is geweest en checkt of
                                               // hij alles nu mag resetten
    {
      if (!communicationInitialized)
      {
        if (inBus == secondSyncCheck &&
            isSender) //  geedt aan de andere kant aan dat de
                      //  communicatie is geïnitializeerd
        {
          outBus = secondSyncCheck;
        }
        if (inBus == secondSyncCheck && !isSender)
        {
          outBus = thirdSyncCheck;
          communicationNearlyInitialized = true;
        }
        if (inBus == thirdSyncCheck && isSender)
        {
          communicationInitialized = true;
        }
        if (communicationNearlyInitialized && !isSender)
        {
          if (syncCounter <= syncCount)
          {
            syncCounter = syncCounter + 1;
          }
          if (syncCounter > syncCount)
          {
            communicationInitialized = true;
          }
        }
      }

      if (communicationFrameCounter >= 15)
      {
        runFrame = true;
        communicationFrameCounter = 0;
      }
      if (isSender)
      {
        communicationFrameCounter = communicationFrameCounter + 1;
      }
      if (!isSender) // zet int 0 interrupt aan om te zorgen dat de !isSender de
                     // startbit binnen krijgt om de timers synchroon te laten
                     // lopen
      {
        EIMSK |= (1 << INT0); // INT0 interrupt enable
        TIMSK1 &= ~(1 << OCIE1A);
      }
      printBus = true;
      busBitIndex = 0;

    }
    IRWaiting = true; // zorgt ervoor dat de IR-reciever een pauze krijgt
  }
  else
  {
    TCCR0A &= ~(1 << COM0A0);
    PORTD |= (1 << PD6); // Ensure PD6 is LOW
    IRWaiting = false;
  }
  runCommunicationFrame = false;
}