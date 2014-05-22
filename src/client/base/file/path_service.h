/*
  Copyright 2014 Google Inc.

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

#ifndef GOOPY_BASE_FILE_PATH_SERVICE_H_
#define GOOPY_BASE_FILE_PATH_SERVICE_H_
#pragma once

#include <string>
#include "base/basictypes.h"

class FilePath;

// The path service is a global table mapping keys to file system paths.  It is
// OK to use this service from multiple threads.
//
class PathService {
 public:
  // Retrieves a path to a special directory or file and places it into the
  // string pointed to by 'path'. If you ask for a directory it is guaranteed
  // to NOT have a path separator at the end. For example, "c:\windows\temp"
  // Directories are also guaranteed to exist when this function succeeds.
  //
  // Returns true if the directory or file was successfully retrieved. On
  // failure, 'path' will not be changed.
  static bool Get(int key, FilePath* path);

  // Overrides the path to a special directory or file.  This cannot be used to
  // change the value of DIR_CURRENT, but that should be obvious.  Also, if the
  // path specifies a directory that does not exist, the directory will be
  // created by this method.  This method returns true if successful.
  //
  // If the given path is relative, then it will be resolved against
  // DIR_CURRENT.
  //
  // WARNING: Consumers of PathService::Get may expect paths to be constant
  // over the lifetime of the app, so this method should be used with caution.
  static bool Override(int key, const FilePath& path);

  // To extend the set of supported keys, you can register a path provider,
  // which is just a function mirroring PathService::Get.  The ProviderFunc
  // returns false if it cannot provide a non-empty path for the given key.
  // Otherwise, true is returned.
  //
  // WARNING: This function could be called on any thread from which the
  // PathService is used, so a the ProviderFunc MUST BE THREADSAFE.
  //
  typedef bool (*ProviderFunc)(int, FilePath*);

  // Call to register a path provider.  You must specify the range "[key_start,
  // key_end)" of supported path keys.
  static void RegisterProvider(ProviderFunc provider,
                               int key_start,
                               int key_end);

 private:
  static bool GetFromCache(int key, FilePath* path);
  static bool GetFromOverrides(int key, FilePath* path);
  static void AddToCache(int key, const FilePath& path);

  DISALLOW_IMPLICIT_CONSTRUCTORS(PathService);
};

#endif  // GOOPY_BASE_FILE_PATH_SERVICE_H_
