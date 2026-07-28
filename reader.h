#pragma once

#include <Arduino.h>

bool readerInit(const char* filename, size_t blockSize);
const char* readerNextWord(uint16_t* pauseMs);
bool readerIsAtEnd();
bool readerRestart();
bool readerRewindSentence();
bool readerSaveProgress();
bool readerClearProgress();
