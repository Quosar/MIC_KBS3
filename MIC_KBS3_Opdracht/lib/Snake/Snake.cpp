#include "Snake.h"
#include "string.h"
#include "HardwareSerial.h"

// Color defines
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define LGREY 0x4208
#define DBLUE 0x0007

// Element Render Data
// 0 = POSX, 1 = POSX, 2 = SIZEX, 3 = SIZEY, 4 = BODYCOLOR, 5 = BORDERCOLOR, 6 = TEXTCOLOR, 7 = TEXTCURSORPOSX, 8 = TEXTCURSORPOSY, 9 = TEXTSIZE
const uint16_t menuTopRenderData[] = {70, 10, 100, 20, WHITE, RED, RED, 20, 3, 2}; 
const uint16_t menuPlr1RenderData[] = {10, 45, 100, 20, WHITE, BLUE, BLUE, 3, 3, 2};
const uint16_t menuPlr2RenderData[] = {130, 45, 100, 20, WHITE, BLUE, BLUE, 3, 3, 2};
const uint16_t menuMode1RenderData[] = {70, 90, 100, 20, WHITE, MAGENTA, MAGENTA, 20, 3, 2};
const uint16_t menuMode2RenderData[] = {70, 115, 100, 20, WHITE, MAGENTA, MAGENTA, 10, 3, 2};
const uint16_t menuMode3RenderData[] = {70, 140, 100, 20, WHITE, MAGENTA, MAGENTA, 18, 3, 2};
const uint16_t menuStartPlr1RenderData[] = {50, 180, 140, 70, DBLUE, CYAN, CYAN, 28, 10, 3};
const uint16_t menuStartPlr2RenderData[] = {50, 180, 140, 70, DBLUE, CYAN, CYAN, 7, 16, 2};
const uint16_t menuHScoreRenderData[] = {45, 280, 150, 20, DBLUE, YELLOW, YELLOW, 5, 3, 2};

const uint16_t menuModeColor = GREEN;

// BODYCOLOR, BORDERCOLOR, TEXTCOLOR
const uint16_t menuFastModeSelectRenderData[3] = {RED, CYAN, CYAN};

// BODYCOLOR, BORDERCOLOR, TEXTCOLOR
const uint16_t menuPlr1SelectRenderData[3] = {GREEN, BLACK, BLACK};

// 0 = PLR1POSX, 1 = PLR1POSY, 2 = PLR2POSX, 3 = PLR2POSY
const uint16_t menuStartCursorLn2Data[4] = {30, 36, 7, 34};

// Top, Plr1, Plr2, Mode1, Mode2, Normal, Fast, Start1, Start2, Waiting1, Waiting2, HScore
const String menuRenderTextData[] = {"Snake", "Player 1", "Player 2", "8 x 8", "16 x 16", "Normal", "FAST", "Start", "Game!", "Waiting for", "Player 1...", "Highscore:"};

const uint16_t TFT_WIDTH = 240;
const uint16_t TFT_HEIGHT = 320;

const uint8_t SNAKE_START_LENGHT = 3;

void copyArray(const uint16_t arrayOG[10], uint16_t newArray[10], uint8_t arraySize) {
  for (int i=0; i<arraySize; i++) {
    newArray[i] = arrayOG[i];
  }
}

Snake::Snake(uint8_t gridSize, uint16_t cellWidth, uint16_t cellHeight,
             Adafruit_ILI9341 screen, uint16_t colour)
    : gridSize(gridSize), cellWidth(cellWidth), cellHeight(cellHeight),
      screen(screen), colour(colour) {
  snakeLength = SNAKE_START_LENGHT;          // start lengte van de snake
  snakeX = new uint8_t[gridSize * gridSize]; // max grootte van de snake
  snakeY = new uint8_t[gridSize * gridSize]; // max grootte van de snake
  direction = RIGHT;                         // beginrichting is rechts
  spawnRandApple();
}

void Snake::start(uint8_t x, uint8_t y) {
  // snake starten in center scherm
  snakeX[0] = x;
  snakeY[0] = y;
}

// joystick met richting updaten
void Snake::updateDirection(
    uint8_t joyX,
    uint8_t joyY) { // TODO: remove magic numbers from the joystick angles
  if (joyX < 105 && direction != RIGHT)
    direction = LEFT;
  else if (joyX > 145 && direction != LEFT)
    direction = RIGHT;
  else if (joyY < 105 && direction != UP)
    direction = DOWN;
  else if (joyY > 145 && direction != DOWN)
    direction = UP;
}

void Snake::move() {
  // positie van de eindstaart opslaan
  uint8_t tailX = snakeX[snakeLength - 1];
  uint8_t tailY = snakeY[snakeLength - 1];

  // het lichaam bewegen
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }
  
  // hoofd bewegen
  switch (direction) {
  case UP:
    snakeY[0]--;
    break;
  case DOWN:
    snakeY[0]++;
    break;
  case LEFT:
    snakeX[0]--;
    break;
  case RIGHT:
    snakeX[0]++;
    break;
  }

  // oude staart clearen voor het tekenen van de nieuwe
  clearTail(tailX, tailY);
}

void Snake::draw() {
  // snake tekenen
  for (uint8_t i = 0; i < snakeLength; i++) {
    if (i == 0 || i == snakeLength - 1) { // alleen kop en staart tekenen
      drawCell(snakeX[i], snakeY[i], colour);
    }
  }

  // appel tekenen als hij niet gegeten wordt
  if(!(appleX == snakeX[0] && appleY == snakeY[0])){
    drawCell(appleX, appleY, RED);
  }

  // draw border
  screen.drawLine(0, TFT_WIDTH, TFT_WIDTH, TFT_WIDTH, WHITE);

  // draw score
  drawScore();
}

// collision checken met zichzelf en de borders
bool Snake::checkCollision() {
  // check for border collision
  if (snakeX[0] < 0 || snakeX[0] >= gridSize || snakeY[0] < 0 ||
      snakeY[0] >= gridSize) {
    return true;
  }
  // Check collision with itslef
  for (uint8_t i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      return true;
    }
  }
  return false; // als geen collision, return false
}

// lengte groeien van de slang
void Snake::grow() { snakeLength++; }

bool Snake::eatApple(uint8_t appleX, uint8_t appleY) {
  if (snakeX[0] == appleX &&
      snakeY[0] == appleY) { // als hoofd van de snake is op pos van appel
    drawCell(appleX, appleY, colour);
    spawnRandApple();
    return true; // appel is gegeten
  }
  return false; // niet gegeten
}

void Snake::spawnRandApple() {
  srand(millis());                // rand seed //TODO: seed vervangen voor clock waarde
  appleX = rand() % gridSize; // random appel spawn in het veld
  appleY = rand() % gridSize; // random appel spawn in het veld
}

// alleen staart clearen ipv hele scherm
void Snake::clearTail(uint8_t tailX, uint8_t tailY) {
  drawCell(tailX, tailY, BLACK);
}

// teken snake cell
void Snake::drawCell(uint16_t x, uint16_t y, uint16_t colour) {
  screen.drawRect(x * cellWidth, y * cellHeight, cellWidth, cellHeight, colour);
}

void Snake::reset() {
  // snakelengte resetten
  snakeLength = SNAKE_START_LENGHT; // start lenget snake

  // snake position resetten naar het midden
  start(gridSize / 2, gridSize / 2);

  // alles wat niet de snake is clearen
  for (int i = 1; i < gridSize * gridSize; i++)
  {
    snakeX[i] = 0;
    snakeY[i] = 0;
  }

  // start richting zetten
  direction = RIGHT;

  // nieuwe appel spawnen
  spawnRandApple();

  // screen resetten van oude snakes en appels
  screen.fillScreen(BLACK);
}

void Snake::drawScore() {
  screen.setCursor(5 , TFT_WIDTH + 5);
  screen.setTextColor(WHITE, BLACK); // oude overschrijven met zwart
  screen.setTextSize(1);
  screen.print("Score: ");
  screen.print(snakeLength - SNAKE_START_LENGHT);
}

void Snake::drawDeathScreen() {
  screen.fillScreen(BLACK);
  screen.setCursor(60, 160);
  screen.setTextColor(RED);
  screen.setTextSize(2);
  screen.println("Game Over!");
  screen.println("PRESS Z TO CONTINUE");
}
/*!
  @brief Draws an element with text, shadow and border.

  @param element The Index of the element: 0=Top; 1=Player1; 2=Player2; 3=Mode1; 4=Mode2; 5=Mode3; 6=StartPlr1; 7=StartPlr2; 8=Highscore
  @param selected Whether the drawn element has been selected by Touch.
  @param redrawBody Whether the body of the element should be drawn.
  @param isStartUp Whether the shadow of the element should be drawn.
*/
void Snake::drawElement(uint8_t element, bool selected, bool isPlayer1, bool redrawBody, bool isStartup) {
  uint16_t tempStorageArray[10] = {};
  String elementText1 = "";
  String elementText2 = "";
  
  if (element == 0) {
    copyArray(menuTopRenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[0];
  } else if (element == 1) {
    copyArray(menuPlr1RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[1];
  } else if (element == 2) {
    copyArray(menuPlr2RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[2];
    if (selected) {
      tempStorageArray[4] = menuPlr1SelectRenderData[0];
      tempStorageArray[5] = menuPlr1SelectRenderData[1];
      tempStorageArray[6] = menuPlr1SelectRenderData[2];
    }
  } else if (element == 3) {
    copyArray(menuMode1RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[3];
    if (selected) {
      tempStorageArray[5] = menuModeColor;
      tempStorageArray[6] = menuModeColor;
    }
  } else if (element == 4) {
    copyArray(menuMode2RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[4];
        if (selected) {
      tempStorageArray[5] = menuModeColor;
      tempStorageArray[6] = menuModeColor;
    }
  } else if (element == 5) {
    copyArray(menuMode3RenderData, tempStorageArray, 10);
    if (selected) {
      tempStorageArray[4] = menuFastModeSelectRenderData[0];
      tempStorageArray[5] = menuFastModeSelectRenderData[1];
      tempStorageArray[6] = menuFastModeSelectRenderData[2];
      tempStorageArray[7] = menuMode3RenderData[7] + 10;
      elementText1 = menuRenderTextData[6];
    } else {
      elementText1 = menuRenderTextData[5];
    }
  } else if (element == 6) {
    copyArray(menuStartPlr1RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[7];
    elementText2 = menuRenderTextData[8];
  } else if (element == 7) {
    copyArray(menuStartPlr2RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[9];
    elementText2 = menuRenderTextData[10];
  } else if (element == 8) {
    copyArray(menuHScoreRenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[11];
  }

  screen.setCursor(tempStorageArray[7] + tempStorageArray[0], tempStorageArray[8] + tempStorageArray[1]);
  screen.setTextColor(tempStorageArray[6]);
  if (isStartup) {
    screen.fillRect(tempStorageArray[0] + 5, tempStorageArray[1] + 5, tempStorageArray[2], tempStorageArray[3], LGREY);
  }
  if (redrawBody) {
    screen.fillRect(tempStorageArray[0], tempStorageArray[1], tempStorageArray[2], tempStorageArray[3], tempStorageArray[4]);
  }
  screen.drawRect(tempStorageArray[0] - 1, tempStorageArray[1] - 1, tempStorageArray[2] + 2, tempStorageArray[3] + 2, tempStorageArray[5]);
  screen.print(elementText1);
  if (elementText2.length() > 0) {
    if (isPlayer1) {
      screen.setCursor(menuStartCursorLn2Data[0] + tempStorageArray[0], menuStartCursorLn2Data[1] + tempStorageArray[1]);
      screen.print(menuRenderTextData[8]);
    } else {
      screen.setCursor(menuStartCursorLn2Data[2] + tempStorageArray[0], menuStartCursorLn2Data[3] + tempStorageArray[1]);
      screen.print(menuRenderTextData[10]);
    }
  }
}

void Snake::drawStartMenu() {
  screen.setTextSize(2);

  // Extra's
  screen.drawRect(0, 0, screen.width(), screen.height(), WHITE);
  screen.drawRect(1, 1, screen.width() - 2, screen.height() - 2, MAGENTA);
  screen.drawRect(2, 2, screen.width() - 4, screen.height() - 4, WHITE);

  drawElement(0, false, false, true, true);

  drawElement(1, false, false, true, true);

  drawElement(2, false, false, true, true);

  drawElement(3, true, false, true, true);

  drawElement(4, false, false, true, true);

  drawElement(5, false, false, true, true);

  drawElement(8, false, false, true, true);
}
