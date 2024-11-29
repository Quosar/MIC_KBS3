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
  bool eatApple(uint8_t appleX, uint8_t appleY);

  uint8_t appleX; //appel coords public voor communicatie
  uint8_t appleY;

private:
  uint8_t gridSize;
  uint16_t cellWidth, cellHeight;
  uint8_t snakeLength;
  uint8_t *snakeX; // pointer voor dynamische array
  uint8_t *snakeY;
  Adafruit_ILI9341 screen;
  enum Direction { UP, DOWN, LEFT, RIGHT };
  Direction direction;

  void clearTail(uint8_t tailX, uint8_t tailY);
  void drawCell(uint16_t x, uint16_t y, uint16_t color);
};

#endif
