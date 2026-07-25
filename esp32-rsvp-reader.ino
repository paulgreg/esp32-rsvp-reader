#include "display.h"
#include "params.h"
#include "reader.h"
#include "storage.h"

static bool g_ready = false;

void setup() {
  Serial.begin(115200);
  Serial.println("\nRSVP reader");

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
  delay(500);
  g_ready = true;
}

void loop() {
  if (!g_ready) {
    delay(WORD_DELAY_MS);
    return;
  }

  uint16_t pauseMs = WORD_DELAY_MS;
  const char* word = readerNextWord(&pauseMs);

  if (word == nullptr) {
    delay(WORD_DELAY_MS);
    return;
  }

  displayWord(word);
  delay(pauseMs);

}
