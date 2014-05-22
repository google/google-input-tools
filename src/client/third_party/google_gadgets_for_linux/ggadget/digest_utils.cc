/*
  Copyright 2008 Google Inc.

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

// SHA1 algorithm is written based on the reference implementation of SHA1.
// See RFC3174 for details.
// BASE64 algorithm is written according to RFC4648.

#include <cstring>
#include "digest_utils.h"
#include "common.h"
#include "logger.h"

namespace ggadget {

static inline uint32_t SHA1Shift(int n, uint32_t X) {
  return (X << n) | (X >> (32-n));
}

static void SHA1ProcessBlock(unsigned char M[64], uint32_t H[5]) {
  uint32_t W[80];
  for (int t = 0; t < 64; t += 4)
    W[t / 4] = (M[t] << 24) | (M[t+1] << 16) | (M[t+2] << 8) | M[t+3];
  for (int t = 16; t < 80; ++t)
    W[t] = SHA1Shift(1, W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);

  uint32_t A = H[0];
  uint32_t B = H[1];
  uint32_t C = H[2];
  uint32_t D = H[3];
  uint32_t E = H[4];

  for (int t = 0; t < 20; ++t) {
    uint32_t TEMP = SHA1Shift(5, A) + ((B & C) | (~B & D)) +
                    E + W[t] + 0x5a827999;
    E = D; D = C; C = SHA1Shift(30, B); B = A; A = TEMP;
  }
  for (int t = 20; t < 40; ++t) {
    uint32_t TEMP = SHA1Shift(5, A) + (B ^ C ^ D) + E + W[t] + 0x6ed9eba1;
    E = D; D = C; C = SHA1Shift(30, B); B = A; A = TEMP;
  }
  for (int t = 40; t < 60; ++t) {
    uint32_t TEMP = SHA1Shift(5, A) + ((B & C) | (B & D) | (C & D)) +
                    E + W[t] + 0x8f1bbcdc;
    E = D; D = C; C = SHA1Shift(30, B); B = A; A = TEMP;
  }
  for (int t = 60; t < 80; ++t) {
    uint32_t TEMP = SHA1Shift(5, A) + (B ^ C ^ D) + E + W[t] + 0xca62c1d6;
    E = D; D = C; C = SHA1Shift(30, B); B = A; A = TEMP;
  }

  H[0] += A;
  H[1] += B;
  H[2] += C;
  H[3] += D;
  H[4] += E;
}

bool GenerateSHA1(const std::string &input, std::string *result) {
  ASSERT(result);
  if (input.size() >= 0x20000000) {
    LOG("SHA1 input is empty or too long");
    return false;
  }

  uint32_t H[] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0 };
  unsigned char M[64];
  uint32_t input_size = static_cast<uint32_t>(input.size());
  uint32_t bit_length = input_size * 8;
  const char *input_ptr = input.c_str();

  while (input_size >= 64) {
    memcpy(M, input_ptr, 64);
    input_ptr += 64;
    input_size -= 64;
    SHA1ProcessBlock(M, H);
  }

  memcpy(M, input_ptr, input_size);
  M[input_size++] = 0x80;
  if (input_size > 56) {
    memset(M + input_size, 0, 64 - input_size);
    SHA1ProcessBlock(M, H);
    input_size = 0;
  }
  memset(M + input_size, 0, 60 - input_size);
  M[60] = static_cast<unsigned char>(bit_length >> 24);
  M[61] = static_cast<unsigned char>((bit_length >> 16) & 0xff);
  M[62] = static_cast<unsigned char>((bit_length >> 8) & 0xff);
  M[63] = static_cast<unsigned char>(bit_length & 0xff);
  SHA1ProcessBlock(M, H);

  result->resize(20);
  for (int t = 0; t < 5; ++t) {
    (*result)[t*4] = static_cast<char>(H[t] >> 24);
    (*result)[t*4+1] = static_cast<char>((H[t] >> 16) & 0xff);
    (*result)[t*4+2] = static_cast<char>((H[t] >> 8) & 0xff);
    (*result)[t*4+3] = static_cast<char>(H[t] & 0xff);
  }
  return true;
}

// Standard Base64 result chars (see RFC2045).
static const char kBase64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char kWebSafeBase64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static bool EncodeBase64Internal(const std::string &input,
                                 const char *base64_chars,
                                 bool add_padding,
                                 std::string *result) {
  ASSERT(result);
  result->clear();
  size_t length = input.size();
  if (length >= 0xC0000000) {
    // This limit is big enough even in 64-bit environment.
    LOG("EncodeBase64 input is too long");
    return false;
  }

  result->reserve(add_padding ? 4 *((length + 2) / 3) : length + length / 3);
  const char *input_ptr = input.c_str();
  while (length >= 3) {
    unsigned char byte0 = static_cast<unsigned char>(*input_ptr++);
    unsigned char byte1 = static_cast<unsigned char>(*input_ptr++);
    unsigned char byte2 = static_cast<unsigned char>(*input_ptr++);
    *result += base64_chars[byte0 >> 2];
    *result += base64_chars[((byte0 << 4) & 0x3f) | (byte1 >> 4)];
    *result += base64_chars[((byte1 << 2) & 0x3f) | (byte2 >> 6)];
    *result += base64_chars[byte2 & 0x3f];
    length -= 3;
  }

  if (length > 0) {
    unsigned char byte0 = static_cast<unsigned char>(*input_ptr++);
    *result += base64_chars[byte0 >> 2];
    if (length == 1) {
      *result += base64_chars[(byte0 << 4) & 0x3f];
      if (add_padding)
        *result += "==";
    } else {
      ASSERT(length == 2);
      unsigned char byte1 = static_cast<unsigned char>(*input_ptr++);
      *result += base64_chars[((byte0 << 4) & 0x3f) | (byte1 >> 4)];
      *result += base64_chars[(byte1 << 2) & 0x3f];
      if (add_padding)
        *result += '=';
    }
  }
  return true;
}

bool EncodeBase64(const std::string &input, bool add_padding,
                  std::string *result) {
  return EncodeBase64Internal(input, kBase64Chars, add_padding, result);
}

bool WebSafeEncodeBase64(const std::string &input, bool add_padding,
                         std::string *result) {
  return EncodeBase64Internal(input, kWebSafeBase64Chars, add_padding, result);
}

// Table used to decode Base64.
static const unsigned char BD = 255; // Bad char.
static const unsigned char WS = 254; // White spaces.
static const unsigned char PD = 253; // Tail padding.
static const unsigned char kUnBase64Chars[] = {
  BD, BD, BD, BD, BD, BD, BD, BD, // 0 - 7
  BD, WS, WS, BD, BD, WS, BD, BD, // 8 - 0x0f ?,TAB,LF,?,?,CR,?,?,
  BD, BD, BD, BD, BD, BD, BD, BD, // 0x10 - 0x17
  BD, BD, BD, BD, BD, BD, BD, BD, // 0x18 - 0x1f
  WS, BD, BD, BD, BD, BD, BD, BD, // 0x20 - 0x27 SPACE,!,",#,$,%,&,'
  BD, BD, BD, 62, BD, BD, BD, 63, // 0x28 - 0x2f (,),*,+,,,-,.,/
  52, 53, 54, 55, 56, 57, 58, 59, // 0x30 - 0x37 0,1,2,3,4,5,6,7
  60, 61, BD, BD, BD, PD, BD, BD, // 0x38 - 0x3f 8,9,:,;,<,=,>,?
  BD,  0,  1,  2,  3,  4,  5,  6, // 0x40 - 0x47 @,A -->
  7,   8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22,
  23, 24, 25, BD, BD, BD, BD, BD, // 0x58 - 0x5f --> Z
  BD, 26, 27, 28, 29, 30, 31, 32, // 0x60 - 0x67 `,a -->
  33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48,
  49, 50, 51,                     // 0x77 -      --> Z
};

static const unsigned char kWebSafeUnBase64Chars[] = {
  BD, BD, BD, BD, BD, BD, BD, BD, // 0 - 7
  BD, BD, BD, BD, BD, BD, BD, BD, // 8 - 0x0f ?,TAB,LF,?,?,CR,?,?,
  BD, BD, BD, BD, BD, BD, BD, BD, // 0x10 - 0x17
  BD, BD, BD, BD, BD, BD, BD, BD, // 0x18 - 0x1f
  BD, BD, BD, BD, BD, BD, BD, BD, // 0x20 - 0x27 SPACE,!,",#,$,%,&,'
  BD, BD, BD, BD, BD, 62, BD, BD, // 0x28 - 0x2f (,),*,+,,,-,.,/
  52, 53, 54, 55, 56, 57, 58, 59, // 0x30 - 0x37 0,1,2,3,4,5,6,7
  60, 61, BD, BD, BD, PD, BD, BD, // 0x38 - 0x3f 8,9,:,;,<,=,>,?
  BD,  0,  1,  2,  3,  4,  5,  6, // 0x40 - 0x47 @,A -->
  7,   8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22,
  23, 24, 25, BD, BD, BD, BD, 63, // 0x58 - 0x5f --> Z,[,\,],^,_
  BD, 26, 27, 28, 29, 30, 31, 32, // 0x60 - 0x67 `,a -->
  33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48,
  49, 50, 51,                     // 0x77 -      --> Z
};

COMPILE_ASSERT(sizeof(kUnBase64Chars) == sizeof(kWebSafeUnBase64Chars),
               The_two_array_must_be_of_the_same_size);

bool DecodeBase64Internal(const char *input,
                          const unsigned char *unbase64_chars,
                          std::string *result) {
  ASSERT(input);
  ASSERT(result);

  bool padding_met = false;
  std::string temp_result;
  unsigned char group[4];
  int group_size = 0;
  while (true) {
    int c = *input++;
    if (!c)
      break;

    unsigned char b = (c >= 0 &&
                       c < static_cast<int>(arraysize(kUnBase64Chars))) ?
                      unbase64_chars[c] : BD;
    switch (b) {
      case BD: // Dad Base64 char.
        return false;
      case WS: // Nothing to do.
        break;
      case PD:
        padding_met = true;
        break;
      default:
        if (padding_met) {
          // Extra chars after padding.
          return false;
        }
        group[group_size++] = b;
        if (group_size == 4) {
          temp_result += static_cast<char>((group[0] << 2) | (group[1] >> 4));
          temp_result += static_cast<char>((group[1] << 4) | (group[2] >> 2));
          temp_result += static_cast<char>((group[2] << 6) | (group[3]));
          group_size = 0;
        }
        break;
    }
  }

  if (group_size > 0) {
    if (group_size == 1) {
      // This is invalid, because one single byte should be represented in
      // 2 Base64 chars.
      return false;
    }
    temp_result += static_cast<char>((group[0] << 2 | group[1] >> 4));
    if (group_size == 3)
      temp_result += static_cast<char>((group[1] << 4) | (group[2] >> 2));
  }

  result->swap(temp_result);
  return true;
}

bool DecodeBase64(const char *input, std::string *result) {
  return DecodeBase64Internal(input, kUnBase64Chars, result);
}

bool WebSafeDecodeBase64(const char *input, std::string *result) {
  return DecodeBase64Internal(input, kWebSafeUnBase64Chars, result);
}

} // namespace ggadget
