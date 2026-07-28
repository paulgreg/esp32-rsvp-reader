#pragma once

#include <Arduino.h>

bool progressSave(const char* filename, uint8_t sentenceHead,
                  uint8_t sentenceCount, const size_t* sentencePos,
                  uint8_t sentencePosLen);
bool progressLoad(const char* filename, uint8_t* sentenceHead,
                  uint8_t* sentenceCount, size_t* sentencePos,
                  uint8_t sentencePosLen);
bool progressClear();
