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

#ifndef GOOPY_COMPONENTS_COMMON_FILE_UTILS_H_
#define GOOPY_COMPONENTS_COMMON_FILE_UTILS_H_

#include <string>
#include "base/basictypes.h"

namespace ime_goopy {

// This class provides some utility functions that can gets the predefined file
// path for different components and languages. It also provides some utility
// functions to access files. The file paths are all encoded with utf8.
class FileUtils {
 public:
  static std::string GetSystemDataPath();
  static std::string GetSystemPluginPath();
  static std::string GetDataPathForLanguage(const std::string& language);
  static std::string GetDataPathForComponent(const std::string& component);
  static std::string GetUserDataPathForComponent(const std::string& component);
  static std::string GetDataPathForComponentAndLanguage(
      const std::string& components,
      const std::string& language);
  static std::string GetUserDataPath();
  static std::string GetBinaryPath();
  static std::string GetComponentBinaryPath();
  // File content will be saved in |content| which is a binary char arrary.
  // Only uses std::string::Size() to get the size of file content.
  static bool ReadFileContent(const std::string& path, std::string* content);
  static bool CreateDirectoryRecuisively(const std::string& dir);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(FileUtils);
};

}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_COMMON_FILE_UTILS_H_
