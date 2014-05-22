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

#include <cstdlib>
#include <time.h>

#include "encryptor_interface.h"
#include "common.h"

namespace ggadget {
namespace {

// A very weak encryptor.
class SimpleEncryptor : public EncryptorInterface {
 public:
  virtual void Encrypt(const std::string &input, std::string *output) {
    ASSERT(output);
    output->clear();

    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand(static_cast<unsigned int>(rand() + tv.tv_usec));
    char salt1 = static_cast<char>(rand());
    char salt2 = static_cast<char>(rand());
    output->append(1, salt1);
    output->append(1, salt2);

    int x = Compute(salt1, salt2, input.c_str(), input.size(), output);
    output->append(1, static_cast<char>(x));
  }

  virtual bool Decrypt(const std::string &input, std::string *output) {
    ASSERT(output);
    output->clear();

    // Input should have at least 3 chars: 2 salts, 1 checksum.
    if (input.size() < 3)
      return false;

    int x = Compute(input[0], input[1], input.c_str() + 2, input.size() - 3,
                    output);
    if (input[input.size() - 1] != static_cast<char>(x)) {
      // Checksum doesn't match.
      output->clear();
      return false;
    }
    return true;
  }

  int Compute(char salt1, char salt2, const char *input, size_t input_size,
              std::string *output) {
    int x = salt1 * 30103 + salt2 * 70607;
    for (size_t i = 0; i < input_size; i++) {
      output->append(1, static_cast<char>(input[i] ^ (x >> 16)));
      x = x * 275604541 + 15485863;
    }
    return x;
  }
};

static EncryptorInterface *g_encryptor = NULL;
static SimpleEncryptor g_default_encryptor;

} // anonymous namespace

bool SetEncryptor(EncryptorInterface *encryptor) {
  ASSERT(!g_encryptor && encryptor);
  if (!g_encryptor && encryptor) {
    g_encryptor = encryptor;
    return true;
  }
  return false;
}

EncryptorInterface *GetEncryptor() {
  return g_encryptor ? g_encryptor : &g_default_encryptor;
}

} // namespace ggadget
