#ifndef SNAKE___H
#define SNAKE___H

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

class Snake {
public:
  Snake(uint8_t gridSize, uint16_t cellWidth, uint16_t cellHeight,
        Adafruit_ILI9341 screen, uint16_t colour);

  void start(uint8_t x, uint8_t y);
  void updateDirection(uint8_t joyX, uint8_t joyY);
  void move();
  void draw();
  bool checkCollision();
  void grow();
  bool eatApple(uint8_t appleX, uint8_t appleY);
  void spawnRandApple();
  void reset();
  void drawScore();
  void drawDeathScreen();
  void drawStartMenu();
  enum Direction { UP, DOWN, LEFT, RIGHT };
  Direction direction;
  Direction getDirection();
  void setDirection(Direction direction);


  uint8_t appleX; // appel coords public voor communicatie
  uint8_t appleY;
  uint8_t snakeLength;

private:
  uint8_t gridSize;
  uint16_t cellWidth, cellHeight;
  uint8_t *snakeX; // pointer voor dynamische array
  uint8_t *snakeY;
  Adafruit_ILI9341 screen;
  uint16_t colour;

  // enum Direction { UP, DOWN, LEFT, RIGHT };
  // Direction direction;
  
  Direction bufferedDirection;

  void clearTail(uint8_t tailX, uint8_t tailY);
  void drawCell(uint16_t x, uint16_t y, uint16_t color);
  void validateDirection();
  void drawHead(uint16_t x, uint16_t y);
};

#endif
