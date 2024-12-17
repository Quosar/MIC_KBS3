#include "Display.h"

#define _backlight_pin PD5
#define _speaker_pin PD3

Display::Display() : display(TFT_CS, TFT_DC) {
  // Reference voltage set to AVCC (5V), ADC0 as input and left adjusted
  ADMUX &= ~((1<<MUX0)|(1<<MUX1)|(1<<MUX2)|(1<<MUX3));
  ADMUX |= (1<<REFS0) | (1<<ADLAR);
  // Enable ADC, set prescaler to 128 for accuracy (16MHz / 128 = 125kHz)
  ADCSRA = (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
  // Enable ADC
  ADCSRA |= (1<<ADEN);

  // Fast PWM mode, non-inverting
  TCCR0A |= (1<<WGM00) | (1<<WGM01);
  TCCR0A |= (1<<COM0B1);
  //Prescaler 64
  TCCR0B |= (1<<CS00) | (1<<CS01);

  //TIMSK0 |= (1 << OCIE0A);

  TCNT0 = 0;

  // Set the backlight pin as output
  DDRD |= ((1 << _backlight_pin));

  display.begin();
}

Display::~Display() {}

void Display::begin() {
    touchscreen.begin();
    display.fillScreen(ILI9341_BLACK);
}

void Display::fillScreen(uint16_t color) { display.fillScreen(color); }

void Display::drawPixel(int16_t x, int16_t y, uint16_t color) {
  display.drawPixel(x, y, color);
}

void Display::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                       uint16_t color) {
  display.drawLine(x0, y0, x1, y1, color);
}

void Display::setCursor(int16_t x, int16_t y) { display.setCursor(x, y); }

void Display::setTextColor(uint16_t color, uint16_t bg) {
  display.setTextColor(color, bg);
}

void Display::setTextSize(uint8_t size) { display.setTextSize(size); }

void Display::print(const char *text) { display.print(text); }

void Display::println(const char *text) { display.println(text); }

void Display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                       uint16_t color) {
  display.fillRect(x, y, w, h, color);
}

void Display::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                           int16_t x2, int16_t y2, uint16_t color) {
  display.fillTriangle(x0, y0, x1, y1, x2, y2, color);
}

void Display::fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color) {
  display.fillCircle(x, y, r, color);
}

void Display::drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
                       uint16_t color) {
  display.drawRect(x, y, w, h, color);
}

void Display::setTextColor(uint16_t color)
{
    display.setTextColor(color);
}

void Display::print(String text) {
    display.print(text);
}

void Display::println(String text) {
    display.println(text);
}

void Display::print(int8_t text) {
    display.print(text);
}

void Display::println(int8_t text) {
    display.println(text);
}

void Display::refreshBacklight() {
    // Add code to refresh the backlight as needed
    if(!(ADCSRA & (1<<ADSC))){
        OCR0B = ADCH;
        OCR0A = ADCH;
    }

    

    ADCSRA |= (1<<ADSC);
}

TS_Point Display::getPoint() {
  return touchscreen.getPoint();
}

uint8_t Display::width()
{
    return 240;
}

uint16_t Display::height()
{
    return 320;
}
