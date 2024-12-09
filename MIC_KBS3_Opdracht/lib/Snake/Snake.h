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
  void drawHighscore(uint8_t score);
  void drawPlayer1Text(bool selected);
  void drawPlayer2Text(bool selected);
  void drawGameStartButton(bool isPlayer1);
  void drawElement(uint8_t element, bool selected, bool isPlayer1, bool redrawBody, bool isStartup);
  uint8_t appleX; // appel coords public voor communicatie
  uint8_t appleY;

private:
  uint8_t gridSize;
  uint16_t cellWidth, cellHeight;
  uint8_t snakeLength;
  uint8_t *snakeX; // pointer voor dynamische array
  uint8_t *snakeY;
  Adafruit_ILI9341 screen;
  uint16_t colour;

  enum Direction { UP, DOWN, LEFT, RIGHT };
  Direction direction;

  void clearTail(uint8_t tailX, uint8_t tailY);
  void drawCell(uint16_t x, uint16_t y, uint16_t color);
};

#endif
