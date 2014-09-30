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

#ifndef GGADGET_SCRIPTABLE_BINARY_DATA_H__
#define GGADGET_SCRIPTABLE_BINARY_DATA_H__

#include <string>
#include <ggadget/scriptable_helper.h>

namespace ggadget {

/**
 * @ingroup ScriptableObjects
 *
 * This class is used to transfer native binary data opaquely through
 * script code. It doesn't expose any property or method to script.
 */
class ScriptableBinaryData : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x381e0cd617734500, ScriptableInterface);

  ScriptableBinaryData(const char *data, size_t size)
      : data_(data ? data : "", data ? size : 0) { }

  explicit ScriptableBinaryData(const std::string &data)
      : data_(data) { }

  ScriptableBinaryData(const ScriptableBinaryData &data)
      : ScriptableHelperDefault(),
        data_(data.data_) { }

 protected:
  virtual ~ScriptableBinaryData() { }

  virtual void DoClassRegister() {
    RegisterProperty("size", NewSlot(&ScriptableBinaryData::size), NULL);
    RegisterMethod("getByte", NewSlot(&ScriptableBinaryData::GetByte));
  }

 public:
  int GetByte(size_t i) const {
    return i < data_.size() ? data_[i] : 0;
  }
  std::string data() const { return data_; }
  size_t size() const { return data_.size(); }

 private:
  std::string data_;
};

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_BINARY_DATA_H__
