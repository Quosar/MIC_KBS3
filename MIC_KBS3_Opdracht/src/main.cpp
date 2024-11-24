#include <Arduino.h>
#include <Wire.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "Nunchuk.h"

NunChuk nunchuck;
bool zPressed;
const uint8_t NUNCHUCK_ADDRESS = 0x52;

// IR Pins
#define IR_TRANSMITTER_PIN PD6 
#define IR_RECEIVER_PIN PD2   

// I2C address for 7-segment display
uint8_t address = 0x21;

// Timer interrupt voor 38 kHz toggling van IR transmitter
ISR(TIMER0_COMPA_vect) {
  PORTD ^= (1 << IR_TRANSMITTER_PIN); // Toggle PD6
}

void startTimer0() { TIMSK0 |= (1 << OCIE0A); }

void stopTimer0() { TIMSK0 &= ~(1 << OCIE0A); }

// init timer0 voor 38 kHz
void init_timer0() {
  // PD6 OUTPUT
  DDRD |= (1 << IR_TRANSMITTER_PIN);

  // Timer0 CTC modus
  TCCR0A = (1 << WGM01); // CTC modus
  TCCR0B = (1 << CS00);  // Geen prescaler (snelste klok)

  // outputcompare in voor 38 kHz
  OCR0A = 209; // (16 MHz / (2 * 38 kHz)) - 1

  // compare match interrupt aan
  TIMSK0 = (1 << OCIE0A);
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

int main() {
  Wire.begin();
  init_timer0(); // starten van de timer voor freq
  sei();         // global interupts aan

  // Zet PD2 als input
  DDRD &= ~(1 << IR_RECEIVER_PIN);

  while (1) {
    //checken of nunchuck z is ingedrukt
    if(nunchuck.getState(NUNCHUCK_ADDRESS)){
      zPressed = nunchuck.state.z_button;
    }
    if (zPressed)
    {
      startTimer0(); //start timer0 interupts als knop is ingedrukt
    }else{
      stopTimer0(); //stop timer0 ints z is los
    }

    // Controleer de status van IR_RECEIVER_PIN (PD2)
    if (PIND & (1 << IR_RECEIVER_PIN)) {
      sendToSegmentDisplay(0); // Stuur 0 naar display
    } else {
      sendToSegmentDisplay(1); // Stuur 1 naar display
    }

    _delay_ms(10);
  }

  return 0;
}
