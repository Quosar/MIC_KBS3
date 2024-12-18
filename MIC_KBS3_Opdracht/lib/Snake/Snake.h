#ifndef SNAKE___H
#define SNAKE___H

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "Display.h"

class Snake {
public:
  Snake(uint8_t gridSize, uint16_t cellWidth, uint16_t cellHeight,
         Display &screen, uint16_t colour);

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
  void drawDeathScreen(bool isWinner, uint8_t lengthPlr1, uint8_t lengthPlr2);
  void drawStartMenu();
  void drawHighscore(uint8_t score);
  void drawPlayer1Text(bool selected);
  void drawPlayer2Text(bool selected);
  void drawGameStartButton(bool isPlayer1);
  void drawElement(uint8_t element, bool selected, bool isPlayer1, bool redrawBody, bool isStartup);

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

private:
  Display &screen;

  uint8_t gridSize;
  uint16_t cellWidth, cellHeight;
  uint8_t *snakeX; // pointer voor dynamische array
  uint8_t *snakeY;
  uint16_t colour;
  uint8_t score = 0;
  uint8_t highscore = 0;

  // enum Direction { UP, DOWN, LEFT, RIGHT };
  // Direction direction;
  
  Direction bufferedDirection;

  void clearTail(uint8_t tailX, uint8_t tailY);
  void drawCell(uint16_t x, uint16_t y, uint16_t color);
  void validateDirection();
  void drawHead(uint16_t x, uint16_t y);
};

#endif
