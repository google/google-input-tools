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

#ifndef GGADGET_ENCRYPTOR_INTERFACE_H__
#define GGADGET_ENCRYPTOR_INTERFACE_H__

#include <string>

namespace ggadget {

/**
 * @ingroup Interfaces
 * Interface class for real encryptor implementations.
 */
class EncryptorInterface {
 public:
  /**
   * Encrypts a string.
   * @param input the input string to encrypt. The string may contain arbitrary
   *     binary data, including '\0's.
   * @param output[out] the encrypted result. The string may contain arbitrary
   *     binary data, including '\0's.
   */
  virtual void Encrypt(const std::string &input, std::string *output) = 0;

  /**
   * Decrypts a string.
   * @param input the input string to deccrypt. The string may contain arbitrary
   *     binary data, including '\0's.
   * @param output[out] the decrypted result. The string may contain arbitrary
   *     binary data, including '\0's.
   * @return true on success.
   */
  virtual bool Decrypt(const std::string &input, std::string *output) = 0;

 protected:
  virtual ~EncryptorInterface() { }
};

/**
 * @relates EncryptorInterface
 * Sets the global EncryptorInterface instance. An Encryptor extension module
 * can call this function in its @c Initailize() function.
 */
bool SetEncryptor(EncryptorInterface *encryptor);

/**
 * @relates EncryptorInterface
 * Gets the global EncryptorInterface instance. Different from other global
 * instances, this instance has a default one if no SetEncryptor() is called.
 */
EncryptorInterface *GetEncryptor();

} // namespace ggadget

#endif // GGADGET_ENCRYPTOR_INTERFACE_H__
