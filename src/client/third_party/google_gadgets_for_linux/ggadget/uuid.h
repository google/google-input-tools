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

#ifndef GGADGET_UUID_H__
#define GGADGET_UUID_H__

#include <string>

namespace ggadget {

/**
 * @ingroup Utilities
 * Class for generating UUID.
 */
class UUID {
 public:
  /** Initializes a null UUID. */
  UUID();

  /** Initializes a UUID from raw data. */
  explicit UUID(const unsigned char data[16]);

  /** Initializes a UUID from the string representation. */
  explicit UUID(const std::string &data);

  bool operator==(const UUID &another) const;

  /** Generate a UUID. */
  void Generate();

  /**
   * Sets the value of UUID from a string representation. If the string is
   * invalid, the function returns false and the value will not be changed.
   */
  bool SetString(const std::string &data);

  /**
   * Gets the string representation of the UUID in format like
   * xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.
   */
  std::string GetString() const;

  /**
   * Gets the raw UUID data.
   */
  void GetData(unsigned char data[16]) const;

 private:
  unsigned char data_[16];
};

} // namespace ggadget

#endif // GGADGET_UUID_H__
