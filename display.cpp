#include "display.h"

#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <cstring>

#include "Fonts/Cantarell_Bold_euro18pt8b.h"
#include "params.h"

#define TFT_CS 5
#define TFT_RST 0
#define TFT_DC 26

#define COLOR_TEXT ILI9341_WHITE
#define COLOR_ORP ILI9341_RED

#define SCREEN_ORIENTATION 1

static Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

static void drawText(int x, int y, int color, const char* text) {
  tft.setFont(&Cantarell_Bold_euro18pt8b);
  tft.setTextSize(1);
  tft.setTextColor(color);
  tft.setCursor(x, y);
  tft.println(text);
}

void screenDiagnostics() {
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("\nDisplay Power Mode: 0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x");
  Serial.println(x, HEX);
}

void fillScreen(int color) {
  tft.fillScreen(color);
}

void setupScreen() {
  tft.begin(3000000);
  tft.setRotation(SCREEN_ORIENTATION);
  fillScreen(ILI9341_WHITE);
}

void printMsg(const char* text) {
  fillScreen(BACKGND);
  drawText(5, 50, COLOR_TEXT, text);
}

void printError(const char* text) {
  fillScreen(ILI9341_WHITE);
  drawText(5, 50, ILI9341_RED, text);
}

int16_t getOrpColumnX() {
  return ((int16_t)tft.width() / 2) + ORP_OFFSET_FROM_CENTER_PX;
}

void drawOrpMarkers() {
  const int16_t orpX = getOrpColumnX();
  const int16_t baselineY = (int16_t)tft.height() / 2 + 9;

  const int16_t topY = baselineY - (int16_t)Cantarell_Bold_euro18pt8b.yAdvance / 2 -
                       ORP_MARKER_TOP_GAP_PX - ORP_MARKER_HEIGHT_PX;
  const int16_t bottomY = baselineY + (int16_t)Cantarell_Bold_euro18pt8b.yAdvance / 2 +
                          ORP_MARKER_BOTTOM_GAP_PX;
  const int16_t markerX = orpX - (ORP_MARKER_WIDTH_PX / 2) + ORP_MARKER_RIGHT_GAP_PX;

  tft.fillRect(markerX, topY, ORP_MARKER_WIDTH_PX, ORP_MARKER_HEIGHT_PX, COLOR_ORP);
  tft.fillRect(markerX, bottomY, ORP_MARKER_WIDTH_PX, ORP_MARKER_HEIGHT_PX, COLOR_ORP);
}

void drawStatusIcon(bool playing) {
  const int16_t size = STATUS_ICON_SIZE_PX;
  const int16_t margin = STATUS_ICON_MARGIN_PX;
  const int16_t x = (int16_t)tft.width() - margin - size;
  const int16_t y = margin;

  tft.fillRect(x - 1, y - 1, size + 2, size + 2, BACKGND);

  if (!playing) {
    const int16_t barWidth = (size >= 8) ? (size / 4) : 2;
    const int16_t gap = (size >= 8) ? (size / 4) : 2;
    tft.fillRect(x, y, barWidth, size, COLOR_TEXT);
    tft.fillRect(x + barWidth + gap, y, barWidth, size, COLOR_TEXT);
  } else {
    tft.fillTriangle(x, y, x, y + size, x + size, y + (size / 2), COLOR_TEXT);
  }
}

void clearRewindIcon() {
  const int16_t size = STATUS_ICON_SIZE_PX;
  const int16_t margin = STATUS_ICON_MARGIN_PX;
  const int16_t x = (int16_t)tft.width() - (2 * margin) - (2 * size);
  const int16_t y = margin;

  tft.fillRect(x - 1, y - 1, size + 2, size + 2, BACKGND);
}

void drawRewindIcon() {
  const int16_t size = STATUS_ICON_SIZE_PX;
  const int16_t margin = STATUS_ICON_MARGIN_PX;
  const int16_t x = (int16_t)tft.width() - (2 * margin) - (2 * size);
  const int16_t y = margin;
  const int16_t halfH = size / 2;
  const int16_t triW = (size >= 8) ? (size / 2) : 4;
  const int16_t gap = (size >= 8) ? (size / 6) : 1;

  clearRewindIcon();

  const int16_t x1Right = x + triW;
  const int16_t x2Right = x + size;
  const int16_t x2Left = x2Right - triW;

  tft.fillTriangle(x, y + halfH, x1Right, y, x1Right, y + size, COLOR_TEXT);
  tft.fillTriangle(x2Left - gap, y + halfH, x2Right - gap, y, x2Right - gap, y + size,
                   COLOR_TEXT);
}

void displayWord(const char* word) {
  static int16_t prevX = 0;
  static int16_t prevY = 0;
  static uint16_t prevW = 0;
  static uint16_t prevH = 0;

  if (word == nullptr || word[0] == '\0') {
    return;
  }

  const size_t len = strlen(word);
  size_t orpIndex = 0;
  if (len <= 3) {
    orpIndex = 0;
  } else if (len <= 5) {
    orpIndex = 1;
  } else if (len <= 9) {
    orpIndex = 2;
  } else {
    orpIndex = 3;
  }
  if (orpIndex >= len) {
    orpIndex = len - 1;
  }

  const GFXfont* font = &Cantarell_Bold_euro18pt8b;
  int16_t leftAdvance = 0;
  for (size_t i = 0; i < orpIndex; i++) {
    const uint8_t c = (uint8_t)word[i];
    if (c < font->first || c > font->last) {
      continue;
    }
    const GFXglyph* glyph = font->glyph + (c - font->first);
    leftAdvance += glyph->xAdvance;
  }

  char leftPart[MAX_WORD_LEN + 1];
  char pivotPart[2];
  char rightPart[MAX_WORD_LEN + 1];

  if (orpIndex > MAX_WORD_LEN) {
    return;
  }

  memcpy(leftPart, word, orpIndex);
  leftPart[orpIndex] = '\0';
  pivotPart[0] = word[orpIndex];
  pivotPart[1] = '\0';

  const size_t rightLen = (len > (orpIndex + 1)) ? (len - (orpIndex + 1)) : 0;
  if (rightLen > MAX_WORD_LEN) {
    return;
  }
  memcpy(rightPart, word + orpIndex + 1, rightLen);
  rightPart[rightLen] = '\0';

  tft.setFont(font);
  tft.setTextSize(1);
  const int16_t orpX = getOrpColumnX();
  const int16_t x = orpX - leftAdvance;
  const int16_t baselineY = (int16_t)tft.height() / 2 + 9;

  int16_t curX1 = 0;
  int16_t curY1 = 0;
  uint16_t curW = 0;
  uint16_t curH = 0;
  tft.getTextBounds(word, x, baselineY, &curX1, &curY1, &curW, &curH);

  if (prevW > 0 && prevH > 0) {
    tft.fillRect(prevX, prevY, prevW, prevH, BACKGND);
  }

  tft.setCursor(x, baselineY);
  tft.setTextColor(COLOR_TEXT);
  tft.print(leftPart);
  tft.setTextColor(COLOR_ORP);
  tft.print(pivotPart);
  tft.setTextColor(COLOR_TEXT);
  tft.print(rightPart);

  if (curW > 0 && curH > 0) {
    const int16_t margin = 2;
    int16_t nx = curX1 - margin;
    int16_t ny = curY1 - margin;
    int32_t nw = (int32_t)curW + 2 * margin;
    int32_t nh = (int32_t)curH + 2 * margin;

    if (nx < 0) {
      nw += nx;
      nx = 0;
    }
    if (ny < 0) {
      nh += ny;
      ny = 0;
    }

    const int16_t maxW = tft.width();
    const int16_t maxH = tft.height();
    if (nx + nw > maxW) {
      nw = maxW - nx;
    }
    if (ny + nh > maxH) {
      nh = maxH - ny;
    }

    if (nw > 0 && nh > 0) {
      prevX = nx;
      prevY = ny;
      prevW = (uint16_t)nw;
      prevH = (uint16_t)nh;
    }
  }

  drawOrpMarkers();
}

void displayEnd() {
  displayWord("END");
}
