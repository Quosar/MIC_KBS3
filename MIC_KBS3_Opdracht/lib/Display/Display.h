#ifndef DISPLAY___H
#define DISPLAY___H
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// LCD Pin Defines
#define TFT_CLK 13
#define TFT_MISO 12
#define TFT_MOSI 11
#define TFT_DC 9
#define TFT_CS 10
#define TFT_RST 8

class Display {
private:
  Adafruit_ILI9341 display;

public:
  Display();
  ~Display();

  void begin();
  void fillScreen(uint16_t color);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  void setCursor(int16_t x, int16_t y);
  void setTextColor(uint16_t color, uint16_t bg);
  void setTextSize(uint8_t size);
  void print(const char *text);
  void println(const char *text);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2,
                    int16_t y2, uint16_t color);
  void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color);
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void setTextColor(uint16_t color);
  void print(String text);

  uint8_t width();
  uint16_t height();
};

#endif
