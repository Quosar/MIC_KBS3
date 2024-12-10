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
// 0 = POSX, 1 = POSY, 2 = SIZEX, 3 = SIZEY, 4 = BODYCOLOR, 5 = BORDERCOLOR, 6 = TEXTCOLOR, 7 = TEXTCURSORPOSX, 8 = TEXTCURSORPOSY, 9 = TEXTSIZE
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
  bufferedDirection = RIGHT;                 // gebufferde richting dus ook
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
    bufferedDirection = LEFT;
  else if (joyX > 145 && direction != LEFT)
    bufferedDirection = RIGHT;
  else if (joyY < 105 && direction != UP)
    bufferedDirection = DOWN;
  else if (joyY > 145 && direction != DOWN)
    bufferedDirection = UP;
}

void Snake::move() {
  validateDirection();

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

  // teken hoofd
  drawHead(snakeX[0], snakeY[0]);
  // alles na hoofd veranderd naar body
  // eerst hoofd clearen
  screen.fillRect(snakeX[1] * cellWidth, snakeY[1] * cellHeight, cellWidth,
                  cellHeight, BLACK);
  // teken body deel
  drawCell(snakeX[1], snakeY[1], colour);

  // draw appel
  drawCell(appleX, appleY, RED);

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
    drawHead(appleX, appleY);
    spawnRandApple();

    return true; // appel is gegeten
  }
  return false; // niet gegeten
}

void Snake::spawnRandApple() {
  srand(TCNT0); // rand seed //TODO: seed vervangen voor clock waarde
  appleX = rand() % gridSize; // random appel spawn in het veld
  appleY = rand() % gridSize; // random appel spawn in het veld
}

// alleen staart clearen ipv hele scherm
void Snake::clearTail(uint8_t tailX, uint8_t tailY) {
  drawCell(tailX, tailY, BLACK);
}

// teken snake cell
void Snake::drawCell(uint16_t x, uint16_t y, uint16_t colour) {
  // de snake cell centeren op juiste locaties binnen het grid
  uint16_t centerX = x * cellWidth + cellWidth / 2;
  uint16_t centerY = y * cellHeight + cellHeight / 2;
  screen.fillCircle(centerX, centerY, cellWidth / 2, colour);
}

void Snake::validateDirection() {
  if ((direction == UP && bufferedDirection != DOWN) ||
      (direction == DOWN && bufferedDirection != UP) ||
      (direction == LEFT && bufferedDirection != RIGHT) ||
      (direction == RIGHT && bufferedDirection != LEFT)) {
    direction = bufferedDirection;
  }
}

void Snake::reset() {
  // snakelengte resetten
  snakeLength = SNAKE_START_LENGHT; // start lenget snake

  // snake position resetten naar het midden
  start(gridSize / 2, gridSize / 2);

  // alles wat niet de snake is clearen
  for (int i = 1; i < gridSize * gridSize; i++) {
    snakeX[i] = 0;
    snakeY[i] = 0;
  }

  // start richting zetten
  direction = RIGHT;

  // screen resetten van oude snakes en appels
  screen.fillScreen(BLACK);

  // nieuwe appel spawnen
  spawnRandApple();
}

// void Snake::setHighscore(uint8_t newScore) {

// }

uint16_t Snake::getScore() {
  return score;
}

uint16_t Snake::getHighscore() {
  if (score > highscore) {
    highscore = score;
  }
  return highscore;
}

// Draw Score Text
void Snake::drawScore() {
  static uint8_t prevScore = -1;
  int8_t currentScore = snakeLength - SNAKE_START_LENGHT;
  score = currentScore;
  // score alleen updaten/tekenen wanneer de score verhoogd
  if (currentScore != prevScore) {
    screen.setCursor(5, TFT_WIDTH + 5);
    screen.setTextColor(WHITE, BLACK); // oude overschrijven met zwart
    screen.setTextSize(1);
    screen.print("Score: ");
    screen.print(score);
    prevScore = currentScore;
  }
}

// Draw Death Screen
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
  // Temporary Storage for storing Element Data
  uint16_t tempStorageArray[10] = {};
  String elementText1 = "";
  String elementText2 = "";
  
  // Store Element Data In Temporary Array
  if (element == 0) {         // if TopText 
    copyArray(menuTopRenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[0];
  } else if (element == 1) {  // if Player1
    copyArray(menuPlr1RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[1];
  } else if (element == 2) {  // if Player2
    copyArray(menuPlr2RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[2];
    if (selected) {
      tempStorageArray[4] = menuPlr1SelectRenderData[0];
      tempStorageArray[5] = menuPlr1SelectRenderData[1];
      tempStorageArray[6] = menuPlr1SelectRenderData[2];
    }
  } else if (element == 3) {  // if Mode1
    copyArray(menuMode1RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[3];
    if (selected) {
      tempStorageArray[5] = menuModeColor;
      tempStorageArray[6] = menuModeColor;
    }
  } else if (element == 4) {  // if Mode2
    copyArray(menuMode2RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[4];
        if (selected) {
      tempStorageArray[5] = menuModeColor;
      tempStorageArray[6] = menuModeColor;
    }
  } else if (element == 5) {  // if Mode3
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
  } else if (element == 6) {  // if StartButtonPlayer1
    copyArray(menuStartPlr1RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[7];
    elementText2 = menuRenderTextData[8];
  } else if (element == 7) {  // if StartButtonPlayer2
    copyArray(menuStartPlr2RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[9];
    elementText2 = menuRenderTextData[10];
  } else if (element == 8) {  // if Highscore
    copyArray(menuHScoreRenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[11] + getHighscore();
  }

  // Draw Element
  screen.setCursor(tempStorageArray[7] + tempStorageArray[0], tempStorageArray[8] + tempStorageArray[1]);                                     // Set Cursor
  screen.setTextColor(tempStorageArray[6]);                                                                                                   // Set Text Color
  if (isStartup) {                                                                                                                            // If this is on Startup
    screen.fillRect(tempStorageArray[0] + 5, tempStorageArray[1] + 5, tempStorageArray[2], tempStorageArray[3], LGREY);                       // Draw Backdrop Rectangle
  }
  if (redrawBody) {                                                                                                                           // If this element should be redrawn
    screen.fillRect(tempStorageArray[0], tempStorageArray[1], tempStorageArray[2], tempStorageArray[3], tempStorageArray[4]);                 // Draw Background Rectangle
  }
  screen.drawRect(tempStorageArray[0] - 1, tempStorageArray[1] - 1, tempStorageArray[2] + 2, tempStorageArray[3] + 2, tempStorageArray[5]);   // Draw Boundary Rectangle
  screen.print(elementText1);                                                                                                                 // Draw Text
  if (elementText2.length() > 0) {                                                                                                            // if there is a second line of text
    if (isPlayer1) {                                                                                                                          // if this is Player1
      screen.setCursor(menuStartCursorLn2Data[0] + tempStorageArray[0], menuStartCursorLn2Data[1] + tempStorageArray[1]);                     // Set Cursor
      screen.print(menuRenderTextData[8]);                                                                                                    // Draw Line 2
    } else {                                      
      screen.setCursor(menuStartCursorLn2Data[2] + tempStorageArray[0], menuStartCursorLn2Data[3] + tempStorageArray[1]);                     // Set Cursor                
      screen.print(menuRenderTextData[10]);                                                                                                   // Draw Line 2
    }
  }
}

void Snake::drawStartMenu() {
  // Preset Text Size
  screen.setTextSize(2);

  // Screen Border
  screen.drawRect(0, 0, screen.width(), screen.height(), WHITE);
  screen.drawRect(1, 1, screen.width() - 2, screen.height() - 2, MAGENTA);
  screen.drawRect(2, 2, screen.width() - 4, screen.height() - 4, WHITE);

  // Draw Top Game Text
  drawElement(0, false, false, true, true);

  // // Draw Player 1 Button
  // drawElement(1, false, false, true, true);

  // // Draw Player 2 Text
  // drawElement(2, false, false, true, true);

  // // Draw Mode 1 Button (8 x 8)
  // drawElement(3, false, false, true, true);

  // // Draw Mode 2 Button (16 x 16)
  // drawElement(4, true, false, true, true);

  // Draw Mode 3 Button (Normal / Fast)
  drawElement(5, false, false, true, true);

  // TEMPORARY
  // Draw Start Game Button
  drawElement(6, false, true, true, true);

  // Draw Highscore Text
  drawElement(8, false, false, true, true);
}

Snake::Direction Snake::getDirection(){ return direction; }

void Snake::setDirection(Direction direction) { direction = direction; }

void Snake::drawHead(uint16_t x, uint16_t y) {

  // center van driehoek berekenen
  uint16_t centerX = x * cellWidth + cellHeight / 2;
  uint16_t centerY = y * cellHeight + cellWidth / 2;

  // driehoek punten op basis van de richting van de snake
  int16_t x1, y1, x2, y2, x3, y3;
  switch (direction) {
  case UP:
    x1 = centerX - cellWidth / 2;
    y1 = centerY + cellHeight / 2;
    x2 = centerX + cellWidth / 2;
    y2 = centerY + cellHeight / 2;
    x3 = centerX;
    y3 = centerY - cellHeight / 2;
    break;
  case DOWN:
    x1 = centerX - cellWidth / 2;
    y1 = centerY - cellHeight / 2;
    x2 = centerX + cellWidth / 2;
    y2 = centerY - cellHeight / 2;
    x3 = centerX;
    y3 = centerY + cellHeight / 2;
    break;
  case LEFT:
    x1 = centerX + cellWidth / 2;
    y1 = centerY - cellHeight / 2;
    x2 = centerX + cellWidth / 2;
    y2 = centerY + cellHeight / 2;
    x3 = centerX - cellWidth / 2;
    y3 = centerY;
    break;
  case RIGHT:
    x1 = centerX - cellWidth / 2;
    y1 = centerY - cellHeight / 2;
    x2 = centerX - cellWidth / 2;
    y2 = centerY + cellHeight / 2;
    x3 = centerX + cellWidth / 2;
    y3 = centerY;
    break;
  }

  // driehoek tekenen binnen de berekende punten
  screen.fillTriangle(x1, y1, x2, y2, x3, y3, colour);

  // ogern van de snake tekkenen
  // ogen komen op de eerste kwart van het hoofd, dus daarom /4 en hebben een
  // kleine grootte dus straal van 1/8 cellwidth
  switch (direction) {
    // omhoog en omlaag linker en rechter oog tekenen
  case UP:
  case DOWN:
    screen.fillCircle(centerX - cellWidth / 4, centerY, cellWidth / 8,
                      BLACK); // linker ook
    screen.fillCircle(centerX + cellWidth / 4, centerY, cellWidth / 8,
                      BLACK); // rechter oog
    break;
  // linker en rechter richting boven en onder oog tekenen
  case LEFT:
  case RIGHT:
    screen.fillCircle(centerX, centerY - cellHeight / 4, cellHeight / 8,
                      BLACK); // bovenste oog
    screen.fillCircle(centerX, centerY + cellHeight / 4, cellHeight / 8,
                      BLACK); // onderste oog
    break;
  }
}
