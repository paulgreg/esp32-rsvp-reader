#pragma once

#include <Arduino.h>

#define MAX_BOOKS 10
#define MAX_FILENAME_LENGTH 64
#define MAX_BOOK_TITLE_LENGTH 64

struct BookInfo {
  char filename[MAX_FILENAME_LENGTH];
  char title[MAX_BOOK_TITLE_LENGTH];
};

bool initStorage();
int listAvailableBooks(BookInfo* books, int maxBooks);
bool isStorageAvailable();
