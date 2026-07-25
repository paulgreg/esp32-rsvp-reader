# RSVP Reader (ESP32 + TFT)

A simple RSVP ebook reader on ESP32 with an ILI9341 TFT screen.

## Hardware

- ESP32
- ILI9341 TFT (SPI)
- Pins (from `display.h`): `CS=5`, `RST=0`, `DC=26`

## Features

- Reads `.txt` books from LittleFS (`data/` uploaded to board)
- Block-based file reading (`BOOK_BLOCK_SIZE`, default `4096` bytes)
- RSVP word display with ORP alignment (not simple centering)
- ORP pivot letter highlighted in red
- Fixed ORP markers (top/bottom ticks)
- Faster rendering by clearing only previous word bounds (reduced flicker)

## Convert EPUB/TXT to Latin-1 TXT

Scripts are in `scripts/`.

```bash
cd scripts
npm install
node convert.js "../my-book.epub" "../data/my-book.txt"
```

You can also convert a plain `.txt` input.

Notes:
- Output is written in `latin1` encoding (important for French accents)
- The converter adds a metadata header (`TITLE=`, `AUTHOR=`, `---`) at file start

## Upload books with Arduino IDE

1. Put your `.txt` books in the sketch `data/` folder.
2. Use the ESP32 LittleFS data upload tool/plugin in Arduino IDE.
3. Set : `Tools` > `Partition Scheme` > `NO OTA / 1MB App / 3MB SPIFFS (littlefs)` to maximise size for books
4. Flash the sketch

## Tuning

Main parameters are in `params.h`:

- `WORD_DELAY_MS`, `CLAUSE_DELAY_MS`, `SENTENCE_DELAY_MS`
- `BOOK_BLOCK_SIZE`
- `ORP_OFFSET_FROM_CENTER_PX`
- `ORP_MARKER_WIDTH_PX`, `ORP_MARKER_HEIGHT_PX`, `ORP_MARKER_GAP_PX`
