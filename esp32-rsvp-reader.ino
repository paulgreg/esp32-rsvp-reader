#include "display.h"
#include "params.h"
#include "reader.h"
#include "storage.h"

static bool g_ready = false;
static bool g_play = false;
static uint32_t g_nextWordAtMs = 0;

static int g_playButtonLastReading = 1;
static int g_playButtonStableState = 1;
static uint32_t g_playButtonLastChangeMs = 0;

static int g_rewindButtonLastReading = 1;
static int g_rewindButtonStableState = 1;
static uint32_t g_rewindButtonLastChangeMs = 0;
static uint32_t g_rewindPressStartMs = 0;
static bool g_rewindLongPressTriggered = false;
static uint32_t g_rewindIconClearAtMs = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\nRSVP reader");

  pinMode(BUTTON_PLAY_PIN, INPUT_PULLUP);
  pinMode(BUTTON_REWIND_PIN, INPUT_PULLUP);

  setupScreen();
  printMsg("Init LittleFS...");

  if (!initStorage()) {
    printError("LittleFS init failed");
    return;
  }

  BookInfo books[MAX_BOOKS];
  const int bookCount = listAvailableBooks(books, MAX_BOOKS);
  if (bookCount <= 0) {
    printError("No .txt book found");
    return;
  }

  Serial.print("Selected book: ");
  Serial.println(books[0].filename);

  if (!readerInit(books[0].filename, BOOK_BLOCK_SIZE)) {
    printError("Book open failed");
    return;
  }

  printMsg(books[0].title);
  delay(1000);

  fillScreen(BACKGND);

  drawOrpMarkers();
  displayWord("Start ?");
  drawStatusIcon(g_play);

  g_ready = true;
}

void loop() {
  if (!g_ready) {
    delay(100);
    return;
  }

  const uint32_t nowMs = millis();

  const int playButtonReading = gpio_get_level(BUTTON_PLAY_PIN);
  if (playButtonReading != g_playButtonLastReading) {
    g_playButtonLastChangeMs = nowMs;
    g_playButtonLastReading = playButtonReading;
  }

  if (nowMs - g_playButtonLastChangeMs >= BUTTON_DEBOUNCE_DELAY_MS &&
      playButtonReading != g_playButtonStableState) {
    g_playButtonStableState = playButtonReading;
    if (g_playButtonStableState == 0) {
      g_play = !g_play;
      if (g_play) {
        g_nextWordAtMs = nowMs;
      }
      drawStatusIcon(g_play);
      Serial.printf("play: %s\n", g_play ? "on" : "off");
    }
  }

  const int rewindButtonReading = gpio_get_level(BUTTON_REWIND_PIN);
  if (rewindButtonReading != g_rewindButtonLastReading) {
    g_rewindButtonLastChangeMs = nowMs;
    g_rewindButtonLastReading = rewindButtonReading;
  }

  if (nowMs - g_rewindButtonLastChangeMs >= BUTTON_DEBOUNCE_DELAY_MS &&
      rewindButtonReading != g_rewindButtonStableState) {
    const int previousStableState = g_rewindButtonStableState;
    g_rewindButtonStableState = rewindButtonReading;

    if (previousStableState == 1 && g_rewindButtonStableState == 0) {
      g_rewindPressStartMs = nowMs;
      g_rewindLongPressTriggered = false;
    } else if (previousStableState == 0 && g_rewindButtonStableState == 1 &&
               !g_rewindLongPressTriggered) {
      if (readerRewindSentence()) {
        uint16_t pauseMs = SENTENCE_DELAY_MS;
        const char* firstWord = readerNextWord(&pauseMs);
        if (firstWord != nullptr) {
          displayWord(firstWord);
        }
        g_nextWordAtMs = nowMs + SENTENCE_DELAY_MS;
        drawRewindIcon();
        g_rewindIconClearAtMs = nowMs + REWIND_ICON_DISPLAY_MS;
        Serial.println("rewind: sentence");
      } else {
        Serial.println("rewind: failed");
      }
    }
  }

  if (g_rewindButtonStableState == 0 && !g_rewindLongPressTriggered &&
      nowMs - g_rewindPressStartMs >= BUTTON_REWIND_LONG_PRESS_MS) {
    g_rewindLongPressTriggered = true;
    g_play = false;
    if (readerRestart()) {
      uint16_t pauseMs = SENTENCE_DELAY_MS;
      const char* firstWord = readerNextWord(&pauseMs);
      if (firstWord != nullptr) {
        displayWord(firstWord);
      }
      g_nextWordAtMs = nowMs + SENTENCE_DELAY_MS;
      drawRewindIcon();
      g_rewindIconClearAtMs = nowMs + REWIND_ICON_DISPLAY_MS;
      Serial.println("rewind: restart");
    } else {
      Serial.println("restart: failed");
    }
    drawStatusIcon(g_play);
  }

  if (g_rewindIconClearAtMs != 0 && (int32_t)(nowMs - g_rewindIconClearAtMs) >= 0) {
    clearRewindIcon();
    g_rewindIconClearAtMs = 0;
  }

  if (g_play) {
    if ((int32_t)(nowMs - g_nextWordAtMs) < 0) {
      delay(1);
      return;
    }

    uint16_t pauseMs = WORD_DELAY_MS;
    const char* word = readerNextWord(&pauseMs);

    if (word == nullptr) {
      g_nextWordAtMs = nowMs + WORD_DELAY_MS;
      delay(1);
      return;
    }

    displayWord(word);
    g_nextWordAtMs = nowMs + pauseMs;
  } else {
    delay(1);
  }
}
