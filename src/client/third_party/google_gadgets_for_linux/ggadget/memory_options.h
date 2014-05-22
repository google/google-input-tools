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

#ifndef GGADGET_MEMORY_OPTIONS_H__
#define GGADGET_MEMORY_OPTIONS_H__

#include <map>
#include <ggadget/common.h>
#include <ggadget/signals.h>
#include <ggadget/options_interface.h>

namespace ggadget {

/**
 * @ingroup Options
 *
 * In memory implementation of OptionsInterface.
 */
class MemoryOptions : public OptionsInterface {
 public:
  /** Constructs a @c MemoryOptions instance without size limit. */
  MemoryOptions();

  /**
   * Constructs a @c MemoryOptions instance with size limit.
   * If new added value causes the total size of names and values execeeds
   * the size limit, the value will be rejected.
   */
  MemoryOptions(size_t size_limit);
  virtual ~MemoryOptions();

  virtual Connection *ConnectOnOptionChanged(
      Slot1<void, const char *> *handler);
  virtual size_t GetCount();
  virtual void Add(const char *name, const Variant &value);
  virtual bool Exists(const char *name);
  virtual Variant GetDefaultValue(const char *name);
  virtual void PutDefaultValue(const char *name, const Variant &value);
  virtual Variant GetValue(const char *name);
  virtual void PutValue(const char *name, const Variant &value);
  virtual void Remove(const char *name);
  virtual void RemoveAll();
  virtual void EncryptValue(const char *name);
  virtual bool IsEncrypted(const char *name);

  virtual Variant GetInternalValue(const char *name);
  virtual void PutInternalValue(const char *name, const Variant &value);
  virtual bool Flush();
  virtual void DeleteStorage();
  virtual bool EnumerateItems(
      Slot3<bool, const char *, const Variant &, bool> *callback);
  virtual bool EnumerateInternalItems(
      Slot2<bool, const char *, const Variant &> *callback);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(MemoryOptions);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif  // GGADGET_MEMORY_OPTIONS_H__
