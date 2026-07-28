#include "display.h"
#include "params.h"
#include "reader.h"
#include "storage.h"

enum class AppState {
  MENU,
  READING,
  END,
};

static bool g_ready = false;
static AppState g_state = AppState::MENU;
static bool g_play = false;
static uint32_t g_nextWordAtMs = 0;

static BookInfo g_books[MAX_BOOKS];
static int g_bookCount = 0;
static int g_selectedBook = 0;

static int g_playButtonLastReading = 1;
static int g_playButtonStableState = 1;
static uint32_t g_playButtonLastChangeMs = 0;

static int g_rewindButtonLastReading = 1;
static int g_rewindButtonStableState = 1;
static uint32_t g_rewindButtonLastChangeMs = 0;
static uint32_t g_rewindPressStartMs = 0;
static bool g_rewindLongPressTriggered = false;
static uint32_t g_rewindIconClearAtMs = 0;

static void showBookMenu() {
  displayMenu(g_books[g_selectedBook].title, g_selectedBook, g_bookCount);
}

static bool openSelectedBook(uint32_t nowMs) {
  Serial.print("Selected book: ");
  Serial.println(g_books[g_selectedBook].filename);

  if (!readerInit(g_books[g_selectedBook].filename, BOOK_BLOCK_SIZE)) {
    printError("Book open failed");
    return false;
  }

  g_play = false;
  g_nextWordAtMs = nowMs;
  g_rewindLongPressTriggered = false;
  g_rewindIconClearAtMs = 0;

  fillScreen(BACKGND);
  drawOrpMarkers();

  uint16_t pauseMs = WORD_DELAY_MS;
  const char* firstWord = readerNextWord(&pauseMs);
  if (firstWord != nullptr) {
    displayWord(firstWord);
    g_state = AppState::READING;
  } else if (readerIsAtEnd()) {
    displayEnd();
    g_state = AppState::END;
  } else {
    g_state = AppState::READING;
  }

  drawStatusIcon(g_play);
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nRSVP reader");

  pinMode(BUTTON_PLAY_PIN, INPUT_PULLUP);
  pinMode(BUTTON_REWIND_PIN, INPUT_PULLUP);

  setupScreen();
  // screenDiagnostics();

  printMsg("RSVP reader");

  if (!initStorage()) {
    printError("LittleFS init failed");
    return;
  }

  g_bookCount = listAvailableBooks(g_books, MAX_BOOKS);
  if (g_bookCount <= 0) {
    printError("No .txt book found");
    return;
  }

  g_selectedBook = 0;
  g_state = AppState::MENU;
  showBookMenu();

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
      if (g_state == AppState::MENU) {
        if (openSelectedBook(nowMs)) {
          Serial.println("menu: book opened");
        }
        return;
      }

      if (g_state == AppState::END) {
        if (readerRestart()) {
          uint16_t pauseMs = WORD_DELAY_MS;
          const char* firstWord = readerNextWord(&pauseMs);
          if (firstWord != nullptr) {
            displayWord(firstWord);
          }
          g_state = AppState::READING;
          g_play = true;
          g_nextWordAtMs = nowMs + pauseMs;
          drawStatusIcon(g_play);
          Serial.println("restart: from end");
        } else {
          Serial.println("restart: failed");
        }
        return;
      }

      g_play = !g_play;
      if (g_play) {
        g_nextWordAtMs = nowMs;
      } else {
        if (readerSaveProgress()) {
          Serial.println("progress: saved");
        } else {
          Serial.println("progress: save failed");
        }
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
      if (g_state == AppState::MENU) {
        if (g_bookCount > 0) {
          g_selectedBook = (g_selectedBook + 1) % g_bookCount;
          showBookMenu();
          Serial.printf("menu: selected %d\n", g_selectedBook + 1);
        }
        return;
      }

      g_rewindPressStartMs = nowMs;
      g_rewindLongPressTriggered = false;
    } else if (previousStableState == 0 && g_rewindButtonStableState == 1 &&
               !g_rewindLongPressTriggered) {
      if (g_state != AppState::READING && g_state != AppState::END) {
        return;
      }

      if (readerRewindSentence()) {
        uint16_t pauseMs = SENTENCE_DELAY_MS;
        const char* firstWord = readerNextWord(&pauseMs);
        if (firstWord != nullptr) {
          displayWord(firstWord);
          g_state = AppState::READING;
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

  if ((g_state == AppState::READING || g_state == AppState::END) &&
      g_rewindButtonStableState == 0 && !g_rewindLongPressTriggered &&
      nowMs - g_rewindPressStartMs >= BUTTON_REWIND_LONG_PRESS_MS) {
    g_rewindLongPressTriggered = true;
    g_play = false;
    if (readerRestart()) {
      uint16_t pauseMs = SENTENCE_DELAY_MS;
      const char* firstWord = readerNextWord(&pauseMs);
      if (firstWord != nullptr) {
        displayWord(firstWord);
        g_state = AppState::READING;
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

  if (g_state != AppState::READING || !g_play) {
    delay(1);
    return;
  }

  if ((int32_t)(nowMs - g_nextWordAtMs) < 0) {
    delay(1);
    return;
  }

  uint16_t pauseMs = WORD_DELAY_MS;
  const char* word = readerNextWord(&pauseMs);

  if (word == nullptr) {
    if (readerIsAtEnd()) {
      g_state = AppState::END;
      g_play = false;
      displayEnd();
      drawStatusIcon(g_play);
      Serial.println("book: end reached");
    }
    g_nextWordAtMs = nowMs + WORD_DELAY_MS;
    delay(1);
    return;
  }

  displayWord(word);
  g_nextWordAtMs = nowMs + pauseMs;
}
