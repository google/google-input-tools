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

#ifndef GGADGET_SCRIPTABLE_FILE_SYSTEM_H__
#define GGADGET_SCRIPTABLE_FILE_SYSTEM_H__

#include <ggadget/scriptable_helper.h>

namespace ggadget {

class GadgetInterface;

namespace framework {
class FileSystemInterface;

/**
 * @ingroup ScriptableObjects
 *
 * Scriptable counterpart of FileSystemInterface.
 *
 * Please note that ScriptableFileSystem is not NativeOwned, because it's
 * bound to a Gadget instance. Different Gadget must use different
 * ScriptableFileSystem instance.
 *
 * A framework extension must create a new ScriptableFileSystem instance and
 * attach it to the specified framework instance when its
 * RegisterFrameworkExtension() function is called each time.
 * So the ScriptableFileSystem object will be destroyed correctly when the
 * associated framework instance is destroyed by the corresponding gadget.
 *
 * While each Gadget has its own ScriptableFileSystem object, all
 * ScriptableFileSystem objects can share one FileSystemInterface instance.
 * So the specified filesystem pointer can point to a static allocated
 * FileSystemInterface instance. ScriptableFileSystem object will never delete
 * the filesystem pointer.
 */
class ScriptableFileSystem : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x881b7d66c6bf4ca5, ScriptableInterface)

  explicit ScriptableFileSystem(FileSystemInterface *filesystem,
                                GadgetInterface *gadget);
  virtual ~ScriptableFileSystem();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableFileSystem);
  class Impl;
  Impl *impl_;
};

} // namespace framework
} // namespace ggadget

#endif  // GGADGET_SCRIPTABLE_FILE_SYSTEM_H__
