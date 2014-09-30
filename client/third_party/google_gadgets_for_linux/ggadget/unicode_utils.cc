/*
  Copyright 2011 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

// This code is based on ConvertUTF.c and ConvertUTF.h released by Unicode, Inc.

/*
 * Copyright 2001-2004 Unicode, Inc.
 *
 * Disclaimer
 *
 * This source code is provided as is by Unicode, Inc. No claims are
 * made as to fitness for any particular purpose. No warranties of any
 * kind are expressed or implied. The recipient agrees to determine
 * applicability of information provided. If this file has been
 * purchased on magnetic or optical media from Unicode, Inc., the
 * sole remedy for any claim will be exchange of defective media
 * within 90 days of receipt.
 *
 * Limitations on Rights to Redistribute This Code
 *
 * Unicode, Inc. hereby grants the right to freely use the information
 * supplied in this file in the creation of products supporting the
 * Unicode Standard, and to make copies of this file in any form
 * for internal or external distribution as long as this notice
 * remains attached.
 */

#include <cstring>
#include <cstdlib>
#include "unicode_utils.h"
#if defined(OS_POSIX)
namespace std {
template class std::basic_string<ggadget::UTF16Char>;
template class std::basic_string<ggadget::UTF32Char>;
}
#endif

namespace ggadget {
typedef unsigned char UTF8Char;
static const int kHalfShift = 10;
static const UTF32Char kHalfBase = 0x0010000UL;
static const UTF32Char kHalfMask = 0x3FFUL;

static const UTF32Char kSurrogateHighStart = 0xD800;
static const UTF32Char kSurrogateHighEnd = 0xDBFF;
static const UTF32Char kSurrogateLowStart = 0xDC00;
static const UTF32Char kSurrogateLowEnd = 0xDFFF;

static const UTF8Char kTrailingBytesForUTF8[256] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

static const UTF32Char kOffsetsFromUTF8[6] = {
  0x00000000UL, 0x00003080UL, 0x000E2080UL,
  0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

static const UTF8Char kFirstByteMark[7] = {
  0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
};

static inline bool IsLegalUTF8CharInternal(const char *src, size_t length) {
  const UTF8Char *srcptr = reinterpret_cast<const UTF8Char*>(src);
  UTF8Char a;
  UTF8Char ch = *srcptr;
  srcptr += length;
  switch (length) {
    default: return false;
    // Everything else falls through when "true"...
    case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 2: if ((a = (*--srcptr)) > 0xBF) return false;
      switch (ch) {
        // No fall-through in this inner switch
        case 0xE0: if (a < 0xA0) return false; break;
        case 0xED: if (a > 0x9F) return false; break;
        case 0xF0: if (a < 0x90) return false; break;
        case 0xF4: if (a > 0x8F) return false; break;
        default:   if (a < 0x80) return false;
      }
    case 1: if (ch >= 0x80 && ch < 0xC2) return false;
  }
  if (ch > 0xF4) return false;
  return true;
}

static inline size_t ConvertCharUTF8ToUTF32Internal(const char *src,
                                                    size_t src_length,
                                                    UTF32Char *dest) {
  const UTF8Char *p = reinterpret_cast<const UTF8Char*>(src);

  size_t extra_bytes = kTrailingBytesForUTF8[*p];
  if (extra_bytes >= src_length ||
      !IsLegalUTF8CharInternal(src, extra_bytes + 1)) {
    *dest = 0;
    return 0;
  }

  UTF32Char result = 0;
  // The cases all fall through.
  switch (extra_bytes) {
    case 5: result += *p++; result <<= 6;
    case 4: result += *p++; result <<= 6;
    case 3: result += *p++; result <<= 6;
    case 2: result += *p++; result <<= 6;
    case 1: result += *p++; result <<= 6;
    case 0: result += *p++;
  }
  result -= kOffsetsFromUTF8[extra_bytes];
  if (result > kUnicodeMaxLegalChar ||
      (result >= kSurrogateHighStart && result <= kSurrogateLowEnd))
    result = kUnicodeReplacementChar;
  *dest = result;
  return extra_bytes + 1;
}

size_t ConvertCharUTF8ToUTF32(const char *src, size_t src_length,
                              UTF32Char *dest) {
  if (!src || !*src || !src_length || !dest) {
    return 0;
  }
  return ConvertCharUTF8ToUTF32Internal(src, src_length, dest);
}

static inline size_t ConvertCharUTF32ToUTF8Internal(UTF32Char src,
                                                    char *dest,
                                                    size_t dest_length) {
  const UTF32Char kByteMask = 0xBF;
  const UTF32Char kByteMark = 0x80;

  if (src > kUnicodeMaxLegalChar ||
      (src >= kSurrogateHighStart && src <= kSurrogateLowEnd) ||
      !dest || !dest_length)
    return 0;

  UTF8Char *p = reinterpret_cast<UTF8Char*>(dest);

  size_t bytes_to_write = 0;
  if (src < 0x80)
    bytes_to_write = 1;
  else if (src < 0x800)
    bytes_to_write = 2;
  else if (src < 0x10000)
    bytes_to_write = 3;
  else
    bytes_to_write = 4;

  if (bytes_to_write > dest_length)
    return 0;

  p += bytes_to_write;
  // Everything falls through.
  switch (bytes_to_write) {
    case 4:
      *--p = static_cast<UTF8Char>((src | kByteMark) & kByteMask); src >>=6;
    case 3:
      *--p = static_cast<UTF8Char>((src | kByteMark) & kByteMask); src >>=6;
    case 2:
      *--p = static_cast<UTF8Char>((src | kByteMark) & kByteMask); src >>=6;
    case 1:
      *--p = static_cast<UTF8Char>(src | kFirstByteMark[bytes_to_write]);
  }

  return bytes_to_write;
}

size_t ConvertCharUTF32ToUTF8(UTF32Char src, char *dest, size_t dest_length) {
  if (!dest || !dest_length) {
    return 0;
  }
  return ConvertCharUTF32ToUTF8Internal(src, dest, dest_length);
}

static inline size_t ConvertCharUTF16ToUTF32Internal(const UTF16Char *src,
                                                     size_t src_length,
                                                     UTF32Char *dest) {
  UTF32Char high;
  high = *src;
  if (high >= kSurrogateHighStart && high <= kSurrogateHighEnd) {
    if (src_length > 1) {
      UTF32Char low = *(src+1);
      if (low >= kSurrogateLowStart && low <= kSurrogateLowEnd) {
        *dest = ((high - kSurrogateHighStart) << kHalfShift) +
                (low - kSurrogateLowStart) + kHalfBase;
        return 2;
      }
    }
    return 0;
  } else if (high >= kSurrogateLowStart && high <= kSurrogateLowEnd) {
    return 0;
  }
  *dest = high;
  return 1;
}

size_t ConvertCharUTF16ToUTF32(const UTF16Char *src, size_t src_length,
                               UTF32Char *dest) {
  if (!src || !*src || !src_length || !dest) {
    return 0;
  }
  return ConvertCharUTF16ToUTF32Internal(src, src_length, dest);
}

static inline size_t ConvertCharUTF32ToUTF16Internal(UTF32Char src,
                                                     UTF16Char *dest,
                                                     size_t dest_length) {
  if (src <= kUnicodeMaxBMPChar) {
    if (src >= kSurrogateHighStart && src <= kSurrogateLowEnd)
      return 0;
    *dest = static_cast<UTF16Char>(src);
    return 1;
  } else if (src <= kUnicodeMaxLegalChar && dest_length > 1) {
    src -= kHalfBase;
    *dest++ = static_cast<UTF16Char>((src >> kHalfShift) + kSurrogateHighStart);
    *dest = static_cast<UTF16Char>((src & kHalfMask) + kSurrogateLowStart);
    return 2;
  }
  return 0;
}

size_t ConvertCharUTF32ToUTF16(UTF32Char src, UTF16Char *dest,
                               size_t dest_length) {
  if (!dest || !dest_length)
    return 0;
  return ConvertCharUTF32ToUTF16Internal(src, dest, dest_length);
}

size_t ConvertStringUTF8ToUTF32(const char *src, size_t src_length,
                                UTF32String *dest) {
  if (!dest)
    return 0;
  dest->clear();
  if (!src || !src_length)
    return 0;

  dest->reserve(src_length);
  size_t used_length = 0;
  size_t utf8_len;
  UTF32Char utf32;
  while (src_length && *src) {
    utf8_len = ConvertCharUTF8ToUTF32Internal(src, src_length, &utf32);
    if (!utf8_len) break;
    dest->push_back(utf32);
    used_length += utf8_len;
    src += utf8_len;
    src_length -= utf8_len;
  }
  return used_length;
}

size_t ConvertStringUTF8ToUTF32(const std::string &src, UTF32String *dest) {
  return ConvertStringUTF8ToUTF32(src.c_str(), src.length(), dest);
}

size_t ConvertStringUTF32ToUTF8(const UTF32Char *src, size_t src_length,
                                std::string *dest) {
  if (!dest)
    return 0;
  dest->clear();
  if (!src || !src_length)
    return 0;

  dest->reserve(src_length * 3);
  size_t used_length = 0;
  size_t utf8_len;
  char utf8[6];
  while (src_length && *src) {
    utf8_len = ConvertCharUTF32ToUTF8Internal(*src, utf8, 6);
    if (!utf8_len) break;
    dest->append(utf8, utf8_len);
    ++used_length;
    ++src;
    --src_length;
  }
  return used_length;
}

size_t ConvertStringUTF32ToUTF8(const UTF32String &src, std::string *dest) {
  return ConvertStringUTF32ToUTF8(src.c_str(), src.length(), dest);
}

size_t ConvertStringUTF8ToUTF16(const char *src, size_t src_length,
                                UTF16String *dest) {
  if (!dest)
    return 0;
  dest->clear();
  if (!src || !src_length)
    return 0;

  dest->reserve(src_length);
  size_t used_length = 0;
  size_t utf8_len;
  size_t utf16_len;
  UTF16Char utf16[2];
  UTF32Char utf32;
  while (src_length && *src) {
    utf8_len = ConvertCharUTF8ToUTF32Internal(src, src_length, &utf32);
    if (!utf8_len) break;
    utf16_len = ConvertCharUTF32ToUTF16Internal(utf32, utf16, 2);
    if (!utf16_len) break;
    dest->append(utf16, utf16_len);
    used_length += utf8_len;
    src += utf8_len;
    src_length -= utf8_len;
  }
  return used_length;
}

size_t ConvertStringUTF8ToUTF16(const std::string &src, UTF16String *dest) {
  return ConvertStringUTF8ToUTF16(src.c_str(), src.length(), dest);
}

size_t ConvertStringUTF8ToUTF16Buffer(const char *src, size_t src_length,
                                      UTF16Char *dest, size_t dest_length,
                                      size_t *used_dest_length) {
  if (!used_dest_length)
    return 0;
  *used_dest_length = 0;
  if (!dest || !dest_length || !src || !*src)
    return 0;

  size_t used_src_length = 0;
  size_t utf8_len;
  size_t utf16_len;
  UTF16Char utf16[2] = { 0, 0 };
  UTF32Char utf32;
  while (src_length && *src) {
    utf8_len = ConvertCharUTF8ToUTF32Internal(src, src_length, &utf32);
    if (!utf8_len) break;
    if (dest_length >= 2) {
      utf16_len = ConvertCharUTF32ToUTF16Internal(utf32, dest, 2);
      if (!utf16_len) break;
    } else {
      utf16_len = ConvertCharUTF32ToUTF16Internal(utf32, utf16, 2);
      if (!utf16_len || utf16_len > dest_length)
        break;
      dest[0] = utf16[0];
      if (utf16_len == 2)
        dest[1] = utf16[1];
    }
    dest += utf16_len;
    dest_length -= utf16_len;
    *used_dest_length += utf16_len;
    used_src_length += utf8_len;
    src += utf8_len;
    src_length -= utf8_len;
  }
  return used_src_length;
}

size_t ConvertStringUTF8ToUTF16Buffer(const std::string &src,
                                      UTF16Char *dest, size_t dest_length,
                                      size_t *used_dest_length) {
  return ConvertStringUTF8ToUTF16Buffer(src.c_str(), src.length(),
                                        dest, dest_length, used_dest_length);
}

size_t ConvertStringUTF16ToUTF8(const UTF16Char *src, size_t src_length,
                                std::string *dest) {
  if (!dest)
    return 0;
  dest->clear();
  if (!src || !src_length)
    return 0;

  dest->reserve(src_length * 3);
  size_t used_length = 0;
  size_t utf8_len;
  size_t utf16_len;
  char utf8[6];
  UTF32Char utf32;
  while (src_length && *src) {
    utf16_len = ConvertCharUTF16ToUTF32Internal(src, src_length, &utf32);
    if (!utf16_len) break;
    utf8_len = ConvertCharUTF32ToUTF8Internal(utf32, utf8, 6);
    if (!utf8_len) break;
    dest->append(utf8, utf8_len);
    used_length += utf16_len;
    src += utf16_len;
    src_length -= utf16_len;
  }
  return used_length;
}

size_t ConvertStringUTF16ToUTF8(const UTF16String &src, std::string *dest) {
  return ConvertStringUTF16ToUTF8(src.c_str(), src.length(), dest);
}

size_t ConvertStringUTF16ToUTF8Buffer(const UTF16Char *src, size_t src_length,
                                      char *dest, size_t dest_length,
                                      size_t *used_dest_length) {
  if (!used_dest_length)
    return 0;
  *used_dest_length = 0;
  if (!dest || !dest_length || !src || !*src)
    return 0;

  size_t used_src_length = 0;
  size_t utf8_len;
  size_t utf16_len;
  char utf8[4] = { 0, 0, 0, 0 };
  UTF32Char utf32;
  while (src_length && *src) {
    utf16_len = ConvertCharUTF16ToUTF32Internal(src, src_length, &utf32);
    if (!utf16_len) break;
    if (dest_length >= 4) {
      utf8_len = ConvertCharUTF32ToUTF8Internal(utf32, dest, 4);
      if (!utf8_len) break;
    } else {
      utf8_len = ConvertCharUTF32ToUTF8Internal(utf32, utf8, 4);
      if (!utf8_len || utf8_len > dest_length)
        break;
      memcpy(dest, utf8, utf8_len);
    }
    dest += utf8_len;
    dest_length -= utf8_len;
    *used_dest_length += utf8_len;
    used_src_length += utf16_len;
    src += utf16_len;
    src_length -= utf16_len;
  }
  return used_src_length;
}

size_t ConvertStringUTF16ToUTF8Buffer(const UTF16String &src,
                                      char *dest, size_t dest_length,
                                      size_t *used_dest_length) {
  return ConvertStringUTF16ToUTF8Buffer(src.c_str(), src.length(),
                                        dest, dest_length, used_dest_length);
}

void UTF16ToUTF8Converter::DoConvert(const UTF16Char *src, size_t src_length) {
  size_t used = 0;
  if (ConvertStringUTF16ToUTF8Buffer(src, src_length, buffer_,
                                     arraysize(buffer_) - 1, &used) ==
      src_length) {
    // The static buffer can contain the whole result, so use it.
    ASSERT(used < arraysize(buffer_));
    buffer_[used] = 0;
  } else {
    buffer_[0] = 0;
    ConvertStringUTF16ToUTF8(src, src_length, &dynamic_buffer_);
  }
}

size_t ConvertStringUTF16ToUTF32(const UTF16Char *src, size_t src_length,
                                 UTF32String *dest) {
  if (!dest)
    return 0;
  dest->clear();
  if (!src || !src_length)
    return 0;

  dest->reserve(src_length);
  size_t used_length = 0;
  size_t utf16_len;
  UTF32Char utf32;
  while (src_length && *src) {
    utf16_len = ConvertCharUTF16ToUTF32Internal(src, src_length, &utf32);
    if (!utf16_len) break;
    dest->push_back(utf32);
    used_length += utf16_len;
    src += utf16_len;
    src_length -= utf16_len;
  }
  return used_length;
}

size_t ConvertStringUTF16ToUTF32(const UTF16String &src, UTF32String *dest) {
  return ConvertStringUTF16ToUTF32(src.c_str(), src.length(), dest);
}

size_t ConvertStringUTF32ToUTF16(const UTF32Char *src, size_t src_length,
                                 UTF16String *dest) {
  if (!dest)
    return 0;
  dest->clear();
  if (!src || !src_length)
    return 0;

  dest->reserve(src_length);
  size_t used_length = 0;
  size_t utf16_len;
  UTF16Char utf16[2];
  while (src_length && *src) {
    utf16_len = ConvertCharUTF32ToUTF16Internal(*src, utf16, 2);
    if (!utf16_len) break;
    dest->append(utf16, utf16_len);
    ++used_length;
    ++src;
    --src_length;
  }
  return used_length;
}

size_t ConvertStringUTF32ToUTF16(const UTF32String &src, UTF16String *dest) {
  return ConvertStringUTF32ToUTF16(src.c_str(), src.length(), dest);
}

size_t GetUTF8CharLength(const char *src) {
  return src ?
         (kTrailingBytesForUTF8[static_cast<UTF8Char>(*src)] + 1) :
         0;
}

size_t GetUTF8CharsLength(const char *pos, size_t char_count, size_t limit) {
  size_t p = 0;
  for (size_t i = 0; i < char_count && p < limit; ++i) {
    p += GetUTF8CharLength(pos + p);
  }
  return p;
}

size_t GetUTF8StringCharCount(const char *src, size_t bytes) {
  size_t c = 0;
  while (c < bytes) {
    c += GetUTF8CharLength(src + c);
  }
  return c;
}

bool IsLegalUTF8Char(const char *src, size_t length) {
  if (!src || !length) return false;
  return IsLegalUTF8CharInternal(src, length);
}

size_t GetUTF16CharLength(const UTF16Char *src) {
  if (!src) return 0;
  if (*src < kSurrogateHighStart || *src > kSurrogateLowEnd)
    return 1;
  if (*src <= kSurrogateHighEnd &&
      *(src+1) >= kSurrogateLowStart && *(src+1) <= kSurrogateLowEnd)
    return 2;
  return 0;
}

bool IsLegalUTF16Char(const UTF16Char *src, size_t length) {
  return length && length == GetUTF16CharLength(src);
}

bool IsLegalUTF8String(const char *src, size_t length) {
  if (!src) return false;
  while (length > 0) {
    size_t char_length = GetUTF8CharLength(src);
    if (!char_length || char_length > length ||
        !IsLegalUTF8CharInternal(src, char_length))
      return false;
    length -= char_length;
    src += char_length;
  }
  return true;
}

bool IsLegalUTF8String(const std::string &src) {
  return IsLegalUTF8String(src.c_str(), src.length());
}

bool IsLegalUTF16String(const UTF16Char *src, size_t length) {
  if (!src) return false;
  while (length > 0) {
    size_t char_length = GetUTF16CharLength(src);
    if (!char_length || char_length > length ||
        !IsLegalUTF16Char(src, char_length))
      return false;
    length -= char_length;
    src += char_length;
  }
  return true;
}

bool IsLegalUTF16String(const UTF16String &src) {
  return IsLegalUTF16String(src.c_str(), src.length());
}

#define STARTS_WITH(content_ptr, content_size, pattern) \
    ((content_size) >= sizeof(pattern) && \
     memcmp((content_ptr), (pattern), sizeof(pattern)) == 0)

// A simple algorithm to detect BOM-less UTF-16 streams. It only applies to
// western languages that all UTF16 characters < 256.
// Returns 0 if stream is not UTF16, 1 if UTF16-LE, 2 if UTF16-BE.
static int DetectUTF16Encoding(const std::string &stream) {
  size_t content_size = stream.size();
  if (content_size == 0 || content_size % 2 != 0)
    return 0;

  int result = 0;
  for (size_t i = 0; i < content_size; i += 2) {
    char even = stream[i];
    char odd = stream[i + 1];
    if (!even) {
      if (!odd || result == 1)
        return 0;
      result = 2;
    } else if (!odd) {
      if (result == 2)
        return 0;
      result = 1;
    } else {
      return 0;
    }
  }
  return result;
}

bool DetectUTFEncoding(const std::string &stream, std::string *encoding) {
  ASSERT(encoding);
  const char *content_ptr = stream.c_str();
  size_t content_size = stream.size();
  if (STARTS_WITH(content_ptr, content_size, kUTF8BOM)) {
    if (encoding) *encoding = "UTF-8";
    return true;
  }
  if (STARTS_WITH(content_ptr, content_size, kUTF32LEBOM)) {
    if (encoding) *encoding = "UTF-32LE";
    return true;
  }
  if (STARTS_WITH(content_ptr, content_size, kUTF32BEBOM)) {
    if (encoding) *encoding = "UTF-32BE";
    return true;
  }
  if (STARTS_WITH(content_ptr, content_size, kUTF16LEBOM)) {
    if (encoding) *encoding = "UTF-16LE";
    return true;
  }
  if (STARTS_WITH(content_ptr, content_size, kUTF16BEBOM)) {
    if (encoding) *encoding = "UTF-16BE";
    return true;
  }

  // We don't check BOM-less UTF-8, because there is ambiguity among UTF-8 and
  // some CJK encodings. This function returns true only if the detection is
  // confidental.

  switch (DetectUTF16Encoding(stream)) {
    case 1:
      if (encoding) *encoding = "UTF-16LE";
      return true;
    case 2:
      if (encoding) *encoding = "UTF-16BE";
      return true;
    default:
      if (encoding) encoding->clear();
      return false;
  }
}

static void ConvertUTF16LEStreamToString(const char *input, size_t size,
                                         UTF16String *result) {
  ASSERT(result);
  result->clear();
  if (size < 2) return;
  result->reserve(size / 2);
  const unsigned char *stream = reinterpret_cast<const unsigned char *>(input);
  for (size_t i = 0; i < size - 1; i += 2)
    result->push_back(static_cast<UTF16Char>(stream[i] | (stream[i + 1] << 8)));
}

static void ConvertUTF16BEStreamToString(const char *input, size_t size,
                                         UTF16String *result) {
  ASSERT(result);
  result->clear();
  if (size < 2) return;
  result->reserve(size / 2);
  const unsigned char *stream = reinterpret_cast<const unsigned char *>(input);
  for (size_t i = 0; i < size - 1; i += 2)
    result->push_back(static_cast<UTF16Char>((stream[i] << 8) | stream[i + 1]));
}

static void ConvertUTF32LEStreamToString(const char *input, size_t size,
                                         UTF32String *result) {
  ASSERT(result);
  result->clear();
  if (size < 4) return;
  result->reserve(size / 4);
  const unsigned char *stream = reinterpret_cast<const unsigned char *>(input);
  for (size_t i = 0; i < size - 3; i += 4)
    result->push_back(static_cast<UTF32Char>(stream[i] | (stream[i + 1] << 8) |
                      (stream[i + 2] << 16) | (stream[i + 3] << 24)));
}

static void ConvertUTF32BEStreamToString(const char *input, size_t size,
                                         UTF32String *result) {
  ASSERT(result);
  result->clear();
  if (size < 4) return;
  result->reserve(size / 4);
  const unsigned char *stream = reinterpret_cast<const unsigned char *>(input);
  for (size_t i = 0; i < size - 3; i += 4)
    result->push_back(static_cast<UTF32Char>((stream[i] << 24) |
                      (stream[i + 1] << 16) | (stream[i + 2] << 8) |
                      stream[i + 3]));
}

bool DetectAndConvertStreamToUTF8(const std::string &stream,
                                  std::string *result, std::string *encoding) {
  ASSERT(result);
  const char *content_ptr = stream.c_str();
  size_t content_size = stream.size();
  bool valid = false;

  if (STARTS_WITH(content_ptr, content_size, kUTF8BOM)) {
    if (encoding) *encoding = "UTF-8";
    valid = IsLegalUTF8String(stream);
    if (valid) *result = stream.substr(sizeof(kUTF8BOM));
  } else if (STARTS_WITH(content_ptr, content_size, kUTF32LEBOM) &&
             content_size % 4 == 0) {
    if (encoding) *encoding = "UTF-32LE";
    UTF32String utf32;
    ConvertUTF32LEStreamToString(stream.c_str() + sizeof(kUTF32LEBOM),
                                 content_size - sizeof(kUTF32LEBOM), &utf32);
    valid = (ConvertStringUTF32ToUTF8(utf32, result) == utf32.size());
  } else if (STARTS_WITH(content_ptr, content_size, kUTF32BEBOM) &&
             content_size % 4 == 0) {
    if (encoding) *encoding = "UTF-32BE";
    UTF32String utf32;
    ConvertUTF32BEStreamToString(stream.c_str() + sizeof(kUTF32BEBOM),
                                 content_size - sizeof(kUTF32BEBOM), &utf32);
    valid = (ConvertStringUTF32ToUTF8(utf32, result) == utf32.size());
  } else if (STARTS_WITH(content_ptr, content_size, kUTF16LEBOM) &&
             content_size % 2 == 0) {
    if (encoding) *encoding = "UTF-16LE";
    UTF16String utf16;
    ConvertUTF16LEStreamToString(stream.c_str() + sizeof(kUTF16LEBOM),
                                 content_size - sizeof(kUTF16LEBOM), &utf16);
    valid = (ConvertStringUTF16ToUTF8(utf16, result) == utf16.size());
  } else if (STARTS_WITH(content_ptr, content_size, kUTF16BEBOM) &&
             content_size % 2 == 0) {
    if (encoding) *encoding = "UTF-16BE";
    UTF16String utf16;
    ConvertUTF16BEStreamToString(stream.c_str() + sizeof(kUTF16BEBOM),
                                 content_size - sizeof(kUTF16BEBOM), &utf16);
    valid = (ConvertStringUTF16ToUTF8(utf16, result) == utf16.size());
    // No BOM, try UTF-16 first
  } else {
    // The try BOM-less UTF-16.
    switch (DetectUTF16Encoding(stream)) {
      case 0:
        // Not UTF-16BE or UTF-16LE, try UTF-8.
        valid = IsLegalUTF8String(stream);
        if (valid) {
          if (encoding) *encoding = "UTF-8";
          *result = stream;
        }
        break;
      case 1: {
        UTF16String utf16;
        ConvertUTF16LEStreamToString(stream.c_str(), content_size, &utf16);
        if (encoding) *encoding = "UTF-16LE";
        valid = (ConvertStringUTF16ToUTF8(utf16, result) == utf16.size());
        break;
      }
      case 2: {
        UTF16String utf16;
        ConvertUTF16BEStreamToString(stream.c_str(), content_size, &utf16);
        if (encoding) *encoding = "UTF-16BE";
        valid = (ConvertStringUTF16ToUTF8(utf16, result) == utf16.size());
        break;
      }
      default:
        break;
    }
  }

  if (!valid) {
    if (encoding) *encoding = "ISO8859-1";
    // Not valid UTF-8, treat it as ISO8859-1.
    UTF16String utf16;
    utf16.reserve(content_size);
    for (size_t i = 0; i < content_size; i++)
      utf16 += static_cast<unsigned char>(content_ptr[i]);
    valid = (ConvertStringUTF16ToUTF8(utf16, result) == utf16.size());
  }

  if (!valid) {
    result->clear();
    if (encoding) encoding->clear();
  }
  return valid;
}

// Here we assume that if wchar_t is 2 bytes, the encoding is UTF16, and
// if wchar_t is 4 bytes, the encoding is UTF32.
#if GGL_SIZEOF_WCHAR_T != 2 && GGL_SIZEOF_WCHAR_T != 4
#error wchar_t must be 2 bytes or 4 bytes
#endif

bool ConvertLocaleStringToUTF16(const char *input, UTF16String *result) {
  ASSERT(input && result);
  if (!input || !result)
    return false;

  result->clear();
  size_t buffer_size = mbstowcs(NULL, input, 0) + 1;
  if (buffer_size == 0) // mbstowcs returns (size_t)-1 on error.
    return false;
  wchar_t *buffer(new wchar_t[buffer_size]);
  if (!buffer)
    return false;

  bool success = true;
  mbstowcs(buffer, input, buffer_size);
#if GGL_SIZEOF_WCHAR_T == 2
  result->assign(reinterpret_cast<UTF16Char *>(buffer));
#else
  success = ConvertStringUTF32ToUTF16(reinterpret_cast<UTF32Char *>(buffer),
                                      buffer_size - 1, result) ==
            buffer_size - 1;
#endif

  delete [] buffer;
  return success;
}

bool ConvertUTF16ToLocaleString(const UTF16Char *input, size_t input_size,
                                std::string *result) {
  ASSERT(input && result);
  if (!input || !result)
    return false;

  result->clear();
  if (!input_size)
    return true;

  const wchar_t *wchar_input = NULL;
#if GGL_SIZEOF_WCHAR_T == 2
  UTF16String utf16(input, input_size);
  wchar_input = reinterpret_cast<const wchar_t *>(utf16.c_str());
#else
  UTF32String utf32;
  if (ConvertStringUTF16ToUTF32(input, input_size, &utf32) != input_size)
    return false;
  wchar_input = reinterpret_cast<const wchar_t *>(utf32.c_str());
#endif

  size_t buffer_size = wcstombs(NULL, wchar_input, 0) + 1;
  if (buffer_size == 0) // wcstombs returns (size_t)-1 on error.
    return false;
  char *buffer(new char[buffer_size]);
  if (!buffer)
    return false;

  wcstombs(buffer, wchar_input, buffer_size);
  result->assign(buffer);
  delete [] buffer;
  return true;
}

bool ConvertUTF16ToLocaleString(const UTF16String &input, std::string *result) {
  return ConvertUTF16ToLocaleString(input.c_str(), input.size(), result);
}

bool ConvertLocaleStringToUTF8(const char *input, std::string *result) {
  ASSERT(input && result);
  if (!input || !result)
    return false;

  result->clear();
  size_t buffer_size = mbstowcs(NULL, input, 0) + 1;
  if (buffer_size == 0) // mbstowcs returns (size_t)-1 on error.
    return false;
  wchar_t *buffer(new wchar_t[buffer_size]);
  if (!buffer)
    return false;

  bool success = true;
  mbstowcs(buffer, input, buffer_size);
#if GGL_SIZEOF_WCHAR_T == 2
  success = ConvertStringUTF16ToUTF8(reinterpret_cast<UTF16Char *>(buffer),
                                     buffer_size - 1, result) ==
            buffer_size - 1;
#else
  success = ConvertStringUTF32ToUTF8(reinterpret_cast<UTF32Char *>(buffer),
                                     buffer_size - 1, result) ==
            buffer_size - 1;
#endif

  delete [] buffer;
  return success;
}

bool ConvertUTF8ToLocaleString(const char *input, size_t input_size,
                               std::string *result) {
  ASSERT(input && result);
  if (!input || !result)
    return false;

  result->clear();
  if (!input_size)
    return true;

  const wchar_t *wchar_input = NULL;
#if GGL_SIZEOF_WCHAR_T == 2
  UTF16String utf16;
  if (ConvertStringUTF8ToUTF16(input, input_size, &utf16) != input_size)
    return false;
  wchar_input = reinterpret_cast<const wchar_t *>(utf16.c_str());
#else
  UTF32String utf32;
  if (ConvertStringUTF8ToUTF32(input, input_size, &utf32) != input_size)
    return false;
  wchar_input = reinterpret_cast<const wchar_t *>(utf32.c_str());
#endif

  size_t buffer_size = wcstombs(NULL, wchar_input, 0) + 1;
  if (buffer_size == 0) // wcstombs returns (size_t)-1 on error.
    return false;
  char *buffer(new char[buffer_size]);
  if (!buffer)
    return false;

  wcstombs(buffer, wchar_input, buffer_size);
  result->assign(buffer);
  delete [] buffer;
  return true;
}

bool ConvertUTF8ToLocaleString(const std::string &input, std::string *result) {
  return ConvertUTF8ToLocaleString(input.c_str(), input.size(), result);
}

int CompareLocaleStrings(const char *s1, const char *s2) {
  return strcoll(s1, s2);
}

} // namespace ggadget
