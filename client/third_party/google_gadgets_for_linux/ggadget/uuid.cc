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

#include "uuid.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "string_utils.h"

namespace ggadget {

#define UUID_FORMAT "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-" \
                    "%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx"

UUID::UUID() {
  memset(data_, 0, 16);
}

UUID::UUID(const unsigned char data[16]) {
  memcpy(data_, data, 16);
}

UUID::UUID(const std::string &data) {
  if (!SetString(data))
    memset(data_, 0, 16);
}

bool UUID::operator==(const UUID &another) const {
  return memcmp(data_, another.data_, 16) == 0;
}

// RFC 4122 4.4
void UUID::Generate() {
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd >= 0) {
    unsigned char *p = data_;
    int bytes_left = 16;
    int fail_count = 0;
    while (bytes_left > 0 && fail_count < 10) {
      int bytes_read = static_cast<int>(read(fd, p, bytes_left));
      if (bytes_read <= 0) {
        fail_count++;
      } else {
        bytes_left -= bytes_read;
        p += bytes_read;
      }
    }
    close(fd);
  }

  struct timeval tv;
  gettimeofday(&tv, NULL);
  srand(static_cast<unsigned int>(tv.tv_sec ^ tv.tv_usec));
  for (int i = 0; i < 16; i++) {
    data_[i] = static_cast<unsigned char>(data_[i] ^ (rand() >> 6) ^
                                          (rand() >> 14));
  }

  // clock_seq_hi_and_reserved
  data_[8] = static_cast<unsigned char>((data_[8] & 0x3f) | 0x80);
  // The high byte of time_hi_and_version
  data_[6] = static_cast<unsigned char>((data_[6] & 0x0f) | 0x40);
}

void UUID::GetData(unsigned char data[16]) const {
  memcpy(data, data_, 16);
}

bool UUID::SetString(const std::string &data) {
  unsigned char temp[16];
  if (sscanf(data.c_str(), UUID_FORMAT, temp, temp + 1, temp + 2, temp + 3,
             temp + 4, temp + 5, temp + 6, temp + 7, temp + 8, temp + 9,
             temp + 10, temp + 11, temp + 12, temp + 13, temp + 14,
             temp + 15) == 16) {
    memcpy(data_, temp, 16);
    return true;
  }
  return false;
}

std::string UUID::GetString() const {
  return StringPrintf(UUID_FORMAT, data_[0], data_[1], data_[2], data_[3],
                      data_[4], data_[5], data_[6], data_[7], data_[8],
                      data_[9], data_[10], data_[11], data_[12], data_[13],
                      data_[14], data_[15]);
}

} // namespace ggadget
