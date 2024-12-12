#include "Display.h"

Display::Display()
    : display(TFT_CS, TFT_DC) {
    display.begin();
}

Display::~Display() {}

void Display::begin() { display.begin(); }

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

uint8_t Display::width()
{
    return 240;
}

uint16_t Display::height()
{
    return 320;
}
