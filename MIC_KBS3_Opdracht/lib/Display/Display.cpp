#include "Display.h"

#define _backlight_pin PD5
#define _speaker_pin PD3

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
// 0 = POSX, 1 = POSY, 2 = SIZEX, 3 = SIZEY, 4 = BODYCOLOR, 5 = BORDERCOLOR, 6 =
// TEXTCOLOR, 7 = TEXTCURSORPOSX, 8 = TEXTCURSORPOSY, 9 = TEXTSIZE
const uint16_t menuTopRenderData[] = {70,  10,  100, 20, WHITE,
                                      RED, RED, 20,  3,  2};
const uint16_t menuPlr1RenderData[] = {10,   45,   100, 20, WHITE,
                                       BLUE, BLUE, 3,   3,  2};
const uint16_t menuPlr2RenderData[] = {130,  45,   100, 20, WHITE,
                                       BLUE, BLUE, 3,   3,  2};
const uint16_t menuMode1RenderData[] = {70,      90,      100, 20, WHITE,
                                        MAGENTA, MAGENTA, 20,  3,  2};
const uint16_t menuMode2RenderData[] = {70,      115,     100, 20, WHITE,
                                        MAGENTA, MAGENTA, 10,  3,  2};
const uint16_t menuMode3RenderData[] = {70,      140,     100, 20, WHITE,
                                        MAGENTA, MAGENTA, 18,  3,  2};
const uint16_t menuStartPlr1RenderData[] = {50,   180,  140, 70, DBLUE,
                                            CYAN, CYAN, 28,  10, 3};
const uint16_t menuStartPlr2RenderData[] = {50,   180,  140, 70, DBLUE,
                                            CYAN, CYAN, 7,   16, 2};
const uint16_t menuHScoreRenderData[] = {45,     280,    150, 20, DBLUE,
                                         YELLOW, YELLOW, 5,   3,  2};
const uint16_t dscreenWinStateRenderData[] = {45,    20,    150, 40, GREEN,
                                              BLACK, BLACK, 7,   8,  3};
const uint16_t dscreenScoreRenderData[] = {45,    70,    150, 60, WHITE,
                                           DBLUE, DBLUE, 10,  5,  1};
const uint16_t dscreenMenuRenderData[] = {35,   180,  80, 60, DBLUE,
                                          CYAN, CYAN, 18, 24, 2};
const uint16_t dscreenPlayAgainRenderData[] = {125,  180,  80, 60, DBLUE,
                                               CYAN, CYAN, 18, 10, 2};

const uint16_t menuModeColor = GREEN;

// BODYCOLOR, BORDERCOLOR, TEXTCOLOR
const uint16_t menuFastModeSelectRenderData[3] = {RED, CYAN, CYAN};

// BODYCOLOR, BORDERCOLOR, TEXTCOLOR
const uint16_t menuPlr1SelectRenderData[3] = {GREEN, BLACK, BLACK};

const uint16_t dscreenDefeatRenderData[3] = {BLACK, RED, RED};

// 0 = PLR1POSX, 1 = PLR1POSY, 2 = PLR2POSX, 3 = PLR2POSY
const uint16_t menuStartCursorLn2Data[4] = {30, 36, 7, 34};



Display::Display() : display(TFT_CS, TFT_DC) {
  // Reference voltage set to AVCC (5V), ADC0 as input and left adjusted
  ADMUX &= ~((1<<MUX0)|(1<<MUX1)|(1<<MUX2)|(1<<MUX3));
  ADMUX |= (1<<REFS0) | (1<<ADLAR);
  // Enable ADC, set prescaler to 128 for accuracy (16MHz / 128 = 125kHz)
  ADCSRA = (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
  // Enable ADC
  ADCSRA |= (1<<ADEN);

  // Set the backlight pin as output
  DDRD |= ((1 << _backlight_pin));

  display.begin();
}

Display::~Display() {}

void Display::begin() {
    touchscreen.begin();
    display.fillScreen(ILI9341_BLACK);
}

// 0 = Top, 1 = Plr1, 2 = Plr2, 3 = Mode1,
// 4 = Mode2, 5 = Normal, 6 = Fast,
// 7 = Start1, 8 = Start2, 9 = Waiting1,
// 10 = Waiting2, 11 =  HScore, 12 = Victory,
// 13 = Defeat, 14 = YourScore, 15 = OpponentScore,
// 16 = Menu, 17 = Play, 18 = Again
const char *menuRenderTextData[] = {
    "Snake",    "Player 1",    "Player 2",     "8 x 8",
    "16 x 16",  "Normal",      "FAST",         "Start",
    "Game!",    "Waiting for", "Player 1...",  "Highscore:",
    "Victory!", "Defeat!",     "Your Score: ", "Opponent's Score: ",
    "Menu",     "Play",        "Again!"};

void Display::copyArray(const uint16_t arrayOG[10], uint16_t newArray[10],
               uint8_t arraySize) {
  for (int i = 0; i < arraySize; i++) {
    newArray[i] = arrayOG[i];
  }
}

// Draw Death Screen
void Display::drawDeathScreen(bool isWinner, uint8_t lengthPlr1,
                              uint8_t lengthPlr2) {
  display.fillScreen(BLACK);
  if (isWinner) {
    drawElement(9, true, false, true, true);
  } else {
    drawElement(9, false, false, true, true);
  }

  drawElement(10, false, false, true, true);

  drawElement(11, false, false, true, true);
  drawElement(12, false, false, true, true);

  drawElement(8, false, false, true, true);
}
/*!
  @brief Draws an element with text, shadow and border.

  @param element The Index of the element: 0=Top; 1=Player1; 2=Player2; 3=Mode1;
  4=Mode2; 5=Mode3; 6=StartPlr1; 7=StartPlr2; 8=Highscore; 9=DScreenWinState;
  10=DScreenScore; 11=DScreenMenu; 12=DScreenPlayAgain
  @param selected Whether the drawn element has been selected by Touch.
  @param redrawBody Whether the body of the element should be drawn.
  @param isStartUp Whether the shadow of the element should be drawn.
*/
void Display::drawElement(uint8_t element, bool selected, bool isPlayer1,
                          bool redrawBody, bool isStartup) {
  // Temporary Storage for storing Element Data
  uint16_t tempStorageArray[10] = {};
  const char *elementText1 = "";
  const char *elementText2 = "";

  // Store Element Data In Temporary Array
  if (element == 0) { // if TopText
    copyArray(menuTopRenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[0];
  } else if (element == 1) { // if Player1
    copyArray(menuPlr1RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[1];
  } else if (element == 2) { // if Player2
    copyArray(menuPlr2RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[2];
    if (selected) {
      tempStorageArray[4] = menuPlr1SelectRenderData[0];
      tempStorageArray[5] = menuPlr1SelectRenderData[1];
      tempStorageArray[6] = menuPlr1SelectRenderData[2];
    }
  } else if (element == 3) { // if Mode1
    copyArray(menuMode1RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[3];
    if (selected) {
      tempStorageArray[5] = menuModeColor;
      tempStorageArray[6] = menuModeColor;
    }
  } else if (element == 4) { // if Mode2
    copyArray(menuMode2RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[4];
    if (selected) {
      tempStorageArray[5] = menuModeColor;
      tempStorageArray[6] = menuModeColor;
    }
  } else if (element == 5) { // if Mode3
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
  } else if (element == 6) { // if StartButtonPlayer1
    copyArray(menuStartPlr1RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[7];
    elementText2 = menuRenderTextData[8];
  } else if (element == 7) { // if StartButtonPlayer2
    copyArray(menuStartPlr2RenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[9];
    elementText2 = menuRenderTextData[10];
  } else if (element == 8) { // if Highscore
    copyArray(menuHScoreRenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[11];
  } else if (element == 9) { // if DScreen/Victory
    copyArray(dscreenWinStateRenderData, tempStorageArray, 10);
    if (selected) {
      elementText1 = menuRenderTextData[12];
    } else {
      tempStorageArray[4] = dscreenDefeatRenderData[0];
      tempStorageArray[5] = dscreenDefeatRenderData[1];
      tempStorageArray[6] = dscreenDefeatRenderData[2];
      elementText1 = menuRenderTextData[13];
    }
  } else if (element == 10) { // if DScreen/Score
    copyArray(dscreenScoreRenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[14];
    elementText2 = menuRenderTextData[15];
  } else if (element == 11) { // if DScreen/Menu
    copyArray(dscreenMenuRenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[16];
  } else if (element == 12) { // if DScreen/PlayAgain
    copyArray(dscreenPlayAgainRenderData, tempStorageArray, 10);
    elementText1 = menuRenderTextData[17];
    elementText2 = menuRenderTextData[18];
  }

  // Draw Element
  display.setTextSize(tempStorageArray[9]);
  display.setCursor(tempStorageArray[7] + tempStorageArray[0],
                   tempStorageArray[8] + tempStorageArray[1]); // Set Cursor
  display.setTextColor(tempStorageArray[6]);                    // Set Text Color
  if (isStartup) { // If this is on Startup
    display.fillRect(tempStorageArray[0] + 5, tempStorageArray[1] + 5,
                    tempStorageArray[2], tempStorageArray[3],
                    LGREY); // Draw Backdrop Rectangle
  }
  if (redrawBody) { // If this element should be redrawn
    display.fillRect(tempStorageArray[0], tempStorageArray[1],
                    tempStorageArray[2], tempStorageArray[3],
                    tempStorageArray[4]); // Draw Background Rectangle
  }
  display.drawRect(tempStorageArray[0] - 1, tempStorageArray[1] - 1,
                  tempStorageArray[2] + 2, tempStorageArray[3] + 2,
                  tempStorageArray[5]); // Draw Boundary Rectangle
  display.print(elementText1);           // Draw Text
  if (element == 8) {
    display.print(28);
  }
  if (element == 10) {
    display.print(20);
  }
  if (strlen(elementText2) > 0) { // if there is a second line of text
    if (isPlayer1) {              // if this is Player1
      display.setCursor(menuStartCursorLn2Data[0] + tempStorageArray[0],
                       menuStartCursorLn2Data[1] +
                           tempStorageArray[1]); // Set Cursor
      display.print(elementText2);                // Draw Line 2

    } else {
      display.setCursor(menuStartCursorLn2Data[2] + tempStorageArray[0],
                       menuStartCursorLn2Data[3] +
                           tempStorageArray[1]); // Set Cursor
      display.print(elementText2);                // Draw Line 2
    }
    if (element == 10) {
      display.print(10);
    }
  }
}

void Display::drawStartMenu() {
  display.fillScreen(BLACK);
  // Preset Text Size
  display.setTextSize(2);

  // Screen Border
  display.drawRect(0, 0, display.width(), display.height(), WHITE);
  display.drawRect(1, 1, display.width() - 2, display.height() - 2, MAGENTA);
  display.drawRect(2, 2, display.width() - 4, display.height() - 4, WHITE);

  // Draw Top Game Text
  drawElement(0, false, false, true, true);

  // // Draw Player 1 Button
  // drawElement(1, false, false, true, true);

  // // Draw Player 2 Text
  // drawElement(2, false, false, true, true);

  // Draw Mode 1 Button (8 x 8)
  drawElement(3, false, false, true, true);

  // Draw Mode 2 Button (16 x 16)
  drawElement(4, true, false, true, true);

  // Draw Mode 3 Button (Normal / Fast)
  drawElement(5, false, false, true, true);

  // TEMPORARY
  // Draw Start Game Button
  drawElement(6, false, true, true, true);

  // Draw Highscore Text
  drawElement(8, false, false, true, true);
}

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

void Display::refreshBacklight() {
  if(!(ADCSRA & (1<<ADSC))){
    OCR0B = ADCH / 2; // zorgen dat die binnen 209 blijft

    ADCSRA |= (1<<ADSC); // volgende conversie
  }
}

TS_Point Display::getPoint() {
  return touchscreen.getPoint();
}

uint8_t Display::width()
{
    return 240;
}

uint16_t Display::height()
{
    return 320;
}
