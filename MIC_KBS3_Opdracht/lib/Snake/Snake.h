#ifndef SNAKE___H
#define SNAKE___H

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

class Snake {
public:
  Snake(uint8_t gridSize, uint16_t cellWidth, uint16_t cellHeight,
        Adafruit_ILI9341 screen);

  void start();
  void updateDirection(uint8_t joyX, uint8_t joyY);
  void move();
  void draw();
  bool checkCollision();
  void grow();
  uint8_t *snakeX; // pointer for dynamic memory, because we dont always use the
                   // hole array
  uint8_t *snakeY;
  uint8_t snakeLength;

private:
  uint8_t gridSize;
  uint16_t cellWidth, cellHeight;
  Adafruit_ILI9341 screen;
  enum Direction { UP, DOWN, LEFT, RIGHT };
  Direction direction;

  void clearTail(uint8_t tailX, uint8_t tailY);
  void drawCell(uint16_t x, uint16_t y, uint16_t color);
};

#endif
