#include "reader.h"

#include <FS.h>
#include <LittleFS.h>

#include <cstdlib>

#include "params.h"
#include "storage.h"

static File g_bookFile;
static char g_bookFilename[MAX_FILENAME_LENGTH];
static char* g_blockBuffer = nullptr;
static size_t g_blockSize = 0;
static size_t g_blockPos = 0;
static size_t g_blockLen = 0;
static bool g_readerReady = false;
static char g_word[MAX_WORD_LEN + 1];

static bool isWhitespace(char c) {
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

static bool reopenBookFromStart() {
  if (g_bookFile) {
    g_bookFile.close();
  }

  g_bookFile = LittleFS.open(g_bookFilename, "r");
  if (!g_bookFile) {
    return false;
  }

  g_blockPos = 0;
  g_blockLen = 0;
  return true;
}

static bool fillBlock() {
  if (!g_bookFile) {
    return false;
  }

  g_blockLen = g_bookFile.readBytes(g_blockBuffer, g_blockSize);
  g_blockPos = 0;
  return g_blockLen > 0;
}

static bool readByte(char* out) {
  if (out == nullptr || !g_readerReady) {
    return false;
  }

  if (g_blockPos >= g_blockLen) {
    if (!fillBlock()) {
      return false;
    }
  }

  *out = g_blockBuffer[g_blockPos++];
  return true;
}

static uint16_t computePause(const char* token, size_t len) {
  bool clausePause = false;
  bool sentencePause = false;

  for (size_t i = 0; i < len; i++) {
    const char c = token[i];
    if (c == '.' || c == '!' || c == '?') {
      sentencePause = true;
    }
    if (c == ',' || c == ';' || c == ':') {
      clausePause = true;
    }
  }

  if (sentencePause) {
    return SENTENCE_DELAY_MS;
  }
  if (clausePause) {
    return CLAUSE_DELAY_MS;
  }
  return WORD_DELAY_MS;
}

const char* readerNextWord(uint16_t* pauseMs) {
  if (!g_readerReady || pauseMs == nullptr) {
    return nullptr;
  }

  for (uint8_t attempt = 0; attempt < 2; attempt++) {
    size_t len = 0;
    char c = '\0';
    bool hasChar = false;

    while (readByte(&c)) {
      if (!isWhitespace(c)) {
        hasChar = true;
        break;
      }
    }

    if (!hasChar) {
      if (attempt == 0 && reopenBookFromStart()) {
        continue;
      }
      return nullptr;
    }

    do {
      if (len < MAX_WORD_LEN) {
        g_word[len++] = c;
      }
    } while (readByte(&c) && !isWhitespace(c));

    g_word[len] = '\0';
    *pauseMs = computePause(g_word, len);
    return g_word;
  }

  return nullptr;
}

bool readerInit(const char* filename, size_t blockSize) {
  if (filename == nullptr || blockSize == 0) {
    return false;
  }

  if (g_blockBuffer != nullptr) {
    free(g_blockBuffer);
    g_blockBuffer = nullptr;
  }

  g_blockBuffer = (char*)malloc(blockSize);
  if (g_blockBuffer == nullptr) {
    Serial.println("Failed to allocate reader buffer");
    return false;
  }

  strncpy(g_bookFilename, filename, sizeof(g_bookFilename) - 1);
  g_bookFilename[sizeof(g_bookFilename) - 1] = '\0';

  g_blockSize = blockSize;
  g_readerReady = reopenBookFromStart();
  if (!g_readerReady) {
    Serial.println("Failed to open selected book");
    free(g_blockBuffer);
    g_blockBuffer = nullptr;
    g_blockSize = 0;
    return false;
  }

  return true;
}
