#include <Adafruit_GFX.h>       // include Adafruit graphics library
#include <Adafruit_ILI9341.h>   // include Adafruit ILI9341 TFT library
 
#define TFT_CS    5            // TFT CS
#define TFT_RST   0            // TFT RST
#define TFT_DC    26           // TFT DC
Adafruit_ILI9341  tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// https://ee-programming-notepad.blogspot.com/2016/10/16-bit-color-generator-picker.html
#define BACKGND     ILI9341_BLACK 
#define COLOR_TEXT  ILI9341_WHITE

#include "Fonts/Cantarell_Bold_euro18pt8b.h"

#define SCREEN_HORIZONTAL 1
#define SCREEN_HORIZONTAL_INVERSE 3

#define SCREEN_ORIENTATION SCREEN_HORIZONTAL

// https://github.com/adafruit/Adafruit_ILI9341/blob/master/Adafruit_ILI9341.h
// https://github.com/adafruit/Adafruit-GFX-Library/blob/master/Adafruit_GFX.cpp

void screenDiagnostics() {
  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("\nDisplay Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 
}

void fillScreen(int color) {
  tft.fillScreen(color);
}

void setupScreen() {
  // tft.begin();
  tft.begin(3000000);
  // screenDiagnostics();
  tft.setRotation(SCREEN_ORIENTATION);
  fillScreen(ILI9341_WHITE);
}

void _text(int x, int y, int color, const char* text) {
  tft.setTextSize(1);
  tft.setTextColor(color);
  tft.setCursor(x, y);
  tft.println(text);
}

void drawSmallText(int x, int y, int color, const char* text) {
  tft.setFont(&Cantarell_Bold_euro18pt8b);
  _text(x, y, color, text);
}

void drawBigText(int x, int y, int color, const char* text) {
  tft.setFont(&Cantarell_Bold_euro18pt8b);
  _text(x, y, color, text);
}

void drawText(int x, int y, int color, const char* text) {
  tft.setFont(&Cantarell_Bold_euro18pt8b);
  _text(x, y, color, text);
}

void printMsg(const char* text) {
  fillScreen(BACKGND);
  drawText(5, 50, COLOR_TEXT, text);
}

void printError(const char* text) {
  fillScreen(ILI9341_WHITE);
  drawText(5, 50, ILI9341_RED, text);
}

void displayWord(const char* word) {
  int16_t x1;
  int16_t y1;
  uint16_t w;
  uint16_t h;

  fillScreen(BACKGND);
  tft.setFont(&Cantarell_Bold_euro18pt8b);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.getTextBounds(word, 0, 0, &x1, &y1, &w, &h);
  (void)y1;
  (void)h;

  const int16_t x = ((int16_t)tft.width() - (int16_t)w) / 2 - x1;
  const int16_t baselineY = (int16_t)tft.height() / 2 + 9;

  tft.setCursor(x, baselineY);
  tft.print(word);
}
