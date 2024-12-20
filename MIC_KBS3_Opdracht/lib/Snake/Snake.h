#ifndef SNAKE___H
#define SNAKE___H

#include "Display.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

class Snake {
public:
  Snake(uint8_t gridSize, uint16_t cellWidth, uint16_t cellHeight,
        Display &screen, uint16_t colour, bool primarySnake, bool sender);

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
  uint8_t getApplePosition();
  void setApplePosition(uint8_t applePosition);

  enum Sound { EAT, DEATH, STARTGAME };

  void playSound(Sound sound);
  void stopSound();

  enum Direction { UP, DOWN, LEFT, RIGHT };
  Direction direction;
  Direction getDirection();
  void setDirection(Direction direction);

  uint16_t getScore();
  uint16_t getHighscore();
  // void setHighscore(uint8_t newScore);

  uint8_t appleX; // appel coords public voor communicatie
  uint8_t appleY;
  uint8_t snakeLength;

  Direction bufferedDirection;

  bool isPrimarySnake;

private:
  Display &screen;

  uint8_t gridSize;
  uint16_t cellWidth, cellHeight;
  uint16_t colour;
  uint8_t score = 0;
  uint8_t highscore = 0;

  // enum Direction { UP, DOWN, LEFT, RIGHT };
  // Direction direction;

  uint8_t *snakeX; // pointer voor dynamische array
  uint8_t *snakeY;

  void clearTail(uint8_t tailX, uint8_t tailY);
  void drawCell(uint16_t x, uint16_t y, uint16_t color);
  void validateDirection();
  void drawHead(uint16_t x, uint16_t y);
};

#endif
