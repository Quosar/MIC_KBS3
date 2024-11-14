#include "Nunchuk.h"
#include <Arduino.h>

const int NUNCHUCK_ADDRESS = 0x52;
NunChuk nunchuck;

void setup() {
  init();
  Serial.begin(9600);
  nunchuck.begin(NUNCHUCK_ADDRESS);
}

uint8_t getNunchuckXAxis() { return nunchuck.state.joy_x_axis; }
uint8_t getNunchuckYAxis() { return nunchuck.state.joy_y_axis; }

int main() {
  setup();
  while (1)
  {
    if (nunchuck.getState(NUNCHUCK_ADDRESS)) {
      Serial.print("X axis pos: ");
      Serial.println((int)getNunchuckXAxis());
      Serial.print("Y axis pos: ");
      Serial.println((int)getNunchuckYAxis());
      Serial.println("------------------------");
      Serial.flush();
    }
    else
    {
      Serial.println("Foutje bij lezen?!");
    }
    _delay_ms(500);
  }
}
