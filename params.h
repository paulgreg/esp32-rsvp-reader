#pragma once

#include <Arduino.h>

static const uint16_t WORD_DELAY_MS = 250;
static const uint16_t CLAUSE_DELAY_MS = 300;
static const uint16_t SENTENCE_DELAY_MS = 600;

static const size_t BOOK_BLOCK_SIZE = 4096;
static const size_t MAX_WORD_LEN = 48;

static const int16_t ORP_OFFSET_FROM_CENTER_PX = -50;
static const int16_t ORP_MARKER_WIDTH_PX = 3;
static const int16_t ORP_MARKER_HEIGHT_PX = 28;
static const int16_t ORP_MARKER_RIGHT_GAP_PX = 10;
static const int16_t ORP_MARKER_TOP_GAP_PX = 14;
static const int16_t ORP_MARKER_BOTTOM_GAP_PX = -6;
