#pragma once

#include <Arduino.h>

static const uint16_t WORD_DELAY_MS = 200;
static const uint16_t CLAUSE_DELAY_MS = 300;
static const uint16_t SENTENCE_DELAY_MS = 600;
static const uint8_t SENTENCE_HISTORY_SIZE = 25;

static const size_t BOOK_BLOCK_SIZE = 4096;
static const size_t MAX_WORD_LEN = 48;

static const int16_t ORP_OFFSET_FROM_CENTER_PX = -70;
static const int16_t ORP_MARKER_WIDTH_PX = 3;
static const int16_t ORP_MARKER_HEIGHT_PX = 28;
static const int16_t ORP_MARKER_RIGHT_GAP_PX = 10;
static const int16_t ORP_MARKER_TOP_GAP_PX = 14;
static const int16_t ORP_MARKER_BOTTOM_GAP_PX = -6;

static const int16_t STATUS_ICON_SIZE_PX = 14;
static const int16_t STATUS_ICON_MARGIN_PX = 6;


#define BUTTON_PLAY_PIN    GPIO_NUM_27
#define BUTTON_REWIND_PIN  GPIO_NUM_32

#define BUTTON_DEBOUNCE_DELAY_MS  100
static const uint32_t BUTTON_REWIND_LONG_PRESS_MS = 1000;
static const uint32_t REWIND_ICON_DISPLAY_MS = 600;

