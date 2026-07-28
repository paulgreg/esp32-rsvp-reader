#include "progress.h"

#include <Preferences.h>

namespace {
constexpr const char* kProgressNs = "rsvp";
constexpr const char* kKeyFilename = "file";
constexpr const char* kKeySentenceHead = "shead";
constexpr const char* kKeySentenceCount = "scount";
constexpr const char* kKeySentencePos = "spos";
}  // namespace

bool progressSave(const char* filename, uint8_t sentenceHead,
                  uint8_t sentenceCount, const size_t* sentencePos,
                  uint8_t sentencePosLen) {
  if (filename == nullptr || sentencePos == nullptr || sentencePosLen == 0) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(kProgressNs, false)) {
    return false;
  }

  const size_t sentencePosBytes = sizeof(size_t) * sentencePosLen;
  const bool ok =
      prefs.putString(kKeyFilename, filename) > 0 &&
      prefs.putUChar(kKeySentenceHead, sentenceHead) == sizeof(uint8_t) &&
      prefs.putUChar(kKeySentenceCount, sentenceCount) == sizeof(uint8_t) &&
      prefs.putBytes(kKeySentencePos, sentencePos, sentencePosBytes) ==
          sentencePosBytes;

  prefs.end();
  return ok;
}

bool progressLoad(const char* filename, uint8_t* sentenceHead,
                  uint8_t* sentenceCount, size_t* sentencePos,
                  uint8_t sentencePosLen) {
  if (filename == nullptr || sentenceHead == nullptr || sentenceCount == nullptr ||
      sentencePos == nullptr || sentencePosLen == 0) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(kProgressNs, true)) {
    return false;
  }

  if (!prefs.isKey(kKeyFilename) || !prefs.isKey(kKeySentenceHead) ||
      !prefs.isKey(kKeySentenceCount) || !prefs.isKey(kKeySentencePos)) {
    prefs.end();
    return false;
  }

  const String storedFilename = prefs.getString(kKeyFilename, "");
  if (storedFilename.isEmpty() || storedFilename != filename) {
    prefs.end();
    return false;
  }

  const size_t sentencePosBytes = sizeof(size_t) * sentencePosLen;
  if (prefs.getBytesLength(kKeySentencePos) != sentencePosBytes) {
    prefs.end();
    return false;
  }

  *sentenceHead = prefs.getUChar(kKeySentenceHead, 0);
  *sentenceCount = prefs.getUChar(kKeySentenceCount, 0);

  const size_t loadedBytes = prefs.getBytes(kKeySentencePos, sentencePos, sentencePosBytes);
  prefs.end();

  if (loadedBytes != sentencePosBytes) {
    return false;
  }

  if (*sentenceHead >= sentencePosLen) {
    *sentenceHead = sentencePosLen - 1;
  }

  if (*sentenceCount > sentencePosLen) {
    *sentenceCount = sentencePosLen;
  }

  return true;
}

bool progressClear() {
  Preferences prefs;
  if (!prefs.begin(kProgressNs, false)) {
    return false;
  }

  const bool ok = prefs.clear();
  prefs.end();
  return ok;
}
