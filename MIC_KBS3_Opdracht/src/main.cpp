#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Nunchuk.h"
#include <Arduino.h>
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

// I2C address for 7-segment display
uint8_t address = 0x21;

// LCD object
Adafruit_ILI9341 screen(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

NunChuk nunchuck;
bool zPressed;
const uint8_t NUNCHUCK_ADDRESS = 0x52;

// Timer interrupt voor 38 kHz toggling van IR transmitter
ISR(TIMER0_COMPA_vect) {
  PORTD ^= (1 << IR_TRANSMITTER_PIN); // Toggle PD6
}

void startTimer0() { TIMSK0 |= (1 << OCIE0A); }

void stopTimer0() { TIMSK0 &= ~(1 << OCIE0A); }

// Init timer0 voor 38 kHz
void init_timer0() {
  DDRD |= (1 << IR_TRANSMITTER_PIN); // PD6 OUTPUT
  TCCR0A = (1 << WGM01);             // CTC modus
  TCCR0B = (1 << CS00);              // Geen prescaler
  OCR0A = 209;                       // (16 MHz / (2 * 38 kHz)) - 1
  TIMSK0 = (1 << OCIE0A);            // Compare match interrupt aan
}

// Verstuur een cijfer naar het 7-segment display via I2C
void sendToSegmentDisplay(uint8_t value) {
  Wire.beginTransmission(address);
  if (value == 0) {
    Wire.write(0b11000000); // 0 displayen
  } else if (value == 1) {
    Wire.write(0b11111001); // 1 displayen
  }
  Wire.endTransmission();
}

// display updaten per stukje ipv fillscreen
void updateLCD(const char *message, uint16_t color, int x, int y) {
  screen.setCursor(x, y);
  screen.setTextColor(color, BLACK); //tekstoverschrijven met zwart
  screen.print(message);
}

int main() {
  Wire.begin();
  init_timer0();
  sei(); // Globale interrupts aan

  // Zet PD2 als input
  DDRD &= ~(1 << IR_RECEIVER_PIN);

  // LCD setup
  screen.begin();
  screen.fillScreen(BLACK);
  screen.setTextSize(2);

  // Geinitialiseerde staat van de  LCD
  updateLCD("IR Status: OFF", WHITE, 10, 10);
  updateLCD("7-Seg: ", WHITE, 10, 40);

  while (1) {
    // Checken of nunchuck Z-knop is ingedrukt
    if (nunchuck.getState(NUNCHUCK_ADDRESS)) {
      zPressed = nunchuck.state.z_button;
    }

   //starten of stoppen van signaal voor communicatie
    if (zPressed) {
      startTimer0();
      updateLCD("IR Status: zeer actief ", GREEN, 10, 10); // als knop ingedrukt infrarood sturen
    } else {
      stopTimer0();
      updateLCD("IR Status: ik sta best wel uit", RED, 10, 10);
    }

    // kijken of signaal ontvangen
    if (PIND & (1 << IR_RECEIVER_PIN)) {
      sendToSegmentDisplay(0); // Stuur 0 naar display
      updateLCD("7Seg laat zien 0", WHITE, 10, 40); // display aanpassen
    } else {
      sendToSegmentDisplay(1); // Stuur 1 naar display
      updateLCD("7Seg laat zien 1", WHITE, 10, 40); //display aanpassen
    }
    _delay_ms(10);
  }

  return 0;
}
