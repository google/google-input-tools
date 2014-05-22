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

#ifndef GGADGET_UNICODE_UTILS_H__
#define GGADGET_UNICODE_UTILS_H__

#include <string>
#include <ggadget/common.h>

namespace ggadget {
#if defined(OS_WIN)
typedef wchar_t UTF16Char;  // wchar_t is a native type on windows.
#elif defined(OS_POSIX)
typedef uint16_t UTF16Char;
#endif
typedef uint32_t UTF32Char;
}

#if defined(OS_POSIX)
namespace std {
/* To meet the ODR(One Definition Rule) requirement */
extern template class std::basic_string<ggadget::UTF16Char>;
extern template class std::basic_string<ggadget::UTF32Char>;
}
#endif

namespace ggadget {

/**
 * @defgroup UnicodeUtilities Unicode utilities
 * @ingroup Utilities
 * @{
 */

typedef std::basic_string<UTF16Char> UTF16String;
typedef std::basic_string<UTF32Char> UTF32String;

/**
 * Converts a utf8 char sequence to a utf32 char code.
 *
 * @param src the utf8 char sequence to be converted.
 * @param src_length length of the source utf8 char sequence.
 * @param[out] dest result utf32 char code.
 * @return how many bytes in src have been converted.
 */
size_t ConvertCharUTF8ToUTF32(const char *src, size_t src_length,
                              UTF32Char *dest);

/**
 * Converts a utf32 char code to a utf8 char sequence.
 *
 * @param src the utf32 char code be converted.
 * @param[out] dest buffer to store the result utf8 char sequence.
 * @param dest_length length of the dest utf8 char sequence buffer.
 * @return how many bytes have been written into dest.
 */
size_t ConvertCharUTF32ToUTF8(UTF32Char src, char *dest,
                              size_t dest_length);

/**
 * Converts a utf16 char sequence to a utf32 char code.
 *
 * @param src the utf16 char sequence to be converted.
 * @param src_length length of the source utf16 char sequence.
 * @param[out] dest result utf32 char code.
 * @return how many @c UTF16Char elements in src have been converted.
 */
size_t ConvertCharUTF16ToUTF32(const UTF16Char *src, size_t src_length,
                               UTF32Char *dest);

/**
 * Converts a utf32 char code to a utf16 char sequence.
 *
 * @param src the utf32 char code be converted.
 * @param[out] dest buffer to store the result utf16 char sequence.
 * @param dest_length length of the dest utf16 char sequence buffer.
 * @return how many @c UTF16Char elements have been written into dest.
 */
size_t ConvertCharUTF32ToUTF16(UTF32Char src, UTF16Char *dest,
                               size_t dest_length);

/**
 * Converts a utf8 string to a utf32 string.
 *
 * @param src the utf8 string to be converted.
 * @param src_length length of the source utf8 string.
 * @param[out] dest result utf32 string.
 * @return how many bytes in source utf8 string have been converted.
 */
size_t ConvertStringUTF8ToUTF32(const char *src, size_t src_length,
                                UTF32String *dest);

/**
 * Converts a utf8 string to a utf32 string.
 *
 * Same as above function but takes a std::string object as source.
 */
size_t ConvertStringUTF8ToUTF32(const std::string &src, UTF32String *dest);

/**
 * Converts a utf32 string to a utf8 string.
 *
 * @param src the utf32 string to be converted.
 * @param src_length length of the source utf32 string.
 * @param[out] dest result utf8 string.
 * @return how many chars in source utf32 string have been converted.
 */
size_t ConvertStringUTF32ToUTF8(const UTF32Char *src, size_t src_length,
                                std::string *dest);

/**
 * Converts a utf32 string to a utf8 string.
 *
 * Same as above function but takes a @c UTF32String object as source.
 */
size_t ConvertStringUTF32ToUTF8(const UTF32String &src, std::string *dest);

/**
 * Converts a utf8 string to a utf16 string.
 *
 * @param src the utf8 string to be converted.
 * @param src_length length of the source utf8 string.
 * @param[out] dest result utf16 string.
 * @return how many bytes in source utf8 string have been converted.
 */
size_t ConvertStringUTF8ToUTF16(const char *src, size_t src_length,
                                UTF16String *dest);

/**
 * Converts a utf8 string to a utf16 string.
 *
 * Same as above function but takes a std::string object as source.
 */
size_t ConvertStringUTF8ToUTF16(const std::string &src, UTF16String *dest);

/**
 * Converts a utf8 string into a utf16 buffer. Doesn't output the ending
 * zero.
 *
 * @param src the utf8 string to be converted.
 * @param src_length length of the source utf8 string.
 * @param dest the output buffer.
 * @param dest_length the length of the destination buffer (not including the
 *     ending zero).
 * @param[out] used_dest_length actual output length used in output buffer
 *     (no ending zero will be output.)
 * @return how many bytes in source utf8 string have been converted.
 */
size_t ConvertStringUTF8ToUTF16Buffer(const char *src, size_t src_length,
                                      UTF16Char *dest, size_t dest_length,
                                      size_t *used_dest_length);

/**
 * Converts a utf8 string into a utf16 buffer.
 *
 * Same as above function but takes a std::string object as source.
 */
size_t ConvertStringUTF8ToUTF16Buffer(const std::string &src,
                                      UTF16Char *dest, size_t dest_length,
                                      size_t *used_dest_length);

/**
 * Converts a utf16 string to a utf8 string.
 *
 * @param src the utf16 string to be converted.
 * @param src_length length of the source utf16 string.
 * @param[out] dest result utf8 string.
 * @return how many @c UTF16Char elements in source utf16 string have been
 * converted.
 */
size_t ConvertStringUTF16ToUTF8(const UTF16Char *src, size_t src_length,
                                std::string *dest);

/**
 * Converts a utf16 string to a utf8 string.
 *
 * Same as above function but takes a @c UTF16String object as source.
 */
size_t ConvertStringUTF16ToUTF8(const UTF16String &src, std::string *dest);

/**
 * Converts a utf16 string into a utf8 buffer. Doesn't output the ending
 * zero.
 *
 * @param src the utf16 string to be converted.
 * @param src_length length of the source utf8 string.
 * @param dest the output buffer.
 * @param dest_length the length of the destination buffer (not including the
 *     ending zero).
 * @param[out] used_dest_length actual output length used in output buffer
 *     (no ending zero will be output.)
 * @return how many @c UTF16Char elements in source utf16 string have been
 *     converted.
 */
size_t ConvertStringUTF16ToUTF8Buffer(const UTF16Char *src, size_t src_length,
                                      char *dest, size_t dest_length,
                                      size_t *used_dest_length);

/**
 * Converts a utf16 string into a utf16 buffer.
 *
 * Same as above function but takes a UTF16String object as source.
 */
size_t ConvertStringUTF16ToUTF8Buffer(const UTF16String &src,
                                      char *dest, size_t dest_length,
                                      size_t *used_dest_length);

/**
 * This class is a temporary converter to convert a utf16 string to utf8.
 * It call avoid memory allocations if the length of the converted string is
 * less than 64 bytes.
 */
class UTF16ToUTF8Converter {
 public:
  UTF16ToUTF8Converter(const UTF16String &src) {
    DoConvert(src.c_str(), src.length());
  }
  UTF16ToUTF8Converter(const UTF16Char *src, size_t src_length) {
    DoConvert(src, src_length);
  }

  const char *get() {
    return dynamic_buffer_.size() ? dynamic_buffer_.c_str() : buffer_;
  }
 private:
  void DoConvert(const UTF16Char *src, size_t src_length);
  char buffer_[64];
  std::string dynamic_buffer_;
};

/**
 * Converts a utf16 string to a utf32 string.
 *
 * @param src the utf16 string to be converted.
 * @param src_length length of the source utf16 string.
 * @param[out] dest result utf32 string.
 * @return how many @c UTF16Char elements in source utf16 string have been
 * converted.
 */
size_t ConvertStringUTF16ToUTF32(const UTF16Char *src, size_t src_length,
                                 UTF32String *dest);
/**
 * Converts a utf16 string to a utf32 string.
 *
 * Same as above function but takes a @c UTF16String object as source.
 */
size_t ConvertStringUTF16ToUTF32(const UTF16String &src, UTF32String *dest);

/**
 * Converts a utf32 string to a utf16 string.
 *
 * @param src the utf32 string to be converted.
 * @param src_length length of the source utf32 string.
 * @param[out] dest result utf16 string.
 * @return how many chars in source utf32 string have been converted.
 */
size_t ConvertStringUTF32ToUTF16(const UTF32Char *src, size_t src_length,
                                 UTF16String *dest);

/**
 * Converts a utf32 string to a utf16 string.
 *
 * Same as above function but takes a @c UTF32String object as source.
 */
size_t ConvertStringUTF32ToUTF16(const UTF32String &src, UTF16String *dest);

/**
 * Gets the length of a utf8 char sequence.
 *
 * @param src the source utf8 char sequence.
 * @return the length of a utf8 char sequence in bytes.
 */
size_t GetUTF8CharLength(const char *src);

/**
 * Gets the length of a utf8 string.
 *
 * @param pos the starting point of the string.
 * @param char_count the characters contained in the utf8 string.
 * @return the length of the utf8 string in bytes.
 */
size_t GetUTF8CharsLength(const char *pos, size_t char_count, size_t limit);

/**
 * Gets the character count of a utf8 string.
 *
 * @param src the starting point of the string.
 * @param bytes the total bytes of the string.
 * @return the character count of the utf8 string.
 */
size_t GetUTF8StringCharCount(const char *src, size_t bytes);

/**
 * Checks if a utf8 char sequence valid.
 *
 * @param src the source utf8 char sequence to be checked.
 * @param length length of the source utf8 char sequence,
 *        which should be exactly equal to the length
 *        returned by @c GetUTF8CharLength function.
 * @return true if the source utf8 char sequence is valid.
 */
bool IsLegalUTF8Char(const char *src, size_t length);

/**
 * Gets the length of a utf16 char sequence.
 *
 * @param src the source utf16 char sequence.
 * @return the length of a utf16 char sequence in number of @c UTF16Char
 * elements.
 */
size_t GetUTF16CharLength(const UTF16Char *src);

/**
 * Checks if a utf16 char sequence valid.
 *
 * @param src the source utf16 char sequence to be checked.
 * @param length length of the source utf16 char sequence,
 *        which should be exactly equal to the length
 *        returned by @c GetUTF16CharLength function.
 * @return true if the source utf16 char sequence is valid.
 */
bool IsLegalUTF16Char(const UTF16Char *src, size_t length);

/**
 * Checks if a string is a valid UTF8 string.
 *
 * @param src the string to be checked.
 * @param length length of the source string.
 * @return true if the source string is a valid UTF8 string.
 */
bool IsLegalUTF8String(const char *src, size_t length);

/**
 * Checks if a string is a valid UTF8 string.
 *
 * Same as above function but takes a @c std::string object as source.
 */
bool IsLegalUTF8String(const std::string &src);

/**
 * Checks if a string is a valid UTF16 string.
 *
 * @param src the string to be checked.
 * @param length length of the source string.
 * @return true if the source string is a valid UTF16 string.
 */
bool IsLegalUTF16String(const UTF16Char *src, size_t length);

/**
 * Checks if a string is a valid UTF8 string.
 *
 * Same as above function but takes a @c std::string object as source.
 */
bool IsLegalUTF16String(const UTF16String &src);

const UTF32Char kUnicodeReplacementChar = 0xFFFD;
const UTF32Char kUnicodeMaxLegalChar = 0x10FFFF;
const UTF32Char kUnicodeMaxBMPChar = 0xFFFF;

// The Unicode char Zero-Width-Non-Breakable-Space (ZWNBSP) used as the
// byte order mark (BOM).
const UTF32Char kUnicodeZeroWidthNBSP = 0xFEFF;

/** Various BOMs. Note: they are not zero-ended strings, but char arrays. */
const char kUTF8BOM[] = { '\xEF', '\xBB', '\xBF' };
const char kUTF16LEBOM[] = { '\xFF', '\xFE' };
const char kUTF16BEBOM[] = { '\xFE', '\xFF' };
const char kUTF32LEBOM[] = { '\xFF', '\xFE', 0, 0 };
const char kUTF32BEBOM[] = { 0, 0, '\xFE', '\xFF' };

/**
 * Detects the UTF encoding of a byte stream if it has BOM, or from the
 * stream contents if we can confidentally determine its UTF encoding.
 * We don't check BOM-less UTF-8, because there is ambiguity among UTF-8 and
 * some CJK encodings.
 *
 * @param stream the input byte stream.
 * @param[out] encoding (optional) name of the encoding detected from BOM.
 *     Possible values are "UTF-8", "UTF-16LE", "UTF-16BE", "UTF-32LE"
 *     and "UTF-32BE".
 * @return @c true if the input stream has valid BOM or from the stream
 *     contents we can confidentally determine its UTF encoding.
 */
bool DetectUTFEncoding(const std::string &stream, std::string *encoding);

/**
 * Detects the encoding of a byte stream based on both BOM and contents,
 * and converts a byte stream into UTF8. If there is no BOM, but the stream
 * is a valid UTF8 stream, return the stream directly. Otherwise, the stream
 * will be converted to UTF8 as if the stream's encoding is ISO8859-1.
 *
 * @param stream the input byte stream.
 * @param[out] result the converted result.
 * @param[out] encoding (optional)the encoding detected from BOM.
 * @return @c true if succeeds.
 */
bool DetectAndConvertStreamToUTF8(const std::string &stream,
                                  std::string *result, std::string *encoding);

/**
 * Converts a locale string to a UTF16 string, depending on the LC_CTYPE
 * category of the current locale.
 *
 * @param input a NULL-terminated locale string.
 * @param[out] result the converted result.
 * @return @c true if succeeds.
 */
bool ConvertLocaleStringToUTF16(const char *input, UTF16String *result);

/**
 * Converts a UTF16 string to a locale string, depending on the LC_CTYPE
 * category of the current locale.
 *
 * @param input a UTF16 string.
 * @param input_size
 * @param[out] result the converted result.
 * @return @c true if succeeds.
 */
bool ConvertUTF16ToLocaleString(const UTF16Char *input, size_t input_size,
                                std::string *result);

bool ConvertUTF16ToLocaleString(const UTF16String &input, std::string *result);

/**
 * Converts a locale string to a UTF8 string, depending on the LC_CTYPE
 * category of the current locale.
 *
 * @param input a NULL-terminated locale string.
 * @param[out] result the converted result.
 * @return @c true if succeeds.
 */
bool ConvertLocaleStringToUTF8(const char *input, std::string *result);

/**
 * Converts a UTF8 string to a locale string, depending on the LC_CTYPE
 * category of the current locale.
 *
 * @param input a UTF8 string.
 * @param input_size
 * @param[out] result the converted result.
 * @return @c true if succeeds.
 */
bool ConvertUTF8ToLocaleString(const char *input, size_t input_size,
                               std::string *result);

bool ConvertUTF8ToLocaleString(const std::string &input, std::string *result);

/**
 * Compares two locale strings, depending on the LC_COLLATE category of the
 * current locale.
 *
 * @param s1 the first input locale string.
 * @param s2 the second input locale string.
 * @return an integer less than, equal to, or greater than zero if @a s1 is
 *     less than, equals, or is greater than @a s2.
 */
int CompareLocaleStrings(const char *s1, const char *s2);

/** @} */

} // namespace ggadget

#endif // GGADGET_UNICODE_UTILS_H__
