# RSVP Reader (ESP32 + TFT)

A simple RSVP ([rapid serial visual presentation](https://en.wikipedia.org/wiki/Rapid_serial_visual_presentation)) ebook reader on ESP32 with an ILI9341 TFT screen.


## Features

- Reads `.txt` books from LittleFS (`data/` uploaded to board)
- latin1 char (french accents) are supported
- Block-based file reading (`BOOK_BLOCK_SIZE`, default `4096` bytes)
- RSVP word display with ORP alignment (not simple centering)
- ORP (Optimal Recognition Point) pivot letter highlighted in red
- Fixed ORP markers (top/bottom ticks)
- Faster rendering by clearing only previous word bounds (reduced flicker)
- Store book position using Preferences API


## Hardware

- ESP32
- ILI9341 TFT (SPI)
- Pins (from `display.h`): `CS=5`, `RST=0`, `DC=26`
- 2 buttons : play/pause & rewind/reset


## Connections

### Screen

- MISO to IO19
- LED to 3.3V
- SCK to IO18
- MOSI to IO23
- DC to IO26
- RESET to RST
- CS to IO05
- GND
- VCC to 3.3V


### buttons

- GND
- play/pause button to IO27
- rewind/reset button to IO32


## Convert books to simple texts

nodejs scripts to convert ebooks in epub or txt into simple latin1 text files are in `scripts/`.

```bash
cd scripts
npm install
node convert.js "../my-book.epub" "../data/my-book.txt"
```

Notes:
- Output is written in `latin1` encoding (important for french accents)
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
