# EPUB/TXT to E-Reader Converter

Converts EPUB and TXT files to format compatible with the ESP32 e-reader.
Accents are converted to latin1.

## Usage

```bash
node convert.js <input.epub|input.txt> [output.txt]
```

### Examples

```bash
# Convert EPUB to TXT
node convert.js book.epub -> book.txt

# Convert TXT to TXT
node convert.js mobydick.txt

# Output goes to same name without extension
node convert.js mybook.epub
```

## Output Format

The output file contains:

1. **Metadata header** (first 3 lines):
   ```
   TITLE=Book Title
   AUTHOR=Author Name
   ---
   ```

2. **Escaped text content** - The rest of the file contains the book content with:
   - Accented characters converted to hex escapes (e.g., `é` → `\xE9`)
   - Line breaks within sentences removed (merged into spaces)
   - Multiple consecutive line breaks limited to 2
   - Common punctuation normalized (e.g., em-dash → dash)

## Processing

### EPUB Files
- Extracts text from XHTML content files
- Removes HTML tags, keeping only text content
- Handles UTF-8 encoded accented characters
- Preserves paragraph breaks (double newlines)

### TXT Files
- Reads plain text directly
- Same processing applied as EPUB

## Accent Character Encoding

All non-ASCII characters are encoded as hex escape sequences:
- `é` → `\xE9`
- `è` → `\xE8`
- `ê` → `\xEA`
- `à` → `\xE0`
- `ç` → `\xE7`
- `ñ` → `\xF1`
- etc.

This format matches what the ESP32 firmware expects.

## Line Break Handling

- Single line breaks within sentences are removed (merged into spaces)
- Multiple consecutive line breaks (3+) are limited to 2
- Paragraph breaks (double newlines) are preserved

## Dependencies

- Node.js 24+
