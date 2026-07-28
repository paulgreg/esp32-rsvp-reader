#include "reader.h"

#include <FS.h>
#include <LittleFS.h>

#include <cstring>
#include <cstdlib>

#include "params.h"
#include "progress.h"
#include "storage.h"

static File g_bookFile;
static char g_bookFilename[MAX_FILENAME_LENGTH];
static char* g_blockBuffer = nullptr;
static size_t g_blockSize = 0;
static size_t g_blockPos = 0;
static size_t g_blockLen = 0;
static bool g_readerReady = false;
static char g_word[MAX_WORD_LEN + 1];
static size_t g_sentencePos[SENTENCE_HISTORY_SIZE];
static uint8_t g_sentenceHead = SENTENCE_HISTORY_SIZE - 1;
static uint8_t g_sentenceCount = 0;

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

static size_t currentConsumedPosition() {
  if (!g_bookFile) {
    return 0;
  }

  const size_t filePos = (size_t)g_bookFile.position();
  const size_t unreadInBlock = g_blockLen - g_blockPos;
  return (filePos >= unreadInBlock) ? (filePos - unreadInBlock) : 0;
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

static void recordSentenceBoundary(size_t pos) {
  g_sentenceHead = (uint8_t)((g_sentenceHead + 1) % SENTENCE_HISTORY_SIZE);
  g_sentencePos[g_sentenceHead] = pos;
  if (g_sentenceCount < SENTENCE_HISTORY_SIZE) {
    g_sentenceCount++;
  }
}

static bool hasSentenceEnding(const char* token, size_t len) {
  for (size_t i = 0; i < len; i++) {
    const char c = token[i];
    if (c == '.' || c == '!' || c == '?') {
      return true;
    }
  }
  return false;
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
    if (hasSentenceEnding(g_word, len)) {
      recordSentenceBoundary(currentConsumedPosition());
    }
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
  g_sentenceHead = SENTENCE_HISTORY_SIZE - 1;
  g_sentenceCount = 0;
  g_readerReady = reopenBookFromStart();
  if (!g_readerReady) {
    Serial.println("Failed to open selected book");
    free(g_blockBuffer);
    g_blockBuffer = nullptr;
    g_blockSize = 0;
    return false;
  }

  uint8_t sentenceHead = 0;
  uint8_t sentenceCount = 0;
  size_t sentencePos[SENTENCE_HISTORY_SIZE] = {0};
  if (progressLoad(g_bookFilename, &sentenceHead, &sentenceCount, sentencePos,
                   SENTENCE_HISTORY_SIZE)) {
    g_sentenceHead = sentenceHead;
    g_sentenceCount = sentenceCount;
    memcpy(g_sentencePos, sentencePos, sizeof(g_sentencePos));

    size_t resumePos = 0;
    if (g_sentenceCount > 0) {
      resumePos = g_sentencePos[g_sentenceHead];
    }

    if (g_bookFile.seek((size_t)resumePos, SeekSet)) {
      g_blockPos = 0;
      g_blockLen = 0;
      Serial.printf("Progress restored at %u\n", (unsigned)resumePos);
    } else {
      Serial.println("Progress restore seek failed; starting at beginning");
    }
  }

  return true;
}

bool readerRestart() {
  if (!g_readerReady) {
    return false;
  }

  progressClear();
  g_sentenceHead = SENTENCE_HISTORY_SIZE - 1;
  g_sentenceCount = 0;
  return reopenBookFromStart();
}

bool readerRewindSentence() {
  if (!g_readerReady) {
    return false;
  }

  if (g_sentenceCount == 0) {
    return readerRestart();
  }

  const size_t targetPos = g_sentencePos[g_sentenceHead];
  g_sentenceHead = (uint8_t)((g_sentenceHead + SENTENCE_HISTORY_SIZE - 1) % SENTENCE_HISTORY_SIZE);
  g_sentenceCount--;

  if (!g_bookFile.seek(targetPos, SeekSet)) {
    return false;
  }

  g_blockPos = 0;
  g_blockLen = 0;
  return true;
}

bool readerSaveProgress() {
  if (!g_readerReady) {
    return false;
  }

  return progressSave(g_bookFilename, g_sentenceHead, g_sentenceCount,
                      g_sentencePos, SENTENCE_HISTORY_SIZE);
}

bool readerClearProgress() {
  return progressClear();
}
