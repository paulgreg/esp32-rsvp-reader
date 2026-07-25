#include "storage.h"

#include <LittleFS.h>

#include <cstring>

static bool g_storageInitialized = false;

bool initStorage() {
  if (g_storageInitialized) {
    return true;
  }

  if (!LittleFS.begin(true)) {
    Serial.println("Failed to mount LittleFS");
    return false;
  }

  g_storageInitialized = true;
  Serial.println("LittleFS ready");
  return true;
}

int listAvailableBooks(BookInfo* books, int maxBooks) {
  if (books == nullptr || maxBooks <= 0) {
    return 0;
  }

  if (!g_storageInitialized && !initStorage()) {
    return 0;
  }

  int count = 0;
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open root directory");
    return 0;
  }

  File file = root.openNextFile();
  while (file && count < maxBooks) {
    if (!file.isDirectory()) {
      const char* rawName = file.name();
      const size_t len = strlen(rawName);
      const bool isTxt = (len > 4 && strcmp(rawName + len - 4, ".txt") == 0);

      if (isTxt) {
        if (rawName[0] == '/') {
          strncpy(books[count].filename, rawName, MAX_FILENAME_LENGTH - 1);
        } else {
          snprintf(books[count].filename, MAX_FILENAME_LENGTH, "/%s", rawName);
        }
        books[count].filename[MAX_FILENAME_LENGTH - 1] = '\0';

        const char* slash = strrchr(books[count].filename, '/');
        const char* title = (slash == nullptr) ? books[count].filename : slash + 1;
        strncpy(books[count].title, title, MAX_BOOK_TITLE_LENGTH - 1);
        books[count].title[MAX_BOOK_TITLE_LENGTH - 1] = '\0';
        count++;
      }
    }
    file = root.openNextFile();
  }

  root.close();
  return count;
}

bool isStorageAvailable() {
  return g_storageInitialized;
}
