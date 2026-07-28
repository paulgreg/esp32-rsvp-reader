#pragma once

#include <Adafruit_ILI9341.h>

static constexpr uint16_t BACKGND = ILI9341_BLACK;

void screenDiagnostics();
void fillScreen(int color);
void setupScreen();
void printMsg(const char* text);
void printError(const char* text);
int16_t getOrpColumnX();
void drawOrpMarkers();
void drawStatusIcon(bool playing);
void clearRewindIcon();
void drawRewindIcon();
void displayWord(const char* word);
void displayEnd();
