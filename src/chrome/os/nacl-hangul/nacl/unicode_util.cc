/*
 * Copyright 2013 Google Inc. All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <cstdlib>
#include <iostream>

#include "unicode_util.h"

namespace {


const int kInitialBufferSize = 32;

inline int BitMask(int length) {
  return (1 << length) - 1;
}

inline int GetBit(int byte, int pos) {
  return (byte >> pos) & 1;
}

}  // namespace

namespace unicode_util {

string Ucs4ToUtf8(const ucschar *ucs4, size_t length) {
  if (length == 0) length = static_cast<size_t>(-1);
  size_t i;
  for (i = 0; i < length && ucs4[i] != 0; i++) {}
  length = i;
  size_t freesize = kInitialBufferSize;
  char *utf8 = static_cast<char *>(malloc(sizeof(char) * freesize));
  char *putf8 = utf8;
  for (i = 0; i < length; i++) {
    if (static_cast<int>(freesize) - 6 <= 0) {
      freesize = putf8 - utf8;
      utf8 = static_cast<char *>(
          realloc(utf8,
          sizeof(char) * (freesize + freesize)));
      putf8 = utf8 + freesize;
    }
    ucschar c = ucs4[i];
    ucschar byte[4] = {
      (c >> 0) & BitMask(8), (c >> 8) & BitMask(8),
      (c >> 16) & BitMask(8), (c >> 24) & BitMask(8)
    };
    size_t delta = 0;
    if (c <= 0x7F) {
      /* U-00000000 - U-0000007F */
      /* 0xxxxxxx */
      putf8[0] = byte[0] & BitMask(7);
      delta = 1;
    } else if (c <= 0x7FF) {
      /* U-00000080 - U-000007FF */
      /* 110xxxxx 10xxxxxx */
      putf8[1] = 0x80 + (byte[0] & BitMask(6));
      putf8[0] = 0xC0 + ((byte[0] >> 6) & BitMask(2)) +
          ((byte[1] & BitMask(3)) << 2);
      delta = 2;
    } else if (c <= 0xFFFF) {
      /* U-00000800 - U-0000FFFF */
      /* 1110xxxx 10xxxxxx 10xxxxxx */
      putf8[2] = 0x80 + (byte[0] & BitMask(6));
      putf8[1] = 0x80 + ((byte[0] >> 6) & BitMask(2)) +
          ((byte[1] & BitMask(4)) << 2);
      putf8[0] = 0xE0 + ((byte[1] >> 4) & BitMask(4));
      delta = 3;
    } else if (c <= 0x1FFFFF) {
      /* U-00010000 - U-001FFFFF */
      /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
      putf8[3] = 0x80 + (byte[0] & BitMask(6));
      putf8[2] = 0x80 + ((byte[0] >> 6) & BitMask(2)) +
          ((byte[1] & BitMask(4)) << 2);
      putf8[1] = 0x80 + ((byte[1] >> 4) & BitMask(4)) +
          ((byte[2] & BitMask(2)) << 4);
      putf8[0] = 0xF0 + ((byte[2] >> 2) & BitMask(3));
      delta = 4;
    } else if (c <= 0x3FFFFFF) {
      /* U-00200000 - U-03FFFFFF */
      /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
      putf8[4] = 0x80 + (byte[0] & BitMask(6));
      putf8[3] = 0x80 + ((byte[0] >> 6) & BitMask(2)) +
          ((byte[1] & BitMask(4)) << 2);
      putf8[2] = 0x80 + ((byte[1] >> 4) & BitMask(4)) +
          ((byte[2] & BitMask(2)) << 4);
      putf8[1] = 0x80 + ((byte[2] >> 2) & BitMask(6));
      putf8[0] = 0xF8 + (byte[3] & BitMask(2));
      delta = 5;
    } else if (c <= 0x7FFFFFFF) {
      /* U-04000000 - U-7FFFFFFF */
      /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
      putf8[5] = 0x80 + (byte[0] & BitMask(6));
      putf8[4] = 0x80 + ((byte[0] >> 6) & BitMask(2)) +
          ((byte[1] & BitMask(4)) << 2);
      putf8[3] = 0x80 + ((byte[1] >> 4) & BitMask(4)) +
          ((byte[2] & BitMask(2)) << 4);
      putf8[2] = 0x80 + ((byte[2] >> 2) & BitMask(6));
      putf8[1] = 0x80 + (byte[3] & BitMask(6));
      putf8[0] = 0xFC + ((byte[3] >> 6) & BitMask(1));
      delta = 6;
    } else {
      free(utf8);
      std::cerr << "Invalid UCS4 char" << c << std::endl;
      return "";
    }
    putf8 += delta;
    freesize -= delta;
  }
  length = (putf8 - utf8 + 1);
  utf8[length - 1] = '\0';
  string str(utf8);
  free(utf8);
  return str;
}

UCSString Utf8ToUcs4(const char* utf8, size_t length) {
  if (length == 0) {
    length = (size_t) -1;
  }
  size_t i;
  for (i = 0; i < length && utf8[i] != '\0'; i++) {}
  length = i;
  size_t freesize = kInitialBufferSize;
  UCSString retval;
  ucschar* ucs4 = static_cast<ucschar*>(malloc(sizeof(ucschar) * freesize));
  ucschar* pucs4 = ucs4;
  for (i = 0; i < length; i ++) {
    ucschar byte[4] = {0};
    if (GetBit(utf8[i], 7) == 0) {
      /* U-00000000 - U-0000007F */
      /* 0xxxxxxx */
      byte[0] = utf8[i] & BitMask(7);
    } else if (GetBit(utf8[i], 5) == 0) {
      /* U-00000080 - U-000007FF */
      /* 110xxxxx 10xxxxxx */
      if (i + 1 >= length) {
        goto err;
      }
      byte[0] = (utf8[i + 1] & BitMask(6)) +
          ((utf8[i] & BitMask(2)) << 6);
      byte[1] = (utf8[i] >> 2) & BitMask(3);
      i += 1;
    } else if (GetBit(utf8[i], 4) == 0) {
      /* U-00000800 - U-0000FFFF */
      /* 1110xxxx 10xxxxxx 10xxxxxx */
      if (i + 2 >= length) {
        goto err;
      }
      byte[0] = (utf8[i + 2] & BitMask(6)) +
          ((utf8[i + 1] & BitMask(2)) << 6);
      byte[1] = ((utf8[i + 1] >> 2) & BitMask(4))
          + ((utf8[i] & BitMask(4)) << 4);
      i += 2;
    } else if (GetBit(utf8[i], 3) == 0) {
      /* U-00010000 - U-001FFFFF */
      /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
      if (i + 3 >= length) {
        goto err;
      }
      byte[0] = (utf8[i + 3] & BitMask(6)) +
          ((utf8[i + 2] & BitMask(2)) << 6);
      byte[1] = ((utf8[i + 2] >> 2) & BitMask(4)) +
          ((utf8[i + 1] & BitMask(4)) << 4);
      byte[2] = ((utf8[i + 1] >> 4) & BitMask(2)) +
          ((utf8[i] & BitMask(3)) << 2);
      i += 3;
    } else if (GetBit(utf8[i], 2) == 0) {
      /* U-00200000 - U-03FFFFFF */
      /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
      if (i + 4 >= length) {
        goto err;
      }
      byte[0] = (utf8[i + 4] & BitMask(6)) +
          ((utf8[i + 3] & BitMask(2)) << 6);
      byte[1] = ((utf8[i + 3] >> 2) & BitMask(4)) +
          ((utf8[i + 2] & BitMask(4)) << 4);
      byte[2] = ((utf8[i + 2] >> 4) & BitMask(2)) +
          ((utf8[i + 1] & BitMask(6)) << 2);
      byte[3] = utf8[i] & BitMask(2);
      i += 4;
    } else if (GetBit(utf8[i], 1) == 0) {
      /* U-04000000 - U-7FFFFFFF */
      /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
      if (i + 5 >= length) {
        goto err;
      }
      byte[0] = (utf8[i + 5] & BitMask(6)) +
          ((utf8[i + 4] & BitMask(2)) << 6);
      byte[1] = ((utf8[i + 4] >> 2) & BitMask(4)) +
          ((utf8[i + 3] & BitMask(4)) << 4);
      byte[2] = ((utf8[i + 3] >> 4) & BitMask(2)) +
          ((utf8[i + 2] & BitMask(6)) << 2);
      byte[3] = (utf8[i + 1] & BitMask(6)) +
          ((utf8[i] & BitMask(1)) << 6);
      i += 5;
    } else {
      goto err;
    }
    if (freesize == 0) {
      freesize = pucs4 - ucs4;
      ucs4 = static_cast<ucschar*>(
          realloc(ucs4, sizeof(ucschar) * (freesize + freesize)));
      pucs4 = ucs4 + freesize;
    }
    *pucs4 = (byte[3] << 24) + (byte[2] << 16) + (byte[1] << 8) + byte[0];
    pucs4++;
    freesize--;
  }
  length = (pucs4 - ucs4 + 1);
  ucs4 = static_cast<ucschar*>(realloc(ucs4, sizeof(ucschar) * length));
  ucs4[length - 1] = 0;
  retval = ucs4;
  free(ucs4);
  return retval;
err:
  free(ucs4);
  return UCSString();
}

}  // namespace unicode_util
